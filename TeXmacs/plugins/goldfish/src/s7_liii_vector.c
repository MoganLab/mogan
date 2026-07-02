/* s7_liii_vector.c - vector utility implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_liii_vector.h"
#include "s7_internal_helpers.h"
#include <string.h>

#ifndef WITH_PURE_S7
#define WITH_PURE_S7 0
s7_pointer g_make_vector(s7_scheme *sc, s7_pointer args)
{
  return(s7i_make_vector_1(sc, args, s7_make_symbol(sc, "make-vector")));
}

s7_pointer g_vector_fill(s7_scheme *sc, s7_pointer args)
{
  return(s7i_vector_fill_1(sc, s7_make_symbol(sc, "vector-fill!"), args));
}

#endif

s7_pointer g_is_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_vector(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "vector?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_is_float_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_float_vector(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "float-vector?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_is_int_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_int_vector(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "int-vector?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_is_byte_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_byte_vector(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "byte-vector?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_is_complex_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_complex_vector(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "complex-vector?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_string_to_byte_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  if (!s7_is_string(str))
    return(s7i_method_or_bust_p(sc, str, "string->byte-vector", "a string"));
  {
    s7_int len = s7_string_length(str);
    s7_pointer bv = s7_make_byte_vector(sc, len, 0, NULL);
    memcpy(s7_byte_vector_elements(bv), s7_string(str), len);
    return(bv);
  }
}

s7_pointer g_byte_vector_to_string(s7_scheme *sc, s7_pointer args)
{
  s7_pointer bv = s7_car(args);
  if (!s7_is_byte_vector(bv))
    return(s7i_method_or_bust_p(sc, bv, "byte-vector->string", "a byte-vector"));
  {
    s7_int len = s7_vector_length(bv);
    return(s7_make_string_with_length(sc, (const char *)s7_byte_vector_elements(bv), len));
  }
}

s7_pointer g_float_vector(s7_scheme *sc, s7_pointer args)
{
  s7_int len = s7_list_length(sc, args);
  if (len < 0)
    return(s7_error(sc, s7_make_symbol(sc, "read-error"),
                    s7_cons(sc, s7_make_string(sc, "float-vector contents list is not a proper list"), s7_nil(sc))));
  {
    s7_pointer vec = s7_make_float_vector(sc, len, 0, NULL);
    s7_int i = 0;
    if (len == 0) return(vec);
    for (s7_pointer nums = args; s7_is_pair(nums); nums = s7_cdr(nums), i++)
      {
        s7_pointer num = s7_car(nums);
        if (!s7_is_real(num))
          return(s7i_method_or_bust(sc, num, "float-vector", args, "a real", i + 1));
        s7_float_vector_set(vec, i, s7_real(num));
      }
    return(vec);
  }
}

s7_pointer g_int_vector(s7_scheme *sc, s7_pointer args)
{
  s7_int len = s7_list_length(sc, args);
  if (len < 0)
    return(s7_error(sc, s7_make_symbol(sc, "read-error"),
                    s7_cons(sc, s7_make_string(sc, "int-vector contents list is not a proper list"), s7_nil(sc))));
  {
    s7_pointer vec = s7_make_int_vector(sc, len, 0, NULL);
    s7_int i = 0;
    if (len == 0) return(vec);
    for (s7_pointer arglist = args; s7_is_pair(arglist); arglist = s7_cdr(arglist), i++)
      {
        s7_pointer num = s7_car(arglist);
        if (!s7_is_integer(num))
          return(s7i_method_or_bust(sc, num, "int-vector", args, "an integer", i + 1));
        s7_int_vector_set(vec, i, s7_integer(num));
      }
    return(vec);
  }
}

s7_pointer g_byte_vector(s7_scheme *sc, s7_pointer args)
{
  s7_int len = s7_list_length(sc, args);
  if (len < 0)
    return(s7_error(sc, s7_make_symbol(sc, "read-error"),
                    s7_cons(sc, s7_make_string(sc, "byte-vector contents list is not a proper list"), s7_nil(sc))));
  {
    s7_pointer vec = s7_make_byte_vector(sc, len, 0, NULL);
    s7_int i = 0;
    if (len == 0) return(vec);
    for (s7_pointer arglist = args; s7_is_pair(arglist); i++, arglist = s7_cdr(arglist))
      {
        s7_pointer byte = s7_car(arglist);
        s7_int b;
        if (!s7_is_integer(byte))
          return(s7i_method_or_bust(sc, byte, "byte-vector", args, "an integer", i + 1));
        b = s7_integer(byte);
        if ((b < 0) || (b > 255))
          return(s7_wrong_type_arg_error(sc, "byte-vector", i + 1, byte, "a byte"));
        s7_byte_vector_set(vec, i, (uint8_t)b);
      }
    return(vec);
  }
}

s7_pointer g_complex_vector(s7_scheme *sc, s7_pointer args)
{
  s7_int len = s7_list_length(sc, args);
  if (len < 0)
    return(s7_error(sc, s7_make_symbol(sc, "read-error"),
                    s7_cons(sc, s7_make_string(sc, "complex-vector contents list is not a proper list"), s7_nil(sc))));
  {
    s7_pointer vec = s7i_make_simple_complex_vector(sc, len);
    s7_int i = 0;
    if (len == 0) return(vec);
    for (s7_pointer arglist = args; s7_is_pair(arglist); arglist = s7_cdr(arglist), i++)
      {
        s7_pointer num = s7_car(arglist);
        if (!s7_is_number(num))
          return(s7i_method_or_bust(sc, num, "complex-vector", args, "a number", i + 1));
        s7_complex_vector_elements(vec)[i] = s7i_to_c_complex(num);
      }
    return(vec);
  }
}

s7_pointer g_vector_rank(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_sole_arg_method_or_bust(sc, vec, "vector-rank", args, "a vector"));
  return(s7_make_integer(sc, s7_vector_rank(vec)));
}

s7_pointer g_vector_dimension(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  s7_pointer dim = s7_cadr(args);
  s7_int n;

  if (!s7_is_vector(vec))
    return(s7i_method_or_bust(sc, vec, "vector-dimension", args, "a vector", 1));
  if (!s7_is_integer(dim))
    return(s7i_method_or_bust(sc, dim, "vector-dimension", args, "an integer", 2));

  n = s7_number_to_integer(sc, dim);
  if (n < 0)
    return(s7_out_of_range_error(sc, "vector-dimension", 2, dim, "it is negative"));
  if (n >= s7_vector_rank(vec))
    return(s7_out_of_range_error(sc, "vector-dimension", 2, dim, "it is too large"));
  return(s7_make_integer(sc, s7_vector_dimension(vec, n)));
}

s7_pointer g_vector_dimensions(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);

  if (!s7_is_vector(vec))
    return(s7i_sole_arg_method_or_bust(sc, vec, "vector-dimensions", args, "a vector"));

  {
    s7_int rank = s7_vector_rank(vec);
    if (rank == 1)
      return(s7_cons(sc, s7_make_integer(sc, s7_vector_length(vec)), s7_nil(sc)));

    {
      s7_pointer result = s7_nil(sc);
      for (s7_int i = rank - 1; i >= 0; i--)
        result = s7_cons(sc, s7_make_integer(sc, s7_vector_dimension(vec, i)), result);
      return(result);
    }
  }
}

s7_pointer g_is_subvector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7i_is_subvector(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "subvector?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_subvector_position(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (!s7i_is_subvector(p))
    return(s7i_sole_arg_method_or_bust(sc, p, "subvector-position", args, "a subvector"));
  return(s7_make_integer(sc, s7i_subvector_position(p)));
}

s7_pointer g_subvector_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (!s7i_is_subvector(p))
    return(s7i_sole_arg_method_or_bust(sc, p, "subvector-vector", args, "a subvector"));
  return(s7i_subvector_vector(sc, p));
}

s7_pointer g_vector_typer(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_sole_arg_method_or_bust(sc, vec, "vector-typer", args, "a vector"));
  if (s7i_is_typed_t_vector(vec)) return(s7i_typed_vector_typer(sc, vec));
  if (s7_is_float_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "float?")));
  if (s7_is_int_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "integer?")));
  if (s7_is_byte_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "byte?")));
  if (s7_is_complex_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "number?")));
  return(s7_f(sc));
}

s7_pointer g_vector(s7_scheme *sc, s7_pointer args)
{
  s7_int len = s7_list_length(sc, args);
  if (len < 0)
    return(s7_error(sc, s7_make_symbol(sc, "read-error"),
                    s7_cons(sc, s7_make_string(sc, "vector contents list is not a proper list"), s7_nil(sc))));
  {
    s7_pointer vec = s7_make_vector(sc, len);
    s7_int i = 0;
    for (s7_pointer p = args; s7_is_pair(p); p = s7_cdr(p), i++)
      s7_vector_set(sc, vec, i, s7_car(p));
    return(vec);
  }
}

s7_pointer g_vector_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_make_vector(sc, 2);
  s7_vector_set(sc, vec, 0, s7_car(args));
  s7_vector_set(sc, vec, 1, s7_cadr(args));
  return(vec);
}

s7_pointer g_vector_3(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_make_vector(sc, 3);
  s7_vector_set(sc, vec, 0, s7_car(args));
  s7_vector_set(sc, vec, 1, s7_cadr(args));
  s7_vector_set(sc, vec, 2, s7_caddr(args));
  return(vec);
}

s7_pointer g_vector_ref(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_method_or_bust(sc, vec, "vector-ref", args, "a vector", 1));
  return(s7i_vector_ref_1(sc, vec, s7_cdr(args)));
}

s7_pointer g_vector_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_cv_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_complex_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_fv_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_float_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_iv_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_int_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_cv_set_3(s7_scheme *sc, s7_pointer args)
{
  return(s7i_complex_vector_set_p_ppp(sc, s7_car(args), s7_cadr(args), s7_caddr(args)));
}

s7_pointer g_list_to_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (s7_is_null(sc, lst))
    return(s7_make_vector(sc, 0));
  if (!s7_is_proper_list(sc, lst))
    return(s7i_method_or_bust_p(sc, lst, "list->vector", "a proper list"));
  return(g_vector(sc, lst));
}

#if !WITH_PURE_S7

s7_pointer g_vector_length(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_sole_arg_method_or_bust(sc, vec, "vector-length", args, "a vector"));
  return(s7_make_integer(sc, s7_vector_length(vec)));
}

s7_int vector_length_i_7p(s7_scheme *sc, s7_pointer vec)
{
  if (!s7_is_vector(vec))
    return(s7_integer(s7i_method_or_bust_p(sc, vec, "vector-length", "a vector")));
  return(s7_vector_length(vec));
}

s7_pointer vector_length_p_p(s7_scheme *sc, s7_pointer vec)
{
  if (!s7_is_vector(vec))
    return(s7i_method_or_bust_p(sc, vec, "vector-length", "a vector"));
  return(s7_make_integer(sc, s7_vector_length(vec)));
}

#endif
