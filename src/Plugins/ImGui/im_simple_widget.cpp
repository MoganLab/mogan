
/******************************************************************************
 * MODULE     : im_simple_widget.cpp
 * DESCRIPTION: ImGui canvas widget holding a TeXmacs editor.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "gui.hpp" // gui_root_extents
#include "message.hpp"

#include "im_simple_widget.hpp"

/******************************************************************************
 * Constructor
 ******************************************************************************/

im_simple_widget_rep::im_simple_widget_rep ()
    : im_widget_rep (), is_dirty (false), zoomf (1.0), canvas_w (0),
      canvas_h (0), scroll_x (0), scroll_y (0), ext_x1 (0), ext_y1 (0),
      ext_x2 (0), ext_y2 (0), owning_win_id (0) {
  // editor_rep derives from this and overrides the handle_*() hooks;
  // the base defaults below are no-ops
}

void
im_simple_widget_rep::set_window (widget win, int id) {
  owning_window= win;
  owning_win_id= id;
}

/******************************************************************************
 * Editor identification (overridden by edit_interface_rep)
 ******************************************************************************/

bool
im_simple_widget_rep::is_editor_widget () {
  return false;
}

bool
im_simple_widget_rep::is_embedded_widget () {
  return false;
}

/******************************************************************************
 * Default canvas hooks (overridden by editor_rep). No-op defaults match
 * qt_simple_widget_rep.
 ******************************************************************************/

void
im_simple_widget_rep::handle_get_size_hint (SI& w, SI& h) {
  gui_root_extents (w, h);
}

void
im_simple_widget_rep::handle_notify_resize (SI w, SI h) {
  (void) w;
  (void) h;
}

void
im_simple_widget_rep::handle_keypress (string key, time_t t) {
  (void) key;
  (void) t;
}

void
im_simple_widget_rep::handle_keyboard_focus (bool has_focus, time_t t) {
  (void) has_focus;
  (void) t;
}

void
im_simple_widget_rep::handle_mouse (string kind, SI x, SI y, int mods, time_t t,
                                    array<double> data) {
  (void) kind;
  (void) x;
  (void) y;
  (void) mods;
  (void) t;
  (void) data;
}

void
im_simple_widget_rep::handle_set_zoom_factor (double zoom) {
  (void) zoom;
}

void
im_simple_widget_rep::handle_clear (renderer ren, SI x1, SI y1, SI x2, SI y2) {
  (void) ren;
  (void) x1;
  (void) y1;
  (void) x2;
  (void) y2;
}

void
im_simple_widget_rep::handle_repaint (renderer ren, SI x1, SI y1, SI x2,
                                      SI y2) {
  (void) ren;
  (void) x1;
  (void) y1;
  (void) x2;
  (void) y2;
}

/******************************************************************************
 * Invalidation
 ******************************************************************************/

void
im_simple_widget_rep::invalidate_rect (SI x1, SI y1, SI x2, SI y2) {
  rectangle r    = rectangle (x1, y1, x2, y2);
  invalid_regions= invalid_regions | rectangles (r);
}

void
im_simple_widget_rep::invalidate_all () {
  invalid_regions= rectangles ();
  invalidate_rect (0, 0, canvas_w, canvas_h);
  is_dirty= true;
}

bool
im_simple_widget_rep::is_invalid () {
  return is_dirty || !is_nil (invalid_regions);
}

void
im_simple_widget_rep::clear_invalid () {
  is_dirty       = false;
  invalid_regions= rectangles ();
}

void
im_simple_widget_rep::recenter () {
  // 水平：文档窄于画布则水平居中（负偏移把文档右移），否则贴左（ImGui
  // 无水平滚动）。
  SI page_w= ext_x2 - ext_x1;
  if (page_w > 0 && page_w < canvas_w) scroll_x= -((canvas_w - page_w) / 2);
  else scroll_x= 0;
  // 垂直：始终上对齐——短文档也贴顶（不居中，否则文档会被推到下方、与编辑时的
  // 位置不一致）；仅当文档高于画布时，把滚动钳位到可滚动范围
  // [-(doc_h-canvas_h), 0]（0=顶部，负=向下滚动）。
  SI doc_h= ext_y2 - ext_y1;
  if (doc_h >= canvas_h) {
    SI min_sy= -(doc_h - canvas_h);
    if (scroll_y < min_sy) scroll_y= min_sy;
    if (scroll_y > 0) scroll_y= 0;
  }
  else scroll_y= 0;
}

