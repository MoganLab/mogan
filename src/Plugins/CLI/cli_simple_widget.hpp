
/******************************************************************************
 * MODULE     : cli_simple_widget.hpp
 * DESCRIPTION: CLI 前端的 simple_widget_rep
 *
 * editor_rep 继承自 simple_widget_rep（typedef）并重写 handle_*()。CLI 不上屏，
 * 渲染走 make_raster_image，故 handle_*() 与槽分发均用最小空实现；槽的默认
 * 行为继承自 widget_rep。
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef CLI_SIMPLE_WIDGET_HPP
#define CLI_SIMPLE_WIDGET_HPP

#include "cli_widget.hpp"
#include "renderer.hpp"

class cli_simple_widget_rep : public cli_widget_rep {
public:
  inline cli_simple_widget_rep () {}

  virtual bool is_editor_widget () { return false; }
  virtual bool is_embedded_widget () { return false; }

  virtual void handle_get_size_hint (SI& w, SI& h) {
    (void) w;
    (void) h;
  }
  virtual void handle_notify_resize (SI w, SI h) {
    (void) w;
    (void) h;
  }
  virtual void handle_keypress (string key, time_t t) {
    (void) key;
    (void) t;
  }
  virtual void handle_keyboard_focus (bool has_focus, time_t t) {
    (void) has_focus;
    (void) t;
  }
  virtual void handle_mouse (string kind, SI x, SI y, int mods, time_t t,
                             array<double> data= array<double> ()) {
    (void) kind;
    (void) x;
    (void) y;
    (void) mods;
    (void) t;
    (void) data;
  }
  virtual void handle_set_zoom_factor (double zoom) { (void) zoom; }
  virtual void handle_clear (renderer ren, SI x1, SI y1, SI x2, SI y2) {
    (void) ren;
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
  }
  virtual void handle_repaint (renderer ren, SI x1, SI y1, SI x2, SI y2) {
    (void) ren;
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
  }
};

typedef cli_simple_widget_rep simple_widget_rep;

#endif // defined CLI_SIMPLE_WIDGET_HPP
