
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

point operator- (point p);
point operator+ (point p1, point p2);
point operator- (point p1, point p2);
point operator* (double x, point p);
point operator* (point p1, point p2);
point operator/ (point p, double x);
point operator/ (point p1, point p2);
bool  operator== (point p1, point p2);

point  abs (point p);
double min (point p);
double max (point p);

bool is_point (tree t);
inline point
as_point (double x) {
  point p (1);
  p[0]= x;
  return p;
}
point as_point (tree t);
tree  as_tree (point p);

double inner (point p1, point p2);
point  rotate_2D (point p, point o, double angle);
point  slanted (point p, double slant);

double norm (point p);
double arg (point p);
bool   collinear (point p1, point p2);
bool   linearly_dependent (point p1, point p2, point p3);
bool   orthogonalize (point& i, point& j, point p1, point p2, point p3);

typedef struct {
  point p0, p1;
} axis;

point  proj (axis a, point p);
double dist (axis a, point p);
double seg_dist (axis a, point p);
double seg_dist (point p1, point p2, point p);
axis   midperp (point p1, point p2, point p3);
point  intersection (axis A, axis B);

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
bool inside_rectangle (point p, point p1, point p2);

#endif // defined POINT_H
