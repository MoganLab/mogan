
/******************************************************************************
 * MODULE     : curve.hpp
 * DESCRIPTION: mathematical curves
 * COPYRIGHT  : (C) 2003  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef CURVE_H
#define CURVE_H
#include "frame.hpp"
#include "point.hpp"
#ifdef QTTEXMACS
#include <QPainterPath>
#endif

class curve_rep : public abstract_struct {
public:
  inline curve_rep () {}
  virtual ~curve_rep () {}

  virtual int nr_components () { return 1; }
  // the number of components of the curve is useful for getting
  // nice parameterizations when concatenating curves

  virtual point evaluate (double t)= 0;
  // gives a point on the curve for its intrinsic parameterization
  // curves are parameterized from 0.0 to 1.0

  array<point> rectify (double eps);
  // returns a rectification of the curve, which, modulo reparameterization
  // has a uniform distance of at most 'eps' to the original curve

  virtual void rectify_cumul (array<point>& a, double eps)= 0;
  // add rectification of the curve  (except for the starting point)
  // to an existing polysegment

  /*
  NOTE: more routines should be added later so that one
  can reliably compute the intersections between curves
  One might for instance take the following:
  */
  virtual double bound (double t, double eps)= 0;
  // return delta such that |t' - t| < delta => |c(t') - c(t)| < eps.

  virtual point grad (double t, bool& error)= 0;
  // compute the first derivative at t.
  // set error= true if this derivative does not exist.

  virtual double curvature (double t1, double t2)= 0;
  // compute a bound for the second derivative between t1 and t2.
  /* FIXME: What is computed is *really* a bound for the curvature,
       not for the norm of the second derivative. Make it precise
       what it is that is computed exactly. */
  // return a very large number if such a bound does not exist.

  // returns the number of control points which belong to the curve.
  // these control points are ordered and come first in pts & cips.
  virtual int get_control_points (array<double>& abs, array<point>& pts,
                                  array<path>& cip);

  virtual array<double> find_closest_points (double t1, double t2, point p,
                                             double eps);

  virtual double find_closest_point (double t1, double t2, point p, double eps,
                                     bool& found);
};

class curve {
  ABSTRACT_NULL (curve);
  inline point operator() (double t) { return rep->evaluate (t); }
  inline bool  operator== (curve c) { return rep == c.rep; }
  inline bool  operator!= (curve c) { return rep != c.rep; }
  curve_rep*   get_rep () const { return rep; }
};
ABSTRACT_NULL_CODE (curve);

struct transformed_curve_rep : public curve_rep {
  frame f;
  curve c;
  int   n;
  transformed_curve_rep (frame f2, curve c2)
      : f (f2), c (c2), n (c->nr_components ()) {}
  int    nr_components () { return n; }
  point  evaluate (double t) { return f (c (t)); }
  void   rectify_cumul (array<point>& a, double eps);
  double bound (double t, double eps) { return curve_rep::bound (t, eps); }
  point  grad (double t, bool& error);
  double curvature (double t1, double t2) {
    (void) t1;
    (void) t2;
    TM_FAILED ("not yet implemented");
    return 0.0;
  }
  int get_control_points (array<double>& abs, array<point>& pts,
                          array<path>& cip);
};

struct ellipse_rep : public curve_rep {
  array<point> points;
  array<path>  cip;
  point        f1, f2;       // two foci of the ellipsis
  point        center;       // the center of the ellipse
  double       focal_length; // the distance between f1 and f2
  double sum_of_two_dis; // The sum of the distances of any point on the ellipse
                         // to f1 and f2
  point  i, j;           // The two base vectors of the ellipsis's 2D plane
  double r1, r2;         // The two radiuses of the ellipsis
  ellipse_rep (array<point> a, array<path> cip, bool close);
  point  evaluate (double t) override;
  void   rectify_cumul (array<point>& cum, double eps) override;
  double bound (double t, double eps) override;
  point  grad (double t, bool& error) override;
  double curvature (double t1, double t2) override;
  int    get_control_points (array<double>& abs, array<point>& pts,
                             array<path>& cip) override;
};

curve segment (point p1, point p2);
curve poly_segment (array<point> a, array<path> cip);
curve spline (array<point> a, array<path> cip, bool close= false,
              bool interpol= true);
curve bezier (array<point> a);
curve poly_bezier (array<point> a, array<path> cip, bool simple, bool closed);
curve arc (array<point> a, array<path> cip, bool close= false);
curve ellipse (array<point> a, array<path> cip, bool close= true);
curve compound (array<curve> cs);
curve invert (curve c);
curve part (curve c, double start, double end);
curve truncate (curve c, double t0, double eps);
curve recontrol (curve c, array<point> a, array<path> cip);

array<point> intersection (curve f, curve g, point p0, double eps);
point        closest (curve f, point p);

array<point> simplify_polyline (array<point> a, double eps);
array<point> std_bezier_fit (array<point> a, int pack_size);
array<point> alt_bezier_fit (array<point> a, int pack_size);
array<point> bezier_fit (array<point> a, double eps, double advance= 1.0);
array<point> rectify_bezier (array<point> bez, double eps);
array<point> refine (array<point> a, int factor);
array<point> smoothen (array<point> a, int width);
array<point> oval_profile (double rx, double ry, double a, int nr);
array<point> calligraphy (array<point> a, array<point> pen);

#endif // defined CURVE_H
