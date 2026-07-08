
/******************************************************************************
 * MODULE     : init_glue_l4.hpp
 * DESCRIPTION: L4 Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef INIT_GLUE_L5_HPP
#define INIT_GLUE_L5_HPP

void initialize_glue_l5 ();
void initialize_glue_basic ();
void initialize_glue_editor ();
void initialize_glue_font ();
void initialize_glue_server ();
void initialize_glue_widget ();
#ifdef QTTEXMACS
void initialize_glue_qt ();
#endif

#endif
