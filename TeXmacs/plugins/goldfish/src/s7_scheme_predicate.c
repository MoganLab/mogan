/* s7_scheme_predicate.c - predicate implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_predicate.h"
#include "s7_internal_helpers.h"

#ifndef S7_DEBUGGING
  #define S7_DEBUGGING 0
#endif

s7_pointer g_not(s7_scheme *sc, s7_pointer args)
{
  return((s7_car(args) == s7_f(sc)) ? s7_t(sc) : s7_f(sc));
}

#define S7_TYPE_PREDICATE(Func_name, Fast_check, Symbol_fn) \
s7_pointer Func_name(s7_scheme *sc, s7_pointer args)          \
{                                                             \
  s7_pointer p = s7_car(args);                                \
  if (Fast_check) return(s7_t(sc));                           \
  if (!s7i_has_active_methods(sc, p)) return(s7_f(sc));       \
  return(s7i_apply_boolean_method(sc, p, Symbol_fn(sc)));     \
}

S7_TYPE_PREDICATE(g_is_boolean, s7_is_boolean(p), s7i_is_boolean_symbol)
S7_TYPE_PREDICATE(g_is_unspecified, s7_is_unspecified(sc, p), s7i_is_unspecified_symbol)
S7_TYPE_PREDICATE(g_is_number, s7_is_number(p), s7i_is_number_symbol)
S7_TYPE_PREDICATE(g_is_integer, s7_is_integer(p), s7i_is_integer_symbol)
S7_TYPE_PREDICATE(g_is_real, s7_is_real(p), s7i_is_real_symbol)
S7_TYPE_PREDICATE(g_is_rational, s7_is_rational(p), s7i_is_rational_symbol)
S7_TYPE_PREDICATE(g_is_keyword, s7_is_keyword(p), s7i_is_keyword_symbol)
S7_TYPE_PREDICATE(g_is_dilambda, s7_is_dilambda(p), s7i_is_dilambda_symbol)
S7_TYPE_PREDICATE(g_is_sequence, s7i_is_sequence(p), s7i_is_sequence_symbol)
S7_TYPE_PREDICATE(g_is_symbol, s7_is_symbol(p), s7i_is_symbol_symbol)
S7_TYPE_PREDICATE(g_is_input_port, s7_is_input_port(sc, p), s7i_is_input_port_symbol)
S7_TYPE_PREDICATE(g_is_output_port, s7_is_output_port(sc, p), s7i_is_output_port_symbol)
S7_TYPE_PREDICATE(g_is_macro, s7_is_macro(sc, p), s7i_is_macro_symbol)
S7_TYPE_PREDICATE(g_is_undefined, s7i_is_undefined(p), s7i_is_undefined_symbol)
S7_TYPE_PREDICATE(g_is_eof_object, s7i_is_eof(p), s7i_is_eof_object_symbol)
S7_TYPE_PREDICATE(g_is_float, s7i_is_t_real(p), s7i_is_float_symbol)
S7_TYPE_PREDICATE(g_is_random_state, s7_is_random_state(p), s7i_is_random_state_symbol)
S7_TYPE_PREDICATE(g_is_continuation, s7i_is_continuation(p), s7i_is_continuation_symbol)
S7_TYPE_PREDICATE(g_is_iterator, s7_is_iterator(p), s7i_is_iterator_symbol)
S7_TYPE_PREDICATE(g_is_syntax, s7_is_syntax(p), s7i_is_syntax_symbol)
S7_TYPE_PREDICATE(g_is_let, s7_is_let(p), s7i_is_let_symbol)
S7_TYPE_PREDICATE(g_is_c_object, s7_is_c_object(p), s7i_is_c_object_symbol)

s7_pointer g_is_procedure(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7_is_procedure(s7_car(args))));
}

s7_pointer g_is_complex(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_number(p)) return(s7_t(sc));
  if (!s7i_has_active_methods(sc, p)) return(s7_f(sc));
  return(s7i_apply_boolean_method(sc, p, s7i_is_complex_symbol(sc)));
}

s7_pointer g_is_byte(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_integer(p) && s7_integer(p) >= 0 && s7_integer(p) < 256) return(s7_t(sc));
  if (!s7i_has_active_methods(sc, p)) return(s7_f(sc));
  return(s7i_apply_boolean_method(sc, p, s7i_is_byte_symbol(sc)));
}

s7_pointer g_is_gensym(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_symbol(p) && s7i_is_gensym(p)) return(s7_t(sc));
  if (!s7i_has_active_methods(sc, p)) return(s7_f(sc));
  return(s7i_apply_boolean_method(sc, p, s7i_is_gensym_symbol(sc)));
}

s7_pointer g_is_goto(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7i_is_goto(s7_car(args))));
}

s7_pointer g_is_constant(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7i_is_constant(sc, s7_car(args))));
}

s7_pointer g_help(s7_scheme *sc, s7_pointer args)
{
  s7_pointer obj = s7_car(args);
  /* if_method_exists_return_value expansion */
  if (s7i_has_active_methods(sc, obj))
    {
      s7_pointer func = s7i_find_method_with_let(sc, obj, s7i_help_symbol(sc));
      if (func != s7_undefined(sc))
        return(s7_apply_function(sc, func, args));
    }
  const char *doc = s7_help(sc, obj);
  return((doc) ? s7_make_string(sc, doc) : s7_f(sc));
}

