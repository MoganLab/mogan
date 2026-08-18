/******************************************************************************
 * MODULE     : midpoint_snap_test.cpp
 * DESCRIPTION: Unit tests for register_midpoint (dedup + snap candidate
 *              registration of curve edge midpoints)
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Interface/edit_graphics.hpp"
#include "base.hpp"
#include <QtTest/QtTest>

static point
mkp (double x, double y) {
  point p (2);
  p[0]= x;
  p[1]= y;
  return p;
}

class TestRegisterMidpoint : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  // 鼠标与中点距离小于吸附距离：加入显示集合并追加 curve-mid-point 候选
  void test_candidate_appended_when_close ();
  // 距离不小于吸附距离：只加入显示集合，不追加候选（避免长边远端被强拉）
  void test_no_candidate_when_far ();
  // 相同坐标字符串重复注册：显示集合不增长、不重复追加候选
  void test_dedup_by_coordinate_string ();
};

void
TestRegisterMidpoint::test_candidate_appended_when_close () {
  tree          points (TUPLE);
  gr_selections sels;
  array<path>   cp;
  cp << path (1, 2);
  array<point> pts;
  pts << mkp (1, 1);
  register_midpoint (mkp (0, 0), 10.0, mkp (3, 4), "1.5", "2.5", points, sels,
                     cp, pts);
  QCOMPARE (N (points), 1);
  QVERIFY (points[0][0] == "1.5");
  QVERIFY (points[0][1] == "2.5");
  QCOMPARE (N (sels), 1);
  QVERIFY (sels[0]->type == "curve-mid-point");
  QVERIFY (N (sels[0]->p) == 2 && sels[0]->p[0] == 3 && sels[0]->p[1] == 4);
  QCOMPARE (sels[0]->dist, (SI) 5);
  QCOMPARE (N (sels[0]->cp), 1);
  QCOMPARE (N (sels[0]->pts), 1);
}

void
TestRegisterMidpoint::test_no_candidate_when_far () {
  tree          points (TUPLE);
  gr_selections sels;
  register_midpoint (mkp (0, 0), 10.0, mkp (30, 40), "15", "20", points, sels,
                     array<path> (), array<point> ());
  QCOMPARE (N (points), 1);
  QCOMPARE (N (sels), 0);
}

void
TestRegisterMidpoint::test_dedup_by_coordinate_string () {
  tree          points (TUPLE);
  gr_selections sels;
  register_midpoint (mkp (0, 0), 10.0, mkp (3, 4), "1.5", "2.5", points, sels,
                     array<path> (), array<point> ());
  register_midpoint (mkp (0, 0), 10.0, mkp (3, 4), "1.5", "2.5", points, sels,
                     array<path> (), array<point> ());
  QCOMPARE (N (points), 1);
  QCOMPARE (N (sels), 1);
  // 坐标字符串不同则视为不同中点
  register_midpoint (mkp (0, 0), 10.0, mkp (3, 4), "1.50", "2.5", points, sels,
                     array<path> (), array<point> ());
  QCOMPARE (N (points), 2);
  QCOMPARE (N (sels), 2);
}

QTEST_MAIN (TestRegisterMidpoint)
#include "midpoint_snap_test.moc"
