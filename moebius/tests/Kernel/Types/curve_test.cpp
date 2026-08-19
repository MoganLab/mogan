
/******************************************************************************
 * MODULE     : curve_test.cpp
 * DESCRIPTION: Unit tests for curve operations
 * COPYRIGHT  : (C) 2026  PinkMagicFly
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "curve.hpp"
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

TEST_CASE ("segment bound: 直线段 delta 恰为 eps/|grad|") {
  // 从 (0,0) 到 (3,4)，|c'| = 5，位移随参数线性变化
  curve c= segment (mkp (0, 0), mkp (3, 4));
  CHECK (fabs (c->bound (0.5, 0.1) - 0.02) < 1e-12);
  // 端点处 t∓delta 被钳制到参数域 [0,1]，不越界求值
  CHECK (fabs (c->bound (0.0, 0.1) - 0.02) < 1e-12);
  CHECK (fabs (c->bound (1.0, 0.1) - 0.02) < 1e-12);
}

TEST_CASE ("segment bound: 三维点同样适用") {
  point p1 (3), p2 (3);
  p1[0]  = 0;
  p1[1]  = 0;
  p1[2]  = 0;
  p2[0]  = 1;
  p2[1]  = 2;
  p2[2]  = 2;
  curve c= segment (p1, p2); // |c'| = 3
  CHECK (fabs (c->bound (0.5, 0.3) - 0.1) < 1e-12);
}

TEST_CASE ("segment bound: 零梯度（退化线段）返回 tm_infinity") {
  curve c= segment (mkp (1, 1), mkp (1, 1));
  CHECK (c->bound (0.5, 0.1) == tm_infinity);
}

TEST_CASE ("poly_segment bound: 梯度低估位移时二分收缩") {
  // 拐点前平缓、拐点后陡峭：t=0.45 处 |c'|=20，但 t+delta 跨入
  // 陡峭段后位移远超 eps，delta 需两次减半（0.15 -> 0.075 -> 0.0375）
  array<point> a;
  a << mkp (0, 0) << mkp (10, 0) << mkp (10, 100);
  curve c= poly_segment (a, array<path> ());
  CHECK (fabs (c->bound (0.45, 3.0) - 0.0375) < 1e-12);
}

TEST_CASE ("bound 契约: |t'-t|<=delta 时 |c(t')-c(t)|<=eps") {
  array<point> a;
  a << mkp (0, 0) << mkp (10, 0) << mkp (10, 100);
  curve curves[2];
  curves[0] = segment (mkp (0, 0), mkp (3, 4));
  curves[1] = poly_segment (a, array<path> ());
  double eps= 0.5;
  double lim= (eps + 1e-6) * (eps + 1e-6);
  for (int k= 0; k < 2; k++) {
    for (int i= 1; i < 10; i++) {
      double t    = i / 10.0;
      double delta= curves[k]->bound (t, eps);
      point  v1   = curves[k]->evaluate (t);
      point  v2   = curves[k]->evaluate (max (t - delta, 0.0));
      point  v3   = curves[k]->evaluate (min (t + delta, 1.0));
      CHECK (norm2_diff (v2, v1) <= lim);
      CHECK (norm2_diff (v3, v1) <= lim);
    }
  }
}
