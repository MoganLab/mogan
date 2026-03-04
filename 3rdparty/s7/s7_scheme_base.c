/* s7_scheme_base.c - basic function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_base.h"
#include <string.h>
#include <math.h>
#include <stdint.h>

#define S7_INT64_MAX 9223372036854775807LL
/* #define S7_INT64_MIN -9223372036854775808LL */   /* why is this disallowed in C? "warning: integer constant is so large that it is unsigned" */
#define S7_INT64_MIN (int64_t)(-S7_INT64_MAX - 1LL) /* in gcc 9 we had to assign this to an s7_int, then use that! */

/* Helper function to check for NaN */
bool is_NaN(s7_double x)
{
  return x != x;
}

/* Helper function to check for infinity */
static bool is_inf(s7_double x)
{
  return isinf(x);
}

#define DOUBLE_TO_INT64_LIMIT 9.223372036854775807e18  /* 2^63 - 1 */
#define INT64_TO_DOUBLE_LIMIT (1LL << 53)               /* 2^53 */
#define RATIONALIZE_LIMIT 1.0e12

/* -------------------------------- floor -------------------------------- */

s7_pointer floor_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return x;

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      s7_int num = s7_numerator(x);
      s7_int den = s7_denominator(x);
      s7_int val = num / den;  /* C '/' truncates toward zero */
      /* For negative fractions, floor is val-1 */
      return s7_make_integer(sc, (num < 0) ? (val - 1) : val);
    }

  if (s7_is_real(x))
    {
      s7_double z = s7_real(x);
      if (is_NaN(z))
        return s7_out_of_range_error(sc, "floor", 1, x, "it is NaN");
      if (is_inf(z))
        return s7_out_of_range_error(sc, "floor", 1, x, "it is infinite");
      if (fabs(z) > DOUBLE_TO_INT64_LIMIT)
        return s7_out_of_range_error(sc, "floor", 1, x, "it is too large");
      return s7_make_integer(sc, (s7_int)floor(z));
    }

  if (s7_is_complex(x))
    return s7_wrong_type_arg_error(sc, "floor", 1, x, "a real number");

  return s7_wrong_type_arg_error(sc, "floor", 1, x, "a number");
}

s7_pointer g_floor(s7_scheme *sc, s7_pointer args)
{
  #define H_floor "(floor x) returns the integer closest to x toward -inf"
  #define Q_floor s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_real_symbol)
  return floor_p_p(sc, s7_car(args));
}

s7_int floor_i_i(s7_int i)
{
  return i;
}

s7_pointer floor_p_i(s7_scheme *sc, s7_int x)
{
  return s7_make_integer(sc, x);
}

s7_int floor_i_7d(s7_scheme *sc, s7_double x)
{
  if (is_NaN(x))
    s7_out_of_range_error(sc, "floor", 1, s7_make_real(sc, x), "it is NaN");
  if (fabs(x) > DOUBLE_TO_INT64_LIMIT)
    s7_out_of_range_error(sc, "floor", 1, s7_make_real(sc, x), "it is too large");
  return (s7_int)floor(x);
}

s7_int floor_i_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x);
  if (s7_is_real(x))
    return floor_i_7d(sc, s7_real(x));
  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      s7_int num = s7_numerator(x);
      s7_int den = s7_denominator(x);
      s7_int val = num / den;
      return (num < 0) ? val - 1 : val;
    }
  s7_wrong_type_arg_error(sc, "floor", 1, x, "a real number"); return 0;
}

s7_pointer floor_p_d(s7_scheme *sc, s7_double x)
{
  return s7_make_integer(sc, floor_i_7d(sc, x));
}

/* -------------------------------- ceiling -------------------------------- */

s7_pointer ceiling_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return x;

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      s7_int num = s7_numerator(x);
      s7_int den = s7_denominator(x);
      s7_int val = num / den;  /* C '/' truncates toward zero */
      /* For positive fractions, ceiling is val+1; for negative fractions, ceiling is val */
      return s7_make_integer(sc, (num < 0) ? val : (val + 1));
    }

  if (s7_is_real(x))
    {
      s7_double z = s7_real(x);
      if (is_NaN(z))
        return s7_out_of_range_error(sc, "ceiling", 1, x, "it is NaN");
      if (is_inf(z))
        return s7_out_of_range_error(sc, "ceiling", 1, x, "it is infinite");
      if (fabs(z) > DOUBLE_TO_INT64_LIMIT)
        return s7_out_of_range_error(sc, "ceiling", 1, x, "it is too large");
      return s7_make_integer(sc, (s7_int)ceil(z));
    }

  if (s7_is_complex(x))
    return s7_wrong_type_arg_error(sc, "ceiling", 1, x, "a real number");

  return s7_wrong_type_arg_error(sc, "ceiling", 1, x, "a number");
}

