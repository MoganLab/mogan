/** \file point_bench2.cpp
 *  \copyright GPLv3
 *  \details Benchmark for point/curve evaluate temp-elimination optimizations
 *  \date   2026
 */

#include "curve.hpp"
#include "nanobench.h"
#include "point.hpp"

// 优化前的 segment 求值:三个中间 point 临时,用于同二进制 A/B 对比
static point
old_segment_evaluate (point p1, point p2, double t) {
  return (1.0 - t) * p1 + t * p2;
}

// 优化前的 poly_segment 求值,用于同二进制 A/B 对比
static point
old_poly_evaluate (array<point> a, int n, double t) {
  int i= max (min ((int) (n * t), n - 1), 0);
  return (i + 1 - n * t) * a[i] + (n * t - i) * a[i + 1];
}

// 优化前的 rotate_2D:p-o 与 +o 两个中间临时,用于同二进制 A/B 对比
static point
old_rotate_2D (point p, point o, double angle) {
  double c= cos (angle), s= sin (angle);
  point   d= p - o;
  if (N (d) == 0) d= point (0.0, 0.0);
  if (N (d) == 1) d= point (d[0], 0.0);
  return point (c * d[0] - s * d[1], s * d[0] + c * d[1]) + o;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (10000).unit ("x1024");

  curve        seg = segment (point (0.0, 0.0), point (3.0, 4.0));
  array<point> a;
  a << point (0.0, 0.0) << point (1.0, 2.0) << point (4.0, 2.0)
    << point (5.0, 0.0);
  curve  poly= poly_segment (a, array<path> ());
  point  o   = point (1.0, 1.0);
  double acc = 0;

  bench.run ("old segment evaluate x1024", [&] {
    for (int i= 0; i < 1024; i++)
      acc+= old_segment_evaluate (point (0.0, 0.0), point (3.0, 4.0),
                                  i / 1024.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new segment evaluate x1024", [&] {
    for (int i= 0; i < 1024; i++)
      acc+= seg->evaluate (i / 1024.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("old poly_segment evaluate x1024", [&] {
    for (int i= 0; i < 1024; i++)
      acc+= old_poly_evaluate (a, 3, i / 1024.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new poly_segment evaluate x1024", [&] {
    for (int i= 0; i < 1024; i++)
      acc+= poly->evaluate (i / 1024.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("old rotate_2D x1024", [&] {
    for (int i= 0; i < 1024; i++)
      acc+= old_rotate_2D (point (2.0, 3.0), o, i / 512.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new rotate_2D x1024", [&] {
    for (int i= 0; i < 1024; i++)
      acc+= rotate_2D (point (2.0, 3.0), o, i / 512.0)[0];
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  return 0;
}
