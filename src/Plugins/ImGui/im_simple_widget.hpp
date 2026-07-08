
/******************************************************************************
 * MODULE     : im_simple_widget.hpp
 * DESCRIPTION: ImGui canvas widget holding a TeXmacs editor.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_SIMPLE_WIDGET_HPP
#define IM_SIMPLE_WIDGET_HPP

#include "rectangles.hpp"
#include "renderer.hpp"

#include "im_widget.hpp"

/*
 包含 TeXmacs 画布的控件

 Note:

 editor_rep 继承自 simple_widget_rep（typedef），并重写此处声明的
 handle_*() 虚函数。

 ImGui 负责将其原生事件转换为对 handle_*() 的调用，并驱动重绘：

 与 Qt (QTMWidget) 不同，ImGui 没有持久的原生画布控件；所有几何信息
 和滚动状态都以普通的 SI 值保存在此类中。
*/
class im_simple_widget_rep : public im_widget_rep {
public:
  im_simple_widget_rep ();

  virtual bool is_editor_widget ();
  virtual bool is_embedded_widget ();

  virtual void handle_get_size_hint (SI& w, SI& h);
  virtual void handle_notify_resize (SI w, SI h);
  virtual void handle_keypress (string key, time_t t);
  virtual void handle_keyboard_focus (bool has_focus, time_t t);
  virtual void handle_mouse (string kind, SI x, SI y, int mods, time_t t,
                             array<double> data= array<double> ());
  virtual void handle_set_zoom_factor (double zoom);
  virtual void handle_clear (renderer ren, SI x1, SI y1, SI x2, SI y2);
  virtual void handle_repaint (renderer ren, SI x1, SI y1, SI x2, SI y2);

  ////////////////////// Handling of TeXmacs' messages (canvas slots)

  virtual void     send (slot s, blackbox val);
  virtual blackbox query (slot s, int type_id);
  virtual widget   read (slot s, blackbox index);

  ////////////////////// ImGui rendering entry point

  // Notify the canvas of the OS window that contains it (set by
  // im_tm_widget_rep) so that SLOT_WINDOW / SLOT_IDENTIFIER can be answered.
  void set_window (widget win, int id); // TODO: implement id via SLOTs

protected:
  rectangles invalid_regions;
  bool       is_dirty; // whole-canvas invalidation flag

  double zoomf;
  SI     canvas_w, canvas_h;             // visible canvas size (SI pixels)
  SI     scroll_x, scroll_y;             // scroll origin (SI pixels)
  SI     ext_x1, ext_y1, ext_x2, ext_y2; // document(page) extents (SI pixels)

  widget owning_window;
  int    owning_win_id; // SLOT_IDENTIFIER of owning_window, 0 if detached

  void invalidate_rect (SI x1, SI y1, SI x2, SI y2);
  void invalidate_all ();
  bool is_invalid ();

  void recenter_x ();
  void clamp_scroll_y ();
};

typedef im_simple_widget_rep simple_widget_rep;

#endif // defined IM_SIMPLE_WIDGET_HPP
