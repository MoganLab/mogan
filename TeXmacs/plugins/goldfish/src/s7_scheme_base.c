/* s7_scheme_base.c - basic function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_base.h"
#include "s7_internal_helpers.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>

#define Malloc(Size) malloc(Size)

#if __GNUC__
  #define Sentinel __attribute__((sentinel))
#else
  #define Sentinel
#endif

#ifndef S7_DEBUGGING
  #define S7_DEBUGGING 0
#endif

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

/* C string helper functions */

s7_int safe_strlen(const char *str)
{
  const char *tmp = str;
  if ((!tmp) || (!*tmp)) return(0);
  for (; *tmp; ++tmp);
  return(tmp - str);
}

char *copy_string_with_length(const char *str, s7_int len)
{
  char *newstr;
#if S7_DEBUGGING
  if ((len <= 0) || (!str))
    {fprintf(stderr, "%s[%d]: len: %" ld64 ", str: %s\n", __func__, __LINE__, len, str); abort();}
#endif
  if (len > (1LL << 48)) return(NULL);
  newstr = (char *)Malloc(len + 1);
  memcpy((void *)newstr, (const void *)str, len);
  newstr[len] = '\0';
  return(newstr);
}

char *copy_string(const char *str)
{
  return(copy_string_with_length(str, safe_strlen(str)));
}

bool safe_strcmp(const char *s1, const char *s2)
{
  if ((!s1) || (!s2)) return(s1 == s2);
  return(strcmp(s1, s2) == 0);
}

bool local_strncmp(const char *s1, const char *s2, size_t n)
{
#if ((!S7_ALIGNED) && (defined(__x86_64__) || defined(__i386__)))
  if (n >= 8)
    {
      size_t n8 = n >> 3;
      s7_int *is1 = (s7_int *)s1, *is2 = (s7_int *)s2;
      do {if (*is1++ != *is2++) return(false);} while (--n8 > 0);
      s1 = (const char *)is1;
      s2 = (const char *)is2;
      n &= 7;
    }
#endif
  while (n > 0)
    {
      if (*s1++ != *s2++) return(false);
      n--;
    }
  return(true);
}

size_t catstrs(char *dst, size_t len, ...)
{
  const char *dend = (const char *)(dst + len - 1);
  char *d = dst;
  va_list ap;
  while ((*d) && (d < dend)) d++;
  va_start(ap, len);
  for (const char *s = va_arg(ap, const char *); s != NULL; s = va_arg(ap, const char *))
    while ((*s) && (d < dend)) {*d++ = *s++;}
  *d = '\0';
  va_end (ap);
  return(d - dst);
}

size_t catstrs_direct(char *dst, const char *str1, ...)
{
  char *d = dst;
  va_list ap;
  va_start(ap, str1);
  for (const char *s = str1; s != NULL; s = va_arg(ap, const char *))
    while (*s) {*d++ = *s++;}
  *d = '\0';
  va_end (ap);
  return(d - dst);
}

/* NaN payload helper functions */

double nan_with_payload(s7_int payload)
{
  decode_float_t num;
  if (payload <= 0) return(NAN);
  num.fx = NAN;
  num.ix = num.ix | payload;
  return(num.fx);
}

s7_int nan_payload(double x)
{
  decode_float_t num;
  num.fx = x;
  return(num.ix & 0xffffffffffff);
}

#define DOUBLE_TO_INT64_LIMIT 9.223372036854775807e18  /* 2^63 - 1 */
#define INT64_TO_DOUBLE_LIMIT (1LL << 53)               /* 2^53 */
#define RATIONALIZE_LIMIT 1.0e12
#define ld64 PRId64

static s7_int wrap_uint64_to_s7_int(uint64_t bits)
{
  s7_int result;
  memcpy(&result, &bits, sizeof(result));
  return result;
}

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

/* -------------------------------- string->number helpers -------------------------------- */

