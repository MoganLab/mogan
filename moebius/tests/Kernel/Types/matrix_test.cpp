/******************************************************************************
 * MODULE     : matrix_test.cpp
 * DESCRIPTION: Unit tests for matrix operations
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "matrix.hpp"
#include "moe_doctests.hpp"

TEST_CASE ("test matrix construction") {
  matrix<double> m (1.0, 2, 2);
  CHECK_EQ (NR (m), 2);
  CHECK_EQ (NC (m), 2);
  CHECK_EQ (m (0, 0), 1.0);
  CHECK_EQ (m (0, 1), 0.0);
  CHECK_EQ (m (1, 0), 0.0);
  CHECK_EQ (m (1, 1), 1.0);
}

TEST_CASE ("test matrix_2D") {
  matrix<double> m= matrix_2D (1.0, 2.0, 3.0, 4.0);
  CHECK_EQ (m (0, 0), 1.0);
  CHECK_EQ (m (0, 1), 2.0);
  CHECK_EQ (m (1, 0), 3.0);
  CHECK_EQ (m (1, 1), 4.0);
}

TEST_CASE ("test matrix addition and subtraction") {
  matrix<double> a= matrix_2D (1.0, 2.0, 3.0, 4.0);
  matrix<double> b= matrix_2D (5.0, 6.0, 7.0, 8.0);
  matrix<double> s= a + b;
  CHECK_EQ (s (0, 0), 6.0);
  CHECK_EQ (s (0, 1), 8.0);
  CHECK_EQ (s (1, 0), 10.0);
  CHECK_EQ (s (1, 1), 12.0);
  matrix<double> d= b - a;
  CHECK_EQ (d (0, 0), 4.0);
  CHECK_EQ (d (0, 1), 4.0);
  CHECK_EQ (d (1, 0), 4.0);
  CHECK_EQ (d (1, 1), 4.0);
}

TEST_CASE ("test matrix multiplication") {
  matrix<double> a= matrix_2D (1.0, 2.0, 3.0, 4.0);
  matrix<double> b= matrix_2D (2.0, 0.0, 1.0, 2.0);
  matrix<double> p= a * b;
  CHECK_EQ (p (0, 0), 4.0);
  CHECK_EQ (p (0, 1), 4.0);
  CHECK_EQ (p (1, 0), 10.0);
  CHECK_EQ (p (1, 1), 8.0);
}

TEST_CASE ("test matrix vector multiplication") {
  matrix<double> m= matrix_2D (1.0, 2.0, 3.0, 4.0);
  vector<double> v (0.0, 2);
  v[0]= 1.0;
  v[1]= 2.0;
  vector<double> r= m * v;
  CHECK_EQ (N (r), 2);
  CHECK_EQ (r[0], 5.0);
  CHECK_EQ (r[1], 11.0);
}

TEST_CASE ("test transpose") {
  matrix<double> m= matrix_2D (1.0, 2.0, 3.0, 4.0);
  matrix<double> t= transpose (m);
  CHECK_EQ (t (0, 0), 1.0);
  CHECK_EQ (t (0, 1), 3.0);
  CHECK_EQ (t (1, 0), 2.0);
  CHECK_EQ (t (1, 1), 4.0);
}

TEST_CASE ("test invert 2x2") {
  matrix<double> m= matrix_2D (4.0, 7.0, 2.0, 6.0);
  matrix<double> i= invert (m);
  matrix<double> p= m * i;
  CHECK (fabs (p (0, 0) - 1.0) < 1e-9);
  CHECK (fabs (p (0, 1)) < 1e-9);
  CHECK (fabs (p (1, 0)) < 1e-9);
  CHECK (fabs (p (1, 1) - 1.0) < 1e-9);
}

TEST_CASE ("test copy") {
  matrix<double> m= matrix_2D (1.0, 2.0, 3.0, 4.0);
  matrix<double> c= copy (m);
  CHECK_EQ (c (0, 0), 1.0);
  CHECK_EQ (c (1, 1), 4.0);
}

TEST_CASE ("test unary minus") {
  matrix<double> m= matrix_2D (1.0, 2.0, 3.0, 4.0);
  matrix<double> n= -m;
  CHECK_EQ (n (0, 0), -1.0);
  CHECK_EQ (n (1, 1), -4.0);
}

TEST_MEMORY_LEAK_INIT
TEST_MEMORY_LEAK_ALL
