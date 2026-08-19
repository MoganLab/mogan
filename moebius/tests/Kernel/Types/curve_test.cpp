
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

TEST_CASE ("segment evaluate 端点与中点") {
  curve c = segment (mkp (0, 0), mkp (3, 4));
  point e0= c->evaluate (0.0);
  point e1= c->evaluate (1.0);
  point em= c->evaluate (0.5);
  CHECK (e0 == mkp (0, 0));
  CHECK (e1 == mkp (3, 4));
  CHECK (em == mkp (1.5, 2.0));
  // 三维点插值维度保持
  point q0 (3), q1 (3);
  q0[0]   = 0;
  q0[1]   = 0;
  q0[2]   = 0;
  q1[0]   = 3;
  q1[1]   = 4;
  q1[2]   = 5;
  curve c3= segment (q0, q1);
  point m3= c3->evaluate (0.5);
  CHECK_EQ (N (m3), 3);
  CHECK (fabs (m3[0] - 1.5) < 1e-9);
  CHECK (fabs (m3[1] - 2.0) < 1e-9);
  CHECK (fabs (m3[2] - 2.5) < 1e-9);
}

TEST_CASE ("poly_segment evaluate 分段边界") {
  array<point> a;
  a << mkp (0, 0) << mkp (10, 0) << mkp (10, 100);
  curve c= poly_segment (a, array<path> ());
  // n=2,每段占 t 的一半
  CHECK (c->evaluate (0.0) == mkp (0, 0));
  CHECK (c->evaluate (0.25) == mkp (5, 0));
  CHECK (c->evaluate (0.5) == mkp (10, 0));
  CHECK (c->evaluate (0.75) == mkp (10, 50));
  CHECK (c->evaluate (1.0) == mkp (10, 100));
}

TEST_CASE ("poly_segment grad 方向与倍率") {
  array<point> a;
  a << mkp (0, 0) << mkp (10, 0) << mkp (10, 100);
  curve c  = poly_segment (a, array<path> ());
  bool  err= true;
  point g  = c->grad (0.5, err);
  CHECK (!err);
  // n=2,第二段方向 (0,100),grad = 2*(0,100)
  CHECK (g == mkp (0, 200));
}

TEST_CASE ("spline evaluate 端点与缓存一致性") {
  array<point> a;
  a << mkp (0, 0) << mkp (1, 3) << mkp (3, 2) << mkp (5, 5) << mkp (7, 1);
  curve c= spline (a, array<path> (), false, true);
  // 端点插值:开样条经过首末控制点
  CHECK (c->evaluate (0.0) == mkp (0, 0));
  CHECK (c->evaluate (1.0) == mkp (7, 1));
  // 同一 t 反复求值(interval_no 缓存命中路径)结果一致
  point p1= c->evaluate (0.37);
  point p2= c->evaluate (0.37);
  point p3= c->evaluate (0.37);
  CHECK (p1 == p2);
  CHECK (p1 == p3);
  // t 跳跃后回到原区间,结果仍一致(缓存失效重扫路径)
  point p4= c->evaluate (0.9);
  point p5= c->evaluate (0.37);
  CHECK (p1 == p5);
  CHECK (!(p1 == p4));
}

TEST_CASE ("spline rectify 首末点") {
  array<point> a;
  a << mkp (0, 0) << mkp (1, 3) << mkp (3, 2) << mkp (5, 5) << mkp (7, 1);
  curve        c = spline (a, array<path> (), false, true);
  array<point> ps= c->rectify (0.05);
  CHECK (N (ps) >= 2);
  CHECK (ps[0] == mkp (0, 0));
  CHECK (ps[N (ps) - 1] == mkp (7, 1));
}

TEST_CASE ("spline grad 与 bound 契约") {
  array<point> a;
  a << mkp (0, 0) << mkp (1, 3) << mkp (3, 2) << mkp (5, 5) << mkp (7, 1);
  curve c  = spline (a, array<path> (), false, true);
  bool  err= true;
  point g  = c->grad (0.5, err);
  CHECK (!err);
  CHECK_EQ (N (g), 2);
  double eps  = 0.5;
  double delta= c->bound (0.5, eps);
  point  v1   = c->evaluate (0.5);
  point  v2   = c->evaluate (max (0.5 - delta, 0.0));
  CHECK (norm2_diff (v2, v1) <= (eps + 1e-6) * (eps + 1e-6));
}

TEST_CASE ("find_closest_point on segment") {
  curve c  = segment (mkp (0, 0), mkp (10, 0));
  bool  err= true;
  // 查询点取在曲线上,内点即为精确最近点
  double t= c->find_closest_point (0.0, 1.0, mkp (4, 0), 0.01, err);
  CHECK (err);
  point q= c->evaluate (t);
  CHECK (fabs (q[0] - 4.0) < 0.1);
  CHECK (fabs (q[1]) < 1e-9);
}

TEST_CASE ("find_closest_point on poly_segment") {
  array<point> a;
  a << mkp (0, 0) << mkp (10, 0) << mkp (10, 100);
  curve c  = poly_segment (a, array<path> ());
  bool  err= true;
  // 距离第二段更近的查询点
  double t= c->find_closest_point (0.0, 1.0, mkp (9, 60), 0.01, err);
  CHECK (err);
  point q= c->evaluate (t);
  CHECK (fabs (q[0] - 10.0) < 0.5);
  CHECK (fabs (q[1] - 60.0) < 1.0);
}

TEST_CASE ("closest returns near-minimum distance") {
  array<point> a;
  a << mkp (0, 0) << mkp (10, 0) << mkp (10, 100);
  curve c= poly_segment (a, array<path> ());
  // 查询点取在曲线上,最近距离应为 0
  point  q= closest (c, mkp (5, 0));
  double d= sqrt (norm2_diff (q, mkp (5, 0)));
  CHECK (d < 0.1);
}

TEST_CASE ("intersection of crossing segments") {
  // (curve,curve,double&,double&) 未导出到 hpp,补声明
  bool   intersection (curve f, curve g, double& t, double& u);
  curve  f= segment (mkp (0, 0), mkp (10, 10));
  curve  g= segment (mkp (0, 10), mkp (10, 0));
  double t= 0.2, u= 0.2;
  bool   ok= intersection (f, g, t, u);
  CHECK (ok);
  point pf= f->evaluate (t);
  CHECK (fabs (pf[0] - 5.0) < 1e-6);
  CHECK (fabs (pf[1] - 5.0) < 1e-6);
}
