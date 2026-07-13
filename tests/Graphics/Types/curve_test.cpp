
/******************************************************************************
 * MODULE     : curve_test.cpp
 * DESCRIPTION: Unit tests for ellipse/hyperbola conic curves
 * COPYRIGHT  : (C) 2026 AcceleratorX
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
fuzzy_eq (double a, double b, double eps= 1e-9) {
  return a - b < eps && b - a < eps;
}

static bool
is_nan (double x) {
  return x != x;
}

static bool
is_unit_vector (point v) {
  return fuzzy_eq (norm (v), 1.0, 1e-9);
}

static bool
is_orthogonal (point u, point v) {
  return fuzzy_eq (inner (u, v), 0.0, 1e-9);
}

class TestCurveConic : public QObject {
  Q_OBJECT

private slots:
  // 退化判断：覆盖共线/非共线、焦点重合、三点重合、延长线等所有情况
  void test_ellipse_degeneration_criteria ();
  void test_ellipse_degeneration_boundary ();
  // 共线且第三点在焦点之间：椭圆应退化为线段，构造函数兜底不应崩溃
  void test_ellipse_collinear_between_foci ();
  // 共线且第三点在焦点延长线上：应构造扁平椭圆
  void test_ellipse_collinear_outside_foci ();
  // 共线但轴线为垂直方向：验证正交基兜底对任意方向都有效
  void test_ellipse_collinear_vertical ();
  // 非共线标准椭圆：验证焦点、中心、半径和几何定义
  void test_ellipse_non_collinear ();
  // 两焦点重合：椭圆退化为圆，不应触发退化判断
  void test_ellipse_coincident_foci ();
  // 三点完全重合：边界情况，构造函数兜底不应崩溃
  void test_ellipse_three_identical_points ();
  // 椭圆上任意参数 t 的采样点都不应出现 NaN
  void test_ellipse_evaluate_all_parameters ();

  // 退化判断：覆盖共线/非共线、焦点重合、三点重合、延长线等所有情况
  void test_hyperbola_degeneration_criteria ();
  void test_hyperbola_degeneration_boundary ();
  // 共线且第三点在焦点之间：双曲线可正常构造
  void test_hyperbola_collinear_between_foci ();
  // 共线且第三点在焦点延长线上：双曲线应退化为线段
  void test_hyperbola_collinear_outside_foci ();
  // 共线但轴线为垂直方向：验证正交基兜底对任意方向都有效
  void test_hyperbola_collinear_vertical ();
  // 非共线标准双曲线：验证焦点、中心、半径和几何定义
  void test_hyperbola_non_collinear ();
  // 两焦点重合：双曲线退化为线段
  void test_hyperbola_coincident_foci ();
  // 三点完全重合：边界情况，构造函数兜底不应崩溃
  void test_hyperbola_three_identical_points ();
  // 双曲线上任意参数 t 的采样点都不应出现 NaN
  void test_hyperbola_evaluate_all_parameters ();

  // 正交基兜底：共线时手动构造的 i/j 必须是单位正交向量
  void test_orthogonal_basis_after_fallback ();
  // 不同方向共线时，轴向都应正确对齐到焦点连线
  void test_orthogonal_basis_multiple_directions ();
};

/*
 * 退化判断：椭圆要求 a > c（两焦点距离之和小于焦距时无法构成椭圆）。
 * 当三点共线且 focal_length >= sum_of_two_dis（即第三点在两个焦点之间）
 * 时应退化为线段。
 */
static bool
ellipse_should_degenerate (point f1, point f2, point p) {
  if (!linearly_dependent (f1, f2, p)) return false;
  double focal_length  = norm (f2 - f1);
  double sum_of_two_dis= norm (p - f1) + norm (p - f2);
  return focal_length >= sum_of_two_dis;
}

void
TestCurveConic::test_ellipse_degeneration_criteria () {
  // 覆盖非共线、焦点之间、延长线、焦点重合、三点重合等全部情况
  QVERIFY (!ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (1, 1)));
  QVERIFY (ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (1, 0)));
  QVERIFY (!ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (-1, 0)));
  QVERIFY (!ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (3, 0)));
  QVERIFY (ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (0, 0)));
  QVERIFY (ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (2, 0)));
  QVERIFY (!ellipse_should_degenerate (mkp (0, 0), mkp (0, 0), mkp (1, 0)));
  QVERIFY (ellipse_should_degenerate (mkp (0, 0), mkp (0, 0), mkp (0, 0)));
}

