/* s7_scheme_let.c - let (environment) function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#include "s7_internal.h"
#include "s7_scheme_let.h"
#include "s7_scheme_base.h"

/* s7.c's make_integer/make_real fast-path macros depend on the static small_ints array;
 * the s7_make_* functions contain the same fast path internally. */
#define make_integer(Sc, N) s7_make_integer(Sc, N)
#define make_real(Sc, X)    s7_make_real(Sc, X)

/* ---------------------------------------- unlet ---------------------------------------- */
s7_pointer g_unlet(s7_scheme *sc, s7_pointer unused_args)
{
  /* add sc->unlet bindings to the current environment */

  const s7_pointer result = make_let(sc, sc->curlet);
  begin_temp(sc->y, result);
  set_is_unlet(result);
  if (global_value(sc->else_symbol) != sc->else_symbol)
    add_slot_checked_with_id(sc, result, sc->else_symbol, initial_value(sc->else_symbol));
  for (unlet_entry_t *p = sc->unlet_entries; p; p = p->next)
    {
      s7_pointer sym = p->symbol;
      if ((!is_eq_initial_value(sym, global_value(sym))) ||  /* it has been changed globally */
	  ((!is_global(sym)) &&        /* it might be shadowed locally */
	   (s7_symbol_local_value(sc, sym, sc->curlet) != global_value(sym))))
	add_slot_checked_with_id(sc, result, sym, initial_value(sym));
    }
  end_temp(sc->y);
  return(result);
}


/* ---------------------------------------- openlet? ---------------------------------------- */
bool s7_is_openlet(s7_pointer let) {return(has_methods(let));}


/* ---------------------------------------- openlet ---------------------------------------- */
s7_pointer s7_openlet(s7_scheme *sc, s7_pointer let)
{
  /* if e is not a let, the openlet bit is still set on it (c-pointer etc) */
  set_has_methods(let);
  return(let);
}


s7_pointer g_openlet(s7_scheme *sc, s7_pointer args)
{

  const s7_pointer let = car(args);
  s7_pointer new_let, func;
  if (!is_let(let))
    {
      new_let = find_let(sc, let);
      if ((!is_let(new_let)) || (new_let == sc->rootlet))
	find_let_error_nr(sc, sc->openlet_symbol, let, new_let, 1, args);
    }
  else new_let = let;
  if ((new_let == sc->rootlet) || (new_let == sc->starlet))
    error_nr(sc, sc->out_of_range_symbol, set_elist_2(sc, wrap_string(sc, "can't openlet ~S", 17), let));
  if (is_unlet(new_let)) /* protect against infinite loop: (let () (define + -) (with-let (unlet) (+ (openlet (unlet)) 2))) */
    error_nr(sc, sc->out_of_range_symbol, set_elist_1(sc, wrap_string(sc, "can't openlet unlet", 19)));
  if ((has_active_methods(sc, let)) &&
      ((func = find_method(sc, new_let, sc->openlet_symbol)) != sc->undefined))
    return(s7_apply_function(sc, func, args));
  set_has_methods(let);
  return(let); /* openlet and coverlet return their argument */
}


/* ---------------------------------------- coverlet ---------------------------------------- */
s7_pointer g_coverlet(s7_scheme *sc, s7_pointer args)
{

  const s7_pointer let = car(args);
  s7_pointer new_let, func;
  if (!is_let(let))
    {
      new_let = find_let(sc, let);
      if ((!is_let(new_let))  || (new_let == sc->rootlet))
	find_let_error_nr(sc, sc->coverlet_symbol, let, new_let, 1, args);
    }
  else new_let = let;
  if ((new_let == sc->rootlet) || (new_let == sc->starlet))
    error_nr(sc, sc->out_of_range_symbol, set_elist_2(sc, wrap_string(sc, "can't coverlet ~S", 17), let));
  if (is_unlet(new_let))
    error_nr(sc, sc->out_of_range_symbol, set_elist_1(sc, wrap_string(sc, "can't coverlet unlet", 20)));
  if ((has_active_methods(sc, let)) &&
      ((func = find_method(sc, new_let, sc->coverlet_symbol)) != sc->undefined))
    return(s7_apply_function(sc, func, args));
  clear_has_methods(let);
  return(let); /* mimic openlet in everything */
}


/* ---------------------------------------- varlet ---------------------------------------- */
static void check_let_fallback(s7_scheme *sc, const s7_pointer symbol, s7_pointer let)
{
  if (symbol == sc->let_ref_fallback_symbol)
    set_has_let_ref_fallback(let);
  else
    if (symbol == sc->let_set_fallback_symbol)
      set_has_let_set_fallback(let);
}


static void append_let(s7_scheme *sc, s7_pointer new_let, s7_pointer old_let)
{
  if (new_let == sc->rootlet)
    for (s7_pointer slot = let_slots(old_let); is_not_slot_end(slot); slot = next_slot(slot))
      {
	s7_pointer sym = slot_symbol(slot), val = slot_value(slot);
	if (is_slot(global_slot(sym)))
	  set_global_value(sym, val);
	else s7_make_slot(sc, sc->rootlet, sym, val);
      }
  else
    if (old_let == sc->starlet)
      {
	const s7_pointer iter = s7_make_iterator(sc, sc->starlet);
	const s7_int gc_loc = gc_protect_1(sc, iter);
	iterator_carrier(iter) = cons_unchecked(sc, sc->F, sc->F);
	set_has_carrier(iter); /* so carrier is GC protected by mark_iterator */
	while (true)
	  {
	    s7_pointer field = s7_iterate(sc, iter);
	    if (iterator_is_at_end(iter)) break;
	    add_slot_checked_with_id(sc, new_let, car(field), cdr(field));
	  }
	s7_gc_unprotect_at(sc, gc_loc);
      }
    else
      for (s7_pointer slot = let_slots(old_let); is_not_slot_end(slot); slot = next_slot(slot))
	add_slot_checked_with_id(sc, new_let, slot_symbol(slot), slot_value(slot)); /* not add_slot here because it might run off the free heap end */
}


s7_pointer s7_varlet(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value)
{
  if (!is_let(let))
    wrong_type_error_nr(sc, sc->varlet_symbol, 1, let, a_let_string);
  if (!is_symbol(symbol))
    wrong_type_error_nr(sc, sc->varlet_symbol, 2, symbol, a_symbol_string);
  if ((is_slot(global_slot(symbol))) &&
      (is_syntax(global_value(symbol))))
    wrong_type_error_nr(sc, sc->varlet_symbol, 2, symbol, wrap_string(sc, "a non-syntactic symbol", 22));

  if (let == sc->rootlet)
    {
      if (is_slot(global_slot(symbol)))
	set_global_value(symbol, value);
      else s7_make_slot(sc, sc->rootlet, symbol, value);
    }
  else
    {
      add_slot_checked_with_id(sc, let, symbol, value);
      check_let_fallback(sc, symbol, let);
    }
  return(value);
}


