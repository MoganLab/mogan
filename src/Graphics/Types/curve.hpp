
/******************************************************************************
 * MODULE     : curve.hpp
 * DESCRIPTION: mathematical curves
 * COPYRIGHT  : (C) 2003  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef CURVE_H
#define CURVE_H
#include "frame.hpp"
#include "point.hpp"

class curve_rep : public abstract_struct {
public:
  inline curve_rep () {}
  virtual ~curve_rep () {}

  virtual int nr_components () { return 1; }
  // the number of components of the curve is useful for getting
  // nice parameterizations when concatenating curves

  virtual point evaluate (double t)= 0;
  // gives a point on the curve for its intrinsic parameterization
  // curves are parameterized from 0.0 to 1.0

  array<point> rectify (double eps);
  // returns a rectification of the curve, which, modulo reparameterization
  // has a uniform distance of at most 'eps' to the original curve

  virtual void rectify_cumul (array<point>& a, double eps)= 0;
  // add rectification of the curve  (except for the starting point)
  // to an existing polysegment

  /*
  NOTE: more routines should be added later so that one
  can reliably compute the intersections between curves
  One might for instance take the following:
  */
  virtual double bound (double t, double eps)= 0;
  // return delta such that |t' - t| < delta => |c(t') - c(t)| < eps.

  virtual point grad (double t, bool& error)= 0;
  // compute the first derivative at t.
  // set error= true if this derivative does not exist.

  virtual double curvature (double t1, double t2)= 0;
  // compute a bound for the second derivative between t1 and t2.
  /* FIXME: What is computed is *really* a bound for the curvature,
       not for the norm of the second derivative. Make it precise
       what it is that is computed exactly. */
  // return a very large number if such a bound does not exist.

  // returns the number of control points which belong to the curve.
  // these control points are ordered and come first in pts & cips.
  virtual int get_control_points (array<double>& abs, array<point>& pts,
                                  array<path>& cip);

  virtual array<double> find_closest_points (double t1, double t2, point p,
                                             double eps);

  virtual double find_closest_point (double t1, double t2, point p, double eps,
                                     bool& found);
};

class curve {
  ABSTRACT_NULL (curve);
  inline point operator() (double t) { return rep->evaluate (t); }
  inline bool  operator== (curve c) { return rep == c.rep; }
  inline bool  operator!= (curve c) { return rep != c.rep; }
  curve_rep*   get_rep () const { return rep; }
};
ABSTRACT_NULL_CODE (curve);

struct transformed_curve_rep : public curve_rep {
  frame f;
  curve c;
  int   n;
  transformed_curve_rep (frame f2, curve c2)
      : f (f2), c (c2), n (c->nr_components ()) {}
  int    nr_components () { return n; }
  point  evaluate (double t) { return f (c (t)); }
  void   rectify_cumul (array<point>& a, double eps);
  double bound (double t, double eps) { return curve_rep::bound (t, eps); }
  point  grad (double t, bool& error);
  double curvature (double t1, double t2) {
    (void) t1;
    (void) t2;
    TM_FAILED ("not yet implemented");
    return 0.0;
  }
  int get_control_points (array<double>& abs, array<point>& pts,
                          array<path>& cip);
};

struct ellipse_rep : public curve_rep {
  array<point> points;
  array<path>  cip;
  point        f1, f2;       // two foci of the ellipsis
  point        center;       // the center of the ellipse
  double       focal_length; // the distance between f1 and f2
  double sum_of_two_dis; // The sum of the distances of any point on the ellipse
                         // to f1 and f2
  point  i, j;           // The two base vectors of the ellipsis's 2D plane
  double r1, r2;         // The two radiuses of the ellipsis
  ellipse_rep (array<point> a, array<path> cip, bool close);
  point  evaluate (double t) override;
  void   rectify_cumul (array<point>& cum, double eps) override;
  double bound (double t, double eps) override;
  point  grad (double t, bool& error) override;
  double curvature (double t1, double t2) override;
  int    get_control_points (array<double>& abs, array<point>& pts,
                             array<path>& cip) override;
};

struct hyperbola_rep : public curve_rep {
  array<point> points;
  array<path>  cip;
  point        f1, f2;       // two foci of the hyperbola
  point        center;       // the center of the hyperbola
  double       focal_length; // the distance between f1 and f2
  double diff_of_two_dis; // The difference of the distances of any point on the
                          // hyperbola to f1 and f2
  point  i, j;            // The two base vectors of the hyperbola's 2D plane
  double r1, r2;          // The semi-axes a and b
  double u_max;           // The maximum value of the parameter u
  hyperbola_rep (array<point> a, array<path> cip, bool close);
  point  evaluate (double t) override;
  void   rectify_cumul (array<point>& cum, double eps) override;
  double bound (double t, double eps) override;
  point  grad (double t, bool& error) override;
  double curvature (double t1, double t2) override;
  int    get_control_points (array<double>& abs, array<point>& pts,
                             array<path>& cip) override;
};

