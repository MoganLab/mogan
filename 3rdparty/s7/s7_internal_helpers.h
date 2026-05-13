/* s7_internal_helpers.h - internal helper bridge declarations
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_INTERNAL_HELPERS_H
#define S7_INTERNAL_HELPERS_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer s7i_method_or_bust(s7_scheme *sc, s7_pointer obj, const char *method_name,
                              s7_pointer args, const char *type_name, s7_int arg_pos);

bool s7i_method_or_bust_bool(s7_scheme *sc, s7_pointer obj, const char *method_name,
                             s7_pointer args, const char *type_name, s7_int arg_pos);

s7_pointer s7i_sole_arg_method_or_bust(s7_scheme *sc, s7_pointer obj, const char *method_name, s7_pointer args, const char *type_name);

bool s7i_sole_arg_method_or_bust_bool(s7_scheme *sc, s7_pointer obj, const char *method_name, s7_pointer args, const char *type_name);

bool s7i_is_sequence(s7_pointer p);
bool s7i_sequence_is_empty(s7_scheme *sc, s7_pointer seq);
s7_int s7i_sequence_length(s7_scheme *sc, s7_pointer seq);
s7_pointer s7i_find_method_with_let(s7_scheme *sc, s7_pointer obj, s7_pointer method);
bool s7i_has_active_methods(s7_scheme *sc, s7_pointer obj);
void s7i_wrong_type_error_nr(s7_scheme *sc, s7_pointer caller, s7_int arg_num, s7_pointer arg, s7_pointer typ);
s7_pointer s7i_copy_1(s7_scheme *sc, s7_pointer caller, s7_pointer args);
s7_int s7i_position_of(const s7_pointer p, s7_pointer args);
s7_pointer s7i_nil_string(void);

/* write-related helpers */
typedef enum {S7I_P_DISPLAY, S7I_P_WRITE, S7I_P_READABLE, S7I_P_KEY, S7I_P_CODE} s7i_use_write_t;

bool s7i_port_is_closed(s7_pointer p);
s7_pointer s7i_object_out(s7_scheme *sc, s7_pointer obj, s7_pointer port, s7i_use_write_t choice);
void s7i_port_write_character(s7_scheme *sc, uint8_t c, s7_pointer port);
void s7i_port_write_string(s7_scheme *sc, const char *str, s7_int len, s7_pointer port);
void s7i_port_write_unicode_char(s7_scheme *sc, uint32_t c, s7_pointer port);
s7_pointer s7i_start_and_end(s7_scheme *sc, s7_pointer caller, s7_pointer args, int32_t position, s7_pointer index_args, s7_int *start, s7_int *end);
bool s7i_is_unused(s7_scheme *sc, s7_pointer p);
s7_pointer s7i_method_or_bust_p(s7_scheme *sc, s7_pointer obj, const char *method_name, const char *type_name);
s7_pointer s7i_method_or_bust_pp(s7_scheme *sc, s7_pointer obj, const char *method_name, s7_pointer x1, s7_pointer x2, const char *type_name, s7_int arg_pos);

#ifdef __cplusplus
}
#endif

#endif /* S7_INTERNAL_HELPERS_H */
