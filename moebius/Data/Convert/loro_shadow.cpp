/** \file loro_shadow.cpp
 *  \copyright GPLv3
 *  \details Phase 2 shadow LoroDoc 实现：seed（树->live doc +
 * 身份表）、export、to_tree。
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
    : doc (mogan_loro_doc_new ()), id_map (mogan_tree_id{0, 0}),
      root_id (mogan_tree_id{0, 0}), _update_cb (nullptr),
      _update_user_data (nullptr) {}

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

  const uint8_t* lp= N (label) > 0
                         ? reinterpret_cast<const uint8_t*> (label.begin ())
                         : nullptr;
  mogan_tree_id  id=
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
  root_id                  = seed_node (root, root_parent, 0);
  // 立即提交 seed：让文本容器在首次编辑前完整落地。否则 seed 与首次编辑会同
  // 处一个 commit，接收端合并双根时某棵根的原子文本可能缺这次编辑（竞态）。
  mogan_loro_doc_commit (doc);
}

// 并行遍历 buffer 和增强 IR（shadow 的当前状态），把 buffer 的 rep 关联到
// shadow 的 TreeID。 如果结构匹配 → 填充 id_map + root_id，返回 true。否则返回
// false（调用方 fallback 到 seed）。
namespace {
bool
sync_walk (tree t, string& ir, int& pos, mogan_tree_id& root_id,
           hashmap<tree_rep*, mogan_tree_id>& id_map) {
  if (pos + 12 > N (ir)) return false;
  auto get_u32= [&] () -> uint32_t {
    uint32_t v= (uint32_t) (unsigned char) ir[pos] |
                ((uint32_t) (unsigned char) ir[pos + 1] << 8) |
                ((uint32_t) (unsigned char) ir[pos + 2] << 16) |
                ((uint32_t) (unsigned char) ir[pos + 3] << 24);
    pos+= 4;
    return v;
  };
  auto get_str= [&] () -> string {
    uint32_t n= get_u32 ();
    string   r;
    for (uint32_t i= 0; i < n; i++)
      r << ir[pos + i];
    pos+= n;
    return r;
  };
  // TreeID
  uint64_t peer= 0;
  for (int b= 0; b < 8; b++)
    peer|= ((uint64_t) (unsigned char) ir[pos + b]) << (8 * b);
  pos+= 8;
  mogan_tree_id tid{peer, (int32_t) get_u32 ()};
  if (root_id.peer == 0) root_id= tid;
  // kind/label/text/n_children
  pos++;                           // kind
  get_str ();                      // label
  get_str ();                      // text
  uint32_t n         = get_u32 (); // n_children
  id_map (inside (t))= tid;
  int nc             = is_atomic (t) ? 0 : N (t);
  if (nc != (int) n) return false; // 结构不匹配
  for (int i= 0; i < nc; i++)
    if (!sync_walk (t[i], ir, pos, root_id, id_map)) return false;
  return true;
}
} // namespace

bool
loro_shadow_rep::sync_id_map_from_shadow (tree buffer) {
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_to_ir_with_ids (doc, &out, &out_len) != 0 ||
      out == nullptr || out_len == 0) {
    if (out) mogan_loro_free (out, out_len);
    return false; // shadow 为空
  }
  string ir ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  id_map = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  root_id= mogan_tree_id{0, 0};
  int pos= 0;
  if (!sync_walk (buffer, ir, pos, root_id, id_map)) return false;
  return root_id.peer != 0;
}

/******************************************************************************
 * mirror_mod：把 modification 镜像到 live doc（增量 op，消除整树重 seed）
 ******************************************************************************/

