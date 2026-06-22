/* s7_continuation.c - continuation and goto implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#include "s7_internal.h"
#include "s7_continuation.h"

/* -------------------------------- GC sweep helper -------------------------------- */

void process_continuation(s7_scheme *sc, s7_pointer cc)
{
  continuation_op_stack(cc) = NULL;
  liberate_block(sc, continuation_block(cc)); /* from mallocate_block (s7_make_continuation) */
}

/* -------------------------------- GC mark helper -------------------------------- */

void mark_continuation(s7_pointer cc)
{
  set_mark(cc);
  if (!is_marked(continuation_stack(cc))) /* can these be cyclic? */
    mark_stack_1(continuation_stack(cc), continuation_stack_top(cc));
  s7_mark(continuation_op_stack(cc));
}

/* -------------------------------- continuations and gotos -------------------------------- */

/* ----------------------- continuation? -------------------------------- */
/* g_is_continuation is now defined in s7_scheme_predicate.c */

bool is_continuation_b_p(s7_pointer p) {return(is_continuation(p));}

#if S7_DEBUGGING
static s7_pointer check_wrap_return(s7_pointer lst)
{
  for (s7_pointer fast = lst, slow = lst; is_pair(fast); slow = cdr(slow), fast = cdr(fast))
    {
      if (is_matched_pair(fast)) fprintf(stderr, "%s[%d]: matched_pair not cleared\n", __func__, __LINE__);
      fast = cdr(fast);
      if (!is_pair(fast)) return(lst);
      if (fast == slow) return(lst);
      if (is_matched_pair(fast)) fprintf(stderr, "%s[%d]: matched_pair not cleared\n", __func__, __LINE__);
    }
  return(lst);
}
#endif

s7_pointer copy_any_list(s7_scheme *sc, s7_pointer a)
{
  s7_pointer slow = cdr(a);
  s7_pointer fast = slow;
  s7_pointer p;
#if S7_DEBUGGING
  #define wrap_return(W) do {fast = W; W = sc->unused; end_temp(sc->y); return(check_wrap_return(fast));} while (0)
#else
  #define wrap_return(W) do {fast = W; W = sc->unused; end_temp(sc->y); return(fast);} while (0)
#endif
  begin_temp(sc->y, a); /* gc_protect_via_stack doesn't work here because we're called in copy_stack, I think (trouble is in call/cc stuff) */
  sc->w = list_1(sc, car(a));
  p = sc->w;
  while (true)
    {
      if (!is_pair(fast))
	{
	  if (is_null(fast))
	    wrap_return(sc->w);
	  set_cdr(p, fast);
	  wrap_return(sc->w);
	}

      set_cdr(p, list_1(sc, car(fast)));
      p = cdr(p);
      fast = cdr(fast);
      if (!is_pair(fast))
	{
	  if (is_null(fast))
	    wrap_return(sc->w);
	  set_cdr(p, fast);
	  wrap_return(sc->w);
	}
      /* if unrolled further, it's a lot slower? */
      set_cdr(p, list_1_unchecked(sc, car(fast)));
      p = cdr(p);
      fast = cdr(fast);
      slow = cdr(slow);
      if (fast == slow)
	{
	  /* try to preserve the original cyclic structure */
	  s7_pointer p1, f1, p2, f2;
	  set_match_pair(a);
	  for (p1 = sc->w, f1 = a; !(is_matched_pair(cdr(f1))); f1 = cdr(f1), p1 = cdr(p1))
	    set_match_pair(f1);
	  for (p2 = sc->w, f2 = a; cdr(f1) != f2; f2 = cdr(f2), p2 = cdr(p2))
	    clear_match_pair(f2);
	  for (f1 = f2; is_pair(f1); f1 = cdr(f1), f2 = cdr(f2))
	    {
	      clear_match_pair(f1);
	      f1 = cdr(f1);
	      clear_match_pair(f1);
	      if (f1 == f2) break;
	    }
	  clear_match_pair(a);
	  if (is_null(p1))
	    set_cdr(p2, p2);
	  else set_cdr(p1, p2);
	  wrap_return(sc->w);
	}}
  wrap_return(sc->w);
}

