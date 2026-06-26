/******************************************************************************
 * MODULE     : glue_modification_extra.hpp
 * DESCRIPTION: helper functions used by glue_modification (generated,
 *              standalone) and by init_glue_l3.cpp's own registrations.
 *              Extracted so glue_modification.cpp can be compiled as an
 *              independent translation unit.
 ******************************************************************************/

#ifndef GLUE_MODIFICATION_EXTRA_HPP
#define GLUE_MODIFICATION_EXTRA_HPP

#include "modification.hpp"
#include "tree.hpp"
#include "tree_observer.hpp"

inline tree
var_apply (tree& t, modification m) {
  apply (t, copy (m));
  return t;
}

inline tree
var_clean_apply (tree& t, modification m) {
  return clean_apply (t, copy (m));
}

#endif
