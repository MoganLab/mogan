
/******************************************************************************
 * MODULE     : typesetter.hpp
 * DESCRIPTION: Implementation of the main TeXmacs typesetting routines
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Bridge/impl_typesetter.hpp"
#include "iterator.hpp"

using namespace moebius;

/******************************************************************************
 * Constructor and destructor
 ******************************************************************************/

typesetter_rep::typesetter_rep (edit_env& env2, tree et, path ip)
    : env (env2), old_patch (UNINIT), progressive_enabled (false),
      progressive_target_pages (10), progressive_target_child (-1),
      progressive_cur_child (0), progressive_total_children (0),
      progressive_pending (false), page_text_height (0),
      accumulated_stream_height (0) {
  paper= (env->get_string (PAGE_MEDIUM) == "paper");
  br   = make_bridge (this, et, ip);
  x1= y1= x2= y2= 0;
}

typesetter
new_typesetter (edit_env& env, tree et, path ip) {
  return tm_new<typesetter_rep> (env, et, ip);
}

void
delete_typesetter (typesetter ttt) {
  tm_delete (ttt);
}

/******************************************************************************
 * Output flux
 ******************************************************************************/

void
typesetter_rep::insert_stack (array<page_item> l2, stack_border sb2) {
  merge_stack (l, sb, l2, sb2);
}

void
typesetter_rep::insert_parunit (tree t, path ip) {
  insert_paragraph (t, ip);
}

void
typesetter_rep::insert_paragraph (tree t, path ip) {
  stack_border     temp_sb;
  array<page_item> temp_l= typeset_stack (env, t, ip, a, b, temp_sb);
  insert_stack (temp_l, temp_sb);
}

void
typesetter_rep::insert_surround (array<line_item> a2, array<line_item> b2) {
  a << a2;
  array<line_item> temp_b= b;
  b                      = copy (b2);
  b << temp_b;
}

void
typesetter_rep::insert_marker (tree st, path ip) {
  (void) st;
  // if (!is_multi_paragraph (st)) {
  array<line_item> a2= typeset_marker (env, descend (ip, 0));
  array<line_item> b2= typeset_marker (env, descend (ip, 1));
  insert_surround (a2, b2);
  // }
}

void
typesetter_rep::local_start (array<page_item>& prev_l, stack_border& prev_sb) {
  prev_l = l;
  prev_sb= sb;
  l      = array<page_item> ();
  sb     = stack_border ();
}

void
typesetter_rep::local_end (array<page_item>& prev_l, stack_border& prev_sb) {
  array<page_item> temp_l = l;
  stack_border     temp_sb= sb;
  l                       = prev_l;
  sb                      = prev_sb;
  prev_l                  = temp_l;
  prev_sb                 = temp_sb;
}

/******************************************************************************
 * Main typesetting routines
 ******************************************************************************/

static array<rectangle>
requires_update (array<rectangle> log) {
  array<rectangle> rs;
  int              log_size= N (log);
  for (int i= 0; 2 * i + 1 < log_size; i++) {
    rectangle r1= log[log_size - 2 * i - 1];
    rectangle r2= log[log_size - 2 * i - 2];
    if (r1 == rectangle (0, 0, 0, 0)) rs << r2;
    else if (r2 == rectangle (0, 0, 0, 0)) rs << r1;
    else if (r1 != r2) rs << r2 << r1;
  }
  return rs;
}

void
typesetter_rep::determine_page_references (box b) {
  hashmap<string, tree> h ("?");
  b->collect_page_numbers (h, "?");
  iterator<string> it= iterate (h);
  while (it->busy ()) {
    string var= it->next ();
    tree   val= copy (h[var]);
    tree   old= env->local_ref[var];
    if (is_func (old, TUPLE, 2)) env->local_ref (var)= tuple (old[0], val);
    else if (is_func (old, TUPLE, 3))
      env->local_ref (var)= tuple (old[0], val, old[2]);
    else env->local_ref (var)= tuple (old, val);
    env->touched (var)= true;
  }
}