s7_pointer g_varlet(s7_scheme *sc, s7_pointer args)   /* varlet = with-let + define */
{
  s7_pointer let = car(args);
  if (!is_let(let))
    {
      s7_pointer new_let = find_let(sc, let);
      if ((!is_let(new_let)) || (new_let == sc->rootlet))
	find_let_error_nr(sc, sc->varlet_symbol, let, new_let, 1, args);
      let = new_let;
    }
  if ((is_immutable_let(let)) || (let == sc->starlet))
    immutable_object_error_nr(sc, set_elist_3(sc, wrap_string(sc, "can't (varlet ~{~S~^ ~}), ~S is immutable", 41), args, let));

  for (s7_pointer arglist = cdr(args); is_pair(arglist); arglist = cdr(arglist))
    {
      s7_pointer sym, val;
      const s7_pointer arg = car(arglist);
      if (is_symbol(arg))
	{
	  sym = (is_keyword(arg)) ? keyword_symbol(arg) : arg;
	  if (!is_pair(cdr(arglist)))
	    error_nr(sc, sc->syntax_error_symbol, set_elist_3(sc, wrap_string(sc, "varlet: symbol ~S, but no value: ~S", 35), arg, args));
	  if (is_constant_symbol(sc, sym))
	    wrong_type_error_nr(sc, sc->varlet_symbol, position_of(arglist, args), sym, a_non_constant_symbol_string);
	  arglist = cdr(arglist);
	  val = car(arglist);
	}
      else
	if (is_let(arg))
	  {
	    if ((arg != sc->rootlet) && (let != sc->starlet))   /* (varlet (inlet 'a 1) (rootlet)) is trouble */
	      {
		append_let(sc, let, arg);
		if (has_let_set_fallback(arg)) set_has_let_set_fallback(let);
		if (has_let_ref_fallback(arg)) set_has_let_ref_fallback(let);
	      }
	    continue;
	  }
	else
	  if (is_pair(arg))
	    {
	      sym = car(arg);
	      if (!is_symbol(sym))
		wrong_type_error_nr(sc, sc->varlet_symbol, position_of(arglist, args), arg, a_symbol_string);
	      if (is_constant_symbol(sc, sym))
		wrong_type_error_nr(sc, sc->varlet_symbol, position_of(arglist, args), sym, a_non_constant_symbol_string);
	      val = cdr(arg);
	    }
	  else wrong_type_error_nr(sc, sc->varlet_symbol, position_of(arglist, args), arg, wrap_string(sc, "a symbol, let, or cons", 22));

      if (let == sc->rootlet)
	{
	  s7_pointer gslot = global_slot(sym);
	  if (is_slot(gslot))
	    {
	      if (is_immutable(gslot)) /* (immutable! 'abs) (varlet (rootlet) 'abs 1) */
		immutable_object_error_nr(sc, set_elist_5(sc, wrap_string(sc, "~S is immutable in (varlet ~S '~S ~S)", 37), sym, car(args), arg, val));
	      slot_set_value_with_hook(global_slot(sym), val);
	    }
	  else s7_make_slot(sc, sc->rootlet, sym, val);
	}
      else
	{
	  check_let_fallback(sc, sym, let);
	  add_slot_checked_with_id(sc, let, sym, val);
	  /* this used to check for sym already defined, and set its value, but that greatly slows down
	   *   the most common use (adding a slot), and makes it hard to shadow explicitly.  Don't use
	   *   varlet as a substitute for set!/let-set!.
	   */
	}}
  return(let);
}


/* ---------------------------------------- cutlet ---------------------------------------- */
s7_pointer g_cutlet(s7_scheme *sc, s7_pointer args)
{

  s7_pointer let = car(args);
  s7_int the_un_id;
  if (let != sc->rootlet)
    {
      if_method_exists_return_value(sc, let, sc->cutlet_symbol, args);
      if (!is_let(let))
	{
	  s7_pointer new_let = find_let(sc, let);
	  if ((!is_let(new_let)) || (new_let == sc->rootlet))
	    find_let_error_nr(sc, sc->cutlet_symbol, let, new_let, 1, args);
	  let = new_let;
	}}
  if ((is_immutable_let(let)) || (let == sc->starlet))
    immutable_object_error_nr(sc, set_elist_3(sc, immutable_error_string, sc->cutlet_symbol, let));

  /* besides removing the slot we have to make sure the symbol_id does not match, else
   *   let-ref and others will use the old slot!  So use the next (unused) id.
   *   (let ((b 1)) (let ((b 2)) (cutlet (curlet) 'b)) b)
   */
  the_un_id = ++sc->let_number;

  for (s7_pointer syms = cdr(args); is_pair(syms); syms = cdr(syms))
    {
      s7_pointer sym = car(syms);
      if (!is_symbol(sym))
	wrong_type_error_nr(sc, sc->cutlet_symbol, position_of(syms, args), sym, a_symbol_string);
      if (is_keyword(sym))
	sym = keyword_symbol(sym);

      if (let == sc->rootlet)
	{
	  if (!is_slot(global_slot(sym)))
	    error_nr(sc, sc->out_of_range_symbol, set_elist_2(sc, wrap_string(sc, "cutlet can't remove ~S", 22), sym));
	  if (is_immutable(global_slot(sym)))
	    immutable_object_error_nr(sc, set_elist_3(sc, immutable_error_string, sc->cutlet_symbol, sym));
	  symbol_set_id(sym, the_un_id);
	  set_global_value(sym, sc->undefined);
	  /* here we need to at least clear bits: syntactic binder clean-symbol(?) etc, maybe also locally */
	}
      else
	{
	  s7_pointer slot;
	  if ((has_let_fallback(let)) &&
	      ((sym == sc->let_ref_fallback_symbol) || (sym == sc->let_set_fallback_symbol)))
	    error_nr(sc, sc->out_of_range_symbol, set_elist_2(sc, wrap_string(sc, "cutlet can't remove ~S", 22), sym));
	  slot = let_slots(let);
	  if (is_not_slot_end(slot))
	    {
	      if (slot_symbol(slot) == sym)
		{
		  if (is_immutable_slot(slot))
		    immutable_object_error_nr(sc, set_elist_3(sc, immutable_error_string, sc->cutlet_symbol, sym));
		  let_set_slots(let, next_slot(let_slots(let)));
		  symbol_set_id(sym, the_un_id);
		}
	      else
		{
		  s7_pointer last_slot = slot;
		  for (slot = next_slot(let_slots(let)); is_not_slot_end(slot); last_slot = slot, slot = next_slot(slot))
		    if (slot_symbol(slot) == sym)
		      {
			if (is_immutable_slot(slot))
			  immutable_object_error_nr(sc, set_elist_3(sc, immutable_error_string, sc->cutlet_symbol, sym));
			symbol_set_id(sym, the_un_id);
			slot_set_next(last_slot, next_slot(slot));
			break;
		      }}}}}
  return(let);
}


