/******************************************************************************
 * MODULE     : frame_test.cpp
 * DESCRIPTION: Unit tests for coordinate frame operations
 * COPYRIGHT  : (C) 2026  Da Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "frame.hpp"
#include "math_util.hpp"
#include "matrix.hpp"
#include "moe_doctests.hpp"

static point
mkp (double x, double y) {
  return point (x, y);
}

TEST_CASE ("test shift_2D") {
  frame f= shift_2D (mkp (3.0, 4.0));
  CHECK (f (mkp (1.0, 2.0)) == mkp (4.0, 6.0));
  CHECK (f[mkp (4.0, 6.0)] == mkp (1.0, 2.0));
  CHECK (f->linear);
}

TEST_CASE ("test scaling double") {
  frame f= scaling (2.0, mkp (1.0, 1.0));
  CHECK (f (mkp (1.0, 2.0)) == mkp (3.0, 5.0));
  CHECK (f[mkp (3.0, 5.0)] == mkp (1.0, 2.0));
  CHECK (f->linear);
}

TEST_CASE ("test scaling point") {
  frame f= scaling (mkp (2.0, 3.0), mkp (1.0, 1.0));
  CHECK (f (mkp (1.0, 2.0)) == mkp (3.0, 7.0));
  CHECK (f[mkp (3.0, 7.0)] == mkp (1.0, 2.0));
  CHECK (f->linear);
}

TEST_CASE ("test rotation_2D") {
  frame f= rotation_2D (mkp (0.0, 0.0), tm_PI / 2);
  point r= f (mkp (1.0, 0.0));
  CHECK (fabs (r[0]) < 1e-9);
  CHECK (fabs (r[1] - 1.0) < 1e-9);
  CHECK (f->linear);
}

TEST_CASE ("test slanting") {
  frame f= slanting (mkp (0.0, 0.0), 1.0);
  point r= f (mkp (1.0, 1.0));
  CHECK_EQ (r[0], 2.0);
  CHECK_EQ (r[1], 1.0);
  CHECK (f->linear);
}

TEST_CASE ("test linear_2D") {
  matrix<double> m= matrix_2D (2.0, 0.0, 0.0, 3.0);
  frame          f= linear_2D (m);
  CHECK (f (mkp (1.0, 1.0)) == mkp (2.0, 3.0));
  CHECK (f->linear);
}

TEST_CASE ("test affine_2D") {
  matrix<double> m (0.0, 3, 3);
  m (0, 0)= 2.0;
  m (0, 1)= 0.0;
  m (0, 2)= 1.0;
  m (1, 0)= 0.0;
  m (1, 1)= 3.0;
  m (1, 2)= 1.0;
  m (2, 0)= 0.0;
  m (2, 1)= 0.0;
  m (2, 2)= 1.0;
  frame f = affine_2D (m);
  CHECK (f (mkp (1.0, 1.0)) == mkp (3.0, 4.0));
  CHECK (f->linear);
}

TEST_CASE ("test compound frame") {
  frame s= shift_2D (mkp (1.0, 2.0));
  frame t= scaling (2.0, mkp (0.0, 0.0));
  frame c= t * s;
  CHECK (c (mkp (1.0, 1.0)) == mkp (4.0, 6.0));
}

TEST_CASE ("test invert") {
  frame s= shift_2D (mkp (3.0, 4.0));
  frame i= invert (s);
  CHECK (i (mkp (4.0, 6.0)) == mkp (1.0, 2.0));
}

TEST_CASE ("test frame equality") {
  frame a= shift_2D (mkp (1.0, 2.0));
  frame b= a;
  frame c= shift_2D (mkp (2.0, 2.0));
  CHECK (a == b);
  CHECK (a != c);
}

TEST_CASE ("test rectangle transform") {
  frame     f= scaling (2.0, mkp (0.0, 0.0));
  rectangle r (0, 0, 10, 10);
  rectangle t= f (r);
  CHECK_EQ (t->x1, 0);
  CHECK_EQ (t->y1, 0);
  CHECK_EQ (t->x2, 20);
  CHECK_EQ (t->y2, 20);
}

TEST_CASE ("test jacobian") {
  frame f    = rotation_2D (mkp (0.0, 0.0), tm_PI / 2);
  bool  error= true;
  point j    = f->jacobian (mkp (0.0, 0.0), mkp (1.0, 0.0), error);
  CHECK (!error);
  CHECK (fabs (j[0]) < 1e-9);
  CHECK (fabs (j[1] - 1.0) < 1e-9);
}

TEST_CASE ("test bounds") {
  frame f= scaling (2.0, mkp (0.0, 0.0));
  CHECK (fabs (f->direct_bound (mkp (0.0, 0.0), 2.0) - 1.0) < 1e-9);
  CHECK (fabs (f->inverse_bound (mkp (0.0, 0.0), 2.0) - 4.0) < 1e-9);
}

TEST_MEMORY_LEAK_INIT
TEST_MEMORY_LEAK_ALL
