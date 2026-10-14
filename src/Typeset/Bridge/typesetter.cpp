
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
#include "tm_debug.hpp"

using namespace moebius;

/******************************************************************************
 * Constructor and destructor
 ******************************************************************************/

typesetter_rep::typesetter_rep (edit_env& env2, tree et, path ip)
    : env (env2), old_patch (UNINIT), progressive_enabled (false),
      progressive_cur_child (0), progressive_total_children (0),
      progressive_pending (false), progressive_bench_active (false),
      frozen_pages_positioned (0), doc_name (""), chunk_start (0),
      chunk_end (0), doc_l (array<page_item> ()), doc_sb (stack_border ()),
      frozen_page_boxes (array<box> ()), frozen_border_boxes (array<box> ()),
      frozen_line_offset (0), dynamic_buffer_pages (3), page_text_height (0) {
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

// 结束全生命周期基准（若已启动）；开始于 typeset() 的 progressive 初始化处
static void
progressive_bench_end (typesetter_rep* ttt) {
  if (ttt->progressive_bench_active) {
    bench_end ("progressive_typeset: " * ttt->doc_name, 0);
    ttt->progressive_bench_active= false;
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
      // cur_child 归零的两条路径（构造、progressive_reset）已清空累计状态
      chunk_start= 0;
      chunk_end  = min (n, 50);
      if (doc_name != "") {
        bench_start ("progressive_typeset: " * doc_name);
        progressive_bench_active= true;
      }
    }
  }
  else {
    progressive_enabled    = false;
    progressive_pending    = false;
    progressive_cur_child  = n;
    chunk_start            = 0;
    chunk_end              = n;
    frozen_page_boxes      = array<box> ();
    frozen_border_boxes    = array<box> ();
    frozen_line_offset     = 0;
    frozen_pages_positioned= 0;
    // 文档中途退出 progressive 模式时闭环基准，避免计时泄漏
    progressive_bench_end (this);
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
  if (progressive_enabled && chunk_end < n) {
    progressive_pending= true;
    env->complete      = false;
  }
  else {
    progressive_pending= false;
  }
  bench_start ("typeset: br->typeset");
  br->typeset (PROCESSED + WANTED_PARAGRAPH);
  bench_end ("typeset: br->typeset", 5);

  if (progressive_enabled) {
    if (chunk_start == 0) {
      doc_l                  = copy (l);
      doc_sb                 = sb;
      frozen_page_boxes      = array<box> ();
      frozen_border_boxes    = array<box> ();
      frozen_line_offset     = 0;
      frozen_pages_positioned= 0;
    }
    else {
      merge_stack (doc_l, doc_sb, l, sb);
    }
    progressive_cur_child= chunk_end;
  }

  array<page_item> unfrozen_l;
  if (progressive_enabled) {
    unfrozen_l= copy (doc_l);
  }
  else {
    unfrozen_l= copy (l);
  }

  bench_start ("typeset: make_pages");
  pager ppp= tm_new<pager_rep> (
      br->ip, env, unfrozen_l,
      progressive_enabled ? frozen_page_boxes : array<box> (),
      progressive_enabled ? frozen_border_boxes : array<box> ());
  box rb= ppp->make_pages ();
  bench_end ("typeset: make_pages", 5);

  if (progressive_enabled) {
    int  num_new = N (ppp->new_pages);
    bool is_final= (chunk_end >= n);
    if (!is_final && num_new > dynamic_buffer_pages) {
      int to_freeze= num_new - dynamic_buffer_pages;
      for (int fi= 0; fi < to_freeze; fi++) {
        frozen_page_boxes << ppp->new_pages[fi];
        int p_idx= N (frozen_border_boxes);
        int nx   = max (1, min (env->page_packet, N (frozen_page_boxes)));
        if (env->page_packet == 2) nx= max (1, env->page_packet);
        int d    = env->page_offset % nx;
        int i    = (p_idx + d) % nx;
        int j    = (p_idx + d) / nx;
        SI  pixel= env->pixel;
        SI  l= 0, r= 0, b= 0, t= 0;
        if (env->get_string (PAGE_BORDER) != "none") {
          l= 10 * pixel, r= 10 * pixel;
          b= 10 * pixel, t= 10 * pixel;
          if (j == 0) t= 0;
          if (env->get_string (PAGE_BORDER) == "attached") {
            if (i > 0) l= pixel / 2;
            if (i < nx - 1) r= 0;
          }
        }
        else {
          if (i > 0) l= pixel;
        }
        color bg= tm_background;
        if (env->get_string ("full-screen-mode") == "true") bg= black;
        box bb= page_border_box (ppp->new_pages[fi]->ip, ppp->new_pages[fi], bg,
                                 l, r, b, t, pixel);
        frozen_border_boxes << bb;
      }
      pagelet last_frozen_pg = ppp->last_sk[to_freeze - 1];
      int     end_in_unfrozen= 0;
      if (N (last_frozen_pg->ins) > 0) {
        end_in_unfrozen=
            last_frozen_pg->ins[N (last_frozen_pg->ins) - 1]->end->item;
      }
      if (end_in_unfrozen > 0 && end_in_unfrozen < N (doc_l)) {
        doc_l= range (doc_l, end_in_unfrozen, N (doc_l));
      }
      else if (end_in_unfrozen >= N (doc_l)) {
        doc_l= array<page_item> ();
      }
    }
    else if (is_final) {
      for (int fi= 0; fi < num_new; fi++) {
        frozen_page_boxes << ppp->new_pages[fi];
      }
      doc_l             = array<page_item> ();
      frozen_line_offset= 0;
      progressive_bench_end (this);
    }
  }
  if (env->complete && paper) determine_page_references (rb);
  tm_delete (ppp);
  // env->complete= false;  // moved to edit_typeset_rep::typeset
  return rb;
}

box
typesetter_rep::typeset (DI& x1b, DI& y1b, DI& x2b, DI& y2b) {
  x1= x1b;
  y1= y1b;
  x2= x2b;
  y2= y2b;

  // For old destroyed boxes, the changed_least_upper_bound will be updated
  box b= typeset ();

  // Append pairs of rectangles to the change_log from the new typesetting box
  array<rectangle> change_log;
  bench_start ("typeset: position_at");
  if (progressive_enabled && b->subnr () == 1) {
    box sb       = b->subbox (0);
    int n_pages  = sb->subnr ();
    int frozen_n = N (frozen_page_boxes);
    int safe_skip= max (0, min (frozen_pages_positioned, frozen_n) - 2);
    for (int pi= safe_skip; pi < n_pages; pi++) {
      sb->subbox (pi)->position_at (0, 0, change_log, changed_ptr);
    }
    frozen_pages_positioned= frozen_n;
  }
  else {
    b->position_at (0, 0, change_log, changed_ptr);
  }
  bench_end ("typeset: position_at", 5);

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
typeset (typesetter ttt, DI& x1, DI& y1, DI& x2, DI& y2) {
  return ttt->typeset (x1, y1, x2, y2);
}

void
typesetter_rep::progressive_advance (int additional_paragraphs) {
  chunk_start= progressive_cur_child;
  chunk_end=
      min (progressive_total_children, chunk_start + additional_paragraphs);
}

void
typesetter_rep::progressive_fast_forward_until (int target_child) {
  chunk_start= progressive_cur_child;
  chunk_end  = min (progressive_total_children,
                    max (progressive_cur_child + 10, target_child + 50));
}

void
typesetter_rep::progressive_reset () {
  progressive_cur_child   = 0;
  chunk_start             = 0;
  chunk_end               = 0;
  doc_l                   = array<page_item> ();
  doc_sb                  = stack_border ();
  frozen_page_boxes       = array<box> ();
  frozen_border_boxes     = array<box> ();
  frozen_line_offset      = 0;
  progressive_pending     = false;
  progressive_bench_active= false;
  frozen_pages_positioned = 0;
}

bool
typeset_in_progress (typesetter ttt) {
  return ttt && ttt->typeset_in_progress ();
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
typeset_frontier (typesetter ttt) {
  return ttt ? ttt->progressive_cur_child : 0;
}

DI
progressive_get_page_height (typesetter ttt) {
  return ttt ? ttt->page_text_height : 0;
}

void
progressive_set_doc_name (typesetter ttt, string name) {
  if (ttt) ttt->doc_name= name;
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