/* ---------------------------------------- sublet ---------------------------------------- */
static s7_pointer sublet_1(s7_scheme *sc, s7_pointer let, s7_pointer bindings, s7_pointer caller)
{
  const s7_pointer new_let = make_let(sc, let);
  set_all_methods(new_let, let);

  if (!is_null(bindings))
    {
      sc->temp3 = new_let;
      for (s7_pointer slot = NULL, entries = bindings; is_pair(entries); entries = cdr(entries))
	{
	  s7_pointer entry = car(entries), sym, val;

	  switch (type(entry))
	    {
	    case T_SYMBOL:
	      sym = (is_keyword(entry)) ? keyword_symbol(entry) : entry;
	      if (!is_pair(cdr(entries)))
		error_nr(sc, sc->syntax_error_symbol,
			 set_elist_4(sc, wrap_string(sc, "~A: entry ~S, but no value: ~S", 30), caller, entry, bindings));
	      entries = cdr(entries);
	      val = car(entries);
	      break;

	    case T_PAIR:  /* (cons sym val) */
	      sym = car(entry);
	      if (!is_symbol(sym))
		wrong_type_error_nr(sc, caller, 1 + position_of(entries, bindings), entry, a_symbol_string);
	      if (is_keyword(sym))
		sym = keyword_symbol(sym);
	      val = cdr(entry);
	      break;

	    case T_LET:
	      if ((entry == sc->rootlet) || (new_let == sc->starlet)) continue;
	      append_let(sc, new_let, entry);
	      if (is_not_slot_end(let_slots(new_let))) /* make sure the end slot (slot) is correct */
		for (slot = let_slots(new_let); is_not_slot_end(next_slot(slot)); slot = next_slot(slot)); /* slot can't be local -- see below */
	      continue;

	    default:
	      wrong_type_error_nr(sc, caller, 1 + position_of(entries, bindings), entry, a_symbol_string);
	    }
	  if (is_constant_symbol(sc, sym))
	    wrong_type_error_nr(sc, caller, 1 + position_of(entries, bindings), sym, a_non_constant_symbol_string);
#if 0
	  if ((is_slot(global_slot(sym))) &&
	      (is_syntax_or_qq(global_value(sym))))
	    wrong_type_error_nr(sc, caller, 2, sym, wrap_string(sc, "a non-syntactic symbol", 22));
	  /* this is a local redefinition which we accept elsewhere: (let ((if 3)) if) -> 3 */
	  /*   so s7_inlet (which calls sublet) differs from g_inlet? which is correct? */
	  /*   (define (f1) (with-let (sublet (curlet)) (inlet 'quasiquote 1))) (f1) */

#endif
	  /* here we know new_let is a let and is not rootlet */
	  if (!slot)
	    slot = add_slot_checked_with_id(sc, new_let, sym, val);
	  else
	    {
	      /* if (sc->free_heap_top <= sc->free_heap_trigger) try_to_call_gc(sc);*/ /* or maybe add add_slot_at_end_checked? */
	      slot = add_slot_checked_at_end(sc, let_id(new_let), slot, sym, val);
	      set_local(sym); /* ? */
	    }
	  check_let_fallback(sc, sym, new_let);
	}
      if ((S7_DEBUGGING) && (sc->temp3 != new_let)) fprintf(stderr, "%s[%d]: temp3: %s\n", __func__, __LINE__, display(sc->temp3));
      sc->temp3 = sc->unused;
    }
  return(new_let);
}


s7_pointer s7_sublet(s7_scheme *sc, s7_pointer let, s7_pointer bindings) {return(sublet_1(sc, let, bindings, sc->sublet_symbol));}


s7_pointer g_sublet(s7_scheme *sc, s7_pointer args)
{

  s7_pointer let = car(args);
  if (!is_let(let))
    {
      s7_pointer new_let = find_let(sc, let);
      if ((!is_let(new_let)) || (new_let == sc->rootlet))
	find_let_error_nr(sc, sc->sublet_symbol, let, new_let, 1, args);
      let = new_let;
    }
  return(sublet_1(sc, let, cdr(args), sc->sublet_symbol));
}


s7_pointer g_sublet_curlet(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = cadr(args), new_let;
  if_let_method_exists_return_value(sc, sc->curlet, sc->sublet_symbol, args); /* curlet is a let so... */
  new_let = inline_make_let_with_slot(sc, sc->curlet, sym, caddr(args));
  set_all_methods(new_let, sc->curlet);
  check_let_fallback(sc, sym, new_let);
  return(new_let);
}


s7_pointer sublet_chooser(s7_scheme *sc, s7_pointer func, int32_t num_args, s7_pointer expr)
{
  if (num_args == 3)
    {
      s7_pointer args = cdr(expr);
      if ((is_pair(car(args))) && (caar(args) == sc->curlet_symbol) && (is_null(cdar(args))) &&
	  (is_quoted_symbol(sc, cadr(args))))
	return(sc->sublet_curlet);
    }
  return(func);
}


/* ---------------------------------------- inlet ---------------------------------------- */
s7_pointer s7_inlet(s7_scheme *sc, s7_pointer args)
{
  return(sublet_1(sc, sc->rootlet, args, sc->inlet_symbol));
}


s7_pointer g_simple_inlet(s7_scheme *sc, s7_pointer args)
{
  /* here all args are paired with normal symbol/value, no fallbacks, no immutable symbols, no syntax, etc */
  const s7_pointer new_let = make_let(sc, sc->rootlet);
  const s7_int id = let_id(new_let);

  begin_temp(sc->temp6, new_let);
  for (s7_pointer x = args, last_slot = NULL; is_pair(x); x = cddr(x))
    {
      s7_pointer symbol = car(x);
      if (is_keyword(symbol))                 /* (inlet ':allow-other-keys 3) */
	symbol = keyword_symbol(symbol);
      if (is_constant_symbol(sc, symbol))     /* (inlet 'pi 1) */
	{
	  end_temp(sc->temp6);
	  wrong_type_error_nr(sc, sc->inlet_symbol, 1, symbol, a_non_constant_symbol_string);
	}
      if (!last_slot)
	{
	  add_slot_unchecked(sc, new_let, symbol, cadr(x), id);
	  last_slot = let_slots(new_let);
	}
      else last_slot = add_slot_checked_at_end(sc, id, last_slot, symbol, cadr(x));
    }
  end_temp(sc->temp6);
  return(new_let);
}


s7_pointer inlet_p_pp(s7_scheme *sc, s7_pointer symbol, s7_pointer value)
{
  if (!is_symbol(symbol))
    return(sublet_1(sc, sc->rootlet, set_plist_2(sc, symbol, value), sc->inlet_symbol));
  if (is_keyword(symbol))
    symbol = keyword_symbol(symbol);
  if (is_constant_symbol(sc, symbol))
    wrong_type_error_nr(sc, sc->inlet_symbol, 1, symbol, a_non_constant_symbol_string);
  if ((is_defined_global(symbol)) &&
      (is_syntax_or_qq(global_value(symbol))))
    wrong_type_error_nr(sc, sc->inlet_symbol, 1, symbol, wrap_string(sc, "a non-syntactic symbol", 22));
  {
    s7_pointer new_let;
    new_cell(sc, new_let, T_LET | T_SAFE_PROCEDURE);
    begin_temp(sc->x, new_let);
    let_set_id(new_let, ++sc->let_number);
    let_set_outlet(new_let, sc->rootlet);
    let_set_slots(new_let, slot_end);
    add_slot_unchecked(sc, new_let, symbol, value, let_id(new_let));
    end_temp(sc->x);
    return(new_let);
  }
}


s7_pointer internal_inlet(s7_scheme *sc, s7_int num_args, ...) /* used in *->let */
{
  va_list ap;
  const s7_pointer new_let = make_let(sc, sc->rootlet);
  const s7_int id = let_id(new_let);
  s7_pointer last_slot = NULL;

  begin_temp(sc->x, new_let);
  va_start(ap, num_args);
  for (s7_int i = 0; i < num_args; i += 2)
    {
      s7_pointer symbol = T_Sym(va_arg(ap, s7_pointer));
      s7_pointer value = T_Ext(va_arg(ap, s7_pointer));
      if (!last_slot)
	{
	  add_slot_unchecked(sc, new_let, symbol, value, id);
	  last_slot = let_slots(new_let);
	}
      else last_slot = add_slot_at_end(sc, id, last_slot, symbol, value);
    }
  va_end(ap);
  end_temp(sc->x);
  return(new_let);
}


