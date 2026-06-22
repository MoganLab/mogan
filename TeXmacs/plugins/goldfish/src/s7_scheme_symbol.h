/* s7_scheme_symbol.h - symbol and keyword function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_SYMBOL_H
#define S7_SCHEME_SYMBOL_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

/* symbol->string functions */
s7_pointer g_symbol_to_string(s7_scheme *sc, s7_pointer args);
s7_pointer g_symbol_to_string_uncopied(s7_scheme *sc, s7_pointer args);
s7_pointer symbol_to_string_p_p(s7_scheme *sc, s7_pointer sym);
s7_pointer symbol_to_string_uncopied_p(s7_scheme *sc, s7_pointer sym);

/* string->symbol functions */
s7_pointer g_string_to_symbol(s7_scheme *sc, s7_pointer args);
s7_pointer string_to_symbol_p_p(s7_scheme *sc, s7_pointer p);

/* keyword functions */
s7_pointer g_keyword_to_symbol(s7_scheme *sc, s7_pointer args);
s7_pointer g_symbol_to_keyword(s7_scheme *sc, s7_pointer args);
s7_pointer g_string_to_keyword(s7_scheme *sc, s7_pointer args);

/* symbol-initial-value function */
s7_pointer g_symbol_initial_value(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_SYMBOL_H */