s7_pointer g_arity(s7_scheme *sc, s7_pointer args)
{
  return(s7_arity(sc, s7_car(args)));
}

s7_pointer g_is_c_pointer(s7_scheme *sc, s7_pointer args)
{
  s7_pointer obj = s7_car(args);
  if (s7_is_c_pointer(obj))
    return((s7_is_pair(s7_cdr(args))) ? s7_make_boolean(sc, s7i_c_pointer_type(obj) == s7_cadr(args)) : s7_t(sc));
  if (!s7i_has_active_methods(sc, obj)) return(s7_f(sc));
  return(s7i_apply_boolean_method(sc, obj, s7i_is_c_pointer_symbol(sc)));
}

s7_pointer g_is_openlet(s7_scheme *sc, s7_pointer args)
{
  s7_pointer let = s7_car(args);
  /* if_method_exists_return_value expansion */
  if (s7i_has_active_methods(sc, let))
    {
      s7_pointer func = s7i_find_method_with_let(sc, let, s7i_is_openlet_symbol(sc));
      if (func != s7_undefined(sc))
        return(s7_apply_function(sc, func, args));
    }
  return(s7_make_boolean(sc, s7i_has_methods(let)));
}

s7_pointer g_is_funclet(s7_scheme *sc, s7_pointer args)
{
  s7_pointer let = s7_car(args);
  if (let == s7i_rootlet(sc)) return(s7_f(sc));
  if ((s7_is_let(let)) && ((s7i_is_funclet(let)) || (s7i_is_maclet(let))))
    return(s7_t(sc));
  if (!s7i_has_active_methods(sc, let))
    return(s7_f(sc));
  return(s7i_apply_boolean_method(sc, let, s7i_is_funclet_symbol(sc)));
}

s7_pointer g_tree_is_cyclic(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7i_tree_is_cyclic(sc, s7_car(args))));
}

s7_pointer g_type_of(s7_scheme *sc, s7_pointer args)
{
  return(s7i_type_of(sc, s7_car(args)));
}

s7_pointer g_is_eq(s7_scheme *sc, s7_pointer args)
{
  s7_pointer a = s7_car(args);
  s7_pointer b = s7_cadr(args);
  return(s7_make_boolean(sc, ((a == b) ||
    ((s7_is_unspecified(sc, a)) && (s7_is_unspecified(sc, b))))));
}

s7_pointer g_is_eqv(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7_is_eqv(sc, s7_car(args), s7_cadr(args))));
}

s7_pointer g_is_equal(s7_scheme *sc, s7_pointer args)
{
  #define H_is_equal "(equal? obj1 obj2) returns #t if obj1 is equal to obj2"
  #define Q_is_equal sc->pcl_bt
  return(s7_make_boolean(sc, s7_is_equal(sc, s7_car(args), s7_cadr(args))));
}

s7_pointer g_is_equivalent(s7_scheme *sc, s7_pointer args)
{
  #define H_is_equivalent "(equivalent? obj1 obj2) returns #t if obj1 is close enough to obj2."
  #define Q_is_equivalent sc->pcl_bt
  return(s7_make_boolean(sc, s7_is_equivalent(sc, s7_car(args), s7_cadr(args))));
}

