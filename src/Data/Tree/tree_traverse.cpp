
/******************************************************************************
 * MODULE     : tree_traverse.cpp
 * DESCRIPTION: abstract cursor movement and tree traversal
 * COPYRIGHT  : (C) 2005  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tree_traverse.hpp"
#include "analyze.hpp"
#include "converter.hpp"
#include "cork.hpp"
#include "hashset.hpp"
#include "scheme.hpp"
#include "tree_helper.hpp"

#include <moebius/drd/drd_mode.hpp>
#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::drd::drd_decode_type;
using moebius::drd::set_access_mode;
using moebius::drd::the_drd;

/******************************************************************************
 * Accessability
 ******************************************************************************/

bool
is_macro (tree_label l) {
  return the_drd->get_var_type (l) != VAR_PARAMETER;
}

bool
is_parameter (tree_label l) {
  return the_drd->get_var_type (l) != VAR_MACRO;
}

string
get_tag_type (tree_label l) {
  return drd_decode_type (the_drd->get_type (l));
}

array<string>
get_all_primitives () {
  return moebius::get_all_primitives ();
}

int
minimal_arity (tree_label l) {
  return the_drd->get_minimal_arity (l);
}

int
maximal_arity (tree_label l) {
  return the_drd->get_maximal_arity (l);
}

bool
correct_arity (tree_label l, int n) {
  return the_drd->correct_arity (l, n);
}

int
minimal_arity (tree t) {
  return the_drd->get_minimal_arity (L (t));
}

int
maximal_arity (tree t) {
  return the_drd->get_maximal_arity (L (t));
}

bool
correct_arity (tree t, int n) {
  return the_drd->correct_arity (L (t), n);
}

int
insert_point (tree t, int i) {
  return the_drd->insert_point (L (t), i, N (t));
}

bool
is_dynamic (tree t) {
  return the_drd->is_dynamic (t, false);
}

bool
is_accessible_child (tree t, int i) {
  return the_drd->is_accessible_child (t, i);
}

array<tree>
accessible_children (tree t) {
  array<tree> a;
  int         i, n= N (t);
  for (i= 0; i < n; i++)
    if (the_drd->is_accessible_child (t, i)) a << t[i];
  return a;
}

bool
all_accessible (tree t) {
  if (is_atomic (t)) return false;
  return the_drd->all_accessible (L (t));
}

bool
none_accessible (tree t) {
  if (is_atomic (t)) return false;
  return the_drd->none_accessible (L (t));
}

bool
exists_accessible_inside (tree t) {
  if (is_atomic (t)) return true;
  if (!the_drd->is_child_enforcing (t)) return true;
  for (int i= 0; i < N (t); i++)
    if (the_drd->is_accessible_child (t, i) && exists_accessible_inside (t[i]))
      return true;
  return false;
}

/******************************************************************************
 * Further properties
 ******************************************************************************/

string
get_name (tree t) {
  return the_drd->get_name (L (t));
}

string
get_long_name (tree t) {
  return the_drd->get_long_name (L (t));
}

string
get_child_name (tree t, int i) {
  return the_drd->get_child_name (t, i);
}

string
get_child_long_name (tree t, int i) {
  return the_drd->get_child_long_name (t, i);
}

string
get_child_type (tree t, int i) {
  return drd_decode_type (the_drd->get_type_child (t, i));
}

tree
get_env_child (tree t, int i, string var, tree val) {
  return the_drd->get_env_child (t, i, var, val);
}

tree
get_env_child (tree t, int i, tree env) {
  return the_drd->get_env_child (t, i, env);
}

tree
get_env_descendant (tree t, path p, tree env) {
  return the_drd->get_env_descendant (t, p, env);
}

tree
get_env_descendant (tree t, path p, string var, tree val) {
  return the_drd->get_env_descendant (t, p, var, val);
}

/******************************************************************************
 * Traversal of a tree
 ******************************************************************************/