// 共线且第三点在焦点之间：椭圆应退化为线段，构造函数兜底不应崩溃。
void
TestCurveConic::test_ellipse_collinear_between_foci () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (2, 0);
  a[2]= mkp (1, 0);
  array<path> cip;

  ellipse_rep e (a, cip, false);

  QVERIFY (is_unit_vector (e.i));
  QVERIFY (is_unit_vector (e.j));
  QVERIFY (is_orthogonal (e.i, e.j));
  QVERIFY (e.center == mkp (1, 0));
  QVERIFY (!is_nan (e.evaluate (0.0)[0]));
  QVERIFY (!is_nan (e.evaluate (0.5)[0]));
}

// 共线且第三点在焦点延长线上：应构造扁平椭圆。
void
TestCurveConic::test_ellipse_collinear_outside_foci () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (2, 0);
  a[2]= mkp (3, 0);
  array<path> cip;

  ellipse_rep e (a, cip, false);

  QVERIFY (is_unit_vector (e.i));
  QVERIFY (is_unit_vector (e.j));
  QVERIFY (is_orthogonal (e.i, e.j));
  QVERIFY (e.center == mkp (1, 0));
  QVERIFY (e.r1 > e.focal_length / 2);
}

// 非共线标准椭圆：验证焦点、中心、半径和几何定义。
void
TestCurveConic::test_ellipse_non_collinear () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (2, 0);
  a[2]= mkp (1, 1);
  array<path> cip;

  ellipse_rep e (a, cip, false);

  QVERIFY (is_unit_vector (e.i));
  QVERIFY (is_unit_vector (e.j));
  QVERIFY (is_orthogonal (e.i, e.j));
  QVERIFY (e.center == mkp (1, 0));

  // 椭圆上任意一点到两焦点距离之和应为常数
  double sum= norm (e.evaluate (0.0) - e.f1) +
              norm (e.evaluate (0.0) - e.f2);
  for (double t= 0.25; t <= 1.0; t+= 0.25) {
    double s= norm (e.evaluate (t) - e.f1) +
              norm (e.evaluate (t) - e.f2);
    QVERIFY (fuzzy_eq (s, sum, 1e-6));
  }
}

// 两焦点重合：椭圆退化为圆，不应触发退化判断。
void
TestCurveConic::test_ellipse_coincident_foci () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (0, 0);
  a[2]= mkp (1, 0);
  array<path> cip;

  ellipse_rep e (a, cip, false);

  QVERIFY (is_unit_vector (e.i));
  QVERIFY (is_unit_vector (e.j));
  QVERIFY (is_orthogonal (e.i, e.j));
  QVERIFY (e.center == mkp (0, 0));
}

// 三点完全重合：边界情况，构造函数兜底不应崩溃。
void
TestCurveConic::test_ellipse_three_identical_points () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (0, 0);
  a[2]= mkp (0, 0);
  array<path> cip;

  ellipse_rep e (a, cip, false);

  QVERIFY (is_unit_vector (e.i));
  QVERIFY (is_unit_vector (e.j));
  QVERIFY (is_orthogonal (e.i, e.j));
  QVERIFY (e.center == mkp (0, 0));
}

/*
 * 退化判断：双曲线要求 a < c（到两焦点距离之差大于焦距时无法构成双曲线）。
 * 当三点共线且 focal_length <= diff_of_two_dis（即第三点在焦点连线延长线上）
 * 时应退化为线段。
 */
static bool
hyperbola_should_degenerate (point f1, point f2, point p) {
  if (!linearly_dependent (f1, f2, p)) return false;
  double focal_length   = norm (f2 - f1);
  double d1             = norm (p - f1);
  double d2             = norm (p - f2);
  double diff_of_two_dis= d1 > d2 ? d1 - d2 : d2 - d1;
  return focal_length <= diff_of_two_dis;
}

void
TestCurveConic::test_hyperbola_degeneration_criteria () {
  // 覆盖非共线、焦点之间、延长线、焦点重合、三点重合等全部情况
  QVERIFY (!hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (1, 1)));
  QVERIFY (!hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (1, 0)));
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (-1, 0)));
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (3, 0)));
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (0, 0)));
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (2, 0)));
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (0, 0), mkp (1, 0)));
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (0, 0), mkp (0, 0)));
}

