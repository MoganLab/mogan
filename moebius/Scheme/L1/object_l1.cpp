
/******************************************************************************
 * MODULE     : object_l1.cpp
 * DESCRIPTION: Implementation of scheme objects (L1)
 * COPYRIGHT  : (C) 1999-2011 Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "object_l1.hpp"
#include "analyze.hpp"
#include "object.hpp"
#include "scheme.hpp"

#include "moebius/data/scheme.hpp"
#include "moebius/tree_label.hpp"

#include <lolly/data/base64.hpp>

using moebius::make_tree_label;
using moebius::RAW_DATA;
using moebius::data::scm_quote;
using moebius::data::scm_unquote;
using moebius::data::tree_to_scheme_tree;

/******************************************************************************
 * Trees
 ******************************************************************************/

bool
tmscm_is_tree (tmscm u) {
  return (tmscm_is_blackbox (u) &&
          (type_box (tmscm_to_blackbox (u)) == type_helper<tree>::id));
}

tmscm
tree_to_tmscm (tree o) {
  return blackbox_to_tmscm (close_box<tree> (o));
}

tree
tmscm_to_tree (tmscm obj) {
  return open_box<tree> (tmscm_to_blackbox (obj));
}

tmscm
tree_label_to_tmscm (tree_label l) {
  string s= as_string (l);
  return symbol_to_tmscm (s);
}

tree_label
tmscm_to_tree_label (tmscm p) {
  string s= tmscm_to_symbol (p);
  return make_tree_label (s);
}

bool
is_tree (object obj) {
  return tmscm_is_tree (object_to_tmscm (obj));
}

tree
as_tree (object obj) {
  tmscm t= object_to_tmscm (obj);
  if (!tmscm_is_tree (t)) return tree ();
  return tmscm_to_tree (t);
}

/******************************************************************************
 * Scheme trees
 ******************************************************************************/

tmscm
scheme_tree_to_tmscm (scheme_tree t) {
  if (is_atomic (t)) {
    string s= t->label;
    if (s == "#t") return tmscm_true ();
    if (s == "#f") return tmscm_false ();
    if (is_int (s)) return int_to_tmscm (as_int (s));
    if (is_quoted (s)) return string_to_tmscm (scm_unquote (s));
    // if ((N(s)>=2) && (s[0]=='\42') && (s[N(s)-1]=='\42'))
    // return string_to_tmscm (s (1, N(s)-1));
    if (N (s) >= 1 && s[0] == '\'') return symbol_to_tmscm (s (1, N (s)));
    return symbol_to_tmscm (s);
  }
  else {
    int   i;
    tmscm p= tmscm_null ();
    for (i= N (t) - 1; i >= 0; i--)
      p= tmscm_cons (scheme_tree_to_tmscm (t[i]), p);
    return p;
  }
}

scheme_tree
tmscm_to_scheme_tree (tmscm p) {
  if (tmscm_is_list (p)) {
    tree t (TUPLE);
    while (!tmscm_is_null (p)) {
      t << tmscm_to_scheme_tree (tmscm_car (p));
      p= tmscm_cdr (p);
    }
    return t;
  }
  if (tmscm_is_symbol (p)) return tmscm_to_symbol (p);
  if (tmscm_is_string (p)) return scm_quote (tmscm_to_string (p));
  // if (tmscm_is_string (p)) return "\"" * tmscm_to_string (p) * "\"";
  if (tmscm_is_int (p)) return as_string ((int) tmscm_to_int (p));
  if (tmscm_is_bool (p))
    return (tmscm_to_bool (p) ? string ("#t") : string ("#f"));
  if (tmscm_is_tree (p)) return tree_to_scheme_tree (tmscm_to_tree (p));
  return "?";
}

/******************************************************************************
 * Content
 ******************************************************************************/

