
/******************************************************************************
 * MODULE     : edit_mouse.cpp
 * DESCRIPTION: Mouse handling
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Modify/edit_table.hpp"
#include "analyze.hpp"
#include "edit_interface.hpp"
#include "link.hpp"
#include "message.hpp"
#include "moebius/tree_label.hpp"
#include "moebius/vars.hpp"
#include "observer.hpp"
#include "observers.hpp"
#include "path.hpp"
#include "qapplication.h"
#include "qnamespace.h"
#include "qt_simple_widget.hpp"
#include "scheme.hpp"
#include "sys_utils.hpp"
#include "tm_buffer.hpp"
#include "tm_timer.hpp"

#include <moebius/data/scheme.hpp>
#include <moebius/drd/drd_mode.hpp>

using namespace moebius;
using moebius::data::scm_quote;
using moebius::drd::set_access_mode;

void disable_double_clicks ();

/******************************************************************************
 * Status of graphics mode
 ******************************************************************************/

bool is_in_graphics_mode= false;

bool
edit_interface_rep::should_show_image_popup (tree t) {
  if (is_nil (t)) return false;

  int t_N= N (t);
  if (t_N <= 2) return false;

  if (is_func (t, WITH)) {
    for (int i= 0; i < t_N; ++i) {
      if (t[i] == PAR_MODE) {
        return true;
      }
    }
  }

  path ip= obtain_ip (t);
  if (is_nil (ip) || ip->item == DETACHED) return false;

  path p= path_up (reverse (ip));
  tree sub;
  // 持续向上遍历至最顶层的编辑树，若过程中出现了非 document
  // 节点，说明图片被其他节点包裹，返回 false
  while (rp <= p) {
    sub= subtree (et, p);
    if (!is_func (sub, DOCUMENT)) {
      return false;
    }
    p= path_up (p);
  }
  return true;
}

/******************************************************************************
 * Routines for the mouse
 ******************************************************************************/

bool
edit_interface_rep::mouse_message (string message, SI x, SI y) {
  rectangles rs;
  tree       r= eb->message (message, x, y, rs);
  if (N (rs) != 0) invalidate (rs);
  return r != "";
}

color
edit_interface_rep::mouse_clickable_color () {
  path sp= find_innermost_scroll (eb, tp);
  path p = tree_path (sp, last_x, last_y, 0);
  tree t = "#20A060";
  if (rp <= p) t= get_env_value (CLICKABLE_COLOR, p);
  if (!is_atomic (t)) t= "#20A060";
  return named_color (t->label);
}

void
edit_interface_rep::mouse_click (SI x, SI y) {
  if (mouse_message ("click", x, y)) return;
  start_x= x;
  start_y= y;
  send_mouse_grab (this, true);
}

bool
edit_interface_rep::mouse_extra_click (SI x, SI y) {
  go_to (x, y);
  if (mouse_message ("double-click", x, y)) return true;
  go_to (x, y);
  path p1, p2;
  get_selection (p1, p2);
  if ((p1 == p2) || path_less (tp, p1) || path_less (p2, tp)) select (tp, tp);
  select_enlarge ();
  if (selection_active_any ()) selection_set ("mouse", selection_get (), true);
  return false;
}

void
edit_interface_rep::mouse_adjust_selection (SI x, SI y, int mods) {
  if (inside_graphics () || mods <= 1) return;
  if (mouse_message ("drag", x, y)) return;
  go_to (x, y);
  end_x  = x;
  end_y  = y;
  path sp= find_innermost_scroll (eb, tp);
  path p1= tree_path (sp, start_x, start_y, 0);
  path p2= tree_path (sp, end_x, end_y, 0);
  path p3= tree_path (sp, x, y, 0);

  bool p1_p2= path_inf (p1, p2);
  bool p1_p3= path_inf (p1, p3);
  bool p2_p3= path_inf (p2, p3);

  if (mods & ShiftMask) {  // Holding shift: enlarge in direction start_ -> end_
    if (!p1_p2 && p1_p3) { // p2<p1<p3
      start_x= end_x;
      start_y= end_y;
      end_x  = x;
      end_y  = y;
      p1     = p2;
      p2     = p3;
    }
    else if (!p1_p3 && p1_p2) { // p3<p1<p2
      start_x= end_x;
      start_y= end_y;
      end_x  = x;
      end_y  = y;
      p1     = p3;
    }
    else if ((p2_p3 && !p1_p3) || (!p1_p2 && !p2_p3)) { // p2<p3<p1, p3<p2<p1
      end_x= x;
      end_y= y;
      p2   = p1;
      p1   = p3;
    }
    else if ((p1_p2 && p2_p3) || (p1_p3 && !p2_p3)) { // p1<p2<p3, p1<p3<p2
      end_x= x;
      end_y= y;
      p2   = p3;
    }
    selection_visible ();
    set_selection (p1, p2);
    notify_change (THE_SELECTION);
    selection_set ("mouse", selection_get (), true);
  }
}

