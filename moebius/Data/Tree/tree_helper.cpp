
/******************************************************************************
 * MODULE     : tree_helper.cpp
 * DESCRIPTION: helpers of trees
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tree_helper.hpp"
#include "analyze.hpp"
#include "modification.hpp"

using namespace moebius;

tree_label
L (modification mod) {
  ASSERT (mod->k == MOD_ASSIGN_NODE, "assign_node modification expected");
  return L (mod->t);
}

/******************************************************************************
 * Compound trees
 ******************************************************************************/

tree
compound (string s) {
  return tree (make_tree_label (s));
}

tree
compound (string s, tree t1) {
  return tree (make_tree_label (s), t1);
}

tree
compound (string s, tree t1, tree t2) {
  return tree (make_tree_label (s), t1, t2);
}

tree
compound (string s, tree t1, tree t2, tree t3) {
  return tree (make_tree_label (s), t1, t2, t3);
}

tree
compound (string s, tree t1, tree t2, tree t3, tree t4) {
  return tree (make_tree_label (s), t1, t2, t3, t4);
}

tree
compound (string s, tree t1, tree t2, tree t3, tree t4, tree t5) {
  return tree (make_tree_label (s), t1, t2, t3, t4, t5);
}

tree
compound (string s, tree t1, tree t2, tree t3, tree t4, tree t5, tree t6) {
  return tree (make_tree_label (s), t1, t2, t3, t4, t5, t6);
}

tree
compound (string s, array<tree> a) {
  return tree (make_tree_label (s), a);
}

bool
is_compound (tree t, string s) {
  return as_string (L (t)) == s;
}

bool
is_compound (tree t, string s, int n) {
  return (as_string (L (t)) == s) && (N (t) == n);
}

/******************************************************************************
 * Tree predicates
 ******************************************************************************/

bool
is_document (tree t) {
  return L (t) == DOCUMENT;
}

bool
is_snippet (tree doc) {
  if (!is_document (doc)) return true;
  int i, n= N (doc);
  for (i= 0; i < n; i++)
    if (is_compound (doc[i], "TeXmacs", 1)) return false;
  return true;
}

bool
is_concat (tree t) {
  return L (t) == CONCAT;
}

bool
is_format (tree t) {
  return is_document (t) || is_concat (t);
}

bool
is_formatting (tree t) {
  return (L (t) >= WITH_LIMITS) && (L (t) <= NEW_DPAGE);
}

bool
is_table (tree t) {
  return is_func (t, TABLE) || is_func (t, SUBTABLE) || is_func (t, ROW) ||
         is_func (t, CELL);
}

bool
is_table_format (tree t) {
  return is_func (t, TFORMAT);
}

bool
is_multi_paragraph (tree t) {
  switch (L (t)) {
  case DOCUMENT:
    return true;
  case SURROUND:
    return is_multi_paragraph (t[2]);
  case DATOMS:
  case DLINES:
  case DPAGES:
  case WITH:
  case MARK:
  case EXPAND_AS:
  case STYLE_WITH:
  case VAR_STYLE_WITH:
  case STYLE_ONLY:
  case VAR_STYLE_ONLY:
  case ACTIVE:
  case VAR_ACTIVE:
    return is_multi_paragraph (t[N (t) - 1]);
  case VAR_INCLUDE:
    return true;
  case WITH_PACKAGE:
    return is_multi_paragraph (t[N (t) - 1]);
  case LOCUS:
  case CANVAS:
    return is_multi_paragraph (t[N (t) - 1]);
  default: {
    static hashset<tree_label> inline_set; // FIXME: use drd
    if (N (inline_set) == 0) {
      inline_set->insert (make_tree_label ("footnote"));
      inline_set->insert (make_tree_label ("footnote-anchor"));
      inline_set->insert (make_tree_label ("note-footnote"));
      inline_set->insert (make_tree_label ("note-footnote*"));
      inline_set->insert (make_tree_label ("folded-comment"));
      inline_set->insert (make_tree_label ("script-input"));
      inline_set->insert (make_tree_label ("converter-input"));
    }
    if (L (t) < START_EXTENSIONS) return false;
    else if (inline_set->contains (L (t))) return false;
    else {
      int i, n= N (t);
      for (i= 0; i < n; i++)
        if (is_multi_paragraph (t[i])) return true;
      return false;
    }
  }
  }
}

bool
is_around (tree t) {
  return is_func (t, AROUND, 3) || is_func (t, VAR_AROUND, 3);
}

bool
is_script (tree t) {
  return is_func (t, LSUB) || is_func (t, LSUP) || is_func (t, RSUB) ||
         is_func (t, RSUP);
}