static s7_pointer copy_counter(s7_scheme *sc, s7_pointer obj)
{
  s7_pointer nobj;
  new_cell(sc, nobj, T_COUNTER);
  counter_set_result(nobj, counter_result(obj));
  counter_set_list(nobj, counter_list(obj));
  counter_set_capture(nobj, counter_capture(obj));
  counter_set_let(nobj, counter_let(obj));
  counter_set_slots(nobj, counter_slots(obj));
  return(nobj);
}

static void stack_list_set_immutable(s7_pointer pold, s7_pointer pnew)
{
  for (s7_pointer p1 = pold, p2 = pnew, slow = pold; is_pair(p2); p1 = cdr(p1), p2 = cdr(p2))
    {
      if (is_immutable(p1)) set_immutable_pair(p2);
      if (is_pair(cdr(p1)))
	{
	  p1 = cdr(p1);
	  p2 = cdr(p2);
	  if (is_immutable(p1)) set_immutable_pair(p2);
	  if (p1 == slow) break;
	  slow = cdr(slow);
	}}
}

static s7_pointer copy_stack(s7_scheme *sc, s7_pointer new_v, s7_pointer old_v, s7_int top)
{
  bool has_pairs = false;
  s7_pointer *nv = stack_elements(new_v);
  s7_pointer *ov = stack_elements(old_v);
  memcpy((void *)nv, (void *)ov, top * sizeof(s7_pointer));
  stack_clear_flags(new_v);

  s7_gc_on(sc, false);
  if (stack_has_counters(old_v))
    {
      for (s7_int i = 2; i < top; i += 4)
	{
	  const s7_pointer p = ov[i];               /* args */
	  /* if op_gc_protect, any ov[i] (except op) can be a list, but it isn't the arglist, so it seems to be safe */
	  if (is_pair(p))                           /* args need not be a list (it can be a port or #f, etc) */
	    {
	      has_pairs = true;
	      if (is_null(cdr(p)))
		nv[i] = cons_unchecked(sc, car(p), sc->nil); /* GC is off -- could uncheck list_2 et al also */
	      else
		if ((is_pair(cdr(p))) && (is_null(cddr(p))))
		  nv[i] = list_2_unchecked(sc, car(p), cadr(p));
		else nv[i] = copy_any_list(sc, p);  /* args (copy is needed -- see s7test.scm) */
	      /* if op=eval_args4 for example, this has to be a proper list, and in many cases it doesn't need to be copied */
	      stack_list_set_immutable(p, nv[i]);
	    }
	  /* lst can be dotted or circular here.  The circular list only happens in a case like:
	   *    (dynamic-wind (lambda () (eq? (let ((lst (cons 1 2))) (set-cdr! lst lst) lst) (call/cc (lambda (k) k)))) (lambda () #f) (lambda () #f))
	   *    proper_list_reverse_in_place(sc->args) is one reason we need to copy
	   */
	  else
	    if (is_counter(p))                     /* these can only occur in this context (not in a list etc) */
	      {
		stack_set_has_counters(new_v);
		nv[i] = copy_counter(sc, p);
	      }}}
  else
    for (s7_int i = 2; i < top; i += 4)
      if (is_pair(ov[i]))
	{
	  const s7_pointer p = ov[i];
	  has_pairs = true;
	  if (is_null(cdr(p)))
	    nv[i] = cons_unchecked(sc, car(p), sc->nil);
	  else
	    if ((is_pair(cdr(p))) && (is_null(cddr(p))))
	      nv[i] = list_2_unchecked(sc, car(p), cadr(p));
	    else nv[i] = copy_any_list(sc, p);  /* args (copy is needed -- see s7test.scm) */
	  stack_list_set_immutable(p, nv[i]);
	}
  if (has_pairs) stack_set_has_pairs(new_v);
  s7_gc_on(sc, true);
  return(new_v);
}

static s7_pointer copy_op_stack(s7_scheme *sc)
{
  int32_t len = (int32_t)(sc->op_stack_now - sc->op_stack);
  s7_pointer nv = make_simple_vector(sc, len); /* not sc->op_stack_size */
  if (len > 0)
    {
      s7_pointer *src = sc->op_stack;
      s7_pointer *dst = (s7_pointer *)vector_elements(nv);
      for (int32_t i = len; i > 0; i--) *dst++ = *src++;
    }
  return(nv);
}

