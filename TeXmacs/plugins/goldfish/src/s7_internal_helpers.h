/* s7_internal_helpers.h - internal helper bridge declarations
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_INTERNAL_HELPERS_H
#define S7_INTERNAL_HELPERS_H

#include "s7.h"

#ifndef S7_INT64_MAX
  #define S7_INT64_MAX 9223372036854775807LL
#endif
#ifndef S7_INT64_MIN
  #define S7_INT64_MIN (int64_t)(-S7_INT64_MAX - 1LL)
#endif
#ifndef s7_int_abs
  #if defined(__GNUC__) || defined(__clang__)
    #define s7_int_abs(x) ({s7_int _X_; _X_ = x; _X_ >= 0 ? _X_ : -_X_;})
  #else
    #define s7_int_abs(x) ((x) >= 0 ? (x) : -(x))
  #endif
#endif

#if HAVE_OVERFLOW_CHECKS
  #if defined(__clang__)
    #define multiply_overflow(A, B, C) __builtin_mul_overflow(A, B, C)
  #elif defined(__GNUC__) && (__GNUC__ >= 5)
    #define multiply_overflow(A, B, C) __builtin_mul_overflow(A, B, C)
  #endif
#endif

/* forward declarations for internal types used in bridge functions */
typedef struct block_t block_t;

#ifndef ld64
  #include <inttypes.h>
  #define ld64 PRId64
#endif

#ifndef no_return
  #ifdef _MSC_VER
    #define no_return _Noreturn
  #elif defined(__GNUC__) || defined(__clang__)
    #define no_return __attribute__((noreturn))
  #else
    #define no_return
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* format_data_t type - needed by s7_scheme_format.c */
typedef struct {
  s7_int loc, curly_len, ctr;
  char *curly_str;
  s7_pointer args, orig_str, curly_arg, port, strport;
} format_data_t;

s7_pointer s7i_method_or_bust(s7_scheme *sc, s7_pointer obj, const char *method_name,
                              s7_pointer args, const char *type_name, s7_int arg_pos);

bool s7i_method_or_bust_bool(s7_scheme *sc, s7_pointer obj, const char *method_name,
                             s7_pointer args, const char *type_name, s7_int arg_pos);

s7_pointer s7i_sole_arg_method_or_bust(s7_scheme *sc, s7_pointer obj, const char *method_name, s7_pointer args, const char *type_name);

bool s7i_sole_arg_method_or_bust_bool(s7_scheme *sc, s7_pointer obj, const char *method_name, s7_pointer args, const char *type_name);

bool s7i_is_sequence(s7_pointer p);
bool s7i_sequence_is_empty(s7_scheme *sc, s7_pointer seq);
s7_int s7i_sequence_length(s7_scheme *sc, s7_pointer seq);
s7_pointer s7i_find_method_with_let(s7_scheme *sc, s7_pointer obj, s7_pointer method);
bool s7i_has_active_methods(s7_scheme *sc, s7_pointer obj);
/* boolean method dispatch for type predicate migration */
s7_pointer s7i_apply_boolean_method(s7_scheme *sc, s7_pointer obj, s7_pointer method);
void s7i_wrong_type_error_nr(s7_scheme *sc, s7_pointer caller, s7_int arg_num, s7_pointer arg, s7_pointer typ);
s7_pointer s7i_copy_1(s7_scheme *sc, s7_pointer caller, s7_pointer args);
s7_pointer s7i_copy_proper_list(s7_scheme *sc, s7_pointer lst);
s7_int s7i_position_of(const s7_pointer p, s7_pointer args);
s7_pointer s7i_nil_string(void);
s7_pointer s7i_make_empty_string(s7_scheme *sc, s7_int len, char fill);
s7_int s7i_max_string_length(s7_scheme *sc);
s7_int s7i_max_list_length(s7_scheme *sc);

