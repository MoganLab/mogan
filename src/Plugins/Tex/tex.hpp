
/******************************************************************************
 * MODULE     : tex.hpp
 * DESCRIPTION: various conversion routines on TeX
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TEX_HPP
#define TEX_HPP

#include "object.hpp"
#include "string.hpp"
#include "tree.hpp"

tree   parse_latex (string s, bool change= false, bool as_pic= false);
tree   parse_latex_document (string s, bool change= false, bool as_pic= false);
tree   latex_to_tree (tree t);
tree   latex_document_to_tree (string s, bool as_pic= false);
tree   latex_class_document_to_tree (string s);
string latex_verbarg_to_string (tree t);
string get_latex_style (tree t);
string string_arg (tree t, bool u= false);
array<tree> tokenize_concat (tree t, array<tree> a, bool keep= false);
int         latex_search_forwards (string s, int pos, string in);
int         latex_search_forwards (string s, string in);
tree        tracked_latex_to_texmacs (string s, bool as_pic);
string      conservative_texmacs_to_latex (tree doc, object opts);
string      tracked_texmacs_to_latex (tree doc, object opts);
tree        conservative_latex_to_texmacs (string s, bool as_pic);
int         number_latex_pages (url log);
tree        postprocess_metadata (tree t);

void latex_progress_start (int total);
void latex_progress_update (int current);
void latex_progress_end ();

#endif
