
/******************************************************************************
 * MODULE     : equations.cpp
 * DESCRIPTION: Some tools for solving systems of equations
 * COPYRIGHT  : (C) 2004  Henri Lesourd
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "math_util.hpp"
#include "point.hpp"

// Tridiagonal & quasi tridiagonal systems
void
tridiag_solve (array<double> a, array<double> b, array<double> c,
               array<point> x, array<point> y, int n) {
  array<double> u (n);
  int           i;
  double        li;
  li= b[0];
  ASSERT (b[0] != 0, "failed tridiag_solve (1)");
  x[0]= y[0] / li;
  for (i= 0; i < n - 1; i++) {
    u[i]= c[i] / li;
    li  = b[i + 1] - a[i + 1] * u[i];
    ASSERT (li != 0, "failed tridiag_solve (2)");
    // 逐分量写入单个新点,免去差向量/除法两个中间 point 临时;
    // x 各行初始为空点,维度不足时整行重建
    int m= min (N (y[i + 1]), N (x[i]));
    if (N (x[i + 1]) != m) x[i + 1]= point (m);
    for (int j= 0; j < m; j++)
      x[i + 1][j]= (y[i + 1][j] - a[i + 1] * x[i][j]) / li;
  }
  for (i= n - 2; i >= 0; i--) {
    // 逐分量就地回代,免去每行一次标量乘临时
    int m= min (N (x[i]), N (x[i + 1]));
    for (int j= 0; j < m; j++)
      x[i][j]-= u[i] * x[i + 1][j];
  }
}

void
quasitridiag_solve (array<double> a, array<double> b, array<double> c,
                    array<double> u, array<double> v, array<point> x,
                    array<point> y, int n) {
  int          i;
  array<point> z (n), up (n);
  for (i= 0; i < n; i++)
    up[i]= as_point (u[i]);
  tridiag_solve (a, b, c, x, y, n);
  tridiag_solve (a, b, c, z, up, n);
  // 累加到一个预分配的点,免去每行 v[i]*x[i] 与 + 的中间临时
  int   d= N (x[0]);
  point vx (d);
  for (int j= 0; j < d; j++)
    vx[j]= v[0] * x[0][j];
  for (i= 1; i < n; i++)
    for (int j= 0; j < min (d, N (x[i])); j++)
      vx[j]+= v[i] * x[i][j];
  double vz;
  vz= v[0] * z[0][0];
  for (i= 1; i < n; i++)
    vz+= v[i] * z[i][0];
  double inv= 1.0 / (1 + vz);
  for (i= 0; i < n; i++) {
    // 逐分量就地修正,免去每行一个标量乘临时
    double zi= z[i][0] * inv;
    for (int j= 0; j < N (x[i]); j++)
      x[i][j]-= zi * vx[j];
  }
}

void
xtridiag_solve (array<double> a, array<double> b, array<double> c, double a0,
                double a1, array<point> x, array<point> y, int n) {
  array<double> u (n), v (n);
  int           i;
  for (i= 0; i < n; i++)
    u[i]= v[i]= 0;
  u[0]= u[n - 1]= 1;
  v[0]          = a0;
  v[n - 1]      = a1;
  b[0]-= a0;
  b[n - 1]-= a1;
  quasitridiag_solve (a, b, c, u, v, x, y, n);
  b[0]+= a0;
  b[n - 1]+= a1;
}
