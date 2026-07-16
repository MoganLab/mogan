/** \file loro_shadow.cpp
 *  \copyright GPLv3
 *  \details Phase 2 shadow LoroDoc 实现：seed（树->live doc + 身份表）、export、to_tree。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_shadow.hpp"

#ifdef LORO_ENABLED

#include "basic.hpp"       // pointer, hash(pointer)
#include "loro_ir.hpp"     // loro_ir_decode, loro_ir_to_tree, LORO_* kind
#include "tree_helper.hpp" // L, as_string, inside
#include <moebius/tree_label.hpp> // make_tree_label

#include <cstdint>

// tree_rep* 的 hash（hashmap key 用），仿 observer::hash((pointer)rep)
inline int
hash (tree_rep* p) {
  return hash ((pointer) p);
}

/******************************************************************************
 * 构造与析构
 ******************************************************************************/

loro_shadow_rep::loro_shadow_rep ()
    : doc (mogan_loro_doc_new ()), id_map (mogan_tree_id{0, 0}), _update_cb (nullptr), _update_user_data (nullptr) {}

loro_shadow_rep::~loro_shadow_rep () {
  if (doc) mogan_loro_doc_free (doc);
}

loro_shadow::loro_shadow () : rep (tm_new<loro_shadow_rep> ()) {}

/******************************************************************************
 * seed：递归构建 live doc + 填充身份表
 ******************************************************************************/

mogan_tree_id
loro_shadow_rep::seed_node (tree t, mogan_tree_id parent, uint32_t index) {
  uint8_t kind;
  string  label;
  if (is_atomic (t)) kind= LORO_ATOMIC;
  else if (is_compound (t)) {
    kind = LORO_COMPOUND;
    label= as_string (L (t));
  }
  else {
    kind = LORO_GENERIC;
    label= "generic:" * as_string ((int) L (t));
  }

  const uint8_t* lp=
      N (label) > 0 ? reinterpret_cast<const uint8_t*> (label.begin ()) : nullptr;
  mogan_tree_id id=
      mogan_loro_node_create (doc, parent, index, kind, lp, (size_t) N (label));
  id_map (inside (t))= id; // 记录身份

  if (is_atomic (t)) {
    const uint8_t* tp= reinterpret_cast<const uint8_t*> (t->label.begin ());
    size_t         tn= (size_t) N (t->label);
    // 先尝试 LoroText（合法 UTF-8）；失败（非 UTF-8，如图片）回退 Binary
    if (mogan_loro_node_text_insert (doc, id, 0, tp, tn) != 0)
      mogan_loro_node_set_binary (doc, id, tp, tn);
  }
  else {
    int n= N (t);
    for (int i= 0; i < n; i++)
      seed_node (t[i], id, (uint32_t) i);
  }
  return id;
}

void
loro_shadow_rep::seed (tree root) {
  mogan_tree_id root_parent= {UINT64_MAX, 0}; // Root 哨兵
  seed_node (root, root_parent, 0);
}

/******************************************************************************
 * mirror_mod：把 modification 镜像到 live doc
 ******************************************************************************/

void
loro_shadow_rep::mirror_mod (tree doc_root, modification mod) {
  bool mirrored= false;
  // 精确：文本原子的 INSERT/REMOVE（原子 rep 在敲字时稳定，身份表直接命中）
  if (mod->k == MOD_INSERT || mod->k == MOD_REMOVE) {
    path parent_path= root (mod);
    if (has_subtree (doc_root, parent_path)) {
      tree& parent= subtree (doc_root, parent_path);
      if (is_atomic (parent) && id_map->contains (inside (parent))) {
        mogan_tree_id id= id_map (inside (parent));
        if (mod->k == MOD_INSERT) {
          string s= mod->t->label; // 插入的文本（通常单字）
          mogan_loro_node_text_insert (doc, id, (uint32_t) index (mod),
              reinterpret_cast<const uint8_t*> (s.begin ()), (size_t) N (s));
        }
        else {
          mogan_loro_node_text_delete (doc, id, (uint32_t) index (mod),
              (uint32_t) argument (mod));
        }
        mirrored= true;
      }
    }
  }
  // 兜底：其余 mod（结构、SPLIT/JOIN、ASSIGN_NODE、INSERT_NODE/REMOVE_NODE 等）
  //       暂整树重 seed，保证 shadow 与 buffer 一致；后续逐步精确化
  if (!mirrored) {
    if (doc) mogan_loro_doc_free (doc);
    doc   = mogan_loro_doc_new ();
    if (_update_cb) {
      mogan_loro_doc_on_local_update(doc, _update_cb, _update_user_data);
    }
    id_map= hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
    seed (doc_root);
  }
  // 显式提交：auto_commit 是延迟的，这里同步提交以触发 local-update 事件（增量同步）。
  mogan_loro_doc_commit (doc);
}

/******************************************************************************
 * 导出 / 读取
 ******************************************************************************/

string
loro_shadow_rep::export_snapshot () {
  uint8_t* out   = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_export (doc, &out, &out_len) != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return string ();
  }
  string s ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return s;
}

bool
loro_shadow_rep::import_data (string bytes) {
  return mogan_loro_doc_import (doc, reinterpret_cast<const uint8_t*> (bytes.begin ()),
                                (size_t) N (bytes)) == 0;
}

