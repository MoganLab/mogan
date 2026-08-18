
/******************************************************************************
 * MODULE     : point.cpp
 * DESCRIPTION: points
 * COPYRIGHT  : (C) 2003  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "point.hpp"
#include "math_util.hpp"
#include "tree_helper.hpp"

point
operator- (const point& p) {
  int   i, n= N (p);
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= -p[i];
  return r;
}

point
operator+ (const point& p1, const point& p2) {
  int   i, n= min (N (p1), N (p2));
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= p1[i] + p2[i];
  return r;
}

point
operator- (const point& p1, const point& p2) {
  int   i, n= min (N (p1), N (p2));
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= p1[i] - p2[i];
  return r;
}

point
operator* (double x, const point& p) {
  int   i, n= N (p);
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= x * p[i];
  return r;
}

point
operator* (const point& p1, const point& p2) {
  int   i, n= min (N (p1), N (p2));
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= p1[i] * p2[i];
  return r;
}

point
operator/ (const point& p, double x) {
  int   i, n= N (p);
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= p[i] / x;
  return r;
}

point
operator/ (const point& p1, const point& p2) {
  int   i, n= min (N (p1), N (p2));
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= p1[i] / p2[i];
  return r;
}

bool
operator== (const point& p1, const point& p2) {
  if (N (p1) != N (p2)) return false;
  int i, n= N (p1);
  for (i= 0; i < n; i++)
    if (!fnull (p1[i] - p2[i], 1e-6)) return false;
  return true;
}

point
abs (const point& p) {
  int   i, n= N (p);
  point r (n);
  for (i= 0; i < n; i++)
    r[i]= fabs (p[i]);
  return r;
}

double
min (const point& p) {
  int i, n= N (p);
  ASSERT (N (p) > 0, "non empty point expected");
  double r= p[0];
  for (i= 1; i < n; i++)
    r= min (r, p[i]);
  return r;
}

double
max (const point& p) {
  int i, n= N (p);
  ASSERT (N (p) > 0, "non empty point expected");
  double r= p[0];
  for (i= 1; i < n; i++)
    r= max (r, p[i]);
  return r;
}

bool
is_point (tree t) {
  return L (t) == moebius::POINT;
}

point
as_point (tree t) {
  if (!is_tuple (t) && !is_point (t)) return point ();
  else {
    int   i, n= N (t);
    point p (n);
    for (i= 0; i < n; i++)
      p[i]= as_double (t[i]);
    return p;
  }
}

tree
as_tree (const point& p) {
  int  i, n= N (p);
  tree t (moebius::POINT, n);
  for (i= 0; i < n; i++)
    t[i]= as_string (p[i]);
  return t;
}

double
inner (const point& p1, const point& p2) {
  int    i, n= min (N (p1), N (p2));
  double r= 0;
  for (i= 0; i < n; i++)
    r+= p1[i] * p2[i];
  return r;
}

// 以下三个内联辅助函数在不构造临时 point 的情况下直接计算
// 差向量的内积与范数平方，供距离/共线等热点路径使用

/** @brief 计算 (a1-a0) 与 q 的内积 */
static inline double
inner_diff (const point& a1, const point& a0, const point& q) {
  int    i, n= min (N (a1), min (N (a0), N (q)));
  double r= 0;
  for (i= 0; i < n; i++)
    r+= (a1[i] - a0[i]) * q[i];
  return r;
}

/** @brief 计算 (a1-a0) 与 (b1-b0) 的内积 */
static inline double
inner_ddiff (const point& a1, const point& a0, const point& b1,
             const point& b0) {
  int    i, n= min (min (N (a1), N (a0)), min (N (b1), N (b0)));
  double r= 0;
  for (i= 0; i < n; i++)
    r+= (a1[i] - a0[i]) * (b1[i] - b0[i]);
  return r;
}

/** @brief 计算 (p1-p0) 的范数平方 */
static inline double
norm2_diff (const point& p1, const point& p0) {
  int    i, n= min (N (p1), N (p0));
  double r= 0;
  for (i= 0; i < n; i++) {
    double d= p1[i] - p0[i];
    r+= d * d;
  }
  return r;
}

static point
mult (double re, double im, point p) {
  if (N (p) == 0) p= point (0.0, 0.0);
  if (N (p) == 1) p= point (p[0], 0.0);
  return point (re * p[0] - im * p[1], re * p[1] + im * p[0]);
}

point
rotate_2D (const point& p, const point& o, double angle) {
  return mult (cos (angle), sin (angle), p - o) + o;
}

point
slanted (const point& p, double slant) {
  return point (p[0] + p[1] * slant, p[1]);
}

double
arg (point p) {
  double n= norm (p);
  p       = p / n;
  if (p[1] < 0) return 2 * tm_PI - acos (p[0]);
  else return acos (p[0]);
}

point
proj (const axis& ax, const point& p) {
  int n= min (N (ax.p0), N (ax.p1));
  // t = (a·p - a·p0) / (a·a)，a = p1 - p0，全程不构造差向量
  double aa= inner_ddiff (ax.p1, ax.p0, ax.p1, ax.p0);
  if (sqrt (aa) < 1.0e-6) return ax.p0;
  double t=
      (inner_diff (ax.p1, ax.p0, p) - inner_diff (ax.p1, ax.p0, ax.p0)) / aa;
  point r (n);
  for (int i= 0; i < n; i++)
    r[i]= ax.p0[i] + t * (ax.p1[i] - ax.p0[i]);
  return r;
}

