/******************************************************************************
 * MODULE     : glue_tree_extra.hpp
 * DESCRIPTION: helper functions used by glue_tree (generated, standalone)
 *              and by init_glue_l4.cpp's own registrations.
 *              Extracted so glue_tree.cpp can be compiled as an
 *              independent translation unit.
 ******************************************************************************/

#ifndef GLUE_TREE_EXTRA_HPP
#define GLUE_TREE_EXTRA_HPP

#include "analyze.hpp"
#include "new_document.hpp"
#include "observers.hpp"
#include "path.hpp"
#include "tree.hpp"
#include "tree_observer.hpp"

extern tree the_et;

inline tree
coerce_string_tree (string s) {
  return s;
}

inline string
coerce_tree_string (tree t) {
  return as_string (t);
}

inline tree
tree_ref (tree t, int i) {
  return t[i];
}

inline tree
tree_set (tree t, int i, tree u) {
  t[i]= u;
  return u;
}

inline tree
tree_range (tree t, int i, int j) {
  return t (i, j);
}

inline tree
tree_append (tree t1, tree t2) {
  return t1 * t2;
}

inline tree
tree_child_insert (tree t, int pos, tree x) {
  int  i, n= N (t);
  tree r (t, n + 1);
  for (i= 0; i< pos; i++)
    r[i]= t[i];
  r[pos]= x;
  for (i= pos; i< n; i++)
    r[i + 1]= t[i];
  return r;
}

inline tree
tree_assign (tree r, tree t) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    assign (reverse (ip), copy (t));
    return subtree (the_et, reverse (ip));
  }
  else {
    assign (r, copy (t));
    return r;
  }
}

inline tree
tree_insert (tree r, int pos, tree t) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    insert (reverse (path (pos, ip)), copy (t));
    return subtree (the_et, reverse (ip));
  }
  else {
    insert (r, pos, copy (t));
    return r;
  }
}

inline tree
tree_remove (tree r, int pos, int nr) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    remove (reverse (path (pos, ip)), nr);
    return subtree (the_et, reverse (ip));
  }
  else {
    remove (r, pos, nr);
    return r;
  }
}

inline tree
tree_split (tree r, int pos, int at) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    split (reverse (path (at, pos, ip)));
    return subtree (the_et, reverse (ip));
  }
  else {
    split (r, pos, at);
    return r;
  }
}

inline tree
tree_join (tree r, int pos) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    join (reverse (path (pos, ip)));
    return subtree (the_et, reverse (ip));
  }
  else {
    join (r, pos);
    return r;
  }
}

inline tree
tree_assign_node (tree r, tree_label op) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    assign_node (reverse (ip), op);
    return subtree (the_et, reverse (ip));
  }
  else {
    assign_node (r, op);
    return r;
  }
}

inline tree
tree_insert_node (tree r, int pos, tree t) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    insert_node (reverse (path (pos, ip)), copy (t));
    return subtree (the_et, reverse (ip));
  }
  else {
    insert_node (r, pos, copy (t));
    return r;
  }
}

inline tree
tree_remove_node (tree r, int pos) {
  path ip= copy (obtain_ip (r));
  if (ip_attached (ip)) {
    remove_node (reverse (path (pos, ip)));
    return subtree (the_et, reverse (ip));
  }
  else {
    remove_node (r, pos);
    return r;
  }
}

#endif