tree
tmscm_to_content (tmscm p) {
  if (tmscm_is_string (p)) return tmscm_to_string (p);
  if (tmscm_is_tree (p)) return tmscm_to_tree (p);
  if (tmscm_is_pair (p)) {
    if (!tmscm_is_symbol (tmscm_car (p))) return "?";
    tree_label tl= make_tree_label (tmscm_to_symbol (tmscm_car (p)));
    if (tl == RAW_DATA) {
      // RAW_DATA in Scheme S-expression contains base64-encoded data;
      // decode back to binary for the C++ tree representation
      tree t (RAW_DATA, 1);
      t[0]->label=
          lolly::data::decode_base64 (tmscm_to_string (tmscm_cadr (p)));
      return t;
    }
    tree t (tl);
    p= tmscm_cdr (p);
    while (!tmscm_is_null (p)) {
      t << tmscm_to_content (tmscm_car (p));
      p= tmscm_cdr (p);
    }
    return t;
  }
  return "?";
}

bool
tmscm_is_content (tmscm p) {
  if (tmscm_is_string (p) || tmscm_is_tree (p)) return true;
  else if (!tmscm_is_pair (p) || !tmscm_is_symbol (tmscm_car (p))) return false;
  else {
    for (p= tmscm_cdr (p); !tmscm_is_null (p); p= tmscm_cdr (p))
      if (!tmscm_is_pair (p) || !tmscm_is_content (tmscm_car (p))) return false;
    return true;
  }
}

/******************************************************************************
 * Commands
 ******************************************************************************/

bool
tmscm_is_command (tmscm u) {
  return (tmscm_is_blackbox (u) &&
          (type_box (tmscm_to_blackbox (u)) == type_helper<command>::id));
}

tmscm
command_to_tmscm (command o) {
  return blackbox_to_tmscm (close_box<command> (o));
}

command
tmscm_to_command (tmscm o) {
  return open_box<command> (tmscm_to_blackbox (o));
}

/******************************************************************************
 * List types
 ******************************************************************************/

bool
tmscm_is_list_string (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_string (tmscm_car (p)) &&
           tmscm_is_list_string (tmscm_cdr (p));
}

tmscm
list_string_to_tmscm (list_string l) {
  if (is_nil (l)) return tmscm_null ();
  return tmscm_cons (string_to_tmscm (l->item), list_string_to_tmscm (l->next));
}

list_string
tmscm_to_list_string (tmscm p) {
  if (tmscm_is_null (p)) return list_string ();
  return list_string (tmscm_to_string (tmscm_car (p)),
                      tmscm_to_list_string (tmscm_cdr (p)));
}

bool
tmscm_is_list_tree (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_tree (tmscm_car (p)) &&
           tmscm_is_list_tree (tmscm_cdr (p));
}

tmscm
list_tree_to_tmscm (list_tree l) {
  if (is_nil (l)) return tmscm_null ();
  return tmscm_cons (tree_to_tmscm (l->item), list_tree_to_tmscm (l->next));
}

list_tree
tmscm_to_list_tree (tmscm p) {
  if (tmscm_is_null (p)) return list_tree ();
  return list_tree (tmscm_to_tree (tmscm_car (p)),
                    tmscm_to_list_tree (tmscm_cdr (p)));
}

/******************************************************************************
 * Paths
 ******************************************************************************/

bool
tmscm_is_path (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else return tmscm_is_int (tmscm_car (p)) && tmscm_is_path (tmscm_cdr (p));
}

tmscm
path_to_tmscm (path p) {
  if (is_nil (p)) return tmscm_null ();
  else return tmscm_cons (int_to_tmscm (p->item), path_to_tmscm (p->next));
}

path
tmscm_to_path (tmscm p) {
  if (tmscm_is_null (p)) return path ();
  else
    return path ((int) tmscm_to_int (tmscm_car (p)),
                 tmscm_to_path (tmscm_cdr (p)));
}

/******************************************************************************
 * Modification
 ******************************************************************************/

bool
tmscm_is_modification (tmscm m) {
  return (tmscm_is_blackbox (m) && (type_box (tmscm_to_blackbox (m)) ==
                                    type_helper<modification>::id)) ||
         (tmscm_is_string (m));
}

