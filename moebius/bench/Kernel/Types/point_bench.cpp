/** \file point_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for point (array<double>) operations
 *  \date   2026
 */

#include "math_util.hpp"
#include "nanobench.h"
#include "point.hpp"

static point
mkp (double x, double y) {
  point p (2);
  p[0]= x;
  p[1]= y;
  return p;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (1000).unit ("op");

  point a= mkp (1.25, -2.5), b= mkp (0.75, 3.125), o= mkp (0, 0);

  bench.run ("operator+", [&] { a + b; });
  bench.run ("operator-", [&] { a - b; });
  bench.run ("scalar operator*", [&] { 2.0 * a; });
  bench.run ("operator/", [&] { a / 2.0; });
  bench.run ("operator==", [&] { a == b; });
  bench.run ("inner", [&] { inner (a, b); });
  bench.run ("norm", [&] { norm (a); });

  axis ax;
  ax.p0= mkp (0, 0);
  ax.p1= mkp (10, 0);
  bench.run ("proj", [&] { proj (ax, a); });
  bench.run ("dist", [&] { dist (ax, a); });
  bench.run ("seg_dist", [&] { seg_dist (ax, a); });
  bench.run ("rotate_2D", [&] { rotate_2D (a, o, 0.5); });

  point p1= mkp (0, 0), p2= mkp (1, 0), p3= mkp (2, 1);
  bench.run ("linearly_dependent", [&] { linearly_dependent (p1, p2, p3); });
  bench.run ("midperp", [&] { midperp (p1, p2, p3); });

  point acc= mkp (0, 0);
  bench.run ("composite a-b+2*c", [&] { acc= (a - b) + 2.0 * (b + a); });

  ankerl::nanobench::doNotOptimizeAway (acc);
  return 0;
}