s7_pointer s7i_string_append_1(s7_scheme *sc, s7_pointer args, s7_pointer caller);
s7_pointer s7i_string_1(s7_scheme *sc, s7_pointer args, s7_pointer sym);
s7_pointer s7i_string_c1(s7_scheme *sc, s7_pointer args);
s7_pointer s7i_string_to_number(s7_scheme *sc, char *str, int32_t radix);
s7_pointer make_atom(s7_scheme *sc, char *q, int32_t radix, bool want_symbol, bool with_error);

/* pre-allocated list helpers for string-append migration */
s7_pointer s7i_set_plist_2(s7_scheme *sc, s7_pointer x1, s7_pointer x2);
s7_pointer s7i_set_ulist_1(s7_scheme *sc, s7_pointer x1, s7_pointer x2);
void s7i_string_append_length_error(s7_scheme *sc, s7_pointer caller, s7_int len);
bool s7i_is_string_append_or_symbol_caller(s7_scheme *sc, s7_pointer caller);
void s7i_set_string_value(s7_pointer str, const char *val);
char *s7i_string_value_ptr(s7_pointer str);

/* string comparison helpers for string cmp migration */
int32_t s7i_scheme_strcmp(s7_pointer s1, s7_pointer s2);
bool s7i_scheme_strings_are_equal(s7_pointer x, s7_pointer y);
bool s7i_is_string_via_method(s7_scheme *sc, s7_pointer obj);
s7_pointer s7i_method_or_bust_sym(s7_scheme *sc, s7_pointer obj, s7_pointer method_sym, s7_pointer args, s7_pointer typ, s7_int arg_pos);
s7_pointer s7i_set_plist_1(s7_scheme *sc, s7_pointer x1);
s7_pointer s7i_string_type_name(s7_scheme *sc);
s7_pointer s7i_string_eq_symbol(s7_scheme *sc);
s7_pointer s7i_string_lt_symbol(s7_scheme *sc);
s7_pointer s7i_string_gt_symbol(s7_scheme *sc);
s7_pointer s7i_string_leq_symbol(s7_scheme *sc);
s7_pointer s7i_string_geq_symbol(s7_scheme *sc);
bool s7i_is_true(s7_scheme *sc, s7_pointer p);
s7_pointer s7i_is_string_symbol(s7_scheme *sc);
s7_pointer s7i_is_boolean_symbol(s7_scheme *sc);
s7_pointer s7i_is_unspecified_symbol(s7_scheme *sc);
s7_pointer s7i_is_number_symbol(s7_scheme *sc);
s7_pointer s7i_is_integer_symbol(s7_scheme *sc);
s7_pointer s7i_is_real_symbol(s7_scheme *sc);
s7_pointer s7i_is_complex_symbol(s7_scheme *sc);
s7_pointer s7i_is_rational_symbol(s7_scheme *sc);
s7_pointer s7i_is_keyword_symbol(s7_scheme *sc);
s7_pointer s7i_is_dilambda_symbol(s7_scheme *sc);
s7_pointer s7i_is_sequence_symbol(s7_scheme *sc);
s7_pointer s7i_is_symbol_symbol(s7_scheme *sc);
s7_pointer s7i_is_input_port_symbol(s7_scheme *sc);
s7_pointer s7i_is_output_port_symbol(s7_scheme *sc);
s7_pointer s7i_is_macro_symbol(s7_scheme *sc);
s7_pointer s7i_is_undefined_symbol(s7_scheme *sc);
s7_pointer s7i_is_eof_object_symbol(s7_scheme *sc);
s7_pointer s7i_is_byte_symbol(s7_scheme *sc);
s7_pointer s7i_is_float_symbol(s7_scheme *sc);
s7_pointer s7i_is_random_state_symbol(s7_scheme *sc);
s7_pointer s7i_is_continuation_symbol(s7_scheme *sc);
s7_pointer s7i_is_iterator_symbol(s7_scheme *sc);
s7_pointer s7i_is_gensym_symbol(s7_scheme *sc);
s7_pointer s7i_is_syntax_symbol(s7_scheme *sc);
s7_pointer s7i_is_let_symbol(s7_scheme *sc);
bool s7i_is_goto(s7_pointer p);
bool s7i_is_constant(s7_scheme *sc, s7_pointer p);
s7_pointer s7i_is_c_object_symbol(s7_scheme *sc);
s7_pointer s7i_help_symbol(s7_scheme *sc);
bool s7i_is_undefined(s7_pointer p);
bool s7i_is_eof(s7_pointer p);
bool s7i_is_t_real(s7_pointer p);
bool s7i_is_continuation(s7_pointer p);
const uint8_t *s7i_uppers_ptr(void);

