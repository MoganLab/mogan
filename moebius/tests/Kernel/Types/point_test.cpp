/******************************************************************************
 * MODULE     : point_test.cpp
 * DESCRIPTION: Unit tests for point (array<double>) operations
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "math_util.hpp"
#include "moe_doctests.hpp"
#include "point.hpp"

static point
mkp (double x, double y) {
  point p (2);
  p[0]= x;
  p[1]= y;
  return p;
}

TEST_CASE ("test equality") {
  CHECK (mkp (1, 2) == mkp (1, 2));
  CHECK (!(mkp (1, 2) == mkp (1, 3)));
}

TEST_CASE ("test arithmetic") {
  CHECK ((mkp (1, 2) + mkp (3, 4)) == mkp (4, 6));
  CHECK ((mkp (3, 4) - mkp (1, 2)) == mkp (2, 2));
  CHECK ((-mkp (1, -2)) == mkp (-1, 2));
}

TEST_CASE ("test scalar ops") {
  CHECK ((2.0 * mkp (1, 2)) == mkp (2, 4));
  CHECK ((mkp (4, 6) / 2.0) == mkp (2, 3));
}

TEST_CASE ("test inner and norm") {
  CHECK_EQ (inner (mkp (1, 2), mkp (3, 4)), 11.0);
  CHECK_EQ (norm (mkp (3, 4)), 5.0);
}

TEST_CASE ("test min/max/abs") {
  CHECK_EQ (min (mkp (3, -1)), -1.0);
  CHECK_EQ (max (mkp (3, -1)), 3.0);
  CHECK (abs (mkp (-3, 2)) == mkp (3, 2));
}

TEST_CASE ("test rotate_2D") {
  point r= rotate_2D (mkp (1, 0), mkp (0, 0), tm_PI / 2);
  CHECK (fabs (r[0]) < 1e-6);
  CHECK (fabs (r[1] - 1.0) < 1e-6);
}

TEST_CASE ("test slanted") { CHECK (slanted (mkp (2, 4), 0.5) == mkp (4, 4)); }

TEST_CASE ("test inside_rectangle") {
  CHECK (inside_rectangle (mkp (1, 1), mkp (0, 0), mkp (2, 2)));
  CHECK (!inside_rectangle (mkp (3, 1), mkp (0, 0), mkp (2, 2)));
}

TEST_CASE ("test collinear") {
  CHECK (collinear (mkp (1, 0), mkp (2, 0)));
  CHECK (!collinear (mkp (1, 0), mkp (0, 1)));
}

TEST_CASE ("test tree roundtrip") {
  point p= mkp (1.5, 2.5);
  tree  t= as_tree (p);
  CHECK (is_point (t));
  point q= as_point (t);
  CHECK (q == p);
}

TEST_CASE ("test axis proj/dist") {
  axis a;
  a.p0   = mkp (0, 0);
  a.p1   = mkp (10, 0);
  point p= mkp (3, 4);
  CHECK (proj (a, p) == mkp (3, 0));
  CHECK (fabs (dist (a, p) - 4.0) < 1e-6);
  CHECK (fabs (seg_dist (a, p) - 4.0) < 1e-6);
}
