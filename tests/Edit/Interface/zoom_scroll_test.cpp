
/******************************************************************************
 * MODULE     : zoom_scroll_test.cpp
 * DESCRIPTION: Test zoom scroll computation logic
 * COPYRIGHT  : (C) 2026
 ******************************************************************************/

#include "base.hpp"
#include <QtTest/QtTest>

// Forward declare the function under test (defined in edit_interface.cpp)
extern void compute_zoom_scroll (SI cursor_x, SI cursor_y, SI old_sx, SI old_sy,
                                 double old_magf, double new_magf, SI& new_sx,
                                 SI& new_sy);

class TestZoomScroll : public QObject {
  Q_OBJECT

private slots:
  void test_zoom_in_centered ();
  void test_zoom_out_centered ();
  void test_zoom_in_at_edge ();
  void test_zoom_out_at_edge ();
  void test_no_zoom_change ();
  void test_rounding_precision ();
};

// 光标在视口中央，放大2倍：scroll 应向光标靠拢
void
TestZoomScroll::test_zoom_in_centered () {
  SI     cursor_x= 1000, cursor_y= 1000;
  SI     old_sx= 500, old_sy= 500; // scroll 在光标左上方
  double old_magf= 1.0, new_magf= 2.0;
  SI     new_sx, new_sy;

  compute_zoom_scroll (cursor_x, cursor_y, old_sx, old_sy, old_magf, new_magf,
                       new_sx, new_sy);

  // 光标偏移 = 1000 - 500 = 500，缩放后 = 500 * 1/2 = 250
  // 新 scroll = 1000 - 250 = 750
  QCOMPARE (new_sx, 750);
  QCOMPARE (new_sy, 750);
}

// 光标在视口中央，缩小1/2：scroll 应远离光标
void
TestZoomScroll::test_zoom_out_centered () {
  SI     cursor_x= 1000, cursor_y= 1000;
  SI     old_sx= 500, old_sy= 500;
  double old_magf= 2.0, new_magf= 1.0;
  SI     new_sx, new_sy;

  compute_zoom_scroll (cursor_x, cursor_y, old_sx, old_sy, old_magf, new_magf,
                       new_sx, new_sy);

  // 光标偏移 = 1000 - 500 = 500，缩放后 = 500 * 2/1 = 1000
  // 新 scroll = 1000 - 1000 = 0
  QCOMPARE (new_sx, 0);
  QCOMPARE (new_sy, 0);
}

// 光标在视口左边缘，放大2倍：光标应仍在边缘
void
TestZoomScroll::test_zoom_in_at_edge () {
  SI     cursor_x= 500, cursor_y= 500;
  SI     old_sx= 500, old_sy= 500; // scroll 正好在光标处（光标在视口左边缘）
  double old_magf= 1.0, new_magf= 2.0;
  SI     new_sx, new_sy;

  compute_zoom_scroll (cursor_x, cursor_y, old_sx, old_sy, old_magf, new_magf,
                       new_sx, new_sy);

  // 光标偏移 = 0，缩放后仍为 0
  // 新 scroll = 500
  QCOMPARE (new_sx, 500);
  QCOMPARE (new_sy, 500);
}

// 光标在视口右边缘，缩小1/2
void
TestZoomScroll::test_zoom_out_at_edge () {
  SI     cursor_x= 1000, cursor_y= 1000;
  SI     old_sx= 1000, old_sy= 1000; // scroll 正好在光标处（光标在视口左边缘）
  double old_magf= 2.0, new_magf= 1.0;
  SI     new_sx, new_sy;

  compute_zoom_scroll (cursor_x, cursor_y, old_sx, old_sy, old_magf, new_magf,
                       new_sx, new_sy);

  // 光标偏移 = 0，缩放后仍为 0
  QCOMPARE (new_sx, 1000);
  QCOMPARE (new_sy, 1000);
}

// zoom 不变时，scroll 应保持不变
void
TestZoomScroll::test_no_zoom_change () {
  SI     cursor_x= 1000, cursor_y= 1000;
  SI     old_sx= 300, old_sy= 400;
  double old_magf= 1.5, new_magf= 1.5;
  SI     new_sx, new_sy;

  compute_zoom_scroll (cursor_x, cursor_y, old_sx, old_sy, old_magf, new_magf,
                       new_sx, new_sy);

  QCOMPARE (new_sx, old_sx);
  QCOMPARE (new_sy, old_sy);
}

// 验证整数舍入精度
void
TestZoomScroll::test_rounding_precision () {
  SI     cursor_x= 1000, cursor_y= 1000;
  SI     old_sx= 333, old_sy= 333;
  double old_magf= 1.0, new_magf= 3.0;
  SI     new_sx, new_sy;

  compute_zoom_scroll (cursor_x, cursor_y, old_sx, old_sy, old_magf, new_magf,
                       new_sx, new_sy);

  // 光标偏移 = 1000 - 333 = 667，缩放后 = 667 * 1/3 = 222.333...
  // tm_round(222.333...) = 222
  // 新 scroll = 1000 - 222 = 778
  QCOMPARE (new_sx, 778);
  QCOMPARE (new_sy, 778);
}

QTEST_MAIN (TestZoomScroll)
#include "zoom_scroll_test.moc"
