/******************************************************************************
 * MODULE     : glue_l5_extra.hpp
 * DESCRIPTION: helper functions, type aliases, and macros shared by the
 *              L5 standalone glues (glue_basic/editor/font/server/widget,
 *              generated) and by init_glue_l5.cpp's own registrations.
 *              Extracted so each generated glue_*.cpp can be compiled as
 *              an independent translation unit.
 ******************************************************************************/

#ifndef GLUE_L5_EXTRA_HPP
#define GLUE_L5_EXTRA_HPP

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
#include "tree_observer.hpp"
#include "universal.hpp"
#include "widget.hpp"

#include "Concat/concater.hpp"
#include "boot.hpp"
#include "client_server.hpp"
#include "connect.hpp"
#include "convert.hpp"
#include "converter.hpp"
#include "cork.hpp"
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

extern string original_path;

inline string
get_original_path () {
  return original_path;
}

inline string
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

inline void
set_fast_environments (bool b) {
  enable_fastenv= b;
}

inline void
win32_display (string s) {
  cout << s;
  cout.flush ();
}

inline void
tm_output (string s) {
  cout << s;
  cout.flush ();
}

inline void
tm_errput (string s) {
  cerr << s;
  cerr.flush ();
}

inline void
cpp_error () {
  TM_FAILED ("an error occurred");
}

inline array<int>
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
  return ret;
}

inline bool
is_busy_versioning () {
  return busy_versioning;
}

inline array<SI>
get_screen_size () {
  array<SI> r;
  SI        w, h;
  gui_root_extents (w, h);
  r << w << h;
  return r;
}

inline void
cout_buffer () {
  cout.buffer ();
}

inline string
cout_unbuffer () {
  return cout.unbuffer ();
}

inline bool
tree_active (tree t) {
  path ip= obtain_ip (t);
  return is_nil (ip) || last_item (ip) != DETACHED;
}

typedef hashmap<string, string> table_string_string;

inline bool
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

inline tmscm
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

inline hashmap<string, string>
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

inline bool
tmscm_is_array_widget (tmscm p) {
  if (tmscm_is_null (p)) return true;
  else
    return tmscm_is_pair (p) && tmscm_is_widget (tmscm_car (p)) &&
           tmscm_is_array_widget (tmscm_cdr (p));
}

#define TMSCM_ASSERT_ARRAY_WIDGET(p, arg, rout)                                \
  TMSCM_ASSERT (tmscm_is_array_widget (p), p, arg, rout)

inline tmscm
array_widget_to_tmscm (array<widget> a) {
  int   i, n= N (a);
  tmscm p= tmscm_null ();
  for (i= n - 1; i >= 0; i--)
    p= tmscm_cons (widget_to_tmscm (a[i]), p);
  return p;
}

inline array<widget>
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

inline void
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

inline void
bench_print_all () {
  lolly::system::bench_print (std_bench);
}

inline string
freetype_version () {
  return as_string (FREETYPE_MAJOR) * "." * as_string (FREETYPE_MINOR) * "." *
         as_string (FREETYPE_PATCH);
}

inline void
open_pricing_url () {
  if (!has_current_window ()) return;
  tm_window win= concrete_window ();
  if (win == NULL) return;
  widget            w        = win->win;
  qt_tm_widget_rep* tm_widget= dynamic_cast<qt_tm_widget_rep*> (w.rep);
  if (tm_widget != NULL) {
    tm_widget->openRenewalPage ();
  }
}

#endif
