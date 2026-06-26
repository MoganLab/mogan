/******************************************************************************
 * MODULE     : point_test.cpp
 * DESCRIPTION: Unit tests for point (array<double>) operations
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "math_util.hpp"
#include "point.hpp"
#include <QtTest/QtTest>

class TestPoint : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void cleanup () { cleanup_qt_top_level_widgets (); }
  void test_equality ();
  void test_arithmetic ();
  void test_scalar_ops ();
  void test_inner_and_norm ();
  void test_min_max_abs ();
  void test_rotate_2D ();
  void test_slanted ();
  void test_inside_rectangle ();
  void test_collinear ();
  void test_tree_roundtrip ();
  void test_axis_proj_dist ();
};

static point
mkp (double x, double y) {
  point p (2);
  p[0]= x;
  p[1]= y;
  return p;
}

void
TestPoint::test_equality () {
  QVERIFY (mkp (1, 2) == mkp (1, 2));
  QVERIFY (!(mkp (1, 2) == mkp (1, 3)));
}

void
TestPoint::test_arithmetic () {
  QVERIFY ((mkp (1, 2) + mkp (3, 4)) == mkp (4, 6));
  QVERIFY ((mkp (3, 4) - mkp (1, 2)) == mkp (2, 2));
  QVERIFY ((-mkp (1, -2)) == mkp (-1, 2));
}

void
TestPoint::test_scalar_ops () {
  QVERIFY ((2.0 * mkp (1, 2)) == mkp (2, 4));
  QVERIFY ((mkp (4, 6) / 2.0) == mkp (2, 3));
}

void
TestPoint::test_inner_and_norm () {
  QCOMPARE (inner (mkp (1, 2), mkp (3, 4)), 11.0);
  QCOMPARE (norm (mkp (3, 4)), 5.0);
}

void
TestPoint::test_min_max_abs () {
  QCOMPARE (min (mkp (3, -1)), -1.0);
  QCOMPARE (max (mkp (3, -1)), 3.0);
  QVERIFY (abs (mkp (-3, 2)) == mkp (3, 2));
}

void
TestPoint::test_rotate_2D () {
  point r= rotate_2D (mkp (1, 0), mkp (0, 0), tm_PI / 2);
  QVERIFY (fabs (r[0]) < 1e-6);
  QVERIFY (fabs (r[1] - 1.0) < 1e-6);
}

void
TestPoint::test_slanted () {
  QVERIFY (slanted (mkp (2, 4), 0.5) == mkp (4, 4));
}

void
TestPoint::test_inside_rectangle () {
  QVERIFY (inside_rectangle (mkp (1, 1), mkp (0, 0), mkp (2, 2)));
  QVERIFY (!inside_rectangle (mkp (3, 1), mkp (0, 0), mkp (2, 2)));
}

void
TestPoint::test_collinear () {
  QVERIFY (collinear (mkp (1, 0), mkp (2, 0)));
  QVERIFY (!collinear (mkp (1, 0), mkp (0, 1)));
}

void
TestPoint::test_tree_roundtrip () {
  point p= mkp (1.5, 2.5);
  tree  t= as_tree (p);
  QVERIFY (is_point (t));
  point q= as_point (t);
  QVERIFY (q == p);
}

void
TestPoint::test_axis_proj_dist () {
  axis a;
  a.p0   = mkp (0, 0);
  a.p1   = mkp (10, 0);
  point p= mkp (3, 4);
  QVERIFY (proj (a, p) == mkp (3, 0));
  QVERIFY (fabs (dist (a, p) - 4.0) < 1e-6);
  QVERIFY (fabs (seg_dist (a, p) - 4.0) < 1e-6);
}

QTEST_APPLESS_MAIN (TestPoint)
#include "point_test.moc"
