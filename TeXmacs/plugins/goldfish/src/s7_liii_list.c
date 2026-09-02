/* s7_liii_list.c - list utility implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#include "s7_liii_list.h"
#include "s7_internal_helpers.h"

#include <stddef.h>

s7_pointer g_is_null(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_null(sc, p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "null?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

/* -------------------------------- pair? -------------------------------- */

s7_pointer g_is_pair(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_pair(p)) return(s7_t(sc));
  if (!s7i_has_active_methods(sc, p)) return(s7_f(sc));
  {
    s7_pointer sym = s7_make_symbol(sc, "pair?");
    s7_pointer func = s7i_find_method_with_let(sc, p, sym);
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

/* -------------------------------- list? -------------------------------- */

s7_pointer g_is_list(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_list(sc, p)) return(s7_t(sc));
  if (!s7i_has_active_methods(sc, p)) return(s7_f(sc));
  {
    s7_pointer sym = s7_make_symbol(sc, "list?");
    s7_pointer func = s7i_find_method_with_let(sc, p, sym);
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

/* -------------------------------- proper-list? -------------------------------- */

s7_pointer g_is_proper_list(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7_is_proper_list(sc, s7_car(args))));
}

s7_pointer g_car(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (s7_is_pair(lst)) return(s7_car(lst));
  return(s7i_sole_arg_method_or_bust(sc, lst, "car", args, "a pair"));
}

s7_pointer g_cdr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (s7_is_pair(lst)) return(s7_cdr(lst));
  return(s7i_sole_arg_method_or_bust(sc, lst, "cdr", args, "a pair"));
}

s7_pointer g_caar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "caar", 1, lst, "a pair whose car is also a pair"));
  return(s7_caar(lst));
}

s7_pointer g_cadr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cadr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cadr", 1, lst, "a pair whose cdr is also a pair"));
  return(s7_cadr(lst));
}

s7_pointer g_cdar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cdar", 1, lst, "a pair whose car is also a pair"));
  return(s7_cdar(lst));
}

s7_pointer g_cddr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cddr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cddr", 1, lst, "a pair whose cdr is also a pair"));
  return(s7_cddr(lst));
}

/* -------------------------------- 3-level cxxxr -------------------------------- */

s7_pointer g_caaar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caaar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "caaar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_caar(lst)))
    return(s7_wrong_type_arg_error(sc, "caaar", 1, lst, "a pair whose caar is also a pair"));
  return(s7_caaar(lst));
}

s7_pointer g_caadr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caadr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "caadr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cadr(lst)))
    return(s7_wrong_type_arg_error(sc, "caadr", 1, lst, "a pair whose cadr is also a pair"));
  return(s7_caadr(lst));
}

s7_pointer g_cadar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cadar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cadar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_cdar(lst)))
    return(s7_wrong_type_arg_error(sc, "cadar", 1, lst, "a pair whose cdar is also a pair"));
  return(s7_cadar(lst));
}

s7_pointer g_caddr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caddr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "caddr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cddr(lst)))
    return(s7_wrong_type_arg_error(sc, "caddr", 1, lst, "a pair whose cddr is also a pair"));
  return(s7_caddr(lst));
}

s7_pointer g_cdaar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdaar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_caar(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaar", 1, lst, "a pair whose caar is also a pair"));
  return(s7_cdaar(lst));
}

s7_pointer g_cdddr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdddr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdddr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cddr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdddr", 1, lst, "a pair whose cddr is also a pair"));
  return(s7_cdddr(lst));
}

s7_pointer g_cdadr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdadr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdadr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cadr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdadr", 1, lst, "a pair whose cadr is also a pair"));
  return(s7_cdadr(lst));
}

s7_pointer g_cddar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cddar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cddar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_cdar(lst)))
    return(s7_wrong_type_arg_error(sc, "cddar", 1, lst, "a pair whose cdar is also a pair"));
  return(s7_cddar(lst));
}

/* -------------------------------- 4-level cxxxr -------------------------------- */

s7_pointer g_caaaar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caaaar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "caaaar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_caar(lst)))
    return(s7_wrong_type_arg_error(sc, "caaaar", 1, lst, "a pair whose caar is also a pair"));
  if (!s7_is_pair(s7_caaar(lst)))
    return(s7_wrong_type_arg_error(sc, "caaaar", 1, lst, "a pair whose caaar is also a pair"));
  return(s7_caaaar(lst));
}

s7_pointer g_caaadr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caaadr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "caaadr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cadr(lst)))
    return(s7_wrong_type_arg_error(sc, "caaadr", 1, lst, "a pair whose cadr is also a pair"));
  if (!s7_is_pair(s7_caadr(lst)))
    return(s7_wrong_type_arg_error(sc, "caaadr", 1, lst, "a pair whose caadr is also a pair"));
  return(s7_caaadr(lst));
}

s7_pointer g_caadar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caadar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "caadar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_cdar(lst)))
    return(s7_wrong_type_arg_error(sc, "caadar", 1, lst, "a pair whose cdar is also a pair"));
  if (!s7_is_pair(s7_cadar(lst)))
    return(s7_wrong_type_arg_error(sc, "caadar", 1, lst, "a pair whose cadar is also a pair"));
  return(s7_caadar(lst));
}

s7_pointer g_cadaar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cadaar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cadaar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_caar(lst)))
    return(s7_wrong_type_arg_error(sc, "cadaar", 1, lst, "a pair whose caar is also a pair"));
  if (!s7_is_pair(s7_cdaar(lst)))
    return(s7_wrong_type_arg_error(sc, "cadaar", 1, lst, "a pair whose cdaar is also a pair"));
  return(s7_cadaar(lst));
}

s7_pointer g_caaddr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caaddr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "caaddr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cddr(lst)))
    return(s7_wrong_type_arg_error(sc, "caaddr", 1, lst, "a pair whose cddr is also a pair"));
  if (!s7_is_pair(s7_caddr(lst)))
    return(s7_wrong_type_arg_error(sc, "caaddr", 1, lst, "a pair whose caddr is also a pair"));
  return(s7_caaddr(lst));
}

s7_pointer g_cadddr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cadddr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cadddr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cddr(lst)))
    return(s7_wrong_type_arg_error(sc, "cadddr", 1, lst, "a pair whose cddr is also a pair"));
  if (!s7_is_pair(s7_cdddr(lst)))
    return(s7_wrong_type_arg_error(sc, "cadddr", 1, lst, "a pair whose cdddr is also a pair"));
  return(s7_cadddr(lst));
}

