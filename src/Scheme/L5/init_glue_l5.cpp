
/******************************************************************************
 * MODULE     : init_glue_l5.cpp
 * DESCRIPTION: L5 Glue for linking TeXmacs commands to scheme
 * COPYRIGHT  : (C) 1999-2011  Joris van der Hoeven and Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "init_glue_l5.hpp"

#include <lolly/system/timer.hpp>

#include "object.hpp"
#include "object_l1.hpp"
#include "object_l2.hpp"
#include "object_l3.hpp"
#include "object_l5.hpp"

#include "Freetype/tt_tools.hpp"
#include "Qt/qt_tm_widget.hpp"
#include "boxes.hpp"
#include "editor.hpp"
#include "iterator.hpp"
#include "locale.hpp"
#include "observers.hpp"
#include "preferences.hpp"
#include "promise.hpp"
#include "qt_chat_controller.hpp"
#include "qt_floating_search_bar.hpp"
#include "tm_debug.hpp"
#include "tm_locale.hpp"
#include "tree_observer.hpp"
#include "universal.hpp"
#include "widget.hpp"

#if 0
template<class T> tmscm box_to_tmscm (T o) {
  return blackbox_to_tmscm (close_box<T> (o)); }
template<class T> T tmscm_to_box (tmscm obj) { 
  return open_box<T>(tmscm_to_blackbox (obj));  }
template<class T> tmscm cmp_box (tmscm o1, tmscm o2) { 
  return bool_to_tmscm (tmscm_to_box<T> (o1) == tmscm_to_box<T> (o2)); }
template<class T> tmscm boxP (tmscm t) {
  bool b= tmscm_is_blackbox (t) && 
          (type_box (blackboxvalue(t)) == type_helper<T>::id);
  return bool_to_tmscm (b);
}
#endif

/******************************************************************************
 * Miscellaneous routines for use by glue only
 ******************************************************************************/

string original_path;

string
get_original_path () {
  return original_path;
}

string
texmacs_version (string which) {
  if (which == "tgz") return TM_DEVEL;
  if (which == "rpm") return TM_DEVEL_RELEASE;
  if (which == "stgz") return TM_STABLE;
  if (which == "srpm") return TM_STABLE_RELEASE;
  if (which == "devel") return TM_DEVEL;
  if (which == "stable") return TM_STABLE;
  if (which == "devel-release") return TM_DEVEL_RELEASE;
  if (which == "stable-release") return TM_STABLE_RELEASE;
  if (which == "revision") return TEXMACS_REVISION;
  return TEXMACS_VERSION;
}

void
set_fast_environments (bool b) {
  enable_fastenv= b;
}

void
win32_display (string s) {
  cout << s;
  cout.flush ();
}

void
tm_output (string s) {
  cout << s;
  cout.flush ();
}

void
tm_errput (string s) {
  cerr << s;
  cerr.flush ();
}

void
cpp_error () {
  // char *np= 0; *np= 1;
  TM_FAILED ("an error occurred");
}

array<int>
get_bounding_rectangle (tree t) {
  editor     ed  = get_current_editor ();
  rectangle  wr  = ed->get_window_extents ();
  path       p   = reverse (obtain_ip (t));
  selection  sel = ed->search_selection (p * start (t), p * end (t));
  SI         sz  = ed->get_pixel_size ();
  double     sf  = ((double) sz) / 256.0;
  rectangle  selr= least_upper_bound (sel->rs) / sf;
  rectangle  r   = translate (selr, wr->x1, wr->y2);
  array<int> ret;
  ret << (r->x1) << (r->y1) << (r->x2) << (r->y2);
  // ret << (r->x1/PIXEL) << (r->y1/PIXEL) << (r->x2/PIXEL) << (r->y2/PIXEL);
  return ret;
}

bool
is_busy_versioning () {
  return busy_versioning;
}

array<SI>
get_screen_size () {
  array<SI> r;
  SI        w, h;
  gui_root_extents (w, h);
  r << w << h;
  return r;
}

/******************************************************************************
 * Redirections
 ******************************************************************************/

void
cout_buffer () {
  cout.buffer ();
}

string
cout_unbuffer () {
  return cout.unbuffer ();
}

bool
tree_active (tree t) {
  path ip= obtain_ip (t);
  return is_nil (ip) || last_item (ip) != DETACHED;
}

/******************************************************************************
 * Table types
 ******************************************************************************/

typedef hashmap<string, string> table_string_string;