void
edit_interface_rep::mouse_drag (SI x, SI y) {
  if (inside_graphics ()) return;
  if (mouse_message ("drag", x, y)) return;
  go_to (x, y);
  end_x= x;
  end_y= y;
  selection_visible ();
  path sp= find_innermost_scroll (eb, tp);
  path p1= tree_path (sp, start_x, start_y, 0);
  path p2= tree_path (sp, end_x, end_y, 0);
  if (path_inf (p2, p1)) {
    path temp= p1;
    p1       = p2;
    p2       = temp;
  }
  set_selection (p1, p2);
  notify_change (THE_SELECTION);
}

void
edit_interface_rep::mouse_select (SI x, SI y, int mods, bool drag) {
  if (mouse_message ("select", x, y)) return;
  if (!is_nil (mouse_ids) && !drag) {
    bool ctrl_pressed   = ((mods & ControlMask) != 0);
    bool command_pressed= ((mods & Mod2Mask) != 0);
    bool is_inner_link=
        as_bool (call ("link-contains-inner-link?", object (mouse_ids)));
    if (is_inner_link ||
        (!is_inner_link && ((!(os_macos ()) && ctrl_pressed) ||
                            (os_macos () && command_pressed)))) {
      call ("link-follow-ids", object (mouse_ids), object ("click"));
      disable_double_clicks ();
      return;
    }
  }
  tree g;
  bool b0= inside_graphics (false);
  bool b = inside_graphics ();
  if (b) g= get_graphics ();
  go_to (x, y);
  if ((!b0 && inside_graphics (false)) || (b0 && !inside_graphics (false)))
    drag= false;
  if (!b && inside_graphics ()) eval ("(graphics-reset-context 'begin)");
  tree g2= get_graphics ();
  if (b && (!inside_graphics () || obtain_ip (g) != obtain_ip (g2))) {
    invalidate_graphical_object ();
    eval ("(graphics-reset-context 'exit)");
  }
  if (!drag) {
    path sp= find_innermost_scroll (eb, tp);
    path p0= tree_path (sp, x, y, 0);
    set_selection (p0, p0);
    notify_change (THE_SELECTION);
  }
  if (selection_active_any ()) selection_set ("mouse", selection_get (), true);
}

void
edit_interface_rep::mouse_paste (SI x, SI y) {
  (void) x;
  (void) y;
  if (mouse_message ("paste", x, y)) return;
  go_to (x, y);
  selection_paste ("mouse");
}

void
edit_interface_rep::mouse_adjust (SI x, SI y, int mods) {
  if (mouse_message ("adjust", x, y)) return;
  x= (SI) (x * magf);
  y= (SI) (y * magf);
  abs_round (x, y);
  if (is_nil (popup_win)) {
    SI wx, wy;
    ::get_position (get_window (this), wx, wy);
    widget wid;
    string menu= "texmacs-popup-menu";
    if ((mods & (ShiftMask + ControlMask)) != 0)
      menu= "texmacs-alternative-popup-menu";
    SERVER (menu_widget ("(vertical (link " * menu * "))", wid));
    widget popup_wid= ::popup_widget (wid);
    popup_win       = ::popup_window_widget (popup_wid, "Popup menu");
#if defined(QTTEXMACS) || defined(AQUATEXMACS)
    SI ox, oy, sx, sy;
    get_position (this, ox, oy);
    get_scroll_position (this, sx, sy);
    ox-= sx;
    oy-= sy;
#endif
    set_position (popup_win, wx + ox + x, wy + oy + y);
    set_visibility (popup_win, true);
    send_keyboard_focus (this);
    send_mouse_grab (popup_wid, true);
  }
}

void
edit_interface_rep::mouse_scroll (SI x, SI y, bool up) {
  string message= up ? string ("scroll up") : string ("scroll down");
  if (mouse_message (message, x, y)) return;
  SI dy= 100 * PIXEL;
  if (!up) dy= -dy;
  path sp= find_innermost_scroll (eb, tp);
  if (is_nil (sp)) {
    SERVER (scroll_where (x, y));
    y+= dy;
    SERVER (scroll_to (x, y));
  }
  else {
    SI        x, y, sx, sy;
    rectangle outer, inner;
    find_canvas_info (eb, sp, x, y, sx, sy, outer, inner);
    SI ty= inner->y2 - inner->y1;
    SI cy= outer->y2 - outer->y1;
    if (ty > cy) {
      tree   old_yt= eb[path_up (sp)]->get_info ("scroll-y");
      string old_ys= as_string (old_yt);
      double old_p = 0.0;
      if (ends (old_ys, "%")) old_p= as_double (old_ys (0, N (old_ys) - 1));
      double new_p= old_p + 100.0 * ((double) dy) / ((double) (ty - cy));
      new_p       = max (min (new_p, 100.0), 0.0);
      tree new_yt = as_string (new_p) * "%";
      if (new_yt != old_yt && is_accessible (obtain_ip (old_yt))) {
        object fun= symbol_object ("tree-set");
        object cmd= list_object (fun, old_yt, new_yt);
        exec_delayed (scheme_cmd (cmd));
        temp_invalid_cursor= true;
      }
    }
  }
}

