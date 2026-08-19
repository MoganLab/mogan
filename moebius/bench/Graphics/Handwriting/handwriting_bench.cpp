/** \file handwriting_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for handwriting glyph recognition
 *  \date   2026
 */

#include <cstdio>
#include <cstdlib>

#include "handwriting.hpp"
#include "nanobench.h"
#include "tm_ostream.hpp"

/** \brief 生成模拟手写笔画的折线：k 段折线拼接，含 k 个明显转角 */
static poly_line
mk_stroke (int corners, double w, double h, double phase) {
  poly_line pl;
  int       n= 8 * corners;
  for (int i= 0; i < n; i++) {
    double t= (double) i / (n - 1);
    double x= w * t;
    // 分段线性锯齿：每段一个转角，模拟真实笔画的顶点结构
    double y= h * ((i / 8 % 2 == 0) ? t * 8 - (i / 8) * 2 + 0.5
                                    : 1.5 - (t * 8 - (i / 8) * 2));
    pl << point (x, y + 0.01 * sin (6.28 * t + phase));
  }
  return pl;
}

/** \brief 由基础折线构造单笔轮廓，可选加噪声 */
static contours
mk_glyph (int corners, double phase, double noise, unsigned int& seed) {
  contours gl;
  gl << mk_stroke (corners, 1.0, 0.6, phase);
  if (noise > 0.0)
    for (int i= 0; i < N (gl[0]); i++) {
      seed= seed * 1103515245 + 12345;
      gl[0][i][0]+= noise * ((int) (seed >> 16) % 1000 - 500) / 500.0;
    }
  return gl;
}

/* ---------- 优化前实现（对照组）：无哈希预过滤、二级不变量总是计算 ----------
 */

static void
legacy_recognize_one (contours gl, int& level, string& best, double& best_rec) {
  array<tree>   disc1;
  array<double> cont1;
  invariants (gl, 1, disc1, cont1);
  array<tree>   disc2;
  array<double> cont2;
  invariants (gl, 2, disc2, cont2);

  best    = "";
  best_rec= -100.0;
  for (int i= 0; i < N (learned_glyphs); i++) {
    const glyph_record& r= learned_glyphs[i];
    if (N (r.gl) == N (gl) && disc1 == r.disc1) {
      double dist= l2_norm (r.cont1 - cont1) / sqrt (N (cont1));
      double rec = 1.0 - dist;
      if (rec > best_rec) {
        best_rec= rec;
        best    = r.name;
      }
    }
  }
  if (best != "") {
    level= 1;
    return;
  }
  for (int i= 0; i < N (learned_glyphs); i++) {
    const glyph_record& r= learned_glyphs[i];
    if (N (r.gl) == N (gl) && disc2 == r.disc2) {
      double dist= l2_norm (r.cont2 - cont2) / sqrt (N (cont2));
      double rec = 1.0 - dist;
      if (rec > best_rec) {
        best_rec= rec;
        best    = r.name;
      }
    }
  }
  level= 2;
}

int
main () {
  // 模拟真实规模：数百个已学习字形
  const int    glyph_count= 400;
  unsigned int seed       = 42;
  clear_learned_glyphs ();
  for (int i= 0; i < glyph_count; i++)
    register_glyph ("g" * as_string (i),
                    mk_glyph (2 + i % 5, 0.1 * i, 0.0, seed));

  // 命中一级匹配的查询（常见路径）
  contours hit= mk_glyph (2 + 7 % 5, 0.1 * 7, 0.01, seed);
  // 无匹配查询（走二级回退，无字形命中）
  // 双笔画轮廓：已学习字形均为单笔画，disc 必然不一致，走二级回退且无命中
  contours miss= mk_glyph (2 + 7 % 5, 0.1 * 7 + 3.14159, 0.01, seed);
  miss << mk_stroke (3, 0.5, 0.3, 0.0);

  int    lev;
  string best;
  double rec;
  legacy_recognize_one (hit, lev, best, rec);
  string legacy_hit= best;
  recognize_glyph_one (hit, lev, best, rec);
  if (best != legacy_hit) { // 优化前后结果必须一致
    cout << "FATAL: optimized result " << best << " != legacy " << legacy_hit
         << LF;
    std::exit (1);
  }

  ankerl::nanobench::Bench ()
      .title ("handwriting recognize_glyph_one")
      .unit ("glyph")
      .run ("legacy: hit (level 1)",
            [&] { legacy_recognize_one (hit, lev, best, rec); })
      .run ("optimized: hit (level 1)",
            [&] { recognize_glyph_one (hit, lev, best, rec); })
      .run ("legacy: miss (level 2 fallback)",
            [&] { legacy_recognize_one (miss, lev, best, rec); })
      .run ("optimized: miss (level 2 fallback)",
            [&] { recognize_glyph_one (miss, lev, best, rec); });

  {
    ankerl::nanobench::Bench ()
        .title ("handwriting invariants alone")
        .unit ("glyph")
        .run ("invariants(level=1)",
              [&] {
                array<tree>   disc;
                array<double> cont;
                invariants (hit, 1, disc, cont);
                ankerl::nanobench::doNotOptimizeAway (cont);
              })
        .run ("invariants(level=2)", [&] {
          array<tree>   disc;
          array<double> cont;
          invariants (hit, 2, disc, cont);
          ankerl::nanobench::doNotOptimizeAway (cont);
        });
  }

  clear_learned_glyphs ();
  return 0;
}
