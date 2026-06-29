#include "tree_observer.hpp"
#include "analyze.hpp"
#include "basic.hpp"
#include "modification.hpp"
#include "observers.hpp"
#include "tree_cursor.hpp"
#include "tree_helper.hpp"

extern tree the_et;

/******************************************************************************
 * Debugging facilities
 ******************************************************************************/

static void
consistency_check (tree t, path ip) {
  if (obtain_ip (t) != ip)
    cout << "Wrong ip] " << t << " " << obtain_ip (t) << " instead of " << ip
         << "\n";
  if (is_compound (t)) {
    int i, n= N (t);
    for (i= 0; i < n; i++) {
      // if (!strong_equal (ip, obtain_ip (t[i])->next))
      if (obtain_ip (t) != obtain_ip (t[i])->next)
        cout << "Bad node] " << t << " " << obtain_ip (t) << " #" << i << "\n";
      consistency_check (t[i], path (i, ip));
    }
  }
}

void
consistency_check () {
  consistency_check (the_et, path ());
  cout << HRULE;
}

void
stretched_print (tree t, bool ips, int indent) {
  int i;
  for (i= 0; i < indent; i++)
    cout << "  ";
  if (is_atomic (t)) {
    cout << raw_quote (t->label);
    if (ips) cout << " -- " << obtain_ip (t);
    cout << "\n";
  }
  else {
    cout << as_string (L (t));
    if (ips) cout << " -- " << obtain_ip (t);
    cout << "\n";
    for (i= 0; i < N (t); i++)
      stretched_print (t[i], ips, indent + 1);
  }
}

/******************************************************************************
 * Wrappers which take into account mirroring
 ******************************************************************************/

bool                      busy_modifying = false;
bool                      busy_versioning= false;
static bool               is_busy        = false;
static list<path>         busy_paths;
static list<modification> upcoming;

bool
busy_path (path p) {
  for (list<path> l= busy_paths; !is_nil (l); l= l->next)
    if (l->item <= p) return true;
  return false;
}

bool
busy_tree (tree& ref) {
  path ip= obtain_ip (ref);
  if (ip_attached (ip)) return busy_path (reverse (ip));
  else return true;
}

/******************************************************************************
 * Wrappers for trees given by a path
 ******************************************************************************/

void
assign (path p, tree t) {
  assign (subtree (the_et, p), t);
}

void
insert (path p, tree ins) {
  insert (subtree (the_et, path_up (p)), last_item (p), ins);
}

void
remove (path p, int nr) {
  remove (subtree (the_et, path_up (p)), last_item (p), nr);
}

void
split (path p) {
  tree& st= subtree (the_et, path_up (path_up (p)));
  int   l1= last_item (path_up (p));
  int   l2= last_item (p);
  split (st, l1, l2);
}

void
join (path p) {
  join (subtree (the_et, path_up (p)), last_item (p));
}

void
assign_node (path p, tree_label op) {
  assign_node (subtree (the_et, p), op);
}

void
insert_node (path p, tree ins) {
  insert_node (subtree (the_et, path_up (p)), last_item (p), ins);
}

void
remove_node (path p) {
  remove_node (subtree (the_et, path_up (p)), last_item (p));
}

void
set_cursor (path p, tree data) {
  if (is_inside (the_et, p))
    set_cursor (subtree (the_et, path_up (p)), last_item (p), data);
  else cout << "TeXmacs] warning: invalid cursor position " << p << "\n";
}

void
touch (path p) {
  touch (subtree (the_et, p));
}

void
assign (tree& ref, tree t) {
  apply (ref, mod_assign (path (), t));
}

void
insert (tree& ref, int pos, tree t) {
  apply (ref, mod_insert (path (), pos, t));
}

void
remove (tree& ref, int pos, int nr) {
  apply (ref, mod_remove (path (), pos, nr));
}

void
split (tree& ref, int pos, int at) {
  apply (ref, mod_split (path (), pos, at));
}

void
join (tree& ref, int pos) {
  apply (ref, mod_join (path (), pos));
}

void
assign_node (tree& ref, tree_label op) {
  apply (ref, mod_assign_node (path (), op));
}

void
insert_node (tree& ref, int pos, tree t) {
  apply (ref, mod_insert_node (path (), pos, t));
}

void
remove_node (tree& ref, int pos) {
  apply (ref, mod_remove_node (path (), pos));
}

void
set_cursor (tree& ref, int pos, tree data) {
  apply (ref, mod_set_cursor (path (), pos, data));
}

/******************************************************************************
 * Routines for modifying trees
 *******************************************************************************
 * 1) The "inserting modifications" (insert, split and insert_node) invoke
 *    the observers call-back routines after the actual modification and
 *    "assigning and deleting modifications" (assign, remove, join,
 *    assign_node and remove_node) before the actual modification.
 *    set_cursor does not make any modifications in the tree
 * 2) The split and join modifications pass the joined tree
 *    at position pos as an additional argument to the call-back routines.
 * 3) They also admit variant call back routines for the split/join nodes.
 ******************************************************************************/

