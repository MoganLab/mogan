
/******************************************************************************
 * MODULE     : init_glue_l5.cpp
 * DESCRIPTION: L5 Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "init_glue_l5.hpp"

#include "glue_l5_extra.hpp"

string original_path;

void
initialize_glue_l5 () {
  initialize_glue_font ();
  initialize_glue_widget ();
  initialize_glue_basic ();
  initialize_glue_editor ();
  initialize_glue_server ();
}
