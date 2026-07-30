/** \file loro_shadow_seed.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"

namespace {
uint32_t
ir_get_u32 (const string& ir, int& pos) {
  uint32_t v= (uint32_t) (unsigned char) ir[pos] |
              ((uint32_t) (unsigned char) ir[pos + 1] << 8) |
              ((uint32_t) (unsigned char) ir[pos + 2] << 16) |
              ((uint32_t) (unsigned char) ir[pos + 3] << 24);
  pos+= 4;
  return v;
}
void
ir_get_str (const string& ir, int& pos, string& out) {
  uint32_t n= ir_get_u32 (ir, pos);
  for (uint32_t i= 0; i < n; i++)
    out << ir[pos + i];
  pos+= n;
}
// 把 pos 推进过一个 IR 子树（不触碰 buffer）：peer:u64 counter:u32 kind:u8
// label:str text:str n_children:u32 children[]。用于 buffer/shadow 子节点数
// 不等时跳过 IR 多余的部分，让 id_map 重建继续进行（仅截断返回 false）。
bool
ir_skip_subtree (const string& ir, int& pos) {
  if (pos + 12 > N (ir)) return false;
  pos+= 8;                     // peer
  (void) ir_get_u32 (ir, pos); // counter
  pos++;                       // kind
  string dummy;
  ir_get_str (ir, pos, dummy);        // label
  ir_get_str (ir, pos, dummy);        // text
  uint32_t nch= ir_get_u32 (ir, pos); // n_children
  for (uint32_t i= 0; i < nch; i++)
    if (!ir_skip_subtree (ir, pos)) return false;
  return true;
}
bool
sync_walk (tree t, string& ir, int& pos, mogan_tree_id& root_id,
           hashmap<tree_rep*, mogan_tree_id>& id_map,
           hashmap<mogan_tree_id, path>& rev_id_map, path acc) {
  if (pos + 12 > N (ir)) return false;
  // TreeID
  uint64_t peer= 0;
  for (int b= 0; b < 8; b++)
    peer|= ((uint64_t) (unsigned char) ir[pos + b]) << (8 * b);
  pos+= 8;
  mogan_tree_id tid{peer, (int32_t) ir_get_u32 (ir, pos)};
  if (root_id.peer == 0) root_id= tid;
  // kind/label/text/n_children
  pos++; // kind
  string dummy;
  ir_get_str (ir, pos, dummy);               // label
  ir_get_str (ir, pos, dummy);               // text
  uint32_t n         = ir_get_u32 (ir, pos); // n_children
  id_map (inside (t))= tid;
  rev_id_map (tid)   = acc; // 与 id_map 同处维护：TreeID -> buffer-相对 path
  int nc             = is_atomic (t) ? 0 : N (t);
  // 子节点数不等（merge 后 buffer/shadow 顺序错位）不再整体放弃：递归公共前缀，
  // 跳过 IR 多余子树，让兄弟/祖先的映射继续就位。仅 IR 截断返回 false。
  // 注：公共前缀仍按位置对齐，重排+数量不等同发的极端情形仍可能绑错，但比
  // 整树放弃（留下残缺 id_map → 后续镜像必然重 seed）要好。
  int common= nc < (int) n ? nc : (int) n;
  for (int i= 0; i < common; i++)
    if (!sync_walk (t[i], ir, pos, root_id, id_map, rev_id_map, acc * path (i)))
      return false;
  for (int i= common; i < (int) n; i++) // IR 有更多孩子：推进游标
    if (!ir_skip_subtree (ir, pos)) return false;
  return true;
}
} // namespace

mogan_tree_id
loro_shadow_rep::seed_node (tree t, mogan_tree_id parent, uint32_t index,
                            path p) {
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
  rev_id_map (id)    = p;  // 与 id_map 同处维护：TreeID -> buffer-相对 path

  if (is_atomic (t)) {
    const uint8_t* tp= reinterpret_cast<const uint8_t*> (t->label.begin ());
    size_t         tn= (size_t) N (t->label);
    if (mogan_loro_node_text_insert (doc, id, 0, tp, tn) != 0)
      mogan_loro_node_set_binary (doc, id, tp, tn);
  }
  else {
    int n= N (t);
    for (int i= 0; i < n; i++)
      seed_node (t[i], id, (uint32_t) i, p * path (i));
  }
  return id;
}

void
loro_shadow_rep::seed (tree root) {
  mogan_tree_id root_parent= {UINT64_MAX, 0}; // Root 哨兵
  root_id                  = seed_node (root, root_parent, 0, path ());
}

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
  id_map    = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  rev_id_map= hashmap<mogan_tree_id, path> (path ());
  root_id   = mogan_tree_id{0, 0};
  int pos   = 0;
  if (!sync_walk (buffer, ir, pos, root_id, id_map, rev_id_map, path ()))
    return false;
  return root_id.peer != 0;
}
