/******************************************************************************
 * MODULE     : glue_patch_extra.hpp
 * DESCRIPTION: helper functions used by glue_patch (generated, standalone)
 *              and by init_glue_l3.cpp's own registrations. Extracted so
 *              glue_patch.cpp can be compiled as an independent translation
 *              unit.
 ******************************************************************************/

#ifndef GLUE_PATCH_EXTRA_HPP
#define GLUE_PATCH_EXTRA_HPP

#include "patch.hpp"
#include "tree.hpp"
#include "tree_patch.hpp"

inline patch
branch_patch (array<patch> a) {
  return patch (true, a);
}

inline tree
var_clean_apply (tree t, patch p) {
  return clean_apply (copy (p), t);
}

inline tree
var_apply (tree& t, patch p) {
  apply (copy (p), t);
  return t;
}

#endif