array<mogan_tree_id>
loro_shadow_rep::node_children (mogan_tree_id parent) {
  array<mogan_tree_id> kids;
  uint8_t*             out    = nullptr;
  size_t               out_len= 0;
  if (mogan_loro_node_children (doc, parent, &out, &out_len) == 0 &&
      out != nullptr) {
    size_t n= out_len / 12;
    for (size_t i= 0; i < n; i++) {
      uint64_t peer= 0;
      for (int b= 0; b < 8; b++)
        peer|= ((uint64_t) out[i * 12 + b]) << (8 * b);
      int32_t counter= (int32_t) ((uint32_t) out[i * 12 + 8] |
                                  ((uint32_t) out[i * 12 + 9] << 8) |
                                  ((uint32_t) out[i * 12 + 10] << 16) |
                                  ((uint32_t) out[i * 12 + 11] << 24));
      kids << mogan_tree_id{peer, counter};
    }
    mogan_loro_free (out, out_len);
  }
  return kids;
}

void
loro_shadow_rep::mirror_mod (tree doc_root, modification mod) {
  bool mirrored= false;
  path rp_mod  = root (mod);

  if (mod->k == MOD_INSERT || mod->k == MOD_REMOVE) {
    if (has_subtree (doc_root, rp_mod)) {
      tree& parent= subtree (doc_root, rp_mod);
      if (is_atomic (parent) && id_map->contains (inside (parent))) {
        // 文本原子 INSERT/REMOVE → LoroText（敲字/退格，rep 稳定）
        mogan_tree_id id= id_map (inside (parent));
        if (mod->k == MOD_INSERT) {
          string s= mod->t->label;
          mogan_loro_node_text_insert (
              doc, id, (uint32_t) index (mod),
              reinterpret_cast<const uint8_t*> (s.begin ()), (size_t) N (s));
        }
        else {
          mogan_loro_node_text_delete (doc, id, (uint32_t) index (mod),
                                       (uint32_t) argument (mod));
        }
        mirrored= true;
      }
      else if (is_compound (parent) && id_map->contains (inside (parent))) {
        // 复合子节点 INSERT/REMOVE → node_create / node_delete（块增删）
        mogan_tree_id pid= id_map (inside (parent));
        if (mod->k == MOD_INSERT) {
          int pos= index (mod);
          // mod->t 可能含多个子节点（如粘贴多行=多个 para），逐个 seed 到
          // 父节点 pos 起的连续位置（post-apply 后它们在 buffer 的
          // rp_mod*(pos+i)）。
          int nr= is_compound (mod->t) ? N (mod->t) : 1;
          for (int i= 0; i < nr; i++) {
            path child_path= rp_mod * (pos + i);
            if (has_subtree (doc_root, child_path)) {
              tree& child= subtree (doc_root, child_path);
              seed_node (child, pid, (uint32_t) (pos + i)); // 建子树+映射身份
            }
          }
          mirrored= true;
        }
        else {
          int                  pos= index (mod);
          int                  nr = argument (mod);
          array<mogan_tree_id> kids=
              node_children (pid); // LoroDoc 当前子节点（镜像前状态）
          for (int j= pos + nr - 1; j >= pos; j--)
            if (j < N (kids)) mogan_loro_node_delete (doc, kids[j]);
          mirrored= true;
        }
      }
    }
  }
  else if (mod->k == MOD_ASSIGN_NODE) {
    // 改标签（格式/样式变更）→ node_set_label（rep 稳定）
    if (has_subtree (doc_root, rp_mod)) {
      tree& node= subtree (doc_root, rp_mod);
      if (id_map->contains (inside (node))) {
        mogan_tree_id id = id_map (inside (node));
        string        lab= as_string (L (mod)); // 新 op 名
        mogan_loro_node_set_label (
            doc, id, reinterpret_cast<const uint8_t*> (lab.begin ()),
            (size_t) N (lab));
        mirrored= true;
      }
    }
  }
  else if (mod->k == MOD_SPLIT) {
    // 拆分：P=pos 的子节点 X 拆成 t1(P[pos]) + t2(P[pos+1])
    int pos= index (mod);
    int at = argument (mod);
    if (has_subtree (doc_root, rp_mod) &&
        id_map->contains (inside (subtree (doc_root, rp_mod)))) {
      mogan_tree_id        pid = id_map (inside (subtree (doc_root, rp_mod)));
      array<mogan_tree_id> kids= node_children (pid);
      if (pos < N (kids)) {
        mogan_tree_id x_id= kids[pos];
        // raw_split 把 ref[pos] 替换成新树 t1（左半）+ t2（右半）。Loro 里 X
        // 截断/留前半 即为 t1，故把 buffer 的 t1（在 rp_mod*pos）映射到 X 的
        // TreeID，供后续 mod 解析。
        if (has_subtree (doc_root, rp_mod * pos))
          id_map (inside (subtree (doc_root, rp_mod * pos)))= x_id;
        path t2p= rp_mod * (pos + 1);
        if (has_subtree (doc_root, t2p)) {
          tree& t2= subtree (doc_root, t2p);
          if (is_atomic (t2)) {
            // 原子拆分：截断 X 的文本到 at，建 t2（原子）放 t2 的文本
            mogan_loro_node_text_delete (doc, x_id, (uint32_t) at,
                                         (uint32_t) N (t2->label));
            mogan_tree_id y_id= mogan_loro_node_create (
                doc, pid, (uint32_t) (pos + 1), LORO_ATOMIC, nullptr, 0);
            const uint8_t* tp=
                reinterpret_cast<const uint8_t*> (t2->label.begin ());
            mogan_loro_node_text_insert (doc, y_id, 0, tp,
                                         (size_t) N (t2->label));
            id_map (inside (t2))= y_id;
            mirrored            = true;
          }
          else {
            // 复合拆分：建 t2（复合），把 X 的 [at..] 子节点 mov 到 t2
            string        lab = as_string (L (t2));
            mogan_tree_id y_id= mogan_loro_node_create (
                doc, pid, (uint32_t) (pos + 1), LORO_COMPOUND,
                reinterpret_cast<const uint8_t*> (lab.begin ()),
                (size_t) N (lab));
            int m= N (t2);
            for (int i= 0; i < m; i++) {
              tree& c= subtree (doc_root, t2p * i);
              if (id_map->contains (inside (c)))
                mogan_loro_node_mov (doc, id_map (inside (c)), y_id,
                                     (uint32_t) i);
            }
            id_map (inside (t2))= y_id;
            mirrored            = true;
          }
        }
      }
    }
  }
  else if (mod->k == MOD_INSERT_NODE) {
    // 包一层：rp_mod=包装 W 的路径，argument(mod)=被包节点在 W 内的位置
    int pos= argument (mod);
    if (has_subtree (doc_root, rp_mod)) {
      tree& W = subtree (doc_root, rp_mod);
      path  pp= path_up (rp_mod);
      int   wi= last_item (rp_mod);
      // W 的父 TreeID：顶层（path_up 为空）→ root_id；否则 id_map[父]
      mogan_tree_id pid;
      bool          pid_ok= false;
      if (is_nil (pp)) {
        pid   = root_id;
        pid_ok= (root_id.peer != 0);
      }
      else if (has_subtree (doc_root, pp) &&
               id_map->contains (inside (subtree (doc_root, pp)))) {
        pid   = id_map (inside (subtree (doc_root, pp)));
        pid_ok= true;
      }
      path rfp= rp_mod * pos;
      if (pid_ok && has_subtree (doc_root, rfp) &&
          id_map->contains (inside (subtree (doc_root, rfp)))) {
        mogan_tree_id rf_id= id_map (inside (subtree (doc_root, rfp)));
        string        lab  = as_string (L (W));
        mogan_tree_id w_id = mogan_loro_node_create (
            doc, pid, (uint32_t) wi, LORO_COMPOUND,
            reinterpret_cast<const uint8_t*> (lab.begin ()), (size_t) N (lab));
        mogan_loro_node_mov (doc, rf_id, w_id, (uint32_t) pos);
        id_map (inside (W))= w_id;
        mirrored           = true;
      }
    }
  }
  else if (mod->k == MOD_REMOVE_NODE) {
    // 拆一层：rp_mod=包装 W 的路径，index(mod)=被提升的子节点在 W 内的位置
    int           pos= index (mod);
    path          pp = path_up (rp_mod);
    int           wi = last_item (rp_mod);
    mogan_tree_id pid;
    bool          pid_ok= false;
    if (is_nil (pp)) {
      pid   = root_id;
      pid_ok= (root_id.peer != 0);
    }
    else if (has_subtree (doc_root, pp) &&
             id_map->contains (inside (subtree (doc_root, pp)))) {
      pid   = id_map (inside (subtree (doc_root, pp)));
      pid_ok= true;
    }
    if (pid_ok && has_subtree (doc_root, rp_mod)) {
      array<mogan_tree_id> kids= node_children (pid);
      if (wi < N (kids)) {
        mogan_tree_id w_id= kids[wi];
        mogan_tree_id c_id=
            id_map->contains (inside (subtree (doc_root, rp_mod)))
                ? id_map (inside (subtree (doc_root, rp_mod)))
                : mogan_tree_id{0, 0};
        if (c_id.peer != 0) {
          mogan_loro_node_mov (doc, c_id, pid,
                               (uint32_t) wi); // 子提到 W 的位置
          mogan_loro_node_delete (doc, w_id);  // 删空包装
          mirrored= true;
        }
      }
    }
  }

  else if (mod->k == MOD_ASSIGN) {
    // 替换子树：rp_mod=节点路径。删旧节点（Loro 在父/wi 的子）+ 从 buffer
    // 当前（=mod->t）重建。
    if (!is_nil (rp_mod) && has_subtree (doc_root, rp_mod)) {
      tree&         node= subtree (doc_root, rp_mod); // post-assign = mod->t
      path          pp  = path_up (rp_mod);
      int           wi  = last_item (rp_mod);
      mogan_tree_id pid;
      bool          pid_ok= false;
      if (is_nil (pp)) {
        pid   = root_id;
        pid_ok= (root_id.peer != 0);
      }
      else if (has_subtree (doc_root, pp) &&
               id_map->contains (inside (subtree (doc_root, pp)))) {
        pid   = id_map (inside (subtree (doc_root, pp)));
        pid_ok= true;
      }
      if (pid_ok) {
        array<mogan_tree_id> kids= node_children (pid);
        if (wi < N (kids)) mogan_loro_node_delete (doc, kids[wi]); // 删旧
        seed_node (node, pid, (uint32_t) wi); // 建新（映射身份）
        mirrored= true;
      }
    }
  }
  else if (mod->k == MOD_JOIN) {
    // 合并 P[pos]+P[pos+1] → P[pos]（X 保留合并内容，Y 删除）
    int pos= index (mod);
    if (has_subtree (doc_root, rp_mod) &&
        id_map->contains (inside (subtree (doc_root, rp_mod)))) {
      mogan_tree_id        pid = id_map (inside (subtree (doc_root, rp_mod)));
      array<mogan_tree_id> kids= node_children (pid);
      if (pos + 1 < N (kids)) {
        mogan_tree_id x_id= kids[pos];
        mogan_tree_id y_id= kids[pos + 1];
        if (mogan_loro_node_join_text (doc, x_id, y_id) != 0) {
          // 复合 join：mov Y 的子节点到 X 末尾 + delete Y
          array<mogan_tree_id> x_kids= node_children (x_id);
          array<mogan_tree_id> y_kids= node_children (y_id);
          int                  base  = N (x_kids);
          int                  m     = N (y_kids);
          for (int i= 0; i < m; i++)
            mogan_loro_node_mov (doc, y_kids[i], x_id, (uint32_t) (base + i));
          mogan_loro_node_delete (doc, y_id);
        }
        // 合并后 buffer 的 P*pos（merged）对应 X（X 保留了合并内容）
        if (has_subtree (doc_root, rp_mod * pos))
          id_map (inside (subtree (doc_root, rp_mod * pos)))= x_id;
        mirrored= true;
      }
    }
  }
  if (!mirrored) {
    // 兜底（INSERT_NODE/REMOVE_NODE/ASSIGN/SPLIT/JOIN
    // 等，回车等复杂结构改动）：
    //   块级重 seed——只删+重建包含改动的 buffer
    //   根直接子节点（段落/块），而非整篇。 回车的 5 个 mod 序列各重 seed
    //   一次所在块（块级，远小于整篇），保 peer 血统。
    path p       = mod->p;
    bool reseeded= false;
    if (!is_nil (p) && root_id.peer != 0) {
      int  block_idx = p->item;
      path block_path= path (block_idx);
      if (has_subtree (doc_root, block_path) &&
          id_map->contains (inside (subtree (doc_root, block_path)))) {
        mogan_tree_id block_id=
            id_map (inside (subtree (doc_root, block_path)));
        mogan_loro_node_delete (doc, block_id);
        seed_node (subtree (doc_root, block_path), root_id,
                   (uint32_t) block_idx);
        reseeded= true;
      }
    }
    if (!reseeded) {
      // 连块级都定位不了（如改动在 buffer 根本身，或身份缺失）→ 整树重
      // seed（保血统）。
      if (root_id.peer != 0 || root_id.counter != 0)
        mogan_loro_node_delete (doc, root_id);
      id_map= hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
      seed (doc_root);
    }
  }
  // 显式提交：同步触发 local-update 事件（增量同步）。
  mogan_loro_doc_commit (doc);
}

