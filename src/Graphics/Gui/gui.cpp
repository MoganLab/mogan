
/******************************************************************************
 * MODULE     : gui.cpp
 * DESCRIPTION: several gui related routines
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "gui.hpp"
#include "boot.hpp"

bool
gui_is_x () {
#ifdef QTTEXMACS
  return false;
#else
  return true;
#endif
}

bool
gui_is_qt () {
#ifdef QTTEXMACS
  return true;
#else
  return false;
#endif
}

bool
gui_is_headless () {
  return headless_mode;
}
