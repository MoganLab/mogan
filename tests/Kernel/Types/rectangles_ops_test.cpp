/******************************************************************************
 * MODULE     : rectangles_ops_test.cpp
 * DESCRIPTION: Unit tests for rectangle/rectangles operations
 *              (complement to rectangles_test.cpp which covers disjoint_union)
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "rectangles.hpp"
#include <QtTest/QtTest>

class TestRectanglesOps : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }
  void test_equality ();
  void test_area ();
  void test_is_zero ();
  void test_intersect ();
  void test_subset ();
  void test_translate ();
  void test_thicken ();
  void test_scaling ();
  void test_lub_two ();
  void test_lub_list ();
  void test_area_list ();
  void test_union_and_difference ();
  void test_correct_drops_degenerate ();
};

void
TestRectanglesOps::test_equality () {
  rectangle a (0, 0, 10, 10);
  rectangle b (0, 0, 10, 10);
  rectangle c (1, 1, 10, 10);
  QVERIFY (a == b);
  QVERIFY (a != c);
}

void
TestRectanglesOps::test_area () {
  QCOMPARE (area (rectangle (0, 0, 4, 5)), 20.0);
  QCOMPARE (area (rectangle (5, 5, 5, 5)), 0.0);
}

void
TestRectanglesOps::test_is_zero () {
  QVERIFY (is_zero (rectangle (0, 0, 0, 0)));
  QVERIFY (!is_zero (rectangle (1, 0, 0, 0)));
}

void
TestRectanglesOps::test_intersect () {
  rectangle r1 (0, 0, 10, 10);
  rectangle r2 (5, 5, 15, 15);
  rectangle r3 (20, 20, 30, 30);
  QVERIFY (intersect (r1, r2));
  QVERIFY (!intersect (r1, r3));
}

void
TestRectanglesOps::test_subset () {
  rectangle r1 (0, 0, 10, 10);
  rectangle small (2, 2, 8, 8);
  rectangle r2 (5, 5, 15, 15);
  QVERIFY (small <= r1);
  QVERIFY (!(r2 <= r1));
}

void
TestRectanglesOps::test_translate () {
  rectangle r (0, 0, 10, 10);
  QVERIFY (translate (r, 5, 7) == rectangle (5, 7, 15, 17));
}

void
TestRectanglesOps::test_thicken () {
  rectangle r (0, 0, 10, 10);
  QVERIFY (thicken (r, 2, 3) == rectangle (-2, -3, 12, 13));
}

void
TestRectanglesOps::test_scaling () {
  rectangle r (1, 1, 4, 4);
  QVERIFY ((r * 3) == rectangle (3, 3, 12, 12));
  QVERIFY ((r / 2) == rectangle (0, 0, 2, 2));
}

void
TestRectanglesOps::test_lub_two () {
  rectangle r1 (0, 0, 5, 5);
  rectangle r2 (3, 3, 10, 10);
  QVERIFY (least_upper_bound (r1, r2) == rectangle (0, 0, 10, 10));
}

void
TestRectanglesOps::test_lub_list () {
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                rectangles (rectangle (5, 5, 8, 8), rectangles ()));
  QVERIFY (least_upper_bound (l) == rectangle (0, 0, 8, 8));
}

void
TestRectanglesOps::test_area_list () {
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                rectangles (rectangle (0, 0, 3, 3), rectangles ()));
  QCOMPARE (area (l), 13.0);
}

void
TestRectanglesOps::test_union_and_difference () {
  rectangle  r1 (0, 0, 4, 4);
  rectangle  r2 (2, 0, 6, 4);
  rectangles l1= rectangles (r1, rectangles ());
  rectangles l2= rectangles (r2, rectangles ());
  rectangles u = l1 | l2;
  QVERIFY (least_upper_bound (u) == rectangle (0, 0, 6, 4));
  rectangles d= l1 - l2;
  QCOMPARE (area (d), 8.0);
}

void
TestRectanglesOps::test_correct_drops_degenerate () {
  rectangles l= rectangles (rectangle (5, 5, 5, 5),
                rectangles (rectangle (0, 0, 2, 2), rectangles ()));
  rectangles c= correct (l);
  QCOMPARE (N (c), 1);
}

QTEST_APPLESS_MAIN (TestRectanglesOps)
#include "rectangles_ops_test.moc"