double
dist (const axis& ax, const point& p) {
  // 与 proj 同公式，但直接累加残差平方和，不构造投影点与差向量
  double aa= inner_ddiff (ax.p1, ax.p0, ax.p1, ax.p0);
  if (sqrt (aa) < 1.0e-6) return sqrt (norm2_diff (p, ax.p0));
  double t= inner_ddiff (ax.p1, ax.p0, p, ax.p0) / aa;
  int    i, n= min (N (p), min (N (ax.p0), N (ax.p1)));
  double r= 0;
  for (i= 0; i < n; i++) {
    double d= p[i] - ax.p0[i] - t * (ax.p1[i] - ax.p0[i]);
    r+= d * d;
  }
  return sqrt (r);
}

double
seg_dist (const axis& ax, const point& p) {
  // inner(ab,ap)>0 && inner(ba,bp)>0 等价于两个差向量内积均为正
  if (inner_ddiff (ax.p1, ax.p0, p, ax.p0) > 0 &&
      inner_ddiff (ax.p0, ax.p1, p, ax.p1) > 0)
    return dist (ax, p);
  else return min (sqrt (norm2_diff (p, ax.p0)), sqrt (norm2_diff (p, ax.p1)));
}

double
seg_dist (const point& p1, const point& p2, const point& p) {
  axis ax;
  ax.p0= p1;
  ax.p1= p2;
  return seg_dist (ax, p);
}

bool
collinear (const point& p1, const point& p2) {
  return fnull (fabs (inner (p1, p2)) - norm (p1) * norm (p2), 1.0e-6);
}

bool
linearly_dependent (const point& p1, const point& p2, const point& p3) {
  // norm(p1-p2)<eps 等价于 norm2_diff < eps*eps，避免构造差向量
  if (norm2_diff (p1, p2) < 1e-12 || norm2_diff (p2, p3) < 1e-12 ||
      norm2_diff (p3, p1) < 1e-12)
    return true;
  double c= inner_ddiff (p2, p1, p3, p1);
  return fnull (fabs (c) - sqrt (norm2_diff (p2, p1) * norm2_diff (p3, p1)),
                1.0e-6);
}

bool
orthogonalize (point& i, point& j, const point& p1, const point& p2,
               const point& p3) {
  if (linearly_dependent (p1, p2, p3)) return false;
  // i = (p2-p1)/|p2-p1|，j = Gram-Schmidt 归一化，均只分配一次结果数组
  int    n  = min (N (p2), N (p1));
  double inv= 1.0 / sqrt (norm2_diff (p2, p1));
  i         = point (n);
  for (int k= 0; k < n; k++)
    i[k]= (p2[k] - p1[k]) * inv;
  int    m= min (N (p3), N (p1));
  point  d (m);
  double c= 0;
  for (int k= 0; k < m; k++) {
    d[k]= p3[k] - p1[k];
    c+= d[k] * i[k];
  }
  j       = point (m);
  double s= 0;
  for (int k= 0; k < m; k++) {
    j[k]= d[k] - c * i[k];
    s+= j[k] * j[k];
  }
  double invj= 1.0 / sqrt (s);
  for (int k= 0; k < m; k++)
    j[k]*= invj;
  return true;
}

// perpendicular bisector of P1 and P2. P3 is meaningless!!
axis
midperp (const point& p1, const point& p2, const point& p3) {
  axis a;
  if (linearly_dependent (p1, p2, p3)) a.p0= a.p1= point (0);
  else {
    point i, j;
    orthogonalize (i, j, p1, p2, p3);
    int n= min (N (p1), N (p2));
    a.p0 = point (n);
    a.p1 = point (n);
    for (int k= 0; k < n; k++) {
      a.p0[k]= (p1[k] + p2[k]) / 2;
      a.p1[k]= a.p0[k] + j[k];
    }
  }
  return a;
}

point
intersection (const axis& A, const axis& B) {
  point i, j;
  if (!orthogonalize (i, j, A.p0, A.p1, B.p0)) {
    if (orthogonalize (i, j, A.p0, A.p1, B.p1)) return B.p0;
    else return point (0);
  }
  point a (2), b (2), u (2), v (2), p (2);
  a[0]= a[1]= 0;
  u[0]      = inner (A.p1 - A.p0, i);
  u[1]      = inner (A.p1 - A.p0, j);
  b[0]      = inner (B.p0 - A.p0, i);
  b[1]      = inner (B.p0 - A.p0, j);
  v[0]      = inner (B.p1 - B.p0, i);
  v[1]      = inner (B.p1 - B.p0, j);
  if (fnull (norm (u), 1e-6) || fnull (norm (v), 1e-6) || collinear (u, v))
    return point (0);
  else {
    double t;
    t= (v[0] * (b[1] - a[1]) + v[1] * (a[0] - b[0])) /
       (v[0] * u[1] - v[1] * u[0]);
    return A.p0 + t * (u[0] * i + u[1] * j);
  }
}

bool
inside_rectangle (const point& p, const point& p1, const point& p2) {
  return p[0] >= p1[0] && p[1] >= p1[1] && p[0] <= p2[0] && p[1] <= p2[1];
}