s7_pointer g_cadadr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cadadr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cadadr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cadr(lst)))
    return(s7_wrong_type_arg_error(sc, "cadadr", 1, lst, "a pair whose cadr is also a pair"));
  if (!s7_is_pair(s7_cdadr(lst)))
    return(s7_wrong_type_arg_error(sc, "cadadr", 1, lst, "a pair whose cdadr is also a pair"));
  return(s7_cadadr(lst));
}

s7_pointer g_caddar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "caddar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "caddar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_cdar(lst)))
    return(s7_wrong_type_arg_error(sc, "caddar", 1, lst, "a pair whose cdar is also a pair"));
  if (!s7_is_pair(s7_cddar(lst)))
    return(s7_wrong_type_arg_error(sc, "caddar", 1, lst, "a pair whose cddar is also a pair"));
  return(s7_caddar(lst));
}

s7_pointer g_cdaaar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdaaar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaaar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_caar(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaaar", 1, lst, "a pair whose caar is also a pair"));
  if (!s7_is_pair(s7_caaar(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaaar", 1, lst, "a pair whose caaar is also a pair"));
  return(s7_cdaaar(lst));
}

s7_pointer g_cdaadr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdaadr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaadr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cadr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaadr", 1, lst, "a pair whose cadr is also a pair"));
  if (!s7_is_pair(s7_caadr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaadr", 1, lst, "a pair whose caadr is also a pair"));
  return(s7_cdaadr(lst));
}

s7_pointer g_cdadar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdadar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cdadar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_cdar(lst)))
    return(s7_wrong_type_arg_error(sc, "cdadar", 1, lst, "a pair whose cdar is also a pair"));
  if (!s7_is_pair(s7_cadar(lst)))
    return(s7_wrong_type_arg_error(sc, "cdadar", 1, lst, "a pair whose cadar is also a pair"));
  return(s7_cdadar(lst));
}

s7_pointer g_cddaar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cddaar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cddaar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_caar(lst)))
    return(s7_wrong_type_arg_error(sc, "cddaar", 1, lst, "a pair whose caar is also a pair"));
  if (!s7_is_pair(s7_cdaar(lst)))
    return(s7_wrong_type_arg_error(sc, "cddaar", 1, lst, "a pair whose cdaar is also a pair"));
  return(s7_cddaar(lst));
}

s7_pointer g_cdaddr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdaddr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaddr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cddr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaddr", 1, lst, "a pair whose cddr is also a pair"));
  if (!s7_is_pair(s7_caddr(lst)))
    return(s7_wrong_type_arg_error(sc, "cdaddr", 1, lst, "a pair whose caddr is also a pair"));
  return(s7_cdaddr(lst));
}

s7_pointer g_cddddr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cddddr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cddddr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cddr(lst)))
    return(s7_wrong_type_arg_error(sc, "cddddr", 1, lst, "a pair whose cddr is also a pair"));
  if (!s7_is_pair(s7_cdddr(lst)))
    return(s7_wrong_type_arg_error(sc, "cddddr", 1, lst, "a pair whose cdddr is also a pair"));
  return(s7_cddddr(lst));
}

s7_pointer g_cddadr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cddadr", args, "a pair"));
  if (!s7_is_pair(s7_cdr(lst)))
    return(s7_wrong_type_arg_error(sc, "cddadr", 1, lst, "a pair whose cdr is also a pair"));
  if (!s7_is_pair(s7_cadr(lst)))
    return(s7_wrong_type_arg_error(sc, "cddadr", 1, lst, "a pair whose cadr is also a pair"));
  if (!s7_is_pair(s7_cdadr(lst)))
    return(s7_wrong_type_arg_error(sc, "cddadr", 1, lst, "a pair whose cdadr is also a pair"));
  return(s7_cddadr(lst));
}

s7_pointer g_cdddar(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  if (!s7_is_pair(lst))
    return(s7i_sole_arg_method_or_bust(sc, lst, "cdddar", args, "a pair"));
  if (!s7_is_pair(s7_car(lst)))
    return(s7_wrong_type_arg_error(sc, "cdddar", 1, lst, "a pair whose car is also a pair"));
  if (!s7_is_pair(s7_cdar(lst)))
    return(s7_wrong_type_arg_error(sc, "cdddar", 1, lst, "a pair whose cdar is also a pair"));
  if (!s7_is_pair(s7_cddar(lst)))
    return(s7_wrong_type_arg_error(sc, "cdddar", 1, lst, "a pair whose cddar is also a pair"));
  return(s7_cdddar(lst));
}

s7_pointer g_set_car(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer val = s7_cadr(args);
  if (s7_is_pair(lst) && !s7_is_immutable(lst))
    return(s7_set_car(lst, val));
  if (!s7_is_pair(lst))
    return(s7_wrong_type_arg_error(sc, "set-car!", 1, lst, "a pair"));
  return(s7_wrong_type_arg_error(sc, "set-car!", 1, lst, "a mutable pair"));
}

s7_pointer g_set_cdr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer val = s7_cadr(args);
  if (s7_is_pair(lst) && !s7_is_immutable(lst))
    return(s7_set_cdr(lst, val));
  if (!s7_is_pair(lst))
    return(s7_wrong_type_arg_error(sc, "set-cdr!", 1, lst, "a pair"));
  return(s7_wrong_type_arg_error(sc, "set-cdr!", 1, lst, "a mutable pair"));
}

s7_pointer g_list_ref(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer ind = s7_cadr(args);
  if (!s7_is_pair(lst))
    return(s7i_method_or_bust(sc, lst, "list-ref", args, "a pair", 1));
  if (!s7_is_integer(ind))
    return(s7i_method_or_bust(sc, ind, "list-ref", args, "an integer", 2));
  s7_int index = s7_integer(ind);
  if (index < 0)
    return(s7_out_of_range_error(sc, "list-ref", 2, ind, "it is negative"));
  s7_pointer p = lst;
  for (s7_int i = 0; (i < index) && s7_is_pair(p); i++)
    p = s7_cdr(p);
  if (!s7_is_pair(p))
    return(s7_out_of_range_error(sc, "list-ref", 2, ind, "it is too large"));
  return(s7_car(p));
}

