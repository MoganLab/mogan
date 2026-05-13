/* s7_liii_string.c - string utility implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_liii_string.h"
#include "s7.h"
#include "s7_internal_helpers.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Externally defined in s7.c - character cache */
extern s7_pointer *chars;

/* Helper function for out-of-range errors */
static s7_pointer string_ref_out_of_range(s7_scheme *sc, s7_int index, bool is_negative)
{
  return s7_out_of_range_error(sc, "string-ref", 2, s7_make_integer(sc, index),
                               is_negative ? "it is negative" : "it is too large");
}

static s7_pointer method_or_bust(s7_scheme *sc, s7_pointer obj, const char *name, const char *type_name)
{
  s7_pointer sym = s7_make_symbol(sc, name);
  s7_pointer func = s7_method(sc, obj, sym);
  if (func != s7_undefined(sc))
    return(s7_apply_function(sc, func, s7_cons(sc, obj, s7_nil(sc))));
  return(s7_wrong_type_arg_error(sc, name, 1, obj, type_name));
}

/* -------------------------------- string-ref -------------------------------- */

s7_pointer string_ref_1(s7_scheme *sc, s7_pointer strng, s7_pointer index)
{
  if (!s7_is_integer(index))
    return s7_wrong_type_arg_error(sc, "string-ref", 2, index, "an integer");

  s7_int ind = s7_integer(index);
  if (ind < 0)
    return string_ref_out_of_range(sc, ind, true);
  if (ind >= s7_string_length(strng))
    return string_ref_out_of_range(sc, ind, false);

  const char *str = s7_string(strng);
  return chars[((uint8_t *)str)[ind]];
}

s7_pointer g_string_ref(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  if (!s7_is_string(str))
    return method_or_bust(sc, str, "string-ref", "a string");
  return string_ref_1(sc, str, s7_cadr(args));
}

/* -------------------------------- string-set! -------------------------------- */

s7_pointer g_string_set(s7_scheme *sc, s7_pointer args)
{
  s7_pointer strng = s7_car(args);
  s7_pointer index = s7_cadr(args);

  if (!s7_is_string(strng))
    return method_or_bust(sc, strng, "string-set!", "a string");
  if (s7_is_immutable(strng))
    return s7_wrong_type_arg_error(sc, "string-set!", 1, strng, "a mutable string");
  if (!s7_is_integer(index))
    return s7_wrong_type_arg_error(sc, "string-set!", 2, index, "an integer");

  s7_int ind = s7_integer(index);
  if (ind < 0)
    return s7_out_of_range_error(sc, "string-set!", 2, index, "it is negative");
  if (ind >= s7_string_length(strng))
    return s7_out_of_range_error(sc, "string-set!", 2, index, "it is too large");

  s7_pointer c = s7_caddr(args);
  if (!s7_is_character(c))
    return s7_wrong_type_arg_error(sc, "string-set!", 3, c, "a character");

  char *str = (char *)s7_string(strng);
  str[ind] = (char)s7_character(c);
  return c;
}

/*---------------------------------string-length---------------------------------*/

s7_pointer g_string_length(s7_scheme *sc, s7_pointer args)
{
  #define H_string_length "(string-length str) returns the length of the string str"
  #define Q_string_length s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_string_symbol)
  s7_pointer str = s7_car(args);
  if (!s7_is_string(str))
    return(s7i_sole_arg_method_or_bust(sc, str, "string-length", args, "a string"));
  return(s7_make_integer(sc, s7_string_length(str)));
}