box
typesetter_rep::typeset () {
  old_patch= hashmap<string, tree> (UNINIT);
  l        = array<page_item> ();
  sb       = stack_border ();
  a        = array<line_item> ();
  b        = array<line_item> ();
  paper    = (env->get_string (PAGE_MEDIUM) == "paper");

  // Determine if progressive typesetting is eligible:
  // Must be paginated ("paper"), not in printed export, and have enough
  // children
  tree st                   = br->st;
  int  n                    = N (st);
  progressive_total_children= n;
  bool is_printing          = (env->get_string (PAGE_PRINTED) == "true");

  if (paper && !is_printing && n >= 60) {
    progressive_enabled= true;
    if (page_text_height <= 0) {
      SI tw, th, w, h, odd, even, top, bot;
      env->get_page_pars (tw, th, w, h, odd, even, top, bot);
      page_text_height= th;
    }
    if (progressive_cur_child == 0) {
      // Initial batch: first 50 paragraphs
      progressive_cur_child= min (n, 50);
    }
  }
  else {
    progressive_enabled  = false;
    progressive_pending  = false;
    progressive_cur_child= n;
  }

  // Test whether we are doing a complete typesetting
  env->complete= br->my_typeset_will_be_complete ();
  int i        = 0;
  if (n > 0) {
    if (is_compound (st[0], "show-preamble")) {
      i++;
      env->complete= false;
    }
    if (is_compound (st[0], "hide-preamble")) i++;
    for (; i < n && env->complete; i++) {
      if (is_compound (st[i], "hide-part")) env->complete= false;
      if (!is_compound (st[i], "show-part")) break;
    }
  }

  // Typeset
  if (env->complete) {
    env->local_aux= hashmap<string, tree> (UNINIT);
    env->missing  = hashmap<string, tree> (UNINIT);
    env->redefined= array<tree> ();
    env->touched  = hashmap<string, bool> (false);
  }
  if (progressive_enabled && progressive_cur_child < n) {
    progressive_pending= true;
    env->complete      = false;
  }
  else {
    progressive_pending= false;
  }
  br->typeset (PROCESSED + WANTED_PARAGRAPH);
  pager ppp= tm_new<pager_rep> (br->ip, env, l);
  box   rb = ppp->make_pages ();
  if (env->complete && paper) determine_page_references (rb);
  tm_delete (ppp);
  // env->complete= false;  // moved to edit_typeset_rep::typeset
  return rb;
}

box
typesetter_rep::typeset (SI& x1b, SI& y1b, SI& x2b, SI& y2b) {
  x1= x1b;
  y1= y1b;
  x2= x2b;
  y2= y2b;

  // For old destroyed boxes, the changed_least_upper_bound will be updated
  box b= typeset ();

  // Append pairs of rectangles to the change_log from the new typesetting box
  array<rectangle> change_log;
  b->position_at (0, 0, change_log, changed_ptr);

  // Keeps the box rectangle in pair <before, after>
  // if before is zero rect, we only need to rendered the after rect
  // if after is zero rect, we only need to rendered the before rect
  // if before != after, both of them need to be re-rendered
  change_log= requires_update (change_log);

  // Append the least upper bound to the change_log from the destroyed old boxes
  if (!is_zero (*changed_ptr)) {
    change_log << *changed_ptr;
  }
  // Reset the least upper bound for the old destroyed boxes
  (*changed_ptr)= rectangle (0, 0, 0, 0);

  rectangle r (0, 0, 0, 0);
  if (N (change_log) != 0) r= least_upper_bound (change_log);

  array<brush>     new_bgs;
  array<rectangle> rs;
  b->collect_page_colors (new_bgs, rs);
  for (int i= 0; i < min (N (old_bgs), N (new_bgs)); i++)
    if (new_bgs[i] != old_bgs[i]) r= least_upper_bound (r, rs[i]);
  old_bgs= new_bgs;

  x1b= r->x1;
  y1b= r->y1;
  x2b= r->x2;
  y2b= r->y2;
  return b;
}

/******************************************************************************
 * Event notification
 ******************************************************************************/

void
notify_assign (typesetter ttt, path p, tree u) {
  if (is_nil (p)) ttt->br= make_bridge (ttt, u, ttt->br->ip);
  else ttt->br->notify_assign (p, u);
}

void
notify_insert (typesetter ttt, path p, tree u) {
#ifdef LIII_DEBUG
  // cout << "Insert " << p << ", " << u << "\n";
  cout << "Insert ...\n";
#endif
  ttt->br->notify_insert (p, u);
}

