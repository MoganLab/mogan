/* s7_scheme_let.h - let (environment) function declarations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_SCHEME_LET_H
#define S7_SCHEME_LET_H

#include "s7.h"

#ifdef __cplusplus
extern "C" {
#endif

s7_pointer g_unlet(s7_scheme *sc, s7_pointer unused_args);
s7_pointer g_openlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_coverlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_varlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_cutlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_sublet(s7_scheme *sc, s7_pointer args);
s7_pointer g_sublet_curlet(s7_scheme *sc, s7_pointer args);
s7_pointer sublet_chooser(s7_scheme *sc, s7_pointer func, int32_t num_args, s7_pointer expr);
s7_pointer g_simple_inlet(s7_scheme *sc, s7_pointer args);
s7_pointer inlet_p_pp(s7_scheme *sc, s7_pointer symbol, s7_pointer value);
s7_pointer internal_inlet(s7_scheme *sc, s7_int num_args, ...);
s7_pointer inlet_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr);
s7_pointer g_let_to_list(s7_scheme *sc, s7_pointer args);
s7_pointer call_let_ref_fallback(s7_scheme *sc, s7_pointer let, s7_pointer symbol);
s7_pointer call_let_set_fallback(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value);
s7_pointer let_ref(s7_scheme *sc, s7_pointer let, s7_pointer symbol);
s7_pointer g_let_ref(s7_scheme *sc, s7_pointer args);
s7_pointer slot_in_let(s7_scheme *sc, s7_pointer let, const s7_pointer sym);
s7_pointer let_ref_p_pp(s7_scheme *sc, s7_pointer let, s7_pointer sym);
s7_pointer g_cdr_let_ref(s7_scheme *sc, s7_pointer args);
s7_pointer g_starlet_ref(s7_scheme *sc, s7_pointer args);
s7_pointer g_rootlet_ref(s7_scheme *sc, s7_pointer args);
s7_pointer let_ref_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr);
s7_pointer let_set_1(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value);
s7_pointer let_set_2(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value);
s7_pointer g_let_set(s7_scheme *sc, s7_pointer args);
s7_pointer let_set_p_ppp_2(s7_scheme *sc, s7_pointer let, s7_pointer sym, s7_pointer val);
s7_pointer g_cdr_let_set(s7_scheme *sc, s7_pointer args);
s7_pointer g_starlet_set(s7_scheme *sc, s7_pointer args);
s7_pointer g_unlet_set(s7_scheme *sc, s7_pointer args);
s7_pointer let_set_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr);
s7_pointer reverse_slots(s7_pointer let_slots);
s7_pointer let_copy(s7_scheme *sc, s7_pointer let);
void update_symbol_ids(s7_scheme *sc, s7_pointer let);
s7_pointer outlet_p_p(s7_scheme *sc, s7_pointer let);
s7_pointer outlet_chooser(s7_scheme *sc, s7_pointer func, int32_t num_args, s7_pointer expr);
s7_pointer g_set_outlet(s7_scheme *sc, s7_pointer args);
s7_pointer g_symbol_to_value(s7_scheme *sc, s7_pointer args);
s7_pointer symbol_to_value_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr);
s7_pointer g_symbol_to_dynamic_value(s7_scheme *sc, s7_pointer args);
s7_pointer g_is_defined(s7_scheme *sc, s7_pointer args);
s7_pointer g_funclet(s7_scheme *sc, s7_pointer args);
s7_pointer g_owlet(s7_scheme *sc, s7_pointer args);

#ifdef __cplusplus
}
#endif

#endif /* S7_SCHEME_LET_H */
