/******************************************************************************
 * MODULE     : widget_rasterize_test.cpp
 * DESCRIPTION: 单元测试 widget → QImage → base64 PNG data URL 光栅化。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "qt_simple_widget.hpp" // qt_simple_widget_rep / concrete_simple_widget
#include "qt_widget_rasterize.hpp" // rasterize_widget_to_png_data_url

#include "base.hpp"
#include "renderer.hpp" // renderer + set_brush 等

#include <QtTest/QtTest>

static const int DATA_URL_PREFIX_LEN= 22;

// 测试本地 stub：固定 100×60 逻辑像素，handle_repaint 填一个红色矩形。
class stub_simple_widget_rep : public qt_simple_widget_rep {
public:
  void handle_get_size_hint (SI& w, SI& h) override {
    w= 100 * PIXEL;
    h= 60 * PIXEL;
  }
  void handle_repaint (renderer win, SI x1, SI y1, SI x2, SI y2) override {
    win->set_background (rgb_color (200, 40, 40));
    win->clear (x1, y1, x2, y2);
  }
};

class TestWidgetRasterize : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_rasterize_simple_widget ();
  void test_rasterize_nil_returns_empty ();
};

void
TestWidgetRasterize::test_rasterize_simple_widget () {
  widget wid (tm_new<stub_simple_widget_rep> ());
  QVERIFY (!is_nil (wid));
  string url= rasterize_widget_to_png_data_url (wid);
  QVERIFY2 (starts (url, "data:image/png;base64,"), "data URL 前缀不对");
  QVERIFY2 (N (url) > DATA_URL_PREFIX_LEN + 200, "光栅化产物过小，疑似空图");
}

void
TestWidgetRasterize::test_rasterize_nil_returns_empty () {
  widget wid; // nil widget
  string url= rasterize_widget_to_png_data_url (wid);
  QCOMPARE (url, string (""));
}

#ifdef QTTEXMACS
QTEST_MAIN (TestWidgetRasterize)
#else
int
main () {
  return 0;
}
#endif
#include "widget_rasterize_test.moc"