bool
is_script (tree t, bool& right) {
  if (is_func (t, LSUB) || is_func (t, LSUP)) {
    right= false;
    return true;
  }
  if (is_func (t, RSUB) || is_func (t, RSUP)) {
    right= true;
    return true;
  }
  return false;
}

bool
is_prime (tree t) {
  return ((L (t) == LPRIME) || (L (t) == RPRIME)) && (N (t) == 1);
}

bool
is_left_script_prime (tree t) {
  return is_func (t, LSUB, 1) || is_func (t, LSUP, 1) || is_func (t, LPRIME, 1);
}

bool
is_right_script_prime (tree t) {
  return is_func (t, RSUB, 1) || is_func (t, RSUP, 1) || is_func (t, RPRIME, 1);
}

bool
is_mod_active (tree t) {
  return (N (t) == 1) && (L (t) >= STYLE_ONLY) && (L (t) <= VAR_INACTIVE);
}

bool
is_mod_active_once (tree t) {
  return (N (t) == 1) &&
         ((L (t) == STYLE_ONLY) || (L (t) == ACTIVE) || (L (t) == INACTIVE));
}

bool
is_graphical_text (tree t) {
  return is_func (t, TEXT_AT) || is_func (t, MATH_AT) ||
         is_func (t, DOCUMENT_AT);
}

bool
is_empty (tree t) {
  if (is_atomic (t)) return (t == "");
  if (is_document (t) || is_concat (t)) {
    int i, n= N (t);
    for (i= 0; i < n; i++)
      if (!is_empty (t[i])) return false;
    return is_concat (t) || (n <= 1);
  }
  return is_compound (t, "suppressed");
}

bool
is_multi_line (tree t) {
  if (is_atomic (t)) return false;
  else if (is_func (t, DOCUMENT)) return true;
  else if (is_func (t, CONCAT) || is_func (t, TABLE)) return false;
  else {
    int i, n= N (t);
    for (i= 0; i < n; i++)
      if (is_multi_line (t[i])) return true;
    return false;
  }
}

bool
is_extension (tree_label l) {
  return l >= START_EXTENSIONS;
}

bool
is_extension (tree t) {
  return L (t) >= START_EXTENSIONS;
}

bool
is_extension (tree t, int n) {
  return (L (t) >= START_EXTENSIONS) && (N (t) == n);
}

tree
freeze (tree t) {
  if (is_atomic (t)) return copy (t->label);
  if (is_func (t, UNFREEZE, 1)) return t[0];
  else {
    int  i, n= N (t);
    tree r (t, n);
    for (i= 0; i < n; i++)
      r[i]= freeze (t[i]);
    return r;
  }
}

string
tree_as_string (tree t) {
  if (is_atomic (t)) return t->label;
  else if (is_concat (t) || is_document (t)) {
    int    i, n= N (t);
    string cumul;
    for (i= 0; i < n; i++)
      cumul << tree_as_string (t[i]);
    return cumul;
  }
  else if (is_func (t, WITH)) return tree_as_string (t[N (t) - 1]);
  else if (is_compound (t, "nbsp", 0)) return " ";
  return "";
}

void
print_tree (tree t, int tab) {
  int i;
  for (i= 0; i < tab; i++)
    cout << " ";
  if (is_atomic (t)) cout << t->label << "\n";
  else {
    cout << as_string (L (t)) << "\n";
    for (i= 0; i < N (t); i++)
      print_tree (t[i], tab + 2);
  }
}

/******************************************************************************
 * Percentages and magnifications
 ******************************************************************************/

bool
is_percentage (tree t, string s) {
  return is_atomic (t) && ends (t->label, s) &&
         is_double (t->label (0, N (t->label) - 1));
}

bool
is_percentage (tree t) {
  return is_percentage (t, "%");
}

double
as_percentage (tree t) {
  return as_double (t->label (0, N (t->label) - 1)) / 100.0;
}

bool
is_magnification (string s) {
  double result;
  if (N (s) == 0) return false;
  for (int i= 0; i < N (s); /*nop*/) {
    if (s[i] == '*') {
      i++;
      read_double (s, i, result);
    }
    else if (s[i] == '/') {
      i++;
      read_double (s, i, result);
    }
    else return false;
  }
  return true;
}

double
get_magnification (string s) {
  int    i   = 0;
  double magn= 1.0, result;
  while (i < N (s)) {
    if (s[i] == '*') {
      i++;
      read_double (s, i, result);
      magn*= result;
    }
    else if (s[i] == '/') {
      i++;
      read_double (s, i, result);
      magn/= result;
    }
    else return magn;
  }
  return magn;
}