/******************************************************************************
 * getting the cursor (both for text and graphics)
 ******************************************************************************/

cursor
edit_interface_rep::get_cursor () {
  if (inside_graphics ()) {
    frame f= find_frame ();
    if (!is_nil (f)) {
      point p= f[point (last_x, last_y)];
      p      = f (adjust (p));
      SI x   = (SI) p[0];
      SI y   = (SI) p[1];
      return cursor (x, y, 0, -5 * pixel, 5 * pixel, 1.0);
    }
  }
  return copy (the_cursor ());
}

array<SI>
edit_interface_rep::get_mouse_position () {
  rectangle wr= get_window_extents ();
  SI        sz= get_pixel_size ();
  double    sf= ((double) sz) / 256.0;
  SI        mx= ((SI) (last_x / sf)) + wr->x1;
  SI        my= ((SI) (last_y / sf)) + wr->y2;
  return array<SI> (mx, my);
}

void
edit_interface_rep::set_pointer (string name) {
  send_mouse_pointer (this, name);
}

void
edit_interface_rep::set_pointer (string curs_name, string mask_name) {
  send_mouse_pointer (this, curs_name, mask_name);
}

// https://doc.qt.io/qt-5.15/qcursor.html
void
edit_interface_rep::set_cursor_style (string style_name) {
  QWidget* mainwindow= QApplication::activeWindow ();
  if (mainwindow == nullptr) return;
  if (style_name == "openhand") mainwindow->setCursor (Qt::OpenHandCursor);
  else if (style_name == "normal" || style_name == "top_left_arrow")
    mainwindow->setCursor (Qt::ArrowCursor);
  else if (style_name == "closehand")
    mainwindow->setCursor (Qt::ClosedHandCursor);
  else if (style_name == "cross") mainwindow->setCursor (Qt::CrossCursor);
  else if (style_name == "up_arrow") mainwindow->setCursor (Qt::UpArrowCursor);
  else if (style_name == "ibeam") mainwindow->setCursor (Qt::IBeamCursor);
  else if (style_name == "wait") mainwindow->setCursor (Qt::WaitCursor);
  else if (style_name == "fobidden")
    mainwindow->setCursor (Qt::ForbiddenCursor);
  else if (style_name == "pointing_hand")
    mainwindow->setCursor (Qt::PointingHandCursor);
  else if (style_name == "size_ver") mainwindow->setCursor (Qt::SizeVerCursor);
  else if (style_name == "size_hor") mainwindow->setCursor (Qt::SizeHorCursor);
  else if (style_name == "size_bdiag")
    mainwindow->setCursor (Qt::SizeBDiagCursor);
  else if (style_name == "size_fdiag")
    mainwindow->setCursor (Qt::SizeFDiagCursor);
  else if (style_name == "size_all") mainwindow->setCursor (Qt::SizeAllCursor);
  else TM_FAILED ("invalid cursor style");
}

/******************************************************************************
 * Active loci
 ******************************************************************************/

void
edit_interface_rep::update_mouse_loci () {
  if (is_nil (eb)) {
    locus_new_rects= rectangles ();
    mouse_ids      = list<string> ();
    return;
  }

  try {
    int  old_mode= set_access_mode (DRD_ACCESS_SOURCE);
    path cp      = path_up (tree_path (path (), last_x, last_y, 0));
    set_access_mode (old_mode);
    tree         mt= subtree (et, cp);
    path         p = cp;
    list<string> ids1, ids2;
    rectangles   rs1, rs2;
    eb->loci (last_x, last_y, 0, ids1, rs1);
    while (rp <= p) {
      ids2 << get_ids (subtree (et, p));
      p= path_up (p);
    }

    locus_new_rects= rectangles ();
    mouse_ids      = list<string> ();
    if (!is_nil (ids1 * ids2) && !has_changed (THE_FOCUS)) {
      ids1        = as_list_string (call ("link-mouse-ids", object (ids1)));
      ids2        = as_list_string (call ("link-mouse-ids", object (ids2)));
      list<tree> l= as_list_tree (call ("link-active-upwards", object (mt)));
      while (!is_nil (l)) {
        tree      lt= l->item;
        path      lp= reverse (obtain_ip (lt));
        selection sel=
            eb->find_check_selection (lp * start (lt), lp * end (lt));
        rs2 << outlines (sel->rs, pixel);
        l= l->next;
      }
      ids1= as_list_string (call ("link-active-ids", object (ids1)));
      ids2= as_list_string (call ("link-active-ids", object (ids2)));
      if (is_nil (ids1)) rs1= rectangles ();
      if (is_nil (ids2)) rs2= rectangles ();
      // FIXME: we should keep track which id corresponds to which rectangle
      if (!is_nil (ids1 * ids2)) {
        locus_new_rects= rs1 * rs2;
        mouse_ids      = ids1 * ids2;
      }
    }
    if (locus_new_rects != locus_rects) notify_change (THE_LOCUS);
  } catch (string msg) {
  }
  handle_exceptions ();
}