/* -------------------------------- with-baffle -------------------------------- */
/* (with-baffle . body) calls body guaranteeing that there can be no jumps into the
 *    middle of it from outside -- no outer evaluation of a continuation can jump across this
 *    barrier:  The flip-side of call-with-exit.
 */

static bool find_baffle(s7_scheme *sc, s7_int key)
{
  /* search backwards through sc->curlet for baffle_let with (continuation_)key as its baffle_key value */
  if (sc->baffle_ctr > 0)
    for (s7_pointer let = sc->curlet; let; let = let_outlet(let))
      if ((is_baffle_let(let)) &&
	  (let_baffle_key(let) == key))
	return(true);
  return(false);
}

static s7_int find_any_baffle(s7_scheme *sc)
{
  /* search backwards through sc->curlet for any sc->baffle_symbol -- called by s7_make_continuation to set continuation_key */
  if (sc->baffle_ctr > 0)
    for (s7_pointer let = sc->curlet; let; let = let_outlet(let))
      if (is_baffle_let(let))
	return(let_baffle_key(let));
  return(NOT_BAFFLED);
}

void check_with_baffle(s7_scheme *sc)
{
  if (!s7_is_proper_list(sc, sc->code))
    syntax_error_nr(sc, "with-baffle: unexpected dot? ~A", 31, sc->code);
  pair_set_syntax_op(sc->code, OP_WITH_BAFFLE_UNCHECKED);
}

bool op_with_baffle_unchecked(s7_scheme *sc)
{
  sc->code = cdr(sc->code);
  if (is_null(sc->code))
    {
      sc->value = sc->nil;
      return(true);
    }
  set_curlet(sc, make_let(sc, sc->curlet));
  set_baffle_let(sc->curlet);
  let_set_baffle_key(sc->curlet, sc->baffle_ctr++);
  return(false);
}


/* -------------------------------- call/cc -------------------------------- */
static void make_room_for_cc_stack(s7_scheme *sc)
{
  if ((s7_int)(sc->free_heap_top - sc->free_heap) < (s7_int)(sc->heap_size / 32)) /* we probably never need this much space (8 becomes enormous, 512 seems ok) */
    {                                                                               /*  but this doesn't seem to make much difference in timings */
      call_gc(sc);
      if ((s7_int)(sc->free_heap_top - sc->free_heap) < (s7_int)(sc->heap_size / 32))
	resize_heap(sc);
    }
}

s7_pointer s7_make_continuation(s7_scheme *sc)
{
  /* precede this with make_room_for_cc_stack(sc); */
  const s7_int loc = stack_top(sc);
  const s7_pointer stack = make_simple_vector(sc, loc);
  s7_pointer new_cc;
  block_t *block;

  set_full_type(stack, T_STACK);
  temp_stack_top(stack) = loc;
  begin_temp(sc->x, stack);
  copy_stack(sc, stack, sc->stack, loc);

  new_cell(sc, new_cc, T_CONTINUATION);
  block = mallocate_block(sc);
#if S7_DEBUGGING
  sc->blocks_mallocated[BLOCK_LIST]++;
#endif
  continuation_block(new_cc) = block;
  continuation_set_stack(new_cc, stack);
  continuation_stack_size(new_cc) = vector_length(continuation_stack(new_cc));
  continuation_stack_start(new_cc) = stack_elements(continuation_stack(new_cc));
  continuation_stack_end(new_cc) = (s7_pointer *)(continuation_stack_start(new_cc) + loc);
  continuation_op_stack(new_cc) = copy_op_stack(sc);
  continuation_op_loc(new_cc) = (int32_t)(sc->op_stack_now - sc->op_stack);
  continuation_op_size(new_cc) = sc->op_stack_size;
  continuation_key(new_cc) = find_any_baffle(sc);
  continuation_name(new_cc) = sc->F;
  end_temp(sc->x);
  add_continuation(sc, new_cc);
  return(new_cc);
}

