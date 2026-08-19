/** \file curve_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for curve operations
 *  \date   2026
 */

#include "curve.hpp"
#include "nanobench.h"
#include "point.hpp"

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (100000).unit ("op");

  curve        seg= segment (point (0.0, 0.0), point (3.0, 4.0));
  array<point> a;
  a << point (0.0, 0.0) << point (1.0, 2.0) << point (4.0, 2.0)
    << point (5.0, 0.0);
  curve poly= poly_segment (a, array<path> ());

  double acc= 0;
  bench.run ("segment bound", [&] { acc+= seg->bound (0.5, 1e-3); });
  bench.run ("poly_segment bound", [&] { acc+= poly->bound (0.5, 1e-3); });

  ankerl::nanobench::doNotOptimizeAway (acc);

  // spline 求值/取直:interval_no 缓存与逐点临时消除的整体收益
  ankerl::nanobench::Bench sbench;
  sbench.minEpochIterations (100).unit ("sweep");

  array<point> sa;
  sa << point (0.0, 0.0) << point (1.0, 3.0) << point (3.0, 2.0)
     << point (5.0, 5.0) << point (7.0, 1.0);
  curve sp= spline (sa, array<path> (), false, true);

  double sacc= 0;
  // t 单调推进的典型求值扫(渲染/取直),interval_no 缓存命中最多的场景
  sbench.run ("spline evaluate sweep monotonic", [&] {
    for (int i= 0; i <= 512; i++)
      sacc+= sp->evaluate (i / 512.0)[0];
    ankerl::nanobench::doNotOptimizeAway (sacc);
  });
  // t 来回震荡,缓存大多不命中,验证回退路径无明显回退
  sbench.run ("spline evaluate sweep alternating", [&] {
    for (int i= 0; i <= 512; i++)
      sacc+= sp->evaluate ((i & 1) ? i / 512.0 : 1.0 - i / 512.0)[0];
    ankerl::nanobench::doNotOptimizeAway (sacc);
  });
  sbench.run ("spline rectify eps=0.1", [&] {
    ankerl::nanobench::doNotOptimizeAway (sp->rectify (0.1));
  });
  return 0;
}