/* Digit conversion table for number parsing */
static const uint8_t s7_digit_table[256] = {
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   255, 255, 255, 255, 255, 255,
  255, 10,  11,  12,  13,  14,  15,  255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 10,  11,  12,  13,  14,  15,  255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

/* Convert string to integer with radix support.
 * In the current non-GMP build, integers wrap to s7_int the same way as the
 * stable gf reader.
 */
s7_int s7_string_to_integer(const char *str, int32_t radix, bool *overflow)
{
  bool negative = false;
  uint64_t new_int = 0;
  int32_t dig;
  const char *tmp = str;

  *overflow = false;

  if (str[0] == '+')
    tmp++;
  else if (str[0] == '-')
    {
      negative = true;
      tmp++;
    }

  while (*tmp == '0') tmp++;

  while (true)
    {
      dig = s7_digit_table[(uint8_t)(*tmp++)];
      if (dig >= radix) break;
      new_int = (new_int * (uint64_t)radix) + (uint64_t)dig;
    }

  if (negative)
    new_int = (uint64_t)(0 - new_int);

  return wrap_uint64_to_s7_int(new_int);
}

/* String to double conversion with radix support.
 * Base-10 literals use strtod after normalizing Scheme exponent markers.
 * Other radices keep the existing integer-based fallback.
 */
double s7_string_to_double_simple(const char *str, int32_t radix)
{
  if (radix == 10)
    {
      const char *marker = str;

      while (*marker)
        {
          if ((marker != str) &&
              (((marker[1] >= '0') && (marker[1] <= '9')) ||
               (marker[1] == '+') ||
               (marker[1] == '-')) &&
              ((*marker == '@') ||
               (*marker == 's') || (*marker == 'S') ||
               (*marker == 'f') || (*marker == 'F') ||
               (*marker == 'd') || (*marker == 'D') ||
               (*marker == 'l') || (*marker == 'L')))
            {
              size_t len = strlen(str);
              char *copy = (char *)malloc(len + 1);
              double result;

              if (!copy)
                return strtod(str, NULL);

              memcpy(copy, str, len + 1);
              copy[marker - str] = 'e';
              result = strtod(copy, NULL);
              free(copy);
              return result;
            }
          marker++;
        }

      return strtod(str, NULL);
    }

  const char *p = str;
  double sign = 1.0;
  int64_t mantissa = 0;      /* Use integer for precise digit accumulation */
  int32_t mantissa_digits = 0;
  int32_t fraction_digits = 0;
  int32_t exponent = 0;
  bool has_exponent = false;
  bool exp_negative = false;
  bool in_fraction = false;

  /* Handle sign */
  if (*p == '-') {
    sign = -1.0;
    p++;
  } else if (*p == '+') {
    p++;
  }

  /* Skip leading zeros in integer part */
  while (*p == '0') p++;

  /* Process digits */
  while (*p) {
    char c = *p;

    /* Decimal point */
    if (c == '.') {
      in_fraction = true;
      p++;
      continue;
    }

    /* Exponent marker (only in base 10) */
    if (radix == 10 && (c == 'e' || c == 'E' || c == '@')) {
      has_exponent = true;
      p++;
      /* Handle exponent sign */
      if (*p == '-') {
        exp_negative = true;
        p++;
      } else if (*p == '+') {
        p++;
      }
      break;
    }

    /* Convert digit */
    int32_t digit;
    if (c >= '0' && c <= '9')
      digit = c - '0';
    else if (c >= 'a' && c <= 'f')
      digit = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      digit = c - 'A' + 10;
    else
      break;

    if (digit >= radix)
      break;

    /* Accumulate mantissa as integer (limited precision) */
    if (mantissa_digits < 17) {  /* double has ~15-17 decimal digits */
      mantissa = mantissa * radix + digit;
      mantissa_digits++;
      if (in_fraction)
        fraction_digits++;
    } else if (!in_fraction) {
      /* Skip extra integer digits (will be handled by exponent) */
      exponent++;
    }
    p++;
  }

  /* Parse exponent */
  if (has_exponent) {
    int32_t exp_val = 0;
    while (*p >= '0' && *p <= '9') {
      exp_val = exp_val * 10 + (*p - '0');
      p++;
    }
    if (exp_negative)
      exp_val = -exp_val;
    exponent += exp_val;
  }

  /* Calculate final value: mantissa * 10^(exponent - fraction_digits) */
  double result = (double)mantissa;
  int32_t final_exp = exponent - fraction_digits;

  if (final_exp != 0) {
    /* Use exponentiation by squaring for power calculation */
    double base = (double)radix;
    int32_t exp = final_exp;
    double factor = 1.0;

    if (exp > 0) {
      /* Positive exponent: multiply */
      double pow_val = base;
      while (exp > 0) {
        if (exp & 1)
          factor *= pow_val;
        pow_val *= pow_val;
        exp >>= 1;
      }
      result *= factor;
    } else {
      /* Negative exponent: divide */
      double pow_val = base;
      exp = -exp;
      while (exp > 0) {
        if (exp & 1)
          factor *= pow_val;
        pow_val *= pow_val;
        exp >>= 1;
      }
      result /= factor;
    }
  }

  return sign * result;
}

/* -------------------------------- read-line -------------------------------- */

s7_pointer g_read_line(s7_scheme *sc, s7_pointer args)
{
  s7_pointer port;
  bool with_eol = false;

  if (s7_is_pair(args))
    {
      port = s7_car(args);
      if (!s7_is_input_port(sc, port))
        return s7i_method_or_bust(sc, port, "read-line", args, s7i_an_input_port_string(), 1);
      if (s7_is_pair(s7_cdr(args)))
        {
          s7_pointer with_eol_arg = s7_cadr(args);
          with_eol = (with_eol_arg == s7_t(sc));
          if ((!with_eol) && (with_eol_arg != s7_f(sc)))
            return s7_wrong_type_arg_error(sc, "read-line", 2, with_eol_arg, s7i_a_boolean_string());
        }
    }
  else
    {
      port = s7i_input_port_if_not_loading(sc);
      if (!port) return s7_eof_object(sc);
    }
  return s7i_port_read_line(sc, port, with_eol);
}

/* ---------------------------------------- max ---------------------------------------- */

s7_pointer g_max(s7_scheme *sc, s7_pointer args)
{
  #define H_max "(max ...) returns the maximum of its arguments"
  #define Q_max sc->pcl_r

  s7_pointer x = s7_car(args);
  if (s7_is_null(sc, s7_cdr(args)))
    {
      if (s7_is_real(x)) return(x);
      return s7_wrong_type_arg_error(sc, "max", 1, x, "a real number");
    }
  for (s7_pointer nums = s7_cdr(args); s7_is_pair(nums); nums = s7_cdr(nums))
    x = max_p_pp(sc, x, s7_car(nums));
  return(x);
}

s7_pointer g_max_2(s7_scheme *sc, s7_pointer args) {return(max_p_pp(sc, s7_car(args), s7_cadr(args)));}
s7_pointer g_max_3(s7_scheme *sc, s7_pointer args) {return(max_p_pp(sc, max_p_pp(sc, s7_car(args), s7_cadr(args)), s7_caddr(args)));}

/* ---------------------------------------- min ---------------------------------------- */

s7_pointer g_min(s7_scheme *sc, s7_pointer args)
{
  #define H_min "(min ...) returns the minimum of its arguments"
  #define Q_min sc->pcl_r

  s7_pointer x = s7_car(args);
  if (s7_is_null(sc, s7_cdr(args)))
    {
      if (s7_is_real(x)) return(x);
      return s7_wrong_type_arg_error(sc, "min", 1, x, "a real number");
    }
  for (s7_pointer nums = s7_cdr(args); s7_is_pair(nums); nums = s7_cdr(nums))
    x = min_p_pp(sc, x, s7_car(nums));
  return(x);
}

s7_pointer g_min_2(s7_scheme *sc, s7_pointer args) {return(min_p_pp(sc, s7_car(args), s7_cadr(args)));}
s7_pointer g_min_3(s7_scheme *sc, s7_pointer args) {return(min_p_pp(sc, min_p_pp(sc, s7_car(args), s7_cadr(args)), s7_caddr(args)));}

/* -------------------------------- c_gcd -------------------------------- */

static s7_int c_gcd_1(s7_int u, s7_int v)
{
  /* can't take abs of these so do it by hand */
  s7_int divisor = 1;
  if (u == v) return(u);
  while (((u & 1) == 0) && ((v & 1) == 0))
    {
      u /= 2;
      v /= 2;
      divisor *= 2;
    }
  return(divisor);
}

s7_int c_gcd(s7_int u, s7_int v)
{
  s7_int a, b;
  if (u < 0)
    {
      if (u == S7_INT64_MIN) return(c_gcd_1(u, v));
      a = -u;
    }
  else a = u;
  if (v < 0)
    {
      if (v == S7_INT64_MIN) return(c_gcd_1(u, v));
      b = -v;
    }
  else b = v;
  while (b != 0)
    {
      s7_int temp = a % b;
      a = b;
      b = temp;
    }
  return(a);
}

/* -------------------------------- truncate -------------------------------- */

s7_pointer truncate_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return(x);
  if (s7_is_ratio(x))
    return(s7_make_integer(sc, (s7_int)(s7_numerator(x) / s7_denominator(x))));
  if (s7_is_real(x))
    {
      const s7_double z = s7_real(x);
      if (is_NaN(z))
        return s7_out_of_range_error(sc, "truncate", 1, x, "it is NaN");
      if (isinf(z))
        return s7_out_of_range_error(sc, "truncate", 1, x, "it is infinite");
      if (fabs(z) > DOUBLE_TO_INT64_LIMIT)
        return s7_out_of_range_error(sc, "truncate", 1, x, "it is too large");
      return(s7_make_integer(sc, (z > 0.0) ? (s7_int)floor(z) : (s7_int)ceil(z)));
    }
  if (s7_is_complex(x))
    return s7_wrong_type_arg_error(sc, "truncate", 1, x, "a real number");
  return s7i_method_or_bust_p(sc, x, "truncate", "a real number");
}