s7_pointer g_is_port_closed(s7_scheme *sc, s7_pointer args)
{
  s7_pointer port = s7_car(args);
  if (s7_is_input_port(sc, port) || s7_is_output_port(sc, port))
    return(s7_make_boolean(sc, s7i_port_is_closed(port)));
  if ((port == s7_current_output_port(sc)) && (port == s7_f(sc)))
    return(s7_f(sc));
  return(s7i_method_or_bust_p(sc, port, "port-closed?", "a port"));
}

s7_pointer g_iterator_sequence(s7_scheme *sc, s7_pointer args)
{
  #define H_iterator_sequence "(iterator-sequence iterator) returns the sequence that iterator is traversing."
  #define Q_iterator_sequence s7_make_signature(sc, 2, sc->is_sequence_symbol, sc->is_iterator_symbol)
  s7_pointer iter = s7_car(args);
  if (!s7_is_iterator(iter))
    return(s7i_sole_arg_method_or_bust(sc, iter, "iterator-sequence", args, "an iterator"));
  return(s7i_iterator_sequence(iter));
}

s7_pointer g_c_pointer_info(s7_scheme *sc, s7_pointer args)
{
  return(s7i_c_pointer_info_p_p(sc, s7_car(args)));
}

s7_pointer g_c_pointer_type(s7_scheme *sc, s7_pointer args)
{
  return(s7i_c_pointer_type_p_p(sc, s7_car(args)));
}

s7_pointer g_c_object_type(s7_scheme *sc, s7_pointer args)
{
  #define H_c_object_type "(c-object-type obj) returns the c_object's type tag."
  #define Q_c_object_type s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_c_object_symbol)

  s7_pointer cobj = s7_car(args);
  if (!s7_is_c_object(cobj))
    return(s7i_sole_arg_method_or_bust(sc, cobj, "c-object-type", args, "a c-object"));
  return(s7_make_integer(sc, s7_c_object_type(cobj)));
}

s7_pointer g_c_object_let(s7_scheme *sc, s7_pointer args)
{
  #define H_c_object_let "(c-object-let obj) returns the c_object's local let, if any."
  #define Q_c_object_let s7_make_signature(sc, 2, sc->is_let_symbol, sc->is_c_object_symbol)

  s7_pointer cobj = s7_car(args);
  if (!s7_is_c_object(cobj))
    return(s7i_sole_arg_method_or_bust(sc, cobj, "c-object-let", args, "a c-object"));
  return(s7_c_object_let(cobj));
}

/* ---- Pattern A: thin wrappers (delegate to one internal helper) ---- */

s7_pointer g_c_pointer_weak1(s7_scheme *sc, s7_pointer args)
{
  #define H_c_pointer_weak1 "(c-pointer-weak1 obj) returns the c-pointer weak1 field"
  #define Q_c_pointer_weak1 s7_make_signature(sc, 2, sc->T, sc->is_c_pointer_symbol)
  return(s7i_c_pointer_weak1_p_p(sc, s7_car(args)));
}

s7_pointer g_c_pointer_weak2(s7_scheme *sc, s7_pointer args)
{
  #define H_c_pointer_weak2 "(c-pointer-weak2 obj) returns the c-pointer weak2 field"
  #define Q_c_pointer_weak2 s7_make_signature(sc, 2, sc->T, sc->is_c_pointer_symbol)
  return(s7i_c_pointer_weak2_p_p(sc, s7_car(args)));
}

s7_pointer g_tree_leaves(s7_scheme *sc, s7_pointer args)
{
  #define H_tree_leaves "(tree-leaves tree) returns the number of leaves in the tree"
  #define Q_tree_leaves s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_list_symbol)
  return(s7i_tree_leaves_p_p(sc, s7_car(args)));
}

s7_pointer g_cyclic_sequences(s7_scheme *sc, s7_pointer args)
{
  #define H_cyclic_sequences "(cyclic-sequences obj) returns a list of elements that are cyclic."
  #define Q_cyclic_sequences s7_make_signature(sc, 2, sc->is_proper_list_symbol, sc->T)
  return(s7i_cyclic_sequences_p_p(sc, s7_car(args)));
}

