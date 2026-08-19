/******************************************************************************
 * MODULE     : poly_line_test.cpp
 * DESCRIPTION: Unit tests for poly line routines
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "moe_doctests.hpp"
#include "poly_line.hpp"

static poly_line
mk_pl (double x0, double y0, double x1, double y1) {
  poly_line pl;
  pl << point (x0, y0) << point (x1, y1);
  return pl;
}

static poly_line
mk_pl (double x0, double y0, double x1, double y1, double x2, double y2) {
  poly_line pl;
  pl << point (x0, y0) << point (x1, y1) << point (x2, y2);
  return pl;
}

TEST_CASE ("test l2_norm and point distance") {
  CHECK_EQ (l2_norm (point (3.0, 4.0)), 5.0);
  CHECK_EQ (distance (point (0.0, 0.0), point (3.0, 4.0)), 5.0);
  CHECK_EQ (distance (point (1.0, 1.0), point (1.0, 1.0)), 0.0);
}

TEST_CASE ("test project onto segment") {
  CHECK (project (point (0.5, 1.0), point (0.0, 0.0), point (1.0, 0.0)) ==
         point (0.5, 0.0));
  // 端点截断
  CHECK (project (point (2.0, 1.0), point (0.0, 0.0), point (1.0, 0.0)) ==
         point (1.0, 0.0));
  CHECK (project (point (-1.0, 1.0), point (0.0, 0.0), point (1.0, 0.0)) ==
         point (0.0, 0.0));
}

TEST_CASE ("test point to segment distance") {
  CHECK_EQ (distance (point (0.5, 1.0), point (0.0, 0.0), point (1.0, 0.0)),
            1.0);
  CHECK_EQ (distance (point (2.0, 0.0), point (0.0, 0.0), point (1.0, 0.0)),
            1.0);
  // 退化线段：端点重合
  CHECK_EQ (distance (point (0.0, 3.0), point (0.0, 0.0), point (0.0, 0.0)),
            3.0);
}

TEST_CASE ("test inf and sup on points") {
  CHECK (inf (point (1.0, 5.0), point (3.0, 2.0)) == point (1.0, 2.0));
  CHECK (sup (point (1.0, 5.0), point (3.0, 2.0)) == point (3.0, 5.0));
}

TEST_CASE ("test distance to poly_line") {
  poly_line pl= mk_pl (0.0, 0.0, 1.0, 1.0, 2.0, 0.0);
  CHECK_EQ (distance (point (0.5, 1.0), pl), sqrt (0.125));
  CHECK (distance (point (1.0, 1.0), pl) < 1e-9);
  // 单点折线
  poly_line one;
  one << point (1.0, 2.0);
  CHECK_EQ (distance (point (4.0, 6.0), one), 5.0);
}

TEST_CASE ("test nearby") {
  poly_line pl= mk_pl (0.0, 0.0, 10.0, 0.0);
  CHECK (nearby (point (5.0, 3.0), pl));
  CHECK (!nearby (point (5.0, 6.0), pl));
}

TEST_CASE ("test inf and sup on poly_line") {
  poly_line pl= mk_pl (1.0, 4.0, -2.0, 0.0, 3.0, -5.0);
  CHECK (inf (pl) == point (-2.0, -5.0));
  CHECK (sup (pl) == point (3.0, 4.0));
}

TEST_CASE ("test poly_line arithmetic operators") {
  poly_line pl= mk_pl (1.0, 1.0, 2.0, 2.0);
  poly_line sh= pl + point (1.0, -1.0);
  CHECK (sh[0] == point (2.0, 0.0));
  CHECK (sh[1] == point (3.0, 1.0));
  poly_line bk= sh - point (1.0, -1.0);
  CHECK (bk[0] == pl[0]);
  CHECK (bk[1] == pl[1]);
  poly_line sc= 2.0 * pl;
  CHECK (sc[0] == point (2.0, 2.0));
  CHECK (sc[1] == point (4.0, 4.0));
}

TEST_CASE ("test normalize poly_line") {
  poly_line pl= mk_pl (2.0, 2.0, 4.0, 4.0);
  poly_line nr= normalize (pl);
  CHECK (almost_eq (nr[0], point (0.0, 0.0)));
  CHECK (almost_eq (nr[1], point (1.0, 1.0)));
  // 退化：所有点相同，缩放因子为 0
  poly_line deg= mk_pl (1.0, 1.0, 1.0, 1.0);
  poly_line dr = normalize (deg);
  CHECK (dr[0] == point (0.0, 0.0));
  // 空折线
  poly_line empty;
  CHECK (N (normalize (empty)) == 0);
}

TEST_CASE ("test length and access") {
  poly_line pl= mk_pl (0.0, 0.0, 3.0, 0.0, 3.0, 4.0);
  CHECK_EQ (length (pl), 7.0);
  CHECK (access (pl, 0.0) == point (0.0, 0.0));
  CHECK (almost_eq (access (pl, 1.5), point (1.5, 0.0)));
  CHECK (almost_eq (access (pl, 5.0), point (3.0, 2.0)));
  CHECK (almost_eq (access (pl, -1.0), point (0.0, 0.0)));
  CHECK (almost_eq (access (pl, 100.0), point (3.0, 4.0)));
}

TEST_CASE ("test vertices") {
  // 直线段：只有首尾
  poly_line     line= mk_pl (0.0, 0.0, 0.5, 0.0, 1.0, 0.0);
  array<double> ts  = vertices (line);
  // REQUIRE 依赖异常，MSVC 关异常下不可用；CHECK 后需自行防止越界访问
  CHECK (N (ts) == 2);
  if (N (ts) == 2) {
    CHECK_EQ (ts[0], 0.0);
    CHECK_EQ (ts[1], 1.0);
  }
  // 直角折线：中间顶点应被检出
  poly_line     bend= mk_pl (0.0, 0.0, 1.0, 0.0, 1.0, 1.0);
  array<double> vs  = vertices (bend);
  CHECK (N (vs) == 3);
  CHECK (vs[1] > 0.0);
  CHECK (vs[1] < 1.0);
  CHECK_EQ (vs[0], 0.0);
  CHECK_EQ (vs[2], 1.0);
}

TEST_CASE ("test vertices normalized arc-length semantics") {
  // L 形折线：总长 2，拐点在弧长 1 处 → 归一化参数应为 0.5
  poly_line     bend= mk_pl (0.0, 0.0, 1.0, 0.0, 1.0, 1.0);
  array<double> vs  = vertices (bend);
  CHECK (N (vs) == 3);
  if (N (vs) == 3) {
    CHECK (almost_eq (vs[1], 0.5));
    // 乘回总长应取回拐点坐标（与 access 的配套契约）
    CHECK (almost_eq (access (bend, vs[1] * length (bend)),
                      point (1.0, 0.0)));
  }

  // 非对称 L：总长 1+3=4，拐点在弧长 1 处 → 参数 0.25
  poly_line     bend2= mk_pl (0.0, 0.0, 1.0, 0.0, 1.0, 3.0);
  array<double> vs2  = vertices (bend2);
  CHECK (N (vs2) == 3);
  if (N (vs2) == 3)
    CHECK (almost_eq (vs2[1], 0.25));

  // 密采样同一 L 形（拐点前后各插 10 个共线点）：参数不应随采样密度漂移
  poly_line dense;
  for (int i= 0; i <= 10; i++)
    dense << point (0.1 * i, 0.0);
  for (int i= 1; i <= 10; i++)
    dense << point (1.0, 0.3 * i);
  array<double> vsd= vertices (dense);
  CHECK (N (vsd) == 3);
  if (N (vsd) == 3)
    CHECK (almost_eq (vsd[1], 0.25));
}

TEST_CASE ("test vertices on multi-corner zigzag") {
  // 三段等长折线：两个拐点，参数应约在 1/3 与 2/3
  poly_line zz;
  zz << point (0.0, 0.0) << point (1.0, 0.0) << point (1.0, 1.0)
     << point (2.0, 1.0);
  array<double> vs= vertices (zz);
  CHECK (N (vs) == 4);
  if (N (vs) == 4) {
    CHECK (almost_eq (vs[1], 1.0 / 3.0));
    CHECK (almost_eq (vs[2], 2.0 / 3.0));
  }

  // 通用性质：严格递增，首 0 末 1，相邻间距 >= 0.025
  poly_line     dense;
  for (int i= 0; i < 40; i++)
    dense << point (i % 2 ? 1.0 : 0.0, 0.25 * i);
  array<double> vdz= vertices (dense);
  CHECK_EQ (vdz[0], 0.0);
  CHECK_EQ (vdz[N (vdz) - 1], 1.0);
  for (int i= 1; i < N (vdz); i++) {
    CHECK (vdz[i] > vdz[i - 1]);
    CHECK (vdz[i] - vdz[i - 1] >= 0.025);
  }
}

TEST_CASE ("test contours operations") {
  contours gl;
  gl << mk_pl (0.0, 0.0, 2.0, 2.0);
  gl << mk_pl (10.0, 0.0, 12.0, 2.0);

  CHECK (almost_eq (inf (gl), point (0.0, 0.0)));
  CHECK (almost_eq (sup (gl), point (12.0, 2.0)));
  CHECK_EQ (distance (point (6.0, 0.0), gl), 4.0);
  CHECK (nearby (point (6.0, 3.0), gl));
  CHECK (!nearby (point (6.0, 6.0), gl));

  contours sh= gl + point (1.0, 1.0);
  CHECK (sh[0][0] == point (1.0, 1.0));
  contours bk= sh - point (1.0, 1.0);
  CHECK (bk[0][0] == gl[0][0]);
  CHECK (bk[1][1] == gl[1][1]);
  contours sc= 0.5 * gl;
  CHECK (sc[1][1] == point (6.0, 1.0));

  contours nr= normalize (gl);
  CHECK (almost_eq (inf (nr), point (0.0, 0.0), 1e-9));
  CHECK (max (sup (nr)) <= 1.0 + 1e-9);
}

TEST_CASE ("test invariants") {
  contours gl;
  gl << mk_pl (0.0, 0.0, 2.0, 2.0);
  array<tree>   disc;
  array<double> cont;
  invariants (gl, 1, disc, cont);
  // 离散特征：轮廓条数 + 每条折线顶点数
  CHECK (N (disc) == 2);
  if (N (disc) == 2) {
    CHECK (disc[0] == tree ("1"));
    CHECK (disc[1] == tree ("2"));
  }
  // 连续特征：21 个采样点坐标 + 顶点参数（缩放 2.5 倍）
  CHECK (N (cont) == 21 * 2 + 2);
  CHECK_EQ (cont[0], 0.0);
  CHECK_EQ (cont[1], 0.0);
  CHECK (cont[N (cont) - 1] <= 2.5 + 1e-9);
}

TEST_CASE ("test invariants level 2 shape") {
  // level 2：无顶点信息，disc 仅轮廓条数，cont 仅坐标采样
  contours gl;
  gl << mk_pl (0.0, 0.0, 2.0, 2.0);
  array<tree>   disc;
  array<double> cont;
  invariants (gl, 2, disc, cont);
  CHECK (N (disc) == 1);
  if (N (disc) == 1) CHECK (disc[0] == tree ("1"));
  CHECK (N (cont) == 21 * 2);
}

TEST_CASE ("test invariants translation and scale invariance") {
  // 同一 L 形折线平移 + 整体放大：cont 应逐点一致（内部 normalize）
  contours base;
  base << mk_pl (0.0, 0.0, 1.0, 0.0, 1.0, 3.0);
  contours moved;
  moved << mk_pl (5.0, 7.0, 9.0, 7.0, 9.0, 23.0);
  array<tree>   d1, d2;
  array<double> c1, c2;
  invariants (base, 1, d1, c1);
  invariants (moved, 1, d2, c2);
  CHECK (N (c1) == N (c2));
  bool same= true;
  for (int i= 0; i < min (N (c1), N (c2)); i++)
    if (!almost_eq (c1[i], c2[i])) same= false;
  CHECK (same);
  CHECK (N (d1) == N (d2));
}

TEST_CASE ("test invariants multi-contour and append semantics") {
  // 多轮廓：disc 首项为条数，随后每条折线一个顶点数
  contours gl;
  gl << mk_pl (0.0, 0.0, 1.0, 0.0, 1.0, 1.0);
  gl << mk_pl (3.0, 0.0, 5.0, 0.0);
  array<tree>   disc;
  array<double> cont;
  invariants (gl, 1, disc, cont);
  CHECK (N (disc) == 3);
  if (N (disc) == 3) {
    CHECK (disc[0] == tree ("2"));
    CHECK (disc[1] == tree ("3"));
    CHECK (disc[2] == tree ("2"));
  }
  // cont 等长性：两条折线各 21 点坐标 + 各自顶点参数（3 个与 2 个）
  CHECK (N (cont) == 2 * (21 * 2) + 3 + 2);

  // out 参数为追加写入：再调一次 disc/cont 翻倍
  invariants (gl, 1, disc, cont);
  CHECK (N (disc) == 6);
  CHECK (N (cont) == 2 * (2 * (21 * 2) + 3 + 2));
}