/******************************************************************************
 * 导出 / 读取
 ******************************************************************************/

string
loro_shadow_rep::export_snapshot () {
  uint8_t* out    = nullptr;
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
  return mogan_loro_doc_import (
             doc, reinterpret_cast<const uint8_t*> (bytes.begin ()),
             (size_t) N (bytes)) == 0;
}

void
loro_shadow_rep::on_local_update (mogan_local_update_cb cb, void* user_data) {
  _update_cb       = cb;
  _update_user_data= user_data;
  mogan_loro_doc_on_local_update (doc, cb, user_data);
}

void
loro_shadow_rep::broadcast_update () {
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  // commit 后导出"自上次广播以来"的本地增量（Rust 侧推进 vv 水位），
  // 经保存的回调送出——与 subscribe_local_update 走同一通道。
  mogan_loro_doc_commit (doc);
  if (mogan_loro_doc_export_local_update (doc, &out, &out_len) == 0 &&
      out != nullptr && _update_cb != nullptr) {
    _update_cb (_update_user_data, out, out_len);
  }
  if (out) mogan_loro_free (out, out_len);
}

// 字符级 diff：把 before 文本变到 after 文本所需的 INSERT/REMOVE
// mod（前缀/后缀对齐）。 p 是该原子在 buffer 中的路径；mod 用
// mod_insert/remove(p, pos, ...)。
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
  int rm_len = bn - pre - suf; // before 中被删的长度
  int ins_len= an - pre - suf; // after 中新增的长度
  if (rm_len > 0) mods= mods * mod_remove (p, pre, rm_len);
  if (ins_len > 0) {
    string ins= a (pre, pre + ins_len);
    mods      = mods * mod_insert (p, pre, tree (ins));
  }
}