static bool check_for_dynamic_winds(s7_scheme *sc, s7_pointer cont)
{
  /* called only from call_with_current_continuation.
   *   if call/cc jumps into a dynamic-wind, the init/finish funcs are wrapped in with-baffle
   *   so they'll complain.  Otherwise we're supposed to re-run the init func before diving
   *   into the body.  Similarly for let-temporarily.  If a call/cc jumps out of a dynamic-wind
   *   body-func, we're supposed to call the finish-func.  The continuation is called at
   *   stack_top(sc); the continuation form is at continuation_stack_top(cont).
   *
   * check sc->stack for dynamic-winds we're jumping out of
   *    we need to check from the current stack top down to where the continuation stack matches the current stack??
   *    this was (i > 0), but that goes too far back; perhaps s7 should save the position of the call/cc invocation.
   *    also the two stacks can be different sizes (either can be larger)
   */
  const s7_pointer cc_stack = continuation_stack(cont);
  const s7_int cc_top = continuation_stack_top(cont);
  for (s7_int op_loc = stack_top(sc) - 1; (op_loc > 0) && ((op_loc >= cc_top) || (stack_code(sc->stack, op_loc) != stack_code(cc_stack, op_loc))); op_loc -= 4)
    {
      const opcode_t op = stack_op(sc->stack, op_loc);
      switch (op)
	{
	case OP_DYNAMIC_WIND:
	case OP_LET_TEMP_DONE:
	  {
	    const s7_pointer code = stack_code(sc->stack, op_loc);
	    s7_int s_base = 0;
	    for (s7_int j = 3; j < cc_top; j += 4)
	      if (((stack_op(cc_stack, j) == OP_DYNAMIC_WIND) ||
		   (stack_op(cc_stack, j) == OP_LET_TEMP_DONE)) &&
		  (code == stack_code(cc_stack, j)))
		{
		  s_base = op_loc;
		  break;
		}
	    if (s_base == 0)
	      {
		if (op == OP_DYNAMIC_WIND)
		  {
		    if (dynamic_wind_state(code) == dwind_body)
		      {
			dynamic_wind_state(code) = dwind_finish;
			if (dynamic_wind_out(code) != sc->F)
			  sc->value = s7_call(sc, dynamic_wind_out(code), sc->nil);
		      }}
		else let_temp_done(sc, stack_args(sc->stack, op_loc), T_Let(stack_let(sc->stack, op_loc)));
	      }}
	  break;

	case OP_DYNAMIC_UNWIND:
	  {
	    s7_pointer func = stack_code(sc->stack, op_loc);
	    s7_pointer args = stack_args(sc->stack, op_loc);
	    if ((is_pair(cdr(args))) && (is_pair(cddr(args))) && (caddr(args) == sc->T))
	      dynamic_unwind(sc, func, args);
	  }
	case OP_DYNAMIC_UNWIND_PROFILE:
	  set_stack_op(sc->stack, op_loc, OP_GC_PROTECT);
	  break;

	case OP_LET_TEMP_UNWIND:
	  let_temp_unwind(sc, stack_code(sc->stack, op_loc), stack_args(sc->stack, op_loc));
	  break;

	case OP_LET_TEMP_S7_UNWIND:
	  starlet_set_1(sc, stack_code(sc->stack, op_loc), stack_args(sc->stack, op_loc));
	  break;

	case OP_LET_TEMP_S7_OPENLETS_UNWIND:
	  sc->has_openlets = (stack_args(sc->stack, op_loc) != sc->F);
	  break;

	case OP_BARRIER:
	  if (op_loc > cc_top)                /* otherwise it's some unproblematic outer eval-string? */
	    return(false);                    /*    but what if we've already evaluated a dynamic-wind closer? */
	  break;

	case OP_DEACTIVATE_GOTO:              /* here we're jumping out of an unrelated call-with-exit block */
	  if (op_loc > cc_top)
	    call_exit_active(stack_args(sc->stack, op_loc)) = false;
	  break;

	case OP_UNWIND_INPUT:
	  if (stack_args(sc->stack, op_loc) != sc->unused)
	    set_current_input_port(sc, stack_args(sc->stack, op_loc));    /* "args" = port that we shadowed */
	  break;

	case OP_UNWIND_OUTPUT:
	  if (stack_args(sc->stack, op_loc) != sc->unused)
	    set_current_output_port(sc, stack_args(sc->stack, op_loc));   /* "args" = port that we shadowed */
	  break;

	default:
	  if ((S7_DEBUGGING) && (op == OP_MAP_UNWIND)) fprintf(stderr, "%s[%d]: unwind %" ld64 "\n", __func__, __LINE__, sc->map_call_ctr);
	  break;
	}}

  /* check continuation-stack for dynamic-winds we're jumping into */
  for (s7_int op_loc = stack_top(sc) - 1; op_loc < cc_top; op_loc += 4)
    {
      const opcode_t op = stack_op(cc_stack, op_loc);
      if (op == OP_DYNAMIC_WIND)
	{
	  s7_pointer dw = T_Dyn(stack_code(cc_stack, op_loc));
	  if (dynamic_wind_in(dw) != sc->F)
	    sc->value = s7_call(sc, dynamic_wind_in(dw), sc->nil);
	  dynamic_wind_state(dw) = dwind_body;
	}
      else
	if (op == OP_DEACTIVATE_GOTO)
	  call_exit_active(stack_args(cc_stack, op_loc)) = true;
      /* not let_temp_done here! */
      /* if op == OP_LET_TEMP_DONE, we're jumping back into a let-temporarily.  MIT and Chez scheme say they remember the
       *   let-temp vars (fluid-let or parameters in their terminology) at the point of the call/cc, and restore them
       *   on re-entry; that strikes me as incoherently complex -- they've wrapped a hidden dynamic-wind around the
       *   call/cc to restore all let-temp vars!  I think let-temp here should be the same as let -- if you jump back
       *   in, nothing hidden happens. So,
       *     (let ((x #f) (cc #f))
       *       (let-temporarily ((x 1))
       *         (set! x 2) (call/cc (lambda (r) (set! cc r))) (display x) (unless (= x 2) (newline) (exit)) (set! x 3) (cc)))
       *   behaves the same (in this regard) if let-temp is replaced with let.
       */
    }
  return(true);
}

