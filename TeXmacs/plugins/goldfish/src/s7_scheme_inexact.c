/* s7_scheme_inexact.c - inexact number implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifdef _MSC_VER
  #ifndef HAVE_COMPLEX_NUMBERS
    #define HAVE_COMPLEX_NUMBERS 0
  #endif
#else
  #ifndef HAVE_COMPLEX_NUMBERS
    #if __TINYC__ || (__clang__ && __cplusplus)
      #define HAVE_COMPLEX_NUMBERS 0
    #else
      #define HAVE_COMPLEX_NUMBERS 1
    #endif
  #endif
#endif

#ifndef M_PI
  #define M_PI 3.1415926535897932384626433832795029L
#endif

#if HAVE_COMPLEX_NUMBERS
  #if WITH_CLANG_PP
    #define s7_complex_i ((double)1.0i)
  #else
  #if (defined(__GNUC__))
    #define s7_complex_i 1.0i
  #else
    #define s7_complex_i (s7_complex)_Complex_I /* a float, but we want a double */
  #endif
  #endif
#else
  #define _Complex_I 1.0
  #define s7_complex_i 1.0
#endif

#include "s7_scheme_inexact.h"
#include "s7_internal_helpers.h"
#include <string.h>
#include <time.h>
#include <math.h>
#if HAVE_COMPLEX_NUMBERS
  #include <complex.h>
#endif

#ifndef S7_INT_BITS
#define S7_INT_BITS 63
#endif


/* -------------------------------- sqrt -------------------------------- */

/* Helper to create complex number with 0 imaginary part optimized */
static s7_pointer make_complex_not_0i(s7_scheme *sc, double r, double i)
{
  if (i == 0.0) return s7_make_real(sc, r);
  return s7_make_complex(sc, r, i);
}

s7_pointer sqrt_p_p(s7_scheme *sc, s7_pointer num)
{
  if (s7_is_integer(num))
    {
      s7_int iv = s7_integer(num);
      if (iv >= 0)
        {
          double sqx = sqrt((double)iv);
          s7_int ix = (s7_int)sqx;
          if ((ix * ix) == iv)
            return s7_make_integer(sc, ix);
          return s7_make_real(sc, sqx);
        }
#if HAVE_COMPLEX_NUMBERS
      return make_complex_not_0i(sc, 0.0, sqrt((double)(-iv)));
#else
      return s7_out_of_range_error(sc, "sqrt", 1, num, "no complex numbers");
#endif
    }

  if (s7_is_rational(num) && !s7_is_integer(num))
    {
      s7_int numr = s7_numerator(num);
      if (numr > 0)
        {
          s7_int nm = (s7_int)sqrt((double)numr);
          if (nm * nm == numr)
            {
              s7_int den = s7_denominator(num);
              s7_int dn = (s7_int)sqrt((double)den);
              if (dn * dn == den)
                return s7_make_ratio(sc, nm, dn);
            }
          double frac = (double)numr / (double)s7_denominator(num);
          return s7_make_real(sc, sqrt(frac));
        }
#if HAVE_COMPLEX_NUMBERS
      double frac = (double)numr / (double)s7_denominator(num);
      return s7_make_complex(sc, 0.0, sqrt(-frac));
#else
      return s7_out_of_range_error(sc, "sqrt", 1, num, "no complex numbers");
#endif
    }

  if (s7_is_real(num))
    {
      double rv = s7_real(num);
      if (is_NaN(rv)) return num;
      if (rv >= 0.0)
        return s7_make_real(sc, sqrt(rv));
      return make_complex_not_0i(sc, 0.0, sqrt(-rv));
    }

  if (s7_is_complex(num))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(num);
      double i = s7_imag_part(num);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = csqrt(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "sqrt", 1, num, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "sqrt", 1, num, "a number");
}

s7_pointer g_sqrt(s7_scheme *sc, s7_pointer args)
{
  #define H_sqrt "(sqrt z) returns the square root of z"
  #define Q_sqrt sc->pl_nn
  return(sqrt_p_p(sc, s7_car(args)));
}

/* ---------------------------------------- nan? ---------------------------------------- */
bool s7_is_nan(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_real(x))
    {
      if (s7_is_integer(x) || s7_is_rational(x))
        return false;
      return is_NaN(s7_real(x));
    }
  if (s7_is_complex(x))
    return is_NaN(s7_real_part(x)) || is_NaN(s7_imag_part(x));
  return false;
}

s7_pointer g_is_nan(s7_scheme *sc, s7_pointer args)
{
  #define H_is_nan "(nan? obj) returns #t if obj is a NaN"
  #define Q_is_nan sc->pl_bt
  return s7_make_boolean(sc, s7_is_nan(sc, s7_car(args)));
}

