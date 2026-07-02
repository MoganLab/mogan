/* s7_liii_list.c - list utility implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#include "s7_liii_list.h"
#include "s7_internal_helpers.h"

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
