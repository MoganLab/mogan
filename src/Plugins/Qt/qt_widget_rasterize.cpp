
/******************************************************************************
 * MODULE      : qt_widget_rasterize.cpp
 * DESCRIPTION : 把一个 TeXmacs widget 光栅化为 PNG data URL（见配套 .hpp）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "qt_widget_rasterize.hpp"

#include "qt_renderer.hpp"
#include "qt_simple_widget.hpp"
#include "qt_utilities.hpp"

#include "qt_gui.hpp" // the_gui

#include <QBuffer>
#include <QImage>

/**
 * @brief 照搬 impress()（qt_simple_widget.cpp），目标改为 QImage。
 *
 * @details QImage 按 retina_factor 放大物理分辨率（hidpi 2× 像素），QML Image
 * 按逻辑 尺寸降采样显示，清晰不糊。the_qt_renderer()->begin(void*) 接受任意
 * QPaintDevice*， QImage 即 QPaintDevice，无需额外 surface。wid 非
 * simple_widget 时返回空。
 */
string
rasterize_widget_to_png_data_url (widget wid) {
  qt_simple_widget_rep* rep= concrete_simple_widget (wid);
  if (rep == nullptr) return "";

  int width, height;
  rep->handle_get_size_hint (width, height);
  QSize s     = to_qsize (width, height);
  QSize phys_s= s;
  phys_s*= retina_factor;

  QImage img (phys_s, QImage::Format_ARGB32_Premultiplied);
  if (img.isNull ()) return "";
  img.fill (Qt::transparent);
  {
    qt_renderer_rep* ren= the_qt_renderer ();
    ren->begin (static_cast<QPaintDevice*> (&img));
    rectangle r= rectangle (0, 0, phys_s.width (), phys_s.height ());
    ren->set_origin (0, 0);
    ren->encode (r->x1, r->y1);
    ren->encode (r->x2, r->y2);
    ren->set_clipping (r->x1, r->y2, r->x2, r->y1);
    {
      // 与 impress() 同：绘制期间不希望被事件中断。the_gui 在非主程序上下文
      // （单测）可能未构造 = nullptr，守卫之。
      bool has_gui= (the_gui != nullptr);
      if (has_gui) the_gui->set_check_events (false);
      rep->handle_repaint (ren, r->x1, r->y2, r->x2, r->y1);
      if (has_gui) the_gui->set_check_events (true);
    }
    ren->end ();
  }

  QByteArray ba;
  QBuffer    buf (&ba);
  if (!buf.open (QIODevice::WriteOnly)) return "";
  if (!img.save (&buf, "PNG")) return "";
  string data_url= "data:image/png;base64,";
  return data_url * from_qstring (QString::fromLatin1 (ba.toBase64 ()));
}

string
cpp_rasterize_widget (widget wid) {
  return rasterize_widget_to_png_data_url (wid);
}