/* ---------------------------------------- infinite? ---------------------------------------- */
bool s7_is_infinite(s7_scheme *sc, s7_pointer x)
{
  (void)sc;

  if (s7_is_real(x))
    {
      if (s7_is_integer(x) || s7_is_rational(x))
        return false;
      return isinf(s7_real(x));
    }
  if (s7_is_complex(x))
    return isinf(s7_real_part(x)) || isinf(s7_imag_part(x));
  return false;
}

s7_pointer g_is_infinite(s7_scheme *sc, s7_pointer args)
{
  #define H_is_infinite "(infinite? obj) returns #t if obj has an infinite real or imaginary part"
  #define Q_is_infinite sc->pl_bt
  return s7_make_boolean(sc, s7_is_infinite(sc, s7_car(args)));
}

/* -------------------------------- sin -------------------------------- */
#define SIN_LIMIT 1.0e16
#define SINH_LIMIT 20.0

s7_pointer sin_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0);
      return s7_make_real(sc, sin((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, sin(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, sin(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = csin(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "sin", 1, x, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "sin", 1, x, "a number");
}

s7_pointer g_sin(s7_scheme *sc, s7_pointer args)
{
  #define H_sin "(sin z) returns sin(z)"
  #define Q_sin sc->pl_nn
  return(sin_p_p(sc, s7_car(args)));
}

s7_pointer sin_p_d(s7_scheme *sc, s7_double x)
{
  return(s7_make_real(sc, sin(x)));
}

s7_double sin_d_d(s7_double x)
{
  return(sin(x));
}

/* -------------------------------- cos -------------------------------- */

s7_pointer cos_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 1); /* (cos 0) -> 1 */
      return s7_make_real(sc, cos((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, cos(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, cos(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = ccos(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "cos", 1, x, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "cos", 1, x, "a number");
}

s7_pointer g_cos(s7_scheme *sc, s7_pointer args)
{
  #define H_cos "(cos z) returns cos(z)"
  #define Q_cos sc->pl_nn
  return(cos_p_p(sc, s7_car(args)));
}

s7_pointer cos_p_d(s7_scheme *sc, s7_double x)
{
  return(s7_make_real(sc, cos(x)));
}

s7_double cos_d_d(s7_double x)
{
  return(cos(x));
}

/* -------------------------------- tan -------------------------------- */
#define TAN_LIMIT 1.0e18

s7_pointer tan_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0); /* (tan 0) -> 0 */
      return s7_make_real(sc, tan((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, tan(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, tan(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      if (i > 350.0)
        return s7_make_complex(sc, 0.0, 1.0);
      if (i < -350.0)
        return s7_make_complex(sc, 0.0, -1.0);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = ctan(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "tan", 1, x, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "tan", 1, x, "a number");
}

s7_pointer g_tan(s7_scheme *sc, s7_pointer args)
{
  #define H_tan "(tan z) returns tan(z)"
  #define Q_tan sc->pl_nn
  return(tan_p_p(sc, s7_car(args)));
}

s7_double tan_d_d(s7_double x)
{
  return(tan(x));
}

/* -------------------------------- exp -------------------------------- */

s7_pointer exp_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 1); /* (exp 0) -> 1 */
      return s7_make_real(sc, exp((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, exp(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, exp(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = cexp(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "exp", 1, x, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "exp", 1, x, "a number");
}

s7_pointer g_exp(s7_scheme *sc, s7_pointer args)
{
  #define H_exp "(exp z) returns e^z, (exp 1) is 2.718281828459"
  #define Q_exp sc->pl_nn
  return(exp_p_p(sc, s7_car(args)));
}

s7_pointer exp_p_d(s7_scheme *sc, s7_double x)
{
  return(s7_make_real(sc, exp(x)));
}

s7_double exp_d_d(s7_double x)
{
  return(exp(x));
}

/* -------------------------------- log -------------------------------- */
#if __cplusplus
  #define LOG_2 1.4426950408889634074
#else
  #define LOG_2 1.4426950408889634073599246810018921L /* (/ (log 2.0)) */
#endif

#define LOG_RATIONALIZE_LIMIT 1.0e12
#define LOG_RATIONALIZE_ERROR 1.0e-12

static s7_double log_round(s7_double number)
{
  return ((number < 0.0) ? ceil(number - 0.5) : floor(number + 0.5));
}

static bool log_is_positive_real(s7_pointer x)
{
  return (s7_is_real(x) && (s7_real(x) > 0.0));
}

static bool log_is_zero_number(s7_pointer x)
{
  if (s7_is_real(x))
    return (s7_real(x) == 0.0);
  if (s7_is_complex(x))
    return ((s7_real_part(x) == 0.0) && (s7_imag_part(x) == 0.0));
  return false;
}

static bool log_is_one_number(s7_pointer x)
{
  if (s7_is_real(x))
    return (s7_real(x) == 1.0);
  if (s7_is_complex(x))
    return ((s7_real_part(x) == 1.0) && (s7_imag_part(x) == 0.0));
  return false;
}

static s7_complex log_to_c_complex(s7_pointer x)
{
  if (s7_is_complex(x))
    return s7_real_part(x) + s7_imag_part(x) * _Complex_I;
  return s7_real(x) + 0.0 * _Complex_I;
}

static s7_pointer log_from_c_complex(s7_scheme *sc, s7_complex z)
{
  return make_complex_not_0i(sc, creal(z), cimag(z));
}

s7_pointer g_log(s7_scheme *sc, s7_pointer args)
{
  #define H_log "(log z1 (z2 e)) returns log(z1) / log(z2) where z2 (the base) defaults to e: (log 8 2) = 3"
  #define Q_log sc->pcl_n

  const s7_pointer x = s7_car(args);

  if (!s7_is_number(x))
    return s7_wrong_type_arg_error(sc, "log", 1, x, "a number");

  if (s7_is_pair(s7_cdr(args)))
    {
      const s7_pointer y = s7_cadr(args);
      if (!s7_is_number(y))
        return s7_wrong_type_arg_error(sc, "log", 2, y, "a number");

      if ((s7_is_integer(y)) && (s7_integer(y) == 2))
        {
          if (s7_is_integer(x))
            {
              s7_int ix = s7_integer(x);
              if (ix > 0)
                {
                  s7_double fx;
#if (__ANDROID__) || (MS_WINDOWS)
                  fx = log((double)ix) * LOG_2;
#else
                  fx = log2((double)ix);
#endif
                  return(((ix & (ix - 1)) == 0) ? s7_make_integer(sc, (s7_int)log_round(fx)) : s7_make_real(sc, fx));
                }
            }
          if (log_is_positive_real(x))
            return s7_make_real(sc, log(s7_real(x)) * LOG_2);
          return log_from_c_complex(sc, clog(log_to_c_complex(x)) * LOG_2);
        }

      if ((s7_is_integer(x)) && (s7_integer(x) == 1) && (s7_is_integer(y)) && (s7_integer(y) == 1))
        return s7_make_integer(sc, 0);

      if (log_is_zero_number(y))
        {
          if ((s7_is_integer(y)) && (s7_is_integer(x)) && (s7_integer(x) == 1))
            return y;
          return s7_out_of_range_error(sc, "log", 2, y, "can't be zero");
        }

      if ((s7_is_real(x)) && (is_NaN(s7_real(x))))
        return x;
      if (log_is_one_number(y))
        return (log_is_one_number(x)) ? s7_make_real(sc, 0.0) : s7_make_real(sc, INFINITY);

      if ((log_is_positive_real(x)) && (log_is_positive_real(y)))
        {
          if ((s7_is_rational(x)) && (s7_is_rational(y)))
            {
              const s7_double result = log(s7_real(x)) / log(s7_real(y));
              const s7_int ires = (s7_int)result;
              if (result - ires == 0.0)
                return s7_make_integer(sc, ires);
              if (fabs(result) < LOG_RATIONALIZE_LIMIT)
                {
                  s7_pointer rat = s7_rationalize(sc, result, LOG_RATIONALIZE_ERROR);
                  if (rat != s7_f(sc))
                    return rat;
                }
              return s7_make_real(sc, result);
            }
          return s7_make_real(sc, log(s7_real(x)) / log(s7_real(y)));
        }

      if ((s7_is_real(x)) && (is_NaN(s7_real(x))))
        return x;
      if ((s7_is_complex(y)) && ((is_NaN(s7_real_part(y))) || (is_NaN(s7_imag_part(y)))))
        return y;
      return log_from_c_complex(sc, clog(log_to_c_complex(x)) / clog(log_to_c_complex(y)));
    }

  if (!s7_is_real(x))
    return log_from_c_complex(sc, clog(log_to_c_complex(x)));
  if (log_is_positive_real(x))
    return s7_make_real(sc, log(s7_real(x)));
  return make_complex_not_0i(sc, log(-s7_real(x)), M_PI);
}

/* -------------------------------- asin -------------------------------- */

static s7_pointer c_asin(s7_scheme *sc, s7_double x)
{
  s7_double absx = fabs(x);

  if (absx <= 1.0) return s7_make_real(sc, asin(x));

#if HAVE_COMPLEX_NUMBERS
  /* otherwise use maxima code: */
  s7_double recip = 1.0 / absx;
  s7_complex result = (M_PI / 2.0) - (s7_complex_i * clog(absx * (1.0 + (sqrt(1.0 + recip) * csqrt(1.0 - recip)))));
  return s7_make_complex(sc, (x < 0.0) ? -creal(result) : creal(result), (x < 0.0) ? -cimag(result) : cimag(result));
#else
  /* without complex numbers, we can't handle |x| > 1 */
  return s7_out_of_range_error(sc, "asin", 1, s7_make_real(sc, x), "no complex numbers");
#endif
}

s7_pointer asin_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0);                    /* (asin 0) -> 0 */
      /* in netBSD, (asin 2) returns 0.25383842987008+0.25383842987008i according to Peter Bex */
      return c_asin(sc, (s7_double)iv);
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return c_asin(sc, frac);
    }

  if (s7_is_real(x))
    {
      return c_asin(sc, s7_real(x));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      /* if either real or imag part is very large, use explicit formula, not casin */
      /*   this code taken from sbcl's src/code/irrat.lisp; break is around x+70000000i */
      if ((fabs(r) > 1.0e7) || (fabs(i) > 1.0e7))
        {
          s7_complex sq1mz, sq1pz, z = r + i * _Complex_I;
          sq1mz = csqrt(1.0 - z);
          sq1pz = csqrt(1.0 + z);
          return s7_make_complex(sc, atan(r / creal(sq1mz * sq1pz)), asinh(cimag(sq1pz * conj(sq1mz))));
        }
      s7_complex z = r + i * _Complex_I;
      s7_complex result = casin(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "asin", 1, x, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "asin", 1, x, "a number");
}

s7_pointer g_asin(s7_scheme *sc, s7_pointer args)
{
  #define H_asin "(asin z) returns asin(z); (sin (asin x)) = x"
  #define Q_asin sc->pl_nn
  return asin_p_p(sc, s7_car(args));
}

s7_pointer asin_p_d(s7_scheme *sc, s7_double x)
{
  return c_asin(sc, x);
}

s7_double asin_d_d(s7_double x)
{
  return asin(x);
}

/* -------------------------------- acos -------------------------------- */

static s7_pointer c_acos(s7_scheme *sc, s7_double x)
{
  s7_double absx = fabs(x);
  s7_double recip;

  if (absx <= 1.0) return s7_make_real(sc, acos(x));

#if HAVE_COMPLEX_NUMBERS
  /* else follow maxima again: */
  recip = 1.0 / absx;
  if (x > 0.0)
    {
      s7_complex result = s7_complex_i * clog(absx * (1.0 + (sqrt(1.0 + recip) * csqrt(1.0 - recip))));
      return s7_make_complex(sc, creal(result), cimag(result));
    }
  else
    {
      s7_complex result = M_PI - s7_complex_i * clog(absx * (1.0 + (sqrt(1.0 + recip) * csqrt(1.0 - recip))));
      return s7_make_complex(sc, creal(result), cimag(result));
    }
#else
  /* without complex numbers, we can't handle |x| > 1 */
  return s7_out_of_range_error(sc, "acos", 1, s7_make_real(sc, x), "no complex numbers");
#endif
}

s7_pointer acos_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 1) return s7_make_integer(sc, 0);                    /* (acos 1) -> 0 */
      return c_acos(sc, (s7_double)iv);
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return c_acos(sc, frac);
    }

  if (s7_is_real(x))
    {
      return c_acos(sc, s7_real(x));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      /* if either real or imag part is very large, use explicit formula, not cacos */
      /*   this code taken from sbcl's src/code/irrat.lisp; break is around x+70000000i */
      if ((fabs(r) > 1.0e7) || (fabs(i) > 1.0e7))
        {
          s7_complex sq1mz, sq1pz, z = r + i * _Complex_I;
          sq1mz = csqrt(1.0 - z);
          sq1pz = csqrt(1.0 + z);
          /* creal(sq1pz) can be 0.0 */
          if (creal(sq1pz) == 0.0)        /* so the atan arg will be inf, so the real part will be pi/2(?) */
            return s7_make_complex(sc, M_PI / 2.0, asinh(cimag(sq1mz * conj(sq1pz))));
          return s7_make_complex(sc, 2.0 * atan(creal(sq1mz) / creal(sq1pz)), asinh(cimag(sq1mz * conj(sq1pz))));
        }
      s7_complex z = r + i * _Complex_I;
      s7_complex result = cacos(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "acos", 1, x, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "acos", 1, x, "a number");
}

s7_pointer g_acos(s7_scheme *sc, s7_pointer args)
{
  #define H_acos "(acos z) returns acos(z); (cos (acos 1)) = 1"
  #define Q_acos sc->pl_nn
  return acos_p_p(sc, s7_car(args));
}

s7_pointer acos_p_d(s7_scheme *sc, s7_double x)
{
  return c_acos(sc, x);
}

s7_double acos_d_d(s7_double x)
{
  return acos(x);
}

/* -------------------------------- atan -------------------------------- */
/* Helper function for single argument atan */
static s7_pointer c_atan(s7_scheme *sc, s7_double x)
{
  if (x == 0.0) return s7_make_integer(sc, 0);
  return s7_make_real(sc, atan(x));
}

s7_pointer atan_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0);
      return s7_make_real(sc, atan((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, atan(frac));
    }

  if (s7_is_real(x))
    {
      return c_atan(sc, s7_real(x));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = catan(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "atan", 1, x, "no complex numbers");
#endif
    }

  return s7_wrong_type_arg_error(sc, "atan", 1, x, "a number");
}

s7_pointer g_atan(s7_scheme *sc, s7_pointer args)
{
  #define H_atan "(atan z) returns atan(z), (atan y x) returns atan(y/x)"
  #define Q_atan s7_make_signature(sc, 3, sc->is_number_symbol, sc->is_number_symbol, sc->is_real_symbol)
  /* actually if there are two args, both should be real, but how to express that in the signature? */

  const s7_pointer x = s7_car(args);
  s7_pointer y;

  if (!s7_is_pair(s7_cdr(args)))
    {
      return atan_p_p(sc, x);
    }

  y = s7_cadr(args);
  /* this is one place where s7 notices -0.0 != 0.0 -- this is apparently built into atan2, so I guess I'll leave it, but:
   *   (atan 0.0 0.0): 0.0, (atan 0.0 -0.0): pi, (atan 0 -0.0): pi, (atan 0 -0) 0.0, (atan 0 -0.0): pi.
   *   so you can sneak up on 0.0 from the left, but you can't fool 0??
   */

  /* Check if both arguments are real numbers */
  if (!s7_is_real(x))
    return s7_wrong_type_arg_error(sc, "atan", 1, x, "a real number");
  if (!s7_is_real(y))
    return s7_wrong_type_arg_error(sc, "atan", 2, y, "a real number");

  return s7_make_real(sc, atan2(s7_real(x), s7_real(y)));
}

s7_pointer atan_p_d(s7_scheme *sc, s7_double x)
{
  return c_atan(sc, x);
}

s7_double atan_d_d(s7_double x)
{
  return atan(x);
}

s7_double atan_d_dd(s7_double x, s7_double y)
{
  return atan2(x, y);
}

/* -------------------------------- sinh -------------------------------- */
s7_pointer sinh_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0);                    /* (sinh 0) -> 0 */
      return s7_make_real(sc, sinh((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, sinh(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, sinh(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = csinh(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "sinh", 1, x, "no complex numbers");
#endif
    }

  return s7i_method_or_bust_p(sc, x, "sinh", "a number");
}

s7_pointer g_sinh(s7_scheme *sc, s7_pointer args)
{
  #define H_sinh "(sinh z) returns sinh(z)"
  #define Q_sinh sc->pl_nn
  return(sinh_p_p(sc, s7_car(args)));
}

s7_double sinh_d_d(s7_double x) {return(sinh(x));}
s7_pointer sinh_p_d(s7_scheme *sc, s7_double x) {return(s7_make_real(sc, sinh(x)));}


/* -------------------------------- cosh -------------------------------- */
s7_pointer cosh_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 1);                   /* (cosh 0) -> 1 */
      return s7_make_real(sc, cosh((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, cosh(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, cosh(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      s7_complex z = r + i * _Complex_I;
      s7_complex result = ccosh(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "cosh", 1, x, "no complex numbers");
#endif
    }

  return s7i_method_or_bust_p(sc, x, "cosh", "a number");
}

s7_pointer g_cosh(s7_scheme *sc, s7_pointer args)
{
  #define H_cosh "(cosh z) returns cosh(z)"
  #define Q_cosh sc->pl_nn
  return(cosh_p_p(sc, s7_car(args)));
}

s7_double cosh_d_d(s7_double x) {return(cosh(x));}
s7_pointer cosh_p_d(s7_scheme *sc, s7_double x) {return(s7_make_real(sc, cosh(x)));}


/* -------------------------------- tanh -------------------------------- */
#define TANH_LIMIT 350.0

s7_pointer tanh_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0);
      return s7_make_real(sc, tanh((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, tanh(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, tanh(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      if (r > TANH_LIMIT)
        return s7_make_real(sc, 1.0);                         /* closer than 0.0 which is what ctanh is about to return! */
      if (r < -TANH_LIMIT)
        return s7_make_real(sc, -1.0);              /* closer than ctanh's -0.0 */
      s7_complex z = r + i * _Complex_I;
      s7_complex result = ctanh(z);
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "tanh", 1, x, "no complex numbers");
#endif
    }

  return s7i_method_or_bust_p(sc, x, "tanh", "a number");
}

s7_pointer g_tanh(s7_scheme *sc, s7_pointer args)
{
  #define H_tanh "(tanh z) returns tanh(z)"
  #define Q_tanh sc->pl_nn
  return(tanh_p_p(sc, s7_car(args)));
}

s7_double tanh_d_d(s7_double x) {return(tanh(x));}


/* -------------------------------- asinh -------------------------------- */
s7_pointer asinh_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0);
      return s7_make_real(sc, asinh((double)iv));
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      return s7_make_real(sc, asinh(frac));
    }

  if (s7_is_real(x))
    {
      return s7_make_real(sc, asinh(s7_real(x)));
    }

  if (s7_is_complex(x))
    {
#if HAVE_COMPLEX_NUMBERS
      double r = s7_real_part(x);
      double i = s7_imag_part(x);
      s7_complex z = r + i * _Complex_I;
#if (defined(__OpenBSD__)) || (defined(__NetBSD__))
      s7_complex result = casinh_1(z);
#else
      s7_complex result = casinh(z);
#endif
      return s7_make_complex(sc, creal(result), cimag(result));
#else
      return s7_out_of_range_error(sc, "asinh", 1, x, "no complex numbers");
#endif
    }

  return s7i_method_or_bust_p(sc, x, "asinh", "a number");
}

s7_pointer g_asinh(s7_scheme *sc, s7_pointer args)
{
  #define H_asinh "(asinh z) returns asinh(z)"
  #define Q_asinh sc->pl_nn
  return(asinh_p_p(sc, s7_car(args)));
}


/* -------------------------------- acosh -------------------------------- */
s7_pointer acosh_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 1) return s7_make_integer(sc, 0);
      if (iv >= 1)
        return s7_make_real(sc, acosh((double)iv));
    }
  else if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      if (frac >= 1.0)
        return s7_make_real(sc, acosh(frac));
    }
  else if (s7_is_real(x))
    {
      double rv = s7_real(x);
      if (rv >= 1.0)
        return s7_make_real(sc, acosh(rv));
    }
  else if (!s7_is_complex(x))
    {
      return s7i_method_or_bust_p(sc, x, "acosh", "a number");
    }

#if HAVE_COMPLEX_NUMBERS
#ifdef __OpenBSD__
  {
    s7_complex z = s7_real_part(x) + s7_imag_part(x) * _Complex_I;
    s7_complex result = cacosh_1(z);
    return s7_make_complex(sc, creal(result), cimag(result));
  }
#else
  {
    s7_complex z = s7_real_part(x) + s7_imag_part(x) * _Complex_I;
    s7_complex result = cacosh(z);
    return s7_make_complex(sc, creal(result), cimag(result));
  }
#endif
#else
  return s7_out_of_range_error(sc, "acosh", 1, x, "no complex numbers");
#endif
}

s7_pointer g_acosh(s7_scheme *sc, s7_pointer args)
{
  #define H_acosh "(acosh z) returns acosh(z)"
  #define Q_acosh sc->pl_nn
  return(acosh_p_p(sc, s7_car(args)));
}


/* -------------------------------- atanh -------------------------------- */
s7_pointer atanh_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int iv = s7_integer(x);
      if (iv == 0) return s7_make_integer(sc, 0);                    /* (atanh 0) -> 0 */
    }
  else if (s7_is_rational(x) && !s7_is_integer(x))
    {
      double frac = (double)s7_numerator(x) / (double)s7_denominator(x);
      if (fabs(frac) < 1.0)
        return s7_make_real(sc, atanh(frac));
    }
  else if (s7_is_real(x))
    {
      double rv = s7_real(x);
      if (fabs(rv) < 1.0)
        return s7_make_real(sc, atanh(rv));
    }
  else if (!s7_is_complex(x))
    {
      return s7i_method_or_bust_p(sc, x, "atanh", "a number");
    }

#if HAVE_COMPLEX_NUMBERS
  {
    s7_complex z = s7_real_part(x) + s7_imag_part(x) * _Complex_I;
#if (defined(__OpenBSD__)) || (defined(__NetBSD__))
    s7_complex result = catanh_1(z);
#else
    s7_complex result = catanh(z);
#endif
    return s7_make_complex(sc, creal(result), cimag(result));
  }
#else
  return s7_out_of_range_error(sc, "atanh", 1, x, "no complex numbers");
#endif
}

s7_pointer g_atanh(s7_scheme *sc, s7_pointer args)
{
  #define H_atanh "(atanh z) returns atanh(z)"
  #define Q_atanh sc->pl_nn
  return(atanh_p_p(sc, s7_car(args)));
}


/* -------------------------------- expt -------------------------------- */

static bool expt_is_zero(s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x) == 0;
  if (s7_is_real(x))
    return s7_real(x) == 0.0;
  return false;
}

static bool expt_is_one(s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x) == 1;
  if (s7_is_real(x))
    return s7_real(x) == 1.0;
  return false;
}

static bool expt_is_negative(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x) < 0;
  if (s7_is_rational(x) && !s7_is_integer(x))
    return s7_numerator(x) < 0;
  if (s7_is_real(x))
    return s7_real(x) < 0.0;
  s7_wrong_type_arg_error(sc, "expt", 2, x, "a real number");
  return false;
}

static s7_int int_to_int(s7_int x, s7_int n)
{
  /* from GSL */
  s7_int value = 1;
  do {
    if (n & 1) value *= x;
    n >>= 1;
    if (multiply_overflow(x, x, &x))
      break;
  } while (n);
  return(value);
}

static const s7_int nth_roots[63] = {
  S7_INT64_MAX, S7_INT64_MAX, 3037000499LL, 2097151, 55108, 6208, 1448, 511, 234, 127, 78, 52, 38, 28, 22,
  18, 15, 13, 11, 9, 8, 7, 7, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

static bool int_pow_ok(s7_int x, s7_int y) {return((y < S7_INT_BITS) && (nth_roots[y] >= s7_int_abs(x)));}

s7_pointer expt_p_pp(s7_scheme *sc, s7_pointer n, s7_pointer pw)
{
  if (!s7_is_number(n))
    return(s7i_method_or_bust_pp(sc, n, "expt", n, pw, "a number", 1));
  if (!s7_is_number(pw))
    return(s7i_method_or_bust_pp(sc, pw, "expt", n, pw, "a number", 2));

  if (expt_is_zero(n))
    {
      if (expt_is_zero(pw))
        {
          if ((s7_is_integer(n)) && (s7_is_integer(pw)))       /* (expt 0 0) -> 1 */
            return(s7_make_integer(sc, 1));
          return(s7_make_real(sc, 0.0));                                   /* (expt 0.0 0) -> 0.0 */
        }
      if (s7_is_real(pw))
        {
          if (expt_is_negative(sc, pw))                              /* (expt 0 -1) */
            s7i_division_by_zero_error(sc, "expt", n, pw);
          /* (Clisp gives divide-by-zero error here, Guile returns inf.0) */

          if (is_NaN(s7_real(pw)))                             /* (expt 0 +nan.0) */
            return(pw);
        }
      else
        {                                                      /* (expt 0 a+bi) */
          if (s7_real_part(pw) < 0.0)                             /* (expt 0 -1+i) */
            s7i_division_by_zero_error(sc, "expt", n, pw);
          if ((is_NaN(s7_real_part(pw))) ||                       /* (expt 0 0+1/0i) */
              (is_NaN(s7_imag_part(pw))))
            return(pw);
        }
      if ((s7_is_integer(n)) && (s7_is_integer(pw)))           /* pw != 0, (expt 0 2312) */
        return(s7_make_integer(sc, 0));
      return(s7_make_real(sc, 0.0));                                       /* (expt 0.0 123123) */
    }

  if (expt_is_one(pw))
    {
      if (s7_is_integer(pw))                                   /* (expt x 1) */
        return(n);
      if (s7_is_rational(n) && !s7_is_integer(n))                                      /* (expt ratio 1.0) */
        return(s7_make_real(sc, (double)s7_numerator(n) / (double)s7_denominator(n)));
      return(n);
    }
  if (s7_is_integer(pw))
    {
      const s7_int y = s7_integer(pw);
      if (y == 0)
        {
          if (s7_is_rational(n))                                 /* (expt 3 0) */
            return(s7_make_integer(sc, 1));
          if ((is_NaN(s7_real_part(n))) ||                    /* (expt 1/0 0) -> NaN */
              (is_NaN(s7_imag_part(n))))                      /* (expt (complex 0 1/0) 0) -> NaN */
            return(n);
          return(s7_make_real(sc, 1.0));                                   /* (expt 3.0 0) */
        }

      if (s7_is_integer(n))
        {
          const s7_int x = s7_integer(n);
          if (x == 1)                                       /* (expt 1 y) */
            return(n);

          if (x == -1)
            {
              if (y == S7_INT64_MIN)                        /* (expt -1 most-negative-fixnum) */
                return(s7_make_integer(sc, 1));
              if (y & 1)                                    /* (expt -1 odd-int) */
                return(n);                                  /*    n == -1 */
              return(s7_make_integer(sc, 1));                              /* (expt -1 even-int) */
            }

          if (y == S7_INT64_MIN)                            /* (expt x most-negative-fixnum) */
            return(s7_make_real(sc, 0.0));
          if (x == S7_INT64_MIN)                            /* (expt most-negative-fixnum y) */
            return(s7_make_real(sc, pow((double)x, (double)y)));

          if (int_pow_ok(x, s7_int_abs(y)))
            {
              if (y > 0)
                return(s7_make_integer(sc, int_to_int(x, y)));
              return(s7_make_ratio(sc, 1, int_to_int(x, -y)));
            }
        }

      if (s7_is_rational(n) && !s7_is_integer(n))
        {
          const s7_int nm = s7_numerator(n), dn = s7_denominator(n);
          if (y == S7_INT64_MIN)
            {
              if (s7_int_abs(nm) > dn)
                return(s7_make_real(sc, 0.0));                  /* (expt 4/3 most-negative-fixnum) -> 0? */
              return(s7_make_real(sc, INFINITY));               /* (expt 3/4 most-negative-fixnum) -> inf? */
            }
          if ((int_pow_ok(nm, s7_int_abs(y))) &&
              (int_pow_ok(dn, s7_int_abs(y))))
            {
              if (y > 0)
                return(s7_make_ratio(sc, int_to_int(nm, y), int_to_int(dn, y)));
              return(s7_make_ratio(sc, int_to_int(dn, -y), int_to_int(nm, -y)));
            }
        }

      if (s7_is_real(n))
        {
          /* (expt -1.0 most-positive-fixnum) should be -1.0
           * (expt -1.0 (+ (expt 2 53) 1)) -> -1.0
           * (expt -1.0 (- 1 (expt 2 54))) -> -1.0
           */
          if (s7_real(n) == -1.0)
            {
              if (y == S7_INT64_MIN)
                return(s7_make_real(sc, 1.0));
              return((s7_int_abs(y) & 1) ? n : s7_make_real(sc, 1.0));
            }
        }

      if (s7_is_complex(n))
        {
#if HAVE_COMPLEX_NUMBERS
          if ((s7_real_part(n) == 0.0) &&
              ((s7_imag_part(n) == 1.0) ||
               (s7_imag_part(n) == -1.0)))
            {
              bool yp = (y > 0), np = (s7_imag_part(n) > 0.0);
              switch (s7_int_abs(y) % 4)
                {
                case 0: return(s7_make_real(sc, 1.0));
                case 1: return(make_complex_not_0i(sc, 0.0, (yp == np) ? 1.0 : -1.0));
                case 2: return(s7_make_real(sc, -1.0));
                case 3: return(make_complex_not_0i(sc, 0.0, (yp == np) ? -1.0 : 1.0));
                }
            }
#else
          return(s7_out_of_range_error(sc, "expt", 2, n, "no complex numbers"));
#endif
        }
    }

  if ((s7_is_real(n)) &&
      (s7_is_real(pw)))
    {
      s7_double x, y;
      if ((s7_is_rational(pw) && !s7_is_integer(pw)) && (s7_numerator(pw) == 1))
        {
          if (s7_denominator(pw) == 2)
            return(sqrt_p_p(sc, n));
          if (s7_denominator(pw) == 3)
            return(s7_make_real(sc, cbrt(s7_real(n)))); /* (expt 27 1/3) should be 3, not 3.0... */
          /* but: (expt 512/729 1/3) -> 0.88888888888889, and 4 -> sqrt(sqrt...) etc? */
        }

      x = s7_real(n);
      y = s7_real(pw);
      if (is_NaN(x)) return(n);
      if (is_NaN(y)) return(pw);
      if (y == 0.0) return(s7_make_real(sc, 1.0));
      /* I think pow(rl, inf) is ok */
      if (x > 0.0)
        return(s7_make_real(sc, pow(x, y)));      /* tricky cases abound here: (expt -1 1/9223372036854775807) */
    }

  /* (expt 0+i 1e+16) = 0.98156860153485-0.19111012657867i ?
   * (expt 0+i 1+1/0i) = 0.0 ??
   */
  {
    s7_complex zn = s7_real_part(n) + s7_imag_part(n) * _Complex_I;
    s7_complex zpw = s7_real_part(pw) + s7_imag_part(pw) * _Complex_I;
    s7_complex result = cpow(zn, zpw);
    return s7_make_complex(sc, creal(result), cimag(result));
  }
}

s7_pointer g_expt(s7_scheme *sc, s7_pointer args)
{
  #define H_expt "(expt z1 z2) returns z1^z2"
  #define Q_expt sc->pcl_n
  return(expt_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_int_log2(s7_scheme *sc, s7_pointer args)
{
  s7_int ix = s7_integer(s7_car(args));
  s7_double fx = log2((double)ix);
  return(((ix & (ix - 1)) == 0) ? s7_make_integer(sc, (s7_int)round(fx)) : s7_make_real(sc, fx));
}
