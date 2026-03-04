/* s7_scheme_char.h - character function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_CHAR_H
#define S7_SCHEME_CHAR_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_scheme_char_tables(void);

s7_pointer g_char_to_integer(s7_scheme *sc, s7_pointer args);
s7_int char_to_integer_i_7p(s7_scheme *sc, s7_pointer c);
s7_pointer char_to_integer_p_p(s7_scheme *sc, s7_pointer c);

s7_pointer g_integer_to_char(s7_scheme *sc, s7_pointer args);
s7_pointer integer_to_char_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer integer_to_char_p_i(s7_scheme *sc, s7_int ind);

s7_pointer char_upcase_p_p(s7_scheme *sc, s7_pointer c);
s7_pointer char_upcase_p_p_unchecked(s7_scheme *sc, s7_pointer c);
s7_pointer g_char_upcase(s7_scheme *sc, s7_pointer args);
s7_pointer g_char_downcase(s7_scheme *sc, s7_pointer args);

s7_pointer g_is_char_alphabetic(s7_scheme *sc, s7_pointer args);
bool is_char_alphabetic_b_7p(s7_scheme *sc, s7_pointer c);
s7_pointer is_char_alphabetic_p_p(s7_scheme *sc, s7_pointer c);

s7_pointer g_is_char_numeric(s7_scheme *sc, s7_pointer args);
bool is_char_numeric_b_7p(s7_scheme *sc, s7_pointer c);
s7_pointer is_char_numeric_p_p(s7_scheme *sc, s7_pointer c);

s7_pointer g_is_char_whitespace(s7_scheme *sc, s7_pointer args);
bool is_char_whitespace_b_7p(s7_scheme *sc, s7_pointer c);
s7_pointer is_char_whitespace_p_p(s7_scheme *sc, s7_pointer c);
s7_pointer is_char_whitespace_p_p_unchecked(s7_scheme *sc, s7_pointer c);

s7_pointer g_is_char_upper_case(s7_scheme *sc, s7_pointer args);
bool is_char_upper_case_b_7p(s7_scheme *sc, s7_pointer c);
s7_pointer g_is_char_lower_case(s7_scheme *sc, s7_pointer args);
bool is_char_lower_case_b_7p(s7_scheme *sc, s7_pointer c);

s7_pointer g_is_char(s7_scheme *sc, s7_pointer args);
s7_pointer is_char_p_p(s7_scheme *sc, s7_pointer p);

s7_pointer g_chars_are_equal(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_less(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_greater(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_geq(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_leq(s7_scheme *sc, s7_pointer args);

s7_pointer g_simple_char_eq(s7_scheme *sc, s7_pointer args);
s7_pointer g_simple_char_eq1(s7_scheme *sc, s7_pointer args);
s7_pointer g_simple_char_eq2(s7_scheme *sc, s7_pointer args);

bool char_lt_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_lt_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_leq_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_leq_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_gt_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_gt_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_geq_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_geq_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_eq_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_eq_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
s7_pointer char_eq_p_pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
s7_pointer g_char_equal_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_char_less_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_char_greater_2(s7_scheme *sc, s7_pointer args);

#if !WITH_PURE_S7
s7_pointer g_chars_are_ci_equal(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_ci_less(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_ci_greater(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_ci_geq(s7_scheme *sc, s7_pointer args);
s7_pointer g_chars_are_ci_leq(s7_scheme *sc, s7_pointer args);

bool char_ci_lt_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_ci_lt_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_ci_leq_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_ci_leq_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_ci_gt_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_ci_gt_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_ci_geq_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_ci_geq_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
bool char_ci_eq_b_unchecked(s7_pointer c1, s7_pointer c2);
bool char_ci_eq_b_7pp(s7_scheme *sc, s7_pointer c1, s7_pointer c2);
#endif

s7_pointer g_char_position(s7_scheme *sc, s7_pointer args);
s7_pointer char_position_p_ppi(s7_scheme *sc, s7_pointer chr, s7_pointer str, s7_int start);
s7_pointer g_char_position_csi(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_CHAR_H */
