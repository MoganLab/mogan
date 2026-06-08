
/******************************************************************************
 * MODULE     : tree_modify.cpp
 * DESCRIPTION: high level tree modification subroutines
 * COPYRIGHT  : (C) 2010  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tree_modify.hpp"
#include "observers.hpp"
#include "path.hpp"
#include "tree_helper.hpp"
#include "tree_observer.hpp"
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using moebius::drd::the_drd;

extern tree the_et;

/******************************************************************************
 * Routines for simplification and correction
 ******************************************************************************/

static void
simplify_concat (tree& r, tree t) {
  if (is_atomic (t)) {
    if (N (r) > 0 && is_atomic (r[N (r) - 1]))
      r[N (r) - 1]= tree (r[N (r) - 1]->label * t->label);
    else r << t;
    return;
  }
  int i, n= N (t);
  for (i= 0; i < n; i++)
    if (is_concat (t[i])) simplify_concat (r, t[i]);
    else if (t[i] == "")
      ;
    else if (is_document (t[i]) && N (t[i]) == 1) simplify_concat (r, t[i][0]);
    else if (is_atomic (t[i]) && (N (r) > 0) && is_atomic (r[N (r) - 1]))
      r[N (r) - 1]= tree (r[N (r) - 1]->label * t[i]->label);
    else r << t[i];
}

tree
simplify_concat (tree t) {
  if (is_atomic (t)) return t;
  tree r (CONCAT);
  simplify_concat (r, t);
  if (N (r) == 0) return "";
  if (N (r) == 1) return r[0];
  return r;
}

static void
simplify_document (tree& r, tree t) {
  int i, n= N (t);
  for (i= 0; i < n; i++)
    if (is_document (t[i])) simplify_document (r, t[i]);
    else r << t[i];
}

tree
simplify_document (tree t) {
  if (!is_document (t)) return t;
  tree r (DOCUMENT);
  simplify_document (r, t);
  return r;
}

tree
simplify_correct (tree t) {
  if (is_atomic (t)) return t;
  if (is_func (t, QUOTE, 1) && (is_atomic (t[0]))) return t[0];
  int  i, n= N (t);
  tree r (t, n);
  for (i= 0; i < n; i++)
    r[i]= simplify_correct (t[i]);
  if (is_concat (r)) r= simplify_concat (r);
  if (is_document (r)) r= simplify_document (r);
  return r;
}

/******************************************************************************
 * DRD-based correction of trees
 ******************************************************************************/

void
correct_concat_node (tree& t, int done) {
  // cout << "Correct " << t << ", " << done << "\n";
  int i, n= N (t);
  if (n == 0) {
    assign (t, "");
    return;
  }
  for (i= done; i < n; i++) {
    if (t[i] == "") {
      remove (t, i, 1);
      correct_concat_node (t, i);
      return;
    }
    if ((i < n - 1) && is_atomic (t[i]) && is_atomic (t[i + 1])) {
      join (t, i);
      correct_concat_node (t, i);
      return;
    }
    if (is_concat (t[i])) {
      insert_node (t, 0, CONCAT);
      split (t, 0, i);
      split (t, 1, 1);
      remove_node (t[1], 0);
      if (t[0] == tree (CONCAT)) remove (t, 0, 1);
      else join (t, 0);
      if (t[1] == tree (CONCAT)) remove (t, 1, 1);
      else join (t, 0);
      remove_node (t, 0);
      correct_concat_node (t, max (i - 1, 0));
      return;
    }
  }
}

void
correct_node (tree& t) {
  // NOTE: this routine should only modify t and its descendants,
  // but not any ancestors
  if (is_compound (t)) {
    if (the_drd->contains (as_string (L (t))) &&
        !the_drd->correct_arity (L (t), N (t)))
      assign (t, "");
    if (is_concat (t)) correct_concat_node (t, 0);
  }
}

void
correct_downwards (tree& t) {
  if (is_compound (t))
    for (int i= 0; i < N (t); i++)
      correct_downwards (t[i]);
  correct_node (t);
}

void
correct_upwards (tree& t) {
  correct_node (t);
  path ip= obtain_ip (t);
  if (ip_attached (ip) && !is_nil (ip))
    correct_upwards (subtree (the_et, reverse (ip->next)));
}