s7_pointer g_list_tail(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer ind = s7_cadr(args);
  if (!s7_is_integer(ind))
    return(s7i_method_or_bust(sc, ind, "list-tail", args, "an integer", 2));
  if (!s7_is_pair(lst) && !s7_is_null(sc, lst))
    return(s7i_method_or_bust(sc, lst, "list-tail", args, "a list", 1));
  s7_int index = s7_integer(ind);
  if (index < 0)
    return(s7_out_of_range_error(sc, "list-tail", 2, ind, "it is negative"));
  s7_pointer p = lst;
  s7_int i;
  for (i = 0; (i < index) && s7_is_pair(p); i++)
    p = s7_cdr(p);
  if (i < index)
    return(s7_out_of_range_error(sc, "list-tail", 2, ind, "it is too large"));
  return(p);
}

s7_pointer g_cons(s7_scheme *sc, s7_pointer args)
{
  return(s7_cons(sc, s7_car(args), s7_cadr(args)));
}

/* -------------------------------- filter -------------------------------- */

s7_pointer g_filter(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_cadr(args);
  if (!s7_is_pair(lst))
    {
      if (s7_is_null(sc, lst)) return(s7_nil(sc));
      return(s7_wrong_type_arg_error(sc, "filter", 2, lst, "a proper list"));
    }
  /* args may live in evaluator-recycled cells, so keep pred and lst in our own pairs.
   *   anchor = ((pred lst) . work), work's car holds the reversed kept elements before
   *   the current all-passing run, work's cdr later holds the result; one protected
   *   pair keeps everything GC-reachable while pred runs */
  s7_pointer keep = s7_cons(sc, s7_car(args), s7_cons(sc, lst, s7_nil(sc)));
  s7_pointer anchor = s7_cons(sc, keep, s7_cons(sc, s7_nil(sc), s7_nil(sc)));
  s7_gc_protect_via_stack(sc, anchor);
  s7_pointer work = s7_cdr(anchor);
  s7_pointer pred = s7_car(keep);
  s7_pointer run_start = NULL;
  s7_pointer p = lst;
  while (s7_is_pair(p))
    {
      if (s7i_is_true(sc, s7_apply_function(sc, pred, s7i_set_plist_1(sc, s7_car(p)))))
        {
          if (!run_start) run_start = p;
        }
      else
        {
          if (run_start)
            {
              for (s7_pointer q = run_start; q != p; q = s7_cdr(q))
                s7_set_car(work, s7_cons(sc, s7_car(q), s7_car(work)));
              run_start = NULL;
            }
        }
      p = s7_cdr(p);
    }
  if (!s7_is_null(sc, p))
    {
      s7_gc_unprotect_via_stack(sc, anchor);
      return(s7_wrong_type_arg_error(sc, "filter", 2, lst, "a proper list"));
    }
  /* share the longest all-passing suffix, like the reference implementation */
  s7_pointer result = (run_start) ? run_start : s7_nil(sc);
  s7_set_cdr(work, result);
  for (s7_pointer q = s7_car(work); s7_is_pair(q); q = s7_cdr(q))
    {
      result = s7_cons(sc, s7_car(q), result);
      s7_set_cdr(work, result);
    }
  s7_gc_unprotect_via_stack(sc, anchor);
  return(result);
}

/* -------------------------------- find -------------------------------- */

s7_pointer g_find(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_cadr(args);
  /* args may live in evaluator-recycled cells, so keep pred and lst in our own
   * protected pair; the walking pointer and the current element stay
   * GC-reachable through lst while pred runs */
  s7_pointer keep = s7_cons(sc, s7_car(args), s7_cons(sc, lst, s7_nil(sc)));
  s7_gc_protect_via_stack(sc, keep);
  s7_pointer pred = s7_car(keep);
  s7_pointer p = lst;
  while (s7_is_pair(p))
    {
      s7_pointer elem = s7_car(p);
      if (s7i_is_true(sc, s7_apply_function(sc, pred, s7i_set_plist_1(sc, elem))))
        {
          s7_gc_unprotect_via_stack(sc, keep);
          return(elem);
        }
      p = s7_cdr(p);
    }
  s7_gc_unprotect_via_stack(sc, keep);
  if (!s7_is_null(sc, p))
    return(s7_wrong_type_arg_error(sc, "find", 2, lst, "a proper list"));
  return(s7_f(sc));
}

/* -------------------------------- any -------------------------------- */

s7_pointer g_any(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_cadr(args);
  /* same GC pattern as g_find: keep pred and lst anchored in our own
   * protected pair while pred runs */
  s7_pointer keep = s7_cons(sc, s7_car(args), s7_cons(sc, lst, s7_nil(sc)));
  s7_gc_protect_via_stack(sc, keep);
  s7_pointer pred = s7_car(keep);
  s7_pointer p = lst;
  while (s7_is_pair(p))
    {
      if (s7i_is_true(sc, s7_apply_function(sc, pred, s7i_set_plist_1(sc, s7_car(p)))))
        {
          s7_gc_unprotect_via_stack(sc, keep);
          return(s7_t(sc));
        }
      p = s7_cdr(p);
    }
  s7_gc_unprotect_via_stack(sc, keep);
  if (!s7_is_null(sc, p))
    return(s7_wrong_type_arg_error(sc, "any", 2, lst, "a proper list"));
  return(s7_f(sc));
}

/* -------------------------------- every -------------------------------- */

s7_pointer g_every(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_cadr(args);
  /* same GC pattern as g_find: keep pred and lst anchored in our own
   * protected pair while pred runs */
  s7_pointer keep = s7_cons(sc, s7_car(args), s7_cons(sc, lst, s7_nil(sc)));
  s7_gc_protect_via_stack(sc, keep);
  s7_pointer pred = s7_car(keep);
  s7_pointer p = lst;
  while (s7_is_pair(p))
    {
      if (!s7i_is_true(sc, s7_apply_function(sc, pred, s7i_set_plist_1(sc, s7_car(p)))))
        {
          s7_gc_unprotect_via_stack(sc, keep);
          return(s7_f(sc));
        }
      p = s7_cdr(p);
    }
  s7_gc_unprotect_via_stack(sc, keep);
  if (!s7_is_null(sc, p))
    return(s7_wrong_type_arg_error(sc, "every", 2, lst, "a proper list"));
  return(s7_t(sc));
}

/* -------------------------------- count -------------------------------- */