s7_pointer g_truncate(s7_scheme *sc, s7_pointer args)
{
  #define H_truncate "(truncate x) returns the integer closest to x toward 0"
  #define Q_truncate s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_real_symbol)
  return(truncate_p_p(sc, s7_car(args)));
}

s7_int truncate_i_i(s7_int i) {return(i);}
s7_pointer truncate_p_i(s7_scheme *sc, s7_int x) {return(s7_make_integer(sc, x));}

s7_int truncate_i_7d(s7_scheme *sc, s7_double x)
{
  if (is_NaN(x))
    s7_out_of_range_error(sc, "truncate", 1, s7_make_real(sc, x), "it is NaN");
  if (isinf(x))
    s7_out_of_range_error(sc, "truncate", 1, s7_make_real(sc, x), "it is infinite");
  if (fabs(x) > DOUBLE_TO_INT64_LIMIT)
    s7_out_of_range_error(sc, "truncate", 1, s7_make_real(sc, x), "it is too large");
  return((x > 0.0) ? (s7_int)floor(x) : (s7_int)ceil(x));
}

s7_pointer truncate_p_d(s7_scheme *sc, s7_double x) {return(s7_make_integer(sc, truncate_i_7d(sc, x)));}

/* -------------------------------- round -------------------------------- */

static s7_double r5rs_round(s7_double x)
{
  s7_double fl = floor(x), ce = ceil(x);
  s7_double dfl = x - fl;
  s7_double dce = ce - x;
  if (dfl > dce) return(ce);
  if (dfl < dce) return(fl);
  return((fmod(fl, 2.0) == 0.0) ? fl : ce);
}

