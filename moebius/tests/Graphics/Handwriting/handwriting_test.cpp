/******************************************************************************
 * MODULE     : handwriting_test.cpp
 * DESCRIPTION: Unit tests for handwriting recognition and simplification
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "moe_doctests.hpp"
#include "handwriting.hpp"

static bool
almost_eq_point (point p, point q, double eps= 1e-9) {
  if (N (p) != N (q)) return false;
  for (int i= 0; i < N (p); i++)
    if (fabs (p[i] - q[i]) > eps) return false;
  return true;
}

/** \brief 生成 k 段折线拼接的锯齿笔画，含 k 个明显转角 */
static poly_line
mk_stroke (int corners, double w, double h) {
  poly_line pl;
  int       n= 8 * corners;
  for (int i= 0; i < n; i++) {
    double t= (double) i / (n - 1);
    double x= w * t;
    double y= h * ((i / 8 % 2 == 0) ? t * 8 - (i / 8) * 2 + 0.5
                                    : 1.5 - (t * 8 - (i / 8) * 2));
    pl << point (x, y);
  }
  return pl;
}

static contours
mk_glyph (int corners) {
  contours gl;
  gl << mk_stroke (corners, 1.0, 0.6);
  return gl;
}

/** \brief 给折线加小幅扰动，模拟同一字形的不同书写 */
static contours
perturb (contours gl, double amp) {
  for (int i= 0; i < N (gl); i++)
    for (int j= 0; j < N (gl[i]); j++) {
      gl[i][j][0]+= amp * sin (3.0 * j);
      gl[i][j][1]+= amp * cos (2.0 * j);
    }
  return gl;
}

TEST_CASE ("test register_glyph and recognize_glyph round trip") {
  clear_learned_glyphs ();
  register_glyph ("two", mk_glyph (2));
  register_glyph ("three", mk_glyph (3));
  register_glyph ("four", mk_glyph (4));
  CHECK_EQ (N (learned_names), 3);

  // 完全相同的轮廓：一级匹配
  int    lev = 0;
  string best= "";
  double rec = 0.0;
  recognize_glyph_one (mk_glyph (3), lev, best, rec);
  CHECK_EQ (best, "three");
  CHECK_EQ (lev, 1);
  CHECK (rec > 0.5);

  // 同形状扰动版本仍应识别为同一字形
  recognize_glyph_one (perturb (mk_glyph (3), 0.002), lev, best, rec);
  CHECK_EQ (best, "three");

  // 无匹配（笔画数不同，一二级均无命中）
  contours miss= mk_glyph (3);
  miss << mk_stroke (2, 0.5, 0.3);
  recognize_glyph_one (miss, lev, best, rec);
  CHECK_EQ (best, "");
  CHECK_EQ (lev, 2);

  // recognize_glyph 对单字形轮廓应返回同一名字
  CHECK_EQ (recognize_glyph (mk_glyph (4)), "four");

  clear_learned_glyphs ();
  CHECK_EQ (N (learned_names), 0);
  CHECK_EQ (recognize_glyph (mk_glyph (3)), "");
}

TEST_CASE ("test learned hash consistency") {
  clear_learned_glyphs ();
  register_glyph ("a", mk_glyph (2));
  // 学习时缓存的哈希应与按 disc 现算的哈希一致（识别预过滤的正确性前提）
  array<tree>   disc1;
  array<double> cont1;
  invariants (mk_glyph (2), 1, disc1, cont1);
  CHECK_EQ (learned_hash1[0], hash (disc1));
  clear_learned_glyphs ();
}

TEST_CASE ("test simplify") {
  // 少于等于两个点：原样返回
  array<point> two;
  two << point (0.0, 0.0) << point (1.0, 1.0);
  CHECK_EQ (N (simplify (two, 0.01, 0.5)), 2);

  // 密采样共线点（间距 < eps，被跳过的点 seg_dist 为 0）应大幅压缩，
  // 且首末点保持不变
  array<point> line;
  for (int i= 0; i < 100; i++)
    line << point (0.01 * i, 0.0);
  array<point> s= simplify (line, 0.05, 10.0);
  CHECK (N (s) < 20);
  CHECK (N (s) >= 2);
  CHECK (almost_eq_point (s[0], line[0]));
  CHECK (almost_eq_point (s[N (s) - 1], line[N (line) - 1]));
  // 保留的相邻点间距不小于 eps（continue 跳过更短跳转）
  for (int i= 1; i < N (s); i++) {
    double d= 0.0;
    for (int k= 0; k < N (s[i]); k++)
      d+= (s[i][k] - s[i - 1][k]) * (s[i][k] - s[i - 1][k]);
    CHECK (sqrt (d) >= 0.05 - 1e-9);
  }

  // 稀疏点列（间距 > eps）不受影响：逐点链的总代价为 0，无需删点
  array<point> sparse;
  for (int i= 0; i < 20; i++)
    sparse << point (0.5 * i, 0.0);
  CHECK_EQ (N (simplify (sparse, 0.05, 10.0)), 20);

  // 密采样的直角折线：拐点 (0.49, 0) 附近应保留转角
  array<point> bent;
  for (int i= 0; i < 50; i++)
    bent << point (0.01 * i, 0.0);
  for (int i= 1; i <= 50; i++)
    bent << point (0.49 + 0.01 * i, 0.01 * i);
  array<point> sb= simplify (bent, 0.05, 10.0);
  CHECK (N (sb) < N (bent));
  bool has_corner= false;
  for (int i= 0; i < N (sb); i++)
    if (almost_eq_point (sb[i], point (0.49, 0.0), 1e-6)) has_corner= true;
  CHECK (has_corner);
  CHECK (almost_eq_point (sb[0], bent[0]));
  CHECK (almost_eq_point (sb[N (sb) - 1], bent[N (bent) - 1]));
}
