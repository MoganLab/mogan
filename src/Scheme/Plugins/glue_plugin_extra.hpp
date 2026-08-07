/******************************************************************************
 * MODULE     : glue_plugin_extra.hpp
 * DESCRIPTION: helper functions used by glue_plugin (generated, standalone).
 *              Extracted so glue_plugin.cpp can be compiled as an
 *              independent translation unit.
 ******************************************************************************/

#ifndef GLUE_PLUGIN_EXTRA_HPP
#define GLUE_PLUGIN_EXTRA_HPP

inline bool
use_plugin_updater () {
#if defined(USE_PLUGIN_VELOPACK)
  return true;
#else
  return false;
#endif
}

inline bool
use_plugin_tex () {
#ifdef USE_PLUGIN_TEX
  return true;
#else
  return false;
#endif
}

inline bool
use_plugin_bibtex () {
#ifdef USE_PLUGIN_BIBTEX
  return true;
#else
  return false;
#endif
}

inline bool
loro_enabled () {
#ifdef LORO_ENABLED
  return true;
#else
  return false;
#endif
}

#endif