bool
tmscm_is_table_string_string (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else if (!tmscm_is_pair (p)) return false;
  else {
    tmscm f= tmscm_car (p);
    return tmscm_is_pair (f) && tmscm_is_string (tmscm_car (f)) &&
           tmscm_is_string (tmscm_cdr (f)) &&
           tmscm_is_table_string_string (tmscm_cdr (p));
  }
}

#define TMSCM_ASSERT_TABLE_STRING_STRING(p, arg, rout)                         \
  TMSCM_ASSERT (tmscm_is_table_string_string (p), p, arg, rout)

tmscm
table_string_string_to_tmscm (hashmap<string, string> t) {
  tmscm            p = tmscm_null ();
  iterator<string> it= iterate (t);
  while (it->busy ()) {
    string s= it->next ();
    tmscm  n= tmscm_cons (string_to_tmscm (s), string_to_tmscm (t[s]));
    p       = tmscm_cons (n, p);
  }
  return p;
}

hashmap<string, string>
tmscm_to_table_string_string (tmscm p) {
  hashmap<string, string> t;
  while (!tmscm_is_null (p)) {
    tmscm n                            = tmscm_car (p);
    t (tmscm_to_string (tmscm_car (n)))= tmscm_to_string (tmscm_cdr (n));
    p                                  = tmscm_cdr (p);
  }
  return t;
}

#define tmscm_is_solution tmscm_is_table_string_string
#define TMSCM_ASSERT_SOLUTION(p, arg, rout)                                    \
  TMSCM_ASSERT (tmscm_is_solution (p), p, arg, rout)
#define solution_to_tmscm table_string_string_to_tmscm
#define tmscm_to_solution tmscm_to_table_string_string

typedef array<widget> array_widget;

static bool
tmscm_is_array_widget (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_widget (tmscm_car (p)) &&
           tmscm_is_array_widget (tmscm_cdr (p));
}

#define TMSCM_ASSERT_ARRAY_WIDGET(p, arg, rout)                                \
  TMSCM_ASSERT (tmscm_is_array_widget (p), p, arg, rout)

/* static */ tmscm
array_widget_to_tmscm (array<widget> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (widget_to_tmscm (a[i]), p);
  return p;
}

/* static */ array<widget>
tmscm_to_array_widget (tmscm p) {
  array<widget> a;
  while (!tmscm_is_null (p)) {
    a << tmscm_to_widget (tmscm_car (p));
    p= tmscm_cdr (p);
  }
  return a;
}

void   register_glyph (string s, array_array_array_double gl);
string recognize_glyph (array_array_array_double gl);

/******************************************************************************
 * Protected evaluation
 ******************************************************************************/

void
protected_call (object cmd) {
  try {
    get_current_editor ()->before_menu_action ();
    call (cmd);
    get_current_editor ()->after_menu_action ();
  } catch (string s) {
    get_current_editor ()->cancel_menu_action ();
  }
  handle_exceptions ();
}

void
bench_print_all () {
  lolly::system::bench_print (std_bench);
}

/******************************************************************************
 * Gluing
 ******************************************************************************/

#include "Concat/concater.hpp"
#include "boot.hpp"
#include "client_server.hpp"
#include "connect.hpp"
#include "convert.hpp"
#include "converter.hpp"
#include "dictionary.hpp"
#include "image_files.hpp"
#include "link.hpp"
#include "new_style.hpp"
#include "packrat.hpp"
#include "server.hpp"
#include "tm_frame.hpp"
#include "tm_timer.hpp"
#include "tm_window.hpp"
#include "web_files.hpp"
#include "wencoding.hpp"

#include "Freetype/tt_file.hpp"
#include "Metafont/tex_files.hpp"
#include "font.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H

string
freetype_version () {
  return as_string (FREETYPE_MAJOR) * "." * as_string (FREETYPE_MINOR) * "." *
         as_string (FREETYPE_PATCH);
}

#include "cork.hpp"
#include "glue_basic.cpp"
#include "glue_editor.cpp"
#include "glue_font.cpp"
#include "glue_server.cpp"
#include "glue_widget.cpp"

void
open_pricing_url () {
  // 获取当前窗口
  if (!has_current_window ()) return;
  tm_window win= concrete_window ();
  if (win == NULL) return;

  // 尝试将widget转换为qt_tm_widget_rep
  widget            w        = win->win;
  qt_tm_widget_rep* tm_widget= dynamic_cast<qt_tm_widget_rep*> (w.rep);
  if (tm_widget != NULL) {
    tm_widget->openRenewalPage ();
  }
}

void
initialize_glue_l5 () {
  initialize_glue_font ();
  initialize_glue_widget ();
  initialize_glue_basic ();
  initialize_glue_editor ();
  initialize_glue_server ();
}