void call_with_current_continuation(s7_scheme *sc)
{
  s7_pointer cont = sc->code;  /* sc->args are the returned values */

  /* check for (baffle ...) blocking the current attempt to continue */
  if ((continuation_key(cont) != NOT_BAFFLED) &&
      (!find_baffle(sc, continuation_key(cont))))
    error_nr(sc, sc->baffled_symbol,
	     (is_symbol(continuation_name(sc->code))) ?
	     set_elist_2(sc, wrap_string(sc, "continuation ~S can't jump into with-baffle", 43), continuation_name(cont)) :
	     set_elist_1(sc, wrap_string(sc, "continuation can't jump into with-baffle", 40)));

  if (check_for_dynamic_winds(sc, cont))
    {
      /* make_room_for_cc_stack(sc); */ /* 28-May-21 */
      /* we push_stack sc->code before calling an embedded eval above, so sc->code should still be cont here, etc */
      if ((stack_has_pairs(continuation_stack(cont))) ||
	  (stack_has_counters(continuation_stack(cont))))
	{
	  make_room_for_cc_stack(sc);
	  copy_stack(sc, sc->stack, continuation_stack(cont), continuation_stack_top(cont));
	}
      else
	{
	  s7_pointer *nv = stack_elements(sc->stack);
	  s7_pointer *ov = stack_elements(continuation_stack(cont));
	  memcpy((void *)nv, (void *)ov, continuation_stack_top(cont) * sizeof(s7_pointer));
	}
      /* copy_stack(sc, sc->stack, continuation_stack(cont), continuation_stack_top(cont)); */
      sc->stack_end = (s7_pointer *)(sc->stack_start + continuation_stack_top(cont));

      {
	const int32_t top = continuation_op_loc(cont);
	s7_pointer *src, *dst;
	sc->op_stack_now = (s7_pointer *)(sc->op_stack + top);
	sc->op_stack_size = continuation_op_size(cont);
	sc->op_stack_end = (s7_pointer *)(sc->op_stack + sc->op_stack_size);
	src = (s7_pointer *)vector_elements(continuation_op_stack(cont));
	dst = sc->op_stack;
	for (int32_t i = 0; i < top; i++) dst[i] = src[i];
      }
      sc->value = (is_null(sc->args)) ? sc->nil : ((is_null(cdr(sc->args))) ? car(sc->args) : splice_in_values(sc, sc->args));
    }
}

