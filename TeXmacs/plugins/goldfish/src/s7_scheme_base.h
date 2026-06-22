/* s7_scheme_base.h - basic function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_BASE_H
#define S7_SCHEME_BASE_H

#include "s7.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Helper function to check for NaN */
bool is_NaN(s7_double x);

/* floor function */
s7_pointer floor_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_floor(s7_scheme *sc, s7_pointer args);
s7_pointer floor_p_d(s7_scheme *sc, s7_double x);
s7_int floor_i_7d(s7_scheme *sc, s7_double x);
s7_int floor_i_7p(s7_scheme *sc, s7_pointer x);
s7_int floor_i_i(s7_int i);
s7_pointer floor_p_i(s7_scheme *sc, s7_int x);

/* ceiling function */
s7_pointer ceiling_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_ceiling(s7_scheme *sc, s7_pointer args);
s7_pointer ceiling_p_d(s7_scheme *sc, s7_double x);
s7_int ceiling_i_7d(s7_scheme *sc, s7_double x);
s7_int ceiling_i_7p(s7_scheme *sc, s7_pointer x);
s7_int ceiling_i_i(s7_int i);
s7_pointer ceiling_p_i(s7_scheme *sc, s7_int x);

/* abs function */
s7_pointer abs_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_abs(s7_scheme *sc, s7_pointer args);
s7_double abs_d_d(s7_double x);
s7_pointer abs_p_d(s7_scheme *sc, s7_double x);
s7_int abs_i_7d(s7_scheme *sc, s7_double x);
s7_int abs_i_7p(s7_scheme *sc, s7_pointer x);
s7_int abs_i_i(s7_int i);
s7_pointer abs_p_i(s7_scheme *sc, s7_int x);

/* even? function */
bool even_b_7p(s7_scheme *sc, s7_pointer x);
s7_pointer even_p_p(s7_scheme *sc, s7_pointer x);
bool even_i(s7_int i1);
s7_pointer g_even(s7_scheme *sc, s7_pointer args);

/* odd? function */
bool odd_b_7p(s7_scheme *sc, s7_pointer x);
s7_pointer odd_p_p(s7_scheme *sc, s7_pointer x);
bool odd_i(s7_int i1);
s7_pointer g_odd(s7_scheme *sc, s7_pointer args);

/* zero? function */
bool zero_b_7p(s7_scheme *sc, s7_pointer x);
s7_pointer zero_p_p(s7_scheme *sc, s7_pointer x);
bool zero_i(s7_int i);
bool zero_d(s7_double x);
s7_pointer g_zero(s7_scheme *sc, s7_pointer args);

/* positive? function */
bool positive_b_7p(s7_scheme *sc, s7_pointer x);
s7_pointer positive_p_p(s7_scheme *sc, s7_pointer x);
bool positive_i(s7_int i);
bool positive_d(s7_double x);
s7_pointer g_positive(s7_scheme *sc, s7_pointer args);

/* negative? function */
bool negative_b_7p(s7_scheme *sc, s7_pointer x);
s7_pointer negative_p_p(s7_scheme *sc, s7_pointer x);
bool negative_i(s7_int p);
bool negative_d(s7_double p);
s7_pointer g_negative(s7_scheme *sc, s7_pointer args);

/* exact? function */
bool exact_b_7p(s7_scheme *sc, s7_pointer x);
s7_pointer exact_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_exact(s7_scheme *sc, s7_pointer args);

/* inexact? function */
bool inexact_b_7p(s7_scheme *sc, s7_pointer x);
s7_pointer inexact_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_inexact(s7_scheme *sc, s7_pointer args);

/* exact->inexact function */
s7_pointer exact_to_inexact_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_exact_to_inexact(s7_scheme *sc, s7_pointer args);

/* inexact->exact function */
s7_pointer inexact_to_exact_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_inexact_to_exact(s7_scheme *sc, s7_pointer args);

/* string->number helper functions */
s7_int s7_string_to_integer(const char *str, int32_t radix, bool *overflow);
double s7_string_to_double_simple(const char *str, int32_t radix);

/* C string helper functions */
s7_int safe_strlen(const char *str);
char *copy_string_with_length(const char *str, s7_int len);
char *copy_string(const char *str);
bool safe_strcmp(const char *s1, const char *s2);
bool local_strncmp(const char *s1, const char *s2, size_t n);
size_t catstrs(char *dst, size_t len, ...);
size_t catstrs_direct(char *dst, const char *str1, ...);

/* NaN payload helpers */
typedef union {s7_int ix; double fx;} decode_float_t;
double nan_with_payload(s7_int payload);
s7_int nan_payload(double x);

/* read-line function */
s7_pointer g_read_line(s7_scheme *sc, s7_pointer args);

/* max function */
s7_pointer g_max(s7_scheme *sc, s7_pointer args);
s7_pointer g_max_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_max_3(s7_scheme *sc, s7_pointer args);

/* min function */
s7_pointer g_min(s7_scheme *sc, s7_pointer args);
s7_pointer g_min_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_min_3(s7_scheme *sc, s7_pointer args);

/* truncate function */
s7_pointer truncate_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_truncate(s7_scheme *sc, s7_pointer args);
s7_int truncate_i_i(s7_int i);
s7_pointer truncate_p_i(s7_scheme *sc, s7_int x);
s7_int truncate_i_7d(s7_scheme *sc, s7_double x);
s7_pointer truncate_p_d(s7_scheme *sc, s7_double x);

