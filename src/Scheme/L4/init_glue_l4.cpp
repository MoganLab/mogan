
/******************************************************************************
 * MODULE     : init_glue_l4.cpp
 * DESCRIPTION: L4 Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "init_glue_l4.hpp"

void
initialize_glue_l4 () {
  initialize_glue_tree ();
  initialize_glue_convert ();
}