s7_pointer g_object_to_let(s7_scheme *sc, s7_pointer args)
{
  #define H_object_to_let "(object->let obj) returns a let (namespace) describing obj."
  #define Q_object_to_let s7_make_signature(sc, 2, sc->is_let_symbol, sc->T)
  return(s7i_object_to_let_p_p(sc, s7_car(args)));
}

s7_pointer g_pair_line_number(s7_scheme *sc, s7_pointer args)
{
  #define H_pair_line_number "(pair-line-number pair) returns the line number at which it read 'pair', or #f if no such number is available"
  #define Q_pair_line_number s7_make_signature(sc, 2, s7_make_signature(sc, 2, sc->is_integer_symbol, sc->not_symbol), sc->is_pair_symbol)
  return(s7i_pair_line_number_p_p(sc, s7_car(args)));
}

s7_pointer g_port_line_number(s7_scheme *sc, s7_pointer args)
{
  #define H_port_line_number "(port-line-number input-file-port) returns the current read line number of port"
  #define Q_port_line_number s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_input_port_symbol)
  return(s7i_port_line_number_p_p(sc, (s7_is_null(sc, args)) ? s7_current_input_port(sc) : s7_car(args)));
}

/* ---- Pattern B: make_boolean wrappers ---- */

s7_pointer g_tree_memq(s7_scheme *sc, s7_pointer args)
{
  #define H_tree_memq "(tree-memq obj tree) is a tree-oriented version of memq, but returning #t if the object is in the tree."
  #define Q_tree_memq s7_make_signature(sc, 3, sc->is_boolean_symbol, sc->T, sc->is_list_symbol)
  return(s7_make_boolean(sc, s7i_tree_memq_b_7pp(sc, s7_car(args), s7_cadr(args))));
}

s7_pointer g_tree_set_memq(s7_scheme *sc, s7_pointer args)
{
  #define H_tree_set_memq "(tree-set-memq symbols tree) returns #t if any of the list of symbols is in the tree"
  #define Q_tree_set_memq s7_make_signature(sc, 3, sc->is_boolean_symbol, sc->is_list_symbol, sc->is_list_symbol)
  return(s7_make_boolean(sc, s7i_tree_set_memq_b_7pp(sc, s7_car(args), s7_cadr(args))));
}

/* ---- Pattern C: struct accessors ---- */

s7_pointer g_format_nr(s7_scheme *sc, s7_pointer args)  /* port == #f, in do body, args already evaluated */
{
  return(s7i_nil_string());
}

s7_pointer g_tree_set_memq_syms(s7_scheme *sc, s7_pointer args)
{
  return(s7i_tree_set_memq_syms_direct(sc, s7_car(args), s7_cadr(args)));
}

#if S7_DEBUGGING
s7_pointer g_heap_analyze(s7_scheme *sc, s7_pointer args)
{
  s7i_heap_analyze(sc);
  return(s7_f(sc));
}

s7_pointer g_show_op_stack(s7_scheme *sc, s7_pointer args)
{
  s7i_show_op_stack(sc);
  return(s7_f(sc));
}

s7_pointer g_is_op_stack(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_boolean(sc, s7i_is_op_stack_active(sc)));
}

s7_pointer g_heap_holder(s7_scheme *sc, s7_pointer args)
{
  return(s7i_heap_holder_p_p(sc, s7_car(args)));
}

s7_pointer g_heap_holders(s7_scheme *sc, s7_pointer args)
{
  return(s7_make_integer(sc, s7i_heap_holders(s7_car(args))));
}
#endif

s7_pointer g_is_defined_in_unlet(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = s7_car(args);
  if (!s7_is_symbol(sym))
    return(s7_wrong_type_arg_error(sc, "defined?", 1, sym, "a symbol"));
  return(s7_make_boolean(sc, s7i_initial_value_is_defined(sc, sym)));
}

s7_pointer g_is_defined_in_rootlet(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = s7_car(args);
  if (!s7_is_symbol(sym))
    return(s7_wrong_type_arg_error(sc, "defined?", 1, sym, "a symbol"));
  return(s7_make_boolean(sc, s7i_is_defined_in_rootlet(sc, sym)));
}

