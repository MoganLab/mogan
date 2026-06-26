/******************************************************************************
 * MODULE     : glue_ghostscript_extra.hpp
 * DESCRIPTION: helper functions used by glue_ghostscript (generated,
 *              standalone). Extracted so glue_ghostscript.cpp can be
 *              compiled as an independent translation unit.
 ******************************************************************************/

#ifndef GLUE_GHOSTSCRIPT_EXTRA_HPP
#define GLUE_GHOSTSCRIPT_EXTRA_HPP

inline bool
supports_ghostscript () {
#ifdef USE_PLUGIN_GS
  return true;
#else
  return false;
#endif
}

#endif