s7_pointer round_p_p(s7_scheme *sc, s7_pointer x)
{
  if (s7_is_integer(x))
    return(x);
  if (s7_is_ratio(x))
    {
      s7_int truncated = s7_numerator(x) / s7_denominator(x), remains = s7_numerator(x) % s7_denominator(x);
      long double frac = fabsl((long double)remains / (long double)s7_denominator(x));
      if ((frac > 0.5) ||
          ((frac == 0.5) &&
           (truncated % 2 != 0)))
        return(s7_make_integer(sc, (s7_numerator(x) < 0) ? (truncated - 1) : (truncated + 1)));
      return(s7_make_integer(sc, truncated));
    }
  if (s7_is_real(x))
    {
      const s7_double z = s7_real(x);
      if (is_NaN(z))
        return s7_out_of_range_error(sc, "round", 1, x, "it is NaN");
      if (isinf(z))
        return s7_out_of_range_error(sc, "round", 1, x, "it is infinite");
      if (fabs(z) > DOUBLE_TO_INT64_LIMIT)
        return s7_out_of_range_error(sc, "round", 1, x, "it is too large");
      return(s7_make_integer(sc, (s7_int)r5rs_round(z)));
    }
  if (s7_is_complex(x))
    return s7_wrong_type_arg_error(sc, "round", 1, x, "a real number");
  return s7i_method_or_bust_p(sc, x, "round", "a real number");
}

s7_pointer g_round(s7_scheme *sc, s7_pointer args)
{
  #define H_round "(round x) returns the integer closest to x"
  #define Q_round s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_real_symbol)
  return(round_p_p(sc, s7_car(args)));
}

s7_int round_i_i(s7_int i) {return(i);}
s7_pointer round_p_i(s7_scheme *sc, s7_int x) {return(s7_make_integer(sc, x));}

s7_int round_i_7d(s7_scheme *sc, s7_double z)
{
  if (is_NaN(z))
    s7_out_of_range_error(sc, "round", 1, s7_make_real(sc, z), "it is NaN");
  if ((isinf(z)) ||
      (z > DOUBLE_TO_INT64_LIMIT) || (z < -DOUBLE_TO_INT64_LIMIT))
    s7_out_of_range_error(sc, "round", 1, s7_make_real(sc, z), "it is too large");
  return((s7_int)r5rs_round(z));
}

s7_pointer round_p_d(s7_scheme *sc, s7_double x) {return(s7_make_integer(sc, round_i_7d(sc, x)));}

/* -------------------------------- gcd -------------------------------- */

s7_pointer g_gcd(s7_scheme *sc, s7_pointer args)
{
  #define H_gcd "(gcd ...) returns the greatest common divisor of its rational arguments"
  #define Q_gcd sc->pcl_f

  s7_int n = 0, d = 1;
  s7_pointer n_args;
  if (!s7_is_pair(args))       /* (gcd) */
    return(s7_make_integer(sc, 0));

  if (!s7_is_pair(s7_cdr(args)))  /* (gcd 3/4) */
    {
      if (!s7_is_rational(s7_car(args)))
        return s7i_method_or_bust(sc, s7_car(args), "gcd", args, "a rational number", 1);
      return(abs_p_p(sc, s7_car(args)));
    }

  if (s7_is_integer(s7_car(args)))
    {
      n = s7_integer(s7_car(args));
      n_args = s7_cdr(args);
    }
  else n_args = args;

  for (s7_pointer nums = n_args; s7_is_pair(nums); nums = s7_cdr(nums))
    {
      const s7_pointer x = s7_car(nums);
      if (s7_is_integer(x))
        {
          if (s7_integer(x) == S7_INT64_MIN)
            {
              if ((n == S7_INT64_MIN) && (s7_is_null(sc, s7_cdr(nums)))) /* gcd is supposed to return a positive integer, but we can't take abs(S7_INT64_MIN) */
                s7_out_of_range_error(sc, "gcd", 1, args, "it is too large");
            }
          n = c_gcd(n, s7_integer(x));
        }
      else if (s7_is_ratio(x))
        {
#if HAVE_OVERFLOW_CHECKS
          s7_int dn;
#endif
          n = c_gcd(n, s7_numerator(x));
          if (d == 1)
            d = s7_denominator(x);
          else
            {
              const s7_int b = s7_denominator(x);
#if HAVE_OVERFLOW_CHECKS
              if (multiply_overflow(d / c_gcd(d, b), b, &dn)) /* (gcd 1/92233720368547758 1/3005) */
                s7_out_of_range_error(sc, "gcd", 1, args, "intermediate result is too large");
              d = dn;
#else
              d = (d / c_gcd(d, b)) * b;
#endif
            }
        }
      else if (s7_is_real(x) || s7_is_complex(x))
        return s7_wrong_type_arg_error(sc, "gcd", s7i_position_of(nums, args), x, "a rational number");
      else
        return s7i_method_or_bust(sc, x, "gcd",
                                  (nums == args) ? s7_cons(sc, x, s7_cdr(nums)) :
                                                   s7_cons(sc, (d <= 1) ? s7_make_integer(sc, n) : s7_make_ratio(sc, n, d), nums),
                                  "a rational number", s7i_position_of(nums, args));
    }
  return((d <= 1) ? s7_make_integer(sc, n) : s7_make_ratio(sc, n, d));
}

