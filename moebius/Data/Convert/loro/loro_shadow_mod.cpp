/** \file loro_shadow_mod.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"

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

bool
loro_shadow_rep::mirror_insert (tree doc_root, modification mod) {
  path rp_mod= root (mod);
  if (!has_subtree (doc_root, rp_mod)) return false;
  tree& parent= subtree (doc_root, rp_mod);
  if (is_atomic (parent) && id_map->contains (inside (parent))) {
    mogan_tree_id id= id_map (inside (parent));
    string        s = mod->t->label;
    mogan_loro_node_text_insert (doc, id, (uint32_t) index (mod),
                                 reinterpret_cast<const uint8_t*> (s.begin ()),
                                 (size_t) N (s));
    return true;
  }
  else if (is_compound (parent) && id_map->contains (inside (parent))) {
    mogan_tree_id pid= id_map (inside (parent));
    int           pos= index (mod);
    int           nr = is_compound (mod->t) ? N (mod->t) : 1;
    for (int i= 0; i < nr; i++) {
      path child_path= rp_mod * (pos + i);
      if (has_subtree (doc_root, child_path)) {
        tree& child= subtree (doc_root, child_path);
        seed_node (child, pid, (uint32_t) (pos + i), child_path);
      }
    }
    return true;
  }
  return false;
}

bool
loro_shadow_rep::mirror_remove (tree doc_root, modification mod) {
  path rp_mod= root (mod);
  if (!has_subtree (doc_root, rp_mod)) return false;
  tree& parent= subtree (doc_root, rp_mod);
  if (is_atomic (parent) && id_map->contains (inside (parent))) {
    mogan_tree_id id= id_map (inside (parent));
    mogan_loro_node_text_delete (doc, id, (uint32_t) index (mod),
                                 (uint32_t) argument (mod));
    return true;
  }
  else if (is_compound (parent) && id_map->contains (inside (parent))) {
    mogan_tree_id        pid = id_map (inside (parent));
    int                  pos = index (mod);
    int                  nr  = argument (mod);
    array<mogan_tree_id> kids= node_children (pid);
    for (int j= pos + nr - 1; j >= pos; j--) {
      if (j < N (kids)) mogan_loro_node_delete (doc, kids[j]);
    }
    return true;
  }
  return false;
}

bool
loro_shadow_rep::mirror_assign_node (tree doc_root, modification mod) {
  path rp_mod= root (mod);
  if (has_subtree (doc_root, rp_mod)) {
    tree& node= subtree (doc_root, rp_mod);
    if (id_map->contains (inside (node))) {
      mogan_tree_id id = id_map (inside (node));
      string        lab= as_string (L (mod));
      mogan_loro_node_set_label (
          doc, id, reinterpret_cast<const uint8_t*> (lab.begin ()),
          (size_t) N (lab));
      return true;
    }
  }
  return false;
}

bool
loro_shadow_rep::mirror_split (tree doc_root, modification mod) {
  path rp_mod= root (mod);
  int  pos   = index (mod);
  int  at    = argument (mod);
  if (has_subtree (doc_root, rp_mod) &&
      id_map->contains (inside (subtree (doc_root, rp_mod)))) {
    mogan_tree_id        pid = id_map (inside (subtree (doc_root, rp_mod)));
    array<mogan_tree_id> kids= node_children (pid);
    if (pos < N (kids)) {
      mogan_tree_id x_id= kids[pos];
      if (has_subtree (doc_root, rp_mod * pos)) {
        id_map (inside (subtree (doc_root, rp_mod * pos)))= x_id;
        rev_id_map (x_id)= rp_mod * pos;
      }
      path t2p= rp_mod * (pos + 1);
      if (has_subtree (doc_root, t2p)) {
        tree& t2= subtree (doc_root, t2p);
        if (is_atomic (t2)) {
          mogan_loro_node_text_delete (doc, x_id, (uint32_t) at,
                                       (uint32_t) N (t2->label));
          mogan_tree_id y_id= mogan_loro_node_create (
              doc, pid, (uint32_t) (pos + 1), LORO_ATOMIC, nullptr, 0);
          const uint8_t* tp=
              reinterpret_cast<const uint8_t*> (t2->label.begin ());
          mogan_loro_node_text_insert (doc, y_id, 0, tp,
                                       (size_t) N (t2->label));
          id_map (inside (t2))= y_id;
          rev_id_map (y_id)= t2p;
          return true;
        }
        else {
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
          rev_id_map (y_id)= t2p;
          return true;
        }
      }
    }
  }
  return false;
}

bool
loro_shadow_rep::mirror_insert_node (tree doc_root, modification mod) {
  path rp_mod= root (mod);
  int  pos   = argument (mod);
  if (has_subtree (doc_root, rp_mod)) {
    tree&         W = subtree (doc_root, rp_mod);
    path          pp= path_up (rp_mod);
    int           wi= last_item (rp_mod);
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
      rev_id_map (w_id)= rp_mod;
      return true;
    }
  }
  return false;
}

bool
loro_shadow_rep::mirror_remove_node (tree doc_root, modification mod) {
  path          rp_mod= root (mod);
  int           pos   = index (mod);
  path          pp    = path_up (rp_mod);
  int           wi    = last_item (rp_mod);
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
      mogan_tree_id c_id= id_map->contains (inside (subtree (doc_root, rp_mod)))
                              ? id_map (inside (subtree (doc_root, rp_mod)))
                              : mogan_tree_id{0, 0};
      if (c_id.peer != 0) {
        mogan_loro_node_mov (doc, c_id, pid, (uint32_t) wi);
        mogan_loro_node_delete (doc, w_id);
        return true;
      }
    }
  }
  return false;
}

bool
loro_shadow_rep::mirror_assign (tree doc_root, modification mod) {
  path rp_mod= root (mod);
  if (!is_nil (rp_mod) && has_subtree (doc_root, rp_mod)) {
    tree&         node= subtree (doc_root, rp_mod);
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
      if (wi < N (kids)) mogan_loro_node_delete (doc, kids[wi]);
      seed_node (node, pid, (uint32_t) wi, rp_mod);
      return true;
    }
  }
  return false;
}

bool
loro_shadow_rep::mirror_join (tree doc_root, modification mod) {
  path rp_mod= root (mod);
  int  pos   = index (mod);
  if (has_subtree (doc_root, rp_mod) &&
      id_map->contains (inside (subtree (doc_root, rp_mod)))) {
    mogan_tree_id        pid = id_map (inside (subtree (doc_root, rp_mod)));
    array<mogan_tree_id> kids= node_children (pid);
    if (pos + 1 < N (kids)) {
      mogan_tree_id x_id= kids[pos];
      mogan_tree_id y_id= kids[pos + 1];
      if (mogan_loro_node_join_text (doc, x_id, y_id) != 0) {
        array<mogan_tree_id> x_kids= node_children (x_id);
        array<mogan_tree_id> y_kids= node_children (y_id);
        int                  base  = N (x_kids);
        int                  m     = N (y_kids);
        for (int i= 0; i < m; i++)
          mogan_loro_node_mov (doc, y_kids[i], x_id, (uint32_t) (base + i));
        mogan_loro_node_delete (doc, y_id);
      }
      if (has_subtree (doc_root, rp_mod * pos)) {
        id_map (inside (subtree (doc_root, rp_mod * pos)))= x_id;
        rev_id_map (x_id)= rp_mod * pos;
      }
      return true;
    }
  }
  return false;
}

void
loro_shadow_rep::mirror_mod (tree doc_root, modification mod) {
  bool mirrored= false;
  switch (mod->k) {
  case MOD_INSERT:
    mirrored= mirror_insert (doc_root, mod);
    break;
  case MOD_REMOVE:
    mirrored= mirror_remove (doc_root, mod);
    break;
  case MOD_ASSIGN_NODE:
    mirrored= mirror_assign_node (doc_root, mod);
    break;
  case MOD_SPLIT:
    mirrored= mirror_split (doc_root, mod);
    break;
  case MOD_INSERT_NODE:
    mirrored= mirror_insert_node (doc_root, mod);
    break;
  case MOD_REMOVE_NODE:
    mirrored= mirror_remove_node (doc_root, mod);
    break;
  case MOD_ASSIGN:
    mirrored= mirror_assign (doc_root, mod);
    break;
  case MOD_JOIN:
    mirrored= mirror_join (doc_root, mod);
    break;
  default:
    break;
  }

  if (!mirrored) {
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
                   (uint32_t) block_idx, block_path);
        reseeded= true;
      }
    }
    if (!reseeded) {
      if (root_id.peer != 0 || root_id.counter != 0)
        mogan_loro_node_delete (doc, root_id);
      id_map    = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
      rev_id_map= hashmap<mogan_tree_id, path> (path ());
      seed (doc_root);
    }
  }
  mogan_loro_doc_commit (doc);
}
