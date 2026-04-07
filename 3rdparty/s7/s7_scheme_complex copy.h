/* s7_scheme_complex.h - complex number declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_COMPLEX_H
#define S7_SCHEME_COMPLEX_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer magnitude_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_magnitude(s7_scheme *sc, s7_pointer args);
s7_int magnitude_i_i(s7_int x);
s7_double magnitude_d_d(s7_double x);
s7_pointer magnitude_p_z(s7_scheme *sc, s7_pointer z);

s7_pointer g_angle(s7_scheme *sc, s7_pointer args);
s7_double angle_d_d(s7_double x);

s7_pointer complex_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer complex_p_pp_wrapped(s7_scheme *sc, s7_pointer x, s7_pointer y);
s7_pointer g_complex(s7_scheme *sc, s7_pointer args);
s7_pointer g_complex_wrapped(s7_scheme *sc, s7_pointer args);
s7_pointer complex_p_ii_wrapped(s7_scheme *sc, s7_int x, s7_int y);
s7_pointer complex_p_dd_wrapped(s7_scheme *sc, s7_double x, s7_double y);
s7_pointer complex_p_ii(s7_scheme *sc, s7_int x, s7_int y);
s7_pointer complex_p_dd(s7_scheme *sc, s7_double x, s7_double y);

s7_pointer g_make_polar(s7_scheme *sc, s7_pointer args);

s7_double real_part_d_7p(s7_scheme *sc, s7_pointer x);
s7_pointer real_part_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_real_part(s7_scheme *sc, s7_pointer args);
s7_double imag_part_d_7p(s7_scheme *sc, s7_pointer x);
s7_pointer imag_part_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer g_imag_part(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_COMPLEX_H */
