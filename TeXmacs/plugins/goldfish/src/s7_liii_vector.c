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

/* Externally defined in s7.c - permanent error description strings */
extern s7_pointer it_is_negative_string, it_is_too_large_string, it_is_too_small_string, immutable_error_string;

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
    return(s7i_vector_method_or_bust_p(sc, str, "string->byte-vector", "a string"));
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
    return(s7i_vector_method_or_bust_p(sc, bv, "byte-vector->string", "a byte-vector"));
  {
    s7_int len = s7_vector_length(bv);
    return(s7_make_string_with_length(sc, (const char *)s7_byte_vector_elements(bv), len));
  }
}

s7_pointer g_make_byte_vector(s7_scheme *sc, s7_pointer args)
{
  s7_int len = 0, ib = 0;
  s7_pointer size = s7_car(args), init;

  if (!s7_is_pair(size))
    {
      if (!s7_is_integer(size))
        return(s7i_vector_method_or_bust(sc, size, "make-byte-vector", args, "an integer", 1));
      len = s7_number_to_integer(sc, size);
      if (len < 0)
        return(s7_out_of_range_error(sc, "make-byte-vector", 1, size, "it is negative"));
      if (len > s7i_max_vector_length(sc))
        return(s7_out_of_range_error(sc, "make-byte-vector", 1, size, "it is too large"));
    }
  if (s7_is_pair(s7_cdr(args)))
    {
      init = s7_cadr(args);
      if (!s7_is_integer(init))
        return(s7i_vector_method_or_bust(sc, init, "make-byte-vector", args, "an integer", 2));
      ib = s7_number_to_integer(sc, init);
      if ((ib < 0) || (ib > 255))
        return(s7_type_error(sc, "make-byte-vector", 2, init, "a byte"));
    }
  else init = s7_make_integer(sc, 0);

  if (!s7_is_integer(size))
    return(s7i_make_vector_1(sc, s7i_set_plist_2(sc, size, init), s7_make_symbol(sc, "make-byte-vector")));
  {
    s7_pointer result = s7_make_byte_vector(sc, len, 0, NULL);
    if (len > 0)
      memset((void *)s7_byte_vector_elements(result), (uint8_t)ib, len);
    return(result);
  }
}

s7_pointer g_make_int_vector(s7_scheme *sc, s7_pointer args)
{
  s7_int len = 0;
  s7_pointer size = s7_car(args), init;

  if (!s7_is_pair(size))
    {
      if (!s7_is_integer(size))
        return(s7i_vector_method_or_bust(sc, size, "make-int-vector", args, "an integer", 1));
      len = s7_number_to_integer(sc, size);
      if (len < 0)
        return(s7_out_of_range_error(sc, "make-int-vector", 1, size, "it is negative"));
      if (len > s7i_max_vector_length(sc))
        return(s7_out_of_range_error(sc, "make-int-vector", 1, size, "it is too large"));
    }
  if (s7_is_pair(s7_cdr(args)))
    {
      init = s7_cadr(args);
      if (!s7_is_integer(init))
        return(s7i_vector_method_or_bust(sc, init, "make-int-vector", args, "an integer", 2));
    }
  else init = s7_make_integer(sc, 0);

  if (!s7_is_integer(size))
    return(s7i_make_vector_1(sc, s7i_set_plist_2(sc, size, init), s7_make_symbol(sc, "make-int-vector")));
  {
    s7_pointer result = s7_make_int_vector(sc, len, 0, NULL);
    s7_int init_val = s7_number_to_integer(sc, init);
    if (len > 0 && init_val != 0)
      {
        s7_int *ints = s7_int_vector_elements(result);
        for (s7_int i = 0; i < len; i++) ints[i] = init_val;
      }
    return(result);
  }
}

s7_pointer g_make_float_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer size = s7_car(args);
  s7_int len = 0;

  if (s7_is_pair(size))
    {
      s7_pointer init;
      if (s7_is_pair(s7_cdr(args)))
        {
          init = s7_cadr(args);
          if (!s7_is_real(init))
            return(s7i_vector_method_or_bust(sc, init, "make-float-vector", args, "a real", 2));
        }
      else init = s7_make_real(sc, 0.0);
      return(s7i_make_vector_1(sc, s7i_set_plist_2(sc, size, init), s7_make_symbol(sc, "make-float-vector")));
    }

  if (!s7_is_integer(size))
    return(s7i_vector_method_or_bust(sc, size, "make-float-vector", args, "an integer or a list of integers", 1));

  len = s7_number_to_integer(sc, size);
  if (len < 0)
    return(s7_out_of_range_error(sc, "make-float-vector", 1, size, "it is negative"));
  if (len > s7i_max_vector_length(sc))
    return(s7_out_of_range_error(sc, "make-float-vector", 1, size, "it is too large"));

  if (s7_is_pair(s7_cdr(args)))
    {
      s7_pointer init = s7_cadr(args);
      if (!s7_is_real(init))
        return(s7i_vector_method_or_bust(sc, init, "make-float-vector", args, "a real", 2));
      {
        s7_pointer vect = s7_make_float_vector(sc, len, 0, NULL);
        s7_double d = s7_number_to_real(sc, init);
        if (len > 0 && d != 0.0)
          {
            s7_double *floats = s7_float_vector_elements(vect);
            for (s7_int i = 0; i < len; i++) floats[i] = d;
          }
        return(vect);
      }
    }

  return(s7_make_float_vector(sc, len, 0, NULL));
}