/* -------------------------------- lcm -------------------------------- */

s7_pointer g_lcm(s7_scheme *sc, s7_pointer args)
{
  /* (/ (* m n) (gcd m n)), (lcm a b c) -> (lcm a (lcm b c)) */
  #define H_lcm "(lcm ...) returns the least common multiple of its rational arguments"
  #define Q_lcm sc->pcl_f

  s7_int n = 1, d = 0;
  if (!s7_is_pair(args))
    return(s7_make_integer(sc, 1));

  if (!s7_is_pair(s7_cdr(args)))
    {
      if (!s7_is_rational(s7_car(args)))
        return s7i_method_or_bust(sc, s7_car(args), "lcm", args, "a rational number", 1);
      return(g_abs(sc, args));
    }

  for (s7_pointer nums = args; s7_is_pair(nums); nums = s7_cdr(nums))
    {
      const s7_pointer x = s7_car(nums);
      s7_int b;
#if HAVE_OVERFLOW_CHECKS
      s7_int n1;
#endif
      if (s7_is_integer(x))
        {
          d = 1;
          if (s7_integer(x) == 0) /* return 0 unless there's a wrong-type-arg (geez what a mess) */
            {
              for (nums = s7_cdr(nums); s7_is_pair(nums); nums = s7_cdr(nums))
                {
                  const s7_pointer x1 = s7_car(nums);
                  if (!s7_is_rational(x1))
                    return s7_wrong_type_arg_error(sc, "lcm", s7i_position_of(nums, args), x1, "a rational number");
                }
              return(s7_make_integer(sc, 0));
            }
          b = s7_integer(x);
          if (b < 0)
            {
              if (b == S7_INT64_MIN)
                s7_out_of_range_error(sc, "lcm", 1, args, "it is too large");
              b = -b;
            }
#if HAVE_OVERFLOW_CHECKS
          if (multiply_overflow(n / c_gcd(n, b), b, &n1))
            s7_out_of_range_error(sc, "lcm", 1, args, "result is too large");
          n = n1;
#else
          n = (n / c_gcd(n, b)) * b;
#endif
        }
      else if (s7_is_ratio(x))
        {
          b = s7_numerator(x);
          if (b < 0)
            {
              if (b == S7_INT64_MIN)
                s7_out_of_range_error(sc, "lcm", 1, args, "it is too large");
              b = -b;
            }
#if HAVE_OVERFLOW_CHECKS
          if (multiply_overflow(n / c_gcd(n, b), b, &n1))  /* (lcm 92233720368547758/3 3005/2) */
            s7_out_of_range_error(sc, "lcm", 1, args, "intermediate result is too large");
          n = n1;
#else
          n = (n / c_gcd(n, b)) * b;
#endif
          if (d == 0)
            d = (nums == args) ? s7_denominator(x) : 1;
          else d = c_gcd(d, s7_denominator(x));
        }
      else if (s7_is_real(x) || s7_is_complex(x))
        return s7_wrong_type_arg_error(sc, "lcm", s7i_position_of(nums, args), x, "a rational number");
      else
        return s7i_method_or_bust(sc, x, "lcm",
                                  (nums == args) ? s7_cons(sc, x, s7_cdr(nums)) :
                                                   s7_cons(sc, (d <= 1) ? s7_make_integer(sc, n) : s7_make_ratio(sc, n, d), nums),
                                  "a rational number", s7i_position_of(nums, args));
    }
  return((d <= 1) ? s7_make_integer(sc, n) : s7_make_ratio(sc, n, d));
}

/* -------------------------------- rationalize -------------------------------- */