s7_pointer g_call_cc(s7_scheme *sc, s7_pointer args)
{
  #define H_call_cc "(call-with-current-continuation (lambda (continuer) ...)) evaluates the body with continuer as a way to goto to the continuation of the body"
  #define Q_call_cc s7_make_signature(sc, 2, sc->values_symbol, sc->is_procedure_symbol)

  const s7_pointer func = car(args);         /* this is the procedure passed to call/cc */
  if (!is_t_procedure(func))                 /* this includes continuations */
    {
      if_method_exists_return_value(sc, func, sc->call_cc_symbol, args);
      if_method_exists_return_value(sc, func, sc->call_with_current_continuation_symbol, args);
      sole_arg_wrong_type_error_nr(sc, sc->call_cc_symbol, func, a_procedure_string);
    }
  if (((!is_closure(func)) ||
       (closure_arity(func) != 1)) &&
      (!s7_is_aritable(sc, func, 1)))
    error_nr(sc, sc->wrong_type_arg_symbol,
	     set_elist_2(sc, wrap_string(sc, "call/cc procedure, ~A, should take one argument", 47), func));

  make_room_for_cc_stack(sc);
  begin_temp(sc->y, s7_make_continuation(sc));
  if ((is_any_closure(func)) && (is_pair(closure_pars(func))) && (is_symbol(car(closure_pars(func)))))
    continuation_name(sc->y) = car(closure_pars(func));
  push_stack(sc, OP_APPLY, list_1_unchecked(sc, sc->y), func); /* apply func to continuation */
  end_temp(sc->y);
  return(sc->nil);
}

void op_call_cc(s7_scheme *sc) /* OP_CALL_CC in eval via optimize_c_function_one_arg */
{
  make_room_for_cc_stack(sc);
  begin_temp(sc->y, s7_make_continuation(sc));
  continuation_name(sc->y) = caar(opt2_pair(sc->code)); /* caadadr(sc->code) */
  set_curlet(sc, inline_make_let_with_slot(sc, sc->curlet, continuation_name(sc->y), sc->y));
  end_temp(sc->y);
  sc->code = cdr(opt2_pair(sc->code)); /* cddadr(sc->code) */
}

bool op_implicit_continuation_a(s7_scheme *sc)
{
  s7_pointer code = sc->code; /* dumb-looking code, but it's faster than the pretty version, according to callgrind */
  s7_pointer cont = lookup_checked(sc, car(code));
  if (!is_continuation(cont)) {sc->last_function = cont; return(false);}
  sc->code = cont;
  sc->args = set_plist_1(sc, fx_call(sc, cdr(code)));
  call_with_current_continuation(sc);
  return(true);
}


