/** \file loro_shadow_seed.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"

namespace {
bool
sync_walk (tree t, string& ir, int& pos, mogan_tree_id& root_id,
           hashmap<tree_rep*, mogan_tree_id>& id_map,
           hashmap<mogan_tree_id, path>& rev_id_map, path acc) {
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
  rev_id_map (tid)   = acc; // 与 id_map 同处维护：TreeID -> buffer-相对 path
  int nc             = is_atomic (t) ? 0 : N (t);
  if (nc != (int) n) return false; // 结构不匹配
  for (int i= 0; i < nc; i++)
    if (!sync_walk (t[i], ir, pos, root_id, id_map, rev_id_map, acc * path (i)))
      return false;
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
  id_map (inside (t))= id;     // 记录身份
  rev_id_map (id)    = p;      // 与 id_map 同处维护：TreeID -> buffer-相对 path

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
