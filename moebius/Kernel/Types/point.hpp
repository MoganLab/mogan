
/******************************************************************************
 * MODULE     : point.hpp
 * DESCRIPTION: points
 * COPYRIGHT  : (C) 2003  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef POINT_H
#define POINT_H
#include "tree.hpp"

typedef array<double> point;

// 只读参数一律按 const 引用传递，避免 array 引用计数的原子加减
point operator- (const point& p);
point operator+ (const point& p1, const point& p2);
point operator- (const point& p1, const point& p2);
point operator* (double x, const point& p);
point operator* (const point& p1, const point& p2);
point operator/ (const point& p, double x);
point operator/ (const point& p1, const point& p2);
bool  operator== (const point& p1, const point& p2);

point  abs (const point& p);
double min (const point& p);
double max (const point& p);

bool is_point (tree t);
inline point
as_point (double x) {
  point p (1);
  p[0]= x;
  return p;
}
point as_point (tree t);
tree  as_tree (const point& p);

double inner (const point& p1, const point& p2);
point  rotate_2D (const point& p, const point& o, double angle);
point  slanted (const point& p, double slant);

double norm (const point& p);
double arg (point p);
bool   collinear (const point& p1, const point& p2);
bool   linearly_dependent (const point& p1, const point& p2, const point& p3);
bool   orthogonalize (point& i, point& j, const point& p1, const point& p2,
                      const point& p3);

typedef struct {
  point p0, p1;
} axis;

/**
 * @brief 求点 p 到轴 a（过 p0、p1 的直线）的正交投影
 *
 * 轴不要求是单位向量，投影点为 p0 + t*(p1 - p0)，其中
 * t = (a·p - a·p0) / (a·a)，a = p1 - p0。t 不受 [0,1] 限制，
 * 即投影可以落在 p0、p1 之外（延长线上）；若 p0 与 p1 几乎重合
 * （|a| < 1e-6），退化为直接返回 p0。结果维度取 min(N(p0), N(p1))，
 * 与 p 的维度无关。
 *
 * @param a 轴（过 a.p0 与 a.p1 的直线）
 * @param p 待投影的点
 * @return  p 在轴上的正交投影点
 */
point  proj (const axis& a, const point& p);
double dist (const axis& a, const point& p);
double seg_dist (const axis& a, const point& p);
double seg_dist (const point& p1, const point& p2, const point& p);
axis   midperp (const point& p1, const point& p2, const point& p3);
point  intersection (const axis& A, const axis& B);

/**
 * @brief 判断二维点是否位于闭合矩形内
 *
 * 矩形由其对角顶点 p1（左下角）与 p2（右上角）确定，要求
 * p1[0] <= p2[0] 且 p1[1] <= p2[1]。判定含边界：坐标恰好落在
 * 矩形边上的点视为在矩形内。
 *
 * @param p  待判断的二维点
 * @param p1 矩形的一个对角顶点（各分量取较小值的一角）
 * @param p2 矩形的另一个对角顶点（各分量取较大值的一角）
 * @return   点在矩形内（含边界）时返回 true，否则返回 false
 */
bool inside_rectangle (const point& p, const point& p1, const point& p2);

#endif // defined POINT_H