s7_pointer g_make_complex_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer size = s7_car(args);
  s7_int len = 0;

  if (s7_is_pair(size))
    {
      s7_pointer init;
      if (s7_is_pair(s7_cdr(args)))
        {
          init = s7_cadr(args);
          if (!s7_is_number(init))
            return(s7i_vector_method_or_bust(sc, init, "make-complex-vector", args, "a number", 2));
        }
      else init = s7_make_real(sc, 0.0);
      return(s7i_make_vector_1(sc, s7i_set_plist_2(sc, size, init), s7_make_symbol(sc, "make-complex-vector")));
    }

  if (!s7_is_integer(size))
    return(s7i_vector_method_or_bust(sc, size, "make-complex-vector", args, "an integer or a list of integers", 1));

  len = s7_number_to_integer(sc, size);
  if (len < 0)
    return(s7_out_of_range_error(sc, "make-complex-vector", 1, size, "it is negative"));
  if (len > s7i_max_vector_length(sc))
    return(s7_out_of_range_error(sc, "make-complex-vector", 1, size, "it is too large"));

  if (s7_is_pair(s7_cdr(args)))
    {
      s7_pointer init = s7_cadr(args);
      if (!s7_is_number(init))
        return(s7i_vector_method_or_bust(sc, init, "make-complex-vector", args, "a number", 2));
      {
        s7_pointer vect = s7_make_complex_vector(sc, len, 0, NULL);
        s7_vector_fill(sc, vect, init);
        return(vect);
      }
    }

  return(s7_make_complex_vector(sc, len, 0, NULL));
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
          return(s7i_vector_method_or_bust(sc, num, "float-vector", args, "a real", i + 1));
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
          return(s7i_vector_method_or_bust(sc, num, "int-vector", args, "an integer", i + 1));
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
          return(s7i_vector_method_or_bust(sc, byte, "byte-vector", args, "an integer", i + 1));
        b = s7_integer(byte);
        if ((b < 0) || (b > 255))
          return(s7_type_error(sc, "byte-vector", i + 1, byte, "a byte"));
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
          return(s7i_vector_method_or_bust(sc, num, "complex-vector", args, "a number", i + 1));
        s7_complex_vector_elements(vec)[i] = s7i_to_c_complex(num);
      }
    return(vec);
  }
}

s7_pointer g_vector_rank(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_vector_sole_arg_method_or_bust(sc, vec, "vector-rank", args, "a vector"));
  return(s7_make_integer(sc, s7_vector_rank(vec)));
}

s7_pointer g_vector_dimension(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  s7_pointer dim = s7_cadr(args);
  s7_int n;

  if (!s7_is_vector(vec))
    return(s7i_vector_method_or_bust(sc, vec, "vector-dimension", args, "a vector", 1));
  if (!s7_is_integer(dim))
    return(s7i_vector_method_or_bust(sc, dim, "vector-dimension", args, "an integer", 2));

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
    return(s7i_vector_sole_arg_method_or_bust(sc, vec, "vector-dimensions", args, "a vector"));

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

s7_pointer g_subvector(s7_scheme *sc, s7_pointer args)
{
  return(s7i_subvector_1(sc, args));
}

s7_pointer g_subvector_position(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (!s7i_is_subvector(p))
    return(s7i_vector_sole_arg_method_or_bust(sc, p, "subvector-position", args, "a subvector"));
  return(s7_make_integer(sc, s7i_subvector_position(p)));
}

s7_pointer g_subvector_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (!s7i_is_subvector(p))
    return(s7i_vector_sole_arg_method_or_bust(sc, p, "subvector-vector", args, "a subvector"));
  return(s7i_subvector_vector(sc, p));
}

s7_pointer g_vector_typer(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_vector_sole_arg_method_or_bust(sc, vec, "vector-typer", args, "a vector"));
  if (s7i_is_typed_t_vector(vec)) return(s7i_typed_vector_typer(sc, vec));
  if (s7_is_float_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "float?")));
  if (s7_is_int_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "integer?")));
  if (s7_is_byte_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "byte?")));
  if (s7_is_complex_vector(vec)) return(s7_symbol_value(sc, s7_make_symbol(sc, "number?")));
  return(s7_f(sc));
}

s7_pointer g_set_vector_typer(s7_scheme *sc, s7_pointer args)
{
  return(s7i_set_vector_typer_1(sc, args));
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

s7_pointer g_vector_append(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = args;
  if (s7_is_null(sc, args))
    return(s7_make_vector(sc, 0));

  if ((s7_is_null(sc, s7_cdr(args))) &&
      (s7i_is_any_vector(s7_car(args))))
    return(s7_vector_copy(sc, s7_car(args)));

  for (int32_t i = 0; s7_is_pair(p); p = s7_cdr(p), i++)
    {
      const s7_pointer vect = s7_car(p);
      if (!s7i_is_any_vector(vect))
        {
          s7_pointer func = s7_method(sc, vect, s7_make_symbol(sc, "vector-append"));
          if (func != s7_undefined(sc))
            {
              if (i == 0)
                return(s7_apply_function(sc, func, args));

              /* args may live in evaluator-recycled cells, so keep func and p in our own
               * anchor pair, with the working list as the anchor's cdr; everything stays
               * GC-reachable across recursive append and method apply */
              s7_pointer anchor = s7_cons(sc, s7_cons(sc, func, p), s7_nil(sc));
              s7_gc_protect_via_stack(sc, anchor);
              s7_pointer arglist = args;
              for (int32_t k = 0; k < i; k++, arglist = s7_cdr(arglist))
                s7_set_cdr(anchor, s7_cons(sc, s7_car(arglist), s7_cdr(anchor)));
              s7_set_cdr(anchor, s7_reverse(sc, s7_cdr(anchor)));
              s7_set_cdr(anchor, g_vector_append(sc, s7_cdr(anchor)));
              s7_set_cdr(anchor, s7_cons(sc, s7_cdr(anchor), p));
              s7_pointer result = s7_apply_function(sc, func, s7_cdr(anchor));
              s7_gc_unprotect_via_stack(sc, anchor);
              return(result);
            }
          return(s7_type_error(sc, "vector-append", i + 1, vect, "a vector"));
        }
    }
  return(s7i_vector_append(sc, args, (uint8_t)s7i_type(s7_car(args)), s7_make_symbol(sc, "vector-append")));
}

s7_pointer g_vector_ref(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_vector_method_or_bust(sc, vec, "vector-ref", args, "a vector", 1));
  return(s7i_vector_ref_1(sc, vec, s7_cdr(args)));
}