/* bridge functions for s7_scheme_predicate.c migration */
s7_pointer s7i_c_pointer_type(s7_pointer p);
bool s7i_has_methods(s7_pointer p);
bool s7i_is_funclet(s7_pointer p);
bool s7i_is_maclet(s7_pointer p);
s7_pointer s7i_rootlet(s7_scheme *sc);
s7_pointer s7i_is_c_pointer_symbol(s7_scheme *sc);
s7_pointer s7i_is_openlet_symbol(s7_scheme *sc);
s7_pointer s7i_is_funclet_symbol(s7_scheme *sc);

/* bridge functions for g_c_pointer_info and g_c_pointer_type migration */
s7_pointer s7i_c_pointer_info_p_p(s7_scheme *sc, s7_pointer cptr);
s7_pointer s7i_c_pointer_type_p_p(s7_scheme *sc, s7_pointer cptr);

/* bridge functions for g_tree_is_cyclic and g_type_of migration */
bool s7i_tree_is_cyclic(s7_scheme *sc, s7_pointer p);
s7_pointer s7i_type_of(s7_scheme *sc, s7_pointer p);

/* bridge functions for g_c_pointer_weak1, g_c_pointer_weak2 migration */
s7_pointer s7i_c_pointer_weak1_p_p(s7_scheme *sc, s7_pointer cptr);
s7_pointer s7i_c_pointer_weak2_p_p(s7_scheme *sc, s7_pointer cptr);

/* bridge functions for g_tree_leaves migration */
s7_pointer s7i_tree_leaves_p_p(s7_scheme *sc, s7_pointer p);

/* bridge function for g_outlet migration */
s7_pointer s7i_outlet_p_p(s7_scheme *sc, s7_pointer let);

/* bridge function for g_quotient migration */
s7_pointer s7i_quotient_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

/* bridge function for g_remainder migration */
s7_pointer s7i_remainder_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

/* bridge function for g_modulo migration */
s7_pointer s7i_modulo_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

/* bridge function for g_curlet_ref migration */
s7_pointer s7i_lookup_p_p(s7_scheme *sc, s7_pointer symbol);

/* bridge functions for g_cyclic_sequences migration */
s7_pointer s7i_cyclic_sequences_p_p(s7_scheme *sc, s7_pointer p);

/* bridge functions for g_object_to_let migration */
s7_pointer s7i_object_to_let_p_p(s7_scheme *sc, s7_pointer p);

/* bridge functions for g_pair_line_number migration */
s7_pointer s7i_pair_line_number_p_p(s7_scheme *sc, s7_pointer p);

/* bridge functions for g_reverse migration */
s7_pointer s7i_reverse_p_p(s7_scheme *sc, s7_pointer p);

/* bridge functions for g_port_line_number migration */
s7_pointer s7i_port_line_number_p_p(s7_scheme *sc, s7_pointer p);

/* bridge functions for g_tree_memq migration */
bool s7i_tree_memq_b_7pp(s7_scheme *sc, s7_pointer sym, s7_pointer tree);

/* bridge functions for g_tree_set_memq migration */
bool s7i_tree_set_memq_b_7pp(s7_scheme *sc, s7_pointer syms, s7_pointer tree);

/* bridge functions for g_unlet_disabled migration */
s7_pointer s7i_unlet_disabled(s7_scheme *sc);

