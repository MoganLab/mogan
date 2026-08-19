/** \file equations_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for tridiagonal system solvers
 *  \date   2026
 */

#include "equations.hpp"
#include "moe_doctests.hpp"
#include "point.hpp"

static bool
close_to (double x, double y) {
  return fabs (x - y) < 1e-9;
}

static bool
pt_close (point p, double x, double y) {
  return close_to (p[0], x) && close_to (p[1], y);
}

TEST_SUITE ("equations") {

TEST_CASE ("tridiag_solve 3x3 2D") {
  // [2 1 0][x0]   [3 5]      解:x0=(1.5,2) 由回代唯一确定
  // [1 2 1][x1] = [6 9]
  // [0 1 2][x2]   [5 8]
  array<double> a (3), b (3), c (3);
  a[0]= 0; a[1]= 1; a[2]= 1;
  b[0]= 2; b[1]= 2; b[2]= 2;
  c[0]= 1; c[1]= 1; c[2]= 0;
  array<point> y;
  y << point (3, 5) << point (6, 9) << point (5, 8);
  array<point> x (3);
  tridiag_solve (a, b, c, x, y, 3);
  // 验证 A*x = y
  for (int i= 0; i < 3; i++) {
    double rx= a[i] * x[max (i - 1, 0)][0] + b[i] * x[i][0] +
              c[i] * x[min (i + 1, 2)][0];
    double ry= a[i] * x[max (i - 1, 0)][1] + b[i] * x[i][1] +
              c[i] * x[min (i + 1, 2)][1];
    if (i == 0) {
      rx= b[0] * x[0][0] + c[0] * x[1][0];
      ry= b[0] * x[0][1] + c[0] * x[1][1];
    }
    if (i == 2) {
      rx= a[2] * x[1][0] + b[2] * x[2][0];
      ry= a[2] * x[1][1] + b[2] * x[2][1];
    }
    CHECK (close_to (rx, y[i][0]));
    CHECK (close_to (ry, y[i][1]));
  }
}

TEST_CASE ("tridiag_solve uniform dims kept") {
  array<double> a (2), b (2), c (2);
  a[0]= 0; a[1]= 1;
  b[0]= 1; b[1]= 1;
  c[0]= 0; c[1]= 0;
  array<point> y;
  y << point (1, 2) << point (3, 4);
  array<point> x (2);
  tridiag_solve (a, b, c, x, y, 2);
  CHECK_EQ (N (x[0]), 2);
  CHECK_EQ (N (x[1]), 2);
  // 对角系统:x0=y0, x1=y1-x0
  CHECK (pt_close (x[0], 1, 2));
  CHECK (pt_close (x[1], 2, 2));
}

TEST_CASE ("quasitridiag_solve zero coupling is identity") {
  // a0=a1=0 时秩一修正项为零,单位对角系统的解就是 y 本身
  int           n= 4;
  array<double> a (n), b (n), c (n);
  for (int i= 0; i < n; i++) {
    a[i]= 0; b[i]= 1; c[i]= 0;
  }
  array<point> y;
  for (int i= 0; i < n; i++)
    y << point (1.0 * i, 2.0 * i);
  array<point> x (n);
  xtridiag_solve (a, b, c, 0.0, 0.0, x, y, n);
  for (int i= 0; i < n; i++) {
    CHECK (close_to (x[i][0], y[i][0]));
    CHECK (close_to (x[i][1], y[i][1]));
  }
}

TEST_CASE ("quasitridiag_solve rank-1 correction") {
  // 直接调 quasitridiag_solve:A = I + u v^T,u=v=ones
  // Sherman-Morrison:x = y - ones (ones^T y)/(1+n)
  int           n= 4;
  array<double> a (n), b (n), c (n), u (n), v (n);
  for (int i= 0; i < n; i++) {
    a[i]= 0; b[i]= 1; c[i]= 0; u[i]= 1; v[i]= 1;
  }
  array<point> y;
  for (int i= 0; i < n; i++)
    y << point (1.0 * i, 2.0 * i);
  array<point> x (n);
  quasitridiag_solve (a, b, c, u, v, x, y, n);
  double sx= 0, sy= 0;
  for (int i= 0; i < n; i++) {
    sx+= y[i][0];
    sy+= y[i][1];
  }
  for (int i= 0; i < n; i++) {
    CHECK (close_to (x[i][0], y[i][0] - sx / (1 + n)));
    CHECK (close_to (x[i][1], y[i][1] - sy / (1 + n)));
  }
}

} // TEST_SUITE