tmscm
modification_to_tmscm (modification m) {
  return blackbox_to_tmscm (close_box<modification> (m));
}

modification
tmscm_to_modification (tmscm obj) {
  return open_box<modification> (tmscm_to_blackbox (obj));
}

/******************************************************************************
 * Several array types
 ******************************************************************************/

bool
tmscm_is_array_int (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_int (tmscm_car (p)) &&
           tmscm_is_array_int (tmscm_cdr (p));
}

tmscm
array_int_to_tmscm (array<int> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (int_to_tmscm (a[i]), p);
  return p;
}

array<int>
tmscm_to_array_int (tmscm p) {
  array<int> a;
  while (!tmscm_is_null (p)) {
    a << ((int) tmscm_to_int (tmscm_car (p)));
    p= tmscm_cdr (p);
  }
  return a;
}

bool
tmscm_is_array_string (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_string (tmscm_car (p)) &&
           tmscm_is_array_string (tmscm_cdr (p));
}

bool
tmscm_is_array_double (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_double (tmscm_car (p)) &&
           tmscm_is_array_double (tmscm_cdr (p));
}

tmscm
array_double_to_tmscm (array<double> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (double_to_tmscm (a[i]), p);
  return p;
}

array<double>
tmscm_to_array_double (tmscm p) {
  array<double> a;
  while (!tmscm_is_null (p)) {
    a << ((double) tmscm_to_double (tmscm_car (p)));
    p= tmscm_cdr (p);
  }
  return a;
}

bool
tmscm_is_array_array_double (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_array_double (tmscm_car (p)) &&
           tmscm_is_array_array_double (tmscm_cdr (p));
}

tmscm
array_array_double_to_tmscm (array<array_double> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (array_double_to_tmscm (a[i]), p);
  return p;
}

array<array_double>
tmscm_to_array_array_double (tmscm p) {
  array<array_double> a;
  while (!tmscm_is_null (p)) {
    a << ((array_double) tmscm_to_array_double (tmscm_car (p)));
    p= tmscm_cdr (p);
  }
  return a;
}

bool
tmscm_is_array_array_array_double (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_array_array_double (tmscm_car (p)) &&
           tmscm_is_array_array_array_double (tmscm_cdr (p));
}

tmscm
array_array_array_double_to_tmscm (array<array_array_double> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (array_array_double_to_tmscm (a[i]), p);
  return p;
}

array<array_array_double>
tmscm_to_array_array_array_double (tmscm p) {
  array<array_array_double> a;
  while (!tmscm_is_null (p)) {
    a << ((array_array_double) tmscm_to_array_array_double (tmscm_car (p)));
    p= tmscm_cdr (p);
  }
  return a;
}

tmscm
array_string_to_tmscm (array<string> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (string_to_tmscm (a[i]), p);
  return p;
}

array<string>
tmscm_to_array_string (tmscm p) {
  array<string> a;
  while (!tmscm_is_null (p)) {
    a << tmscm_to_string (tmscm_car (p));
    p= tmscm_cdr (p);
  }
  return a;
}

bool
tmscm_is_array_tree (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_tree (tmscm_car (p)) &&
           tmscm_is_array_tree (tmscm_cdr (p));
}

tmscm
array_tree_to_tmscm (array<tree> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (tree_to_tmscm (a[i]), p);
  return p;
}

array<tree>
tmscm_to_array_tree (tmscm p) {
  array<tree> a;
  while (!tmscm_is_null (p)) {
    a << tmscm_to_tree (tmscm_car (p));
    p= tmscm_cdr (p);
  }
  return a;
}

bool
tmscm_is_array_path (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_path (tmscm_car (p)) &&
           tmscm_is_array_path (tmscm_cdr (p));
}

tmscm
array_path_to_tmscm (array<path> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (path_to_tmscm (a[i]), p);
  return p;
}

array<path>
tmscm_to_array_path (tmscm p) {
  array<path> a;
  while (!tmscm_is_null (p)) {
    a << tmscm_to_path (tmscm_car (p));
    p= tmscm_cdr (p);
  }
  return a;
}