void
im_simple_widget_rep::scroll_by (SI dx, SI dy) {
  // 增量滚动：直接累加到视口上沿，交 recenter 钳位。不经过 SLOT_SCROLL_POSITION
  // 的居中换算（那是 editor 绝对定位专用）。
  scroll_x+= dx;
  scroll_y+= dy;
  recenter ();
}

/******************************************************************************
 * Handling of TeXmacs' messages (canvas slots).
 ******************************************************************************/

void
im_simple_widget_rep::send (slot s, blackbox val) {
  switch (s) {
  case SLOT_INVALIDATE: {
    coord4 p= open_box<coord4> (val);
    invalidate_rect (p.x1, p.x2, p.x3, p.x4);
  } break;
  case SLOT_INVALIDATE_ALL: {
    invalidate_all ();
  } break;
  case SLOT_EXTENTS: {
    coord4 p= open_box<coord4> (val);
    ext_x1  = p.x1;
    ext_y1  = p.x2;
    ext_x2  = p.x3;
    ext_y2  = p.x4;
    recenter ();
  } break;
  case SLOT_SIZE: {
    coord2 p= open_box<coord2> (val);
    canvas_w= p.x1;
    canvas_h= p.x2;
    recenter ();
  } break;
  case SLOT_SCROLL_POSITION: {
    // editor 的 make-cursor-visible / scroll_to 下达的 (x, y) 是「希望居于视口
    // 中央」的点，与 Qt 一致——Qt 在 setOrigin 前减去半个 surface 尺寸
    // （"adjust because child is centered"），使光标落到视口正中而非贴到上沿。
    // 这里把中心点换算成视口上沿 scroll_y（= y + canvas_h/2）存储；查询
    // SLOT_SCROLL_POSITION 仍返回上沿，与 Qt 返回 origin 同构，弹出框定位等
    // 依赖「上沿」语义的调用方不受影响。增量滚动（滚轮/拖选）走 scroll_by，
    // 不经此分支，避免每帧叠加半个画布的偏移。水平始终由 recenter 居中。
    coord2 p= open_box<coord2> (val);
    scroll_y= p.x2 + (canvas_h >> 1);
    recenter ();
  } break;
  case SLOT_ZOOM_FACTOR: {
    double new_zoom= open_box<double> (val);
    zoomf          = new_zoom;
    handle_set_zoom_factor (new_zoom);
  } break;
  case SLOT_MOUSE_GRAB:
  case SLOT_MOUSE_POINTER:
  case SLOT_CURSOR:
    // honoured once the ImGui event loop forwards input; no-op for now
    break;
  default:
    im_widget_rep::send (s, val);
    return;
  }
}

blackbox
im_simple_widget_rep::query (slot s, int type_id) {
  (void) type_id; // open_box<T> already asserts the caller's expected type
  switch (s) {
  case SLOT_IDENTIFIER:
    return close_box<int> (owning_win_id);
  case SLOT_INVALID:
    return close_box<bool> (is_invalid ());
  case SLOT_POSITION:
    return close_box<coord2> (coord2 (0, 0));
  case SLOT_SIZE:
    return close_box<coord2> (coord2 (canvas_w, canvas_h));
  case SLOT_SCROLL_POSITION:
    return close_box<coord2> (coord2 (scroll_x, scroll_y));
  case SLOT_EXTENTS:
    return close_box<coord4> (coord4 (ext_x1, ext_y1, ext_x2, ext_y2));
  case SLOT_VISIBLE_PART: {
    return close_box<coord4> (
        coord4 (scroll_x, -canvas_h + scroll_y, canvas_w + scroll_x, scroll_y));
  }
  default:
    return im_widget_rep::query (s, type_id);
  }
}

widget
im_simple_widget_rep::read (slot s, blackbox index) {
  (void) index;
  switch (s) {
  case SLOT_WINDOW:
    // the owning im_tm_widget_rep, if attached
    return owning_window;
  default:
    return im_widget_rep::read (s, index);
  }
}