bool c_rationalize(s7_double ux, s7_double error, s7_int *numer, s7_int *denom)
{
  /* from CL code in Canny, Donald, Ressler, "A Rational Rotation Method for Robust Geometric Algorithms" */
  double x0, x1;
  s7_int i, p0, q0 = 1, p1, q1 = 1;
  double e0, e1, e0p, e1p;
  int32_t tries = 0;
  /* don't use long_double: the loop below will hang */

  /* #e1e19 is a killer -- it's bigger than most-positive-fixnum, but if we ceil(ux) below
   *   it turns into most-negative-fixnum.  1e19 is trouble in many places.
   */
  if (fabs(ux) > RATIONALIZE_LIMIT)
    {
      /* (rationalize most-positive-fixnum) should not return most-negative-fixnum
       *   but any number > 1e14 here is so inaccurate that rationalize is useless
       *   for example,
       *     default: (rationalize (/ (*s7* 'most-positive-fixnum) 31111.0)) -> 1185866354261165/4
       *     gmp:     (rationalize (/ (*s7* 'most-positive-fixnum) 31111.0)) -> 9223372036854775807/31111
       * can't return false here because that confuses some of the callers!
       */
      (*numer) = (s7_int)ux;
      (*denom) = 1;
      return(true);
    }

  if (error < 0.0) error = -error;
  x0 = ux - error;
  x1 = ux + error;
  i = (s7_int)ceil(x0);

  if (error >= 1.0) /* aw good grief! */
    {
      if (x0 < 0.0)
        (*numer) = (x1 < 0.0) ? (s7_int)floor(x1) : 0;
      else (*numer) = i;
      (*denom) = 1;
      return(true);
    }
  if (x1 >= i)
    {
      (*numer) = (i >= 0) ? i : (s7_int)floor(x1);
      (*denom) = 1;
      return(true);
    }

  p0 = (s7_int)floor(x0);
  p1 = (s7_int)ceil(x1);
  e0 = p1 - x0;
  e1 = x0 - p0;
  e0p = p1 - x1;
  e1p = x1 - p0;
  while (true)
    {
      s7_int old_p1, old_q1;
      double old_e0, old_e1, old_e0p, r, r1;
      const double val = (double)p0 / (double)q0;

      if (((x0 <= val) && (val <= x1)) || (e1 == 0.0) || (e1p == 0.0) || (tries > 100))
        {
          if ((q0 == S7_INT64_MIN) && (p0 == 1)) /* (rationalize 1.000000004297917e-12) when error is 1e-12 */
            {
              (*numer) = 0;
              (*denom) = 1;
            }
          else
            {
              (*numer) = p0;
              (*denom) = q0;
              if ((S7_DEBUGGING) && (q0 == 0)) fprintf(stderr, "%s[%d]: %f %" ld64 "/0\n", __func__, __LINE__, ux, p0);
            }
          if ((S7_DEBUGGING) && (*denom < 0)) fprintf(stderr, "%s[%d]: denominator is %" ld64 "?\n", __func__, __LINE__, *denom);
          return(true);
        }
      tries++;
      r = (s7_int)floor(e0 / e1);
      r1 = (s7_int)ceil(e0p / e1p);
      if (r1 < r) r = r1;
      /* do handles all step vars in parallel */
      old_p1 = p1;
      p1 = p0;
      old_q1 = q1;
      q1 = q0;
      old_e0 = e0;
      e0 = e1p;
      old_e0p = e0p;
      e0p = e1;
      old_e1 = e1;
      p0 = old_p1 + r * p0;
      q0 = old_q1 + r * q0;
      e1 = old_e0p - r * e1p;  /* if the error is set too low, we can get e1 = 0 here: (rationalize (/ pi) 1e-17) */
      e1p = old_e0 - r * old_e1;
    }
  return(false);
}

s7_pointer g_rationalize(s7_scheme *sc, s7_pointer args)
{
  #define H_rationalize "(rationalize x err) returns the ratio with smallest denominator within err of x"
  #define Q_rationalize s7_make_signature(sc, 3, sc->is_rational_symbol, sc->is_real_symbol, sc->is_real_symbol)

  s7_double err;
  const s7_pointer x = s7_car(args);

  if (!s7_is_real(x))
    return s7i_method_or_bust(sc, x, "rationalize", args, "a real number", 1);
  if (s7_is_null(sc, s7_cdr(args)))
    err = s7i_default_rationalize_error(sc);
  else
    {
      const s7_pointer ex = s7_cadr(args);
      if (!s7_is_real(ex))
        return s7i_method_or_bust(sc, ex, "rationalize", args, "a real number", 2);
      err = s7_number_to_real_with_caller(sc, ex, "rationalize");
      if (is_NaN(err))
        s7_out_of_range_error(sc, "rationalize", 2, ex, "it is NaN");
      if (err < 0.0) err = -err;
    }

  if (s7_is_integer(x))
    {
      s7_int a, b, pa;
      if (err < 1.0) return(x);
      a = s7_integer(x);
      pa = (a < 0) ? -a : a;
      if (err >= pa) return(s7_make_integer(sc, 0));
      b = (s7_int)err;
      pa -= b;
      return(s7_make_integer(sc, (a < 0) ? -pa : pa));
    }
  if (s7_is_ratio(x))
    {
      if (err == 0.0)
        return(x);
    }
  if (s7_is_real(x))
    {
      const s7_double rat = s7_real(x);
      s7_int numer = 0, denom = 1;
      if ((is_NaN(rat)) || (isinf(rat)))
        s7_out_of_range_error(sc, "rationalize", 1, x, "a normal real number");
      if (err >= fabs(rat))
        return(s7_make_integer(sc, 0));
      if (fabs(rat) > RATIONALIZE_LIMIT)
        s7_out_of_range_error(sc, "rationalize", 1, x, "it is too large");
      if ((fabs(rat) + fabs(err)) < 1.0e-18)
        err = 1.0e-18;
      if (fabs(rat) < fabs(err))
        return(s7_make_integer(sc, 0));
      return((c_rationalize(rat, err, &numer, &denom)) ? s7_make_ratio(sc, numer, denom) : s7_f(sc));
    }
  return s7_f(sc);
}

s7_int rationalize_i_i(s7_int x) {return(x);}
s7_pointer rationalize_p_i(s7_scheme *sc, s7_int x) {return(s7_make_integer(sc, x));}

s7_pointer rationalize_p_d(s7_scheme *sc, s7_double x)
{
  if ((is_NaN(x)) || (isinf(x)))
    s7_out_of_range_error(sc, "rationalize", 1, s7_make_real(sc, x), "a normal real number");
  if (fabs(x) > RATIONALIZE_LIMIT)
    s7_out_of_range_error(sc, "rationalize", 1, s7_make_real(sc, x), "it is too large");
  return(s7_rationalize(sc, x, s7i_default_rationalize_error(sc)));
}

