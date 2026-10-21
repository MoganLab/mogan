/* s7_stacktrace.h - stack trace declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_STACKTRACE_H
#define S7_STACKTRACE_H

#include <stdbool.h>
#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ST_MAX_FRAMES 30
#define ST_CODE_COLS 45
#define ST_TOTAL_COLS 80
#define ST_NOTES_START_COL 45
#define ST_AS_COMMENT false

#define H_stacktrace "(stacktrace (max-frames 30) (code-cols 45) (total-cols 80) (note-col 45) as-comment) returns \
a stacktrace as a string.  Each line has two portions, the code being evaluated and a note giving \
the value of local variables in that code.  The first argument sets how many lines are displayed. \
The next three arguments set the length and layout of those lines.  'as-comment' if #t causes each \
line to be preceded by a semicolon."
#define Q_stacktrace s7_make_signature(sc, 6, \
                         sc->is_string_symbol, sc->is_integer_symbol, sc->is_integer_symbol, \
                         sc->is_integer_symbol, sc->is_integer_symbol, sc->is_boolean_symbol)

s7_pointer g_stacktrace(s7_scheme *sc, s7_pointer args);
s7_pointer sl_set_stacktrace_defaults(s7_scheme *sc, s7_pointer sym, s7_pointer val);
/* s7_stacktrace is declared in s7.h */
bool s7i_find_error_location(s7_scheme *sc, s7_pointer cur_code, s7_int *ret_line, s7_int *ret_file, s7_int *ret_pos);

#ifdef __cplusplus
}
#endif

#endif /* S7_STACKTRACE_H */