/* -------------------------------- call-with-exit -------------------------------- */
void call_with_exit(s7_scheme *sc)
{
  s7_int op_loc, new_stack_top, quit = 0;

  if (!call_exit_active(sc->code))
    error_nr(sc, sc->invalid_exit_function_symbol,
	     (is_symbol(call_exit_name(sc->code))) ?
	       set_elist_2(sc, wrap_string(sc, "call-with-exit exit procedure, ~A, called outside its block", 59), call_exit_name(sc->code)) :
	       set_elist_1(sc, wrap_string(sc, "call-with-exit exit procedure called outside its block", 54)));

  call_exit_active(sc->code) = false;
  new_stack_top = call_exit_goto_loc(sc->code);
  sc->op_stack_now = (s7_pointer *)(sc->op_stack + call_exit_op_loc(sc->code));

  /* look for dynamic-wind in the stack section that we are jumping out of */
  op_loc = stack_top(sc) - 1;
  /* op is entirely op_deactivate_goto tgc, for_each_2|3 tcase, dox_step_o texit, lots of ops s7test.scm */
  /* if (stack_op(sc->stack, op_loc) == OP_DEACTIVATE_GOTO) {call_exit_active(stack_args(sc->stack, op_loc)) = false; goto SET_VALUE;} saves >54 in tgc */

  do {
    switch (stack_op(sc->stack, op_loc)) /* the hit rate here is good; exiters[op] slowed us down! (see tmp) tgc/texit slower, tcase faster */
      {
      case OP_DYNAMIC_WIND:
	{
	  const s7_pointer lx = T_Dyn(stack_code(sc->stack, op_loc));
	  if (dynamic_wind_state(lx) == dwind_body)
	    {
	      dynamic_wind_state(lx) = dwind_finish;
	      if (dynamic_wind_out(lx) != sc->F)
		{
		  s7_pointer arg = (sc->args == sc->plist_1) ? car(sc->plist_1) : sc->unused;  /* might also need GC protection here */
		  /* protect the sc->args value across this call if it is sc->plist_1 -- I can't find a broken case */
		  sc->value = s7_call(sc, dynamic_wind_out(lx), sc->nil);
		  if (arg != sc->unused) set_plist_1(sc, arg);
		}}}
	break;

      case OP_DYNAMIC_UNWIND:
      case OP_DYNAMIC_UNWIND_PROFILE:
	set_stack_op(sc->stack, op_loc, OP_GC_PROTECT);
	dynamic_unwind(sc, stack_code(sc->stack, op_loc), stack_args(sc->stack, op_loc));
	break;

      case OP_EVAL_STRING:
	s7_close_input_port(sc, current_input_port(sc));
	pop_input_port(sc);
	break;

      case OP_BARRIER:                /* oops -- we almost certainly went too far */
	goto SET_VALUE;

      case OP_DEACTIVATE_GOTO:        /* here we're jumping into an unrelated call-with-exit block */
	call_exit_active(stack_args(sc->stack, op_loc)) = false;
	break;

      case OP_LET_TEMP_DONE:
	{
	  s7_pointer old_args = sc->args;
	  let_temp_done(sc, stack_args(sc->stack, op_loc), T_Let(stack_let(sc->stack, op_loc)));
	  sc->args = old_args;
	}
	break;

      case OP_LET_TEMP_UNWIND:
	let_temp_unwind(sc, stack_code(sc->stack, op_loc), stack_args(sc->stack, op_loc));
	break;

      case OP_LET_TEMP_S7_UNWIND:
	starlet_set_1(sc, stack_code(sc->stack, op_loc), stack_args(sc->stack, op_loc));
	break;

      case OP_LET_TEMP_S7_OPENLETS_UNWIND:
	sc->has_openlets = (stack_args(sc->stack, op_loc) != sc->F);
	break;

	/* call/cc does not close files, but I think call-with-exit should */
      case OP_GET_OUTPUT_STRING:
      case OP_UNWIND_OUTPUT:
	{
	  s7_pointer port = T_Pro(stack_code(sc->stack, op_loc));  /* "code" = port that we opened */
	  s7_close_output_port(sc, port);
	  port = stack_args(sc->stack, op_loc);                    /* "args" = port that we shadowed, if not #<unused> */
	  if (port != sc->unused)
	    set_current_output_port(sc, port);
	}
	break;

      case OP_UNWIND_INPUT:
	s7_close_input_port(sc, T_Pri(stack_code(sc->stack, op_loc))); /* "code" = port that we opened */
	if (stack_args(sc->stack, op_loc) != sc->unused)
	  set_current_input_port(sc, stack_args(sc->stack, op_loc));   /* "args" = port that we shadowed */
	break;

      case OP_EVAL_DONE: /* goto called in a method -- put off the inner eval return(s) until we clean up the stack */
	quit++;
	break;

      default:
	if ((S7_DEBUGGING) && (stack_op(sc->stack, op_loc) == OP_MAP_UNWIND)) fprintf(stderr, "%s[%d]: unwind %" ld64 "\n", __func__, __LINE__, sc->map_call_ctr);
	break;
      }
    op_loc -= 4;
  } while (op_loc > new_stack_top);

 SET_VALUE:
  sc->stack_end = (s7_pointer *)(sc->stack_start + new_stack_top);

  /* the return value should have an implicit values call, just as in call/cc */
  sc->value = (is_null(sc->args)) ? sc->nil : ((is_null(cdr(sc->args))) ? car(sc->args) : splice_in_values(sc, sc->args));
  if (quit > 0)
    {
      if (sc->longjmp_ok)
	{
	  pop_stack(sc);
	  LongJmp(*(sc->goto_start), call_with_exit_jump);
	}
      for (s7_int i = 0; i < quit; i++)
	push_stack_op_let(sc, OP_EVAL_DONE);
    }
}

/* g_is_goto is now defined in s7_scheme_predicate.c */

#undef wrap_return
static inline s7_pointer make_goto(s7_scheme *sc, s7_pointer name) /* inline for 73=1% in tgc */
{
  s7_pointer new_goto;
  new_cell(sc, new_goto, T_GOTO);
  call_exit_goto_loc(new_goto) = stack_top(sc);
  call_exit_op_loc(new_goto) = (int32_t)(sc->op_stack_now - sc->op_stack);
  call_exit_active(new_goto) = true;
  call_exit_name(new_goto) = name;
  return(new_goto);
}