static path
move_any (tree t, path p, bool forward) {
  path q = path_up (p);
  int  l = last_item (p);
  tree st= subtree (t, q);
  if (!is_nil (q) && is_func (subtree (t, path_up (q)), RAW_DATA)) {
    if (forward) return path_up (q) * 1;
    else return path_up (q) * 0;
  }
  if (is_atomic (st)) {
    string s= st->label;
#ifdef SANITY_CHECKS
    ASSERT (l >= 0 && l <= N (s), "out of range");
#else
    l= max (min (l, N (s)), 0);
#endif
    if (forward) {
      if (l < N (s)) {
        tm_char_forwards (s, l);
        return q * l;
      }
    }
    else {
      if (l > 0) {
        tm_char_backwards (s, l);
        return q * l;
      }
    }
  }
  else if ((forward && l == 0) || (!forward && l == 1)) {
    int i, n= N (st);
    if (forward) {
      for (i= 0; i < n; i++)
        if (the_drd->is_accessible_child (st, i)) return q * path (i, 0);
    }
    else {
      for (i= n - 1; i >= 0; i--)
        if (the_drd->is_accessible_child (st, i))
          return q * path (i, right_index (st[i]));
    }
    return q * (1 - l);
  }
  else if (is_nil (q)) return p;

  l = last_item (q);
  q = path_up (q);
  st= subtree (t, q);
  int i, n= N (st);
  if (forward) {
    for (i= l + 1; i < n; i++)
      if (the_drd->is_accessible_child (st, i)) return q * path (i, 0);
  }
  else {
    for (i= l - 1; i >= 0; i--)
      if (the_drd->is_accessible_child (st, i)) {
        return q * path (i, right_index (st[i]));
      }
  }
  return q * (forward ? 1 : 0);
}

path
next_any (tree t, path p) {
  return move_any (t, p, true);
}
path
previous_any (tree t, path p) {
  return move_any (t, p, false);
}

/******************************************************************************
 * Traversal of a valid cursor positions inside a tree
 ******************************************************************************/

path
closest_up (tree t, path p) {
  if (is_atomic (t)) {
    if (is_nil (p)) return path (0);
    else return path (max (0, min (p->item, N (t->label))));
  }
  else if (is_concat (t) || is_document (t)) {
    if (N (t) == 0) return path (0);
    else if (is_nil (p) || is_atom (p) || p->item < 0)
      return path (0, closest_up (t[0], path ()));
    else if (p->item >= N (t))
      return path (N (t) - 1, closest_up (t[N (t) - 1], path ()));
    else return path (p->item, closest_up (t[p->item], p->next));
  }
  else {
    if (is_nil (p)) return path (0);
    else if (is_atom (p)) return path (max (0, min (p->item, 1)));
    else if (p->item < 0) return path (0);
    else if (p->item >= N (t)) return path (1);
    else return path (p->item, closest_up (t[p->item], p->next));
  }
}

static path
move_valid_sub (tree t, path p, bool forward) {
#ifdef SANITY_CHECKS
  ASSERT (is_inside (t, p), "invalid cursor [move_valid]");
#else
  if (!is_inside (t, p)) p= closest_up (t, p);
#endif
  path q= p;
  while (true) {
    path r= move_any (t, q, forward);
    if (r == q) return p;
    if (valid_cursor (t, r)) return r;
    q= r;
  }
}

static path
move_valid_bis (tree t, path p, bool forward) {
  bool inside= the_drd->is_accessible_path (t, p);
  if (inside) return move_valid_sub (t, p, forward);
  bool old_mode= set_access_mode (DRD_ACCESS_SOURCE);
  path r       = move_valid_sub (t, p, forward);
  set_access_mode (old_mode);
  return r;
}

static bool
inside_graphics (tree t, path p) {
  if (is_func (t, GRAPHICS)) return true;
  if (is_nil (p) || is_nil (p->next)) return false;
  return inside_graphics (t[p->item], p->next);
}

static path
move_valid (tree t, path p, bool forward) {
  // NOTE: extra hook for moving inside graphical text
  path q= move_valid_bis (t, p, forward);
  if (!inside_graphics (t, p)) return q;
  if (inside_contiguous_document (t, p, q)) return q;
  return false;
}

path
next_valid (tree t, path p) {
  return move_valid (t, p, true);
}
path
previous_valid (tree t, path p) {
  return move_valid (t, p, false);
}