s7_pointer g_ceiling(s7_scheme *sc, s7_pointer args)
{
  #define H_ceiling "(ceiling x) returns the integer closest to x toward inf"
  #define Q_ceiling s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_real_symbol)
  return ceiling_p_p(sc, s7_car(args));
}

s7_int ceiling_i_i(s7_int i)
{
  return i;
}

s7_pointer ceiling_p_i(s7_scheme *sc, s7_int x)
{
  return s7_make_integer(sc, x);
}

s7_int ceiling_i_7d(s7_scheme *sc, s7_double x)
{
  if (is_NaN(x))
    s7_out_of_range_error(sc, "ceiling", 1, s7_make_real(sc, x), "it is NaN");
  if (fabs(x) > DOUBLE_TO_INT64_LIMIT)
    s7_out_of_range_error(sc, "ceiling", 1, s7_make_real(sc, x), "it is too large");
  return (s7_int)ceil(x);
}

s7_int ceiling_i_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x);
  if (s7_is_real(x))
    return ceiling_i_7d(sc, s7_real(x));
  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      s7_int num = s7_numerator(x);
      s7_int den = s7_denominator(x);
      s7_int val = num / den;
      return (num < 0) ? val : (val + 1);
    }
  s7_wrong_type_arg_error(sc, "ceiling", 1, x, "a real number"); return 0;
}

s7_pointer ceiling_p_d(s7_scheme *sc, s7_double x)
{
  return s7_make_integer(sc, ceiling_i_7d(sc, x));
}

/* -------------------------------- abs -------------------------------- */

s7_pointer abs_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int val = s7_integer(x);
      if (val >= 0) return x;
      if (val == S7_INT64_MIN)
        return s7_out_of_range_error(sc, "abs", 1, x, "it is too large");
      return s7_make_integer(sc, -val);
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      s7_int num = s7_numerator(x);
      s7_int den = s7_denominator(x);
      if (num >= 0) return x;
      if (num == S7_INT64_MIN)
        return s7_make_ratio(sc, S7_INT64_MAX, den); /* not rationalized, can't call s7_make_ratio */
      return s7_make_ratio(sc, -num, den);
    }

  if (s7_is_real(x))
    {
      s7_double z = s7_real(x);
      if (is_NaN(z))
        {
          /* (abs -nan.0) -> +nan.0, not -nan.0 */
          /* nan_payload not available, just return NaN */
          return s7_make_real(sc, -z); /* -NaN yields NaN? */
        }
      return (signbit(z)) ? s7_make_real(sc, -z) : x;
    }

  if (s7_is_complex(x))
    return s7_wrong_type_arg_error(sc, "abs", 1, x, "a real number");

  return s7_wrong_type_arg_error(sc, "abs", 1, x, "a number");
}

s7_pointer g_abs(s7_scheme *sc, s7_pointer args)
{
  #define H_abs "(abs x) returns the absolute value of the real number x"
  #define Q_abs s7_make_signature(sc, 2, sc->is_real_symbol, sc->is_real_symbol)
  return abs_p_p(sc, s7_car(args));
}

s7_double abs_d_d(s7_double x) {return((signbit(x)) ? (-x) : x);}

s7_int abs_i_i(s7_int i)
{
  return (i < 0) ? -i : i;
}

s7_pointer abs_p_i(s7_scheme *sc, s7_int x)
{
  return s7_make_integer(sc, (x < 0) ? -x : x);
}

s7_int abs_i_7d(s7_scheme *sc, s7_double x)
{
  if (is_NaN(x))
    s7_out_of_range_error(sc, "abs", 1, s7_make_real(sc, x), "it is NaN");
  return (s7_int)((x < 0) ? -x : x);
}

s7_int abs_i_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x) >= 0 ? s7_integer(x) : -s7_integer(x);
  if (s7_is_real(x))
    return abs_i_7d(sc, s7_real(x));
  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      s7_int num = s7_numerator(x);
      s7_int den = s7_denominator(x);
      s7_int val = num / den;  /* C '/' truncates toward zero */
      /* For abs, we need absolute value of the rational number truncated toward zero */
      return (val >= 0) ? val : -val;
    }
  s7_wrong_type_arg_error(sc, "abs", 1, x, "a real number"); return 0;
}

s7_pointer abs_p_d(s7_scheme *sc, s7_double x)
{
  return s7_make_real(sc, (x < 0) ? -x : x);
}

/* -------------------------------- even? -------------------------------- */

bool even_b_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return (s7_integer(x) & 1) == 0;
  s7_wrong_type_arg_error(sc, "even?", 1, x, "an integer");
  return false;
}

s7_pointer even_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_make_boolean(sc, (s7_integer(x) & 1) == 0);
  return s7_make_boolean(sc, even_b_7p(sc, x));
}