/* bridge functions for g_curlet migration */
s7_pointer s7i_curlet(s7_scheme *sc);
void s7i_capture_let_counter_inc(s7_scheme *sc);

/* write-related helpers */
typedef enum {S7I_P_DISPLAY, S7I_P_WRITE, S7I_P_READABLE, S7I_P_KEY, S7I_P_CODE} s7i_use_write_t;

bool s7i_port_is_closed(s7_pointer p);
s7_pointer s7i_object_out(s7_scheme *sc, s7_pointer obj, s7_pointer port, s7i_use_write_t choice);
void s7i_port_write_string(s7_scheme *sc, const char *str, s7_int len, s7_pointer port);
void s7i_port_write_unicode_char(s7_scheme *sc, uint32_t c, s7_pointer port);
s7_pointer s7i_start_and_end(s7_scheme *sc, s7_pointer caller, s7_pointer args, int32_t position, s7_pointer index_args, s7_int *start, s7_int *end);
bool s7i_is_unused(s7_scheme *sc, s7_pointer p);
s7_pointer s7i_method_or_bust_p(s7_scheme *sc, s7_pointer obj, const char *method_name, const char *type_name);
s7_pointer s7i_method_or_bust_pp(s7_scheme *sc, s7_pointer obj, const char *method_name, s7_pointer x1, s7_pointer x2, const char *type_name, s7_int arg_pos);

void s7i_division_by_zero_error(s7_scheme *sc, const char *caller, s7_pointer x, s7_pointer y);

/* max/min core comparison functions (use internal macros, must stay in s7.c) */
s7_pointer max_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer min_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

bool s7i_is_subvector(s7_pointer p);
s7_int s7i_subvector_position(s7_pointer p);
s7_pointer s7i_subvector_vector(s7_scheme *sc, s7_pointer p);
bool s7i_is_typed_t_vector(s7_pointer p);
s7_pointer s7i_typed_vector_typer(s7_scheme *sc, s7_pointer p);

s7_pointer s7i_vector_ref_1(s7_scheme *sc, s7_pointer vect, s7_pointer indices);
s7_pointer s7i_vector_ref_p_pp(s7_scheme *sc, s7_pointer vec, s7_pointer ind);

s7_pointer s7i_make_vector_1(s7_scheme *sc, s7_pointer args, s7_pointer caller);
s7_pointer s7i_make_simple_complex_vector(s7_scheme *sc, s7_int len);
s7_complex s7i_to_c_complex(s7_pointer z);
s7_pointer s7i_vector_fill_1(s7_scheme *sc, s7_pointer caller, s7_pointer args);
s7_pointer s7i_vector_append(s7_scheme *sc, s7_pointer args, uint8_t typ, s7_pointer caller);

/* module system helpers */
bool s7i_is_closure(s7_pointer p);
bool s7i_is_closure_star(s7_pointer p);
s7_pointer s7i_missing_key_value(s7_scheme *sc);
const char *s7i_find_autoload_name(s7_scheme *sc, s7_pointer symbol, bool *already_loaded, bool loading);

/* hash-table helpers */
s7_int s7i_hash_table_entries(s7_pointer table);
s7_pointer s7i_hash_table_key_typer(s7_scheme *sc, s7_pointer table);
s7_pointer s7i_hash_table_value_typer(s7_scheme *sc, s7_pointer table);

s7_pointer s7i_ref_index_checked(s7_scheme *sc, s7_pointer caller, s7_pointer in_obj, s7_pointer args);
s7_pointer s7i_hash_table_1(s7_scheme *sc, s7_pointer args, s7_pointer caller);
s7_pointer s7i_make_hash_table_1(s7_scheme *sc, s7_pointer args, s7_pointer caller);
s7_pointer s7i_hash_table_add(s7_scheme *sc, s7_pointer table, s7_pointer key, s7_pointer value);
bool s7i_is_weak_hash_table(s7_pointer p);
void s7i_set_weak_hash_table(s7_pointer p);
void s7i_set_weak_hash_table_iters(s7_pointer p, s7_int val);

