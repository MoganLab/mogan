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
#include "path.hpp"
#include "tree.hpp"

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
  for (i= 0; i < pos; i++)
    r[i]= t[i];
  r[pos]= x;
  for (i= pos; i < n; i++)
    r[i + 1]= t[i];
  return r;
}

#endif