s7_pointer inlet_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr)
{
  if ((args > 0) && ((args % 2) == 0))
    {
      for (s7_pointer p = cdr(expr); is_pair(p); p = cddr(p))
	{
	  s7_pointer sym;
	  if (is_symbol_and_keyword(car(p)))                  /* (inlet :if ...) */
	    sym = keyword_symbol(car(p));
	  else
	    {
	      if (!is_proper_quote(sc, car(p))) return(func); /* (inlet abs ...) */
	      sym = cadar(p);                                 /* looking for (inlet 'a ...) */
	      if (!is_symbol(sym)) return(func);              /* (inlet '(a . 3) ...) */
	      if (is_keyword(sym)) sym = keyword_symbol(sym); /* (inlet ':abs ...) */
	    }
	  if ((is_possibly_constant(sym)) ||                  /* (inlet 'define-constant ...) or (inlet 'pi ...) */
	      (is_syntactic_symbol(sym))  ||                  /* (inlet 'if 3) */
	      ((is_slot(global_slot(sym))) &&
	       (is_syntax_or_qq(global_value(sym)))) ||       /* (inlet 'quasiquote 1) */
	      (sym == sc->let_ref_fallback_symbol) ||
	      (sym == sc->let_set_fallback_symbol))
	    return(func);
	}
      return(sc->simple_inlet);
    }
  return(func);
}


/* ---------------------------------------- let->list ---------------------------------------- */
static s7_pointer abbreviate_let(s7_scheme *sc, s7_pointer val)
{
  if (is_let(val))
    return(make_symbol(sc, "<inlet...>", 11));
  return(val);
}


s7_pointer s7_let_to_list(s7_scheme *sc, s7_pointer let)
{
  if (let == sc->rootlet)
    {
      begin_temp(sc->temp6, sc->nil);
      for (s7_pointer lib = global_value(sc->libraries_symbol); is_pair(lib); lib = cdr(lib))
	sc->temp6 = cons(sc, caar(lib), sc->temp6);
      sc->temp6 = cons(sc, cons(sc, sc->libraries_symbol, sc->temp6), sc->nil);
      for (s7_pointer slot = sc->rootlet_slots; is_not_slot_end(slot); slot = next_slot(slot))
	if (slot_symbol(slot) != sc->libraries_symbol)
	  sc->temp6 = cons_unchecked(sc, cons(sc, slot_symbol(slot), abbreviate_let(sc, slot_value(slot))), sc->temp6);
      {
	s7_pointer result = proper_list_reverse_in_place(sc, sc->temp6);
	end_temp(sc->temp6);
	return(result);
      }}
  else
    {
      s7_pointer iter, func;
      s7_int gc_loc = -1;
      /* need to check make-iterator method before dropping into let->list */
      sc->temp3 = sc->w;
      sc->w = sc->nil;

      if ((has_active_methods(sc, let)) &&
	  ((func = find_method(sc, let, sc->make_iterator_symbol)) != sc->undefined))
	iter = s7_apply_function(sc, func, set_plist_1(sc, let));
      else
	if (let == sc->starlet) /* (let->list *s7*) via starlet_make_iterator */
	  {
	    iter = s7_make_iterator(sc, let);
	    gc_loc = gc_protect_1(sc, iter);
	  }
	else iter = sc->nil;

      if (is_null(iter))
	for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
	  sc->w = cons_unchecked(sc, cons(sc, slot_symbol(slot), slot_value(slot)), sc->w);
      else
	/* (begin (load "mockery.scm") (let ((lt ((*mock-pair* 'mock-pair) 1 2 3))) (format *stderr* "~{~A ~}" lt))) */
	while (true)
	  {
	    s7_pointer val = s7_iterate(sc, iter);
	    if (iterator_is_at_end(iter)) break;
	    sc->w = cons(sc, val, sc->w);
	  }
      sc->w = proper_list_reverse_in_place(sc, sc->w);
      if (gc_loc != -1)
	s7_gc_unprotect_at(sc, gc_loc);
      {
	s7_pointer result = sc->w;
	sc->w = sc->temp3;
	sc->temp3 = sc->unused;
	return(result);
      }}
}


#if !WITH_PURE_S7
s7_pointer g_let_to_list(s7_scheme *sc, s7_pointer args)
{

  s7_pointer let = car(args);
  if_method_exists_return_value(sc, let, sc->let_to_list_symbol, args);
  if (!is_let(let))
    {
      s7_pointer new_let = find_let(sc, let);
      if ((!is_let(new_let)) || (new_let == sc->rootlet))
	find_let_error_nr(sc, sc->let_to_list_symbol, let, new_let, 1, args);
      /* this is not (let->list (rootlet)) but (say) (let->list func) which defaults in find_let to rootlet */
      let = new_let;
    }
  return(s7_let_to_list(sc, let));
}
/* *s7* in gdb: p display(s7_let_to_list(sc, sc->starlet)) */
#endif


/* ---------------------------------------- let-ref ---------------------------------------- */
s7_pointer call_let_ref_fallback(s7_scheme *sc, s7_pointer let, s7_pointer symbol)
{
  s7_pointer result;
  const s7_pointer val = find_method(sc, let, sc->let_ref_fallback_symbol);
  /* (let ((x #f)) (let begin ((x 1234)) (begin 1) 2)) -> stack overflow eventually, but should we try to catch it? */
  if (!is_applicable(val)) return(val);
  push_stack_no_let(sc, OP_GC_PROTECT, sc->value, sc->code);
  result = s7_apply_function(sc, val, set_qlist_2(sc, let, symbol));
  unstack_gc_protect(sc);
  sc->code = T_Pos(stack_end_code(sc)); /* can be #<unused> */
  sc->value = T_Ext(stack_end_args(sc));
  return(result);
}


s7_pointer call_let_set_fallback(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value)
{
  s7_pointer result;
  push_stack_no_let(sc, OP_GC_PROTECT, sc->value, sc->code);
  result = s7_apply_function(sc, find_method(sc, let, sc->let_set_fallback_symbol), set_qlist_3(sc, let, symbol, value));
  unstack_gc_protect(sc);
  sc->code = T_Pos(stack_end_code(sc));
  sc->value = T_Ext(stack_end_args(sc));
  return(result);
}


