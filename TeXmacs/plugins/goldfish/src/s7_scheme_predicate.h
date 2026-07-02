/* s7_scheme_predicate.h - predicate function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_PREDICATE_H
#define S7_SCHEME_PREDICATE_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_not(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_boolean(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_unspecified(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_number(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_integer(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_real(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_complex(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_rational(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_keyword(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_procedure(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_dilambda(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_sequence(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_symbol(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_input_port(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_output_port(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_macro(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_undefined(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_eof_object(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_byte(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_float(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_random_state(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_continuation(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_iterator(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_gensym(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_syntax(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_let(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_goto(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_constant(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_c_object(s7_scheme *sc, s7_pointer args);
s7_pointer g_help(s7_scheme *sc, s7_pointer args);
s7_pointer g_arity(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_c_pointer(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_openlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_funclet(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_is_cyclic(s7_scheme *sc, s7_pointer args);
s7_pointer g_type_of(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_eq(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_eqv(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_equal(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_equivalent(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_port_closed(s7_scheme *sc, s7_pointer args);
s7_pointer g_iterator_sequence(s7_scheme *sc, s7_pointer args);
s7_pointer g_c_pointer_info(s7_scheme *sc, s7_pointer args);
s7_pointer g_c_pointer_type(s7_scheme *sc, s7_pointer args);
s7_pointer g_c_object_type(s7_scheme *sc, s7_pointer args);
s7_pointer g_c_object_let(s7_scheme *sc, s7_pointer args);
s7_pointer g_c_pointer_weak1(s7_scheme *sc, s7_pointer args);
s7_pointer g_c_pointer_weak2(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_leaves(s7_scheme *sc, s7_pointer args);
s7_pointer g_cyclic_sequences(s7_scheme *sc, s7_pointer args);
s7_pointer g_object_to_let(s7_scheme *sc, s7_pointer args);
s7_pointer g_pair_line_number(s7_scheme *sc, s7_pointer args);
s7_pointer g_port_line_number(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_memq(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_set_memq(s7_scheme *sc, s7_pointer args);
s7_pointer g_format_nr(s7_scheme *sc, s7_pointer args);
s7_pointer g_tree_set_memq_syms(s7_scheme *sc, s7_pointer args);
s7_pointer g_heap_analyze(s7_scheme *sc, s7_pointer args);
s7_pointer g_show_op_stack(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_op_stack(s7_scheme *sc, s7_pointer args);
s7_pointer g_heap_holder(s7_scheme *sc, s7_pointer args);
s7_pointer g_heap_holders(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_defined_in_unlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_defined_in_rootlet(s7_scheme *sc, s7_pointer args);
#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_PREDICATE_H */