struct parabola_rep : public curve_rep {
  array<point> points;
  array<path>  cip;
  point        d1, d2; // two points defining the directrix
  point        f;      // the focus
  point        vertex; // vertex of the parabola
  point i, j; // The two base vectors (i perpendicular to directrix pointing to
              // focus, j parallel to directrix)
  double d;   // distance from focus to directrix
  double u_max; // The maximum value of the parameter u
  parabola_rep (array<point> a, array<path> cip, bool close);
  point  evaluate (double t) override;
  void   rectify_cumul (array<point>& cum, double eps) override;
  double bound (double t, double eps) override;
  point  grad (double t, bool& error) override;
  double curvature (double t1, double t2) override;
  int    get_control_points (array<double>& abs, array<point>& pts,
                             array<path>& cip) override;
};

/**
 * @brief 构造连接两点的直线段曲线
 *
 * 参数 t ∈ [0,1] 线性插值：`(1-t)*p1 + t*p2`；曲率恒为无穷大（直线）。
 * @param p1 起点
 * @param p2 终点
 * @return 直线段曲线对象
 */
curve segment (point p1, point p2);

/**
 * @brief 构造依序连接多个控制点的折线（多段直线）曲线
 *
 * n+1 个控制点 a[0..n] 构成 n 段直线；参数 t ∈ [0,1] 均匀映射到整条折线
 * （每段占 1/n），各顶点对应参数 i/n。绘图工具中的「折线」（`<line>` 标签）
 * 即由此实现。单个 segment 可视为 n=1 的特例。
 * @param a 控制点序列（至少 2 个点）
 * @param cip 各控制点在源树中的位置（用于图形编辑时的反向定位）
 * @return 折线曲线对象
 */
curve poly_segment (array<point> a, array<path> cip);
curve spline (array<point> a, array<path> cip, bool close= false,
              bool interpol= true);
curve bezier (array<point> a);
curve poly_bezier (array<point> a, array<path> cip, bool simple, bool closed);
curve arc (array<point> a, array<path> cip, bool close= false);
curve ellipse (array<point> a, array<path> cip, bool close= true);
curve hyperbola (array<point> a, array<path> cip, bool close= false);
curve parabola (array<point> a, array<path> cip, bool close= false);
curve compound (array<curve> cs);
curve invert (curve c);
curve part (curve c, double start, double end);
curve truncate (curve c, double t0, double eps);
curve recontrol (curve c, array<point> a, array<path> cip);

array<point> intersection (curve f, curve g, point p0, double eps);
point        closest (curve f, point p);
/**
 * @brief 判断曲线 f 是否为（近似）直线段
 *
 * 在参数 t = 0.25、0.5、0.75 处采样切向量，若所有切向量二维且方向一致
 * （叉积接近零），则视为直线。单点切向对圆等曲线没有代表性，故采样多处。
 * @param f 待检测的曲线
 * @return 曲线近似为直线时返回 true；切向计算失败、非二维或方向不一致返回 false
 */
bool is_straight_line (curve f);

/**
 * @brief 判断曲线是否为线段或折线（可被坐标变换包裹）
 *
 * 直线类图形只有 segment 与 poly_segment 两种实现；只需处理直边时，
 * 可先用本函数排除椭圆、样条等曲线，避免逐边做直线性检测。
 * @param c 待检测的曲线，可为 nil
 * @return 线段或折线（含被 transformed_curve_rep / recontrol_curve_rep
 *         包裹的情形）返回 true
 */
bool is_polyline_or_segment (curve c);

/**
 * @brief 求曲线各直边的中点，仅返回参考点所贴近的边的中点
 * @param c   待处理的曲线，可为 nil（返回空数组）
 * @param p   参考点（通常为鼠标位置），与曲线处于同一坐标系
 * @param tol 贴近容差：参考点到边的距离（seg_dist）大于该值时忽略该边
 * @return 满足条件的各直边中点（曲线坐标系），按控制点区间顺序排列
 * @note 按 get_control_points 的控制点区间逐边处理，区间划分与
 *       curve_box_rep::graphical_select 一致：闭合曲线（首末控制点参数
 *       不恰为 0/1）补首尾相连边。退化边（两端点距离 < 1e-6）跳过；
 *       中点取区间参数中值的 evaluate 结果，而非两端点算术平均（对
 *       参数化不均匀的曲线两者可能不同）。调用方须保证传入的是线段或
 *       折线（is_polyline_or_segment），非直边曲线不做过滤。
 */
array<point> straight_edge_midpoints (curve c, point p, double tol);

array<point> simplify_polyline (array<point> a, double eps);
array<point> std_bezier_fit (array<point> a, int pack_size);
array<point> alt_bezier_fit (array<point> a, int pack_size);
array<point> bezier_fit (array<point> a, double eps, double advance= 1.0);
array<point> rectify_bezier (array<point> bez, double eps);
array<point> refine (array<point> a, int factor);
array<point> smoothen (array<point> a, int width);
array<point> oval_profile (double rx, double ry, double a, int nr);
array<point> calligraphy (array<point> a, array<point> pen);

#endif // defined CURVE_H