/* -------------------------------- number->string -------------------------------- */

s7_pointer g_number_to_string(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args);

  if (!s7_is_number(x))
    return(s7i_method_or_bust(sc, x, "number->string", args, "a number", 1));

  if (s7_is_pair(s7_cdr(args)))
    {
      s7_pointer base = s7_cadr(args);
      if (!s7_is_integer(base))
        return(s7i_method_or_bust(sc, base, "number->string", args, "an integer", 2));
      s7_int radix = s7_integer(base);
      if ((radix < 2) || (radix > 16))
        return(s7_out_of_range_error(sc, "number->string", 2, base, "a valid radix"));
      char *str = s7_number_to_string(sc, x, radix);
      s7_pointer result = s7_make_string(sc, str);
      free(str);
      return(result);
    }

  char *str = s7_number_to_string(sc, x, 10);
  s7_pointer result = s7_make_string(sc, str);
  free(str);
  return(result);
}

s7_pointer number_to_string_p_p(s7_scheme *sc, s7_pointer p)
{
  char *str;
  if (!s7_is_number(p))
    return(s7i_method_or_bust_p(sc, p, "number->string", "a number"));
  str = s7_number_to_string(sc, p, 10);
  {
    s7_pointer result = s7_make_string(sc, str);
    free(str);
    return(result);
  }
}

s7_pointer number_to_string_p_i(s7_scheme *sc, s7_int p)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "%" PRId64, p);
  return(s7_make_string(sc, buf));
}

s7_pointer number_to_string_p_pp(s7_scheme *sc, s7_pointer num, s7_pointer base)
{
  s7_int radix;
  char *str;

  if (!s7_is_number(num))
    return(s7_wrong_type_arg_error(sc, "number->string", 1, num, "a number"));
  if (!s7_is_integer(base))
    return(s7_wrong_type_arg_error(sc, "number->string", 2, base, "an integer"));
  radix = s7_integer(base);
  if ((radix < 2) || (radix > 16))
    return(s7_out_of_range_error(sc, "number->string", 2, base, "a valid radix"));

  str = s7_number_to_string(sc, num, radix);
  {
    s7_pointer result = s7_make_string(sc, str);
    free(str);
    return(result);
  }
}

s7_pointer g_numerator(s7_scheme *sc, s7_pointer args)
{
  #define H_numerator "(numerator rat) returns the numerator of the rational number rat"
  #define Q_numerator s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_rational_symbol)

  const s7_pointer x = s7_car(args);
  if (s7_is_ratio(x))
    return(s7_make_integer(sc, s7_numerator(x)));
  if (s7_is_integer(x))
    return(x);
  return(s7i_method_or_bust_p(sc, x, "numerator", "an integer or a ratio"));
}

s7_pointer g_denominator(s7_scheme *sc, s7_pointer args)
{
  #define H_denominator "(denominator rat) returns the denominator of the rational number rat"
  #define Q_denominator s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_rational_symbol)

  const s7_pointer x = s7_car(args);
  if (s7_is_ratio(x))
    return(s7_make_integer(sc, s7_denominator(x)));
  if (s7_is_integer(x))
    return(s7i_int_one(sc));
  return(s7i_method_or_bust_p(sc, x, "denominator", "an integer or a ratio"));
}

s7_pointer g_reverse(s7_scheme *sc, s7_pointer args)
{
  #define H_reverse "(reverse lst) returns a list with the elements of lst in reverse order.  reverse \
also accepts a string or vector argument."
  #define Q_reverse s7_make_signature(sc, 2, sc->is_sequence_symbol, sc->is_sequence_symbol)
  return(s7i_reverse_p_p(sc, s7_car(args)));
}