s7_pointer g_vector_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_vector_ref_3(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer vec = s7_car(args);
  s7_pointer i1, i2;
  s7_int ix, iy;

  if (!s7i_is_any_vector(vec)) return(g_vector_ref(sc, args));
  if (s7_vector_rank(vec) != 2) return(g_vector_ref(sc, args));
  i1 = s7_cadr(args);
  if (!s7_is_integer(i1)) return(g_vector_ref(sc, args));
  i2 = s7_caddr(args);
  if (!s7_is_integer(i2)) return(g_vector_ref(sc, args));
  ix = s7_number_to_integer(sc, i1);
  iy = s7_number_to_integer(sc, i2);
  if ((ix >= 0) && (iy >= 0) &&
      (ix < s7_vector_dimension(vec, 0)) && (iy < s7_vector_dimension(vec, 1)))
    {
      s7_int index = (ix * s7i_vector_offset(vec, 0)) + iy;
      return(s7i_vector_getter_ref(sc, vec, index));
    }
  return(g_vector_ref(sc, args));
}

s7_pointer g_vector_set(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer vec = s7_car(args);
  s7_pointer val;
  s7_int index;

  if (!s7i_is_any_vector(vec))
    return(s7i_vector_method_or_bust(sc, vec, "vector-set!", args, "a vector", 1));
  if (s7i_is_immutable_vector(vec))
    immutable_object_error_nr(sc, s7i_set_elist_3(sc, immutable_error_string, s7_make_symbol(sc, "vector-set!"), vec));
  if (s7_vector_length(vec) == 0)
    out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-set!"), s7i_wrap_integer(sc, 1), vec, it_is_too_large_string);

  if (s7_vector_rank(vec) > 1)
    {
      s7_int i;
      s7_pointer index_list;
      index = 0;
      s7_int rank = s7_vector_rank(vec);
      for (index_list = s7_cdr(args), i = 0; (s7_is_pair(s7_cdr(index_list))) && (i < rank); index_list = s7_cdr(index_list), i++)
        {
          s7_int n;
          const s7_pointer ind = s7_car(index_list);
          if (!s7_is_integer(ind))
            return(s7i_vector_method_or_bust(sc, ind, "vector-set!", args, "an integer", i + 2));
          n = s7_number_to_integer(sc, ind);
          if ((n < 0) || (n >= s7_vector_dimension(vec, i)))
            out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-set!"), s7i_wrap_integer(sc, i + 2), ind, (n < 0) ? it_is_negative_string : it_is_too_large_string);
          index += n * s7i_vector_offset(vec, i);
        }
      if (!s7_is_null(sc, s7_cdr(index_list)))
        return(s7_wrong_number_of_args_error(sc, "vector-set!", args));
      if (i != rank)
        return(s7_wrong_number_of_args_error(sc, "vector-set!", args));

      val = s7_car(index_list);
    }
  else
    {
      const s7_pointer ind = s7_cadr(args);
      if (!s7_is_integer(ind))
        return(s7i_vector_method_or_bust(sc, ind, "vector-set!", args, "an integer", 2));
      index = s7_number_to_integer(sc, ind);
      if ((index < 0) || (index >= s7_vector_length(vec)))
        out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-set!"), s7i_wrap_integer(sc, 2), ind, (index < 0) ? it_is_negative_string : it_is_too_large_string);
      if (!s7_is_null(sc, s7_cdddr(args)))
        {
          const s7_pointer new_vec = s7i_vector_getter_ref(sc, vec, index);
          if (!s7i_is_any_vector(new_vec))
            return(s7_wrong_number_of_args_error(sc, "vector-set!", args));
          return(g_vector_set(sc, s7_cons(sc, new_vec, s7_cddr(args))));
        }
      val = s7_caddr(args);
    }
  if (s7i_is_typed_t_vector(vec))
    return(s7i_typed_vector_setter(sc, vec, index, val));
  if (s7i_is_t_vector(vec))
    s7i_vector_element_set(vec, index, val);
  else s7i_vector_setter_set(sc, vec, index, val);
  return(val);
}

