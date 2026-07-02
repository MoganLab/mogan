/* s7_liii_bitwise.c - bitwise implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_liii_bitwise.h"
#include <limits.h>
#include <stdint.h>

#ifndef S7_INT_BITS
  #define S7_INT_BITS ((int)(sizeof(s7_int) * CHAR_BIT - 1))
#endif

#define S7_INT64_MAX 9223372036854775807LL
#define S7_INT64_MIN (int64_t)(-S7_INT64_MAX - 1LL)

static s7_pointer wrong_integer_arg(s7_scheme *sc, const char *caller, s7_int arg_n, s7_pointer arg)
{
  return s7_wrong_type_arg_error(sc, caller, arg_n, arg, "an integer");
}

s7_pointer g_logior(s7_scheme *sc, s7_pointer args)
{
  s7_int result = 0;
  s7_int pos = 1;
  for (s7_pointer x = args; s7_is_pair(x); x = s7_cdr(x), pos++)
    {
      s7_pointer v = s7_car(x);
      if (!s7_is_integer(v))
        return wrong_integer_arg(sc, "logior", pos, v);
      result |= s7_integer(v);
    }
  return s7_make_integer(sc, result);
}

s7_int logior_i_ii(s7_int i1, s7_int i2) {return(i1 | i2);} 
s7_int logior_i_iii(s7_int i1, s7_int i2, s7_int i3) {return(i1 | i2 | i3);} 
s7_pointer g_logior_ii(s7_scheme *sc, s7_pointer args) {return(s7_make_integer(sc, s7_integer(s7_car(args)) | s7_integer(s7_cadr(args))));}

s7_pointer g_logior_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer arg1 = s7_car(args), arg2 = s7_cadr(args);
  if ((s7_is_integer(arg1)) && (s7_is_integer(arg2)))
    return s7_make_integer(sc, s7_integer(arg1) | s7_integer(arg2));
  return g_logior(sc, args);
}

s7_pointer logior_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr)
{
  (void)sc; (void)args; (void)expr;
  return func;
}

s7_pointer g_logxor(s7_scheme *sc, s7_pointer args)
{
  s7_int result = 0;
  s7_int pos = 1;
  for (s7_pointer x = args; s7_is_pair(x); x = s7_cdr(x), pos++)
    {
      s7_pointer v = s7_car(x);
      if (!s7_is_integer(v))
        return wrong_integer_arg(sc, "logxor", pos, v);
      result ^= s7_integer(v);
    }
  return s7_make_integer(sc, result);
}

s7_int logxor_i_ii(s7_int i1, s7_int i2) {return(i1 ^ i2);} 
s7_int logxor_i_iii(s7_int i1, s7_int i2, s7_int i3) {return(i1 ^ i2 ^ i3);} 

s7_pointer g_logxor_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer arg1 = s7_car(args), arg2 = s7_cadr(args);
  if ((s7_is_integer(arg1)) && (s7_is_integer(arg2)))
    return s7_make_integer(sc, s7_integer(arg1) ^ s7_integer(arg2));
  return g_logxor(sc, args);
}

s7_pointer logxor_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr)
{
  (void)sc; (void)args; (void)expr;
  return func;
}

s7_pointer g_logand(s7_scheme *sc, s7_pointer args)
{
  s7_int result = -1;
  s7_int pos = 1;
  for (s7_pointer x = args; s7_is_pair(x); x = s7_cdr(x), pos++)
    {
      s7_pointer v = s7_car(x);
      if (!s7_is_integer(v))
        return wrong_integer_arg(sc, "logand", pos, v);
      result &= s7_integer(v);
    }
  return s7_make_integer(sc, result);
}

s7_int logand_i_ii(s7_int i1, s7_int i2) {return(i1 & i2);} 
s7_int logand_i_iii(s7_int i1, s7_int i2, s7_int i3) {return(i1 & i2 & i3);} 
s7_pointer g_logand_ii(s7_scheme *sc, s7_pointer args) {return(s7_make_integer(sc, s7_integer(s7_car(args)) & s7_integer(s7_cadr(args))));}

s7_pointer g_logand_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer arg1 = s7_car(args), arg2 = s7_cadr(args);
  if ((s7_is_integer(arg1)) && (s7_is_integer(arg2)))
    return s7_make_integer(sc, s7_integer(arg1) & s7_integer(arg2));
  return g_logand(sc, args);
}

s7_pointer logand_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr)
{
  (void)sc; (void)args; (void)expr;
  return func;
}

s7_pointer g_lognot(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args);
  if (!s7_is_integer(x))
    return wrong_integer_arg(sc, "lognot", 1, x);
  return s7_make_integer(sc, ~s7_integer(x));
}

s7_int lognot_i_i(s7_int i1) {return(~i1);} 

s7_pointer g_logbit(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args), y = s7_cadr(args);
  s7_int index;

  if (!s7_is_integer(x))
    return wrong_integer_arg(sc, "logbit?", 1, x);
  if (!s7_is_integer(y))
    return wrong_integer_arg(sc, "logbit?", 2, y);

  index = s7_integer(y);
  if (index < 0)
    return s7_out_of_range_error(sc, "logbit?", 2, y, "it is negative");

  if (index >= S7_INT_BITS)
    return s7_make_boolean(sc, s7_integer(x) < 0);

  return s7_make_boolean(sc, ((((s7_int)(1LL << (s7_int)index)) & (s7_int)s7_integer(x)) != 0));
}

bool logbit_b_7ii(s7_scheme *sc, s7_int i1, s7_int i2)
{
  if (i2 < 0)
    {
      s7_out_of_range_error(sc, "logbit?", 2, s7_make_integer(sc, i2), "it is negative");
      return false;
    }
  if (i2 >= S7_INT_BITS) return(i1 < 0);
  return ((((s7_int)(1LL << (s7_int)i2)) & (s7_int)i1) != 0);
}

bool logbit_b_7pp(s7_scheme *sc, s7_pointer i1, s7_pointer i2)
{
  if (!s7_is_integer(i1))
    {
      wrong_integer_arg(sc, "logbit?", 1, i1);
      return false;
    }
  if (!s7_is_integer(i2))
    {
      wrong_integer_arg(sc, "logbit?", 2, i2);
      return false;
    }
  return logbit_b_7ii(sc, s7_integer(i1), s7_integer(i2));
}

static s7_int c_ash(s7_scheme *sc, s7_int arg1, s7_int arg2)
{
  if (arg2 >= S7_INT_BITS)
    {
      if ((arg1 == -1) && (arg2 == 63))
        return S7_INT64_MIN;
      if (arg1 == 0) return 0;
      s7_out_of_range_error(sc, "ash", 2, s7_make_integer(sc, arg2), "it is too large");
    }
  if (arg2 < 0)
    {
      if (arg2 < -S7_INT_BITS)
        return((arg1 < 0) ? -1 : 0);
      return(arg1 >> -arg2);
    }
  return(arg1 << arg2);
}

s7_pointer g_ash(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args), y = s7_cadr(args);

  if (!s7_is_integer(x))
    return wrong_integer_arg(sc, "ash", 1, x);
  if (!s7_is_integer(y))
    return wrong_integer_arg(sc, "ash", 2, y);
  return s7_make_integer(sc, c_ash(sc, s7_integer(x), s7_integer(y)));
}

s7_int lsh_i_ii_unchecked(s7_int i1, s7_int i2) {return(i1 << i2);} 
s7_int rsh_i_ii_unchecked(s7_int i1, s7_int i2) {return(i1 >> (-i2));}
s7_int rsh_i_i2_direct(s7_int i1, s7_int unused_i2) { (void)unused_i2; return(i1 >> 1);} 

s7_int ash_i_7ii(s7_scheme *sc, s7_int i1, s7_int i2) {return(c_ash(sc, i1, i2));}
s7_pointer g_ash_ii(s7_scheme *sc, s7_pointer args) {return(s7_make_integer(sc, c_ash(sc, s7_integer(s7_car(args)), s7_integer(s7_cadr(args)))));} 

s7_pointer g_ash_ic(s7_scheme *sc, s7_pointer args)
{
  s7_pointer x = s7_car(args);
  s7_int y = s7_integer(s7_cadr(args));
  if (!s7_is_integer(x))
    return wrong_integer_arg(sc, "ash", 1, x);
  return s7_make_integer(sc, c_ash(sc, s7_integer(x), y));
}

s7_pointer ash_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr)
{
  (void)sc; (void)args; (void)expr;
  return func;
}
