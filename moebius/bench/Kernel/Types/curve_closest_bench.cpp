/** \file curve_closest_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for closest-point search on curves (graphical select)
 *  \date   2026
 */

#include "curve.hpp"
#include "math_util.hpp"
#include "nanobench.h"
#include "point.hpp"

/** 构造折线控制点:n 段锯齿 */
static array<point>
mk_points (int n) {
  array<point> a;
  for (int i= 0; i < n; i++)
    a << point (0.1 * i, ((i & 1) ? 0.5 : -0.5));
  return a;
}

// 优化前的逐步距离计算:每步构造差向量临时,用于同二进制 A/B 对比
static array<double>
old_find_closest (curve c, double t1, double t2, point p, double eps) {
  array<double> res;
  double        closest= -1, n0= tm_infinity, nprec= n0;
  bool          stored= true, decreasing= false;
  point         pclosest;
  double        max_step= 0.5 / max (c->nr_components (), 1);
  for (double t= t1; t <= t2;) {
    point  pt= c->evaluate (t);
    double n = norm (pt - p);
    if (n < n0) {
      n0      = n;
      closest = t;
      pclosest= pt;
      stored  = false;
    }
    decreasing= n < nprec;
    if (!stored && !decreasing) {
      res << closest;
      stored= true;
    }
    if (stored && decreasing) n0= tm_infinity;
    double delta= (n - eps) / 2;
    t+= min (max_step, max (0.00001, c->bound (t, max (eps, delta))));
    nprec= n;
  }
  if (!stored && decreasing) res << closest;
  return res;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (100).unit ("search");

  curve        poly= poly_segment (mk_points (128), array<path> ());
  array<point> sa;
  sa << point (0.0, 0.0) << point (1.0, 3.0) << point (3.0, 2.0)
     << point (5.0, 5.0) << point (7.0, 1.0);
  curve spl= spline (sa, array<path> (), false, true);
  point pin= point (6.0, 0.3);
  int   acc= 0;

  bench.run ("old find_closest poly128", [&] {
    acc+= N (old_find_closest (poly, 0.0, 1.0, pin, 0.01));
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new find_closest poly128", [&] {
    acc+= N (poly->find_closest_points (0.0, 1.0, pin, 0.01));
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new find_closest spline", [&] {
    acc+= N (spl->find_closest_points (0.0, 1.0, pin, 0.01));
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("closest point poly128", [&] {
    acc+= (int) closest (poly, pin)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });

  // 圆弧/椭圆求值与取直:图形渲染的采样内层循环
  ankerl::nanobench::Bench cbench;
  cbench.minEpochIterations (200).unit ("sweep");
  array<point> ea;
  // 两焦点与椭圆上一点:r1=5, r2=3
  ea << point (-4.0, 0.0) << point (4.0, 0.0) << point (0.0, 3.0);
  curve        el= ellipse (ea, array<path> (), true);
  array<point> aa;
  aa << point (0.0, 0.0) << point (10.0, 0.0) << point (0.0, 10.0);
  curve ac= arc (aa, array<path> (), true);
  cbench.run ("ellipse evaluate sweep512", [&] {
    for (int i= 0; i <= 512; i++)
      acc+= el->evaluate (i / 512.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  cbench.run ("arc evaluate sweep512", [&] {
    for (int i= 0; i <= 512; i++)
      acc+= ac->evaluate (i / 512.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  cbench.run ("ellipse rectify eps=0.1", [&] {
    ankerl::nanobench::doNotOptimizeAway (el->rectify (0.1));
  });
  return 0;
}