s7_pointer g_vector_set_3(s7_scheme *sc, s7_pointer args)
{
  /* (vector-set! vector index value) */
  const s7_pointer vec = s7_car(args);
  s7_pointer ind;
  s7_int index;

  if (!s7i_is_any_vector(vec))
    return(g_vector_set(sc, args));
  if (s7i_is_immutable_vector(vec))
    immutable_object_error_nr(sc, s7i_set_elist_3(sc, immutable_error_string, s7_make_symbol(sc, "vector-set!"), vec));
  if (s7_vector_rank(vec) > 1)
    return(g_vector_set(sc, args));

  ind = s7_cadr(args);
  if (!s7_is_integer(ind))
    return(g_vector_set(sc, args));
  index = s7_number_to_integer(sc, ind);
  if ((index < 0) || (index >= s7_vector_length(vec)))
    out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-set!"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index), (index < 0) ? it_is_negative_string : it_is_too_large_string);
  {
    s7_pointer val = s7_caddr(args);
    if (s7i_is_typed_t_vector(vec))
      return(s7i_typed_vector_setter(sc, vec, index, val));
    if (s7i_is_t_vector(vec))
      s7i_vector_element_set(vec, index, val);
    else s7i_vector_setter_set(sc, vec, index, val);
    return(val);
  }
}

s7_pointer g_vector_set_4(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer vec = s7_car(args), ip1 = s7_cadr(args), ip2 = s7_caddr(args);
  s7_pointer val;
  s7_int i1, i2;
  if ((!s7i_is_any_vector(vec)) ||
      (s7_vector_rank(vec) != 2) || (s7i_is_immutable_vector(vec)) ||
      (!s7_is_integer(ip1)) || (!s7_is_integer(ip2)))
    return(g_vector_set(sc, args));
  i1 = s7_number_to_integer(sc, ip1);
  i2 = s7_number_to_integer(sc, ip2);
  if ((i1 < 0) || (i2 < 0) ||
      (i1 >= s7_vector_dimension(vec, 0)) || (i2 >= s7_vector_dimension(vec, 1)))
    return(g_vector_set(sc, args));
  val = s7_cadddr(args);
  if (s7i_is_typed_t_vector(vec))
    return(s7i_typed_vector_setter(sc, vec, i2 + (i1 * s7i_vector_offset(vec, 0)), val));
  if (s7i_is_t_vector(vec))
    s7i_vector_element_set(vec, i2 + (i1 * s7i_vector_offset(vec, 0)), val);
  else s7i_vector_setter_set(sc, vec, i2 + (i1 * s7i_vector_offset(vec, 0)), val);
  return(val);
}

s7_pointer g_complex_vector_ref(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_ref_complex(sc, args));
}

s7_pointer g_cv_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_complex_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_float_vector_ref(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_ref_float(sc, args));
}

s7_pointer g_fv_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_float_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_fv_ref_3(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer fv = s7_car(args);
  s7_pointer index;
  s7_int ind1, ind2;
  if (!s7_is_float_vector(fv))
    return(s7i_vector_method_or_bust(sc, fv, "float-vector-ref", args, "a float-vector", 1));
  if (s7_vector_rank(fv) != 2)
    return(s7i_univect_ref_float(sc, args));
  index = s7_cadr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "float-vector-ref", args, "an integer", 2));
  ind1 = s7_number_to_integer(sc, index);
  if ((ind1 < 0) || (ind1 >= s7_vector_dimension(fv, 0)))
    return(s7_out_of_range_error(sc, "float-vector-ref", 2, index, (ind1 < 0) ? "it is negative" : "it is too large"));
  index = s7_caddr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "float-vector-ref", args, "an integer", 3));
  ind2 = s7_number_to_integer(sc, index);
  if ((ind2 < 0) || (ind2 >= s7_vector_dimension(fv, 1)))
    return(s7_out_of_range_error(sc, "float-vector-ref", 3, index, (ind2 < 0) ? "it is negative" : "it is too large"));
  ind1 = ind1 * s7i_vector_offset(fv, 0) + ind2;
  return(s7_make_real(sc, s7_float_vector_ref(fv, ind1)));
}

s7_pointer g_float_vector_set(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_set_float(sc, args));
}

s7_pointer g_fv_set_3(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer fv = s7_car(args);
  s7_pointer index;
  if (!s7_is_float_vector(fv))
    return(s7i_vector_method_or_bust(sc, fv, "float-vector-set!", args, "a float-vector", 1));
  if (s7_vector_rank(fv) != 1)
    return(s7i_univect_set_float(sc, args));
  if (s7i_is_immutable_vector(fv))
    immutable_object_error_nr(sc, s7i_set_elist_3(sc, immutable_error_string, s7_make_symbol(sc, "float-vector-set!"), fv));
  index = s7_cadr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "float-vector-set!", args, "an integer", 2));
  {
    s7_int ind = s7_number_to_integer(sc, index);
    s7_pointer value = s7_caddr(args);
    if ((ind < 0) || (ind >= s7_vector_length(fv)))
      return(s7_out_of_range_error(sc, "float-vector-set!", 2, index, (ind < 0) ? "it is negative" : "it is too large"));
    if (!s7_is_real(value))
      return(s7i_vector_method_or_bust(sc, value, "float-vector-set!", args, "a real", 3));
    s7_float_vector_set(fv, ind, s7_number_to_real(sc, value));
    return(value);
  }
}

s7_pointer g_int_vector_ref(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_ref_int(sc, args));
}

s7_pointer g_iv_ref_2(s7_scheme *sc, s7_pointer args)
{
  return(s7i_int_vector_ref_p_pp(sc, s7_car(args), s7_cadr(args)));
}

