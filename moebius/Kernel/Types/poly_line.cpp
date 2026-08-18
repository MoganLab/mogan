
/******************************************************************************
 * MODULE     : poly_line.cpp
 * DESCRIPTION: Poly lines
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "poly_line.hpp"
#include "math_util.hpp"

// 点到线段距离的平方：内联投影参数并夹断到 [0,1]；
// t == 0 时 a 取 0，末循环退化为到端点 q1 的距离平方
inline double
seg_dist2 (const point& p, const point& q1, const point& q2) {
  int n= N (p);
  ASSERT (n == N (q1) && n == N (q2), "unequal lengths");
  double s= 0.0, t= 0.0;
  for (int i= 0; i < n; i++) {
    double d= q2[i] - q1[i];
    s+= d * (p[i] - q1[i]);
    t+= square (d);
  }
  double a= (t == 0.0) ? 0.0 : s / t;
  if (a < 0.0) a= 0.0;
  if (a > 1.0) a= 1.0;
  double m= 0.0;
  for (int i= 0; i < n; i++)
    m+= square (q1[i] + a * (q2[i] - q1[i]) - p[i]);
  return m;
}

/******************************************************************************
 * Extra routines for points
 ******************************************************************************/

double
l2_norm (point p) {
  return norm (p);
}

double
distance (point p, point q) {
  int n= N (p);
  ASSERT (n == N (q), "unequal lengths");
  double s= 0.0;
  for (int i= 0; i < n; i++)
    s+= square (q[i] - p[i]);
  return sqrt (s);
}

point
project (point p, point q1, point q2) {
  ASSERT (N (p) == N (q1) && N (p) == N (q2), "unequal lengths");
  int    i, n= N (p);
  double s= 0.0, t= 0.0;
  for (i= 0; i < n; i++) {
    s+= (q2[i] - q1[i]) * (p[i] - q1[i]);
    t+= square (q2[i] - q1[i]);
  }
  double a= s / t;
  if (a < 0.0) return q1;
  if (a > 1.0) return q2;
  return q1 + a * (q2 - q1);
}

double
distance (point p, point q1, point q2) {
  return sqrt (seg_dist2 (p, q1, q2));
}

point
inf (point p, point q) {
  ASSERT (N (p) == N (q), "unequal lengths");
  point r (N (p));
  for (int i= 0; i < N (p); i++)
    r[i]= min (p[i], q[i]);
  return r;
}

point
sup (point p, point q) {
  ASSERT (N (p) == N (q), "unequal lengths");
  point r (N (p));
  for (int i= 0; i < N (p); i++)
    r[i]= max (p[i], q[i]);
  return r;
}

/******************************************************************************
 * Poly lines
 ******************************************************************************/

double
distance (point p, const poly_line& pl) {
  int n= N (pl);
  if (n == 1) return distance (p, pl[0]);
  double m2= 1.0e100;
  for (int i= 0; i + 1 < n; i++)
    m2= min (m2, seg_dist2 (p, pl[i], pl[i + 1]));
  return sqrt (m2);
}

bool
nearby (point p, poly_line pl) {
  return distance (p, pl) <= 5.0;
}

// inf/sup 采用原地分量更新，避免逐点两两合并时的临时 point 分配
point
inf (const poly_line& pl) {
  int n= N (pl);
  ASSERT (n > 0, "non zero length expected");
  point p= copy (pl[0]);
  int   m= N (p);
  for (int i= 1; i < n; i++) {
    const point& q= pl[i];
    for (int j= 0; j < m; j++)
      p[j]= min (p[j], q[j]);
  }
  return p;
}

point
sup (const poly_line& pl) {
  int n= N (pl);
  ASSERT (n > 0, "non zero length expected");
  point p= copy (pl[0]);
  int   m= N (p);
  for (int i= 1; i < n; i++) {
    const point& q= pl[i];
    for (int j= 0; j < m; j++)
      p[j]= max (p[j], q[j]);
  }
  return p;
}

poly_line
operator+ (poly_line pl, point p) {
  int       i, n= N (pl);
  poly_line r (n);
  for (i= 0; i < n; i++)
    r[i]= pl[i] + p;
  return r;
}

poly_line
operator- (poly_line pl, point p) {
  int       i, n= N (pl);
  poly_line r (n);
  for (i= 0; i < n; i++)
    r[i]= pl[i] - p;
  return r;
}

poly_line
operator* (double x, poly_line pl) {
  int       i, n= N (pl);
  poly_line r (n);
  for (i= 0; i < n; i++)
    r[i]= x * pl[i];
  return r;
}