static void
simplify (observer& obs) {
  if (is_nil (obs)) return;
  observer& o1= obs->get_child (0);
  observer& o2= obs->get_child (1);
  if (!is_nil (o1) || !is_nil (o2)) {
    simplify (o1);
    simplify (o2);
    obs= list_observer (o1, o2);
  }
}

static void
detach (tree& ref, tree closest, bool right) {
  if (!is_nil (ref->data)) {
    ref->data->notify_detach (ref, closest, right);
    simplify (ref->data);
  }
  if (is_compound (ref)) {
    int i, n= N (ref);
    for (i= 0; i < n; i++)
      detach (ref[i], closest, right);
  }
}

void
raw_assign (tree& ref, tree t) {
  // cout << "Assign " << ref << " := " << t << "\n";
  modification mod= mod_assign (path (), t);
  if (!is_nil (ref->data)) {
    ref->data->announce (ref, mod);
    ref->data->notify_assign (ref, t);
    simplify (ref->data);
  }
  if (is_compound (ref)) {
    int i, n= N (ref), mid= (n + 1) >> 1;
    for (i= 0; i < n; i++)
      detach (ref[i], t, i >= mid);
  }
  ref= t;
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_insert (tree& ref, int pos, tree t) {
  // cout << "Insert " << ref << " += " << t << " at " << pos << "\n";
  modification mod= mod_insert (path (), pos, t);
  if (!is_nil (ref->data)) ref->data->announce (ref, mod);
  if (is_atomic (ref) && is_atomic (t))
    ref->label=
        ref->label (0, pos) * t->label * ref->label (pos, N (ref->label));
  else {
    int i, n= N (ref), nr= N (t);
    AR (ref)->resize (n + nr);
    for (i= n - 1; i >= pos; i--)
      ref[i + nr]= ref[i];
    for (i= 0; i < nr; i++)
      ref[pos + i]= t[i];
  }
  if (!is_nil (ref->data)) {
    ref->data->notify_insert (ref, pos, is_atomic (t) ? N (t->label) : N (t));
    simplify (ref->data);
  }
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_remove (tree& ref, int pos, int nr) {
  // cout << "Remove " << ref << " -= " << nr << " at " << pos << "\n";
  modification mod= mod_remove (path (), pos, nr);
  if (nr == 0) return;
  if (!is_nil (ref->data)) {
    ref->data->announce (ref, mod);
    ref->data->notify_remove (ref, pos, nr);
    simplify (ref->data);
  }
  if (is_compound (ref)) {
    int i, n= N (ref), end= pos + nr, mid= (pos + end + 1) >> 1;
    for (i= pos; i < mid; i++)
      if (pos == 0) detach (ref[i], ref, false);
      else detach (ref[i], ref[pos - 1], true);
    for (; i < end; i++)
      if (end == n) detach (ref[i], ref, true);
      else detach (ref[i], ref[pos + nr], false);
  }

  if (is_atomic (ref))
    ref->label= ref->label (0, pos) * ref->label (pos + nr, N (ref->label));
  else {
    int i, n= N (ref) - nr;
    for (i= pos; i < n; i++)
      ref[i]= ref[i + nr];
    AR (ref)->resize (n);
  }
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_split (tree& ref, int pos, int at) {
  // cout << "Split " << ref << " at " << pos << ", " << at << "\n";
  modification mod= mod_split (path (), pos, at);
  if (!is_nil (ref->data)) ref->data->announce (ref, mod);
  tree t= ref[pos], t1, t2;
  if (is_atomic (ref[pos])) {
    t1= ref[pos]->label (0, at);
    t2= ref[pos]->label (at, N (ref[pos]->label));
  }
  else {
    t1= ref[pos](0, at);
    t2= ref[pos](at, N (ref[pos]));
  }
  int i, n= N (ref);
  AR (ref)->resize (n + 1);
  for (i= n; i > (pos + 1); i--)
    ref[i]= ref[i - 1];
  ref[pos]    = t1;
  ref[pos + 1]= t2;

  if (!is_nil (ref->data)) {
    ref->data->notify_split (ref, pos, t);
    simplify (ref->data);
  }
  if (!is_nil (t->data)) {
    t->data->notify_var_split (t, t1, t2);
    simplify (t->data);
  }
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_join (tree& ref, int pos) {
  // cout << "Join " << ref << " at " << pos << "\n";
  // the following code is added for security
  if (is_atomic (ref[pos]) && (!is_atomic (ref[pos + 1])))
    insert_node (ref[pos], 0, tree (L (ref[pos + 1])));
  if (is_atomic (ref[pos + 1]) && (!is_atomic (ref[pos])))
    insert_node (ref[pos + 1], 0, tree (L (ref[pos])));
  // end security code

  modification mod= mod_join (path (), pos);
  if (!is_nil (ref->data)) ref->data->announce (ref, mod);
  tree t1= ref[pos], t2= ref[pos + 1], t;
  int  offset= is_atomic (ref) ? N (t1->label) : N (t1);
  if (is_atomic (t1) && is_atomic (t2)) t= t1->label * t2->label;
  else t= t1 * t2;
  if (!is_nil (ref->data)) ref->data->notify_join (ref, pos, t);
  if (!is_nil (t1->data)) {
    t1->data->notify_var_join (t1, t, 0);
    simplify (t1->data);
  }
  if (!is_nil (t2->data)) {
    t2->data->notify_var_join (t2, t, offset);
    simplify (t2->data);
  }
  ref[pos]= t;

  int i, n= N (ref) - 1;
  for (i= pos + 1; i < n; i++)
    ref[i]= ref[i + 1];
  AR (ref)->resize (n);
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_assign_node (tree& ref, tree_label op) {
  // cout << "Assign node " << ref << " : " << tree (op) << "\n";
  modification mod= mod_assign_node (path (), op);
  if (!is_nil (ref->data)) {
    ref->data->announce (ref, mod);
    ref->data->notify_assign_node (ref, op);
    simplify (ref->data);
  }
  LR (ref)= op;
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_insert_node (tree& ref, int pos, tree t) {
  // cout << "Insert node " << ref << " : " << t << " at " << pos << "\n";
  modification mod= mod_insert_node (path (), pos, t);
  if (!is_nil (ref->data)) ref->data->announce (ref, mod);
  int  i, n= N (t);
  tree r (t, n + 1);
  for (i= 0; i < pos; i++)
    r[i]= t[i];
  r[pos]= ref;
  for (i= pos; i < n; i++)
    r[i + 1]= t[i];
  ref= r;
  if (!is_nil (ref[pos]->data)) {
    ref[pos]->data->notify_insert_node (ref, pos);
    simplify (ref[pos]->data);
  }
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_remove_node (tree& ref, int pos) {
  // cout << "Remove node " << ref << " : " << pos << "\n";
  modification mod= mod_remove_node (path (), pos);
  if (!is_nil (ref->data)) {
    ref->data->announce (ref, mod);
    ref->data->notify_remove_node (ref, pos);
    simplify (ref->data);
  }
  for (int i= 0; i < N (ref); i++)
    if (i < pos) detach (ref[i], ref[pos], false);
    else if (i > pos) detach (ref[i], ref[pos], true);
  ref= ref[pos];
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_set_cursor (tree& ref, int pos, tree data) {
  // cout << "Set cursor " << ref << " : " << pos << ", " << data << "\n";
  modification mod= mod_set_cursor (path (), pos, data);
  if (!is_nil (ref->data)) {
    ref->data->announce (ref, mod);
    ref->data->notify_set_cursor (ref, pos, data);
    simplify (ref->data);
  }
  if (!is_nil (ref->data)) ref->data->done (ref, mod);
  // stretched_print (ref, true, 1);
  // consistency_check ();
}

void
raw_apply (tree& t, modification mod) {
  ASSERT (is_applicable (t, mod), "invalid modification");
  switch (mod->k) {
  case MOD_ASSIGN:
    raw_assign (subtree (t, root (mod)), mod->t);
    break;
  case MOD_INSERT:
    raw_insert (subtree (t, root (mod)), index (mod), mod->t);
    break;
  case MOD_REMOVE:
    raw_remove (subtree (t, root (mod)), index (mod), argument (mod));
    break;
  case MOD_SPLIT:
    raw_split (subtree (t, root (mod)), index (mod), argument (mod));
    break;
  case MOD_JOIN:
    raw_join (subtree (t, root (mod)), index (mod));
    break;
  case MOD_ASSIGN_NODE:
    raw_assign_node (subtree (t, root (mod)), L (mod));
    break;
  case MOD_INSERT_NODE:
    raw_insert_node (subtree (t, root (mod)), argument (mod), mod->t);
    break;
  case MOD_REMOVE_NODE:
    raw_remove_node (subtree (t, root (mod)), index (mod));
    break;
  case MOD_SET_CURSOR:
    raw_set_cursor (subtree (t, root (mod)), index (mod), mod->t);
    break;
  }
}

void
apply (tree& ref, modification mod) {
  if (!is_applicable (ref, mod)) {
    cerr << "mod= " << mod << "\n";
    cerr << "ref= " << ref << "\n";
    TM_FAILED ("invalid modification");
  }
  path ip= obtain_ip (ref);
  path rp= reverse (ip);
  path p = rp * root (mod);
  if (busy_modifying) raw_apply (ref, mod);
  else if (is_busy) {
    if (ip_attached (ip) && !busy_path (p)) {
      // cout << "Postpone " << (reverse (ip) * mod) << "\n";
      busy_paths= busy_paths * p;
      upcoming  = upcoming * (reverse (ip) * mod);
    }
  }
  else {
    if (!ip_attached (ip)) raw_apply (ref, mod);
    else {
      is_busy   = true;
      busy_paths= list<path> (p);
      upcoming  = list<modification> (reverse (ip) * mod);
      while (!is_nil (upcoming)) {
        // cout << "Handle " << upcoming->item << "\n";
        raw_apply (the_et, upcoming->item);
        // cout << "Done " << upcoming->item << "\n";
        upcoming= upcoming->next;
      }
      busy_paths= list<path> ();
      is_busy   = false;
      if (has_subtree (the_et, rp)) ref= subtree (the_et, rp);
    }
  }
}