void
edit_interface_rep::update_focus_loci () {
  path         p= path_up (tp);
  list<string> ids;
  while (rp <= p) {
    ids << get_ids (subtree (et, p));
    p= path_up (p);
  }
  focus_ids= list<string> ();
  if (!is_nil (ids) && !has_changed (THE_FOCUS)) {
    ids      = as_list_string (call ("link-active-ids", object (ids)));
    focus_ids= ids;
  }
}

/******************************************************************************
 * drag and double click detection for left button
 ******************************************************************************/

static void*  left_handle       = NULL;
static bool   left_started      = false;
static bool   left_dragging     = false;
static SI     left_x            = 0;
static SI     left_y            = 0;
static time_t left_last         = 0;
static int    double_click_delay= 500;

void
drag_left_reset () {
  left_started = false;
  left_dragging= false;
  left_x       = 0;
  left_y       = 0;
}

void
disable_double_clicks () {
  left_last-= (double_click_delay + 1);
}

static string
detect_left_drag (void* handle, string type, SI x, SI y, time_t t, int m,
                  SI d) {
  if (left_handle != handle) drag_left_reset ();
  left_handle= handle;
  if (left_dragging && type == "move" && (m & 1) == 0) type= "release-left";
  if (type == "press-left") {
    left_dragging= true;
    left_started = true;
    left_x       = x;
    left_y       = y;
  }
  else if (type == "move") {
    if (left_started) {
      if (norm (point (x - left_x, y - left_y)) < d) return "wait-left";
      left_started= false;
      return "start-drag-left";
    }
    if (left_dragging) return "dragging-left";
  }
  else if (type == "release-left") {
    if (left_started) drag_left_reset ();
    if (left_dragging) {
      drag_left_reset ();
      return "end-drag-left";
    }
    if ((t >= left_last) && ((t - left_last) <= double_click_delay)) {
      left_last= t;
      return "double-left";
    }
    left_last= t;
  }
  return type;
}

/******************************************************************************
 * drag and double click detection for right button
 ******************************************************************************/

static void*  right_handle  = NULL;
static bool   right_started = false;
static bool   right_dragging= false;
static SI     right_x       = 0;
static SI     right_y       = 0;
static time_t right_last    = 0;

void
drag_right_reset () {
  right_started = false;
  right_dragging= false;
  right_x       = 0;
  right_y       = 0;
  right_last    = 0;
}

static string
detect_right_drag (void* handle, string type, SI x, SI y, time_t t, int m,
                   SI d) {
  if (right_handle != handle) drag_right_reset ();
  right_handle= handle;
  if (right_dragging && type == "move" && (m & 4) == 0) type= "release-right";
  if (type == "press-right") {
    right_dragging= true;
    right_started = true;
    right_x       = x;
    right_y       = y;
  }
  else if (type == "move") {
    if (right_started) {
      if (norm (point (x - right_x, y - right_y)) < d) return "wait-right";
      right_started= false;
      return "start-drag-right";
    }
    if (right_dragging) return "dragging-right";
  }
  else if (type == "release-right") {
    if (right_started) drag_right_reset ();
    if (right_dragging) {
      drag_right_reset ();
      return "end-drag-right";
    }
    if ((t >= right_last) && ((t - right_last) <= 500)) {
      right_last= t;
      return "double-right";
    }
    right_last= t;
  }
  return type;
}

/******************************************************************************
 * mouse detection for table line resizing
 ******************************************************************************/

bool
edit_interface_rep::table_resize_hit (SI x, SI y, table_hit& hit) {
  rectangles rs;
  tree       r= eb->message (tree ("table-loc?"), x, y, rs);
  if (!is_func (r, TUPLE) || r[0] != "table-loc") return false;

  string orient= as_string (r[1]);
  if ((orient != "col" && orient != "row") || N (r) < 10) return false;

  hit.vertical   = (orient == "col");
  hit.index      = as_int (r[2]);
  hit.first_size = as_int (r[4]);
  hit.second_size= as_int (r[5]);
  hit.fp         = as_path (as_string (r[8]));
  hit.wide_flag  = (as_string (r[9]) == "wide");

  if (is_nil (hit.fp) || hit.index <= 0) return false;
  return true;
}

void
edit_interface_rep::table_resize_start (const table_hit& hit, SI x, SI y) {
  path sp= find_innermost_scroll (eb, tp);
  path tp= tree_path (sp, x, y, 0);
  while (!is_nil (tp) && !has_subtree (et, tp))
    tp= path_up (tp);
  if (is_nil (tp)) return;

  path fp= ::table_search_format (et, tp);
  if (is_nil (fp)) return;

  table_resizing          = true;
  table_resize_vertical   = hit.vertical;
  table_resize_path       = fp;
  table_resize_index      = hit.index;
  table_resize_start_x    = x;
  table_resize_start_y    = y;
  table_resize_first_size = hit.first_size;
  table_resize_second_size= hit.second_size;
  table_resize_wide_flag  = hit.wide_flag;
  table_resize_mark       = new_marker ();
  mark_start (table_resize_mark);
}

