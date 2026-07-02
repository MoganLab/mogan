/* s7_liii_hash_table.c - hash-table utility implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccmu.stanford.edu
 */

#include "s7_liii_hash_table.h"
#include "s7_internal_helpers.h"

s7_pointer g_is_hash_table(s7_scheme *sc, s7_pointer args)
{
  #define H_is_hash_table "(hash-table? obj) returns #t if obj is a hash-table"
  #define Q_is_hash_table sc->pl_bt

  s7_pointer p = s7_car(args);
  if (s7_is_hash_table(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "hash-table?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_hash_table_size(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7_car(args);
  if (!s7_is_hash_table(table))
    return(s7i_sole_arg_method_or_bust(sc, table, "hash-table-size", args, "a hash-table"));
  return(s7_make_integer(sc, s7i_hash_table_entries(table)));
}

s7_int hash_table_size_i_7p(s7_scheme *sc, s7_pointer table)
{
  if (!s7_is_hash_table(table))
    return(s7_integer(s7i_method_or_bust_p(sc, table, "hash-table-size", "a hash-table")));
  return(s7i_hash_table_entries(table));
}

s7_pointer g_hash_table_ref_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7_car(args);
  if (!s7_is_hash_table(table))
    return(s7i_method_or_bust(sc, table, "hash-table-ref", args, "a hash-table", 1));
  return(s7_hash_table_ref(sc, table, s7_cadr(args)));
}

s7_pointer g_hash_table_key_typer(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7_car(args);
  if (!s7_is_hash_table(table))
    return(s7i_sole_arg_method_or_bust(sc, table, "hash-table-key-typer", args, "a hash-table"));
  return(s7i_hash_table_key_typer(sc, table));
}

s7_pointer g_hash_table_value_typer(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7_car(args);
  if (!s7_is_hash_table(table))
    return(s7i_sole_arg_method_or_bust(sc, table, "hash-table-value-typer", args, "a hash-table"));
  return(s7i_hash_table_value_typer(sc, table));
}

s7_pointer g_hash_table_set(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7_car(args);
  if (!s7_is_hash_table(table))
    return(s7i_method_or_bust(sc, table, "s7-hash-table-set!", args, "a hash-table", 1));
  if (s7_is_immutable(table))
    return(s7_wrong_type_arg_error(sc, "s7-hash-table-set!", 1, table, "a mutable hash-table"));
  return(s7_hash_table_set(sc, table, s7_cadr(args), s7_caddr(args)));
}

s7_pointer g_hash_table_ref(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7_car(args);
  s7_pointer result;
  if (!s7_is_hash_table(table))
    return(s7i_method_or_bust(sc, table, "hash-table-ref", args, "a hash-table", 1));
  result = s7_hash_table_ref(sc, table, s7_cadr(args));
  if (s7_is_pair(s7_cddr(args)))
    return(s7i_ref_index_checked(sc, s7_symbol_value(sc, s7_make_symbol(sc, "hash-table-ref")), result, args));
  return(result);
}

s7_pointer g_hash_table(s7_scheme *sc, s7_pointer args)
{
  return(s7i_hash_table_1(sc, args, s7_make_symbol(sc, "hash-table")));
}

s7_pointer g_hash_table_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7_make_hash_table(sc, 8); /* sc->default_hash_table_length */
  if (s7_cadr(args) != s7i_missing_key_value(sc)) /* #f default */
    s7i_hash_table_add(sc, table, s7_car(args), s7_cadr(args));
  return(table);
}

s7_pointer g_weak_hash_table(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7i_hash_table_1(sc, args, s7_make_symbol(sc, "weak-hash-table"));
  s7i_set_weak_hash_table(table);
  s7i_set_weak_hash_table_iters(table, 0);
  return(table);
}

s7_pointer g_make_hash_table(s7_scheme *sc, s7_pointer args)
{
  return(s7i_make_hash_table_1(sc, args, s7_make_symbol(sc, "s7-make-hash-table")));
}

s7_pointer g_make_weak_hash_table(s7_scheme *sc, s7_pointer args)
{
  s7_pointer table = s7i_make_hash_table_1(sc, args, s7_make_symbol(sc, "make-weak-hash-table"));
  if (s7_is_hash_table(table))
    {
      s7i_set_weak_hash_table(table);
      s7i_set_weak_hash_table_iters(table, 0);
    }
  return(table);
}

s7_pointer g_is_weak_hash_table(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_hash_table(p) && s7i_is_weak_hash_table(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "weak-hash-table?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

s7_pointer g_hash_code(s7_scheme *sc, s7_pointer args)
{
  s7_pointer obj = s7_car(args);
  s7_int code;
  if ((s7_is_pair(s7_cdr(args))) &&
      (!s7_is_procedure(s7_cadr(args))))
    return(s7_wrong_type_arg_error(sc, "hash-code", 2, s7_cadr(args), "a function"));
  code = s7_hash_code(sc, obj, s7_undefined(sc));
#ifdef __clang__
  if (code < 0) code = 0;
#endif
  return(s7_make_integer(sc, code));
}