bool even_i(s7_int i1)
{
  return (i1 & 1) == 0;
}

s7_pointer g_even(s7_scheme *sc, s7_pointer args)
{
  #define H_even "(even? int) returns #t if the integer int32_t is even"
  #define Q_even s7_make_signature(sc, 2, sc->is_boolean_symbol, sc->is_integer_symbol)
  return s7_make_boolean(sc, even_b_7p(sc, s7_car(args)));
}

/* -------------------------------- odd? -------------------------------- */

bool odd_b_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return (s7_integer(x) & 1) == 1;
  s7_wrong_type_arg_error(sc, "odd?", 1, x, "an integer");
  return false;
}

s7_pointer odd_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_make_boolean(sc, (s7_integer(x) & 1) == 1);
  return s7_make_boolean(sc, odd_b_7p(sc, x));
}

bool odd_i(s7_int i1)
{
  return (i1 & 1) == 1;
}

s7_pointer g_odd(s7_scheme *sc, s7_pointer args)
{
  #define H_odd "(odd? int) returns #t if the integer int32_t is odd"
  #define Q_odd s7_make_signature(sc, 2, sc->is_boolean_symbol, sc->is_integer_symbol)
  return s7_make_boolean(sc, odd_b_7p(sc, s7_car(args)));
}

/* -------------------------------- zero? -------------------------------- */

bool zero_b_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x) == 0;
  if (s7_is_real(x))
    return s7_real(x) == 0.0;
  if (s7_is_rational(x) && !s7_is_integer(x))
    return false; /* rational numbers with non-zero numerator are not zero */
  if (s7_is_complex(x))
    return (s7_real_part(x) == 0.0) && (s7_imag_part(x) == 0.0);
  s7_wrong_type_arg_error(sc, "zero?", 1, x, "a number");
  return false;
}

s7_pointer zero_p_p(s7_scheme *sc, s7_pointer x)
{
  return s7_make_boolean(sc, zero_b_7p(sc, x));
}

bool zero_i(s7_int i)
{
  return i == 0;
}

bool zero_d(s7_double x)
{
  return x == 0.0;
}

s7_pointer g_zero(s7_scheme *sc, s7_pointer args)
{
  #define H_zero "(zero? num) returns #t if the number num is zero"
  #define Q_zero sc->pl_bn
  return s7_make_boolean(sc, zero_b_7p(sc, s7_car(args)));
}

/* -------------------------------- positive? -------------------------------- */

bool positive_b_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x) > 0;
  if (s7_is_real(x))
    return s7_real(x) > 0.0;
  if (s7_is_rational(x) && !s7_is_integer(x))
    return s7_numerator(x) > 0;
  s7_wrong_type_arg_error(sc, "positive?", 1, x, "a real number");
  return false;
}

s7_pointer positive_p_p(s7_scheme *sc, s7_pointer x)
{
  return s7_make_boolean(sc, positive_b_7p(sc, x));
}

bool positive_i(s7_int i)
{
  return i > 0;
}

bool positive_d(s7_double x)
{
  return x > 0.0;
}

s7_pointer g_positive(s7_scheme *sc, s7_pointer args)
{
  #define H_positive "(positive? num) returns #t if the real number num is positive (greater than 0)"
  #define Q_positive s7_make_signature(sc, 2, sc->is_boolean_symbol, sc->is_real_symbol)
  return s7_make_boolean(sc, positive_b_7p(sc, s7_car(args)));
}

/* -------------------------------- negative? -------------------------------- */

bool negative_b_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return s7_integer(x) < 0;
  if (s7_is_real(x))
    return s7_real(x) < 0.0;
  if (s7_is_rational(x) && !s7_is_integer(x))
    return s7_numerator(x) < 0;
  s7_wrong_type_arg_error(sc, "negative?", 1, x, "a real number");
  return false;
}

s7_pointer negative_p_p(s7_scheme *sc, s7_pointer x)
{
  return s7_make_boolean(sc, negative_b_7p(sc, x));
}

bool negative_i(s7_int p)
{
  return p < 0;
}

bool negative_d(s7_double p)
{
  return p < 0.0;
}

s7_pointer g_negative(s7_scheme *sc, s7_pointer args)
{
  #define H_negative "(negative? num) returns #t if the real number num is negative (less than 0)"
  #define Q_negative s7_make_signature(sc, 2, sc->is_boolean_symbol, sc->is_real_symbol)
  return s7_make_boolean(sc, negative_b_7p(sc, s7_car(args)));
}

/* -------------------------------- exact? -------------------------------- */

bool exact_b_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return true;
  if (s7_is_rational(x) && !s7_is_integer(x))
    return true;
  if (s7_is_real(x))
    return false;
  if (s7_is_complex(x))
    return false;
  s7_wrong_type_arg_error(sc, "exact?", 1, x, "a number");
  return false;
}

