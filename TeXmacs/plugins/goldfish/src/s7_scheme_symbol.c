/* s7_scheme_symbol.c - symbol and keyword function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_symbol.h"
#include "s7_internal_helpers.h"
#include <string.h>

/* -------------------------------- symbol->string -------------------------------- */

s7_pointer g_symbol_to_string(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = s7_car(args);
  if (!s7_is_symbol(sym))
    return(s7i_sole_arg_method_or_bust(sc, sym, "symbol->string", args, "a symbol"));
  if (s7i_symbol_name_length(sym) > s7i_max_string_length(sc))
    return(s7_out_of_range_error(sc, "symbol->string", 1, sym, "symbol name is too large"));
  return(s7_make_string_with_length(sc, s7_symbol_name(sym), s7i_symbol_name_length(sym)));
}

s7_pointer g_symbol_to_string_uncopied(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = s7_car(args);
  if (!s7_is_symbol(sym))
    return(s7i_sole_arg_method_or_bust(sc, sym, "symbol->string", args, "a symbol"));
  if (s7i_is_gensym(sym))
    return(s7_make_string_with_length(sc, s7_symbol_name(sym), s7i_symbol_name_length(sym)));
  return(s7i_symbol_name_cell(sym));
}

s7_pointer symbol_to_string_p_p(s7_scheme *sc, s7_pointer sym)
{
  if (!s7_is_symbol(sym))
    return(s7i_sole_arg_method_or_bust(sc, sym, "symbol->string", s7_cons(sc, sym, s7_nil(sc)), "a symbol"));
  if (s7i_symbol_name_length(sym) > s7i_max_string_length(sc))
    return(s7_out_of_range_error(sc, "symbol->string", 1, sym, "symbol name is too large"));
  return(s7_make_string_with_length(sc, s7_symbol_name(sym), s7i_symbol_name_length(sym)));
}

s7_pointer symbol_to_string_uncopied_p(s7_scheme *sc, s7_pointer sym)
{
  if (!s7_is_symbol(sym))
    return(s7i_sole_arg_method_or_bust(sc, sym, "symbol->string", s7_cons(sc, sym, s7_nil(sc)), "a symbol"));
  if (s7i_is_gensym(sym))
    return(s7_make_string_with_length(sc, s7_symbol_name(sym), s7i_symbol_name_length(sym)));
  return(s7i_symbol_name_cell(sym));
}


/* -------------------------------- string->symbol -------------------------------- */

static s7_pointer g_string_to_symbol_1(s7_scheme *sc, s7_pointer str)
{
  if (!s7_is_string(str))
    return(s7i_method_or_bust_p(sc, str, "string->symbol", "a string"));
  if (s7_string_length(str) <= 0)
    return(s7_wrong_type_arg_error(sc, "string->symbol", 1, str, "a non-null string"));
  return(s7i_make_symbol_with_length(sc, s7_string(str), s7_string_length(str)));
}

s7_pointer g_string_to_symbol(s7_scheme *sc, s7_pointer args)
{
  return(g_string_to_symbol_1(sc, s7_car(args)));
}

s7_pointer string_to_symbol_p_p(s7_scheme *sc, s7_pointer p)
{
  return(g_string_to_symbol_1(sc, p));
}


/* -------------------------------- keyword->symbol -------------------------------- */

s7_pointer g_keyword_to_symbol(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = s7_car(args);
  if (!s7_is_keyword(sym))
    return(s7i_method_or_bust_p(sc, sym, "keyword->symbol", "a keyword"));
  return(s7_keyword_to_symbol(sc, sym));
}


/* -------------------------------- symbol->keyword -------------------------------- */

s7_pointer g_symbol_to_keyword(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = s7_car(args);
  if (!s7_is_symbol(sym))
    return(s7i_sole_arg_method_or_bust(sc, sym, "symbol->keyword", args, "a symbol"));
  return(s7_make_keyword(sc, s7_symbol_name(sym)));
}


/* -------------------------------- string->keyword -------------------------------- */

s7_pointer g_string_to_keyword(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  if (!s7_is_string(str))
    return(s7i_sole_arg_method_or_bust(sc, str, "string->keyword", args, "a string"));
  if ((s7_string_length(str) == 0) ||
      (s7_string(str)[0] == '\0'))
    return(s7_out_of_range_error(sc, "string->keyword", 1, str, "string->keyword wants a non-null string"));
  return(s7_make_keyword(sc, s7_string(str)));
}


/* -------------------------------- symbol-initial-value -------------------------------- */

s7_pointer g_symbol_initial_value(s7_scheme *sc, s7_pointer args)
{
  s7_pointer symbol = s7_car(args);
  if (!s7_is_symbol(symbol))
    return(s7i_sole_arg_method_or_bust(sc, symbol, "symbol-initial-value", s7_cons(sc, symbol, s7_nil(sc)), "a symbol"));
  return(s7i_initial_value(symbol));
}