void
notify_remove (typesetter ttt, path p, int nr) {
#ifdef LIII_DEBUG
  cout << "Remove " << p << ", " << nr << "\n";
#endif
  ttt->br->notify_remove (p, nr);
}

void
notify_split (typesetter ttt, path p) {
#ifdef LIII_DEBUG
  cout << "Split " << p << "\n";
#endif
  ttt->br->notify_split (p);
}

void
notify_join (typesetter ttt, path p) {
#ifdef LIII_DEBUG
  cout << "Join " << p << "\n";
#endif
  ttt->br->notify_join (p);
}

void
notify_assign_node (typesetter ttt, path p, tree_label op) {
#ifdef LIII_DEBUG
  cout << "Assign node " << p << ", " << as_string (op) << "\n";
#endif
  tree t= subtree (ttt->br->st, p);
  int  i, n= N (t);
  tree r (op, n);
  for (i= 0; i < n; i++)
    r[i]= t[i];
  if (is_nil (p)) ttt->br= make_bridge (ttt, r, ttt->br->ip);
  else ttt->br->notify_assign (p, r);
}

void
notify_insert_node (typesetter ttt, path p, tree t) {
#ifdef LIII_DEBUG
  cout << "Insert node " << p << ", " << t << "\n";
#endif
  int  i, pos= last_item (p), n= N (t);
  tree r (t, n + 1);
  for (i= 0; i < pos; i++)
    r[i]= t[i];
  r[pos]= subtree (ttt->br->st, path_up (p));
  for (i= pos; i < n; i++)
    r[i + 1]= t[i];
  if (is_nil (path_up (p))) ttt->br= make_bridge (ttt, r, ttt->br->ip);
  else ttt->br->notify_assign (path_up (p), r);
}

void
notify_remove_node (typesetter ttt, path p) {
#ifdef LIII_DEBUG
  cout << "Remove node " << p << "\n";
#endif
  tree t= subtree (ttt->br->st, p);
  if (is_nil (path_up (p))) ttt->br= make_bridge (ttt, t, ttt->br->ip);
  else ttt->br->notify_assign (path_up (p), t);
}

/******************************************************************************
 * Getting environment variables and typesetting interface
 ******************************************************************************/

void
exec_until (typesetter ttt, path p) {
  ttt->br->exec_until (p);
}

box
typeset (typesetter ttt) {
  return ttt->typeset ();
}

box
typeset (typesetter ttt, SI& x1, SI& y1, SI& x2, SI& y2) {
  return ttt->typeset (x1, y1, x2, y2);
}

void
typesetter_rep::progressive_advance (int additional_paragraphs) {
  progressive_cur_child= min (progressive_total_children,
                              progressive_cur_child + additional_paragraphs);
}

void
typesetter_rep::progressive_fast_forward_until (int target_child) {
  progressive_cur_child= min (progressive_total_children,
                              max (progressive_cur_child, target_child));
}

bool
typesetter_rep::progressive_should_pause (int current_child,
                                          int total_children) {
  (void) current_child;
  (void) total_children;
  return false;
}

void
typesetter_rep::progressive_reset () {
  progressive_cur_child    = 0;
  progressive_target_pages = 10;
  progressive_target_child = -1;
  progressive_pending      = false;
  accumulated_stream_height= 0;
}

bool
is_progressive_pending (typesetter ttt) {
  return ttt && ttt->is_progressive_pending ();
}

void
progressive_advance (typesetter ttt, int additional_paragraphs) {
  if (ttt) ttt->progressive_advance (additional_paragraphs);
}

void
progressive_fast_forward_until (typesetter ttt, int target_child) {
  if (ttt) ttt->progressive_fast_forward_until (target_child);
}

int
progressive_get_cur_child (typesetter ttt) {
  return ttt ? ttt->progressive_cur_child : 0;
}

SI
progressive_get_page_height (typesetter ttt) {
  return ttt ? ttt->page_text_height : 0;
}

void
progressive_reset (typesetter ttt) {
  if (ttt) ttt->progressive_reset ();
}

box
typeset_as_document (edit_env env, tree t, path ip) {
  env->style_init_env ();
  env->update ();
  typesetter ttt= new_typesetter (env, t, ip);
  box        b  = ttt->typeset ();
  delete_typesetter (ttt);
  return b;
}
