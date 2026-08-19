/** \file frame_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for frame point transforms (scaling / linear_2D)
 *  \date   2026
 */

#include "nanobench.h"
#include "frame.hpp"
#include "matrix.hpp"

// 优化前的标量 scaling 直变换:两个中间 point 临时,用于同二进制 A/B 对比
static point
old_scaling_direct (double magnify, point shift, point p) {
  return shift + magnify * p;
}

// 优化前的按轴 scaling 直变换,用于同二进制 A/B 对比
static point
old_anscaling_direct (point magnify, point shift, point p) {
  return shift + magnify * p;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (10000).unit ("op");

  point  origin= point (0.0, 0.0);
  frame  sc    = scaling (2.0, point (1.0, 1.0));
  frame  ansc  = scaling (point (2.0, 3.0), point (1.0, 1.0));
  frame  lin   = linear_2D (matrix_2D (1.5, 0.2, -0.1, 2.5));

  // 模拟图形渲染:同一片点集反复变换
  enum { NPTS= 1024 };
  static point pts[NPTS];
  for (int i= 0; i < NPTS; i++)
    pts[i]= point (0.1 * i, 0.02 * i);

  double acc= 0.0;
  bench.run ("old scaling direct x1024", [&] {
    for (int i= 0; i < NPTS; i++) {
      point q= old_scaling_direct (2.0, point (1.0, 1.0), pts[i]);
      acc+= q[0];
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new scaling direct x1024", [&] {
    for (int i= 0; i < NPTS; i++) {
      point q= sc (pts[i]);
      acc+= q[0];
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("old an_scaling direct x1024", [&] {
    for (int i= 0; i < NPTS; i++) {
      point q= old_anscaling_direct (point (2.0, 3.0), point (1.0, 1.0), pts[i]);
      acc+= q[0];
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new an_scaling direct x1024", [&] {
    for (int i= 0; i < NPTS; i++) {
      point q= ansc (pts[i]);
      acc+= q[0];
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("linear_2D direct x1024 (baseline)", [&] {
    for (int i= 0; i < NPTS; i++) {
      point q= lin (pts[i]);
      acc+= q[0];
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  // 矩形包围盒:graphics 中 frame::enclose 的典型入口
  rectangle r (0, 0, 100, 100);
  bench.run ("enclose rect scaling", [&] {
    ankerl::nanobench::doNotOptimizeAway (sc (r));
  });
  (void) origin;
  return 0;
}