s7_pointer g_count(s7_scheme *sc, s7_pointer args)
{
  /* (count pred clist1 clist2 ...) applies pred to one element of each list
   * per iteration and returns the number of true results; the walk stops at
   * the end of the shortest list */
  s7_pointer pred = s7_car(args);
  if (!s7_is_procedure(pred))
    return(s7_wrong_type_arg_error(sc, "count", 1, pred, "a procedure"));

  s7_pointer rest = s7_cdr(args);   /* (clist1 clist2 ...) */

  if (s7_is_null(sc, s7_cdr(rest)))
    {
      /* single-list case: same GC pattern as g_any -- keep pred and lst
       * anchored in our own protected pair while pred runs */
      s7_pointer lst = s7_car(rest);
      s7_pointer keep = s7_cons(sc, pred, s7_cons(sc, lst, s7_nil(sc)));
      s7_gc_protect_via_stack(sc, keep);
      s7_pointer p = lst;
      s7_int i = 0;
      while (s7_is_pair(p))
        {
          if (s7i_is_true(sc, s7_apply_function(sc, s7_car(keep), s7i_set_plist_1(sc, s7_car(p)))))
            i++;
          p = s7_cdr(p);
        }
      s7_gc_unprotect_via_stack(sc, keep);
      if (!s7_is_null(sc, p))
        return(s7_wrong_type_arg_error(sc, "count", 2, lst, "a proper list"));
      return(s7_make_integer(sc, i));
    }

  /* multi-list case: check properness up front (no Scheme callbacks here, so
   * no GC concerns), then walk all lists in lockstep */
  for (s7_pointer lp = rest; s7_is_pair(lp); lp = s7_cdr(lp))
    if (!s7_is_proper_list(sc, s7_car(lp)))
      return(s7_wrong_type_arg_error(sc, "count", 2, s7_car(lp), "a proper list"));

  /* keep pred, a slot for the current call args, and one "current position"
   * cell per list (in argument order) in our own protected cells: the args
   * cells may be recycled by the evaluator while pred runs; the slot sits at
   * cadr so the position-cell walk below starts after it */
  s7_pointer anchor = s7_cons(sc, pred, s7_cons(sc, s7_cons(sc, s7_nil(sc), s7_nil(sc)), s7_nil(sc)));
  s7_gc_protect_via_stack(sc, anchor);
  s7_pointer tail = s7_cdr(anchor);   /* (call-args-slot) */
  for (s7_pointer lp = rest; s7_is_pair(lp); lp = s7_cdr(lp))
    {
      s7_set_cdr(tail, s7_cons(sc, s7_car(lp), s7_nil(sc)));
      tail = s7_cdr(tail);
    }
  s7_pointer args_slot = s7_car(s7_cdr(anchor));

  s7_int i = 0;
  while (true)
    {
      /* build the call args from the current heads, linking each new pair
       * into args_slot right after s7_cons so the growing list stays
       * GC-reachable; advance the position cells in the same pass */
      s7_set_car(args_slot, s7_nil(sc));
      s7_pointer prev = NULL;
      bool shortest_ended = false;
      for (s7_pointer cell = s7_cdr(s7_cdr(anchor)); s7_is_pair(cell); cell = s7_cdr(cell))
        {
          s7_pointer cur = s7_car(cell);
          if (!s7_is_pair(cur))
            {
              /* pre-checked above, so the shortest list just ended */
              shortest_ended = true;
              break;
            }
          s7_pointer node = s7_cons(sc, s7_car(cur), s7_nil(sc));
          if (prev == NULL)
            s7_set_car(args_slot, node);
          else
            s7_set_cdr(prev, node);
          prev = node;
          s7_set_car(cell, s7_cdr(cur));
        }
      if (shortest_ended)
        break;
      if (s7i_is_true(sc, s7_apply_function(sc, s7_car(anchor), s7_car(args_slot))))
        i++;
    }
  s7_gc_unprotect_via_stack(sc, anchor);
  return(s7_make_integer(sc, i));
}

/* ------------------------------ list-index ------------------------------ */

s7_pointer g_list_index(s7_scheme *sc, s7_pointer args)
{
  /* (list-index pred clist1 clist2 ...) applies pred to one element of each
   * list per iteration and returns the index of the first true result, or #f;
   * the walk stops at the end of the shortest list */
  s7_pointer pred = s7_car(args);
  if (!s7_is_procedure(pred))
    return(s7_wrong_type_arg_error(sc, "list-index", 1, pred, "a procedure"));

  s7_pointer rest = s7_cdr(args);   /* (clist1 clist2 ...) */

  if (s7_is_null(sc, s7_cdr(rest)))
    {
      /* single-list case: same GC pattern as g_count -- keep pred and lst
       * anchored in our own protected pair while pred runs; properness is
       * checked lazily, so an early match returns without touching the tail */
      s7_pointer lst = s7_car(rest);
      s7_pointer keep = s7_cons(sc, pred, s7_cons(sc, lst, s7_nil(sc)));
      s7_gc_protect_via_stack(sc, keep);
      s7_pointer p = lst;
      s7_int i = 0;
      while (s7_is_pair(p))
        {
          if (s7i_is_true(sc, s7_apply_function(sc, s7_car(keep), s7i_set_plist_1(sc, s7_car(p)))))
            {
              s7_gc_unprotect_via_stack(sc, keep);
              return(s7_make_integer(sc, i));
            }
          i++;
          p = s7_cdr(p);
        }
      s7_gc_unprotect_via_stack(sc, keep);
      if (!s7_is_null(sc, p))
        return(s7_wrong_type_arg_error(sc, "list-index", 2, lst, "a proper list"));
      return(s7_f(sc));
    }

  /* multi-list case: check properness up front (no Scheme callbacks here, so
   * no GC concerns), then walk all lists in lockstep */
  for (s7_pointer lp = rest; s7_is_pair(lp); lp = s7_cdr(lp))
    if (!s7_is_proper_list(sc, s7_car(lp)))
      return(s7_wrong_type_arg_error(sc, "list-index", 2, s7_car(lp), "a proper list"));

  /* same anchor layout as g_count: pred, a slot for the current call args,
   * then one "current position" cell per list in argument order */
  s7_pointer anchor = s7_cons(sc, pred, s7_cons(sc, s7_cons(sc, s7_nil(sc), s7_nil(sc)), s7_nil(sc)));
  s7_gc_protect_via_stack(sc, anchor);
  s7_pointer tail = s7_cdr(anchor);   /* (call-args-slot) */
  for (s7_pointer lp = rest; s7_is_pair(lp); lp = s7_cdr(lp))
    {
      s7_set_cdr(tail, s7_cons(sc, s7_car(lp), s7_nil(sc)));
      tail = s7_cdr(tail);
    }
  s7_pointer args_slot = s7_car(s7_cdr(anchor));

  s7_int i = 0;
  while (true)
    {
      /* build the call args from the current heads, linking each new pair
       * into args_slot right after s7_cons so the growing list stays
       * GC-reachable; advance the position cells in the same pass */
      s7_set_car(args_slot, s7_nil(sc));
      s7_pointer prev = NULL;
      bool shortest_ended = false;
      for (s7_pointer cell = s7_cdr(s7_cdr(anchor)); s7_is_pair(cell); cell = s7_cdr(cell))
        {
          s7_pointer cur = s7_car(cell);
          if (!s7_is_pair(cur))
            {
              /* pre-checked above, so the shortest list just ended */
              shortest_ended = true;
              break;
            }
          s7_pointer node = s7_cons(sc, s7_car(cur), s7_nil(sc));
          if (prev == NULL)
            s7_set_car(args_slot, node);
          else
            s7_set_cdr(prev, node);
          prev = node;
          s7_set_car(cell, s7_cdr(cur));
        }
      if (shortest_ended)
        break;
      if (s7i_is_true(sc, s7_apply_function(sc, s7_car(anchor), s7_car(args_slot))))
        {
          s7_gc_unprotect_via_stack(sc, anchor);
          return(s7_make_integer(sc, i));
        }
      i++;
    }
  s7_gc_unprotect_via_stack(sc, anchor);
  return(s7_f(sc));
}

