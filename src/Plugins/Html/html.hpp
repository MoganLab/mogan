
/******************************************************************************
 * MODULE     : html.hpp
 * DESCRIPTION: various conversion routines on HTML/MathML
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef HTML_HPP
#define HTML_HPP

#include "tree.hpp"

tree parse_html (string s);
tree clean_html (tree t);
tree parse_plain_html (string s);

void html_progress_start (int total);
void html_progress_update (int current);
void html_progress_end ();

tree upgrade_mathml (tree t);
tree retrieve_mathjax (int id);

/*** Post corrections ***/
bool   seems_buggy_html_paste (string s);
string correct_buggy_html_paste (string s);
bool   seems_buggy_paste (string s);
string correct_buggy_paste (string s);
tree   default_with_simplify (tree t);

#endif