s7_pointer g_iv_ref_3(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer ivec = s7_car(args);
  s7_pointer index;
  s7_int ind1, ind2;
  if (!s7_is_int_vector(ivec))
    return(s7i_vector_method_or_bust(sc, ivec, "int-vector-ref", args, "an int-vector", 1));
  if (s7_vector_rank(ivec) != 2)
    return(s7i_univect_ref_int(sc, args));
  index = s7_cadr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "int-vector-ref", args, "an integer", 2));
  ind1 = s7_number_to_integer(sc, index);
  if ((ind1 < 0) || (ind1 >= s7_vector_dimension(ivec, 0)))
    return(s7_out_of_range_error(sc, "int-vector-ref", 2, index, (ind1 < 0) ? "it is negative" : "it is too large"));
  index = s7_caddr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "int-vector-ref", args, "an integer", 3));
  ind2 = s7_number_to_integer(sc, index);
  if ((ind2 < 0) || (ind2 >= s7_vector_dimension(ivec, 1)))
    return(s7_out_of_range_error(sc, "int-vector-ref", 3, index, (ind2 < 0) ? "it is negative" : "it is too large"));
  ind1 = ind1 * s7i_vector_offset(ivec, 0) + ind2;
  return(s7_make_integer(sc, s7_int_vector_ref(ivec, ind1)));
}

s7_pointer g_int_vector_set(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_set_int(sc, args));
}

s7_pointer g_iv_set_3(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer vec = s7_car(args);
  s7_pointer index;
  s7_int ind;
  if (!s7_is_int_vector(vec))
    return(s7i_vector_method_or_bust(sc, vec, "int-vector-set!", args, "an int-vector", 1));
  if (s7_vector_rank(vec) != 1)
    return(s7i_univect_set_int(sc, args));
  if (s7i_is_immutable_vector(vec))
    immutable_object_error_nr(sc, s7i_set_elist_3(sc, immutable_error_string, s7_make_symbol(sc, "int-vector-set!"), vec));
  index = s7_cadr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "int-vector-set!", args, "an integer", 2));
  ind = s7_number_to_integer(sc, index);
  if ((ind < 0) || (ind >= s7_vector_length(vec)))
    return(s7_out_of_range_error(sc, "int-vector-set!", 2, index, (ind < 0) ? "it is negative" : "it is too large"));
  {
    s7_pointer value = s7_caddr(args);
    if (!s7_is_integer(value))
      return(s7i_vector_method_or_bust(sc, value, "int-vector-set!", args, "an integer", 3));
    s7_int_vector_set(vec, ind, s7_number_to_integer(sc, value));
    return(value);
  }
}

s7_pointer g_byte_vector_ref(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_ref_byte(sc, args));
}

s7_pointer g_bv_ref_2(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer vec = s7_car(args);
  s7_pointer index;
  s7_int ind;
  if (!s7_is_byte_vector(vec))
    return(s7i_vector_method_or_bust(sc, vec, "byte-vector-ref", args, "a byte-vector", 1));
  if (s7_vector_rank(vec) != 1)
    return(s7i_univect_ref_byte(sc, args));
  index = s7_cadr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "byte-vector-ref", args, "an integer", 2));
  ind = s7_number_to_integer(sc, index);
  if ((ind < 0) || (ind >= s7_vector_length(vec)))
    return(s7_out_of_range_error(sc, "byte-vector-ref", 2, index, (ind < 0) ? "it is negative" : "it is too large"));
  return(s7_make_integer(sc, s7_byte_vector_ref(vec, ind)));
}

s7_pointer g_bv_ref_3(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer iv = s7_car(args);
  s7_pointer index;
  s7_int ind1, ind2;
  if (!s7_is_byte_vector(iv))
    return(s7i_vector_method_or_bust(sc, iv, "byte-vector-ref", args, "a byte-vector", 1));
  if (s7_vector_rank(iv) != 2)
    return(s7i_univect_ref_byte(sc, args));
  index = s7_cadr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "byte-vector-ref", args, "an integer", 2));
  ind1 = s7_number_to_integer(sc, index);
  if ((ind1 < 0) || (ind1 >= s7_vector_dimension(iv, 0)))
    return(s7_out_of_range_error(sc, "byte-vector-ref", 2, index, (ind1 < 0) ? "it is negative" : "it is too large"));
  index = s7_caddr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "byte-vector-ref", args, "an integer", 3));
  ind2 = s7_number_to_integer(sc, index);
  if ((ind2 < 0) || (ind2 >= s7_vector_dimension(iv, 1)))
    return(s7_out_of_range_error(sc, "byte-vector-ref", 3, index, (ind2 < 0) ? "it is negative" : "it is too large"));
  ind1 = ind1 * s7i_vector_offset(iv, 0) + ind2;
  return(s7_make_integer(sc, s7_byte_vector_ref(iv, ind1)));
}

s7_pointer g_byte_vector_set(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_set_byte(sc, args));
}