s7_pointer exact_p_p(s7_scheme *sc, s7_pointer x)
{
  return s7_make_boolean(sc, exact_b_7p(sc, x));
}

s7_pointer g_exact(s7_scheme *sc, s7_pointer args)
{
  #define H_exact "(exact? num) returns #t if num is exact (an integer or a ratio)"
  #define Q_exact sc->pl_bn
  return s7_make_boolean(sc, exact_b_7p(sc, s7_car(args)));
}

/* -------------------------------- inexact? -------------------------------- */

bool inexact_b_7p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return false;
  if (s7_is_rational(x) && !s7_is_integer(x))
    return false;
  if (s7_is_real(x))
    return true;
  if (s7_is_complex(x))
    return true;
  s7_wrong_type_arg_error(sc, "inexact?", 1, x, "a number");
  return false;
}

s7_pointer inexact_p_p(s7_scheme *sc, s7_pointer x)
{
  return s7_make_boolean(sc, inexact_b_7p(sc, x));
}

s7_pointer g_inexact(s7_scheme *sc, s7_pointer args)
{
  #define H_inexact "(inexact? num) returns #t if num is inexact (neither an integer nor a ratio)"
  #define Q_inexact sc->pl_bn
  return s7_make_boolean(sc, inexact_b_7p(sc, s7_car(args)));
}

/* -------------------------------- exact->inexact -------------------------------- */

s7_pointer exact_to_inexact_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    {
      s7_int val = s7_integer(x);
      if ((val > INT64_TO_DOUBLE_LIMIT) || (val < -INT64_TO_DOUBLE_LIMIT))
        /* Without GMP, we still convert but may lose precision */
        return s7_make_real(sc, (s7_double)val);
      return s7_make_real(sc, (s7_double)val);
    }

  if (s7_is_rational(x) && !s7_is_integer(x))
    {
      s7_int num = s7_numerator(x);
      s7_int den = s7_denominator(x);
      if ((num > INT64_TO_DOUBLE_LIMIT) || (num < -INT64_TO_DOUBLE_LIMIT) ||
          (den > INT64_TO_DOUBLE_LIMIT))  /* just a guess */
        /* Without GMP, we still convert but may lose precision */
        return s7_make_real(sc, (s7_double)num / (s7_double)den);
      return s7_make_real(sc, (s7_double)num / (s7_double)den);
    }

  if (s7_is_real(x) || s7_is_complex(x))
    return x; /* apparently (exact->inexact 1+i) is not an error */

  s7_wrong_type_arg_error(sc, "exact->inexact", 1, x, "a number");
  return NULL;
}

s7_pointer g_exact_to_inexact(s7_scheme *sc, s7_pointer args)
{
  #define H_exact_to_inexact "(exact->inexact num) converts num to an inexact number; (exact->inexact 3/2) = 1.5"
  #define Q_exact_to_inexact s7_make_signature(sc, 2, sc->is_number_symbol, sc->is_number_symbol)
  /* arg can be complex -> itself! */
  return exact_to_inexact_p_p(sc, s7_car(args));
}

/* -------------------------------- inexact->exact -------------------------------- */

s7_pointer inexact_to_exact_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x) || (s7_is_rational(x) && !s7_is_integer(x)))
    return x;

  if (s7_is_real(x))
    {
      s7_double val = s7_real(x);
      if (is_NaN(val) || is_inf(val))
        return s7_wrong_type_arg_error(sc, "inexact->exact", 1, x, "a normal real number");

      if ((val > DOUBLE_TO_INT64_LIMIT) || (val < -(DOUBLE_TO_INT64_LIMIT)))
        return s7_out_of_range_error(sc, "inexact->exact", 1, x, "it is too large");

      /* Try to rationalize */
      s7_pointer result = s7_rationalize(sc, val, 1.0e-12); /* default_rationalize_error */
      /* s7_rationalize returns a rational or integer, or #f if cannot rationalize? */
      /* We assume it always returns a rational or integer */
      if (result != s7_f(sc)) /* #f */
        return result;
      /* If rationalization fails, return the original real? But we need exact number.
         Fall through to error? For now, return the real (inexact) as a last resort. */
    }

  if (s7_is_complex(x))
    return s7_wrong_type_arg_error(sc, "inexact->exact", 1, x, "a real number");

  s7_wrong_type_arg_error(sc, "inexact->exact", 1, x, "a real number");
  return NULL;
}

s7_pointer g_inexact_to_exact(s7_scheme *sc, s7_pointer args)
{
  #define H_inexact_to_exact "(inexact->exact num) converts num to an exact number; (inexact->exact 1.5) = 3/2"
  #define Q_inexact_to_exact s7_make_signature(sc, 2, sc->is_real_symbol, sc->is_real_symbol)
  return inexact_to_exact_p_p(sc, s7_car(args));
}