s7_double s7i_default_rationalize_error(s7_scheme *sc);

/* bridge for g_numerator/g_denominator migration */
s7_pointer s7i_int_one(s7_scheme *sc);

/* bridge for g_iterator_sequence migration */
s7_pointer s7i_iterator_sequence(s7_pointer iter);

/* symbol helpers */
bool s7i_is_gensym(s7_pointer p);
s7_pointer s7i_symbol_name_cell(s7_pointer sym);
s7_int s7i_symbol_name_length(s7_pointer sym);
s7_pointer s7i_make_symbol_with_length(s7_scheme *sc, const char *name, s7_int len);
s7_pointer s7i_initial_value(s7_pointer symbol);
void s7i_set_initial_value(s7_pointer symbol, s7_pointer value);
bool s7i_initial_value_is_defined(s7_scheme *sc, s7_pointer symbol);

/* bridge functions for g_memv, g_assq, g_assv migration */
s7_pointer s7i_memv_p_pp(s7_scheme *sc, s7_pointer a, s7_pointer b);
s7_pointer s7i_assq_p_pp(s7_scheme *sc, s7_pointer a, s7_pointer b);
s7_pointer s7i_assv_p_pp(s7_scheme *sc, s7_pointer a, s7_pointer b);

/* bridge functions for g_memq_2, g_memq_4 migration */
s7_pointer s7i_memq_2_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst);
s7_pointer s7i_memq_4_p_pp(s7_scheme *sc, s7_pointer obj, s7_pointer lst);

/* bridge function for g_cv_ref_2 migration */
s7_pointer s7i_complex_vector_ref_p_pp(s7_scheme *sc, s7_pointer vec, s7_pointer index);

/* bridge function for g_cv_set_3 migration */
s7_pointer s7i_complex_vector_set_p_ppp(s7_scheme *sc, s7_pointer vec, s7_pointer index, s7_pointer value);

/* bridge functions for g_fv_ref_2, g_iv_ref_2 migration */
s7_pointer s7i_float_vector_ref_p_pp(s7_scheme *sc, s7_pointer vec, s7_pointer index);
s7_pointer s7i_int_vector_ref_p_pp(s7_scheme *sc, s7_pointer vec, s7_pointer index);

/* bridge functions for g_tree_set_memq_syms migration */
s7_pointer s7i_tree_set_memq_syms_direct(s7_scheme *sc, s7_pointer a, s7_pointer b);

/* bridge functions for g_heap_analyze migration */
void s7i_heap_analyze(s7_scheme *sc);

/* bridge functions for g_show_op_stack migration */
void s7i_show_op_stack(s7_scheme *sc);

/* bridge functions for g_is_op_stack migration */
bool s7i_is_op_stack_active(s7_scheme *sc);

/* bridge functions for g_heap_holder migration */
s7_pointer s7i_heap_holder_p_p(s7_scheme *sc, s7_pointer obj);

/* bridge functions for g_heap_holders migration */
s7_int s7i_heap_holders(s7_pointer obj);

/* bridge functions for g_is_defined_in_rootlet migration */
bool s7i_is_defined_in_rootlet(s7_scheme *sc, s7_pointer sym);


/* bridge functions for g_leq_2/g_geq_2 migration */
bool s7i_leq_b_7pp(s7_scheme *sc, s7_pointer x, s7_pointer y);
bool s7i_geq_b_7pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

/* bridge function for g_num_eq_2 migration */
bool s7i_num_eq_b_7pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

/* bridge function for g_less_2 migration */
s7_pointer s7i_lt_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

/* bridge function for g_num_eq_xi/g_num_eq_ix migration */
s7_pointer s7i_num_eq_xx(s7_scheme *sc, s7_pointer x, s7_pointer y);