void
edit_interface_rep::table_resize_apply (SI x, SI y) {
  if (!table_resizing || is_nil (table_resize_path)) return;

  edit_table_rep* et= dynamic_cast<edit_table_rep*> (this);
  if (et == nullptr) return;

  SI delta   = table_resize_vertical ? (x - table_resize_start_x)
                                     : (table_resize_start_y - y);
  SI min_size= 16 * PIXEL;

  SI first = table_resize_first_size + delta;
  SI second= table_resize_second_size - delta;

  if (table_resize_vertical) {
    if (table_resize_wide_flag && table_resize_second_size != -1) {
      if (first < min_size || second < min_size) return;

      int col1= table_resize_index;
      int col2= table_resize_index + 1;

      et->table_set_format_region (table_resize_path, 1, col1, -1, col1,
                                   "cell-hmode", tree ("exact"));
      et->table_set_format_region (table_resize_path, 1, col2, -1, col2,
                                   "cell-hmode", tree ("exact"));
      et->table_set_format_region (table_resize_path, 1, col1, -1, col1,
                                   "cell-width",
                                   tree (as_string (first) * string ("tmpt")));
      et->table_set_format_region (table_resize_path, 1, col2, -1, col2,
                                   "cell-width",
                                   tree (as_string (second) * string ("tmpt")));
    }
    else {
      if (first < min_size) return;

      int col= table_resize_index;

      et->table_set_format_region (table_resize_path, 1, col, -1, col,
                                   "cell-hmode", tree ("exact"));
      et->table_set_format_region (table_resize_path, 1, col, -1, col,
                                   "cell-width",
                                   tree (as_string (first) * string ("tmpt")));
    }
  }
  else {
    if (first < min_size) return;

    int row= table_resize_index;

    et->table_set_format_region (table_resize_path, row, 1, row, -1,
                                 "cell-vmode", tree ("exact"));
    et->table_set_format_region (table_resize_path, row, 1, row, -1,
                                 "cell-height",
                                 tree (as_string (first) * string ("tmpt")));
  }

  table_resize_notify ();
}

void
edit_interface_rep::table_resize_stop () {
  if (table_resize_mark != 0.0) {
    mark_end (table_resize_mark);
    table_resize_mark= 0.0;
  }
  table_resizing        = false;
  table_resize_path     = path ();
  table_resize_wide_flag= false;
}

/******************************************************************************
 * dispatching
 ******************************************************************************/