s7_pointer g_bv_set_3(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer vec = s7_car(args);
  s7_pointer index, value;
  s7_int ind;
  if (!s7_is_byte_vector(vec))
    return(s7i_vector_method_or_bust(sc, vec, "byte-vector-set!", args, "a byte-vector", 1));
  if (s7_vector_rank(vec) != 1)
    return(s7i_univect_set_byte(sc, args));
  if (s7i_is_immutable_vector(vec))
    immutable_object_error_nr(sc, s7i_set_elist_3(sc, immutable_error_string, s7_make_symbol(sc, "byte-vector-set!"), vec));
  index = s7_cadr(args);
  if (!s7_is_integer(index))
    return(s7i_vector_method_or_bust(sc, index, "byte-vector-set!", args, "an integer", 2));
  ind = s7_number_to_integer(sc, index);
  if ((ind < 0) || (ind >= s7_vector_length(vec)))
    return(s7_out_of_range_error(sc, "byte-vector-set!", 2, index, (ind < 0) ? "it is negative" : "it is too large"));
  value = s7_caddr(args);
  if (!s7_is_integer(value))
    return(s7i_vector_method_or_bust(sc, value, "byte-vector-set!", args, "an integer", 3));
  {
    s7_int byte = s7_number_to_integer(sc, value);
    if ((byte < 0) || (byte > 255))
      return(s7_type_error(sc, "byte-vector-set!", 3, value, "a byte"));
    s7_byte_vector_set(vec, ind, (uint8_t)byte);
  }
  return(value);
}

s7_pointer g_cv_set_3(s7_scheme *sc, s7_pointer args)
{
  return(s7i_complex_vector_set_p_ppp(sc, s7_car(args), s7_cadr(args), s7_caddr(args)));
}

s7_pointer g_complex_vector_set(s7_scheme *sc, s7_pointer args)
{
  return(s7i_univect_set_complex(sc, args));
}

s7_pointer g_multivector(s7_scheme *sc, s7_int dims, s7_pointer data)
{
  return(s7i_multivector_1(sc, dims, data));
}

s7_pointer g_int_multivector(s7_scheme *sc, s7_int dims, s7_pointer data)
{
  return(s7i_int_multivector_1(sc, dims, data));
}

s7_pointer g_byte_multivector(s7_scheme *sc, s7_int dims, s7_pointer data)
{
  return(s7i_byte_multivector_1(sc, dims, data));
}

s7_pointer g_float_multivector(s7_scheme *sc, s7_int dims, s7_pointer data)
{
  return(s7i_float_multivector_1(sc, dims, data));
}

s7_pointer g_complex_multivector(s7_scheme *sc, s7_int dims, s7_pointer data)
{
  return(s7i_complex_multivector_1(sc, dims, data));
}

s7_pointer g_list_to_vector(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (s7_is_null(sc, lst))
    return(s7_make_vector(sc, 0));
  if (!s7_is_proper_list(sc, lst))
    return(s7i_vector_method_or_bust_p(sc, lst, "list->vector", "a proper list"));
  return(g_vector(sc, lst));
}

s7_pointer g_vector_to_list(s7_scheme *sc, s7_pointer args)
{
  s7_int start = 0, end;
  const s7_pointer vec = s7_car(args);

  if (!s7i_is_any_vector(vec))
    return(s7i_vector_sole_arg_method_or_bust(sc, vec, "vector->list", args, "a vector"));
  end = s7_vector_length(vec);
  if (!s7_is_null(sc, s7_cdr(args)))
    {
      s7_pointer p = s7i_vector_start_and_end(sc, s7_make_symbol(sc, "vector->list"), args, 2, s7_cdr(args), &start, &end);
      if (!s7i_is_unused(sc, p)) return(p);
      if (start == end) return(s7_nil(sc));
    }
  else
    if (end == 0) return(s7_nil(sc));

  if ((end - start) > s7i_max_list_length(sc))
    return(s7_out_of_range_error(sc, "vector->list", 2, s7_make_integer(sc, end - start), "it is too large"));

  {
    s7_pointer anchor = s7_cons(sc, vec, s7_nil(sc));
    s7_gc_protect_via_stack(sc, anchor);
    if (s7i_is_t_vector(vec))
      {
        for (s7_int i = end - 1; i >= start; i--)
          s7_set_cdr(anchor, s7_cons(sc, s7i_vector_element(vec, i), s7_cdr(anchor)));
      }
    else
      {
        for (s7_int i = end - 1; i >= start; i--)
          s7_set_cdr(anchor, s7_cons(sc, s7i_vector_getter_ref(sc, vec, i), s7_cdr(anchor)));
      }
    s7_pointer result = s7_cdr(anchor);
    s7_gc_unprotect_via_stack(sc, anchor);
    return(result);
  }
}

s7_pointer g_vector_filter(s7_scheme *sc, s7_pointer args)
{
  s7_pointer pred = s7_car(args);
  s7_pointer vec = s7_cadr(args);
  if (!s7_is_procedure(pred))
    return(s7_type_error(sc, "vector-filter", 1, pred, "a procedure"));
  if (!s7_is_vector(vec))
    return(s7_type_error(sc, "vector-filter", 2, vec, "a vector"));
  /* args may live in evaluator-recycled cells, so keep pred and vec in our own
   * pair, with the result vector as the anchor's cdr; everything stays
   * GC-reachable while pred runs */
  s7_pointer anchor = s7_cons(sc, s7_cons(sc, pred, vec), s7_make_vector(sc, s7_vector_length(vec)));
  s7_gc_protect_via_stack(sc, anchor);
  s7_pointer result = s7_cdr(anchor);
  s7_int len = s7_vector_length(vec);
  s7_int count = 0;
  for (s7_int i = 0; i < len; i++)
    {
      s7_pointer elem = s7_vector_ref(sc, vec, i);
      if (s7i_is_true(sc, s7_apply_function(sc, pred, s7i_set_plist_1(sc, elem))))
        {
          s7_vector_set(sc, result, count, elem);
          count++;
        }
    }
  if (count == len)
    {
      s7_gc_unprotect_via_stack(sc, anchor);
      return(result);
    }
  {
    s7_pointer exact = s7_make_vector(sc, count);
    for (s7_int i = 0; i < count; i++)
      s7_vector_set(sc, exact, i, s7_vector_ref(sc, result, i));
    s7_gc_unprotect_via_stack(sc, anchor);
    return(exact);
  }
}