poly_line
normalize (poly_line pl) {
  if (N (pl) == 0) return pl;
  pl= pl - inf (pl);
  if (N (pl) == 1) return pl;
  double sc= max (sup (pl));
  if (sc == 0.0) return pl;
  return (1.0 / sc) * pl;
}

double
length (poly_line pl) {
  int    n  = N (pl);
  double len= 0.0;
  for (int i= 1; i < n; i++)
    len+= distance (pl[i], pl[i - 1]);
  return len;
}

point
access (poly_line pl, double t) {
  int n= N (pl);
  if (t < 0) return pl[0];
  for (int i= 1; i < n; i++) {
    point  dp = pl[i] - pl[i - 1];
    double len= l2_norm (dp);
    if (t < len) return pl[i - 1] + (t / len) * dp;
    t-= len;
  }
  return pl[n - 1];
}

/******************************************************************************
 * Contours
 ******************************************************************************/

double
distance (point p, contours gl) {
  double m= 1.0e10;
  for (int i= 0; i < N (gl); i++)
    m= min (m, distance (p, gl[i]));
  return m;
}

bool
nearby (point p, contours gl) {
  return distance (p, gl) <= 5.0;
}

point
inf (contours gl) {
  ASSERT (N (gl) > 0, "non zero length expected");
  point p= inf (gl[0]);
  for (int i= 1; i < N (gl); i++)
    p= inf (p, inf (gl[i]));
  return p;
}

point
sup (contours gl) {
  ASSERT (N (gl) > 0, "non zero length expected");
  point p= sup (gl[0]);
  for (int i= 1; i < N (gl); i++)
    p= sup (p, sup (gl[i]));
  return p;
}

contours
operator+ (contours gl, point p) {
  int      i, n= N (gl);
  contours r (n);
  for (i= 0; i < n; i++)
    r[i]= gl[i] + p;
  return r;
}

contours
operator- (contours gl, point p) {
  int      i, n= N (gl);
  contours r (n);
  for (i= 0; i < n; i++)
    r[i]= gl[i] - p;
  return r;
}

contours
operator* (double x, contours gl) {
  int      i, n= N (gl);
  contours r (n);
  for (i= 0; i < n; i++)
    r[i]= x * gl[i];
  return r;
}

contours
normalize (contours gl) {
  if (N (gl) == 0) return gl;
  gl       = gl - inf (gl);
  double sc= max (sup (gl));
  if (sc == 0.0) return gl;
  gl= (1.0 / sc) * gl;
  return gl;
}

/******************************************************************************
 * Vertex detection
 ******************************************************************************/

array<double>
vertices (poly_line pl) {
  pl= (1.0 / length (pl)) * pl;
  array<double> r;
  double        t = 0.0;
  double        dt= 0.025;
  r << 0.0;
  int    todo_i= -1;
  double todo_t= 0.0;
  double todo_p= 0.0;
  for (int i= 0; i + 1 < N (pl); i++) {
    if (t >= r[N (r) - 1] + dt && t <= 1.0 - dt) {
      double t1= max (t - dt, 0.000000001);
      double t2= min (t + dt, 0.999999999);
      point  p = pl[i];
      point  p1= access (pl, t1);
      point  p2= access (pl, t2);
      double pr= inner (p1 - p, p2 - p);
      if (pr >= 0 && (todo_i < 0 || pr > todo_p)) {
        todo_i= i;
        todo_t= t;
        todo_p= pr;
      }
    }
    t+= distance (pl[i + 1], pl[i]);
    if (todo_i >= 0 && t >= todo_t + dt) {
      r << todo_t;
      todo_i= -1;
    }
  }
  r << 1.0;
  return r;
}

/******************************************************************************
 * Associate invariants to glyphs
 ******************************************************************************/

void
invariants (poly_line pl, int level, array<tree>& disc, array<double>& cont) {
  double l     = length (pl);
  int    pieces= 20;
  for (int i= 0; i <= pieces; i++) {
    double t= (0.999999999 * i) / pieces;
    point  p= access (pl, t * l);
    cont << p;
  }

  if (level <= 1) {
    array<double> ts= vertices (pl);
    disc << tree (as_string (N (ts)));
    cont << (2.5 * ts);
  }
}

void
invariants (contours gl, int level, array<tree>& disc, array<double>& cont) {
  gl= normalize (gl);
  disc << tree (as_string (N (gl)));
  for (int i= 0; i < N (gl); i++)
    invariants (gl[i], level, disc, cont);
}