void
edit_interface_rep::mouse_any (string type, SI x, SI y, int mods, time_t t,
                               array<double> data) {
  // cout << "Mouse any " << type << ", " << x << ", " << y << "; " << mods <<
  // ", " << t << ", " << data << "\n";
  if (is_nil (eb)) return;
  if (t < last_t && (last_x != 0 || last_y != 0 || last_t != 0)) {
    // cout << "Ignored " << type << ", " << x << ", " << y << "; " << mods <<
    // ", " << t << "\n";
    return;
  }
  if (t > last_event) last_event= t;
  if (((x > last_x && !tremble_right) || (x < last_x && tremble_right)) &&
      (abs (x - last_x) > abs (y - last_y)) && type == "move") {
    tremble_count= min (tremble_count + 1, 35);
    tremble_right= (x > last_x);
    if (texmacs_time () - last_change > 500) {
      tremble_count= max (tremble_count - 1, 0);
      env_change   = env_change | (THE_CURSOR + THE_FREEZE);
      last_change  = texmacs_time ();
    }
    else if (tremble_count > 3) {
      env_change = env_change | (THE_CURSOR + THE_FREEZE);
      last_change= texmacs_time ();
    }
    // cout << "Tremble+ " << tremble_count << LF;
  }

  bool found_flag= false;
  path old_p     = eb->find_box_path (last_x, last_y, 0, false, found_flag);
  found_flag     = false;
  path new_p     = eb->find_box_path (x, y, 0, false, found_flag);
  if (path_up (old_p) != path_up (new_p)) {
    mouse_message ("leave", last_x, last_y);
    mouse_message ("enter", x, y);
  }

  if (!starts (type, "swipe-") && !starts (type, "pinch-") && type != "scale" &&
      type != "rotate" && type != "wheel") {
    last_x= x;
    last_y= y;
    last_t= t;
  }

  bool move_like=
      (type == "move" || type == "dragging-left" || type == "dragging-right");
  if ((!move_like) || (is_attached (this) && !check_event (MOTION_EVENT)))
    update_mouse_loci ();

  int hovering_table= 0;
  if (type == "move" || type == "dragging-left" || type == "dragging-right") {
    rectangles rs;
    tree       r= eb->message (tree ("table-loc?"), x, y, rs);
    if (is_func (r, TUPLE) && r[0] == "table-loc") {
      string orient= as_string (r[1]);
      if (orient == "cell") hovering_table= -1;
      else if (orient == "row") hovering_table= 1;
      else if (orient == "col") hovering_table= 2;
    }
  }
  bool hovering_hlink= false;
  if (!is_nil (mouse_ids) && type == "move") {
    notify_change (THE_FREEZE);
    // NOTE: this notification is needed to prevent the window to scroll to
    // the current cursor position when hovering over the locus
    // but a cleaner solution would be welcome
    call ("link-follow-ids", object (mouse_ids), object ("mouse-over"));
    bool is_inner_link=
        as_bool (call ("link-contains-inner-link?", object (mouse_ids)));
    if (!is_inner_link) {
      call ("show-hlink-tooltip", object (mouse_ids));
      hovering_hlink= true;
    }
  }
  bool             hovering_image= false;
  bool             over_handles  = false;
  string           handle_cursor = "";
  static path      current_path  = path ();
  static rectangle selr          = rectangle ();
  if (type == "move") {
    if (!is_zero (last_image_brec)) { // already clicked on image
      // 检测鼠标是否在handles上
      SI        handle_r= last_image_hr > 0 ? last_image_hr : 10 * pixel;
      rectangle h       = last_image_brec;
      SI        x1      = h->x1 + handle_r;
      SI        y1      = h->y1 + handle_r;
      SI        x2      = h->x2 - handle_r;
      SI        y2      = h->y2 - handle_r;
      SI        mx      = (x1 + x2) / 2;
      SI        my      = (y1 + y2) / 2;
      SI        hx[8]   = {x1, x2, x1, x2, mx, mx, x1, x2};
      SI        hy[8]   = {y1, y1, y2, y2, y1, y2, my, my};
      for (int i= 0; i < 8 && !over_handles; i++) {
        int dx= x - hx[i];
        int dy= y - hy[i];
        if (1ll * dx * dx + 1ll * dy * dy <= 1ll * handle_r * handle_r)
          over_handles= true;
        if (over_handles) {
          if (i == 0 || i == 3) handle_cursor= "size_bdiag";      // sw / ne
          else if (i == 1 || i == 2) handle_cursor= "size_fdiag"; // se / nw
          else if (i == 4 || i == 5) handle_cursor= "size_ver"; // south / north
          else if (i == 6 || i == 7) handle_cursor= "size_hor"; // west / east
        }
      }
      hovering_image= false;
    }
    else {
      // 检测鼠标是否在图片上
      current_path     = path_up (tree_path (path (), x, y, 0));
      tree current_tree= subtree (et, current_path);
      // 检查当前元素是否是图片
      if (is_func (current_tree, IMAGE)) {
        path      p= reverse (obtain_ip (current_tree));
        selection sel=
            search_selection (p * start (current_tree), p * end (current_tree));
        selr= least_upper_bound (sel->rs);
        if (last_x >= selr->x1 && last_y >= selr->y1 && last_x <= selr->x2 &&
            last_y <= selr->y2 * 0.8) {
          hovering_image= true;
        }
      }
    }
  }
  if (over_handles) {
    if (handle_cursor != "") set_cursor_style (handle_cursor);
    else set_cursor_style ("size_all");
  }
  else if (hovering_table) {
    if (hovering_table == -1) {
      set_cursor_style ("normal");
      // draw table resizing handles
    }
    else set_cursor_style (hovering_table == 1 ? "size_ver" : "size_hor");
  }
  else if (hovering_hlink) set_cursor_style ("pointing_hand");
  else if (hovering_image) {
    set_cursor_style ("pointing_hand");
    path path_of_image_parent= path_up (current_path);
    tree tree_of_image_parent= subtree (et, path_of_image_parent);
    if (should_show_image_popup (tree_of_image_parent)) {
      show_image_popup (tree_of_image_parent, selr, magf, get_scroll_x (),
                        get_scroll_y (), get_canvas_x (), get_canvas_y ());
    }
    hide_text_toolbar ();
  }
  else {
    set_cursor_style ("normal");
    hide_image_popup ();

    // 检查是否应该显示文本工具栏
    update_text_toolbar ();
  }

  if (type == "move") mouse_message ("move", x, y);

  if (type == "leave") set_pointer ("XC_top_left_arrow");
  if ((!move_like) && (type != "enter") && (type != "leave"))
    set_input_normal ();
  if (!is_nil (popup_win) && (type != "leave")) {
    set_visibility (popup_win, false);
    destroy_window_widget (popup_win);
    popup_win= widget ();
  }

  if (starts (type, "swipe-")) eval ("(" * type * ")");
  if (type == "pinch-start") eval ("(pinch-start)");
  if (type == "pinch-end") eval ("(pinch-end)");
  if (type == "scale") eval ("(pinch-scale " * as_string (data[0]) * ")");
  if (type == "rotate") eval ("(pinch-rotate " * as_string (-data[0]) * ")");
  if (starts (type, "press-")) {
    prev_math_comb= "";
    hide_math_completion_popup ();
    hide_completion_popup ();
  }

  // if (inside_graphics (false)) {
  // if (inside_graphics ()) {
  if (inside_graphics (type != "release-left")) {
    if (mouse_graphics (type, x, y, mods, t, data)) {
      if (is_in_graphics_mode) return;
      else {
        if (type == "press-left") {
          is_in_graphics_mode= true;
        }
        eval ("(set-cursor-style-now)");
        return;
      }
    }
    if (!over_graphics (x, y)) {
      eval ("(graphics-reset-context 'text-cursor)");
      if (type == "press-left") {
        set_cursor_style ("normal");
        is_in_graphics_mode= false;
      }
    };
  }

  // table line resizing
  if ((type == "press-left" || type == "start-drag-left") && mods <= 1 &&
      !table_resizing) {
    table_hit hit;
    if (table_resize_hit (x, y, hit)) {
      table_resize_start (hit, x, y);
      return;
    }
  }
  if (type == "dragging-left" && table_resizing) {
    table_resize_apply (x, y);
    return;
  }
  if ((type == "release-left" || type == "end-drag-left") && table_resizing) {
    table_resize_stop ();
    return;
  }

  if (type == "press-left" || type == "start-drag-left") {
    if (mods > 1) {
      mouse_adjusting= mods;
      mouse_adjust_selection (x, y, mods);
    }
    else mouse_click (x, y);
  }
  if (type == "dragging-left") {
    if (mouse_adjusting && mods > 1) {
      mouse_adjusting= mods;
      mouse_adjust_selection (x, y, mods);
    }
    else if (is_attached (this) && check_event (DRAG_EVENT)) return;
    else mouse_drag (x, y);
  }
  if ((type == "release-left" || type == "end-drag-left")) {
    if (!(mouse_adjusting & ShiftMask))
      mouse_select (x, y, mods, type == "end-drag-left");
    mouse_adjusting&= ~mouse_adjusting;
    send_mouse_grab (this, false);
  }

  if (type == "double-left") {
    send_mouse_grab (this, false);
    if (mouse_extra_click (x, y)) drag_left_reset ();
  }
  if (type == "press-middle") mouse_paste (x, y);
  if (type == "press-right") mouse_adjust (x, y, mods);
  if (type == "press-up") mouse_scroll (x, y, true);
  if (type == "press-down") mouse_scroll (x, y, false);

  if ((type == "press-left") || (type == "release-left") ||
      (type == "end-drag-left") || (type == "press-middle") ||
      (type == "press-right")) {
    // 当用户点击其他地方（不在文本工具栏内）时，隐藏文本工具栏
    if (!is_point_in_text_toolbar (x, y)) {
      hide_text_toolbar ();
    }
    notify_change (THE_DECORATIONS);
  }

  if (type == "wheel" && N (data) == 2)
    eval ("(wheel-event " * as_string (data[0]) * " " * as_string (data[1]) *
          ")");
}