/* -------------------------------- fold / fold-right -------------------------------- */

s7_pointer g_fold(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_caddr(args);
  if (!s7_is_pair(lst))
    {
      if (s7_is_null(sc, lst)) return(s7_cadr(args));
      return(s7_wrong_type_arg_error(sc, "fold", 3, lst, "a proper list"));
    }
  /* args may live in evaluator-recycled cells: keep f and lst in our own pairs,
   * and the accumulator in a dedicated cell we rewrite each iteration, so every
   * intermediate value stays GC-reachable while f runs */
  s7_pointer keep = s7_cons(sc, s7_car(args), lst);
  s7_pointer anchor = s7_cons(sc, keep, s7_cons(sc, s7_cadr(args), s7_nil(sc)));
  s7_gc_protect_via_stack(sc, anchor);
  s7_pointer f = s7_car(keep);
  s7_pointer acc_cell = s7_cdr(anchor);
  s7_pointer p = lst;
  while (s7_is_pair(p))
    {
      s7_set_car(acc_cell, s7_apply_function(sc, f, s7i_set_plist_2(sc, s7_car(p), s7_car(acc_cell))));
      p = s7_cdr(p);
    }
  if (!s7_is_null(sc, p))
    {
      s7_gc_unprotect_via_stack(sc, anchor);
      return(s7_wrong_type_arg_error(sc, "fold", 3, lst, "a proper list"));
    }
  s7_pointer result = s7_car(acc_cell);
  s7_gc_unprotect_via_stack(sc, anchor);
  return(result);
}

s7_pointer g_fold_right(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_caddr(args);
  if (!s7_is_pair(lst))
    {
      if (s7_is_null(sc, lst)) return(s7_cadr(args));
      return(s7_wrong_type_arg_error(sc, "fold-right", 3, lst, "a proper list"));
    }
  /* fold-right(f, init, (e1 ... en)) applies f from the right:
   * f(e1, f(e2, ... f(en, init))), i.e. acc = f(elem, acc) walking elements
   * in reverse; first copy the elements into a reversed list so the walk
   * needs no Scheme-stack recursion */
  s7_pointer keep = s7_cons(sc, s7_car(args), lst);
  s7_pointer anchor = s7_cons(sc, keep, s7_cons(sc, s7_cadr(args), s7_nil(sc)));
  s7_gc_protect_via_stack(sc, anchor);
  s7_pointer rev = s7_nil(sc);
  s7_set_cdr(s7_cdr(anchor), rev);
  s7_pointer p = lst;
  while (s7_is_pair(p))
    {
      rev = s7_cons(sc, s7_car(p), rev);
      s7_set_cdr(s7_cdr(anchor), rev);
      p = s7_cdr(p);
    }
  if (!s7_is_null(sc, p))
    {
      s7_gc_unprotect_via_stack(sc, anchor);
      return(s7_wrong_type_arg_error(sc, "fold-right", 3, lst, "a proper list"));
    }
  s7_pointer f = s7_car(keep);
  s7_pointer acc_cell = s7_cdr(anchor);
  while (s7_is_pair(rev))
    {
      s7_set_car(acc_cell, s7_apply_function(sc, f, s7i_set_plist_2(sc, s7_car(rev), s7_car(acc_cell))));
      rev = s7_cdr(rev);
    }
  s7_pointer result = s7_car(acc_cell);
  s7_gc_unprotect_via_stack(sc, anchor);
  return(result);
}

/* -------------------------------- take -------------------------------- */

s7_pointer g_take(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer k = s7_cadr(args);
  if (!s7_is_integer(k))
    return(s7_wrong_type_arg_error(sc, "take", 2, k, "an integer"));
  s7_int n = s7_integer(k);
  if (n < 0)
    return(s7_wrong_type_arg_error(sc, "take", 2, k, "a non-negative integer"));
  if (n == 0) return(s7_nil(sc));
  /* no Scheme callbacks here, so args stay put; only the result being built
   * needs a GC anchor, with each new pair linked in right after s7_cons */
  s7_pointer head = s7_cons(sc, s7_nil(sc), s7_nil(sc));
  s7_gc_protect_via_stack(sc, head);
  s7_pointer tail = head;
  s7_pointer p = lst;
  for (s7_int i = 0; i < n; i++)
    {
      if (!s7_is_pair(p))
        {
          s7_gc_unprotect_via_stack(sc, head);
          return(s7_wrong_type_arg_error(sc, "take", 1, lst, "a list of sufficient length"));
        }
      s7_set_cdr(tail, s7_cons(sc, s7_car(p), s7_nil(sc)));
      tail = s7_cdr(tail);
      p = s7_cdr(p);
    }
  s7_gc_unprotect_via_stack(sc, head);
  return(s7_cdr(head));
}

/* -------------------------------- take-right / drop-right -------------------------------- */

/* shared lead walk: returns NULL on success (lead advanced n pairs),
 * otherwise the error object to return */
