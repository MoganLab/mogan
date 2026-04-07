/* s7_scheme_complex.c - complex number implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_complex.h"
#include <math.h>

#ifndef M_PI
  #define M_PI 3.1415926535897932384626433832795029L
#endif

static s7_pointer make_complex_not_0i(s7_scheme *sc, s7_double r, s7_double i)
{
  if (i == 0.0) return s7_make_real(sc, r);
  return s7_make_complex(sc, r, i);
}

static double my_hypot(double x, double y)
{
  if ((fabs(x) < 1.0e6) && (fabs(y) < 1.0e6))
    return sqrt(x * x + y * y);
  return hypot(x, y);
}

s7_pointer magnitude_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int v = s7_integer(x);
      return (v < 0) ? s7_make_integer(sc, -v) : x;
    }

  if (s7_is_rational(x))
    {
      s7_int n = s7_numerator(x);
      s7_int d = s7_denominator(x);
      return (n < 0) ? s7_make_ratio(sc, -n, d) : x;
    }

  if (s7_is_real(x))
    {
      s7_double r = s7_real(x);
      if (isnan(r)) return x;
      return signbit(r) ? s7_make_real(sc, -r) : x;
    }
  
  if (s7_is_complex(x))
    return s7_make_real(sc, my_hypot(s7_real_part(x), s7_imag_part(x)));

  return s7_wrong_type_arg_error(sc, "magnitude", 1, x, "a number");
}

s7_pointer g_magnitude(s7_scheme *sc, s7_pointer args)
{
  return magnitude_p_p(sc, s7_car(args));
}

s7_int magnitude_i_i(s7_int x) {return((x < 0) ? (-x) : x);} 
s7_double magnitude_d_d(s7_double x) {return((signbit(x)) ? (-x) : x);} 
s7_pointer magnitude_p_z(s7_scheme *sc, s7_pointer z) {return(s7_make_real(sc, my_hypot(s7_real_part(z), s7_imag_part(z))));}

s7_pointer g_angle(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args);

  if (s7_is_integer(x)) return (s7_integer(x) < 0) ? s7_make_real(sc, M_PI) : s7_make_integer(sc, 0);
  if (s7_is_rational(x)) return (s7_numerator(x) < 0) ? s7_make_real(sc, M_PI) : s7_make_integer(sc, 0);
  if (s7_is_complex(x)) return s7_make_real(sc, atan2(s7_imag_part(x), s7_real_part(x)));
  if (s7_is_real(x))
    {
      s7_double r = s7_real(x);
      if (isnan(r)) return x;
      return (r < 0.0) ? s7_make_real(sc, M_PI) : s7_make_real(sc, 0.0);
    }

  return s7_wrong_type_arg_error(sc, "angle", 1, x, "a number");
}

s7_double angle_d_d(s7_double x) {return((isnan(x)) ? x : ((x < 0.0) ? M_PI : 0.0));}

s7_pointer complex_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y)
{
  if (!s7_is_real(x))
    return s7_wrong_type_arg_error(sc, "complex", 1, x, "a real");
  if (!s7_is_real(y))
    return s7_wrong_type_arg_error(sc, "complex", 2, y, "a real");

  if ((s7_is_integer(y) && (s7_integer(y) == 0)) || (s7_is_real(y) && (s7_real(y) == 0.0)))
    return x;

  return make_complex_not_0i(sc, s7_number_to_real(sc, x), s7_number_to_real(sc, y));
}

s7_pointer complex_p_pp_wrapped(s7_scheme *sc, s7_pointer x, s7_pointer y) {return complex_p_pp(sc, x, y);} 
s7_pointer g_complex(s7_scheme *sc, s7_pointer args) {return complex_p_pp(sc, s7_car(args), s7_cadr(args));}
s7_pointer g_complex_wrapped(s7_scheme *sc, s7_pointer args) {return complex_p_pp_wrapped(sc, s7_car(args), s7_cadr(args));}

s7_pointer complex_p_ii_wrapped(s7_scheme *sc, s7_int x, s7_int y) {return make_complex_not_0i(sc, (s7_double)x, (s7_double)y);} 
s7_pointer complex_p_dd_wrapped(s7_scheme *sc, s7_double x, s7_double y) {return make_complex_not_0i(sc, x, y);} 

s7_pointer complex_p_ii(s7_scheme *sc, s7_int x, s7_int y)
{
  return((y == 0) ? s7_make_integer(sc, x) : make_complex_not_0i(sc, (s7_double)x, (s7_double)y));
}

s7_pointer complex_p_dd(s7_scheme *sc, s7_double x, s7_double y)
{
  return((y == 0.0) ? s7_make_real(sc, x) : make_complex_not_0i(sc, x, y));
}

s7_pointer g_make_polar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer mag = s7_car(args), ang = s7_cadr(args);
  if (!s7_is_real(mag))
    return s7_wrong_type_arg_error(sc, "make-polar", 1, mag, "a real");
  if (!s7_is_real(ang))
    return s7_wrong_type_arg_error(sc, "make-polar", 2, ang, "a real");

  s7_double m = s7_number_to_real(sc, mag);
  s7_double a = s7_number_to_real(sc, ang);
  return make_complex_not_0i(sc, m * cos(a), m * sin(a));
}

s7_double real_part_d_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_number(x)) return s7_real_part(x);
  s7_wrong_type_arg_error(sc, "real-part", 1, x, "a number");
  return 0.0;
}

s7_pointer g_real_part(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args);
  if (!s7_is_number(x))
    return s7_wrong_type_arg_error(sc, "real-part", 1, x, "a number");
  return real_part_p_p(sc, x);
}

s7_double imag_part_d_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_number(x)) return s7_imag_part(x);
  s7_wrong_type_arg_error(sc, "imag-part", 1, x, "a number");
  return 0.0;
}

s7_pointer real_part_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x) || s7_is_rational(x) || s7_is_real(x))
    return x;
  if (s7_is_complex(x))
    return s7_make_real(sc, s7_real_part(x));
  return s7_wrong_type_arg_error(sc, "real-part", 1, x, "a number");
}

s7_pointer imag_part_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x) || s7_is_rational(x))
    return s7_make_integer(sc, 0);
  if (s7_is_real(x))
    return s7_make_real(sc, 0.0);
  if (s7_is_complex(x))
    return s7_make_real(sc, s7_imag_part(x));
  return s7_wrong_type_arg_error(sc, "imag-part", 1, x, "a number");
}

s7_pointer g_imag_part(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args);
  return imag_part_p_p(sc, x);
}