/******************************************************************************
 * Event handlers
 ******************************************************************************/

static tree
relativize (tree t, url base) {
  if (is_atomic (t)) return t;
  else {
    tree r (t, N (t));
    for (int i= 0; i < N (t); i++)
      r[i]= relativize (t[i], base);
    if (is_func (r, IMAGE) && N (r) >= 1 && is_atomic (r[0])) {
      url name= url_system (r[0]->label);
      if (descends (name, head (base))) r[0]= as_string (delta (base, name));
    }
    return r;
  }
}

static void
call_drop_event (string kind, SI x, SI y, SI ticket, time_t t, url base) {
#ifdef QTTEXMACS
  (void) kind;
  (void) x;
  (void) y;
  (void) t;
  extern hashmap<int, tree> payloads;
  tree                      doc= payloads[ticket];
  payloads->reset (ticket);
  array<object> args;
  args << object (x) << object (y) << object (relativize (doc, base));
  call ("mouse-drop-event", args);
  // eval (list_object (symbol_object ("insert"), relativize (doc, base)));
  // array<object> args;
  // args << object (kind) << object (x) << object (y)
  //<< object (doc) << object ((double) t);
  // call ("mouse-event", args);
#else
  (void) kind;
  (void) x;
  (void) y;
  (void) ticket;
  (void) t;
#endif
}

static void
call_mouse_event (string kind, SI x, SI y, SI m, time_t t, array<double> d) {
  array<object> args;
  args << object (kind) << object (x) << object (y) << object (m)
       << object ((double) t) << object (d);
  call ("mouse-event", args);
}

static string
as_scm_string (array<double> a) {
  string s= "(list";
  for (int i= 0; i < N (a); i++)
    s << " " << as_string (a[i]);
  s << ")";
  return s;
}

static void
delayed_call_mouse_event (string kind, SI x, SI y, SI m, time_t t,
                          array<double> d) {
  // NOTE: interestingly, the (:idle 1) is not necessary for the Qt port
  // but is required for appropriate updating when using the X11 port
  string cmd= "(delayed (:idle 1) (mouse-event " * scm_quote (kind) * " " *
              as_string (x) * " " * as_string (y) * " " * as_string (m) * " " *
              as_string ((int64_t) t) * " " * as_scm_string (d) * "))";
  eval (cmd);
}

