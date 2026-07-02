/* s7_liii_bitwise.h - bitwise declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_LIII_BITWISE_H
#define S7_LIII_BITWISE_H

#include "s7.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_logior(s7_scheme *sc, s7_pointer args);
s7_int logior_i_ii(s7_int i1, s7_int i2);
s7_int logior_i_iii(s7_int i1, s7_int i2, s7_int i3);
s7_pointer g_logior_ii(s7_scheme *sc, s7_pointer args);
s7_pointer g_logior_2(s7_scheme *sc, s7_pointer args);
s7_pointer logior_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr);

s7_pointer g_logxor(s7_scheme *sc, s7_pointer args);
s7_int logxor_i_ii(s7_int i1, s7_int i2);
s7_int logxor_i_iii(s7_int i1, s7_int i2, s7_int i3);
s7_pointer g_logxor_2(s7_scheme *sc, s7_pointer args);
s7_pointer logxor_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr);

s7_pointer g_logand(s7_scheme *sc, s7_pointer args);
s7_int logand_i_ii(s7_int i1, s7_int i2);
s7_int logand_i_iii(s7_int i1, s7_int i2, s7_int i3);
s7_pointer g_logand_ii(s7_scheme *sc, s7_pointer args);
s7_pointer g_logand_2(s7_scheme *sc, s7_pointer args);
s7_pointer logand_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr);

s7_pointer g_lognot(s7_scheme *sc, s7_pointer args);
s7_int lognot_i_i(s7_int i1);

s7_pointer g_logbit(s7_scheme *sc, s7_pointer args);
bool logbit_b_7ii(s7_scheme *sc, s7_int i1, s7_int i2);
bool logbit_b_7pp(s7_scheme *sc, s7_pointer i1, s7_pointer i2);

s7_pointer g_ash(s7_scheme *sc, s7_pointer args);
s7_int ash_i_7ii(s7_scheme *sc, s7_int i1, s7_int i2);
s7_pointer g_ash_ii(s7_scheme *sc, s7_pointer args);
s7_pointer g_ash_ic(s7_scheme *sc, s7_pointer args);
s7_pointer ash_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr);

s7_int lsh_i_ii_unchecked(s7_int i1, s7_int i2);
s7_int rsh_i_ii_unchecked(s7_int i1, s7_int i2);
s7_int rsh_i_i2_direct(s7_int i1, s7_int unused_i2);

#ifdef __cplusplus
}
#endif

#endif /* S7_LIII_BITWISE_H */