static path
move_accessible (tree t, path p, bool forward) {
#ifdef SANITY_CHECKS
  ASSERT (is_inside (t, p), "invalid cursor [move_accessible]");
#else
  if (!is_inside (t, p)) p= closest_up (t, p);
#endif
  path q= p;
  while (true) {
    path r= move_any (t, q, forward);
    if (r == q) return p;
    if (is_accessible_cursor (t, r)) return r;
    q= r;
  }
}

path
next_accessible (tree t, path p) {
  return move_accessible (t, p, true);
}
path
previous_accessible (tree t, path p) {
  return move_accessible (t, p, false);
}

/******************************************************************************
 * Word based traversal of a tree
 ******************************************************************************/

static inline bool
is_iso_alphanum (char c) {
  return is_iso_alpha (c) || is_digit (c);
}

// Extract a single TeXmacs character at byte position `pos`
// and return a coarse Unicode codepoint for boundary checks.
static inline bool
tm_codepoint_at (string s, int pos, unsigned int& code) {
  if (pos < 0 || pos >= N (s)) return false;

  int i= pos;
  tm_char_forwards (s, i);
  string c= s (pos, i);
  if (c == "") return false;

  // Plain ASCII
  if (N (c) == 1) {
    code= (unsigned int) (unsigned char) c[0];
    return true;
  }

  // TeXmacs internal hexadecimal form: <#....>
  // Only parse leading hex digits for block-level checks.
  if (starts (c, "<#") && ends (c, ">")) {
    unsigned int v= 0;
    int          k= 2, cnt= 0;
    for (; k < N (c) - 1 && cnt < 4; k++, cnt++) {
      char         ch= c[k];
      unsigned int d;
      if (ch >= '0' && ch <= '9') d= (unsigned int) (ch - '0');
      else if (ch >= 'a' && ch <= 'f') d= 10u + (unsigned int) (ch - 'a');
      else if (ch >= 'A' && ch <= 'F') d= 10u + (unsigned int) (ch - 'A');
      else break;
      v= (v << 4) | d;
    }
    if (cnt > 0) {
      code= v;
      return true;
    }
  }

  // Unknown non-ASCII character
  code= 0xFFFFFFFFu;
  return true;
}

// Return true if the codepoint should be treated as a word separator
// for word-based cursor movement.
static inline bool
is_word_separator (unsigned int c) {
  // ASCII whitespace
  if (c == 0x20u || c == 0x09u || c == 0x0Au || c == 0x0Du) return true;

  if (c == 0xFFFFFFFFu) return false;

  // ASCII punctuation
  if (c < 0x80u) {
    char ch= (char) c;
    return (!is_iso_alphanum (ch) && ch != '_');
  }

  // Unicode punctuation blocks
  if (c >= 0x2000u && c <= 0x206Fu) return true; // General Punctuation
  if (c >= 0x3000u && c <= 0x303Fu) return true; // CJK Symbols & Punctuation
  if (c >= 0xFF00u && c <= 0xFFEFu) return true; // Fullwidth / Halfwidth

  return false;
}

static bool
at_border (tree t, path p, bool forward) {
  tree st= subtree (t, path_up (p));
  int  l= last_item (p), n= N (st);
  if (!is_concat (st) && !is_document (st)) return true;
  if ((forward && l != n - 1) || (!forward && l != 0)) return false;
  return at_border (t, path_up (p), forward);
}

static bool
next_is_word (tree t, path p) {
  tree st= subtree (t, path_up (p));
  int  l= last_item (p), n= N (st);
  if (!is_concat (st) || l + 1 >= n) return false;
  if (is_compound (st[l + 1])) return true;
  return st[l + 1] != "" && is_iso_alphanum (st[l + 1]->label[0]);
}

