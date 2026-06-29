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

inline patch
branch_patch (array<patch> a) {
  return patch (true, a);
}

#endif
