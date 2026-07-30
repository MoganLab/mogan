/** \file loro_shadow_seed.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"
#include "tree_observer.hpp"

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
  // 身份对账：把 buffer 对齐到 shadow 当前状态，并由 reconcile_walk 维护
  // id_map/rev_id_map。无 id 的本地节点按位置绑定到 shadow 的同位置 id；
  // 真正被远端删/增/改的节点生成对应 mod 后应用到 buffer。
  id_map    = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  rev_id_map= hashmap<mogan_tree_id, path> (path ());
  root_id   = mogan_tree_id{0, 0};
  list<modification> mods= reconcile_ids (buffer);
  for (list<modification> l= mods; !is_nil (l); l= l->next)
    apply (buffer, l->item);
  return root_id.peer != 0;
}