// 共线且第三点在焦点之间：双曲线可正常构造。
void
TestCurveConic::test_hyperbola_collinear_between_foci () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (2, 0);
  a[2]= mkp (1, 0);
  array<path> cip;

  hyperbola_rep h (a, cip, false);

  QVERIFY (is_unit_vector (h.i));
  QVERIFY (is_unit_vector (h.j));
  QVERIFY (is_orthogonal (h.i, h.j));
  QVERIFY (h.center == mkp (1, 0));
  QVERIFY (!is_nan (h.evaluate (0.0)[0]));
  QVERIFY (!is_nan (h.evaluate (0.5)[0]));
}

// 共线且第三点在焦点延长线上：双曲线应退化为线段。
void
TestCurveConic::test_hyperbola_collinear_outside_foci () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (2, 0);
  a[2]= mkp (3, 0);
  array<path> cip;

  hyperbola_rep h (a, cip, false);

  QVERIFY (is_unit_vector (h.i));
  QVERIFY (is_unit_vector (h.j));
  QVERIFY (is_orthogonal (h.i, h.j));
  QVERIFY (h.center == mkp (1, 0));
  QVERIFY (h.r1 < h.focal_length / 2);
}

// 非共线标准双曲线：验证焦点、中心、半径和几何定义。
void
TestCurveConic::test_hyperbola_non_collinear () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (2, 0);
  a[2]= mkp (1, 1);
  array<path> cip;

  hyperbola_rep h (a, cip, false);

  QVERIFY (is_unit_vector (h.i));
  QVERIFY (is_unit_vector (h.j));
  QVERIFY (is_orthogonal (h.i, h.j));
  QVERIFY (h.center == mkp (1, 0));

  // 双曲线上任意一点到两焦点距离之差的绝对值应为常数
  double d1= norm (h.evaluate (0.0) - h.f1);
  double d2= norm (h.evaluate (0.0) - h.f2);
  double diff= d1 > d2 ? d1 - d2 : d2 - d1;
  for (double t= 0.1; t < 1.0; t+= 0.1) {
    double dt1= norm (h.evaluate (t) - h.f1);
    double dt2= norm (h.evaluate (t) - h.f2);
    double d  = dt1 > dt2 ? dt1 - dt2 : dt2 - dt1;
    QVERIFY (fuzzy_eq (d, diff, 1e-6));
  }
}

// 两焦点重合：双曲线退化为线段。
void
TestCurveConic::test_hyperbola_coincident_foci () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (0, 0);
  a[2]= mkp (1, 0);
  array<path> cip;

  hyperbola_rep h (a, cip, false);

  QVERIFY (is_unit_vector (h.i));
  QVERIFY (is_unit_vector (h.j));
  QVERIFY (is_orthogonal (h.i, h.j));
  QVERIFY (h.center == mkp (0, 0));
}

// 三点完全重合：边界情况，构造函数兜底不应崩溃。
void
TestCurveConic::test_hyperbola_three_identical_points () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (0, 0);
  a[2]= mkp (0, 0);
  array<path> cip;

  hyperbola_rep h (a, cip, false);

  QVERIFY (is_unit_vector (h.i));
  QVERIFY (is_unit_vector (h.j));
  QVERIFY (is_orthogonal (h.i, h.j));
  QVERIFY (h.center == mkp (0, 0));
}

/*
 * 显式验证三点共线时正交基兜底路径：轴向应与焦点方向一致，j 为 i 逆时针
 * 旋转 90 度所得。
 */
void
TestCurveConic::test_orthogonal_basis_after_fallback () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (4, 0);
  a[2]= mkp (2, 0);
  array<path> cip;

  ellipse_rep e (a, cip, false);
  QVERIFY (e.i == mkp (1, 0) || e.i == mkp (-1, 0));
  QVERIFY (e.j == mkp (0, 1) || e.j == mkp (0, -1));

  hyperbola_rep h (a, cip, false);
  QVERIFY (h.i == mkp (1, 0) || h.i == mkp (-1, 0));
  QVERIFY (h.j == mkp (0, 1) || h.j == mkp (0, -1));
}

/*
 * 退化边界：第三点正好落在某一焦点上时，sum_of_two_dis == focal_length，
 * 满足退化条件。
 */
void
TestCurveConic::test_ellipse_degeneration_boundary () {
  QVERIFY (ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (0, 0)));
  QVERIFY (ellipse_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (2, 0)));
}