static path
move_word (tree t, path p, bool forward) {
  while (true) {
    path q= move_accessible (t, p, forward);
    int  l= last_item (q);
    if (q == p) return p;
    tree st= subtree (t, path_up (q));
    if (is_atomic (st)) {
      string s= st->label;
      int    n= N (s);
      if (s == "") return q;

      unsigned int cl= 0, cr= 0;
      bool         has_l= false, has_r= false;

      // Character immediately before the cursor
      if (l > 0) {
        int lp= l;
        tm_char_backwards (s, lp);
        has_l= tm_codepoint_at (s, lp, cl);
      }

      // Character at the cursor
      if (l < n) {
        has_r= tm_codepoint_at (s, l, cr);
      }

      bool lsep = (!has_l) ? true : is_word_separator (cl);
      bool rsep = (!has_r) ? true : is_word_separator (cr);
      bool lword= has_l && !lsep;
      bool rword= has_r && !rsep;

      // Forward word movement
      if (forward && l > 0) {
        if ((lword || rsep || (l == n && at_border (t, path_up (q), true))) &&
            (l == n || !rword))
          return q;
      }

      // Backward word movement
      if (!forward && l < n) {
        if ((rword || lsep || (l == 0 && at_border (t, path_up (q), false))) &&
            (l == 0 || !lword))
          return q;
      }

      // Preserve original special case
      if (!forward && l == n && next_is_word (t, path_up (q))) return q;
    }
    else {
      // Structural traversal (unchanged)
      if (forward && l == 1) return q;
      if (!forward && l == 0) return q;
      if (!forward && next_is_word (t, path_up (q))) return q;
    }
    p= q;
  }
}

path
next_word (tree t, path p) {
  return move_word (t, p, true);
}
path
previous_word (tree t, path p) {
  return move_word (t, p, false);
}

/******************************************************************************
 * Node based traversal of a tree
 ******************************************************************************/

static path
move_node (tree t, path p, bool forward) {
  tree st= subtree (t, path_up (p));
  if (is_atomic (st)) {
    if (forward) p= path_up (p) * N (st->label);
    else p= path_up (p) * 0;
  }
  p = move_accessible (t, p, forward);
  st= subtree (t, path_up (p));
  if (is_atomic (st) && last_item (p) > 0) p= path_up (p) * N (st->label);
  return p;
}

path
next_node (tree t, path p) {
  return move_node (t, p, true);
}
path
previous_node (tree t, path p) {
  return move_node (t, p, false);
}

/******************************************************************************
 * Tag based traversal of a tree
 ******************************************************************************/

static int
tag_border (tree t, path p) {
  if (none_accessible (subtree (t, path_up (p)))) {
    if (last_item (p) == 0) return -1;
    if (last_item (p) == 1) return 1;
  }
  return 0;
}

static bool
distinct_tag_or_argument (tree t, path p, path q, hashset<int> labs) {
  path c= common (p, q);
  path r= path_up (q);
  if (labs->contains ((int) L (subtree (t, r))) && tag_border (t, q) > 0)
    return true;
  while (!is_nil (r) && (r != c)) {
    r= path_up (r);
    if (labs->contains ((int) L (subtree (t, r))) &&
        !none_accessible (subtree (t, r)))
      return true;
  }
  return false;
}

static bool
acceptable_border (tree t, path p, path q, hashset<int> labs) {
  if (tag_border (t, q) == 0) return true;
  if (!labs->contains ((int) L (subtree (t, path_up (q))))) return true;
  if (tag_border (t, q) < 0) return false;
  return tag_border (t, p) > 0;
}

static bool
acceptable_child (tree t, path p, hashset<int> labs) {
  tree st= subtree (t, path_up (p));
  if (is_compound (st) && labs->contains ((int) L (st)))
    if (last_item (p) == 0 || last_item (p) == 1) return true;
  p= path_up (p);
  while (!is_nil (p)) {
    st= subtree (t, path_up (p));
    if (labs->contains ((int) L (st)))
      if (is_accessible_child (st, last_item (p))) return true;
    p= path_up (p);
  }
  return false;
}

static int
tag_index (tree t, path p, hashset<int> labs) {
  p= path_up (p);
  while (!is_nil (p)) {
    if (labs->contains ((int) L (subtree (t, path_up (p)))))
      return last_item (p);
    else p= path_up (p);
  }
  return -1;
}

static bool
tag_inside (tree t, path p, hashset<int> labs) {
  return !labs->contains ((int) L (subtree (t, path_up (p))));
}