s7_pointer g_assq(s7_scheme *sc, s7_pointer args)
{
  return(s7i_assq_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_assv(s7_scheme *sc, s7_pointer args)
{
  return(s7i_assv_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_memv(s7_scheme *sc, s7_pointer args)
{
  return(s7i_memv_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_list_0(s7_scheme *sc, s7_pointer args)
{
  return(s7_nil(sc));
}

s7_pointer g_list_1(s7_scheme *sc, s7_pointer args)
{
  return(s7_cons(sc, s7_car(args), s7_nil(sc)));
}

s7_pointer g_list_2(s7_scheme *sc, s7_pointer args)
{
  return(s7_cons(sc, s7_car(args), s7_cons(sc, s7_cadr(args), s7_nil(sc))));
}

s7_pointer g_list_3(s7_scheme *sc, s7_pointer args)
{
  return(s7_cons(sc, s7_car(args), s7_cons(sc, s7_cadr(args), s7_cons(sc, s7_caddr(args), s7_nil(sc)))));
}

s7_pointer g_list_4(s7_scheme *sc, s7_pointer args)
{
  return(s7_cons(sc, s7_car(args), s7_cons(sc, s7_cadr(args), s7_cons(sc, s7_caddr(args), s7_cons(sc, s7_cadddr(args), s7_nil(sc))))));
}

s7_pointer g_append_2(s7_scheme *sc, s7_pointer args)
{
  return(s7_append(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_leq_2(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7i_leq_b_7pp(sc, s7_car(args), s7_cadr(args))));
}

s7_pointer g_geq_2(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7i_geq_b_7pp(sc, s7_car(args), s7_cadr(args))));
}

s7_pointer g_num_eq_2(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7i_num_eq_b_7pp(sc, s7_car(args), s7_cadr(args))));
}

s7_pointer g_less_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_lt_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_num_eq_xi(s7_scheme *sc, s7_pointer args)
{
  return(s7i_num_eq_xx(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_num_eq_ix(s7_scheme *sc, s7_pointer args)
{
  return(s7i_num_eq_xx(sc, s7_cadr(args), s7_car(args)));
}

s7_pointer g_memq_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_memq_2_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_memq_4(s7_scheme *sc, s7_pointer args)
{
  return(s7i_memq_4_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_add_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_add_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_add_2_wrapped(s7_scheme *sc, s7_pointer args)
{
  return(s7i_add_p_pp_wrapped(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_add_3(s7_scheme *sc, s7_pointer args)
{
  return(s7i_add_p_ppp(sc, s7_car(args), s7_cadr(args), s7_caddr(args)));
}

s7_pointer g_add_3_wrapped(s7_scheme *sc, s7_pointer args)
{
  return(s7i_add_p_ppp_wrapped(sc, s7_car(args), s7_cadr(args), s7_caddr(args)));
}

s7_pointer g_add_4(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7i_add_p_pp_wrapped(sc, s7_car(args), s7_cadr(args));
  s7_pointer p = s7_cddr(args);
  return(s7i_add_p_pp(sc, x, s7i_add_p_pp_wrapped(sc, s7_car(p), s7_cadr(p))));
}

s7_pointer g_subtract_1(s7_scheme *sc, s7_pointer args)
{
  return(s7i_negate_p_p(sc, s7_car(args)));
}

s7_pointer g_subtract_1_wrapped(s7_scheme *sc, s7_pointer args)
{
  return(s7i_negate_p_p_wrapped(sc, s7_car(args)));
}

s7_pointer g_subtract_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_subtract_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_subtract_2_wrapped(s7_scheme *sc, s7_pointer args)
{
  return(s7i_subtract_p_pp_wrapped(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_subtract_3(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args);
  x = s7i_subtract_p_pp_wrapped(sc, x, s7_cadr(args));
  return(s7i_subtract_p_pp(sc, x, s7_caddr(args)));
}

s7_pointer g_abort(s7_scheme *sc, s7_pointer args)
{
  (void)sc;
  (void)args;
  abort();
  return(NULL);
}

s7_pointer g_multiply_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_multiply_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_multiply_2_wrapped(s7_scheme *sc, s7_pointer args)
{
  return(s7i_multiply_p_pp_wrapped(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_multiply_3(s7_scheme *sc, s7_pointer args)
{
  return(s7i_multiply_p_ppp(sc, s7_car(args), s7_cadr(args), s7_caddr(args)));
}

s7_pointer g_multiply_3_wrapped(s7_scheme *sc, s7_pointer args)
{
  return(s7i_multiply_p_ppp_wrapped(sc, s7_car(args), s7_cadr(args), s7_caddr(args)));
}

s7_pointer g_invert_1(s7_scheme *sc, s7_pointer args)
{
  return(s7i_invert_p_p(sc, s7_car(args)));
}

s7_pointer g_divide_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_divide_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_unlet_ref(s7_scheme *sc, s7_pointer args)
{
  return(s7i_initial_value(s7_cadr(args)));
}

s7_pointer g_sv_unlet_ref(s7_scheme *sc, s7_pointer args)
{
  return(s7i_initial_value(s7_car(args)));
}

s7_pointer g_rootlet(s7_scheme *sc, s7_pointer args)
{
  return(s7i_rootlet(sc));
}

s7_pointer g_unlet_disabled(s7_scheme *sc, s7_pointer args)
{
  return(s7i_unlet_disabled(sc));
}

s7_pointer g_curlet(s7_scheme *sc, s7_pointer unused_args)
{
  #define H_curlet "(curlet) returns the current definitions (symbol bindings)"
  #define Q_curlet s7_make_signature(sc, 1, sc->is_let_symbol)
  s7i_capture_let_counter_inc(sc);
  return(s7i_curlet(sc));
}

s7_pointer g_outlet(s7_scheme *sc, s7_pointer args)
{
  return(s7i_outlet_p_p(sc, s7_car(args)));
}

s7_pointer g_curlet_ref(s7_scheme *sc, s7_pointer args)
{
  return(s7i_lookup_p_p(sc, s7_cadr(args)));
}

s7_pointer g_quotient(s7_scheme *sc, s7_pointer args)
{
  return(s7i_quotient_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_remainder(s7_scheme *sc, s7_pointer args)
{
  return(s7i_remainder_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_modulo(s7_scheme *sc, s7_pointer args)
{
  return(s7i_modulo_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_outlet_unlet(s7_scheme *sc, s7_pointer args)
{
  return(s7i_curlet(sc));
}

s7_pointer g_error(s7_scheme *sc, s7_pointer args)
{
  if (s7_is_string(s7_car(args)))
    s7_error(sc, s7_make_symbol(sc, "no-catch"), args);
  s7_error(sc, s7_car(args), s7_cdr(args));
  return(s7_unspecified(sc));
}