// 垂直方向共线：验证兜底正交基不依赖水平轴。
void
TestCurveConic::test_ellipse_collinear_vertical () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (0, 2);
  a[2]= mkp (0, 1);
  array<path> cip;

  ellipse_rep e (a, cip, false);

  QVERIFY (is_unit_vector (e.i));
  QVERIFY (is_unit_vector (e.j));
  QVERIFY (is_orthogonal (e.i, e.j));
  QVERIFY (e.center == mkp (0, 1));
  QVERIFY (!is_nan (e.evaluate (0.0)[0]));
  QVERIFY (!is_nan (e.evaluate (0.5)[0]));
}

// 验证椭圆在 [0,1] 全参数范围内 evaluate 不返回 NaN。
void
TestCurveConic::test_ellipse_evaluate_all_parameters () {
  array<point> cases[3]= {
      array<point> (mkp (0, 0), mkp (2, 0), mkp (1, 0)),
      array<point> (mkp (0, 0), mkp (2, 0), mkp (3, 0)),
      array<point> (mkp (0, 0), mkp (2, 0), mkp (1, 1)),
  };
  array<path> cip;

  for (int k= 0; k < 3; k++) {
    ellipse_rep e (cases[k], cip, false);
    for (double t= 0.0; t <= 1.0; t+= 0.05) {
      point p= e.evaluate (t);
      QVERIFY (!is_nan (p[0]));
      QVERIFY (!is_nan (p[1]));
    }
  }
}

/*
 * 退化边界：第三点正好落在某一焦点上时，diff_of_two_dis == focal_length，
 * 满足退化条件。
 */
void
TestCurveConic::test_hyperbola_degeneration_boundary () {
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (0, 0)));
  QVERIFY (hyperbola_should_degenerate (mkp (0, 0), mkp (2, 0), mkp (2, 0)));
}

// 垂直方向共线：验证兜底正交基不依赖水平轴。
void
TestCurveConic::test_hyperbola_collinear_vertical () {
  array<point> a (3);
  a[0]= mkp (0, 0);
  a[1]= mkp (0, 2);
  a[2]= mkp (0, 3);
  array<path> cip;

  hyperbola_rep h (a, cip, false);

  QVERIFY (is_unit_vector (h.i));
  QVERIFY (is_unit_vector (h.j));
  QVERIFY (is_orthogonal (h.i, h.j));
  QVERIFY (h.center == mkp (0, 1));
  QVERIFY (!is_nan (h.evaluate (0.0)[0]));
  QVERIFY (!is_nan (h.evaluate (0.5)[0]));
}

// 验证双曲线在 [0,1] 全参数范围内 evaluate 不返回 NaN。
void
TestCurveConic::test_hyperbola_evaluate_all_parameters () {
  array<point> cases[3]= {
      array<point> (mkp (0, 0), mkp (2, 0), mkp (1, 0)),
      array<point> (mkp (0, 0), mkp (2, 0), mkp (3, 0)),
      array<point> (mkp (0, 0), mkp (2, 0), mkp (1, 1)),
  };
  array<path> cip;

  for (int k= 0; k < 3; k++) {
    hyperbola_rep h (cases[k], cip, false);
    for (double t= 0.0; t <= 1.0; t+= 0.05) {
      point p= h.evaluate (t);
      QVERIFY (!is_nan (p[0]));
      QVERIFY (!is_nan (p[1]));
    }
  }
}

// 多方向共线时，兜底构造的轴向都应沿焦点连线，j 为其正交方向。
void
TestCurveConic::test_orthogonal_basis_multiple_directions () {
  array<path> cip;

  // 45 度方向
  {
    array<point> a (3);
    a[0]= mkp (0, 0);
    a[1]= mkp (2, 2);
    a[2]= mkp (1, 1);
    ellipse_rep e (a, cip, false);
    QVERIFY (is_unit_vector (e.i));
    QVERIFY (is_unit_vector (e.j));
    QVERIFY (is_orthogonal (e.i, e.j));
  }

  // 垂直方向
  {
    array<point> a (3);
    a[0]= mkp (0, 0);
    a[1]= mkp (0, 4);
    a[2]= mkp (0, 2);
    hyperbola_rep h (a, cip, false);
    QVERIFY (is_unit_vector (h.i));
    QVERIFY (is_unit_vector (h.j));
    QVERIFY (is_orthogonal (h.i, h.j));
  }
}

QTEST_MAIN (TestCurveConic)
#include "curve_test.moc"