s7_pointer let_ref(s7_scheme *sc, s7_pointer let, s7_pointer symbol)
{
  /* (let ((a 1)) ((curlet) 'a)) or ((rootlet) 'abs) */
  if (!is_let(let))
    {
      s7_pointer new_let;
      if (let == sc->unlet_disabled) return(initial_value(symbol));
      new_let = find_let(sc, let);
      if ((!is_let(new_let)) || (new_let == sc->rootlet))
	find_let_error_nr(sc, sc->let_ref_symbol, let, new_let, 1, set_mlist_2(sc, let, symbol));
      let = new_let;
    }
  if (!is_symbol(symbol))
    {
      if ((let != sc->rootlet) && (has_let_ref_fallback(let))) /* let-ref|set-fallback refer to (explicit) let-ref in various forms, not the method lookup process */
	return(call_let_ref_fallback(sc, let, symbol));
      wrong_type_error_nr(sc, sc->let_ref_symbol, 2, symbol, a_symbol_string);
    }
  /* a let-ref method is almost impossible to write without creating an infinite loop:
   *   any reference to the let will probably call let-ref somewhere, calling us again, and looping.
   *   This is not a problem in c-objects and funclets because c-object-ref and funclet-ref don't exist.
   *   After much wasted debugging, I decided to make let-ref and let-set! immutable.
   *   What about other let-as-first-arg funcs?
   */

  if (let_id(let) == symbol_id(symbol))
    return(local_value(symbol)); /* this has to follow the rootlet check(?) */

  if (is_keyword(symbol))
    symbol = keyword_symbol(symbol);
  if (let == sc->rootlet)
    return((is_slot(global_slot(symbol))) ? global_value(symbol) : sc->undefined);

  for (s7_pointer e = let; e; e = let_outlet(e))
    for (s7_pointer slot = let_slots(e); is_not_slot_end(slot); slot = next_slot(slot))
      if (slot_symbol(slot) == symbol)
	return(slot_value(slot));

  if (is_openlet(let))
    {
      /* If a let is a mock-hash-table (for example), implicit indexing of the hash-table collides with the same thing for the let (field names
       *   versus keys), and we can't just try again here because that makes it too easy to get into infinite recursion.  So, 'let-ref-fallback...
       */
      if (has_let_ref_fallback(let))
	return(call_let_ref_fallback(sc, let, symbol));
    }
  return((is_slot(global_slot(symbol))) ? global_value(symbol) : sc->undefined); /* (let () ((curlet) 'pi)) */
}


s7_pointer s7_let_ref(s7_scheme *sc, s7_pointer let, s7_pointer symbol) {return(let_ref(sc, let, symbol));}


s7_pointer g_let_ref(s7_scheme *sc, s7_pointer args)
{
  if (!is_pair(cdr(args)))
    error_nr(sc, sc->syntax_error_symbol,
	     set_elist_2(sc, wrap_string(sc, "let-ref: symbol missing: ~S", 27), set_ulist_1(sc, sc->let_ref_symbol, args)));
  return(let_ref(sc, car(args), cadr(args)));
}


s7_pointer slot_in_let(s7_scheme *sc, s7_pointer let, const s7_pointer sym)
{
  for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
    if (slot_symbol(slot) == sym)
      return(slot);
  return(sc->undefined);
}


s7_pointer let_ref_p_pp(s7_scheme *sc, s7_pointer let, s7_pointer sym)
{
  if (let_id(let) == symbol_id(sym))
    return(local_value(sym)); /* see add in tlet! */
  if (let == sc->rootlet) /* op_implicit_let_ref_c can pass rootlet */
    return((is_slot(global_slot(sym))) ? global_value(sym) : sc->undefined);
  for (s7_pointer e = let; e; e = let_outlet(e))
    for (s7_pointer slot = let_slots(e); is_not_slot_end(slot); slot = next_slot(slot))
      if (slot_symbol(slot) == sym)
	return(slot_value(slot));
  if (has_let_ref_fallback(let))
    return(call_let_ref_fallback(sc, let, sym));
  return((is_slot(global_slot(sym))) ? global_value(sym) : sc->undefined);
}


s7_pointer g_cdr_let_ref(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer let = car(args), sym = cadr(args);
  if (!is_let(let))
    wrong_type_error_nr(sc, sc->let_ref_symbol, 1, let, a_let_string);
  if (let_id(let) == symbol_id(sym))
    return(local_value(sym));
  if (let == sc->rootlet)
    return((is_slot(global_slot(sym))) ? global_value(sym) : sc->undefined);
  for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
    if (slot_symbol(slot) == sym)
      return(slot_value(slot));
  return(let_ref_p_pp(sc, let_outlet(let), sym));
}


s7_pointer g_starlet_ref(s7_scheme *sc, s7_pointer args) {return(starlet(sc, starlet_symbol_id(cadr(args))));}


s7_pointer g_rootlet_ref(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = cadr(args);
  return((is_slot(global_slot(sym))) ? global_value(sym) : sc->undefined);
}


s7_pointer let_ref_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr)
{
  const s7_pointer arg1 = cadr(expr), arg2 = caddr(expr);
  if ((is_quoted_symbol(sc, arg2)) && (!is_keyword(cadr(arg2))))
    {
      if (is_pair(arg1))
	{
	  if ((optimize_op(expr) == HOP_SAFE_C_opSq_C) && (car(arg1) == sc->cdr_symbol))
	    {
	      set_opt3_sym(cdr(expr), cadr(arg2));
	      return(sc->cdr_let_ref);
	    }
	  if (car(arg1) == sc->rootlet_symbol) return(sc->rootlet_ref);
	  if (car(arg1) == sc->curlet_symbol) return(sc->curlet_ref);
	  if (car(arg1) == sc->unlet_symbol)
	    {
	      set_fn_direct(arg1, g_unlet_disabled);
	      return(sc->unlet_ref);
	    }}
      if (arg1 == sc->starlet_symbol) return(sc->starlet_ref); /* should *curlet* be added? */
    }
  return(func);
}


/* ---------------------------------------- let-set! ---------------------------------------- */
s7_pointer let_set_1(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value)
{
  if (is_keyword(symbol))
    symbol = keyword_symbol(symbol);

  if (let == sc->rootlet)
    {
      s7_pointer slot;
      if (is_constant_symbol(sc, symbol))  /* (let-set! (rootlet) 'pi #f) */
	wrong_type_error_nr(sc, sc->let_set_symbol, 2, symbol, a_non_constant_symbol_string);
      /* it would be nice if safety>0 to add an error check for bad arity if a built-in method is set (set! (lt 'write) hash-table-set!),
       *   built_in being (initial_value_is_defined(sc, sym)), but this function is called a ton, and this error can't easily be
       *   checked by the optimizer (we see the names, but not the values, so bad arity check requires assumptions about those values).
       */
      slot = global_slot(symbol);
      if (!is_slot(slot))
	error_nr(sc, sc->wrong_type_arg_symbol,
		 set_elist_3(sc, wrap_string(sc, "let-set!: ~A is not defined in ~A", 33), symbol, let));
      if (is_syntax(slot_value(slot)))
	wrong_type_error_nr(sc, sc->let_set_symbol, 2, symbol, wrap_string(sc, "a non-syntactic symbol", 22));
      if (is_immutable(slot))
	immutable_object_error_nr(sc, set_elist_2(sc, wrap_string(sc, "~S is immutable in (rootlet)", 28), symbol)); /* also (set! (with-let...)...) */
      symbol_increment_ctr(symbol);
      slot_set_value(slot, (slot_has_setter(slot)) ? call_setter(sc, slot, value) : value);
      return(slot_value(slot));
    }
  if (is_unlet(let))
    immutable_object_error_nr(sc, set_elist_2(sc, wrap_string(sc, "~S is immutable in (unlet)", 26), symbol));
  if (let_id(let) == symbol_id(symbol))
   {
     s7_pointer slot = local_slot(symbol);
     if (is_slot(slot))
       {
	 symbol_increment_ctr(symbol);
	 return(checked_slot_set_value(sc, slot, value));
       }}
  for (s7_pointer e = let; e; e = let_outlet(e))
    for (s7_pointer slot = let_slots(e); is_not_slot_end(slot); slot = next_slot(slot))
      if (slot_symbol(slot) == symbol)
	{
	  symbol_increment_ctr(symbol);
	  return(checked_slot_set_value(sc, slot, value));
	}
  if (!has_let_set_fallback(let))
    error_nr(sc, sc->wrong_type_arg_symbol,
	     set_elist_3(sc, wrap_string(sc, "let-set!: ~A is not defined in ~A", 33), symbol, let));
  /* not sure about this -- what's the most useful choice? */
  return(call_let_set_fallback(sc, let, symbol, value));
}


