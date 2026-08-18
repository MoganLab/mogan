/******************************************************************************
 * MODULE     : point_test.cpp
 * DESCRIPTION: Unit tests for point (array<double>) operations
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "math_util.hpp"
#include "moe_doctests.hpp"
#include "point.hpp"

static point
mkp (double x, double y) {
  point p (2);
  p[0]= x;
  p[1]= y;
  return p;
}

TEST_CASE ("test equality") {
  CHECK (mkp (1, 2) == mkp (1, 2));
  CHECK (!(mkp (1, 2) == mkp (1, 3)));
}

TEST_CASE ("test arithmetic") {
  CHECK ((mkp (1, 2) + mkp (3, 4)) == mkp (4, 6));
  CHECK ((mkp (3, 4) - mkp (1, 2)) == mkp (2, 2));
  CHECK ((-mkp (1, -2)) == mkp (-1, 2));
}

TEST_CASE ("test scalar ops") {
  CHECK ((2.0 * mkp (1, 2)) == mkp (2, 4));
  CHECK ((mkp (4, 6) / 2.0) == mkp (2, 3));
}

TEST_CASE ("test inner and norm") {
  CHECK_EQ (inner (mkp (1, 2), mkp (3, 4)), 11.0);
  CHECK_EQ (norm (mkp (3, 4)), 5.0);
}

TEST_CASE ("test min/max/abs") {
  CHECK_EQ (min (mkp (3, -1)), -1.0);
  CHECK_EQ (max (mkp (3, -1)), 3.0);
  CHECK (abs (mkp (-3, 2)) == mkp (3, 2));
}

TEST_CASE ("test rotate_2D") {
  point r= rotate_2D (mkp (1, 0), mkp (0, 0), tm_PI / 2);
  CHECK (fabs (r[0]) < 1e-6);
  CHECK (fabs (r[1] - 1.0) < 1e-6);
}

TEST_CASE ("test slanted") { CHECK (slanted (mkp (2, 4), 0.5) == mkp (4, 4)); }

TEST_CASE ("test inside_rectangle") {
  point pmin= mkp (0, 0), pmax= mkp (2, 2);
  // 内部点
  CHECK (inside_rectangle (mkp (1, 1), pmin, pmax));
  // 四条边外侧各排除一个方向
  CHECK (!inside_rectangle (mkp (-1, 1), pmin, pmax));
  CHECK (!inside_rectangle (mkp (3, 1), pmin, pmax));
  CHECK (!inside_rectangle (mkp (1, -1), pmin, pmax));
  CHECK (!inside_rectangle (mkp (1, 3), pmin, pmax));
  // 判定含边界：四角与四边中点均在矩形内
  CHECK (inside_rectangle (mkp (0, 0), pmin, pmax));
  CHECK (inside_rectangle (mkp (2, 2), pmin, pmax));
  CHECK (inside_rectangle (mkp (0, 2), pmin, pmax));
  CHECK (inside_rectangle (mkp (2, 0), pmin, pmax));
  CHECK (inside_rectangle (mkp (1, 0), pmin, pmax));
  CHECK (inside_rectangle (mkp (1, 2), pmin, pmax));
  CHECK (inside_rectangle (mkp (0, 1), pmin, pmax));
  CHECK (inside_rectangle (mkp (2, 1), pmin, pmax));
  // 退化矩形：对角顶点重合为单点，仅该点自身在内
  CHECK (inside_rectangle (mkp (1, 1), mkp (1, 1), mkp (1, 1)));
  CHECK (!inside_rectangle (mkp (1, 1.5), mkp (1, 1), mkp (1, 1)));
  // 退化矩形：零高度线段，x 方向含边界、y 必须等于 1
  CHECK (inside_rectangle (mkp (1.5, 1), mkp (0, 1), mkp (2, 1)));
  CHECK (!inside_rectangle (mkp (1.5, 1.5), mkp (0, 1), mkp (2, 1)));
}

TEST_CASE ("test collinear") {
  CHECK (collinear (mkp (1, 0), mkp (2, 0)));
  CHECK (!collinear (mkp (1, 0), mkp (0, 1)));
}

TEST_CASE ("test tree roundtrip") {
  point p= mkp (1.5, 2.5);
  tree  t= as_tree (p);
  CHECK (is_point (t));
  point q= as_point (t);
  CHECK (q == p);
}

TEST_CASE ("test axis proj/dist") {
  axis a;
  a.p0   = mkp (0, 0);
  a.p1   = mkp (10, 0);
  point p= mkp (3, 4);
  CHECK (proj (a, p) == mkp (3, 0));
  CHECK (fabs (dist (a, p) - 4.0) < 1e-6);
  CHECK (fabs (seg_dist (a, p) - 4.0) < 1e-6);
}

TEST_CASE ("test point-wise mul/div") {
  CHECK ((mkp (2, 3) * mkp (4, 5)) == mkp (8, 15));
  CHECK ((mkp (8, 6) / mkp (2, 3)) == mkp (4, 2));
}

TEST_CASE ("test seg_dist endpoint cases") {
  axis a;
  a.p0= mkp (0, 0);
  a.p1= mkp (10, 0);
  // 垂足落在延长线上时，取到端点的距离
  CHECK (fabs (seg_dist (a, mkp (-3, 4)) - 5.0) < 1e-6);
  CHECK (fabs (seg_dist (a, mkp (13, 4)) - 5.0) < 1e-6);
  // 三参数版本与 axis 版本一致
  CHECK (fabs (seg_dist (mkp (0, 0), mkp (10, 0), mkp (3, 4)) - 4.0) < 1e-6);
}

TEST_CASE ("test proj degenerate axis") {
  axis a;
  a.p0= mkp (1, 1);
  a.p1= mkp (1, 1);
  CHECK (proj (a, mkp (5, 5)) == mkp (1, 1));
}

TEST_CASE ("test linearly_dependent") {
  CHECK (linearly_dependent (mkp (0, 0), mkp (1, 1), mkp (2, 2)));
  CHECK (!linearly_dependent (mkp (0, 0), mkp (1, 0), mkp (0, 1)));
  // 任意两点重合即线性相关
  CHECK (linearly_dependent (mkp (1, 2), mkp (1, 2), mkp (0, 5)));
}

TEST_CASE ("test orthogonalize") {
  point i, j;
  CHECK (orthogonalize (i, j, mkp (0, 0), mkp (2, 0), mkp (0, 3)));
  CHECK (i == mkp (1, 0));
  CHECK (j == mkp (0, 1));
  // 三点共线时失败
  CHECK (!orthogonalize (i, j, mkp (0, 0), mkp (1, 1), mkp (2, 2)));
}

TEST_CASE ("test midperp") {
  axis a= midperp (mkp (0, 0), mkp (2, 0), mkp (0, 1));
  // 中垂线过中点 (1,0) 且竖直
  CHECK (a.p0 == mkp (1, 0));
  CHECK (fabs (a.p1[0] - 1.0) < 1e-6);
  // 三点共线时退化为空点 point(0)
  axis b= midperp (mkp (0, 0), mkp (1, 1), mkp (2, 2));
  CHECK (N (b.p0) == 0);
  CHECK (N (b.p1) == 0);
}

TEST_CASE ("test intersection") {
  axis A, B;
  A.p0= mkp (0, 0);
  A.p1= mkp (4, 0);
  B.p0= mkp (2, -2);
  B.p1= mkp (2, 2);
  CHECK (intersection (A, B) == mkp (2, 0));
  // 平行轴无交点，返回空点 point(0)
  axis C;
  C.p0= mkp (0, 1);
  C.p1= mkp (4, 1);
  CHECK (N (intersection (A, C)) == 0);
}

TEST_CASE ("test arg") {
  CHECK (fabs (arg (mkp (1, 0)) - 0.0) < 1e-6);
  CHECK (fabs (arg (mkp (0, 1)) - tm_PI / 2) < 1e-6);
  CHECK (fabs (arg (mkp (0, -1)) - 3 * tm_PI / 2) < 1e-6);
}
