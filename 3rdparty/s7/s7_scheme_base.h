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

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_BASE_H */