/* bridge functions for arithmetic g_ functions migration */
s7_pointer s7i_add_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer s7i_add_p_pp_wrapped(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer s7i_add_p_ppp(s7_scheme *sc, s7_pointer x, s7_pointer y, s7_pointer z);
s7_pointer s7i_add_p_ppp_wrapped(s7_scheme *sc, s7_pointer x, s7_pointer y, s7_pointer z);
s7_pointer s7i_negate_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer s7i_negate_p_p_wrapped(s7_scheme *sc, s7_pointer x);
s7_pointer s7i_subtract_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer s7i_subtract_p_pp_wrapped(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer s7i_multiply_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer s7i_multiply_p_pp_wrapped(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer s7i_multiply_p_ppp(s7_scheme *sc, s7_pointer x, s7_pointer y, s7_pointer z);
s7_pointer s7i_multiply_p_ppp_wrapped(s7_scheme *sc, s7_pointer x, s7_pointer y, s7_pointer z);
s7_pointer s7i_invert_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer s7i_divide_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);

#ifdef __cplusplus
}
#endif


/* bridge functions for format module */
s7_int s7i_format_column(s7_scheme *sc);
void s7i_set_format_column(s7_scheme *sc, s7_int val);
void s7i_inc_format_column(s7_scheme *sc);
void s7i_add_format_column(s7_scheme *sc, s7_int n);
s7_int s7i_format_depth(s7_scheme *sc);
void s7i_set_format_depth(s7_scheme *sc, s7_int val);
void s7i_inc_format_depth(s7_scheme *sc);
void s7i_dec_format_depth(s7_scheme *sc);
int32_t s7i_num_fdats(s7_scheme *sc);
void s7i_set_num_fdats(s7_scheme *sc, int32_t val);
format_data_t **s7i_fdats(s7_scheme *sc);
void s7i_set_fdats(s7_scheme *sc, format_data_t **val);
s7_int s7i_print_length(s7_scheme *sc);
s7_int s7i_max_string_length(s7_scheme *sc);
char *s7i_strbuf(s7_scheme *sc);
s7_int s7i_strbuf_size(s7_scheme *sc);
bool s7i_has_openlets(s7_scheme *sc);
void s7i_set_has_openlets(s7_scheme *sc, bool val);
s7_pointer s7i_format_symbol(s7_scheme *sc);
s7_pointer s7i_format_error_symbol(s7_scheme *sc);
s7_pointer s7i_format_just_control_string(s7_scheme *sc);
s7_pointer s7i_format_as_objstr(s7_scheme *sc);
s7_pointer s7i_format_no_column(s7_scheme *sc);
s7_pointer s7i_format_f(s7_scheme *sc);
s7_pointer s7i_is_null_symbol(s7_scheme *sc);
s7_pointer s7i_is_string_symbol(s7_scheme *sc);
s7_pointer s7i_is_output_port_symbol(s7_scheme *sc);
s7_pointer s7i_is_boolean_symbol(s7_scheme *sc);
const char *s7i_type_name_string(s7_scheme *sc);
s7_pointer s7i_F(s7_scheme *sc);
s7_pointer s7i_T(s7_scheme *sc);
s7_pointer s7i_nil(s7_scheme *sc);
s7_pointer s7i_undefined(s7_scheme *sc);

no_return void s7i_error_nr(s7_scheme *sc, s7_pointer sym, s7_pointer list);
s7_pointer s7i_set_elist_2(s7_scheme *sc, s7_pointer a, s7_pointer b);
s7_pointer s7i_set_elist_3(s7_scheme *sc, s7_pointer a, s7_pointer b, s7_pointer c);
s7_pointer s7i_set_elist_4(s7_scheme *sc, s7_pointer a, s7_pointer b, s7_pointer c, s7_pointer d);
s7_pointer s7i_set_elist_5(s7_scheme *sc, s7_pointer a, s7_pointer b, s7_pointer c, s7_pointer d, s7_pointer e);
s7_pointer s7i_set_plist_3(s7_scheme *sc, s7_pointer a, s7_pointer b, s7_pointer c);
s7_pointer s7i_wrap_string(s7_scheme *sc, const char *str, s7_int len);
s7_pointer s7i_wrap_integer(s7_scheme *sc, s7_int n);
char *s7i_number_to_string_base_10(s7_scheme *sc, s7_pointer num, s7_int width, s7_int precision, char float_choice, s7_int *nlen, s7i_use_write_t choice);
block_t *s7i_number_to_string_with_radix(s7_scheme *sc, s7_pointer num, int32_t radix, s7_int width, s7_int precision, char float_choice, s7_int *nlen);
const char *s7i_integer_to_string(s7_scheme *sc, s7_int num, s7_int *nlen);
block_t *s7i_mallocate(s7_scheme *sc, s7_int size);
void s7i_liberate(s7_scheme *sc, block_t *b);
block_t *s7i_inline_mallocate(s7_scheme *sc, s7_int size);
s7_pointer s7i_inline_block_to_string(s7_scheme *sc, block_t *block, s7_int len);
s7_pointer s7i_make_string_with_length(s7_scheme *sc, const char *str, s7_int len);
s7_pointer s7i_object_to_list(s7_scheme *sc, s7_pointer obj);
void s7i_resize_port_data(s7_scheme *sc, s7_pointer port, s7_int len);
void s7i_close_format_port(s7_scheme *sc, s7_pointer port);
s7_pointer s7i_open_format_port(s7_scheme *sc);
s7_int s7i_integer_clamped_if_gmp(s7_scheme *sc, s7_pointer p);
bool s7i_is_one(s7_pointer x);
bool s7i_is_one_or_big_one(s7_scheme *sc, s7_pointer x);
s7_pointer s7i_find_method_with_let(s7_scheme *sc, s7_pointer obj, s7_pointer method);
s7_pointer s7i_current_output_port(s7_scheme *sc);
const char *s7i_string_value(s7_pointer str);
s7_int s7i_string_length(s7_pointer str);
void s7i_set_string_length(s7_pointer str, s7_int len);

s7_int s7i_port_position(s7_pointer port);
void s7i_set_port_position(s7_pointer port, s7_int val);
s7_int s7i_port_data_size(s7_pointer port);
void s7i_set_port_data_size(s7_pointer port, s7_int val);
uint8_t *s7i_port_data(s7_pointer port);
void s7i_set_port_data(s7_pointer port, uint8_t *val);
block_t *s7i_port_data_block(s7_pointer port);
void s7i_set_port_data_block(s7_pointer port, block_t *val);
bool s7i_is_string_port(s7_pointer port);
bool s7i_port_is_closed(s7_pointer port);
void s7i_port_write_character(s7_scheme *sc, char c, s7_pointer port);
void s7i_port_write_string(s7_scheme *sc, const char *str, s7_int len, s7_pointer port);

s7_pointer s7i_format_string_1(s7_scheme *sc);
s7_pointer s7i_format_string_2(s7_scheme *sc);
s7_pointer s7i_format_string_3(s7_scheme *sc);
s7_pointer s7i_format_string_4(s7_scheme *sc);
s7_pointer s7i_an_output_port_string(void);
s7_pointer s7i_a_format_port_string(void);
s7_int s7i_FORMAT_PORT_LENGTH(void);
const int32_t *s7i_digits(void);
const bool *s7i_white_space(void);
bool s7i_digitp(int32_t c);
s7_int s7i_safe_strlen(const char *str);
void *s7i_block_data(block_t *b);
bool s7i_is_columnizing(const char *str);
bool s7i_is_elist(s7_scheme *sc, s7_pointer p);
int32_t s7i_type(s7_pointer p);
int32_t s7i_T_INTEGER(void);
int32_t s7i_T_RATIO(void);
int32_t s7i_T_STRING(void);

#endif /* S7_INTERNAL_HELPERS_H */