#if !WITH_PURE_S7

s7_pointer g_vector_length(s7_scheme *sc, s7_pointer args)
{
  s7_pointer vec = s7_car(args);
  if (!s7_is_vector(vec))
    return(s7i_vector_sole_arg_method_or_bust(sc, vec, "vector-length", args, "a vector"));
  return(s7_make_integer(sc, s7_vector_length(vec)));
}

s7_int vector_length_i_7p(s7_scheme *sc, s7_pointer vec)
{
  if (!s7_is_vector(vec))
    return(s7_integer(s7i_vector_method_or_bust_p(sc, vec, "vector-length", "a vector")));
  return(s7_vector_length(vec));
}

s7_pointer vector_length_p_p(s7_scheme *sc, s7_pointer vec)
{
  if (!s7_is_vector(vec))
    return(s7i_vector_method_or_bust_p(sc, vec, "vector-length", "a vector"));
  return(s7_make_integer(sc, s7_vector_length(vec)));
}

/* -------- optimizer typed-arg (p_p) functions, migrated from s7.c -------- */

/* the optimizer compares these function pointers directly
   (e.g. q_func(opc).p_pi_f == vector_ref_p_pi_unchecked), so each must
   have a single extern definition in this compilation unit */

s7_pointer vector_append_p_pp(s7_scheme *sc, s7_pointer v1, s7_pointer v2)
{
  return(s7i_vector_append_2(sc, v1, v2));
}

s7_pointer vector_append_p_ppp(s7_scheme *sc, s7_pointer v1, s7_pointer v2, s7_pointer v3)
{
  return(s7i_vector_append_3(sc, v1, v2, v3));
}

s7_pointer vector_to_list_p_p(s7_scheme *sc, s7_pointer vec)
{
  if (!s7i_is_any_vector(vec))
    return(s7i_vector_method_or_bust_p(sc, vec, "vector->list", "vector"));
  return(s7_vector_to_list(sc, vec));
}

s7_pointer vector_ref_p_pi(s7_scheme *sc, s7_pointer vec, s7_int index)
{
  if ((!s7i_is_t_vector(vec)) ||
      (s7_vector_rank(vec) > 1) ||
      (index < 0) || (index >= s7_vector_length(vec)))
    return(g_vector_ref(sc, s7i_set_plist_2(sc, vec, s7_make_integer(sc, index))));
  return(s7i_vector_element(vec, index));
}

s7_pointer vector_ref_p_pi_unchecked(s7_scheme *sc, s7_pointer vec, s7_int index) /* callable but just barely (tgsl.scm) */
{
  if ((index < 0) || (index >= s7_vector_length(vec)))
    out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-ref"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index),
                          (index < 0) ? it_is_negative_string : it_is_too_large_string);
  return(s7i_vector_getter_ref(sc, vec, index));
}

s7_pointer t_vector_ref_p_pi_unchecked(s7_scheme *sc, s7_pointer vec, s7_int index)
{
  if ((index < 0) || (index >= s7_vector_length(vec)))
    out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-ref"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index),
                          (index < 0) ? it_is_negative_string : it_is_too_large_string);
  return(s7i_vector_element(vec, index));
}

s7_pointer vector_ref_p_pii(s7_scheme *sc, s7_pointer vec, s7_int i1, s7_int i2)
{
  if ((!s7i_is_any_vector(vec)) ||
      (s7_vector_rank(vec) != 2) ||
      (i1 < 0) || (i2 < 0) ||
      (i1 >= s7_vector_dimension(vec, 0)) || (i2 >= s7_vector_dimension(vec, 1)))
    return(g_vector_ref(sc, s7i_set_plist_3(sc, vec, s7_make_integer(sc, i1), s7_make_integer(sc, i2))));
  return(s7i_vector_getter_ref(sc, vec, i2 + (i1 * s7i_vector_offset(vec, 0))));
}

s7_pointer vector_ref_p_pii_direct(s7_scheme *sc, s7_pointer vec, s7_int i1, s7_int i2)
{
  if ((i1 < 0) || (i2 < 0) ||
      (i1 >= s7_vector_dimension(vec, 0)) || (i2 >= s7_vector_dimension(vec, 1)))
    return(g_vector_ref(sc, s7i_set_plist_3(sc, vec, s7_make_integer(sc, i1), s7_make_integer(sc, i2))));
  return(s7i_vector_element(vec, i2 + (i1 * s7i_vector_offset(vec, 0))));
}

s7_pointer t_vector_ref_p_pi_direct(s7_scheme *unused_sc, s7_pointer vec, s7_int index) {return(s7i_vector_element(vec, index));}

s7_pointer vector_set_p_pip(s7_scheme *sc, s7_pointer vec, s7_int index, s7_pointer value) /* almost never called -- see one case in s7test.scm[13736] */
{
  if ((!s7i_is_any_vector(vec)) || (s7_vector_rank(vec) > 1) || (index < 0) || (index >= s7_vector_length(vec)))
    return(s7i_g_vector_set(sc, s7i_set_plist_3(sc, vec, s7_make_integer(sc, index), value)));
  if (s7i_is_t_vector(vec))
    {
      if (s7i_is_typed_vector(vec)) return(s7i_typed_vector_setter(sc, vec, index, value));
      s7i_vector_element_set(vec, index, value);
    }
  else s7i_vector_setter_set(sc, vec, index, value);
  return(value);
}