static path
move_tag (tree t, path p, hashset<int> labs, bool forward, bool preserve) {
  path q= p;
  while (true) {
    path r= move_node (t, q, forward);
    if (r == q) return p;
    if (!tag_inside (t, r, labs) && !tag_inside (t, p, labs) &&
        last_item (r) == last_item (p))
      return r;
    if (distinct_tag_or_argument (t, p, r, labs) &&
        acceptable_border (t, p, r, labs) && acceptable_child (t, r, labs) &&
        tag_inside (t, r, labs) == tag_inside (t, p, labs) &&
        (!preserve || tag_index (t, r, labs) == tag_index (t, p, labs)))
      return r;
    q= r;
  }
}

static hashset<int>
get_labels (scheme_tree t) {
  hashset<int> labs;
  if (is_atomic (t)) labs->insert ((int) as_tree_label (t->label));
  else {
    int i, n= N (t);
    for (i= 0; i < n; i++)
      if (is_atomic (t[i])) labs->insert ((int) as_tree_label (t[i]->label));
  }
  return labs;
}

path
next_tag (tree t, path p, scheme_tree which) {
  return move_tag (t, p, get_labels (which), true, false);
}
path
previous_tag (tree t, path p, scheme_tree which) {
  return move_tag (t, p, get_labels (which), false, false);
}

path
next_tag_same_argument (tree t, path p, scheme_tree which) {
  return move_tag (t, p, get_labels (which), true, true);
}
path
previous_tag_same_argument (tree t, path p, scheme_tree which) {
  return move_tag (t, p, get_labels (which), false, true);
}

/******************************************************************************
 * Traverse the children of a node
 ******************************************************************************/

static path
move_argument (tree t, path p, bool forward) {
  path q = path_up (p);
  int  l = last_item (p);
  tree st= subtree (t, q);
  int  i, n= N (st);
  if (forward) {
    for (i= l + 1; i < n; i++)
      if (the_drd->is_accessible_child (st, i)) return start (t, q * i);
  }
  else {
    for (i= l - 1; i >= 0; i--)
      if (the_drd->is_accessible_child (st, i)) return end (t, q * i);
  }
  return path ();
}

path
next_argument (tree t, path p) {
  return move_argument (t, p, true);
}
path
previous_argument (tree t, path p) {
  return move_argument (t, p, false);
}

/******************************************************************************
 * Other routines
 ******************************************************************************/

path
search_common (tree t, path p, path q, tree_label which) {
  if (is_atomic (t)) return path ();
  if (is_nil (p) || is_nil (q) || p->item != q->item) return path ();
  if (0 > p->item || p->item >= N (t)) return path ();
  path c= search_common (t[p->item], p->next, q->next, which);
  if (L (subtree (t[p->item], c)) == which) return path (p->item, c);
  return path ();
}

bool
no_other (tree t, path p, tree_label which) {
  if (L (t) == which) return false;
  if (is_nil (p)) return true;
  if (0 > p->item || p->item >= N (t)) return false;
  return no_other (t[p->item], p->next, which);
}

bool
inside_same (tree t, path p, path q, tree_label which) {
  path d= search_common (t, path_up (p), path_up (q), which);
  if (L (subtree (t, d)) != which) return true;
  path sp= p / d;
  path sq= q / d;
  if (is_atom (sp) || is_atom (sq)) return false;
  if (sp->item != sq->item) return false;
  if (0 > sp->item || sp->item >= N (subtree (t, d))) return false;
  tree st= subtree (t, d)[sp->item];
  return no_other (st, path_up (sq->next), which) ||
         no_other (st, path_up (sp->next), which);
}

bool
inside_same_or_more (tree t, path p, path q, tree_label which) {
  path d= search_common (t, path_up (p), path_up (q), which);
  if (L (subtree (t, d)) != which) return true;
  path sp= p / d;
  path sq= q / d;
  if (is_atom (sp) || is_atom (sq)) return false;
  if (sp->item != sq->item) return false;
  if (0 > sp->item || sp->item >= N (subtree (t, d))) return false;
  tree st= subtree (t, d)[sp->item];
  return no_other (st, path_up (sq->next), which);
}