/* round function */
s7_pointer round_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_round(s7_scheme *sc, s7_pointer args);
s7_int round_i_i(s7_int i);
s7_pointer round_p_i(s7_scheme *sc, s7_int x);
s7_int round_i_7d(s7_scheme *sc, s7_double z);
s7_pointer round_p_d(s7_scheme *sc, s7_double x);

/* gcd function */
s7_int c_gcd(s7_int u, s7_int v);
s7_pointer g_gcd(s7_scheme *sc, s7_pointer args);

/* lcm function */
s7_pointer g_lcm(s7_scheme *sc, s7_pointer args);

/* rationalize function */
bool c_rationalize(s7_double ux, s7_double error, s7_int *numer, s7_int *denom);
s7_pointer g_rationalize(s7_scheme *sc, s7_pointer args);
s7_int rationalize_i_i(s7_int x);
s7_pointer rationalize_p_i(s7_scheme *sc, s7_int x);
s7_pointer rationalize_p_d(s7_scheme *sc, s7_double x);

/* quotient function */
s7_pointer g_quotient(s7_scheme *sc, s7_pointer args);

/* remainder function */
s7_pointer g_remainder(s7_scheme *sc, s7_pointer args);

/* modulo function */
s7_pointer g_modulo(s7_scheme *sc, s7_pointer args);

/* Helper functions exported from s7.c */
const char *s7i_an_input_port_string(void);
const char *s7i_a_boolean_string(void);
s7_pointer s7i_input_port_if_not_loading(s7_scheme *sc);
s7_pointer s7i_port_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);
s7_pointer s7i_method_or_bust(s7_scheme *sc, s7_pointer obj, const char *method_name, s7_pointer args, const char *type_name, s7_int arg_pos);

/* number->string function */
s7_pointer g_number_to_string(s7_scheme *sc, s7_pointer args);
s7_pointer number_to_string_p_p(s7_scheme *sc, s7_pointer p);
s7_pointer number_to_string_p_i(s7_scheme *sc, s7_int p);
s7_pointer number_to_string_p_pp(s7_scheme *sc, s7_pointer num, s7_pointer base);

/* numerator function */
s7_pointer g_numerator(s7_scheme *sc, s7_pointer args);

/* denominator function */
s7_pointer g_denominator(s7_scheme *sc, s7_pointer args);

/* reverse function */
s7_pointer g_reverse(s7_scheme *sc, s7_pointer args);

/* assq function */
s7_pointer g_assq(s7_scheme *sc, s7_pointer args);

/* assv function */
s7_pointer g_assv(s7_scheme *sc, s7_pointer args);

/* memv function */
s7_pointer g_memv(s7_scheme *sc, s7_pointer args);

/* list functions */
s7_pointer g_list_0(s7_scheme *sc, s7_pointer args);
s7_pointer g_list_1(s7_scheme *sc, s7_pointer args);
s7_pointer g_list_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_list_3(s7_scheme *sc, s7_pointer args);
s7_pointer g_list_4(s7_scheme *sc, s7_pointer args);
s7_pointer g_append_2(s7_scheme *sc, s7_pointer args);

/* comparison functions */
s7_pointer g_leq_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_geq_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_num_eq_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_less_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_num_eq_xi(s7_scheme *sc, s7_pointer args);
s7_pointer g_num_eq_ix(s7_scheme *sc, s7_pointer args);
s7_pointer g_memq_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_memq_4(s7_scheme *sc, s7_pointer args);

/* arithmetic shortcut functions */
s7_pointer g_add_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_add_2_wrapped(s7_scheme *sc, s7_pointer args);
s7_pointer g_add_3(s7_scheme *sc, s7_pointer args);
s7_pointer g_add_3_wrapped(s7_scheme *sc, s7_pointer args);
s7_pointer g_add_4(s7_scheme *sc, s7_pointer args);
s7_pointer g_subtract_1(s7_scheme *sc, s7_pointer args);
s7_pointer g_subtract_1_wrapped(s7_scheme *sc, s7_pointer args);
s7_pointer g_subtract_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_subtract_2_wrapped(s7_scheme *sc, s7_pointer args);
s7_pointer g_subtract_3(s7_scheme *sc, s7_pointer args);
s7_pointer g_abort(s7_scheme *sc, s7_pointer args);
s7_pointer g_multiply_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_multiply_2_wrapped(s7_scheme *sc, s7_pointer args);
s7_pointer g_multiply_3(s7_scheme *sc, s7_pointer args);
s7_pointer g_multiply_3_wrapped(s7_scheme *sc, s7_pointer args);
s7_pointer g_invert_1(s7_scheme *sc, s7_pointer args);
s7_pointer g_divide_2(s7_scheme *sc, s7_pointer args);

/* unlet functions */
s7_pointer g_unlet_ref(s7_scheme *sc, s7_pointer args);
s7_pointer g_sv_unlet_ref(s7_scheme *sc, s7_pointer args);

/* let functions */
s7_pointer g_rootlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_unlet_disabled(s7_scheme *sc, s7_pointer args);
s7_pointer g_curlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_curlet_ref(s7_scheme *sc, s7_pointer args);
s7_pointer g_outlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_outlet_unlet(s7_scheme *sc, s7_pointer args);

/* error function */
s7_pointer g_error(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_BASE_H */