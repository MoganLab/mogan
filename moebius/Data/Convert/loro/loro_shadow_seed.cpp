/** \file loro_shadow_seed.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"

namespace {
/// 读一个 IR-with-ids 节点，消费 parent 的若干个 buffer 子节点（1 或 N+1），
/// 映射到 TreeID。wrap_para=true 时 buffer 子节点是 PARA
/// 包装，映射其原子子节点。
bool
sync_walk_n (tree parent, int start_idx, string& ir, int& pos,
             mogan_tree_id& root_id, hashmap<tree_rep*, mogan_tree_id>& id_map,
             hashmap<mogan_tree_id, path>& rev_id_map, int& out_consumed,
             path acc, bool wrap_para) {
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
  uint64_t peer= 0;
  for (int b= 0; b < 8; b++)
    peer|= ((uint64_t) (unsigned char) ir[pos + b]) << (8 * b);
  pos+= 8;
  mogan_tree_id tid{peer, (int32_t) get_u32 ()};
  if (root_id.peer == 0) root_id= tid;
  uint8_t kind = (uint8_t) (unsigned char) ir[pos++];
  string  label= get_str ();
  (void) get_str ();      // text
  uint32_t n= get_u32 (); // n_children

  if (kind == LORO_ATOMIC) {
    for (uint32_t i= 0; i < n; i++) {
      int dummy;
      if (!sync_walk_n (parent, start_idx, ir, pos, root_id, id_map, rev_id_map,
                        dummy, acc, false))
        return false;
    }
    uint32_t   ns= get_u32 ();
    array<int> splits;
    for (uint32_t i= 0; i < ns; i++)
      splits << (int) get_u32 ();
    // N+1 段 buffer 子节点全部映射到 tid
    int n_segs= (int) ns + 1;
    if (!is_compound (parent)) return false; // 防御：非复合节点不应有子节点
    for (int seg= 0; seg < n_segs; seg++) {
      if (start_idx + seg >= N (parent)) return false;
      tree& child= parent[start_idx + seg];
      // wrap_para：buffer 子节点是 PARA，映射其原子子节点[0] 到 tid
      if (wrap_para && N (child) >= 1 && is_atomic (child[0]))
        id_map (inside (child[0]))= tid;
      else id_map (inside (child))= tid;
    }
    rev_id_map (tid)= acc;
    out_consumed    = n_segs;
    return true;
  }

  // 复合节点
  int  op= (kind == LORO_COMPOUND) ? (int) moebius::make_tree_label (label)
                                   : as_int (label (8, N (label)));
  bool child_wrap= (op == (int) moebius::DOCUMENT);
  if (!is_compound (parent) || start_idx >= N (parent)) return false;
  tree t             = parent[start_idx];
  id_map (inside (t))= tid;
  rev_id_map (tid)   = acc;
  int nc             = N (t);
  int buf_idx        = 0;
  for (uint32_t i= 0; i < n; i++) {
    int consumed= 0;
    if (!sync_walk_n (t, buf_idx, ir, pos, root_id, id_map, rev_id_map,
                      consumed, acc * path (buf_idx), child_wrap))
      return false;
    buf_idx+= consumed;
  }
  if (buf_idx != nc) return false;
  uint32_t ns= get_u32 ();
  for (uint32_t i= 0; i < ns; i++)
    get_u32 ();
  out_consumed= 1;
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
  id_map      = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  rev_id_map  = hashmap<mogan_tree_id, path> (path ());
  root_id     = mogan_tree_id{0, 0};
  int pos     = 0;
  int consumed= 0;
  // 根节点：用 buffer 自身做 parent（根是唯一的，start_idx=0）
  tree root_wrap ((tree_label) moebius::DOCUMENT, 1);
  root_wrap[0]= buffer;
  if (!sync_walk_n (root_wrap, 0, ir, pos, root_id, id_map, rev_id_map,
                    consumed, path (), false))
    return false;
  return root_id.peer != 0;
}
