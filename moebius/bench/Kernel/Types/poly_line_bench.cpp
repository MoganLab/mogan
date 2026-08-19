/** \file poly_line_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for poly_line / contours operations
 *  \date   2026
 */

#include "nanobench.h"
#include "poly_line.hpp"

/** \brief 生成 n 个顶点的锯齿形折线，模拟手写笔画采样 */
static poly_line
mk_zigzag (int n, double w, double h) {
  poly_line pl;
  for (int i= 0; i < n; i++)
    pl << point (w * i / (n - 1), (i % 2 == 0) ? 0.0 : h);
  return pl;
}

/* ---------- 优化前实现（对照组），用于量化本次优化收益 ---------- */

/// 旧版：逐段经 project（临时分配）+ 每段一次 sqrt
static double
legacy_distance_pl (point p, poly_line pl) {
  double m= 1.0e10;
  if (N (pl) == 1) return distance (p, pl[0]);
  for (int i= 0; i + 1 < N (pl); i++) {
    point proj= project (p, pl[i], pl[i + 1]);
    m         = min (m, distance (p, proj));
  }
  return m;
}

/// 旧版：逐点两两合并，每次迭代分配临时 point
static point
legacy_inf_pl (poly_line pl) {
  point p= pl[0];
  for (int i= 1; i < N (pl); i++)
    p= inf (p, pl[i]);
  return p;
}

static point
legacy_sup_pl (poly_line pl) {
  point p= pl[0];
  for (int i= 1; i < N (pl); i++)
    p= sup (p, pl[i]);
  return p;
}

/// 旧版 access：逐段扫描，每步一次 l2_norm（sqrt + 临时 point 分配），
/// 段长对每个 t 都重复重算
static point
legacy_access_pl (poly_line pl, double t) {
  if (t <= 0) return pl[0];
  for (int i= 1; i < N (pl); i++) {
    double len= l2_norm (pl[i] - pl[i - 1]);
    if (t < len) return pl[i - 1] + (t / len) * (pl[i] - pl[i - 1]);
    t-= len;
  }
  return pl[N (pl) - 1];
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (100).unit ("op");

  // 50 顶点折线接近真实手写笔画的采样规模
  poly_line pl= mk_zigzag (50, 100.0, 10.0);
  point     p = point (50.0, 5.0);
  point     q1= pl[10], q2= pl[11];

  bench.run ("distance(point,point)", [&] { distance (p, q1); });
  bench.run ("distance(point,segment)", [&] { distance (p, q1, q2); });
  bench.run ("distance(point,poly_line)", [&] { distance (p, pl); });
  bench.run ("nearby(point,poly_line)", [&] { nearby (p, pl); });
  bench.run ("inf(poly_line)", [&] { inf (pl); });
  bench.run ("sup(poly_line)", [&] { sup (pl); });
  bench.run ("legacy distance(point,poly_line)",
             [&] { legacy_distance_pl (p, pl); });
  bench.run ("legacy inf(poly_line)", [&] { legacy_inf_pl (pl); });
  bench.run ("legacy sup(poly_line)", [&] { legacy_sup_pl (pl); });
  bench.run ("length(poly_line)", [&] { length (pl); });
  bench.run ("access(poly_line)", [&] { access (pl, 60.0); });
  bench.run ("legacy access(poly_line)",
             [&] { legacy_access_pl (pl, 60.0); });
  // 沿折线均匀取 21 点：模拟 invariants 采样对 access 的批量调用形态
  {
    double l= length (pl);
    bench.run ("access x21 (uniform sampling)", [&] {
      point acc (0.0, 0.0);
      for (int k= 0; k <= 20; k++)
        acc= acc + access (pl, l * k / 20.0);
      ankerl::nanobench::doNotOptimizeAway (acc);
    });
    bench.run ("legacy access x21 (uniform sampling)", [&] {
      point acc (0.0, 0.0);
      for (int k= 0; k <= 20; k++)
        acc= acc + legacy_access_pl (pl, l * k / 20.0);
      ankerl::nanobench::doNotOptimizeAway (acc);
    });
  }
  bench.run ("normalize(poly_line)", [&] { normalize (pl); });
  bench.run ("vertices(poly_line)", [&] { vertices (pl); });

  contours gl;
  gl << mk_zigzag (50, 100.0, 10.0);
  gl << mk_zigzag (30, 80.0, 8.0);
  bench.run ("distance(point,contours)", [&] { distance (p, gl); });
  bench.run ("inf(contours)", [&] { inf (gl); });
  bench.run ("normalize(contours)", [&] { normalize (gl); });
  bench.run ("invariants(level=1)", [&] {
    array<tree>   disc;
    array<double> cont;
    invariants (gl, 1, disc, cont);
    ankerl::nanobench::doNotOptimizeAway (cont);
  });

  ankerl::nanobench::doNotOptimizeAway (pl);
  return 0;
}