void
loro_shadow_rep::on_local_update (mogan_local_update_cb cb, void* user_data) {
  _update_cb = cb;
  _update_user_data = user_data;
  mogan_loro_doc_on_local_update (doc, cb, user_data);
}

// 字符级 diff：把 before 文本变到 after 文本所需的 INSERT/REMOVE mod（前缀/后缀对齐）。
// p 是该原子在 buffer 中的路径；mod 用 mod_insert/remove(p, pos, ...)。
namespace {
void
emit_text_diff (path p, string b, string a, list<modification>& mods) {
  int bn= N (b), an= N (a);
  int pre= 0;
  while (pre < bn && pre < an && b[pre] == a[pre])
    pre++;
  int suf= 0;
  while (suf < bn - pre && suf < an - pre && b[bn - 1 - suf] == a[an - 1 - suf])
    suf++;
  int rm_len= bn - pre - suf;  // before 中被删的长度
  int ins_len= an - pre - suf; // after 中新增的长度
  if (rm_len > 0) mods= mods * mod_remove (p, pre, rm_len);
  if (ins_len > 0) {
    string ins= a (pre, pre + ins_len);
    mods= mods * mod_insert (p, pre, tree (ins));
  }
}

// 并行遍历 before/after（假设同结构）：文本差异 → 字符 diff mod；结构差异 → 返回 false。
bool
diff_walk (tree b, tree a, path base, list<modification>& mods) {
  if (is_atomic (b) && is_atomic (a)) {
    if (b->label != a->label) emit_text_diff (base, b->label, a->label, mods);
    return true;
  }
  if (is_compound (b) && is_compound (a) && N (b) == N (a) && L (b) == L (a)) {
    for (int i= 0; i < N (b); i++)
      if (!diff_walk (b[i], a[i], base * path (i), mods)) return false;
    return true;
  }
  return false; // 结构差异（节点数/op 不同，或类型不同）
}
} // namespace

list<modification>
loro_shadow_rep::diff_from_current (tree buffer) {
  list<modification> mods;
  tree               after= to_tree ();
  if (!diff_walk (buffer, after, path (), mods)) {
    // 结构差异：整树替换（coarse 兜底）
    mods= list<modification> ();
    mods= mods * mod_assign (path (), after);
  }
  return mods;
}

list<modification>
loro_shadow_rep::remote_diff_mods (string bytes, tree buffer) {
  if (!import_data (bytes)) return list<modification> ();
  return diff_from_current (buffer);
}

// 解码"带 TreeID 的增强 IR"：建 moebius 树 + 填 id_map（rep* -> 导入的 TreeID）。
// 格式：node := peer:u64 counter:i32 kind:u8 label:text text:text nchildren:u32 children
namespace {
tree
decode_id_node (string& b, int& pos, hashmap<tree_rep*, mogan_tree_id>& id_map) {
  auto get_u32= [&] () -> uint32_t {
    uint32_t v= (uint32_t) (unsigned char) b[pos]
              | ((uint32_t) (unsigned char) b[pos + 1] << 8)
              | ((uint32_t) (unsigned char) b[pos + 2] << 16)
              | ((uint32_t) (unsigned char) b[pos + 3] << 24);
    pos += 4;
    return v;
  };
  auto get_str= [&] () -> string {
    uint32_t n= get_u32 ();
    string   r;
    for (uint32_t i= 0; i < n; i++)
      r << b[pos + i];
    pos += n;
    return r;
  };
  // TreeID
  uint64_t peer= 0;
  for (int i= 0; i < 8; i++)
    peer |= ((uint64_t) (unsigned char) b[pos + i]) << (8 * i);
  pos += 8;
  mogan_tree_id tid { peer, (int32_t) get_u32 () };

  uint8_t kind= (uint8_t) (unsigned char) b[pos++];
  string  label= get_str ();
  string  text = get_str ();
  uint32_t n   = get_u32 ();

  tree r;
  if (kind == LORO_ATOMIC) r= tree (text);
  else {
    int op= (kind == LORO_COMPOUND) ? (int) moebius::make_tree_label (label)
                                    : as_int (label (8, N (label)));
    r= tree (op, (int) n);
    for (uint32_t i= 0; i < n; i++)
      r[i]= decode_id_node (b, pos, id_map);
  }
  id_map (inside (r))= tid;
  return r;
}
} // namespace

bool
loro_shadow_rep::import_and_build (string bytes, tree& out_buffer) {
  if (!import_data (bytes)) return false;
  uint8_t* out   = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_to_ir_with_ids (doc, &out, &out_len) != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return false;
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  id_map= hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0}); // 重置后由 decode 填充
  int   pos= 0;
  out_buffer= decode_id_node (ir_bytes, pos, id_map);
  return true;
}

tree
loro_shadow_rep::to_tree () {
  uint8_t* out   = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_to_ir (doc, &out, &out_len) != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return tree ("");
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return loro_ir_to_tree (loro_ir_decode (ir_bytes));
}

bool
loro_shadow_rep::has_id (tree t) {
  return id_map->contains (inside (t));
}

mogan_tree_id
loro_shadow_rep::get_id (tree t) {
  return id_map->contains (inside (t)) ? id_map (inside (t)) : mogan_tree_id{0, 0};
}

#endif // LORO_ENABLED
