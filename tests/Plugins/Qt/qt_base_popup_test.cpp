
/******************************************************************************
 * MODULE     : qt_base_popup_test.cpp
 * DESCRIPTION: Tests for QTMBasePopup coordinate mapping
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMBasePopup.hpp"
#include "QTMScrollView.hpp"
#include "base.hpp"
#include "qt_simple_widget.hpp"

#include <QtTest/QtTest>

class DummyBasePopup : public QTMBasePopup {
public:
  DummyBasePopup (QWidget* parent, qt_simple_widget_rep* owner)
      : QTMBasePopup (parent, owner) {}

  void showPopup (qt_renderer_rep* ren, rectangle selr, double magf,
                  int scroll_x, int scroll_y, int canvas_x,
                  int canvas_y) override {
    (void) ren;
    cachePosition (selr, magf, scroll_x, scroll_y, canvas_x, canvas_y);
  }

  using QTMBasePopup::cachePosition;
  using QTMBasePopup::selectionInView;
  using QTMBasePopup::selectionRectPixels;
};

class stub_popup_widget_rep : public qt_simple_widget_rep {
public:
  stub_popup_widget_rep (QTMScrollView* sv) { qwid= sv; }
};

class TestQTMBasePopup : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_selection_rect_ignores_window_canvas_offset ();
};

void
TestQTMBasePopup::test_selection_rect_ignores_window_canvas_offset () {
  QWidget host;
  host.resize (1200, 800);
  QTMScrollView sv (&host);
  sv.resize (host.size ());
  host.show ();
  QTest::qWait (50);

  stub_popup_widget_rep owner (&sv);
  DummyBasePopup        popup (&sv, &owner);

  // 选区在逻辑坐标中（TeXmacs 笛卡尔坐标系：页面向下 y 为负）
  rectangle selr (100 * PIXEL, -200 * PIXEL, 200 * PIXEL, -180 * PIXEL);
  // canvas_x 模拟带左侧侧边栏时 get_canvas_x 的取值：surface 相对主窗口的
  // 全局偏移（300px × SCREEN_PIXEL）。错误实现会把它叠加进选区像素坐标，
  // 导致悬浮框相对父控件（scrollarea）右偏 300px
  popup.cachePosition (selr, 1.0, 0, 0, 300 * PIXEL, 0);

  double x1_px, x2_px, top_px, bottom_px;
  popup.selectionRectPixels (x1_px, x2_px, top_px, bottom_px);

  // 选区宽 100px：x2_px - x1_px 必须精确等于 100.0
  QCOMPARE (x2_px - x1_px, 100.0);

  // surface 在 sv (popup 父控件) 内的实际偏移
  QPoint surf_pt= sv.surface ()->mapTo (&sv, QPoint (0, 0));
  QCOMPARE (x1_px, 100.0 + surf_pt.x ());

  // 选区应当被正确判定为在视口内，而不是被 300px 侧边栏偏移错误挤出视口
  QVERIFY (popup.selectionInView ());
}

#ifdef QTTEXMACS
QTEST_MAIN (TestQTMBasePopup)
#else
int
main () {
  return 0;
}
#endif
#include "qt_base_popup_test.moc"
