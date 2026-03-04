/* s7_liii_string.h - string utility declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_LIII_STRING_H
#define S7_LIII_STRING_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_string_upcase(s7_scheme *sc, s7_pointer args);
s7_pointer g_string_downcase(s7_scheme *sc, s7_pointer args);
s7_pointer g_string_ref(s7_scheme *sc, s7_pointer args);
s7_pointer string_ref_1(s7_scheme *sc, s7_pointer strng, s7_pointer index);
s7_pointer g_string_set(s7_scheme *sc, s7_pointer args);
s7_pointer g_string_length(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_LIII_STRING_H */
