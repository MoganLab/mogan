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
  return 0;
}