s7_pointer g_call_with_exit(s7_scheme *sc, s7_pointer args)   /* (call-with-exit (lambda (return) ...)) */
{
  #define H_call_with_exit "(call-with-exit (lambda (exiter) ...)) is call/cc without the ability to jump back into a previous computation."
  #define Q_call_with_exit s7_make_signature(sc, 2, sc->values_symbol, sc->is_procedure_symbol)

  const s7_pointer func = car(args);
  s7_pointer new_goto;
  if (is_any_closure(func)) /* lambda or lambda* */
    {
      new_goto = make_goto(sc, ((is_pair(closure_pars(func))) && (is_symbol(car(closure_pars(func))))) ? car(closure_pars(func)) : sc->F);
      push_stack(sc, OP_DEACTIVATE_GOTO, new_goto, func); /* this means call-with-exit is not tail-recursive */
      push_stack(sc, OP_APPLY, cons_unchecked(sc, new_goto, sc->nil), func);
      return(sc->nil);
    }
  /* maybe just return an error here -- these gotos as args are stupid; also an error above if closure not aritable 1 */
  if (!is_t_procedure(func))
    return(method_or_bust_p(sc, func, sc->call_with_exit_symbol, a_procedure_string));
  if (!s7_is_aritable(sc, func, 1))
    error_nr(sc, sc->wrong_type_arg_symbol,
	     set_elist_2(sc, wrap_string(sc, "call-with-exit argument should be a function of one argument: ~S", 64), func));
  if (is_continuation(func)) /* (call/cc call-with-exit) ! */
    error_nr(sc, sc->wrong_type_arg_symbol,
	     set_elist_2(sc, wrap_string(sc, "call-with-exit argument should be a normal function (not a continuation: ~S)", 76), func));
  new_goto = make_goto(sc, sc->F);
  call_exit_active(new_goto) = false;
  return((is_c_function(func)) ? c_function_call(func)(sc, set_plist_1(sc, new_goto)) : s7_apply_function_star(sc, func, set_plist_1(sc, new_goto)));
}

inline void op_call_with_exit(s7_scheme *sc)
{
  s7_pointer args = opt2_pair(sc->code);
  s7_pointer go = make_goto(sc, caar(args));
  push_stack_no_let_no_code(sc, OP_DEACTIVATE_GOTO, go); /* was also pushing code */
  set_curlet(sc, inline_make_let_with_slot(sc, sc->curlet, caar(args), go));
  sc->code = T_Pair(cdr(args));
  /* goto begin */
}

void op_call_with_exit_o(s7_scheme *sc)
{
  op_call_with_exit(sc);
  sc->code = car(sc->code);
  /* goto eval */
}

bool op_implicit_goto(s7_scheme *sc)
{
  s7_pointer g = lookup_checked(sc, car(sc->code));
  if (!is_goto(g)) {sc->last_function = g; return(false);}
  sc->args = sc->nil;
  sc->code = g;
  call_with_exit(sc);
  return(true);
}

bool op_implicit_goto_a(s7_scheme *sc)
{
  s7_pointer g = lookup_checked(sc, car(sc->code));
  if (!is_goto(g)) {sc->last_function = g; return(false);}
  sc->args = set_plist_1(sc, fx_call(sc, cdr(sc->code)));
  sc->code = g;
  call_with_exit(sc);
  return(true);
}

/* -------------------------------- display/write helper -------------------------------- */

void continuation_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if (is_symbol(continuation_name(obj)))
    {
      port_write_string(port)(sc, "#<continuation", 14, port);
      port_write_string(port)(sc, (use_write == p_readable) ? "::" : " ", (use_write == p_readable) ? 2 : 1, port);
      symbol_to_port(sc, continuation_name(obj), port, p_display, NULL);
      port_write_character(port)(sc, '>', port);
    }
  else port_write_string(port)(sc, "#<continuation>", 15, port);
}

/* -------------------------------- setter helper -------------------------------- */

s7_pointer b_is_continuation_setter(s7_scheme *sc, s7_pointer args) {return(b_simple_setter(sc, T_CONTINUATION, args));}

