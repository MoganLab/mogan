/******************************************************************************
 * MODULE     : rectangles_ops_test.cpp
 * DESCRIPTION: Unit tests for rectangle/rectangles operations
 *              (complement to rectangles_test.cpp which covers disjoint_union)
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "moe_doctests.hpp"
#include "rectangles.hpp"

TEST_CASE ("test equality") {
  rectangle a (0, 0, 10, 10);
  rectangle b (0, 0, 10, 10);
  rectangle c (1, 1, 10, 10);
  CHECK (a == b);
  CHECK (a != c);
}

TEST_CASE ("test area") {
  CHECK_EQ (area (rectangle (0, 0, 4, 5)), 20.0);
  CHECK_EQ (area (rectangle (5, 5, 5, 5)), 0.0);
}

TEST_CASE ("test is_zero") {
  CHECK (is_zero (rectangle (0, 0, 0, 0)));
  CHECK (!is_zero (rectangle (1, 0, 0, 0)));
}

TEST_CASE ("test intersect") {
  rectangle r1 (0, 0, 10, 10);
  rectangle r2 (5, 5, 15, 15);
  rectangle r3 (20, 20, 30, 30);
  CHECK (intersect (r1, r2));
  CHECK (!intersect (r1, r3));
}

TEST_CASE ("test subset") {
  rectangle r1 (0, 0, 10, 10);
  rectangle small (2, 2, 8, 8);
  rectangle r2 (5, 5, 15, 15);
  CHECK (small <= r1);
  CHECK (!(r2 <= r1));
}

TEST_CASE ("test translate") {
  rectangle r (0, 0, 10, 10);
  CHECK (translate (r, 5, 7) == rectangle (5, 7, 15, 17));
}

TEST_CASE ("test thicken") {
  rectangle r (0, 0, 10, 10);
  CHECK (thicken (r, 2, 3) == rectangle (-2, -3, 12, 13));
}

TEST_CASE ("test scaling") {
  rectangle r (1, 1, 4, 4);
  CHECK ((r * 3) == rectangle (3, 3, 12, 12));
  CHECK ((r / 2) == rectangle (0, 0, 2, 2));
}

TEST_CASE ("test lub two") {
  rectangle r1 (0, 0, 5, 5);
  rectangle r2 (3, 3, 10, 10);
  CHECK (least_upper_bound (r1, r2) == rectangle (0, 0, 10, 10));
}

TEST_CASE ("test lub list") {
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (5, 5, 8, 8), rectangles ()));
  CHECK (least_upper_bound (l) == rectangle (0, 0, 8, 8));
}

TEST_CASE ("test area list") {
  rectangles l= rectangles (rectangle (0, 0, 2, 2),
                            rectangles (rectangle (0, 0, 3, 3), rectangles ()));
  CHECK_EQ (area (l), 13.0);
}

TEST_CASE ("test union and difference") {
  rectangle  r1 (0, 0, 4, 4);
  rectangle  r2 (2, 0, 6, 4);
  rectangles l1= rectangles (r1, rectangles ());
  rectangles l2= rectangles (r2, rectangles ());
  rectangles u = l1 | l2;
  CHECK (least_upper_bound (u) == rectangle (0, 0, 6, 4));
  rectangles d= l1 - l2;
  CHECK_EQ (area (d), 8.0);
}

TEST_CASE ("test correct drops degenerate") {
  rectangles l= rectangles (rectangle (5, 5, 5, 5),
                            rectangles (rectangle (0, 0, 2, 2), rectangles ()));
  rectangles c= correct (l);
  CHECK_EQ (N (c), 1);
}