static s7_pointer take_right_lead(s7_scheme *sc, const char *name, s7_pointer lst, s7_pointer k, s7_int n, s7_pointer *lead)
{
  if (!s7_is_integer(k))
    return(s7_wrong_type_arg_error(sc, name, 2, k, "an integer"));
  if (n < 0)
    return(s7_out_of_range_error(sc, name, 2, k, "it is negative"));
  if (!s7_is_pair(lst) && !s7_is_null(sc, lst))
    return(s7_wrong_type_arg_error(sc, name, 1, lst, "a list"));
  s7_pointer p = lst;
  for (s7_int i = 0; i < n; i++)
    {
      if (!s7_is_pair(p))
        return(s7_out_of_range_error(sc, name, 2, k, "it is too large"));
      p = s7_cdr(p);
    }
  (*lead) = p;
  return(NULL);
}

s7_pointer g_take_right(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer k = s7_cadr(args);
  s7_pointer lead;
  s7_pointer err = take_right_lead(sc, "take-right", lst, k, s7_is_integer(k) ? s7_integer(k) : 0, &lead);
  if (err) return(err);
  /* no allocation and no Scheme callbacks: result is a sublist of lst */
  s7_pointer lag = lst;
  while (s7_is_pair(lead))
    {
      lag = s7_cdr(lag);
      lead = s7_cdr(lead);
    }
  return(lag);
}

s7_pointer g_drop_right(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer k = s7_cadr(args);
  s7_pointer lead;
  s7_pointer err = take_right_lead(sc, "drop-right", lst, k, s7_is_integer(k) ? s7_integer(k) : 0, &lead);
  if (err) return(err);
  /* dummy head anchor, same GC pattern as g_take */
  s7_pointer head = s7_cons(sc, s7_nil(sc), s7_nil(sc));
  s7_gc_protect_via_stack(sc, head);
  s7_pointer tail = head;
  s7_pointer lag = lst;
  while (s7_is_pair(lead))
    {
      s7_set_cdr(tail, s7_cons(sc, s7_car(lag), s7_nil(sc)));
      tail = s7_cdr(tail);
      lag = s7_cdr(lag);
      lead = s7_cdr(lead);
    }
  s7_gc_unprotect_via_stack(sc, head);
  return(s7_cdr(head));
}

s7_pointer g_list(s7_scheme *sc, s7_pointer args)
{
  return(s7i_copy_proper_list(sc, args));
}

s7_pointer g_list_set_1(s7_scheme *sc, s7_pointer lst, s7_pointer args, int32_t arg_num)
{
  #define H_list_set "(list-set! lst i ... val) sets the i-th element (0-based) of the list to val"
  #define Q_list_set s7_make_circular_signature(sc, 3, 4, sc->T, sc->is_pair_symbol, sc->is_integer_symbol, sc->is_integer_or_any_at_end_symbol)

  s7_int index;
  s7_pointer p = lst, ind;

  if (!s7_is_pair(lst) || s7_is_immutable(lst))
    return(s7_wrong_type_arg_error(sc, "list-set!", 1, lst, "a mutable pair"));
  ind = s7_car(args);
  if ((arg_num > 2) && s7_is_null(sc, s7_cdr(args)))
    {
      s7_set_car(lst, ind);
      return(ind);
    }
  if (!s7_is_integer(ind))
    {
      s7_pointer full_args = s7_cons(sc, lst, args);
      return(s7i_method_or_bust(sc, ind, "list-set!", full_args, "an integer", 2));
    }
  index = s7_integer(ind);

  if (index < 0)
    return(s7_out_of_range_error(sc, "list-set!", arg_num, ind, "it is negative"));
  if (index > s7i_max_list_length(sc))
    return(s7_out_of_range_error(sc, "list-set!", arg_num, ind, "it is too large"));

  for (s7_int i = 0; (i < index) && s7_is_pair(p); i++, p = s7_cdr(p)) {}
  if (!s7_is_pair(p))
    {
      if (s7_is_null(sc, p))
        return(s7_out_of_range_error(sc, "list-set!", arg_num, ind, "it is too large"));
      return(s7_wrong_type_arg_error(sc, "list-set!", 1, lst, "a proper list"));
    }
  if (s7_is_null(sc, s7_cddr(args)))
    s7_set_car(p, s7_cadr(args));
  else
    {
      if (!s7_is_pair(s7_car(p)))
        return(s7_wrong_number_of_args_error(sc, "list-set!", args));
      return(g_list_set_1(sc, s7_car(p), s7_cdr(args), arg_num + 1));
    }
  return(s7_cadr(args));
}

s7_pointer g_list_set(s7_scheme *sc, s7_pointer args)
{
  return(g_list_set_1(sc, s7_car(args), s7_cdr(args), 2));
}

s7_pointer g_list_set_i(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lst = s7_car(args);
  s7_pointer p = lst;
  s7_int index;
  if (!s7_is_pair(lst) || s7_is_immutable(lst))
    return(s7_wrong_type_arg_error(sc, "list-set!", 1, lst, "a mutable pair"));
  index = s7_integer(s7_cadr(args));
  if ((index < 0) || (index > s7i_max_list_length(sc)))
    return(s7_out_of_range_error(sc, "list-set!", 2, s7_make_integer(sc, index), "it is negative or too large"));

  for (s7_int i = 0; (i < index) && s7_is_pair(p); i++, p = s7_cdr(p)) {}
  if (!s7_is_pair(p))
    {
      if (s7_is_null(sc, p))
        return(s7_out_of_range_error(sc, "list-set!", 2, s7_make_integer(sc, index), "it is too large"));
      return(s7_wrong_type_arg_error(sc, "list-set!", 1, lst, "a proper list"));
    }
  {
    s7_pointer val = s7_caddr(args);
    s7_set_car(p, val);
    return(val);
  }
}

/* -------- optimizer typed-arg (p_p) functions, migrated from s7.c -------- */

/* the optimizer compares these function pointers directly
   (e.g. q_func1(opc).p_p_f == car_p_p), so each must have a single
   extern definition in this compilation unit */

/* Externally defined in s7.c - permanent type/error description strings */
extern s7_pointer a_list_string, an_association_list_string, a_proper_list_string;
extern s7_pointer it_is_negative_string, it_is_too_large_string;

s7_pointer tree_leaves_p_p(s7_scheme *sc, s7_pointer tree)
{
  if (s7_is_list(sc, tree))
    {
      if (s7i_tree_is_cyclic_checked(sc, tree))
	s7i_error_nr(sc, s7_make_symbol(sc, "wrong-type-arg"),
		     s7i_set_elist_2(sc, s7i_wrap_string(sc, "tree-leaves: tree is cyclic: ~S", 31), tree));
      return(s7_make_integer(sc, s7i_tree_len(sc, tree)));
    }
  return(s7i_method_or_bust_p(sc, tree, "tree-leaves", "a list"));
}