s7_pointer vector_set_p_pip_unchecked(s7_scheme *sc, s7_pointer vec, s7_int index, s7_pointer value)
{
  if ((index >= 0) && (index < s7_vector_length(vec)))
    s7i_vector_element_set(vec, index, value);
  else out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-set!"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index),
                             (index < 0) ? it_is_negative_string : it_is_too_large_string);
  return(value);
}

s7_pointer vector_set_p_piip(s7_scheme *sc, s7_pointer vec, s7_int i1, s7_int i2, s7_pointer value)
{
  if ((!s7i_is_any_vector(vec)) ||
      (s7_vector_rank(vec) != 2) ||
      (i1 < 0) || (i2 < 0) ||
      (i1 >= s7_vector_dimension(vec, 0)) || (i2 >= s7_vector_dimension(vec, 1)))
    return(s7i_g_vector_set(sc, s7i_set_plist_4(sc, vec, s7_make_integer(sc, i1), s7_make_integer(sc, i2), value)));
  if (s7i_is_t_vector(vec))
    {
      if (s7i_is_typed_vector(vec))
	return(s7i_typed_vector_setter(sc, vec, i2 + (i1 * s7i_vector_offset(vec, 0)), value));
      s7i_vector_element_set(vec, i2 + (i1 * s7i_vector_offset(vec, 0)), value);
    }
  else s7i_vector_setter_set(sc, vec, i2 + (i1 * s7i_vector_offset(vec, 0)), value);
  return(value);
}

s7_pointer vector_set_p_piip_direct(s7_scheme *sc, s7_pointer vec, s7_int i1, s7_int i2, s7_pointer value)
{
  /* normal untyped vector, rank == 2 */
  if ((i1 < 0) || (i2 < 0) ||
      (i1 >= s7_vector_dimension(vec, 0)) || (i2 >= s7_vector_dimension(vec, 1)))
    return(s7i_g_vector_set(sc, s7i_set_plist_4(sc, vec, s7_make_integer(sc, i1), s7_make_integer(sc, i2), value)));
  s7i_vector_element_set(vec, i2 + (i1 * s7i_vector_offset(vec, 0)), value);
  return(value);
}

s7_pointer typed_vector_set_p_pip_unchecked(s7_scheme *sc, s7_pointer vec, s7_int index, s7_pointer value)
{
  if ((index >= 0) && (index < s7_vector_length(vec)))
    s7i_typed_vector_setter(sc, vec, index, value);
  else out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-set!"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index),
                             (index < 0) ? it_is_negative_string : it_is_too_large_string);
  return(value);
}

s7_pointer typed_vector_set_p_piip_direct(s7_scheme *sc, s7_pointer vec, s7_int i1, s7_int i2, s7_pointer value)
{
  if ((i1 < 0) || (i2 < 0) ||
      (i1 >= s7_vector_dimension(vec, 0)) || (i2 >= s7_vector_dimension(vec, 1)))
    return(s7i_g_vector_set(sc, s7i_set_plist_4(sc, vec, s7_make_integer(sc, i1), s7_make_integer(sc, i2), value)));
  return(s7i_typed_vector_setter(sc, vec, i2 + (i1 * s7i_vector_offset(vec, 0)), value));
}

s7_pointer t_vector_set_p_pip_direct(s7_scheme *unused_sc, s7_pointer vec, s7_int index, s7_pointer value)
{
  s7i_vector_element_set(vec, index, value);
  return(value);
}

s7_pointer typed_t_vector_set_p_pip_direct(s7_scheme *sc, s7_pointer vec, s7_int index, s7_pointer value)
{
  s7i_typed_vector_setter(sc, vec, index, value);
  return(value);
}

s7_pointer vector_set_p_ppp(s7_scheme *sc, s7_pointer vec, s7_pointer ind, s7_pointer val)
{
  s7_int index;
  if ((!s7i_is_t_vector(vec)) || (s7_vector_rank(vec) > 1))
    return(s7i_g_vector_set(sc, s7i_set_plist_3(sc, vec, ind, val)));
  if (s7i_is_immutable_vector(vec))
    immutable_object_error_nr(sc, s7i_set_elist_3(sc, immutable_error_string, s7_make_symbol(sc, "vector-set!"), vec));
  if (!s7_is_integer(ind))
    return(s7i_g_vector_set(sc, s7i_set_plist_3(sc, vec, ind, val)));
  index = s7i_integer_clamped_if_gmp(sc, ind);
  if ((index < 0) || (index >= s7_vector_length(vec)))
    out_of_range_error_nr(sc, s7_make_symbol(sc, "vector-set!"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index),
                          (index < 0) ? it_is_negative_string : it_is_too_large_string);

  if (s7i_is_typed_vector(vec))
    return(s7i_typed_vector_setter(sc, vec, index, val));
  s7i_vector_element_set(vec, index, val);
  return(val);
}

s7_pointer byte_vector_ref_p_pi_direct(s7_scheme *unused_sc, s7_pointer vec, s7_int index)
{
  return(s7i_small_int(s7i_byte_vector_element(vec, index)));
}

s7_pointer byte_vector_set_p_pip_direct(s7_scheme *unused_sc, s7_pointer vec, s7_int index, s7_pointer byte)
{
  s7i_byte_vector_element_set(vec, index, (uint8_t)s7_integer(byte));
  return(byte);
}

#endif