bool
is_boundary (tree t, path p) {
  if (is_func (subtree (t, p), DOCUMENT)) return true;
  if (is_func (subtree (t, p), GRAPHICS)) return true;
  return false;
}

bool
inside_contiguous_document (tree t, path op, path oq) {
  if (!inside_same (t, op, oq, DOCUMENT)) return false;
  path p= path_up (op), q= path_up (oq);
  while (!is_nil (p) && !is_boundary (t, p))
    p= path_up (p);
  while (!is_nil (q) && !is_boundary (t, q))
    q= path_up (q);
  if (p == q) return true;
  if (q <= p) return inside_contiguous_document (t, oq, op);
  if (!(p <= q)) return false;
  if (path_less (op, oq)) return (oq / q) == start (subtree (t, q));
  else return (oq / q) == end (subtree (t, q));
}

/******************************************************************************
 * Find sections in document
 ******************************************************************************/

hashset<tree_label> section_traverse_tags;
hashset<tree_label> section_tags;

void
init_sections () {
  if (N (section_traverse_tags) == 0) {
    section_traverse_tags= hashset<tree_label> ();
    section_traverse_tags->insert (DOCUMENT);
    section_traverse_tags->insert (CONCAT);
    section_traverse_tags->insert (make_tree_label ("ignore"));
    section_traverse_tags->insert (make_tree_label ("show-part"));
    section_traverse_tags->insert (make_tree_label ("hide-part"));
    section_traverse_tags->insert (make_tree_label ("live-io*"));
  }
  if (N (section_tags) == 0) {
    eval ("(use-modules (text text-drd))");
    object l= eval ("(append (section-tag-list) (section*-tag-list))");
    while (!is_null (l)) {
      tree_label tl= as_tree_label (as_symbol (car (l)));
      if (tl != UNKNOWN) {
        section_tags->insert (tl);
      }
      l= cdr (l);
    }
  }
}

path
previous_section_impl (tree t, path p) {
  if (is_atomic (t)) return path ();
  else if (N (t) == 1 && section_tags->contains (L (t))) return p * 0;
  else if (is_compound (t, "live-io*") && !is_nil (p) && p->item == 2)
    return path (2, previous_section_impl (t[2], p->next));
  else if (section_traverse_tags->contains (L (t))) {
    int i= is_nil (p) ? N (t) - 1 : p->item;
    for (; i >= 0; i--) {
      if (!is_nil (p) && i == p->item) {
        path r= previous_section_impl (t[i], p->next);
        if (!is_nil (r)) return path (i, r);
      }
      else {
        path r= previous_section_impl (t[i], path ());
        if (!is_nil (r)) return path (i, r);
      }
    }
  }
  return path ();
}

path
previous_section (tree t, path p) {
  init_sections ();
  path r= previous_section_impl (t, p);
  if (is_nil (r)) return p;
  return path_up (r);
}

void
search_sections (array<tree>& a, tree t) {
  if (is_atomic (t)) return;
  else if (N (t) == 1 && section_tags->contains (L (t))) a << t;
  else if (section_traverse_tags->contains (L (t))) {
    for (tree subtree : t)
      search_sections (a, subtree);
  }
}

array<tree>
search_sections (tree t) {
  init_sections ();
  array<tree> a;
  search_sections (a, t);
  return a;
}

tree
tree_utf8_to_herk (tree_u8 t) {
  if (is_atomic (t)) {
    return tree (utf8_to_herk (t->label));
  }
  else if (!is_func (t, RAW_DATA)) {
    int  t_N= N (t);
    tree t2 (t, t_N);
    for (int i= 0; i < t_N; i++)
      t2[i]= tree_utf8_to_herk (t[i]);
    return t2;
  }
  else {
    return t;
  }
}

tree_u8
tree_herk_to_utf8 (tree t) {
  if (is_atomic (t)) {
    return tree (herk_to_utf8 (t->label));
  }
  else if (!is_func (t, RAW_DATA)) {
    int  t_N= N (t);
    tree t2 (t, t_N);
    for (int i= 0; i < t_N; i++)
      t2[i]= tree_herk_to_utf8 (t[i]);
    return t2;
  }
  else {
    return t;
  }
}
