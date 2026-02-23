/******************************************************************************
 * MODULE     : convert.hpp
 * DESCRIPTION: various conversion routines
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef CONVERT_H
#define CONVERT_H

#include "analyze.hpp"
#include "hashmap.hpp"
#include "tree.hpp"
#include "url.hpp"

class object;

/*** Miscellaneous ***/
bool is_snippet (tree doc);
void set_file_focus (url u);
url  get_file_focus ();

/*** Generic ***/
string        suffix_to_format (string suffix);
string        format_to_suffix (string format);
bool          format_exists (string format);
string        get_format (string s, string suffix);
tree          generic_to_tree (string s, string format);
string        tree_to_generic (tree doc, string format);
array<string> compute_keys (string s, string fm);
array<string> compute_keys (tree t, string fm);
array<string> compute_keys (url u);
scheme_tree   compute_index (string s, string fm);
scheme_tree   compute_index (tree t, string fm);
scheme_tree   compute_index (url u);

/*** Texmacs ***/
tree                 texmacs_to_tree (string s);
tree                 texmacs_document_to_tree (string s);
string               tree_to_texmacs (tree t);
tree                 extract (tree doc, string attr);
tree                 extract_document (tree doc);
tree                 change_doc_attr (tree doc, string attr, tree val);
tree                 remove_doc_attr (tree doc, string attr);
hashmap<string, int> get_codes (string version);
tree                 string_to_tree (string s, string version);
tree                 upgrade (tree t, string version);
tree                 substitute (tree t, tree which, tree by);
tree                 nonumber_to_eqnumber (tree t);
tree                 eqnumber_to_nonumber (tree t);
string               search_metadata (tree doc, string kind);

/*** Mogan serialization ***/
string mogan_tree_serialize (tree t);

/*** TMU ***/
tree   tmu_to_tree (string s);
tree   tmu_to_tree (string s, string version);
tree   tmu_document_to_tree (string s);
string tree_to_tmu (tree t);

/*** TSU ***/
tree   tsu_document_to_tree (string s);
string tree_to_tsu (tree t);

/*** Verbatim ***/
string tree_to_verbatim (tree t, bool wrap= false, string enc= "default");
tree   verbatim_to_tree (string s, bool wrap= false, string enc= "default");
tree   verbatim_document_to_tree (string s, bool w= false, string e= "default");
bool   is_verbatim (tree t);

#endif // defined CONVERT_H
