/* s7_continuation.h - continuation and goto interface for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#ifndef S7_CONTINUATION_H
#define S7_CONTINUATION_H

#include <stdbool.h>
#include "s7.h"

/* -------------------------------- continuation and goto macros -------------------------------- */

#define continuation_block(p)          (T_Con(p))->object.cwcc.block
#define continuation_stack(p)          T_Stk(T_Con(p)->object.cwcc.stack)
#define continuation_set_stack(p, Val) (T_Con(p))->object.cwcc.stack = T_Stk(Val)
#define continuation_stack_end(p)      (T_Con(p))->object.cwcc.stack_end
#define continuation_stack_start(p)    (T_Con(p))->object.cwcc.stack_start
#define continuation_stack_top(p)      (continuation_stack_end(p) - continuation_stack_start(p))
#define continuation_op_stack(p)       (T_Con(p))->object.cwcc.op_stack
#define continuation_stack_size(p)     continuation_block(p)->nx.ix.i1
#define continuation_op_loc(p)         continuation_block(p)->nx.ix.i2
#define continuation_op_size(p)        continuation_block(p)->ln.iter_or_size
#define continuation_key(p)            continuation_block(p)->ex.ckey
/* this can overflow int32_t -- baffle_key is s7_int, so ckey should be also */
#define continuation_name(p)           continuation_block(p)->dx.d_ptr

#define call_exit_goto_loc(p)          (T_Got(p))->object.rexit.goto_loc
#define call_exit_op_loc(p)            (T_Got(p))->object.rexit.op_stack_loc
#define call_exit_active(p)            (T_Got(p))->object.rexit.active
#define call_exit_name(p)              (T_Got(p))->object.rexit.name

#define is_continuation(p)             (type(p) == T_CONTINUATION)
#define is_goto(p)                     (type(p) == T_GOTO)

/* -------------------------------- GC list helper -------------------------------- */

#define add_continuation(sc, p)      add_to_gc_list(sc, sc->continuations, p)

#define NOT_BAFFLED -1

  #define H_is_continuation "(continuation? obj) returns #t if obj is a continuation"
  #define Q_is_continuation sc->pl_bt

  #define H_is_goto "(goto? obj) returns #t if obj is a call-with-exit exit function"
  #define Q_is_goto sc->pl_bt

  #define H_call_cc "(call-with-current-continuation (lambda (continuer) ...)) evaluates the body with continuer as a way to goto to the continuation of the body"
  #define Q_call_cc s7_make_signature(sc, 2, sc->values_symbol, sc->is_procedure_symbol)

  #define H_call_with_exit "(call-with-exit (lambda (exiter) ...)) is call/cc without the ability to jump back into a previous computation."
  #define Q_call_with_exit s7_make_signature(sc, 2, sc->values_symbol, sc->is_procedure_symbol)

/* -------------------------------- function declarations -------------------------------- */

void process_continuation(s7_scheme *sc, s7_pointer cc);
void mark_continuation(s7_pointer cc);
bool is_continuation_b_p(s7_pointer p);
s7_pointer copy_any_list(s7_scheme *sc, s7_pointer a);
void call_with_current_continuation(s7_scheme *sc);
s7_pointer g_call_cc(s7_scheme *sc, s7_pointer args);
void op_call_cc(s7_scheme *sc);
bool op_implicit_continuation_a(s7_scheme *sc);
void call_with_exit(s7_scheme *sc);
s7_pointer g_call_with_exit(s7_scheme *sc, s7_pointer args);
void op_call_with_exit(s7_scheme *sc);
void op_call_with_exit_o(s7_scheme *sc);
bool op_implicit_goto(s7_scheme *sc);
bool op_implicit_goto_a(s7_scheme *sc);
void check_with_baffle(s7_scheme *sc);
bool op_with_baffle_unchecked(s7_scheme *sc);
void continuation_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci);
s7_pointer b_is_continuation_setter(s7_scheme *sc, s7_pointer args);
s7_pointer s7_make_continuation(s7_scheme *sc);

#endif /* S7_CONTINUATION_H */
