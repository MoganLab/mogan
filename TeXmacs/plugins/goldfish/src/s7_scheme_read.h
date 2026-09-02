/* s7_scheme_read.h - read function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_READ_H
#define S7_SCHEME_READ_H

#include "s7.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* globals defined in s7.c (chars[EOF] is #<eof>) */
extern s7_pointer *chars;
extern s7_pointer eof_object;

/* port read primitives (referenced by the port_functions_t tables in s7.c) */
int32_t file_read_char(s7_scheme *sc, s7_pointer port);
int32_t function_read_char(s7_scheme *sc, s7_pointer port);
int32_t string_read_char(s7_scheme *sc, s7_pointer port);
int32_t output_read_char(s7_scheme *sc, s7_pointer port);
int32_t closed_port_read_char(s7_scheme *sc, s7_pointer port);

s7_pointer output_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);
s7_pointer closed_port_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);
s7_pointer function_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);
s7_pointer stdin_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);
s7_pointer file_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);
s7_pointer string_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);

#ifdef S7_INTERNAL_H
/* token_t is defined in s7_internal.h (s7.c has its own copy and declares these itself) */
token_t file_read_semicolon(s7_scheme *sc, s7_pointer port);
token_t string_read_semicolon(s7_scheme *sc, s7_pointer port);
const port_functions_t *s7i_input_file_functions(void);
const port_functions_t *s7i_input_string_functions_1(void);
#endif

int32_t file_read_white_space(s7_scheme *sc, s7_pointer port);
int32_t terminated_string_read_white_space(s7_scheme *sc, s7_pointer port);

s7_pointer file_read_name(s7_scheme *sc, s7_pointer port);
s7_pointer file_read_sharp(s7_scheme *sc, s7_pointer port);
s7_pointer string_read_name_no_free(s7_scheme *sc, s7_pointer port);
s7_pointer string_read_sharp(s7_scheme *sc, s7_pointer port);
s7_pointer string_read_name(s7_scheme *sc, s7_pointer port);

/* file port creation */
s7_pointer read_file(s7_scheme *sc, FILE *fp, const char *name, s7_int max_size, const char *caller);

/* Public API implementations */
s7_pointer s7_read(s7_scheme *sc, s7_pointer port);
s7_pointer s7_read_char(s7_scheme *sc, s7_pointer port);
s7_pointer s7_peek_char(s7_scheme *sc, s7_pointer port);

/* Scheme accessible functions */
s7_pointer g_read(s7_scheme *sc, s7_pointer args);
s7_pointer g_read_char(s7_scheme *sc, s7_pointer args);
s7_pointer g_read_char_1(s7_scheme *sc, s7_pointer args);
s7_pointer g_peek_char(s7_scheme *sc, s7_pointer args);
s7_pointer g_read_byte(s7_scheme *sc, s7_pointer args);
s7_pointer g_read_string(s7_scheme *sc, s7_pointer args);

/* Optimizer helpers */
s7_pointer read_char_p_p(s7_scheme *sc, s7_pointer port);
s7_pointer read_char_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer unused_expr);
s7_pointer read_line_p_p(s7_scheme *sc, s7_pointer port);
s7_pointer read_line_p_pp(s7_scheme *sc, s7_pointer port, s7_pointer with_eol);

/* Export helpers */
s7_pointer input_port_if_not_loading(s7_scheme *sc);
s7_pointer s7i_port_read_line(s7_scheme *sc, s7_pointer port, bool with_eol);
s7_pointer s7i_input_port_if_not_loading(s7_scheme *sc);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_READ_H */
