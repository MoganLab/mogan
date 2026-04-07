/* s7_scheme_inexact.h - inexact number declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_INEXACT_H
#define S7_SCHEME_INEXACT_H

#include "s7.h"
#include "s7_scheme_base.h"

#ifdef __cplusplus
extern "C" {
#endif


/* inexact number function declarations */
s7_pointer sqrt_p_p(s7_scheme *sc, s7_pointer num);
s7_pointer g_sqrt(s7_scheme *sc, s7_pointer args);

/* sin function */
s7_pointer sin_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_sin(s7_scheme *sc, s7_pointer args);
s7_pointer sin_p_d(s7_scheme *sc, s7_double x);
s7_double sin_d_d(s7_double x);

/* cos function */
s7_pointer cos_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_cos(s7_scheme *sc, s7_pointer args);
s7_pointer cos_p_d(s7_scheme *sc, s7_double x);
s7_double cos_d_d(s7_double x);

/* tan function */
s7_pointer tan_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_tan(s7_scheme *sc, s7_pointer args);
s7_double tan_d_d(s7_double x);

/* exp function */
s7_pointer exp_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_exp(s7_scheme *sc, s7_pointer args);
s7_pointer exp_p_d(s7_scheme *sc, s7_double x);
s7_double exp_d_d(s7_double x);

/* log function */
s7_pointer g_log(s7_scheme *sc, s7_pointer args);

/* asin function */
s7_pointer asin_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_asin(s7_scheme *sc, s7_pointer args);
s7_pointer asin_p_d(s7_scheme *sc, s7_double x);
s7_double asin_d_d(s7_double x);

/* acos function */
s7_pointer acos_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_acos(s7_scheme *sc, s7_pointer args);
s7_pointer acos_p_d(s7_scheme *sc, s7_double x);
s7_double acos_d_d(s7_double x);

/* atan function */
s7_pointer atan_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_atan(s7_scheme *sc, s7_pointer args);
s7_pointer atan_p_d(s7_scheme *sc, s7_double x);
s7_double atan_d_d(s7_double x);
s7_double atan_d_dd(s7_double x, s7_double y);

/* infinite? function */
bool s7_is_infinite(s7_scheme *sc, s7_pointer x);
s7_pointer g_is_infinite(s7_scheme *sc, s7_pointer args);

/* nan? function */
bool s7_is_nan(s7_scheme *sc, s7_pointer x);
s7_pointer g_is_nan(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_INEXACT_H */
