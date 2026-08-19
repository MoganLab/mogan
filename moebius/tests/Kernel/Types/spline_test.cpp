/******************************************************************************
 * MODULE     : spline_test.cpp
 * DESCRIPTION: Unit tests for spline_interval_no (knot interval location)
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "curve.hpp"
#include "moe_doctests.hpp"

/**
 * 构造与 spline_rep（非闭合、n=5）构造函数同型的节点向量：
 * 两端各 3 个 epsilon 间隔的小节点，中间按 1 递增。
 */
static array<double>
open_knots () {
  const double  eps= 0.01;
  array<double> U;
  double        x= 0.0;
  for (int i= 0; i < 3; i++) {
    U << x;
    x+= eps;
  }
  x+= 1.0 - eps;
  for (int i= 3; i <= 5; i++) {
    U << x;
    x+= 1.0;
  }
  for (int i= 0; i < 3; i++) {
    U << x;
    x+= eps;
  }
  return U;
}

TEST_CASE ("test basic intervals") {
  array<double> U= open_knots ();
  int           n= N (U);
  // 任意 u 落在 [U[0], U[n-1]) 内时，返回的 i 必须满足 U[i] <= u < U[i+1]
  for (int i= 0; i + 1 < n; i++) {
    double mid= (U[i] + U[i + 1]) / 2;
    int    no = spline_interval_no (U, mid);
    CHECK (no == i);
    CHECK (U[no] <= mid);
    CHECK (mid < U[no + 1]);
  }
}

TEST_CASE ("test knot boundaries") {
  array<double> U= open_knots ();
  // 恰好落在节点上：左闭右开，u == U[i] 应归入区间 i
  CHECK (spline_interval_no (U, U[0]) == 0);
  for (int i= 1; i + 1 < N (U); i++) {
    int no= spline_interval_no (U, U[i]);
    CHECK (no == i);
    CHECK (U[no] <= U[i]);
    CHECK (U[i] < U[no + 1]);
  }
}

TEST_CASE ("test out of range") {
  array<double> U= open_knots ();
  int           n= N (U);
  CHECK (spline_interval_no (U, U[0] - 1.0) == -1);
  CHECK (spline_interval_no (U, U[n - 1]) == -1);
  CHECK (spline_interval_no (U, U[n - 1] + 1.0) == -1);
}

TEST_CASE ("test degenerate knot vectors") {
  // 节点向量太短：无法构成任何区间
  array<double> empty;
  CHECK (spline_interval_no (empty, 0.0) == -1);
  array<double> single (1);
  single[0]= 1.0;
  CHECK (spline_interval_no (single, 1.0) == -1);
  // 两元素节点向量的唯一区间 [0, 1)
  array<double> two (2);
  two[0]= 0.0;
  two[1]= 1.0;
  CHECK (spline_interval_no (two, 0.0) == 0);
  CHECK (spline_interval_no (two, 0.5) == 0);
  CHECK (spline_interval_no (two, 1.0) == -1);
}

TEST_CASE ("test closed spline uniform knots") {
  // 闭合样条的节点向量：0,1,2,... 严格递增步长 1
  array<double> U (10);
  for (int i= 0; i < 10; i++)
    U[i]= (double) i;
  for (int i= 0; i + 1 < 10; i++) {
    CHECK (spline_interval_no (U, i + 0.5) == i);
    CHECK (spline_interval_no (U, (double) i) == i);
  }
  CHECK (spline_interval_no (U, 9.5) == -1); // 9.5 >= U[9]
  CHECK (spline_interval_no (U, -0.5) == -1);
}