s7_pointer let_set_2(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value)
{
  if (!is_let(let))
    {
      s7_pointer new_let = find_let(sc, let);
      if (!is_let(new_let))
	find_let_error_nr(sc, sc->let_set_symbol, let, new_let, 1, set_plist_3(sc, let, symbol, value));
      let = new_let;
    }
  if (!is_symbol(symbol))
    {
      if ((let != sc->rootlet) && (has_let_set_fallback(let)))
	return(call_let_set_fallback(sc, let, symbol, value));
      wrong_type_error_nr(sc, sc->let_set_symbol, 2, symbol, a_symbol_string);
    }
  /* currently let-set! is immutable, so we don't have to check for a let-set! method (so let_set! is always global) */
  return(let_set_1(sc, let, symbol, value));
}


s7_pointer s7_let_set(s7_scheme *sc, s7_pointer let, s7_pointer symbol, s7_pointer value) {return(let_set_2(sc, let, symbol, value));}


s7_pointer g_let_set(s7_scheme *sc, s7_pointer args)
{
  /* (let ((a 1)) (set! ((curlet) 'a) 32) a) */

  if (!is_pair(cdr(args))) /* (let ((a 123.0)) (define (f) (set! (let-ref) a)) (catch #t f (lambda args #f)) (f)) */
    error_nr(sc, sc->wrong_number_of_args_symbol,
	     set_elist_3(sc, wrap_string(sc, "~S: not enough arguments: ~S", 28), sc->let_set_symbol, sc->code));

  return(let_set_2(sc, car(args), cadr(args), caddr(args)));
}


s7_pointer let_set_p_ppp_2(s7_scheme *sc, s7_pointer let, s7_pointer sym, s7_pointer val)
{
  if (!is_symbol(sym))
    wrong_type_error_nr(sc, sc->let_set_symbol, 2, sym, a_symbol_string);
  return(let_set_1(sc, let, sym, val));
}


s7_pointer g_cdr_let_set(s7_scheme *sc, s7_pointer args)
{
  s7_pointer let = car(args);
  const s7_pointer sym = cadr(args), val = caddr(args);
  if (!is_let(let))
    {
      s7_pointer new_let = find_let(sc, let);
      if (!is_let(new_let))
	find_let_error_nr(sc, sc->let_set_symbol, let, new_let, 1, args);
      let = new_let;
    }
  if (let != sc->rootlet)
    {
      for (s7_pointer e = let; e; e = let_outlet(e))
	for (s7_pointer slot = let_slots(e); is_not_slot_end(slot); slot = next_slot(slot))
	  if (slot_symbol(slot) == sym)
	    {
	      slot_set_value(slot, (slot_has_setter(slot)) ? call_setter(sc, slot, val) : val);
	      return(slot_value(slot));
	    }
      if ((let != sc->rootlet) && (has_let_set_fallback(let)))
	return(call_let_set_fallback(sc, let, sym, val));
    }
  {
    s7_pointer slot = global_slot(sym);
    if (!is_slot(slot))
      error_nr(sc, sc->wrong_type_arg_symbol, set_elist_3(sc, wrap_string(sc, "let-set!: ~A is not defined in ~A", 33), sym, let));
    slot_set_value(slot, (slot_has_setter(slot)) ? call_setter(sc, slot, val) : val);
    return(slot_value(slot));
  }
}


s7_pointer g_starlet_set(s7_scheme *sc, s7_pointer args)
{
  s7_pointer sym = cadr(args);
  if (!is_symbol(sym)) /* (let () (define (func) (let-set! *s7* '(1 . 2) (hash-table))) (func) (func)) */
    error_nr(sc, sc->wrong_type_arg_symbol,
	     set_elist_3(sc, wrap_string(sc, "(let-set! *s7* ~A ...) second argument is ~A but should be a symbol", 67),
			 sym, object_type_name(sc, sym)));
  if (is_keyword(sym))
    sym = keyword_symbol(sym);
  if (starlet_symbol_id(sym) == sl_no_field)
    error_nr(sc, sc->out_of_range_symbol, set_elist_2(sc, wrap_string(sc, "can't set (*s7* '~S); no such field in *s7*", 43), sym));
  return(starlet_set_1(sc, sym, caddr(args)));
}


s7_pointer g_unlet_set(s7_scheme *sc, s7_pointer args)
{
  immutable_object_error_nr(sc, set_elist_2(sc, wrap_string(sc, "~S is immutable in (unlet)", 26), cadr(args)));
  return(sc->F);
}


s7_pointer let_set_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr)
{
  const s7_pointer arg1 = cadr(expr);
  if (optimize_op(expr) == HOP_SAFE_C_opSq_CS)
    {
      const s7_pointer arg2 = caddr(expr), arg3 = cadddr(expr);
      if ((car(arg1) == sc->cdr_symbol) &&
	  (is_quoted_symbol(sc, arg2)) &&
	  (!is_possibly_constant(cadr(arg2))) && /* assumes T_Sym */
	  (!is_possibly_constant(arg3)))
	return(sc->cdr_let_set);
      if (car(arg1) == sc->unlet_symbol)
	{
	  set_fn_direct(arg1, g_unlet_disabled);
	  return(sc->unlet_set);
	}}
  if (arg1 == sc->starlet_symbol) return(sc->starlet_set);
  return(func);
}


/* ---------------------------------------- let copy helpers ---------------------------------------- */
s7_pointer reverse_slots(s7_pointer let_slots)
{
  s7_pointer slot = let_slots, result = slot_end;
  while (is_not_slot_end(slot))
    {
      s7_pointer nextslot = next_slot(slot);
      slot_set_next(slot, result);
      result = slot;
      slot = nextslot;
    }
  return(result);
}


