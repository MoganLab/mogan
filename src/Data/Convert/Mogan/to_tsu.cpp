
/******************************************************************************
 * MODULE     : to_tsu.cpp
 * DESCRIPTION: conversion of TeXmacs trees to the TSU file format
 * COPYRIGHT  : (C) 2024  Darcy Shen
 *              (C) 2026  Eshaan Gupta
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "convert.hpp"
#include "tree_helper.hpp"

static const string TSU_VERSION= "1.1.0";

/******************************************************************************
 * Conversion of TeXmacs trees to TSU strings
 ******************************************************************************/

string
tree_to_tsu (tree t) {
  if (!is_snippet (t)) {
    int  t_N= N (t);
    tree r (t, t_N);
    for (int i= 0; i < t_N; i++) {
      if (is_compound (t[i], "style", 1)) {
        tree style= t[i][0];
        if (is_func (style, TUPLE, 1)) style= style[0];
        r[i]   = copy (t[i]);
        r[i][0]= style;
      }
      else if (is_compound (t[i], "TeXmacs")) {
        r[i]= compound ("TSU", tuple (TSU_VERSION, string (XMACS_VERSION)));
      }
      else r[i]= t[i];
    }
    t= r;
  }

  return mogan_tree_serialize (t);
}