// 并行遍历 before/after（假设同结构）：文本差异 → 字符 diff mod；结构差异 →
// 返回 false。
bool
diff_walk (tree b, tree a, path base, list<modification>& mods) {
  if (b == a) return true; // exactly the same

  if (is_atomic (b) && is_atomic (a)) {
    if (b->label != a->label) emit_text_diff (base, b->label, a->label, mods);
    return true;
  }

  if (is_compound (b) && is_compound (a) && L (b) == L (a)) {
    int bn= N (b), an= N (a);
    if (bn == an) {
      for (int i= 0; i < bn; i++) {
        if (!diff_walk (b[i], a[i], base * path (i), mods)) {
          // If a child differs structurally in an un-diffable way, we assign
          // that child
          mods= mods * mod_assign (base * path (i), a[i]);
        }
      }
      return true;
    }
    else {
      // Find common prefix
      int pre= 0;
      while (pre < bn && pre < an && b[pre] == a[pre])
        pre++;
      // Find common suffix
      int suf= 0;
      while (suf < bn - pre && suf < an - pre &&
             b[bn - 1 - suf] == a[an - 1 - suf])
        suf++;

      int rm_len = bn - pre - suf;
      int ins_len= an - pre - suf;

      if (rm_len > 0) {
        mods= mods * mod_remove (base, pre, rm_len);
      }
      if (ins_len > 0) {
        tree ins (L (a), ins_len);
        for (int i= 0; i < ins_len; i++)
          ins[i]= a[pre + i];
        mods= mods * mod_insert (base, pre, ins);
      }
      // If there are inner modifications in the non-matching parts,
      // replace them via remove+insert is a valid structural edit.
      return true;
    }
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

// 解码"带 TreeID 的增强 IR"：建 moebius 树 + 填 id_map（rep* -> 导入的
// TreeID）。 格式：node := peer:u64 counter:i32 kind:u8 label:text text:text
// nchildren:u32 children
namespace {
tree
decode_id_node (string& b, int& pos,
                hashmap<tree_rep*, mogan_tree_id>& id_map) {
  auto get_u32= [&] () -> uint32_t {
    uint32_t v= (uint32_t) (unsigned char) b[pos] |
                ((uint32_t) (unsigned char) b[pos + 1] << 8) |
                ((uint32_t) (unsigned char) b[pos + 2] << 16) |
                ((uint32_t) (unsigned char) b[pos + 3] << 24);
    pos+= 4;
    return v;
  };
  auto get_str= [&] () -> string {
    uint32_t n= get_u32 ();
    string   r;
    for (uint32_t i= 0; i < n; i++)
      r << b[pos + i];
    pos+= n;
    return r;
  };
  // TreeID
  uint64_t peer= 0;
  for (int i= 0; i < 8; i++)
    peer|= ((uint64_t) (unsigned char) b[pos + i]) << (8 * i);
  pos+= 8;
  mogan_tree_id tid{peer, (int32_t) get_u32 ()};

  uint8_t  kind = (uint8_t) (unsigned char) b[pos++];
  string   label= get_str ();
  string   text = get_str ();
  uint32_t n    = get_u32 ();

  tree r;
  if (kind == LORO_ATOMIC) r= tree (text);
  else {
    int op= (kind == LORO_COMPOUND) ? (int) moebius::make_tree_label (label)
                                    : as_int (label (8, N (label)));
    r     = tree (op, (int) n);
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
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_to_ir_with_ids (doc, &out, &out_len) != 0 ||
      out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return false;
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  id_map= hashmap<tree_rep*, mogan_tree_id> (
      mogan_tree_id{0, 0}); // 重置后由 decode 填充
  int pos   = 0;
  out_buffer= decode_id_node (ir_bytes, pos, id_map);
  return true;
}

tree
loro_shadow_rep::to_tree () {
  uint8_t* out    = nullptr;
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
  return id_map->contains (inside (t)) ? id_map (inside (t))
                                       : mogan_tree_id{0, 0};
}

int
loro_shadow_rep::root_count () {
  return mogan_loro_doc_root_count (doc);
}

#endif // LORO_ENABLED