s7_pointer tree_set_memq_p_pp(s7_scheme *sc, s7_pointer syms, s7_pointer tree)
{
  return(s7_make_boolean(sc, s7i_tree_set_memq_b_7pp(sc, syms, tree)));
}

s7_pointer is_proper_list_p_p(s7_scheme *sc, s7_pointer arg) {return(s7_make_boolean(sc, s7_is_proper_list(sc, arg)));}

s7_pointer make_list_p_pp(s7_scheme *sc, s7_pointer n, s7_pointer init)
{
  s7_int len;
  if (!s7_is_integer(n))
    return(s7i_method_or_bust(sc, n, "make-list", s7i_set_plist_2(sc, n, init), "integer", 1));

  len = s7i_integer_clamped_if_gmp(sc, n);
  if (len == 0) return(s7_nil(sc));          /* what about (make-list 0 123)? */
  if (len < 0)
    out_of_range_error_nr(sc, s7_make_symbol(sc, "make-list"), s7i_wrap_integer(sc, 1), n, it_is_negative_string);
  if (len > s7i_max_list_length(sc))
    s7i_error_nr(sc, s7_make_symbol(sc, "out-of-range"),
		 s7i_set_elist_3(sc, s7i_wrap_string(sc, "make-list length argument ~D is greater than (*s7* 'max-list-length), ~D", 72),
				 s7i_wrap_integer(sc, len), s7i_wrap_integer(sc, s7i_max_list_length(sc))));
  return(s7_make_list(sc, len, init));
}

s7_pointer list_ref_p_pi_unchecked(s7_scheme *sc, s7_pointer lst, s7_int index)
{
  s7_pointer p = lst;
  if (index < 0)
    out_of_range_error_nr(sc, s7_make_symbol(sc, "list-ref"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index), it_is_negative_string);
  if (index > s7i_max_list_length(sc))
    s7i_error_nr(sc, s7_make_symbol(sc, "out-of-range"),
		 s7i_set_elist_3(sc, s7i_wrap_string(sc, "list-ref index ~D is too large, (*s7* 'max-list-length) is ~D", 61),
				 s7i_wrap_integer(sc, index), s7i_wrap_integer(sc, s7i_max_list_length(sc))));
  for (s7_int i = 0; ((s7_is_pair(p)) && (i < index)); i++, p = s7_cdr(p));
  if (!s7_is_pair(p))
    {
      if (s7_is_null(sc, p))
	out_of_range_error_nr(sc, s7_make_symbol(sc, "list-ref"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index), it_is_too_large_string);
      sole_arg_wrong_type_error_nr(sc, s7_make_symbol(sc, "list-ref"), lst, a_proper_list_string);
    }
  return(s7_car(p));
}

s7_pointer list_ref_p_pi(s7_scheme *sc, s7_pointer lst, s7_int index)
{
  if (!s7_is_pair(lst))
    sole_arg_wrong_type_error_nr(sc, s7_make_symbol(sc, "list-ref"), lst, s7i_wrap_string(sc, "pair", 4));
  return(list_ref_p_pi_unchecked(sc, lst, index));
}

s7_pointer list_ref_p_pp(s7_scheme *sc, s7_pointer lst, s7_pointer index)
{
  if (!s7_is_pair(lst))
    return(g_list_ref(sc, s7i_set_plist_2(sc, lst, index)));
  if (!s7_is_integer(index))
    sole_arg_wrong_type_error_nr(sc, s7_make_symbol(sc, "list-ref"), index, s7i_wrap_string(sc, "integer", 7));
  return(list_ref_p_pi_unchecked(sc, lst, s7i_integer_clamped_if_gmp(sc, index)));
}

no_return void list_set_index_check_nr(s7_scheme *sc, s7_int index)
{
  if (index < 0)
    out_of_range_error_nr(sc, s7_make_symbol(sc, "list-set!"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index), it_is_negative_string);
  s7i_error_nr(sc, s7_make_symbol(sc, "out-of-range"),
	       s7i_set_elist_3(sc, s7i_wrap_string(sc, "list-set! index ~D is too large, (*s7* 'max-list-length) is ~D", 62),
			       s7i_wrap_integer(sc, index), s7i_wrap_integer(sc, s7i_max_list_length(sc))));
}

s7_pointer list_set_p_pip_unchecked(s7_scheme *sc, s7_pointer lst, s7_int index, s7_pointer value)
{
  s7_pointer p = lst;
  if ((index < 0) || (index > s7i_max_list_length(sc))) list_set_index_check_nr(sc, index);
  for (s7_int i = 0; ((s7_is_pair(p)) && (i < index)); i++, p = s7_cdr(p));
  if (!s7_is_pair(p))
    {
      if (s7_is_null(sc, p))
	out_of_range_error_nr(sc, s7_make_symbol(sc, "list-set!"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index), it_is_too_large_string);
      sole_arg_wrong_type_error_nr(sc, s7_make_symbol(sc, "list-set!"), lst, a_proper_list_string);
    }
  s7_set_car(p, value);
  return(value);
}

s7_pointer list_set_p_pip(s7_scheme *sc, s7_pointer lst, s7_int index, s7_pointer value) /* called in t101-12|14... */
{
  if (!s7_is_pair(lst))
    sole_arg_wrong_type_error_nr(sc, s7_make_symbol(sc, "list-set!"), lst, s7i_wrap_string(sc, "pair", 4));
  return(list_set_p_pip_unchecked(sc, lst, index, value));
}

s7_pointer list_tail_p_pp(s7_scheme *sc, s7_pointer lst, s7_pointer ind)
{
  s7_int i, index;
  if (!s7_is_integer(ind))
    return(s7i_method_or_bust_pp(sc, ind, "list-tail", lst, ind, "integer", 2));
  index = s7i_integer_clamped_if_gmp(sc, ind);

  if (!s7_is_list(sc, lst)) /* (list-tail () 0) -> () */
    return(s7i_method_or_bust_pp(sc, lst, "list-tail", lst, ind, "a list", 1));
  if (index < 0)
    out_of_range_error_nr(sc, s7_make_symbol(sc, "list-tail"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index), it_is_negative_string);
  if (index > s7i_max_list_length(sc))
    s7i_error_nr(sc, s7_make_symbol(sc, "out-of-range"),
		 s7i_set_elist_3(sc, s7i_wrap_string(sc, "list-tail index ~D is too large, (*s7* 'max-list-length) is ~D", 62),
				 s7i_wrap_integer(sc, index), s7i_wrap_integer(sc, s7i_max_list_length(sc))));

  for (i = 0; (i < index) && (s7_is_pair(lst)); i++, lst = s7_cdr(lst)) {}
  if (i < index)
    out_of_range_error_nr(sc, s7_make_symbol(sc, "list-tail"), s7i_wrap_integer(sc, 2), s7i_wrap_integer(sc, index), it_is_too_large_string);
  return(lst);
}

