/* s7_liii_list.h - list utility declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_LIII_LIST_H
#define S7_LIII_LIST_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_is_null(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_pair(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_list(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_proper_list(s7_scheme *sc, s7_pointer args);
s7_pointer g_car(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdr(s7_scheme *sc, s7_pointer args);
s7_pointer g_caar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cadr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cddr(s7_scheme *sc, s7_pointer args);
s7_pointer g_caaar(s7_scheme *sc, s7_pointer args);
s7_pointer g_caadr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cadar(s7_scheme *sc, s7_pointer args);
s7_pointer g_caddr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdaar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdddr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdadr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cddar(s7_scheme *sc, s7_pointer args);

s7_pointer g_caaaar(s7_scheme *sc, s7_pointer args);
s7_pointer g_caaadr(s7_scheme *sc, s7_pointer args);
s7_pointer g_caadar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cadaar(s7_scheme *sc, s7_pointer args);
s7_pointer g_caaddr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cadddr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cadadr(s7_scheme *sc, s7_pointer args);
s7_pointer g_caddar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdaaar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdaadr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdadar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cddaar(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdaddr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cddddr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cddadr(s7_scheme *sc, s7_pointer args);
s7_pointer g_cdddar(s7_scheme *sc, s7_pointer args);

s7_pointer g_set_car(s7_scheme *sc, s7_pointer args);
s7_pointer g_set_cdr(s7_scheme *sc, s7_pointer args);

s7_pointer g_list_ref(s7_scheme *sc, s7_pointer args);
s7_pointer g_list_tail(s7_scheme *sc, s7_pointer args);

s7_pointer g_cons(s7_scheme *sc, s7_pointer args);
s7_pointer g_list(s7_scheme *sc, s7_pointer args);
s7_pointer g_filter(s7_scheme *sc, s7_pointer args);
s7_pointer g_take(s7_scheme *sc, s7_pointer args);
s7_pointer g_take_right(s7_scheme *sc, s7_pointer args);
s7_pointer g_drop_right(s7_scheme *sc, s7_pointer args);

s7_pointer g_list_set(s7_scheme *sc, s7_pointer args);
s7_pointer g_list_set_i(s7_scheme *sc, s7_pointer args);
s7_pointer g_list_set_1(s7_scheme *sc, s7_pointer lst, s7_pointer args, int32_t arg_num);

#ifdef __cplusplus
}
#endif

#endif /* S7_LIII_LIST_H */