void
edit_interface_rep::handle_mouse (string kind, SI x, SI y, int m, time_t t,
                                  array<double> data) {
  if (is_nil (buf)) return;
  if (kind != "move" || selection_active_any ()) set_user_active (true);
  bool started= false;
  try {
    if (is_nil (eb) || (env_change & (THE_TREE + THE_ENVIRONMENT)) != 0) {
      // cout << "handle_mouse in " << buf->buf->name << ", " << got_focus <<
      // LF; cout << kind << " (" << x << ", " << y << "; " << m << ", " << data
      // << ")"
      //      << " at " << t << "\n";
      if (!got_focus) return;
      apply_changes ();
    }
    start_editing ();
    started= true;
    x      = ((SI) (x / magf));
    y      = ((SI) (y / magf));
    // cout << kind << " (" << x << ", " << y << "; " << m << ", " << data <<
    // ")"
    //      << " at " << t << "\n";

    if (kind == "drop") {
      call_drop_event (kind, x, y, m, t, buf->buf->name);
      if (inside_graphics (true))
        mouse_graphics ("drop-object", x, y, m, t, data);
    }
    else {
      string rew = kind;
      SI     dist= (SI) (5 * PIXEL / magf);
      rew        = detect_left_drag ((void*) this, rew, x, y, t, m, dist);
      if (rew == "start-drag-left") {
        call_mouse_event (rew, left_x, left_y, m, t, data);
        delayed_call_mouse_event ("dragging-left", x, y, m, t, data);
      }
      else {
        rew= detect_right_drag ((void*) this, rew, x, y, t, m, dist);
        if (rew == "start-drag-right") {
          call_mouse_event (rew, right_x, right_y, m, t, data);
          delayed_call_mouse_event ("dragging-right", x, y, m, t, data);
        }
        else call_mouse_event (rew, x, y, m, t, data);
      }
    }
    end_editing ();
  } catch (string msg) {
    if (started) cancel_editing ();
  }
  handle_exceptions ();
}

/******************************************************************************
 * Text toolbar support
 ******************************************************************************/

bool
edit_interface_rep::should_show_text_toolbar () {
  // 检查是否有活动的文本选区
  if (!selection_active_any ()) return false;

  // 检查选区是否非空
  tree sel_tree= selection_get ();
  if (is_atomic (sel_tree) && as_string (sel_tree) == "") return false;

  return true;
}

rectangle
edit_interface_rep::get_text_selection_rect () {
  rectangle sel_rect;

  if (selection_active_any () && !is_nil (selection_rects)) {
    // 使用现有的选区矩形
    sel_rect= least_upper_bound (selection_rects);
  }
  else if (selection_active_any ()) {
    // 如果没有选区矩形，但选区存在，计算一个默认矩形
    path p1, p2;
    selection_get (p1, p2);
    if (p1 != p2) {
      selection sel= search_selection (p1, p2);
      if (!is_nil (sel->rs)) {
        sel_rect= least_upper_bound (sel->rs);
      }
      else {
        // 如果选区矩形为空，使用光标位置创建一个最小矩形
        cursor cu= get_cursor ();
        sel_rect = rectangle (cu->ox - 10 * pixel, cu->oy - 5 * pixel,
                              cu->ox + 10 * pixel, cu->oy + 5 * pixel);
      }
    }
  }

  return sel_rect;
}

void
edit_interface_rep::show_text_toolbar (rectangle selr, double magf,
                                       int scroll_x, int scroll_y, int canvas_x,
                                       int canvas_y) {
  // 通过qt_simple_widget显示文本工具栏
  // this指针实际上是edit_interface_rep，它继承自editor_rep，而editor_rep继承自simple_widget_rep
  // 在Qt环境下，simple_widget_rep实际上是qt_simple_widget_rep
  qt_simple_widget_rep* qsw= static_cast<qt_simple_widget_rep*> (this);
  qsw->show_text_toolbar (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
}

void
edit_interface_rep::hide_text_toolbar () {
  // 通过qt_simple_widget隐藏文本工具栏
  qt_simple_widget_rep* qsw= static_cast<qt_simple_widget_rep*> (this);
  qsw->hide_text_toolbar ();
}

bool
edit_interface_rep::is_point_in_text_toolbar (SI x, SI y) {
  // 通过qt_simple_widget检查点是否在文本工具栏内
  qt_simple_widget_rep* qsw= static_cast<qt_simple_widget_rep*> (this);
  return qsw->is_point_in_text_toolbar (x, y);
}

void
edit_interface_rep::update_text_toolbar () {
  // 检查是否应该显示文本工具栏
  if (should_show_text_toolbar ()) {
    rectangle text_selr= get_text_selection_rect ();
    // 检查矩形是否有效（非零面积）
    // 注意：rectangle 不是 list 类型，不能使用 is_nil
    // 我们检查矩形坐标是否有效
    if (text_selr->x1 < text_selr->x2 && text_selr->y1 < text_selr->y2) {
      show_text_toolbar (text_selr, magf, get_scroll_x (), get_scroll_y (),
                         get_canvas_x (), get_canvas_y ());
    }
    else {
      // 即使矩形无效，也尝试显示工具栏（例如单个字符选区）
      hide_text_toolbar ();
    }
  }
  else {
    hide_text_toolbar ();
  }
}