s7_pointer let_copy(s7_scheme *sc, s7_pointer let)
{
  s7_pointer new_let;
  if (T_Let(let) == sc->rootlet)   /* (copy (rootlet)) or (copy (funclet abs)) etc */
    return(sc->rootlet);
  /* we can't make copy handle lets-as-objects specially because the make-object function in define-class uses copy to make a new object!
   *   So if it is present, we get it here, and then there's almost surely trouble.
   */
  new_let = make_let(sc, let_outlet(let));
  set_all_methods(new_let, let);
  begin_temp(sc->x, new_let);
  if (is_not_slot_end(let_slots(let)))
    {
      const s7_int id = let_id(new_let);
      for (s7_pointer last_slot = NULL, slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
	{
	  s7_pointer new_slot;
	  new_cell(sc, new_slot, T_SLOT);
	  slot_set_symbol_and_value(new_slot, slot_symbol(slot), slot_value(slot));
	  if (symbol_id(slot_symbol(new_slot)) != id) /* keep shadowing intact */
	    symbol_set_local_slot(slot_symbol(slot), id, new_slot);
	  if (slot_has_setter(slot))
	    {
	      slot_set_setter(new_slot, slot_setter(slot));
	      slot_set_has_setter(new_slot);
	    }
	  if (last_slot)
	    slot_set_next(last_slot, new_slot);
	  else let_set_slots(new_let, new_slot);
	  slot_set_next(new_slot, slot_end);        /* in case GC runs during this loop */
	  last_slot = new_slot;
	}}
  /* We can't do a (normal) loop here then reverse the slots later because the symbol's local_slot has to
   *    match the unshadowed slot, not the last in the list:
   *    (let ((e1 (inlet 'a 1 'a 2))) (let ((e2 (copy e1))) (list (equal? e1 e2) (equal? (e1 'a) (e2 'a)))))
   */
  end_temp(sc->x);
  return(new_let);
}


s7_pointer s7_rootlet(s7_scheme *sc) {return(sc->rootlet);}


s7_pointer s7_shadow_rootlet(s7_scheme *sc) {return(sc->shadow_rootlet);}


s7_pointer s7_set_shadow_rootlet(s7_scheme *sc, s7_pointer let)
{
  s7_pointer old_let = sc->shadow_rootlet;
  sc->shadow_rootlet = let;
  return(old_let); /* like s7_set_curlet below */
}


s7_pointer s7_curlet(s7_scheme *sc) /* see also fx_curlet */
{
  sc->capture_let_counter++;
  return(sc->curlet);
}


void update_symbol_ids(s7_scheme *sc, s7_pointer let)
{
  for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
    {
      s7_pointer sym = slot_symbol(slot);
      if (symbol_id(sym) != sc->let_number)
	symbol_set_local_slot_unincremented(sym, sc->let_number, slot);
    }
}


s7_pointer s7_set_curlet(s7_scheme *sc, s7_pointer let)
{
  const s7_pointer old_let = sc->curlet;
  if (is_let(let))
    {
      set_curlet(sc, let);
      if (let_id(let) > 0)
	{
	  let_set_id(let, ++sc->let_number);
	  update_symbol_ids(sc, let);
	}}
  return(old_let);
}


s7_pointer s7_outlet(s7_scheme *sc, s7_pointer let) {return(let_outlet(let));}


s7_pointer outlet_p_p(s7_scheme *sc, s7_pointer let)
{
  if (!is_let(let))
    {
      s7_pointer new_let = find_let(sc, let);
      if (!is_let(new_let))
	find_let_error_nr(sc, sc->outlet_symbol, let, new_let, 1, set_mlist_1(sc, let));
      let = new_let;
    }
  return((let == sc->rootlet) ? sc->rootlet : let_outlet(let)); /* rootlet check is needed(!) */
}


s7_pointer s7i_outlet_p_p(s7_scheme *sc, s7_pointer let) {return(outlet_p_p(sc, let));}


s7_pointer outlet_chooser(s7_scheme *sc, s7_pointer func, int32_t num_args, s7_pointer expr)
{
  if ((num_args == 1) && (is_pair(cadr(expr))) && (caadr(expr) == sc->unlet_symbol))
    {
      set_fn_direct(cadr(expr), g_unlet_disabled);
      return(sc->outlet_unlet);
    }
  return(func);
}


s7_pointer g_set_outlet(s7_scheme *sc, s7_pointer args)
{
  /* (let ((a 1)) (let ((b 2)) (set! (outlet (curlet)) (rootlet)) ((curlet) 'a))) */
  s7_pointer let = car(args), new_outer;

  if (!is_let(let))
    {
      s7_pointer new_let = find_let(sc, let);
      if (!is_let(new_let))
	find_let_error_nr(sc, wrap_string(sc, "set! outlet", 11), let, new_let, 1, args);
      let = new_let;
    }
  if (let == sc->starlet)
    error_nr(sc, sc->out_of_range_symbol, set_elist_1(sc, wrap_string(sc, "can't set! (outlet *s7*)", 24)));
  if (is_immutable_let(let))
    immutable_object_error_nr(sc, set_elist_4(sc, wrap_string(sc, "can't (set! (outlet ~S) ~S), ~S is immutable", 44), let, cadr(args), let));
  new_outer = cadr(args);
  if (!is_let(new_outer))
    {
      s7_pointer new_let = find_let(sc, new_outer);
      if (!is_let(new_let))
	find_let_error_nr(sc, wrap_string(sc, "set! outlet", 11), new_outer, new_let, 2, args);
      new_outer = new_let;
    }
  if (let != sc->rootlet)
    {
      /* here it's possible to get cyclic let chains; maybe do this check only if safety>0 */
      for (s7_pointer new_let = new_outer; new_let; new_let = let_outlet(new_let))
	if (let == new_let)
	  error_nr(sc, make_symbol(sc, "cyclic-let", 10),
		   set_elist_2(sc, wrap_string(sc, "set! (outlet ~A) creates a cyclic let chain", 43), let));
      let_set_outlet(let, new_outer);
    }
  return(new_outer);
}


/* ---------------------------------------- symbol->value ---------------------------------------- */
s7_pointer g_symbol_to_value(s7_scheme *sc, s7_pointer args)
{

  const s7_pointer sym = car(args);
  if (!is_symbol(sym))
    return(method_or_bust(sc, sym, sc->symbol_to_value_symbol, args, sc->type_names[T_SYMBOL], 1));
  if (is_keyword(sym))
    {
      if ((is_pair(cdr(args))) && (!is_let(cadr(args))) && (!is_let(find_let(sc, cadr(args)))))
	wrong_type_error_nr(sc, sc->symbol_to_value_symbol, 2, cadr(args), sc->type_names[T_LET]);
      return(sym);
    }
  if (is_pair(cdr(args)))
    {
      s7_pointer local_let = cadr(args);
      if (!is_let(local_let))
	{
	  local_let = find_let(sc, local_let);
	  if (!is_let(local_let))
	    return(method_or_bust(sc, cadr(args), sc->symbol_to_value_symbol, args, a_let_string, 2)); /* not local_let */
	}
      if (local_let == sc->rootlet) return((is_slot(global_slot(sym))) ? global_value(sym) : sc->undefined);
      if (is_unlet(local_let)) return(initial_value(sym));
      if (local_let == sc->starlet) return(starlet(sc, starlet_symbol_id(sym)));
      return(s7_symbol_local_value(sc, sym, local_let));
    }
  if (is_defined_global(sym))
    return(global_value(sym));
  return(s7_symbol_value(sc, sym));
}


s7_pointer symbol_to_value_chooser(s7_scheme *sc, s7_pointer func, int32_t unused_args, s7_pointer expr)
{
  s7_pointer arg1 = cadr(expr), arg2 = (is_pair(cddr(expr))) ? caddr(expr) : sc->F;
  if ((is_quoted_symbol(sc, arg1)) && (!is_keyword(cadr(arg1))) && (is_pair(arg2)) && (car(arg2) == sc->unlet_symbol)) /* old-style (obsolete) unlet as third arg(!) */
    {
      set_fn_direct(arg2, g_unlet_disabled);
      return(sc->sv_unlet_ref);
    }
  return(func);
}


/* ---------------------------------------- symbol->dynamic-value ---------------------------------------- */
static s7_pointer find_dynamic_value(s7_scheme *sc, s7_pointer let, s7_pointer sym, s7_int *id)
{
  for (; let_id(let) > symbol_id(sym); let = let_outlet(let));
  if (let_id(let) == symbol_id(sym))
    {
      (*id) = let_id(let);
      return(local_value(sym));
    }
  for (; (let) && (let_id(let) > (*id)); let = let_outlet(let))
    for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
      if (slot_symbol(slot) == sym)
	{
	  (*id) = let_id(let);
	  return(slot_value(slot));
	}
  return(sc->unused);
}


s7_pointer g_symbol_to_dynamic_value(s7_scheme *sc, s7_pointer args)
{

  const s7_pointer sym = car(args);
  s7_pointer val;
  s7_int top_id = -1;

  if (!is_symbol(sym))
    return(method_or_bust(sc, sym, sc->symbol_to_dynamic_value_symbol, args, sc->type_names[T_SYMBOL], 1));

  if (is_defined_global(sym))
    return(global_value(sym));

  if (let_id(sc->curlet) == symbol_id(sym))
    return(local_value(sym));

  val = find_dynamic_value(sc, sc->curlet, sym, &top_id);
  if (top_id == symbol_id(sym))
    return(val);

  for (s7_int op_loc = stack_top(sc) - 1; op_loc > 0; op_loc -= 4)
    if (is_let_unchecked(stack_let(sc->stack, op_loc))) /* OP_GC_PROTECT let slot can be anything (even free) */
      {
	s7_pointer cur_val = find_dynamic_value(sc, stack_let(sc->stack, op_loc), sym, &top_id);
	if (cur_val != sc->unused)
	  val = cur_val;
	if (top_id == symbol_id(sym))
	  return(val);
      }
  /* what about call/cc stacks? */
  return((val == sc->unused) ? s7_symbol_value(sc, sym) : val);
}


/* ---------------------------------------- defined? ---------------------------------------- */
s7_pointer g_is_defined(s7_scheme *sc, s7_pointer args)
{
  /* if the symbol has a global slot and e is unset or rootlet, this returns #t */

  s7_pointer sym = car(args);
  if (!is_symbol(sym))
    return(method_or_bust(sc, sym, sc->is_defined_symbol, args, sc->type_names[T_SYMBOL], 1));

  if (is_pair(cdr(args)))
    {
      s7_pointer let = cadr(args);
      const s7_pointer ignore_globals = (is_pair(cddr(args))) ? caddr(args) : sc->F;
      if (!is_let(let))
	{
	  const s7_pointer new_let = find_let(sc, let);  /* returns () if none */
	  if (!is_let(new_let))
	    find_let_error_nr(sc, sc->is_defined_symbol, let, new_let, 2, args);
	  if ((new_let == sc->rootlet) && (is_pair(cddr(args))) && (ignore_globals != sc->F))
	    {
	      if (ignore_globals != sc->T) /* signature claims this should be a boolean */
		return(method_or_bust(sc, ignore_globals, sc->is_defined_symbol, args, a_boolean_string, 3));
	      return(sc->F);
	    }
	  let = new_let;
	}
      /* if (is_unlet(let)) return(make_boolean(sc, initial_value_is_defined(sc, sym))); */
      /* this ^ is wrong: (with-let (unlet) (define xx 1) (list (defined? 'xx) (defined? 'xx (curlet)))) should be (#t #t) */

      if (is_keyword(sym))                       /* if no "let", is global -> #t */
	{                                        /* we're treating :x as 'x outside rootlet, but consider all keywords defined (as themselves) in rootlet? */
	  if (let == sc->rootlet) return(sc->T); /* (defined? x (rootlet)) where x value is a keyword */
	  sym = keyword_symbol(sym);             /* (defined? :print-length *s7*) */
	}
      if (let == sc->starlet)
	return(make_boolean(sc, starlet_symbol_id(sym) != sl_no_field));
      if (!is_boolean(ignore_globals))
	return(method_or_bust(sc, ignore_globals, sc->is_defined_symbol, args, a_boolean_string, 3));
      if (let == sc->rootlet) /* we checked (let? let) above */
	{
	  if (ignore_globals == sc->F)
	    return(make_boolean(sc, is_slot(global_slot(sym)))); /* new_symbol and gensym initialize global_slot to #<undefined> */
	  return(sc->F);
	}
      if (is_slot(symbol_to_local_slot(sc, sym, T_Let(let)))) return(sc->T);
      return((ignore_globals == sc->T) ? sc->F : make_boolean(sc, is_slot(global_slot(sym))));
    }
  return((is_defined_global(sym)) ? sc->T : make_boolean(sc, is_bound_symbol(sc, sym)));
}


/* ---------------------------------------- funclet ---------------------------------------- */
s7_pointer g_funclet(s7_scheme *sc, s7_pointer args)
{
  s7_pointer func = car(args);
  if (is_symbol(func))
    {
      if ((func = s7_symbol_value(sc, func)) == sc->undefined)
	error_nr(sc, sc->wrong_type_arg_symbol,
		 set_elist_2(sc, wrap_string(sc, "funclet argument, '~S, is unbound", 33), car(args))); /* not func here */
    }
  if_method_exists_return_value(sc, func, sc->funclet_symbol, args);
  if (!((is_any_procedure(func)) || (is_c_object(func))))
    sole_arg_wrong_type_error_nr(sc, sc->funclet_symbol, func, a_procedure_or_a_macro_string);
  return(find_let(sc, func));
}


/* ---------------------------------------- owlet ---------------------------------------- */
s7_pointer g_owlet(s7_scheme *sc, s7_pointer args)
{
  /* if owlet is not copied, (define e (owlet)), e changes as owlet does! */

  s7_pointer let;
  const bool old_gc = sc->gc_off;
  if (is_pair(args))
    error_nr(sc, sc->wrong_number_of_args_symbol, set_elist_3(sc, too_many_arguments_string, sc->owlet_symbol, args));
#if WITH_HISTORY
  slot_set_value(sc->error_history, sanitize_history(sc, slot_value(sc->error_history)));
#endif
  let = let_copy(sc, sc->owlet);
  gc_protect_via_stack(sc, let);

  /* make sure the pairs/reals/strings/integers are copied: should be error-data, error-code, and error-history */
  sc->gc_off = true;
  for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
    if (is_pair(slot_value(slot)))
      {
	const s7_pointer new_list = copy_any_list(sc, slot_value(slot));
	slot_set_value(slot, new_list);
	for (s7_pointer p = new_list, sp = p; is_pair(p); p = cdr(p), sp = cdr(sp))
	  {
	    s7_pointer val = car(p);
	    if (is_t_real(val))
	      set_car(p, make_real(sc, real(val)));
	    else
	      if (is_string(val))
		set_car(p, make_string_with_length(sc, string_value(val), string_length(val)));
	      else
		if (is_t_integer(val))
		  set_car(p, make_integer(sc, integer(val)));
	    p = cdr(p);
	    if ((!is_pair(p)) || (p == sp)) break;
	    val = car(p);
	    if (is_t_real(val))
	      set_car(p, make_real(sc, real(val)));
	    else
	      if (is_string(val))
		set_car(p, make_string_with_length(sc, string_value(val), string_length(val)));
	  }}
  sc->gc_off = old_gc;
  unstack_gc_protect(sc);
  return(let);
}
