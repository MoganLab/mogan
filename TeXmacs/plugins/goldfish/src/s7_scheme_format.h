/* s7_scheme_format.h - format function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_FORMAT_H
#define S7_SCHEME_FORMAT_H

#include "s7.h"
#include "s7_internal_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* format chooser for optimizer */
s7_pointer format_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr);

/* format entry functions */
s7_pointer g_format(s7_scheme *sc, s7_pointer args);
s7_pointer g_format_f(s7_scheme *sc, s7_pointer args);
s7_pointer g_format_just_control_string(s7_scheme *sc, s7_pointer args);
s7_pointer g_format_as_objstr(s7_scheme *sc, s7_pointer args);
s7_pointer g_format_no_column(s7_scheme *sc, s7_pointer args);

/* internal functions used by s7.c */
s7_pointer format_to_port_1(s7_scheme *sc, s7_pointer port, const char *str, s7_pointer args, s7_pointer *next_arg, bool with_result, bool columnized, s7_int len, s7_pointer orig_str);
format_data_t *make_fdat(s7_scheme *sc);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_FORMAT_H */
