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

static bool
almost_eq (point p, point q, double eps= 1e-9) {
  int n= N (p);
  if (n != N (q)) return false;
  for (int i= 0; i < n; i++)
    if (fabs (p[i] - q[i]) > eps) return false;
  return true;
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
