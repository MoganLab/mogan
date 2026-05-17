
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

static void
assert_zoom_scroll (SI cursor_x, SI cursor_y, SI old_sx, SI old_sy,
                    double old_magf, double new_magf, SI expected_sx,
                    SI expected_sy) {
  SI new_sx, new_sy;
  compute_zoom_scroll (cursor_x, cursor_y, old_sx, old_sy, old_magf, new_magf,
                       new_sx, new_sy);
  QCOMPARE (new_sx, expected_sx);
  QCOMPARE (new_sy, expected_sy);
}

void
TestZoomScroll::test_zoom_in_centered () {
  assert_zoom_scroll (1000, 1000, 500, 500, 1.0, 2.0, 750, 750);
}

void
TestZoomScroll::test_zoom_out_centered () {
  assert_zoom_scroll (1000, 1000, 500, 500, 2.0, 1.0, 0, 0);
}

void
TestZoomScroll::test_zoom_in_at_edge () {
  assert_zoom_scroll (500, 500, 500, 500, 1.0, 2.0, 500, 500);
}

void
TestZoomScroll::test_zoom_out_at_edge () {
  assert_zoom_scroll (1000, 1000, 1000, 1000, 2.0, 1.0, 1000, 1000);
}

void
TestZoomScroll::test_no_zoom_change () {
  assert_zoom_scroll (1000, 1000, 300, 400, 1.5, 1.5, 300, 400);
}

void
TestZoomScroll::test_rounding_precision () {
  assert_zoom_scroll (1000, 1000, 333, 333, 1.0, 3.0, 778, 778);
}

QTEST_MAIN (TestZoomScroll)
#include "zoom_scroll_test.moc"