s7_pointer cons_p_pp(s7_scheme *sc, s7_pointer p1, s7_pointer p2)
{
  return(s7i_cons_safe(sc, p1, p2));
}

s7_pointer car_p_p(s7_scheme *sc, s7_pointer lst)
{
  if (s7_is_pair(lst))
    return(s7_car(lst));
  return(s7i_sole_arg_method_or_bust(sc, lst, "car", s7i_set_plist_1(sc, lst), "pair"));
}

s7_pointer set_car_p_pp(s7_scheme *sc, s7_pointer lst, s7_pointer value) {return(s7i_inline_set_car(sc, lst, value));}

s7_pointer cdr_p_p(s7_scheme *sc, s7_pointer lst)
{
  if (s7_is_pair(lst))
    return(s7_cdr(lst));
  return(s7i_sole_arg_method_or_bust(sc, lst, "cdr", s7i_set_plist_1(sc, lst), "pair"));
}

s7_pointer set_cdr_p_pp(s7_scheme *sc, s7_pointer lst, s7_pointer value) {return(s7i_inline_set_cdr(sc, lst, value));}

s7_pointer assq_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  return((s7_is_pair(lst)) ? s7_assq(sc, obj, lst) :
	 ((s7_is_null(sc, lst)) ? s7_f(sc) :
	  s7i_method_or_bust_pp(sc, lst, "assq", obj, lst, "an association list", 2)));
}

s7_pointer assv_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  s7_pointer slow;
  if (!s7_is_pair(lst))
    {
      if (s7_is_null(sc, lst)) return(s7_f(sc));
      if (s7i_scheme_version_is_s7(sc))
	return(s7i_method_or_bust_pp(sc, lst, "assv", obj, lst, "an association list", 2));
      return(s7i_methods_or_bust_pp(sc, lst, "assv", "assq", obj, lst, an_association_list_string, 2));
    }
  if (s7i_is_simple(obj))
    return(s7_assq(sc, obj, lst));

  slow = lst;
  while (true)
    {
      /* here we can't play the assq == game because s7_is_eqv thinks it's getting a legit s7 object */
      if ((s7_is_pair(s7_car(lst))) && (s7_is_eqv(sc, obj, s7_caar(lst)))) return(s7_car(lst));
      lst = s7_cdr(lst);
      if (!s7_is_pair(lst)) return(s7_f(sc));

      if ((s7_is_pair(s7_car(lst))) && (s7_is_eqv(sc, obj, s7_caar(lst)))) return(s7_car(lst));
      lst = s7_cdr(lst);
      if (!s7_is_pair(lst)) return(s7_f(sc));

      slow = s7_cdr(slow);
      if (slow == lst) return(s7_f(sc));
    }
  return(s7_f(sc)); /* not reached */
}

s7_pointer assoc_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer p)
{
  if (!s7_is_pair(p))
    {
      if (s7_is_null(sc, p)) return(s7_f(sc));
      return(s7i_method_or_bust(sc, p, "assoc", s7i_set_plist_2(sc, obj, p), "an association list", 2));
    }
  if (!s7_is_pair(s7_car(p))) sole_arg_wrong_type_error_nr(sc, s7_make_symbol(sc, "assoc"), p, an_association_list_string);
  if (s7i_is_simple(obj)) return(s7_assq(sc, obj, p));
  return(s7i_assoc_1(sc, obj, p));
}

s7_pointer memq_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  return((s7_is_pair(lst)) ? s7_memq(sc, obj, lst) :
	 ((s7_is_null(sc, lst)) ? s7_f(sc) : s7i_method_or_bust_pp(sc, lst, "memq", obj, lst, "a list", 2)));
}

s7_pointer memq_2_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  if (obj == s7_car(lst)) return(lst);
  return((obj == s7_cadr(lst)) ? s7_cdr(lst) : s7_f(sc));
}

s7_pointer memq_3_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  if (obj == s7_car(lst)) return(lst);
  if (obj == s7_cadr(lst)) return(s7_cdr(lst));
  return((obj == s7_caddr(lst)) ? s7_cddr(lst) : s7_f(sc));
}

s7_pointer memq_4_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  while (true)
    {
      for (int32_t k = 0; k < 4; k++)
	{
	  if (obj == s7_car(lst)) return(lst);
	  lst = s7_cdr(lst);
	}
      if (!s7_is_pair(lst)) return(s7_f(sc));
    }
  return(s7_f(sc)); /* not reached */
}

s7_pointer memv_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  s7_pointer p;
  if (!s7_is_pair(lst))
    {
      if (s7_is_null(sc, lst)) return(s7_f(sc));
      if (s7i_scheme_version_is_s7(sc))
	return(s7i_method_or_bust_pp(sc, lst, "memv", obj, lst, "a list", 2));
      return(s7i_methods_or_bust_pp(sc, lst, "memv", "memq", obj, lst, a_list_string, 2));
    }
  if (s7i_is_simple(obj)) return(s7_memq(sc, obj, lst));
  if (s7_is_number(obj)) return(s7i_memv_number(sc, obj, lst));

  p = lst;
  while (true)
    {
      if (s7_is_eqv(sc, obj, s7_car(lst))) return(lst);
      lst = s7_cdr(lst);
      if (!s7_is_pair(lst)) return(s7_f(sc));

      if (s7_is_eqv(sc, obj, s7_car(lst))) return(lst);
      lst = s7_cdr(lst);
      if (!s7_is_pair(lst)) return(s7_f(sc));

      p = s7_cdr(p);
      if (p == lst) return(s7_f(sc));
    }
  return(s7_f(sc)); /* not reached */
}

s7_pointer member_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst)
{
  if (s7_is_null(sc, lst)) return(s7_f(sc));
  if (!s7_is_pair(lst)) return(s7i_method_or_bust(sc, lst, "member", s7i_set_plist_2(sc, obj, lst), "a list", 2));
  if (s7i_is_simple(obj)) return(s7_memq(sc, obj, lst));
  if (s7_is_number(obj)) return(s7i_memv_number(sc, obj, lst));
  return(s7i_member(sc, obj, lst));
}
