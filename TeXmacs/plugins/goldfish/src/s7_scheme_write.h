/* s7_scheme_write.h - write function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#ifndef S7_SCHEME_WRITE_H
#define S7_SCHEME_WRITE_H

#include "s7.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* cycles / shared-info: circular reference detection for the printer */
struct shared_info;
int32_t shared_ref(struct shared_info *ci, s7_pointer p);
void flip_ref(struct shared_info *ci, s7_pointer p);
int32_t peek_shared_ref_1(struct shared_info *ci, s7_pointer p);
int32_t peek_shared_ref(struct shared_info *ci, s7_pointer p);
void enlarge_shared_info(struct shared_info *ci);
struct shared_info *make_shared_info(s7_scheme *sc);
void free_shared_info(struct shared_info *ci);
struct shared_info *clear_shared_info(struct shared_info *ci);
struct shared_info *load_shared_info(s7_scheme *sc, s7_pointer top, bool stop_at_print_length, struct shared_info *ci);
bool collect_shared_info(s7_scheme *sc, struct shared_info *ci, s7_pointer top, bool stop_at_print_length);
s7_pointer cyclic_sequences_p_p(s7_scheme *sc, s7_pointer obj);

/* object->port printer: entries used by s7.c */
int32_t circular_list_entries(s7_pointer lst);
s7_pointer find_closure(s7_scheme *sc, s7_pointer closure, s7_pointer current_let);
s7_pointer find_typer(s7_scheme *sc, s7_pointer typer);
const char *hash_table_typer_name(s7_scheme *sc, s7_pointer typer);
s7_pointer closure_name(s7_scheme *sc, s7_pointer closure);
void init_display_functions(void);

/* Public API implementations */
void s7_newline(s7_scheme *sc, s7_pointer port);
s7_pointer s7_write(s7_scheme *sc, s7_pointer obj, s7_pointer port);
s7_pointer s7_display(s7_scheme *sc, s7_pointer obj, s7_pointer port);
s7_pointer s7_write_char(s7_scheme *sc, s7_pointer c, s7_pointer port);

/* Scheme accessible functions */
s7_pointer g_newline(s7_scheme *sc, s7_pointer args);
s7_pointer g_write(s7_scheme *sc, s7_pointer args);
s7_pointer g_write_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_display(s7_scheme *sc, s7_pointer args);
s7_pointer g_display_2(s7_scheme *sc, s7_pointer args);
s7_pointer g_display_f(s7_scheme *sc, s7_pointer args);
s7_pointer g_write_char(s7_scheme *sc, s7_pointer args);
s7_pointer g_write_string(s7_scheme *sc, s7_pointer args);
s7_pointer g_write_byte(s7_scheme *sc, s7_pointer args);

/* Port accessor functions */
s7_pointer g_current_input_port(s7_scheme *sc, s7_pointer unused_args);
s7_pointer g_current_output_port(s7_scheme *sc, s7_pointer unused_args);
s7_pointer g_current_error_port(s7_scheme *sc, s7_pointer unused_args);
s7_pointer g_open_output_string(s7_scheme *sc, s7_pointer unused_args);

/* Optimizer helpers */
s7_pointer newline_p(s7_scheme *sc);
s7_pointer newline_p_p(s7_scheme *sc, s7_pointer port);
s7_pointer write_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer write_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer port);
s7_pointer display_p_p(s7_scheme *sc, s7_pointer x);
s7_pointer display_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer port);
s7_pointer write_char_p_p(s7_scheme *sc, s7_pointer c);
s7_pointer write_char_p_pp(s7_scheme *sc, s7_pointer c, s7_pointer port);
s7_pointer write_string_p_pp(s7_scheme *sc, s7_pointer str, s7_pointer port);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_WRITE_H */
