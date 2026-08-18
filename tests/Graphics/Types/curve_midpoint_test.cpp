/******************************************************************************
 * MODULE     : curve_midpoint_test.cpp
 * DESCRIPTION: Unit tests for straight_edge_midpoints (per-edge midpoints of
 *              a curve's straight edges near a reference point)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "curve.hpp"
#include "point.hpp"
#include <QtTest/QtTest>

static point
mkp (double x, double y) {
  point p (2);
  p[0]= x;
  p[1]= y;
  return p;
}

static bool
pt_eq (point a, double x, double y, double eps= 1e-9) {
  return N (a) == 2 && fabs (a[0] - x) < eps && fabs (a[1] - y) < eps;
}

class TestStraightEdgeMidpoints : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // nil 曲线与无控制点曲线不崩溃、返回空
  void test_nil_curve ();
  // 单线段：参考点贴近时返回唯一中点
  void test_segment_hit ();
  // 单线段：参考点距离超过容差时返回空
  void test_segment_miss ();
  // 开放折线：逐边独立判定，只返回贴近边的中点
  void test_polyline_per_edge ();
  // 顶点处两条边都贴近时返回两个中点，顺序与控制点区间一致
  void test_polyline_vertex_both_edges ();
  // cline 形式的闭合折线（首尾控制点重复）：闭合边正常取中点
  void test_closed_polyline_closing_edge ();
  // 退化边（两端点重合）跳过，不影响其余边
  void test_degenerate_edge_skipped ();

  // is_polyline_or_segment：线段、折线（含被坐标变换包裹）为真，
  // 圆弧、样条、nil 为假
  void test_is_polyline_or_segment ();
};

void
TestStraightEdgeMidpoints::test_nil_curve () {
  curve c;
  QVERIFY (N (straight_edge_midpoints (c, mkp (0, 0), 1.0)) == 0);
}

void
TestStraightEdgeMidpoints::test_segment_hit () {
  curve        c   = segment (mkp (0, 0), mkp (4, 0));
  array<point> mids= straight_edge_midpoints (c, mkp (2, 0.4), 0.5);
  QCOMPARE (N (mids), 1);
  QVERIFY (pt_eq (mids[0], 2, 0));
}

void
TestStraightEdgeMidpoints::test_segment_miss () {
  curve c= segment (mkp (0, 0), mkp (4, 0));
  QVERIFY (N (straight_edge_midpoints (c, mkp (2, 5), 0.5)) == 0);
}

void
TestStraightEdgeMidpoints::test_polyline_per_edge () {
  array<point> a;
  a << mkp (0, 0) << mkp (4, 0) << mkp (4, 4);
  curve        c = poly_segment (a, array<path> ());
  array<point> m1= straight_edge_midpoints (c, mkp (2, 0.3), 0.5);
  QCOMPARE (N (m1), 1);
  QVERIFY (pt_eq (m1[0], 2, 0));
  array<point> m2= straight_edge_midpoints (c, mkp (3.7, 2), 0.5);
  QCOMPARE (N (m2), 1);
  QVERIFY (pt_eq (m2[0], 4, 2));
}

void
TestStraightEdgeMidpoints::test_polyline_vertex_both_edges () {
  array<point> a;
  a << mkp (0, 0) << mkp (4, 0) << mkp (4, 4);
  curve        c   = poly_segment (a, array<path> ());
  array<point> mids= straight_edge_midpoints (c, mkp (4, 0), 0.5);
  QCOMPARE (N (mids), 2);
  QVERIFY (pt_eq (mids[0], 2, 0));
  QVERIFY (pt_eq (mids[1], 4, 2));
}

void
TestStraightEdgeMidpoints::test_closed_polyline_closing_edge () {
  // 文档中 <cline> 的构造方式：首尾控制点重复（concat_graphics.cpp）
  array<point> a;
  a << mkp (0, 0) << mkp (4, 0) << mkp (2, 4) << mkp (0, 0);
  curve        c   = poly_segment (a, array<path> ());
  array<point> mids= straight_edge_midpoints (c, mkp (1, 2), 0.3);
  QCOMPARE (N (mids), 1);
  QVERIFY (pt_eq (mids[0], 1, 2));
}

void
TestStraightEdgeMidpoints::test_degenerate_edge_skipped () {
  array<point> a;
  a << mkp (0, 0) << mkp (0, 0) << mkp (4, 0);
  curve        c   = poly_segment (a, array<path> ());
  array<point> mids= straight_edge_midpoints (c, mkp (0, 0), 0.5);
  QCOMPARE (N (mids), 1);
  QVERIFY (pt_eq (mids[0], 2, 0));
}

void
TestStraightEdgeMidpoints::test_is_polyline_or_segment () {
  curve nil_c;
  QVERIFY (!is_polyline_or_segment (nil_c));
  QVERIFY (is_polyline_or_segment (segment (mkp (0, 0), mkp (4, 0))));
  array<point> a;
  a << mkp (0, 0) << mkp (4, 0) << mkp (4, 4);
  curve poly= poly_segment (a, array<path> ());
  QVERIFY (is_polyline_or_segment (poly));
  // curve_box_rep::transform 用 fr(c) 包裹曲线
  frame fr= shift_2D (mkp (1, 1));
  QVERIFY (is_polyline_or_segment (fr (poly)));
  QVERIFY (is_polyline_or_segment (fr (segment (mkp (0, 0), mkp (4, 0)))));
  array<point> arc_pts;
  arc_pts << mkp (-1, -1) << mkp (1, -1) << mkp (0, 0);
  QVERIFY (!is_polyline_or_segment (parabola (arc_pts, array<path> (), false)));
  QVERIFY (!is_polyline_or_segment (arc (arc_pts, array<path> (), false)));
  QVERIFY (!is_polyline_or_segment (spline (a, array<path> ())));
}

QTEST_MAIN (TestStraightEdgeMidpoints)
#include "curve_midpoint_test.moc"
