
/******************************************************************************
 * MODULE     : glue_l3.hpp
 * DESCRIPTION: L3 Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef INIT_GLUE_PLUGINS_HPP
#define INIT_GLUE_PLUGINS_HPP

void initialize_glue_plugins ();
void initialize_glue_bibtex ();
void initialize_glue_ghostscript ();
void initialize_glue_html ();
void initialize_glue_pdf ();
void initialize_glue_plugin ();
#ifdef LORO_ENABLED
void initialize_glue_collab ();
#endif
void initialize_glue_tex ();
void initialize_glue_updater ();
void initialize_glue_xml ();

#endif
