
/******************************************************************************
 * MODULE     : glue.cpp
 * DESCRIPTION: Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "glue.hpp"
#include "init_glue_l2.hpp"
#include "init_glue_l3.hpp"
#include "init_glue_l4.hpp"
#include "init_glue_l5.hpp"
#include "init_glue_plugins.hpp"

// Claw AI Glue
#include "init_glue_claw_ai.cpp"

void
initialize_glue () {
  initialize_glue_l2 ();
  initialize_glue_l3 ();
  initialize_glue_l4 ();
  initialize_glue_l5 ();
  initialize_glue_plugins ();
  initialize_claw_ai_glue ();  // Initialize Claw AI
}
