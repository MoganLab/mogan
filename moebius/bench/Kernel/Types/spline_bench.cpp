/** \file spline_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for spline construction (tridiagonal solves)
 *  \date   2026
 */

#include "curve.hpp"
#include "nanobench.h"
#include "point.hpp"

/** 构造 n 个控制点的点集 */
static array<point>
mk_points (int n) {
  array<point> a;
  for (int i= 0; i < n; i++)
    a << point (0.1 * i, 0.05 * ((i * 7) % 23));
  return a;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (100).unit ("ctor");

  array<point> pts= mk_points (256);
  // 开样条走 tridiag_solve,闭合样条走 xtridiag/quasitridiag_solve
  bench.run ("spline open pts256", [&] {
    ankerl::nanobench::doNotOptimizeAway (
        spline (pts, array<path> (), false, true));
  });
  bench.run ("spline closed pts256", [&] {
    ankerl::nanobench::doNotOptimizeAway (
        spline (pts, array<path> (), true, true));
  });
  return 0;
}
