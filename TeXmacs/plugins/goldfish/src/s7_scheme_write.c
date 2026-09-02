/* s7_scheme_write.c - write function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_internal.h"
#include "s7_scheme_write.h"
#include "s7_scheme_let.h"
#include "s7_continuation.h"

#define IF_METHOD_EXISTS_RETURN_VALUE(Sc, Obj, Method_name, Args) \
  do { \
    if (s7i_has_active_methods(Sc, Obj)) { \
      s7_pointer _func_ = s7i_find_method_with_let(Sc, Obj, s7_make_symbol(Sc, Method_name)); \
      if (_func_ != s7_undefined(Sc)) \
        return s7_apply_function(Sc, _func_, Args); \
    } \
  } while (0)

/* -------- cycles -------- */

#define INITIAL_SHARED_INFO_SIZE 8

int32_t shared_ref(shared_info_t *ci, const s7_pointer p)
{
  /* from print after collecting refs, not called by equality check, only called in object_to_port_with_circle_check_1 */
  s7_pointer *objs = ci->objs;
  for (int32_t i = 0; i < ci->top; i++)
    if (objs[i] == p)
      {
	int32_t val = ci->refs[i];
	if (val > 0)
	  ci->refs[i] = -ci->refs[i];
	return(val);
      }
  return(0);
}

void flip_ref(shared_info_t *ci, const s7_pointer p)
{
  s7_pointer *objs = ci->objs;
  for (int32_t i = 0; i < ci->top; i++)
    if (objs[i] == p)
      {
	ci->refs[i] = -ci->refs[i];
	break;
      }
}

int32_t peek_shared_ref_1(shared_info_t *ci, const s7_pointer p)
{
  /* returns 0 if not found, otherwise the ref value for p */
  s7_pointer *objs = ci->objs;
  for (int32_t i = 0; i < ci->top; i++)
    if (objs[i] == p)
      return(ci->refs[i]);
  return(0);
}

int32_t peek_shared_ref(shared_info_t *ci, s7_pointer p)
{
  /* returns 0 if not found, otherwise the ref value for p */
  return((is_collected_unchecked(p)) ? peek_shared_ref_1(ci, p) : 0);
}

void enlarge_shared_info(shared_info_t *ci)
{
  ci->size *= 2;
  ci->size2 = ci->size - 2;
  ci->objs = (s7_pointer *)Realloc(ci->objs, ci->size * sizeof(s7_pointer));
  ci->refs = (int32_t *)Realloc(ci->refs, ci->size * sizeof(int32_t));
  ci->defined = (bool *)Realloc(ci->defined, ci->size * sizeof(bool));
  /* this clearing is needed, memclr is not faster */
  for (int32_t i = ci->top; i < ci->size; i++)
    {
      ci->refs[i] = 0;
      ci->objs[i] = NULL;
    }
}

static bool check_collected(s7_pointer top, shared_info_t *ci)
{
  const s7_pointer *objs_end = (s7_pointer *)(ci->objs + ci->top);
  for (s7_pointer *p = ci->objs; p < objs_end; p++)
    if ((*p) == top)
      {
	int32_t i = (int32_t)(p - ci->objs);
	if (ci->refs[i] == 0)
	  {
	    ci->has_hits = true;
	    ci->refs[i] = ++ci->ref;  /* if found, set the ref number */
	  }
	break;
      }
  set_cyclic(top);
  return(true);
}


static bool collect_vector_info(s7_scheme *sc, shared_info_t *ci, s7_pointer top, bool stop_at_print_length)
{
  s7_int plen;
  bool cyclic = false;

  if (stop_at_print_length)
    {
      plen = sc->print_length;
      if (plen > vector_length(top))
	plen = vector_length(top);
    }
  else plen = vector_length(top);
  for (s7_int i = 0; i < plen; i++)
    {
      const s7_pointer vel = vector_element_unchecked(top, i);   /* "unchecked" because top might be rootlet, I think */
      if ((has_structure(vel)) &&
	  (collect_shared_info(sc, ci, vel, stop_at_print_length)))
	{
	  set_cyclic(vel);
	  cyclic = true;
	  if ((is_c_pointer(vel)) ||
	      (is_iterator(vel)) ||
	      (is_c_object(vel)))
	    check_collected(top, ci);
	}}
  if (cyclic) set_cyclic(top);
  return(cyclic);
}

bool collect_shared_info(s7_scheme *sc, shared_info_t *ci, s7_pointer top, bool stop_at_print_length)
{
  /* look for top in current list.
   * As we collect objects (guaranteed to have structure) we set the collected bit.  If we ever
   *   encounter an object with that bit on, we've seen it before so we have a possible cycle.
   *   Once the collection pass is done, we run through our list, and clear all these bits.
   */
  bool top_cyclic;

  if (is_collected_or_shared(top))
    return((!is_shared(top)) && (check_collected(top, ci)));

  /* top not seen before -- add it to the list */
  set_collected(top);
  if (ci->top == ci->size)
    enlarge_shared_info(ci);
  ci->objs[ci->top++] = top;

  top_cyclic = false;
  /* now search the rest of this structure */
  if (is_pair(top))
    {
      s7_pointer p;
      if ((has_structure(car(top))) &&
	  (collect_shared_info(sc, ci, car(top), stop_at_print_length)))
	top_cyclic = true;

      for (p = cdr(top); is_pair(p); p = cdr(p))
	{
	  if (is_collected_or_shared(p))
	    {
	      set_cyclic(top);
	      set_cyclic(p);
	      if (!is_shared(p))
		return(check_collected(p, ci));
	      if (!top_cyclic)
		for (s7_pointer cp = top; cp != p; cp = cdr(cp)) set_shared(cp);
	      return(top_cyclic);
	    }
 	  set_collected(p);
	  if (ci->top == ci->size)
	    enlarge_shared_info(ci);
	  ci->objs[ci->top++] = p;
	  if ((has_structure(car(p))) &&
	      (collect_shared_info(sc, ci, car(p), stop_at_print_length)))
	    top_cyclic = true;
	}
      if ((has_structure(p)) &&
	  (collect_shared_info(sc, ci, p, stop_at_print_length)))
	{
	  set_cyclic(top);
	  return(true);
	}
      if (!top_cyclic)
	for (s7_pointer cp = top; is_pair(cp); cp = cdr(cp)) set_shared(cp);
      else set_cyclic(top);
      return(top_cyclic);
    }
  switch (type(top))
    {
    case T_VECTOR:
      if (collect_vector_info(sc, ci, top, stop_at_print_length))
	top_cyclic = true;
      break;

    case T_ITERATOR:
      if ((is_sequence(iterator_sequence(top))) && /* might be a function with +iterator+ local */
	  (collect_shared_info(sc, ci, iterator_sequence(top), stop_at_print_length)))
	{
	  if (peek_shared_ref(ci, iterator_sequence(top)) == 0)
	    check_collected(iterator_sequence(top), ci);
	  top_cyclic = true;
	}
      break;

    case T_HASH_TABLE:
      if (hash_table_entries(top) > 0)
	{
	  const s7_int len = (s7_int)hash_table_size(top);
	  hash_entry_t **entries = hash_table_elements(top);
	  const bool keys_safe = hash_keys_not_cyclic(sc, top);
	  for (s7_int i = 0; i < len; i++)
	    for (hash_entry_t *entry = entries[i]; entry; entry = hash_entry_next(entry))
	      {
		if ((!keys_safe) &&
		    (has_structure(hash_entry_key(entry))) &&
		    (collect_shared_info(sc, ci, hash_entry_key(entry), stop_at_print_length)))
		  top_cyclic = true;
		if ((has_structure(hash_entry_value(entry))) &&
		    (collect_shared_info(sc, ci, hash_entry_value(entry), stop_at_print_length)))
		  {
		    if ((is_c_pointer(hash_entry_value(entry))) ||
			(is_iterator(hash_entry_value(entry))) ||
			(is_c_object(hash_entry_value(entry))))
		      check_collected(top, ci);
		    top_cyclic = true;
		  }}}
      break;

    case T_SLOT: /* this can be hit if we somehow collect_shared_info on sc->rootlet via collect_vector_info (see the let case below) */
      if ((has_structure(slot_value(top))) &&
	  (collect_shared_info(sc, ci, slot_value(top), stop_at_print_length)))
	top_cyclic = true;
      break;

    case T_LET:
      if (top == sc->rootlet)
	{
	  if (collect_vector_info(sc, ci, top, stop_at_print_length))
	    top_cyclic = true;
	}
      else
	for (s7_pointer let = top; let; let = let_outlet(let))
	  for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
	    if ((has_structure(slot_value(slot))) &&
		(collect_shared_info(sc, ci, slot_value(slot), stop_at_print_length)))
	      {
		top_cyclic = true;
		if ((is_c_pointer(slot_value(slot))) ||
		    (is_iterator(slot_value(slot))) ||
		    (is_c_object(slot_value(slot))))
		  check_collected(top, ci);
	      }
      break;

    case T_CLOSURE: case T_CLOSURE_STAR:
      if (collect_shared_info(sc, ci, closure_body(top), stop_at_print_length))
	{
	  if (peek_shared_ref(ci, top) == 0)
	    check_collected(top, ci);
	  top_cyclic = true;
	}
      break;

    case T_C_POINTER:
      if ((has_structure(c_pointer_type(top))) &&
	  (collect_shared_info(sc, ci, c_pointer_type(top), stop_at_print_length)))
	{
	  if (peek_shared_ref(ci, c_pointer_type(top)) == 0)
	    check_collected(c_pointer_type(top), ci);
	  top_cyclic = true;
	}
      if ((has_structure(c_pointer_info(top))) &&
	  (collect_shared_info(sc, ci, c_pointer_info(top), stop_at_print_length)))
	{
	  if (peek_shared_ref(ci, c_pointer_info(top)) == 0)
	    check_collected(c_pointer_info(top), ci);
	  top_cyclic = true;
	}
      break;

    case T_C_OBJECT:
      if ((c_object_to_list(sc, top)) &&
	  (c_object_set(sc, top)) &&
	  (collect_shared_info(sc, ci, (*(c_object_to_list(sc, top)))(sc, set_plist_1(sc, top)), stop_at_print_length)))
	{
	  if (peek_shared_ref(ci, top) == 0)
	    check_collected(top, ci);
	  top_cyclic = true;
	}
      break;
    }
  if (!top_cyclic)
    set_shared(top);
  else set_cyclic(top);
  return(top_cyclic);
}

shared_info_t *make_shared_info(s7_scheme *sc)
{
  shared_info_t *ci = (shared_info_t *)Calloc(1, sizeof(shared_info_t));
  ci->size = INITIAL_SHARED_INFO_SIZE;
  ci->size2 = ci->size - 2;
  ci->objs = (s7_pointer *)Malloc(ci->size * sizeof(s7_pointer));
  ci->refs = (int32_t *)Calloc(ci->size, sizeof(int32_t));   /* finder expects 0 = unseen previously */
  ci->defined = (bool *)Calloc(ci->size, sizeof(bool));
  ci->cycle_port = sc->F;
  ci->init_port = sc->F;
  return(ci);
}

void free_shared_info(shared_info_t *ci)
{
  if (ci)
    {
      free(ci->objs);
      free(ci->refs);
      free(ci->defined);
      free(ci);
    }
}

shared_info_t *clear_shared_info(shared_info_t *ci)
{
  if (ci->top > 0)
    {
      memclr((void *)(ci->refs), ci->top * sizeof(int32_t));
      memclr((void *)(ci->defined), ci->top * sizeof(bool));
      for (int32_t i = 0; i < ci->top; i++)
	clear_cyclic_bits(ci->objs[i]); /* LOOP_4 is not faster */
      ci->top = 0;
    }
  ci->ref = 0;
  ci->has_hits = false;
  ci->ctr = 0;
  return(ci);
}

shared_info_t *load_shared_info(s7_scheme *sc, s7_pointer top, bool stop_at_print_length, shared_info_t *ci)
{
  /* for the printer, here only if is_structure(top) and top is not sc->rootlet */
  bool no_problem = true;
  s7_int stop_len;

  /* check for simple cases first */
  if (is_pair(top))
    {
      s7_pointer p = top;
      if (stop_at_print_length)
	{
	  s7_pointer slow = top;
	  stop_len = sc->print_length;
	  for (s7_int k = 0; k < stop_len; k += 2)
	    {
	      if (!is_pair(p)) break;
	      if (has_structure(car(p))) {no_problem = false; break;}
	      p = cdr(p);
	      if (!is_pair(p)) break;
	      if (has_structure(car(p))) {no_problem = false; break;}
	      p = cdr(p);
	      slow = cdr(slow);
	      if (p == slow) {no_problem = false; break;}
	    }}
      else
	if (s7_list_length(sc, top) == 0) /* it is circular at the top level (following cdr) */
	  no_problem = false;
	else
	  for (; is_pair(p); p = cdr(p))
	    if (has_structure(car(p))) {no_problem = false; break;} /* perhaps (and (length > 0 via sequence_is_empty)) or vector typer etc */
      if ((no_problem) &&
	  (!is_null(p)) && (has_structure(p)))
	no_problem = false;
      if (no_problem) return(NULL);
    }
  else
    if (is_t_vector(top)) /* any other vector can't happen */
      {
	stop_len = vector_length(top);
	if ((stop_at_print_length) &&
	    (stop_len > sc->print_length))
	  stop_len = sc->print_length;
	for (s7_int k = 0; k < stop_len; k++)
	  if (has_structure(vector_element(top, k))) {no_problem = false; break;}
	if (no_problem) return(NULL);
      }

    else /* added these 19-Oct-22 -- helps in tgc, but not much elsewhere */
      if ((is_let(top)) && (top != sc->rootlet))
	{
	  for (s7_pointer let = top; (no_problem) && (let); let = let_outlet(let))
	    for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
	      if (has_structure(slot_value(slot))) /* slot_symbol need not be checked? */
		{no_problem = false; break;}
	  if (no_problem) return(NULL);
	}
      else
	if (is_hash_table(top))
	  {
	    hash_entry_t **entries = hash_table_elements(top);
	    bool keys_safe = hash_keys_not_cyclic(sc, top);
	    if (hash_table_entries(top) == 0) return(NULL);
	    for (s7_int len = (s7_int)hash_table_size(top), i = 0; i < len; i++)
	      for (hash_entry_t *entry = entries[i]; entry; entry = hash_entry_next(entry))
		if (((!keys_safe) && (has_structure(hash_entry_key(entry)))) || (has_structure(hash_entry_value(entry))))
		  {no_problem = false; break;}
	    if (no_problem) return(NULL);
	  }

  if ((S7_DEBUGGING) && (is_any_vector(top)) && (!is_t_vector(top))) fprintf(stderr, "%s[%d]: got abnormal vector\n", __func__, __LINE__);
  clear_shared_info(ci);
  {
    /* collect all pointers associated with top */
    const bool cyclic = collect_shared_info(sc, ci, top, stop_at_print_length);
    s7_pointer *ci_objs = ci->objs;
    int32_t *ci_refs = ci->refs;
    int32_t refs = 0;

    for (int32_t i = 0; i < ci->top; i++)
      clear_collected_and_shared(ci_objs[i]);
    if (!cyclic)
      return(NULL);
    if (!(ci->has_hits))
      return(NULL);

    /* find if any were referenced twice (once for just being there, so twice=shared)
     *   we know there's at least one such reference because has_hits is true.
     */
    for (int32_t i = 0; i < ci->top; i++)
      if (ci_refs[i] > 0)
	{
	  set_collected(ci_objs[i]);
	  if (i == refs)
	    refs++;
	  else
	    {
	      ci_objs[refs] = ci_objs[i];
	      ci_refs[refs++] = ci_refs[i];
	      ci_refs[i] = 0;
	      ci_objs[i] = NULL;
	    }}
    ci->top = refs;
    return(ci);
  }
}


/* -------------------------------- cyclic-sequences -------------------------------- */
s7_pointer cyclic_sequences_p_p(s7_scheme *sc, s7_pointer obj)
{
  if (has_structure(obj))
    {
      shared_info_t *ci = (sc->object_out_locked) ? sc->circle_info : load_shared_info(sc, obj, false, sc->circle_info); /* false=don't stop at print length (vectors etc) */
      if (ci)
	{
	  s7i_check_free_heap_size(sc, ci->top);
	  begin_temp(sc->y, sc->nil);
	  for (int32_t i = 0; i < ci->top; i++)
	    sc->y = cons_unchecked(sc, ci->objs[i], sc->y);
	  return_with_end_temp(sc->y);
	}}
  return(sc->nil);
}


/* -------------------------------- object->port (display format etc) -------------------------------- */
int32_t circular_list_entries(s7_pointer lst)
{
  int32_t i = 1;
  for (s7_pointer x = cdr(lst); ; i++, x = cdr(x))
    {
      int32_t j = 0;
      for (s7_pointer y = lst; j < i; y = cdr(y), j++)
	if (x == y)
	  return(i);
    }
}

static void object_to_port_with_circle_check_1(s7_scheme *sc, s7_pointer vr, s7_pointer port, use_write_t use_write, shared_info_t *ci);
#define object_to_port_with_circle_check(Sc, Vr, Port, Use_Write, Ci) \
  do {								      \
    s7_pointer _V_ = Vr;						\
    if ((Ci) && (has_structure(_V_)))					\
      object_to_port_with_circle_check_1(Sc, _V_, Port, Use_Write, Ci); \
    else object_to_port(Sc, _V_, Port, Use_Write, Ci);			\
  } while (0)

static void (*display_functions[256])(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *ci);
#define object_to_port(Sc, Obj, Port, Use_Write, Ci) (*display_functions[type_unchecked(Obj)])(Sc, Obj, Port, Use_Write, Ci)

static bool string_needs_slashification(const uint8_t *str, s7_int len)
{
  /* we have to go by len (str len) not *s==0 because s7 strings can have embedded nulls */
  for (const uint8_t *p = str, *pend = (const uint8_t *)(str + len); p < pend; p++)
    if (slashify_table[*p])
      return(true);
  return(false);
}

#define IN_QUOTES true
#define NOT_IN_QUOTES false

static void slashify_string_to_port(s7_scheme *sc, s7_pointer port, const char *p, s7_int len, bool quoted)
{
  const uint8_t *pcur, *pend, *pstart = NULL;
  if (len == 0)
    {
      if (quoted)
	port_write_string(port)(sc, "\"\"", 2, port);
      return;
    }
  pend = (const uint8_t *)(p + len);

  /* what about the trailing nulls? Guile writes them out (as does s7 currently)
   *    but that is not ideal.  I'd like to use ~S for error messages, so that
   *    strings are clearly identified via the double-quotes, but this way of
   *    writing them is ugly:
   *      (let ((str (make-string 8 #\null))) (set! (str 0) #\a) str) -> "a\x00\x00\x00\x00\x00\x00\x00"
   *    but it would be misleading to omit them because:
   *      (let ((str (make-string 8 #\null))) (set! (str 0) #\a) (string-append str "bc")) -> "a\x00\x00\x00\x00\x00\x00\x00bc"
   * also it is problematic to use sc->print_length here (rather than a separate string-print-length) because
   *    it is normally (say) 12 which truncates just about every string.  In CL, *print-length*
   *    does not affect strings, symbols, or bit-vectors.  But if the string is enormous,
   *    this function can bring us to a complete halt.  string-print-length (as a *s7* field) is
   *    also problematic -- it does not behave as expected in many cases if it is limited to this
   *    function and string_to_port below, and if set too low, disables the repl.
   */
  if (quoted) port_write_character(port)(sc, '"', port);
  for (pcur = (const uint8_t *)p; pcur < pend; pcur++)
    if (slashify_table[*pcur])
      {
	if (pstart) pstart++; else pstart = (const uint8_t *)p;
	if (pstart != pcur)
	  {
	    port_write_string(port)(sc, (const char *)pstart, pcur - pstart, port);
	    pstart = pcur;
	  }
	port_write_character(port)(sc, '\\', port);
	switch (*pcur)
	  {
	  case '"':   port_write_character(port)(sc, '"', port);   break;
	  case '\\':  port_write_character(port)(sc, '\\', port);  break;
	  case '\'':  port_write_character(port)(sc, '\'', port);  break;
	  case '\t':  port_write_character(port)(sc, 't', port);   break;
	  case '\r':  port_write_character(port)(sc, 'r', port);   break;
          case '\n':  port_write_character(port)(sc, 'n', port);   break; /* added 17-Sep-25 for r7rs */
	  case '\b':  port_write_character(port)(sc, 'b', port);   break;
	  case '\f':  port_write_character(port)(sc, 'f', port);   break;
	  case '\?':  port_write_character(port)(sc, '?', port);   break;
	  case 'x':   port_write_character(port)(sc, 'x', port);   break;
	  default:
	    {
	      char buf[5];
	      s7_int n = (s7_int)(*pcur);
	      buf[0] = 'x';
	      buf[1] = (n < 16) ? '0' : dignum[(n / 16) % 16];
	      buf[2] = dignum[n % 16];
	      buf[3] = ';';
	      buf[4] = '\0';
	      port_write_string(port)(sc, buf, 4, port);
	    }
	    break;
	  }}
  if (!pstart)
    port_write_string(port)(sc, (const char *)p, len, port);
  else
    {
      pstart++;
      if (pstart != pcur)
	port_write_string(port)(sc, (const char *)pstart, pcur - pstart, port);
    }
  if (quoted) port_write_character(port)(sc, '"', port);
}

static void output_port_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if ((obj == sc->standard_output) || (obj == sc->standard_error))
    port_write_string(port)(sc, port_filename(obj), port_filename_length(obj), port);
  else
    if (use_write == p_readable)
      {
	if (port_is_closed(obj))
	  port_write_string(port)(sc, "(let ((p (open-output-string))) (close-output-port p) p)", 56, port);
	else
	  if (is_string_port(obj))
	    {
	      port_write_string(port)(sc, "(let ((p (open-output-string)))", 31, port);
	      if (port_position(obj) > 0)
		{
		  port_write_string(port)(sc, " (display ", 10, port);
		  slashify_string_to_port(sc, port, (const char *)port_data(obj), port_position(obj), IN_QUOTES);
		  port_write_string(port)(sc, " p)", 3, port);
		}
	      port_write_string(port)(sc, " p)", 3, port);
	    }
	  else
	    if (is_file_port(obj))
	      {
		char str[256];
		int32_t nlen;
		str[0] = '\0';
		nlen = (int32_t)catstrs(str, 256, "(open-output-file \"", port_filename(obj), "\" \"a\")", (char *)NULL);
		port_write_string(port)(sc, str, nlen, port);
	      }
	    else port_write_string(port)(sc, "#<output-function-port>", 23, port);
      }
    else
      {
	if (is_string_port(obj))
	  port_write_string(port)(sc, "#<output-string-port", 20, port);
	else
	  if (is_file_port(obj))
	    port_write_string(port)(sc, "#<output-file-port", 18, port);
	  else port_write_string(port)(sc, "#<output-function-port", 22, port);
	if (port_is_closed(obj))
	  port_write_string(port)(sc, ":closed>", 8, port);
	else port_write_character(port)(sc, '>', port);
      }
}

static void input_port_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if (obj == sc->standard_input)
    port_write_string(port)(sc, port_filename(obj), port_filename_length(obj), port);
  else
    if (use_write == p_readable)
      {
	if (port_is_closed(obj))
	  port_write_string(port)(sc, "(call-with-input-string \"\" (lambda (p) p))", 42, port);
	else
	  if (is_function_port(obj))
	    port_write_string(port)(sc, "#<input-function-port>", 22, port);
	  else
	    if (is_file_port(obj))
	      {
		char str[256];
		int32_t nlen;
		str[0] = '\0';
		nlen = (int32_t)catstrs(str, 256, "(open-input-file \"", port_filename(obj), "\")", (char *)NULL);
		port_write_string(port)(sc, str, nlen, port);
	      }
	    else
	      {
		const s7_int data_len = port_data_size(obj) - port_position(obj);
		if (data_len > 100)
		  {
		    const char *filename = (const char *)s7_port_filename(sc, obj);
		    if (filename)
		      {
                        #define DO_STR_LEN 1024
			char do_str[DO_STR_LEN];
			int32_t len;
			do_str[0] = '\0';
			if (port_position(obj) > 0)
			  {
			    len = (int32_t)catstrs(do_str, DO_STR_LEN, "(let ((port (open-input-file \"", filename, "\")))", (char *)NULL);
			    port_write_string(port)(sc, do_str, len, port);
			    do_str[0] = '\0';
			    len = (int32_t)catstrs(do_str, DO_STR_LEN, " (do ((i 0 (+ i 1)) (c (read-char port) (read-char port))) ((= i ",
						   pos_int_to_str_direct(sc, port_position(obj) - 1),
						   ") port)))", (char *)NULL);
			  }
			else len = (int32_t)catstrs(do_str, DO_STR_LEN, "(open-input-file \"", filename, "\")", (char *)NULL);
			port_write_string(port)(sc, do_str, len, port);
			return;
		      }}
		port_write_string(port)(sc, "(open-input-string ", 19, port);
		/* not port_write_string here because there might be embedded double-quotes */
		slashify_string_to_port(sc, port, (const char *)(port_data(obj) + port_position(obj)), port_data_size(obj) - port_position(obj), IN_QUOTES);
		port_write_character(port)(sc, ')', port);
	      }}
    else
      {
	if (is_string_port(obj))
	  port_write_string(port)(sc, "#<input-string-port", 19, port);
	else
	  if (is_file_port(obj))
	    port_write_string(port)(sc, "#<input-file-port", 17, port);
	  else port_write_string(port)(sc, "#<input-function-port", 21, port);
	if (port_filename(obj))
	  {
	    port_write_character(port)(sc, ' ', port);
	    port_write_string(port)(sc, port_filename(obj), port_filename_length(obj), port);
	  }
	if (port_is_closed(obj))
	  port_write_string(port)(sc, " :closed>", 9, port);
	else port_write_character(port)(sc, '>', port);
      }
}

static bool symbol_needs_slashification(s7_scheme *sc, s7_pointer obj)
{
  uint8_t *pend;
  char *str = symbol_name(obj); /* not const for make_atom */
  s7_int len;

  if ((str[0] == '#') || (str[0] == '\'') || (str[0] == ','))
    return(true);
  if (is_number(make_atom(sc, str, 10, NO_SYMBOLS, WITHOUT_OVERFLOW_ERROR)))
    return(true);

  len = symbol_name_length(obj);
  pend = (uint8_t *)(str + len);
  for (uint8_t *p = (uint8_t *)str; p < pend; p++)
    if (symbol_slashify_table[*p])
      return(true);
  set_clean_symbol(obj);
  return(false);
}

void symbol_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  /* I think this is the only place we print a symbol's name */
  if ((!is_clean_symbol(obj)) &&
      (symbol_needs_slashification(sc, obj)))
    {
      /* this can't work in general if use_write == p_readable:
       *    (define f (apply lambda (list () (list 'let (list (list (symbol "a b") 3)) (symbol "a b"))))) ; (f) -> 3
       *  prints "readably" as "(lambda () (let (((symbol \"a b\") 3)) (symbol \"a b\")))"
       *  so, 30-May-24 added (*s7* 'symbol-printer).
       */
      if (is_any_procedure(sc->symbol_printer)) /* we see p_write here */
	{
	  const s7_pointer printer = sc->symbol_printer;
	  s7_pointer result;
	  sc->symbol_printer = sc->F; /* avoid infinite recursion */
	  result = s7_call(sc, printer, set_plist_1(sc, obj));
	  if (!is_string(result))
	    error_nr(sc, sc->wrong_type_arg_symbol,
		     set_elist_2(sc, wrap_string(sc, "(*s7* 'symbol-printer) should return a string: ~S", 49), result));
	  /* if we restore symbol-printer before the error, and the printer function stupidly returned the bad symbol, infinite loop */
	  sc->symbol_printer = printer;
	  port_write_string(port)(sc, string_value(result), string_length(result), port);
	}
      else
	{
	  port_write_string(port)(sc, "(symbol \"", 9, port);
	  slashify_string_to_port(sc, port, symbol_name(obj), symbol_name_length(obj), NOT_IN_QUOTES);
	  port_write_string(port)(sc, "\")", 2, port);
	}}
  else
    {
      char c = '\0';
      if ((use_write == p_readable) || (use_write == p_code))
	{
	  if (!is_keyword(obj)) c = '\'';
	}
      else if ((use_write == p_key) && (!is_keyword(obj))) c = ':';
      if (is_string_port(port))
	{
	  s7_int new_len = port_position(port) + symbol_name_length(obj) + ((c) ? 1 : 0);
	  if (new_len >= port_data_size(port))
	    resize_port_data(sc, port, new_len * 2);
	  if (c) port_data(port)[port_position(port)++] = c;
	  memcpy((void *)(port_data(port) + port_position(port)), (void *)symbol_name(obj), symbol_name_length(obj));
	  port_position(port) = new_len;
	}
      else
	{
	  if (c) port_write_character(port)(sc, c, port);
	  port_write_string(port)(sc, symbol_name(obj), symbol_name_length(obj), port);
	}}
}

static char *multivector_indices_to_string(s7_scheme *sc, s7_int index, s7_pointer vect, char *str, int32_t str_len, int32_t cur_dim)
{
  s7_int size = vector_dimension(vect, cur_dim);
  s7_int ind = index % size;
  if (cur_dim > 0)
    multivector_indices_to_string(sc, (index - ind) / size, vect, str, str_len, cur_dim - 1);
  catstrs(str, str_len, " ", pos_int_to_str_direct(sc, ind), (char *)NULL);
  return(str);
}

#define not_p_display(Choice) ((Choice == p_display) ? p_write : Choice)

static int32_t multivector_to_port_1(s7_scheme *sc, s7_pointer vec, s7_pointer port,
				     int32_t out_len, int32_t flat_ref, int32_t dimension, int32_t dimensions, bool *last,
				     use_write_t use_write, shared_info_t *ci)
{
  if (use_write != p_readable)
    {
      if (*last)
	port_write_string(port)(sc, " (", 2, port);
      else port_write_character(port)(sc, '(', port);
      (*last) = false;
    }
  for (int32_t i = 0; i < vector_dimension(vec, dimension); i++)
    if (dimension == (dimensions - 1))
      {
	if (flat_ref < out_len)
	  {
	    object_to_port_with_circle_check(sc, vector_getter(vec)(sc, vec, flat_ref), port, not_p_display(use_write), ci);
	    if (use_write == p_readable)
	      port_write_string(port)(sc, ") ", 2, port);
	    flat_ref++;
	  }
	else
	  {
	    port_write_string(port)(sc, "...)", 4, port);
	    return(flat_ref);
	  }
	if ((use_write != p_readable) &&
	    (i < (vector_dimension(vec, dimension) - 1)))
	  port_write_character(port)(sc, ' ', port);
      }
    else
      if (flat_ref < out_len)
	flat_ref = multivector_to_port_1(sc, vec, port, out_len, flat_ref, dimension + 1, dimensions, last, not_p_display(use_write), ci);
      else
	{
	  port_write_string(port)(sc, "...)", 4, port);
	  return(flat_ref);
	}
  if (use_write != p_readable)
    port_write_character(port)(sc, ')', port);
  (*last) = true;
  return(flat_ref);
}

static int32_t multivector_to_port(s7_scheme *sc, s7_pointer vec, s7_pointer port,
				   int32_t out_len, int32_t flat_ref, int32_t dimension, int32_t dimensions,
				   use_write_t use_write, shared_info_t *ci)
{
  bool last = false;
  return(multivector_to_port_1(sc, vec, port, out_len, flat_ref, dimension, dimensions, &last, use_write, ci));
}

static void make_vector_to_port(s7_scheme *sc, s7_pointer vect, s7_pointer port)
{
  const s7_int vlen = vector_length(vect);
  int32_t plen;
  char buf[128];
  const char *vtyp = "";

  if (is_float_vector(vect))
    vtyp = "float-";
  else
    if (is_int_vector(vect))
      vtyp = "int-";
    else
      if (is_byte_vector(vect))
	vtyp = "byte-";
      else
	if (is_complex_vector(vect))
	  vtyp = "complex-";

  if (vector_rank(vect) == 1)
    {
      plen = (int32_t)catstrs_direct(buf, "(make-", vtyp, "vector ", integer_to_string_no_length(sc, vlen), " ", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
    }
  else
    {
      s7_int dim;
      plen = (int32_t)catstrs_direct(buf, "(make-", vtyp, "vector '(", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
      for (dim = 0; dim < vector_ndims(vect) - 1; dim++)
	{
	  plen = (int32_t)catstrs_direct(buf, integer_to_string_no_length(sc, vector_dimension(vect, dim)), " ", (const char *)NULL);
	  port_write_string(port)(sc, buf, plen, port);
	}
      plen = (int32_t)catstrs_direct(buf, integer_to_string_no_length(sc, vector_dimension(vect, dim)), ") ", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
    }
}

static void write_vector_dimensions(s7_scheme *sc, s7_pointer vect, s7_pointer port)
{
  char buf[128];
  s7_int dim, plen;
  port_write_string(port)(sc, " '(", 3, port);
  for (dim = 0; dim < vector_ndims(vect) - 1; dim++)
    {
      plen = catstrs_direct(buf, integer_to_string_no_length(sc, vector_dimension(vect, dim)), " ", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
    }
  plen = catstrs_direct(buf, integer_to_string_no_length(sc, vector_dimension(vect, dim)), "))", (const char *)NULL);
  port_write_string(port)(sc, buf, plen, port);
}


static void vector_to_port(s7_scheme *sc, s7_pointer vect, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  s7_int i, len = vector_length(vect), plen;
  bool too_long = false;
  char buf[2048]; /* 128 is too small -- this is the list of indices with a few minor flourishes */

  if (len == 0)
    {
      if (vector_rank(vect) > 1)
	{
	  plen = catstrs_direct(buf, "#", pos_int_to_str_direct(sc, vector_ndims(vect)), "d()", (const char *)NULL);
	  port_write_string(port)(sc, buf, plen, port);
	}
      else port_write_string(port)(sc, "#()", 3, port);
      return;
    }
  if (use_write != p_readable)
    {
      if (sc->print_length == 0)
	{
	  if (vector_rank(vect) > 1)
	    {
	      plen = catstrs_direct(buf, "#", pos_int_to_str_direct(sc, vector_ndims(vect)), "d(...)", (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	    }
	  else port_write_string(port)(sc, "#(...)", 6, port);
	  return;
	}
      if (len > sc->print_length)
	{
	  too_long = true;
	  len = sc->print_length;
	}}
  if ((!ci) &&
      (len > 1000))
    {
      const s7_int vlen = vector_length(vect);
      s7_pointer *els = vector_elements(vect);
      const s7_pointer p0 = els[0];
      for (i = 1; i < vlen; i++)
	if (els[i] != p0)
	  break;
      if (i == vlen)
	{
	  make_vector_to_port(sc, vect, port);
	  object_to_port(sc, p0, port, use_write, NULL);
	  if (is_typed_vector(vect))
	    {
	      port_write_character(port)(sc, ' ', port);
	      port_write_vector_typer(sc, vect, port);
	    }
	  port_write_character(port)(sc, ')', port);
	  return;
	}}
  check_stack_size(sc);
  gc_protect_via_stack(sc, vect);
  if (use_write == p_readable)
    {
      int32_t vref;
      if ((ci) &&
	  (is_cyclic(vect)) &&
	  ((vref = peek_shared_ref(ci, vect)) != 0))
	{
	  s7_pointer *els = vector_elements(vect);
	  if (vref < 0) vref = -vref;
	  if ((ci->defined[vref]) || (port == ci->cycle_port))
	    {
	      plen = catstrs_direct(buf, "<", pos_int_to_str_direct(sc, vref), ">", (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	      unstack_gc_protect(sc);
	      return;
	    }

	  if (is_typed_vector(vect))
	    port_write_string(port)(sc, "(let ((<v> ", 11, port);
	  if (vector_rank(vect) > 1)
	    port_write_string(port)(sc, "(subvector ", 11, port);

	  port_write_string(port)(sc, "(vector", 7, port); /* top level let */
	  for (i = 0; i < len; i++)
	    if (has_structure(els[i]))
	      {
		int32_t eref = peek_shared_ref(ci, els[i]);
		port_write_string(port)(sc, " #f", 3, port);
		if (eref != 0)
		  {
		    if (eref < 0) eref = -eref;
		    if (vector_rank(vect) > 1)
		      {
			const s7_int dimension = vector_rank(vect) - 1;
			const int32_t str_len = (dimension < 8) ? 128 : ((dimension + 1) * 16);
			block_t *b = callocate(sc, str_len);
			char *indices = (char *)block_data(b);
			multivector_indices_to_string(sc, i, vect, indices, str_len, dimension); /* calls pos_int_to_str_direct, writes to indices */
			plen = catstrs_direct(buf, "  (set! (<", pos_int_to_str_direct(sc, vref), ">",
					      indices, ") <", pos_int_to_str_direct_1(sc, eref), ">) ", (const char *)NULL);
			port_write_string(ci->cycle_port)(sc, buf, plen, ci->cycle_port);
			liberate(sc, b);
		      }
		    else
		      {
			size_t len1 = catstrs_direct(buf, "  (set! (<", pos_int_to_str_direct(sc, vref), "> ", integer_to_string(sc, i, &plen), ") <",
						     pos_int_to_str_direct_1(sc, eref), ">) ", (const char *)NULL);
			port_write_string(ci->cycle_port)(sc, buf, len1, ci->cycle_port);
		      }}
		else
		  {
		    if (vector_rank(vect) > 1)
		      {
			const s7_int dimension = vector_rank(vect) - 1;
			const int32_t str_len = (dimension < 8) ? 128 : ((dimension + 1) * 16);
			block_t *b = callocate(sc, str_len);
			char *indices = (char *)block_data(b);
			buf[0] = '\0';
			multivector_indices_to_string(sc, i, vect, indices, str_len, dimension); /* writes to indices */
			plen = catstrs(buf, 2048, "  (set! (<", pos_int_to_str_direct(sc, vref), ">", indices, ") ", (char *)NULL);
			port_write_string(ci->cycle_port)(sc, buf, plen, ci->cycle_port);
			liberate(sc, b);
		      }
		    else
		      {
			size_t len1 = catstrs_direct(buf, "  (set! (<", pos_int_to_str_direct(sc, vref), "> ", integer_to_string_no_length(sc, i), ") ", (const char *)NULL);
			port_write_string(ci->cycle_port)(sc, buf, len1, ci->cycle_port);
		      }
		    object_to_port_with_circle_check(sc, els[i], ci->cycle_port, p_readable, ci);
		    port_write_string(ci->cycle_port)(sc, ") ", 2, ci->cycle_port);
		  }}
	    else
	      {
		port_write_character(port)(sc, ' ', port);
		object_to_port_with_circle_check(sc, els[i], port, p_readable, ci);
	      }
	  port_write_character(port)(sc, ')', port);
	  if (vector_rank(vect) > 1)
	    {
	      plen = catstrs_direct(buf, " 0 ", pos_int_to_str_direct(sc, len), (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	      write_vector_dimensions(sc, vect, port);
	    }
	  if (is_typed_vector(vect))
	    {
	      port_write_string(port)(sc, ")) (set! (vector-typer <v>) ", 28, port);
	      port_write_vector_typer(sc, vect, port);
	      port_write_string(port)(sc, ") <v>)", 6, port);
	    }}
      else
	{
	  if (is_typed_vector(vect))
	    port_write_string(port)(sc, "(let ((<v> ", 11, port);
	  /* (let ((v (make-vector 3 'a symbol?))) (object->string v :readable)): "(let ((<v> (vector 'a 'a 'a))) (set! (vector-typer <v>) symbol?) <v>)" */

	  if (vector_rank(vect) > 1)
	    port_write_string(port)(sc, "(subvector ", 11, port);
	  if (is_immutable_vector(vect))
	    port_write_string(port)(sc, "(immutable! ", 12, port);

	  port_write_string(port)(sc, "(vector", 7, port);
	  for (i = 0; i < len; i++)
	    {
	      port_write_character(port)(sc, ' ', port);
	      object_to_port_with_circle_check(sc, vector_element(vect, i), port, p_readable, ci);
	    }

	  if (is_immutable_vector(vect))
	    port_write_string(port)(sc, "))", 2, port);
	  else port_write_character(port)(sc, ')', port);

	  if (vector_rank(vect) > 1)          /* subvector above */
	    {
	      plen = catstrs_direct(buf, " 0 ", pos_int_to_str_direct(sc, len), (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	      write_vector_dimensions(sc, vect, port);
	    }
	  if (is_typed_vector(vect))
	    {
	      port_write_string(port)(sc, ")) (set! (vector-typer <v>) ", 28, port);
	      port_write_vector_typer(sc, vect, port);
	      port_write_string(port)(sc, ") <v>)", 6, port);
	    }}}
  else /* not readable write */
    {
      if (vector_rank(vect) > 1) /* if rank>1, ndims exists */
	{
	  plen = catstrs_direct(buf, "#", pos_int_to_str_direct(sc, vector_ndims(vect)), "d", (const char *)NULL);
	  port_write_string(port)(sc, buf, plen, port);
	  multivector_to_port(sc, vect, port, len, 0, 0, vector_ndims(vect), use_write, ci);
	}
      else
	{
	  port_write_string(port)(sc, "#(", 2, port);
	  for (i = 0; i < len - 1; i++)
	    {
	      object_to_port_with_circle_check(sc, vector_element(vect, i), port, not_p_display(use_write), ci);
	      port_write_character(port)(sc, ' ', port);
	    }
	  object_to_port_with_circle_check(sc, vector_element(vect, i), port, not_p_display(use_write), ci);

	  if (too_long)
	    port_write_string(port)(sc, " ...)", 5, port);
	  else port_write_character(port)(sc, ')', port);
	}}
  unstack_gc_protect(sc);
}

static s7_int print_vector_length(s7_scheme *sc, s7_pointer vect, s7_pointer port, use_write_t use_write)
{
  const s7_int len = vector_length(vect);
  const char *vtype = "r"; /* "const" here for g++ */

  if (is_int_vector(vect)) vtype = "i";
  else if (is_complex_vector(vect)) vtype = "c";
  else if (is_byte_vector(vect)) vtype = "u";
  if (len == 0)
    {
      char buf[128];
      s7_int plen;
      if (vector_rank(vect) > 1)
	plen = (s7_int)catstrs_direct(buf, "#", vtype, pos_int_to_str_direct(sc, vector_ndims(vect)), "d()", (const char *)(const char *)NULL);
      else plen = (s7_int)catstrs_direct(buf, "#", vtype, "()", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
      return(-1);
    }
  if (use_write == p_readable)
    return(len);
  if (sc->print_length != 0)
    return((len > sc->print_length) ? sc->print_length : len);

  if (vector_rank(vect) > 1)
    {
      char buf[128];
      s7_int plen = (s7_int)catstrs_direct(buf, "#", vtype, pos_int_to_str_direct(sc, vector_ndims(vect)), "d(...)", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
    }
  else
    if (is_int_vector(vect))
      port_write_string(port)(sc, "#i(...)", 7, port);
    else
      if (is_float_vector(vect))
	port_write_string(port)(sc, "#r(...)", 7, port);
      else
	if (is_byte_vector(vect))
	  port_write_string(port)(sc, "#u8(...)", 8, port);
	else port_write_string(port)(sc, "#c(...)", 7, port);
  return(-1);
}

static void int_vector_to_port(s7_scheme *sc, s7_pointer vect, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  s7_int plen;
  bool too_long;
  char buf[128];
  const char *str;
  const s7_int len = print_vector_length(sc, vect, port, use_write);
  if (len < 0) return; /* actually -1, see above -- this means there's nothing more to print */
  too_long = (len < vector_length(vect));

  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_string(port)(sc, "(immutable! ", 12, port);

  if (len > 1000)
    {
      s7_int i;
      const s7_int vlen = vector_length(vect);
      const s7_int *els = int_vector_ints(vect);
      s7_int first = els[0];
      for (i = 1; i < vlen; i++)
	if (els[i] != first)
	  break;
      if (i == vlen)
	{
	  make_vector_to_port(sc, vect, port);
	  str = integer_to_string(sc, int_vector(vect, 0), &plen);
	  port_write_string(port)(sc, str, plen, port);
	  if ((use_write == p_readable) &&
	      (is_immutable_vector(vect)))
	    port_write_string(port)(sc, "))", 2, port);
	  else port_write_character(port)(sc, ')', port);
	  return;
	}}
  if (vector_rank(vect) == 1)
    {
      port_write_string(port)(sc, "#i(", 3, port);
      if (!is_string_port(port))
	{
	  str = integer_to_string(sc, int_vector(vect, 0), &plen);
	  port_write_string(port)(sc, str, plen, port);
	  for (s7_int i = 1; i < len; i++)
	    {
	      plen = catstrs_direct(buf, " ", integer_to_string_no_length(sc, int_vector(vect, i)), (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	    }}
      else
	{
	  s7_int new_len = port_position(port);
	  s7_int next_len = port_data_size(port) - 128;
	  uint8_t *dbuf = port_data(port);
	  if (new_len >= next_len)
	    {
	      resize_port_data(sc, port, port_data_size(port) * 2);
	      next_len = port_data_size(port) - 128;
	      dbuf = port_data(port);
	    }
	  str = integer_to_string(sc, int_vector(vect, 0), &plen);
	  memcpy((void *)(dbuf + new_len), (const void *)str, plen);
	  new_len += plen;
	  for (s7_int i = 1; i < len; i++)
	    {
	      if (new_len >= next_len)
		{
		  resize_port_data(sc, port, port_data_size(port) * 2);
		  next_len = port_data_size(port) - 128;
		  dbuf = port_data(port);
		}
	      plen = catstrs_direct((char *)(dbuf + new_len), " ", integer_to_string_no_length(sc, int_vector(vect, i)), (const char *)NULL);
	      new_len += plen;
	    }
	  port_position(port) = new_len;
	}
      if (too_long)
	port_write_string(port)(sc, " ...)", 5, port);
      else port_write_character(port)(sc, ')', port);
    }
  else
    {
      plen = catstrs_direct(buf, "#i", pos_int_to_str_direct(sc, vector_ndims(vect)), "d", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
      gc_protect_via_stack(sc, vect);
      multivector_to_port(sc, vect, port, len, 0, 0, vector_ndims(vect), p_display, NULL);
      unstack_gc_protect(sc);
    }
  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_character(port)(sc, ')', port);
}

static void float_vector_to_port(s7_scheme *sc, s7_pointer vect, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  #define FV_BUFSIZE 512 /* some floats can take around 312 bytes */
  char buf[FV_BUFSIZE];
  s7_int plen;
  bool too_long;
  const s7_double *els = float_vector_floats(vect);
  const s7_int len = print_vector_length(sc, vect, port, use_write);
  if (len < 0) return;  /* vector-length=0 etc */
  too_long = (len < vector_length(vect));

  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_string(port)(sc, "(immutable! ", 12, port);

  if (len > 1000)
    {
      s7_int i;
      const s7_int vlen = vector_length(vect);
      const s7_double first = els[0];
      for (i = 1; i < vlen; i++)
	if (els[i] != first)
	  break;
      if (i == vlen)
	{
	  make_vector_to_port(sc, vect, port);
	  plen = snprintf(buf, FV_BUFSIZE, "%.*g)", sc->float_format_precision, first);
	  port_write_string(port)(sc, buf, clamp_length(plen, FV_BUFSIZE), port);
	  if ((use_write == p_readable) &&
	      (is_immutable_vector(vect)))
	    port_write_character(port)(sc, ')', port);
	  return;
	}}

  if (vector_rank(vect) == 1)
    {
      port_write_string(port)(sc, "#r(", 3, port);
      plen = snprintf(buf, FV_BUFSIZE - 4, "%.*g", sc->float_format_precision, els[0]); /* -4 so floatify has room */
      floatify(buf, &plen);
      port_write_string(port)(sc, buf, clamp_length(plen, FV_BUFSIZE), port);
      for (s7_int i = 1; i < len; i++)
	{
	  plen = snprintf(buf, FV_BUFSIZE - 4, " %.*g", sc->float_format_precision, els[i]);
	  plen--; /* fixup for the initial #\space */
	  floatify((char *)(buf + 1), &plen);
	  port_write_string(port)(sc, buf, clamp_length(plen + 1, FV_BUFSIZE), port);
	}
      if (too_long)
	port_write_string(port)(sc, " ...)", 5, port);
      else port_write_character(port)(sc, ')', port);
    }
  else
    {
      plen = catstrs_direct(buf, "#r", pos_int_to_str_direct(sc, vector_ndims(vect)), "d", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
      gc_protect_via_stack(sc, vect);
      multivector_to_port(sc, vect, port, len, 0, 0, vector_ndims(vect), p_display, NULL);
      unstack_gc_protect(sc);
    }
  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_character(port)(sc, ')', port);
}

static char *complex_to_string_base_10(s7_scheme *sc, s7_complex obj, s7_int width, s7_int precision,
				      char float_choice, s7_int *nlen, use_write_t choice)
{
  char *imag;
  s7_int len = width + precision;
  len = (len > 512) ? (512 + 2 * len) : 1024;
  if (len > sc->num_to_str_size)
    {
      sc->num_to_str = (sc->num_to_str) ? (char *)Realloc(sc->num_to_str, len) : (char *)Malloc(len);
      sc->num_to_str_size = len;
    }
  sc->num_to_str[0] = '\0';
  imag = copy_string(number_to_string_base_10(sc, wrap_real(sc, cimag(obj)), 0, precision, float_choice, &len, choice));
  sc->num_to_str[0] = '\0';
  number_to_string_base_10(sc, wrap_real(sc, creal(obj)), 0, precision, float_choice, &len, choice);
  sc->num_to_str[len] = '\0';
  len = catstrs(sc->num_to_str, sc->num_to_str_size, ((imag[0] == '+') || (imag[0] == '-')) ? "" : "+", imag, "i", (char *)NULL);
  free(imag);
  if (width > len)
    {
      insert_spaces(sc, sc->num_to_str, width, len); /* this checks sc->num_to_str_size */
      (*nlen) = width;
    }
  else (*nlen) = len;
  return(sc->num_to_str);
}

static void complex_vector_to_port(s7_scheme *sc, s7_pointer vect, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  #define CV_BUFSIZE 1024 /* some floats can take around 312 bytes */
  bool too_long;
  const s7_complex *els = complex_vector_complexes(vect);
  s7_int len = print_vector_length(sc, vect, port, use_write);
  if (len < 0) return;  /* vector-length=0 etc */
  too_long = (len < vector_length(vect));

  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_string(port)(sc, "(immutable! ", 12, port);

  if (len > 1000)
    {
      s7_int i;
      const s7_int vlen = vector_length(vect);
      const s7_complex first = els[0];
      for (i = 1; i < vlen; i++)
	if (els[i] != first)
	  break;
      if (i == vlen)
	{
	  s7_int plen;
	  char *num = complex_to_string_base_10(sc, first, 0, sc->float_format_precision, 'g', &plen, use_write);
	  make_vector_to_port(sc, vect, port);
	  port_write_string(port)(sc, num, clamp_length(plen, CV_BUFSIZE), port);
	  if ((use_write == p_readable) &&
	      (is_immutable_vector(vect)))
	    port_write_string(port)(sc, "))", 2, port);
	  else port_write_character(port)(sc, ')', port);
	  return;
	}}

  if (vector_rank(vect) == 1)
    {
      s7_int plen;
      char *num = complex_to_string_base_10(sc, els[0], 0, sc->float_format_precision, 'g', &plen, use_write);
      port_write_string(port)(sc, "#c(", 3, port);
      port_write_string(port)(sc, num, clamp_length(plen, CV_BUFSIZE), port);
      for (s7_int i = 1; i < len; i++)
	{
	  num = complex_to_string_base_10(sc, els[i], 0, sc->float_format_precision, 'g', &plen, use_write);
	  port_write_character(port)(sc, ' ', port);
	  port_write_string(port)(sc, num, clamp_length(plen, CV_BUFSIZE), port);
	}
      if (too_long)
	port_write_string(port)(sc, " ...)", 5, port);
      else port_write_character(port)(sc, ')', port);
    }
  else
    {
      char buf[CV_BUFSIZE];
      s7_int plen = catstrs_direct(buf, "#c", pos_int_to_str_direct(sc, vector_ndims(vect)), "d", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
      gc_protect_via_stack(sc, vect);
      multivector_to_port(sc, vect, port, len, 0, 0, vector_ndims(vect), p_display, NULL);
      unstack_gc_protect(sc);
    }
  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_character(port)(sc, ')', port);
}

static void byte_vector_to_port(s7_scheme *sc, s7_pointer vect, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  bool too_long;
  const s7_int len = print_vector_length(sc, vect, port, use_write);
  if (len < 0) return;
  too_long = (len < vector_length(vect));

  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_string(port)(sc, "(immutable! ", 12, port);

  if (len > 1000)
    {
      s7_int i;
      const s7_int vlen = vector_length(vect);
      const uint8_t *els = byte_vector_bytes(vect);
      uint8_t first = els[0];
      for (i = 1; i < vlen; i++)
	if (els[i] != first)
	  break;
      if (i == vlen)
	{
	  s7_int plen;
	  const char *str; /* const for integer_to_string */
	  make_vector_to_port(sc, vect, port);
	  str = integer_to_string(sc, byte_vector(vect, 0), &plen); /* only 0..10 start out with names: init_small_ints */
	  port_write_string(port)(sc, str, plen, port);
	  if ((use_write == p_readable) &&
	      (is_immutable_vector(vect)))
	    port_write_string(port)(sc, "))", 2, port);
	  else port_write_character(port)(sc, ')', port);
	  return;
	}}

  if (vector_rank(vect) == 1)
    {
      s7_int plen;
      const char *str;
      port_write_string(port)(sc, "#u8(", 4, port);
      str = integer_to_string(sc, byte_vector(vect, 0), &plen);
      port_write_string(port)(sc, str, plen, port);
      for (s7_int i = 1; i < len; i++)
	{
	  char buf[128];
	  plen = catstrs_direct(buf, " ", integer_to_string_no_length(sc, byte_vector(vect, i)), (const char *)NULL);
	  port_write_string(port)(sc, buf, plen, port);
	}
      if (too_long)
	port_write_string(port)(sc, " ...)", 5, port);
      else port_write_character(port)(sc, ')', port);
    }
  else
    {
      char buf[128];
      s7_int plen = catstrs_direct(buf, "#u8", pos_int_to_str_direct(sc, vector_ndims(vect)), "d", (const char *)NULL);
      port_write_string(port)(sc, buf, plen, port);
      multivector_to_port(sc, vect, port, len, 0, 0, vector_ndims(vect), p_display, NULL);
    }
  if ((use_write == p_readable) &&
      (is_immutable_vector(vect)))
    port_write_character(port)(sc, ')', port);
}

static void string_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  bool immutable = ((use_write == p_readable) &&
		    (is_immutable_string(obj)) &&
		    (string_length(obj) > 0));  /* (immutable "") looks dumb */
  if (immutable)
    port_write_string(port)(sc, "(immutable! ", 12, port);

  if (string_length(obj) > 0)
    {
      /* since string_length is a scheme length, not C, this write can embed nulls from C's point of view */
      if (string_length(obj) > 1000) /* was 10000 28-Feb-18 */
	{
	  size_t size;
	  char buf[128];
	  buf[0] = string_value(obj)[0];
	  buf[1] = '\0';
	  size = strspn((const char *)(string_value(obj) + 1), buf); /* if all #\null, this won't work */
	  if (size == (size_t)(string_length(obj) - 1))
	    {
	      const s7_pointer c = chars[(int32_t)((uint8_t)(buf[0]))];
	      const int32_t nlen = (int32_t)catstrs_direct(buf, "(make-string ", pos_int_to_str_direct(sc, string_length(obj)), " ", (const char *)NULL);
	      port_write_string(port)(sc, buf, nlen, port);
	      port_write_string(port)(sc, character_name(c), character_name_length(c), port);
	      if (immutable)
		port_write_string(port)(sc, "))", 2, port);
	      else port_write_character(port)(sc, ')', port);
	      return;
	    }}
      if (use_write == p_display)
	port_write_string(port)(sc, string_value(obj), string_length(obj), port);
      else
	if (!string_needs_slashification((const uint8_t *)string_value(obj), string_length(obj)))
	  {
	    port_write_character(port)(sc, '"', port);
	    port_write_string(port)(sc, string_value(obj), string_length(obj), port);
	    port_write_character(port)(sc, '"', port);
	  }
	else slashify_string_to_port(sc, port, string_value(obj), string_length(obj), IN_QUOTES);
    }
  else
    if (use_write != p_display)
      port_write_string(port)(sc, "\"\"", 2, port);

  if (immutable)
    port_write_character(port)(sc, ')', port);
}

static s7_int list_length_with_immutable_check(s7_scheme *sc, s7_pointer a, bool *immutable)
{
  s7_pointer slow = a, fast = a;
  for (s7_int i = 0; ; i += 2)
    {
      if (!is_pair(fast)) return((is_null(fast)) ? i : -i);
      if (is_immutable_pair(fast)) *immutable = true;
      fast = cdr(fast);
      if (!is_pair(fast)) return((is_null(fast)) ? (i + 1) : (-i - 1));
      if (is_immutable_pair(fast)) *immutable = true;
      fast = cdr(fast);
      slow = cdr(slow);
      if (fast == slow) return(0);
    }
  return(0);
}

static void simple_list_readable_display(s7_scheme *sc, s7_pointer lst, s7_int true_len, s7_int len, s7_pointer port, shared_info_t *ci, bool immutable)
{
  /* the easier cases: no circles or shared refs to patch up */
  if ((true_len > 0) && (!immutable))
    {
      port_write_string(port)(sc, "list", 4, port);
      for (s7_pointer p = lst; is_pair(p); p = cdr(p))
	{
	  port_write_character(port)(sc, ' ', port);
	  object_to_port_with_circle_check(sc, car(p), port, p_readable, ci);
	}
      port_write_character(port)(sc, ')', port);
    }
  else
    {
      s7_pointer p;
      s7_int immutable_ctr = 0;
      if (is_immutable_pair(lst))
	{
	  port_write_string(port)(sc, "immutable! (cons ", 17, port);
	  immutable_ctr++;
	}
      else port_write_string(port)(sc, "cons ", 5, port);
      object_to_port_with_circle_check(sc, car(lst), port, p_readable, ci);

      for (p = cdr(lst); is_pair(p); p = cdr(p))
	{
	  if (is_immutable_pair(p))
	    {
	      port_write_string(port)(sc, " (immutable! (cons ", 19, port);
	      immutable_ctr++;
	    }
	  else port_write_string(port)(sc, " (cons ", 7, port);
	  object_to_port_with_circle_check(sc, car(p), port, p_readable, ci);
	}
      if (is_null(p))
	port_write_string(port)(sc, " ()", 3, port);
      else
	{
	  port_write_character(port)(sc, ' ', port);
	  object_to_port_with_circle_check(sc, p, port, p_readable, ci);
	}
      for (s7_int i = (true_len <= 0) ? 1 : 0; i < len; i++)
	port_write_character(port)(sc, ')', port);
      for (s7_int i = 0; i < immutable_ctr; i++)
	port_write_character(port)(sc, ')', port);
    }
}

static void pair_to_port(s7_scheme *sc, s7_pointer lst, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  s7_int len;
  bool immutable = false;
  const s7_int true_len = list_length_with_immutable_check(sc, lst, &immutable);
  if (true_len < 0)                    /* a dotted list -- handle cars, then final cdr */
    len = (-true_len + 1);
  else len = (true_len == 0) ? circular_list_entries(lst) : true_len; /* circular list (nil is handled by unique_to_port) */

  if ((use_write == p_readable) && (ci))
    {
      int32_t href = peek_shared_ref(ci, lst);
      if (href != 0)
	{
	  if (href < 0) href = -href;
	  if ((ci->defined[href]) || (port == ci->cycle_port))
	    {
	      char buf[128];
	      int32_t plen = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, href), ">", (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	      return;
	    }}}
  if ((use_write != p_readable) &&
      ((car(lst) == sc->quote_function) || (car(lst) == sc->quote_symbol)) &&
      (true_len == 2))
    {
      const bool need_new_ci = ((!ci) && (is_pair(cadr(lst))));
      shared_info_t *new_ci = NULL, *temp_ci = NULL;
      const bool old_locked = sc->object_out_locked;
      /* true_len == 2 is important, otherwise (list 'quote 1 2) -> '1 2 which looks weird
       *   or (object->string (apply . `''1)) -> "'quote 1"
       * so (quote x) = 'x but (quote x y z) should be left alone (if evaluated, it's an error)
       * :readable is tricky because the list might be something like (list 'quote (lambda () #f)) which needs to be evalable back to its original
       */
      if (car(lst) == sc->quote_symbol)
	port_write_string(port)(sc, "(quote ", 7, port);
      else port_write_character(port)(sc, '\'', port);
      if (need_new_ci)
	{
	  new_ci = make_shared_info(sc);
	  /* clear_shared_info(new_ci); */
	  temp_ci = load_shared_info(sc, cadr(lst), false, new_ci); /* temp_ci can be NULL! */
	}
      else temp_ci = ci;
      if (need_new_ci) sc->object_out_locked = true;
      object_to_port_with_circle_check(sc, cadr(lst), port, p_write, temp_ci);
      if (need_new_ci)
	{
	  sc->object_out_locked = old_locked;
	  free_shared_info(new_ci);
	}
      if (car(lst) == sc->quote_symbol)
	port_write_character(port)(sc, ')', port);
      return;
    }
#if WITH_IMMUTABLE_UNQUOTE
  if ((car(lst) == sc->unquote_symbol) && (true_len == 2))
    {
      port_write_character(port)(sc, ',', port);
      object_to_port_with_circle_check(sc, cadr(lst), port, p_write, ci);
      return;
    }
#endif

  if (is_multiple_value(lst))
    port_write_string(port)(sc, "(values ", 8, port);
  else port_write_character(port)(sc, '(', port);

  if (use_write == p_readable)
    {
      if (!is_cyclic(lst))
	{
	  /* here (and in the cyclic case) we need to handle immutable pairs -- this requires using cons rather than list etc */
	  simple_list_readable_display(sc, lst, true_len, len, port, ci, immutable);
	  return;
	}
      if (ci)
	{
	  int32_t plen;
	  s7_pointer p, local_port;
	  char buf[128], lst_name[128];
	  bool lst_local = false;
	  int32_t lst_ref = peek_shared_ref(ci, lst);
	  if (lst_ref == 0)
	    {
	      for (p = lst; is_pair(p); p = cdr(p))
		if ((has_structure(car(p))) ||
		    ((is_pair(cdr(p))) &&
		     (peek_shared_ref(ci, cdr(p)) != 0)))
		  {
		    lst_name[0] = '<'; lst_name[1] = 'L'; lst_name[2] = '>'; lst_name[3] = '\0';
		    lst_local = true;
		    port_write_string(port)(sc, "let ((<L> (list", 15, port); /* '(' above */
		    break;
		  }
	      if (!lst_local)
		{
		  if (has_structure(p))
		    {
		      lst_name[0] = '<'; lst_name[1] = 'L'; lst_name[2] = '>'; lst_name[3] = '\0';
		      lst_local = true;
		      port_write_string(port)(sc, "let ((<L> (list", 15, port); /* '(' above */
		    }
		  else
		    {
		      simple_list_readable_display(sc, lst, true_len, len, port, ci, immutable);
		      return;
		    }}}
	  else
	    {
	      if (lst_ref < 0) lst_ref = -lst_ref;
	      catstrs_direct(lst_name, "<", pos_int_to_str_direct(sc, lst_ref), ">", (const char *)NULL);
	      port_write_string(port)(sc, "list", 4, port); /* '(' above */
	    }
	  p = lst;
	  for (s7_int i = 0; (i < len) && (is_pair(p)); p = cdr(p), i++)
	    {
	      if ((has_structure(car(p))) &&
		  (is_cyclic(car(p))))
		port_write_string(port)(sc, " #f", 3, port);
	      else
		{
		  port_write_character(port)(sc, ' ', port);
		  object_to_port_with_circle_check(sc, car(p), port, use_write, ci);
		}
	      if ((is_pair(cdr(p))) &&
		  (peek_shared_ref(ci, cdr(p)) != 0))
		break;
	    }

	  if (lst_local)
	    port_write_string(port)(sc, "))) ", 4, port);
	  else port_write_character(port)(sc, ')', port);

	  /* fill in the cyclic entries */
	  local_port = ((lst_local) || (ci->cycle_port == sc->F)) ? port : ci->cycle_port; /* (object->string (list-values `(p . 1) (signature (int-vector))) :readable) */
	  p = lst;
	  for (s7_int i = 0; (i < len) && (is_pair(p)); p = cdr(p), i++)
	    {
	      int32_t lref;
	      if ((has_structure(car(p))) &&
		  (is_cyclic(car(p))))
		{
		  if (i == 0)
		    plen = (int32_t)catstrs_direct(buf, "  (set-car! ", lst_name, " ", (const char *)NULL);
		  else plen = (int32_t)catstrs_direct(buf, "  (set! (", lst_name, " ", pos_int_to_str_direct(sc, i), ") ", (const char *)NULL);
		  port_write_string(local_port)(sc, buf, plen, local_port);
		  lref = peek_shared_ref(ci, car(p));
		  if (lref == 0)
		    object_to_port_with_circle_check(sc, car(p), local_port, use_write, ci);
		  else
		    {
		      if (lref < 0) lref = -lref;
		      plen = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, lref), ">", (const char *)NULL);
		      port_write_string(local_port)(sc, buf, plen, local_port);
		    }
		  port_write_string(local_port)(sc, ") ", 2, local_port);
		}
	      if ((is_pair(cdr(p))) &&
		  ((lref = peek_shared_ref(ci, cdr(p))) != 0))
		{
		  if (lref < 0) lref = -lref;
		  if (i == 0)
		    plen = (int32_t)catstrs_direct(buf, (lst_local) ? "    " : "  ",
						   "(set-cdr! ", lst_name, " <", pos_int_to_str_direct(sc, lref), ">) ", (const char *)NULL);
		  else
		    if (i == 1)
		      plen = (int32_t)catstrs_direct(buf, (lst_local) ? "    " : "  ",
						     "(set-cdr! (cdr ", lst_name, ") <", pos_int_to_str_direct(sc, lref), ">) ", (const char *)NULL);
		    else plen = (int32_t)catstrs_direct(buf, (lst_local) ? "    " : "  ",
							"(set-cdr! (list-tail ", lst_name, " ", pos_int_to_str_direct_1(sc, i),
							") <", pos_int_to_str_direct(sc, lref), ">) ", (const char *)NULL);
		  port_write_string(local_port)(sc, buf, plen, local_port);
		  break;
		}}
	  if (true_len < 0) /* dotted list */
	    {
	      s7_pointer end_p;
	      for (end_p = lst; is_pair(end_p); end_p = cdr(end_p)); /* or maybe faster, start at p? */
	      /* we can't depend on the loops above to set p to the last element because they sometimes break out */
	      if (true_len == -1) /* cons cell */
		plen = (int32_t)catstrs_direct(buf, (lst_local) ? "    " : "  ", "(set-cdr! ", lst_name, " ", (const char *)NULL);
	      else
		if (true_len == -2)
		  plen = (int32_t)catstrs_direct(buf, (lst_local) ? "    " : "  ", "(set-cdr! (cdr ", lst_name, ") ", (const char *)NULL);
		else plen = (int32_t)catstrs_direct(buf, "(set-cdr! (list-tail ", lst_name, " ", pos_int_to_str_direct(sc, len - 2), ") ", (const char *)NULL);
	      port_write_string(local_port)(sc, buf, plen, local_port);
	      object_to_port_with_circle_check(sc, end_p, local_port, use_write, ci);
	      port_write_string(local_port)(sc, ") ", 2, local_port);
	    }
	  if (lst_local)
	    port_write_string(local_port)(sc, "    <L>)", 8, local_port);
	}
      else simple_list_readable_display(sc, lst, true_len, len, port, ci, immutable);
    }
  else /* not :readable */
    {
      const s7_int plen = (len > sc->print_length) ? sc->print_length : len;
      if (plen <= 0)
	{
	  port_write_string(port)(sc, "(...))", 6, port); /* open paren above about 150 lines, "list" here is wrong if it's a cons */
	  return;
	}
      if (ci)
	{
	  s7_pointer p;
	  s7_int i;
	  for (p = lst, i = 0; (is_pair(p)) && (i < plen) && ((i == 0) || (peek_shared_ref(ci, p) == 0)); i++, p = cdr(p))
	    {
	      ci->ctr++;
	      if (ci->ctr > sc->print_length)
		{
		  port_write_string(port)(sc, " ...)", 5, port);
		  return;
		}
	      object_to_port_with_circle_check(sc, car(p), port, not_p_display(use_write), ci);
	      if (i < (len - 1))
		port_write_character(port)(sc, ' ', port);
	    }
	  if (is_not_null(p))
	    {
	      if (plen < len)
		port_write_string(port)(sc, " ...", 4, port);
	      else
		{
		  if ((true_len == 0) &&
		      (i == len))
		    port_write_string(port)(sc, " . ", 3, port);
		  else port_write_string(port)(sc, ". ", 2, port);
		  object_to_port_with_circle_check(sc, p, port, not_p_display(use_write), ci);
		}}
	  port_write_character(port)(sc, ')', port);
	}
      else
	{
	  s7_pointer p = lst;
	  const s7_int len1 = plen - 1;
	  if (is_string_port(port))
	    {
	      for (s7_int i = 0; (is_pair(p)) && (i < len1); i++, p = cdr(p))
		{
		  object_to_port(sc, car(p), port, not_p_display(use_write), ci);
		  if (port_position(port) >= sc->objstr_max_len)
		    return;
		  if (port_position(port) >= port_data_size(port))
		    resize_port_data(sc, port, port_data_size(port) * 2);
		  port_data(port)[port_position(port)++] = (uint8_t)' ';
		}}
	  else
	    for (s7_int i = 0; (is_pair(p)) && (i < len1); i++, p = cdr(p))
	      {
		object_to_port(sc, car(p), port, not_p_display(use_write), ci);  /* lst free here if unprotected */
		port_write_character(port)(sc, ' ', port);
	      }
	  if (is_pair(p))
	    {
	      object_to_port(sc, car(p), port, not_p_display(use_write), ci);
	      p = cdr(p);
	    }
	  if (is_not_null(p))
	    {
	      if (plen < len)
		port_write_string(port)(sc, " ...", 4, port);
	      else
		{
		  port_write_string(port)(sc, ". ", 2, port);
		  object_to_port(sc, p, port, not_p_display(use_write), ci);
		}}
	  port_write_character(port)(sc, ')', port);
	}}
}

s7_pointer find_closure(s7_scheme *sc, s7_pointer closure, s7_pointer current_let);

s7_pointer find_typer(s7_scheme *sc, s7_pointer typer)
{
  s7_pointer sym = find_closure(sc, typer, closure_let(typer));
  if (!is_symbol(sym))
    sym = find_closure(sc, typer, sc->curlet);
  return(sym);
}

const char *hash_table_typer_name(s7_scheme *sc, s7_pointer typer)
{
  if (is_c_function(typer)) return(c_function_name(typer));
  if (is_boolean(typer)) return("#t");
#if S7_DEBUGGING /* I don't think this happens anymore */
  if (typer == sc->unused)
    {
      fprintf(stderr, "%s[%d]: hash typer is #<unused>\n", __func__, __LINE__);
      return("#<unused>"); /* mapper can be sc->unused briefly -- where? */
    }
#endif
  {
    s7_pointer sym = find_typer(sc, typer);
    return((is_symbol(sym)) ? symbol_name(sym) : NULL); /* see below in hash_table_procedures_to_port */
  }
}

static void hash_typers_to_port(s7_scheme *sc, s7_pointer hash, s7_pointer port)
{
  if (((is_typed_hash_table(hash)) || (is_pair(hash_table_procedures(hash)))) &&
      ((!is_boolean(hash_table_key_typer(hash))) || (!is_boolean(hash_table_value_typer(hash)))))
    {
      const char *typer = hash_table_typer_name(sc, hash_table_key_typer(hash));
      port_write_string(port)(sc, " (cons ", 7, port);
      port_write_string(port)(sc, typer, safe_strlen(typer), port);
      port_write_character(port)(sc, ' ', port);
      typer = hash_table_typer_name(sc, hash_table_value_typer(hash));
      port_write_string(port)(sc, typer, safe_strlen(typer), port);
      port_write_string(port)(sc, "))", 2, port);
    }
  else port_write_character(port)(sc, ')', port);
}

static void hash_table_procedures_to_port(s7_scheme *sc, s7_pointer hash, s7_pointer port, bool closed, shared_info_t *ci)
{
  const char *typer = hash_table_checker_name(sc, hash);
  if ((closed) && (is_immutable_hash_table(hash)))
    port_write_string(port)(sc, "(immutable! ", 12, port);

  if (typer[0] == '#') /* #f */
    {
      if (is_pair(hash_table_procedures(hash)))
	{
	  s7_int nlen = 0;
	  const char *str = (const char *)integer_to_string(sc, hash_table_size(hash), &nlen);
	  const char *checker = hash_table_typer_name(sc, hash_table_procedures_checker(hash));
	  const char *mapper = hash_table_typer_name(sc, hash_table_procedures_mapper(hash));
	  if (is_weak_hash_table(hash))
	    port_write_string(port)(sc, "(make-weak-hash-table ", 22, port);
	  else port_write_string(port)(sc, "(make-hash-table ", 17, port);
	  port_write_string(port)(sc, str, nlen, port);
	  if ((checker) && (mapper))
	    {
	      if ((is_boolean(hash_table_procedures_checker(hash))) && (is_boolean(hash_table_procedures_mapper(hash))))
		port_write_string(port)(sc, " #f", 3, port); /* no checker/mapper set? */
	      else
		{
		  port_write_string(port)(sc, " (cons ", 7, port);
		  port_write_string(port)(sc, checker, safe_strlen(checker), port);
		  port_write_character(port)(sc, ' ', port);
		  port_write_string(port)(sc, mapper, safe_strlen(mapper), port);
		  port_write_character(port)(sc, ')', port);
		}}
	  else
	    if ((is_any_closure(hash_table_procedures_checker(hash))) ||
		(is_any_closure(hash_table_procedures_mapper(hash))))
	      {
		port_write_string(port)(sc, " (cons ", 7, port);
		if (is_any_closure(hash_table_procedures_checker(hash)))
		  object_to_port_with_circle_check(sc, hash_table_procedures_checker(hash), port, p_readable, ci);
		else port_write_string(port)(sc, checker, safe_strlen(checker), port);
		port_write_character(port)(sc, ' ', port);
		if (is_any_closure(hash_table_procedures_mapper(hash)))
		  object_to_port_with_circle_check(sc, hash_table_procedures_mapper(hash), port, p_readable, ci);
		else port_write_string(port)(sc, mapper, safe_strlen(mapper), port);
		port_write_character(port)(sc, ')', port);
	      }
	    else port_write_string(port)(sc, " #f", 3, port); /* no checker/mapper set? */
	  hash_typers_to_port(sc, hash, port);
	}
      else
	if (is_weak_hash_table(hash))
	  port_write_string(port)(sc, "(weak-hash-table)", 17, port);
	else port_write_string(port)(sc, "(hash-table)", 12, port);
    }
  else
    {
      s7_int nlen = 0;
      const char *str = integer_to_string(sc, hash_table_size(hash), &nlen);
      if (is_weak_hash_table(hash))
	port_write_string(port)(sc, "(make-weak-hash-table ", 22, port);
      else port_write_string(port)(sc, "(make-hash-table ", 17, port);
      port_write_string(port)(sc, str, nlen, port);
      port_write_character(port)(sc, ' ', port);
      port_write_string(port)(sc, typer, safe_strlen(typer), port);
      hash_typers_to_port(sc, hash, port);
    }
  if (is_immutable_hash_table(hash))
    port_write_character(port)(sc, ')', port);
}

static void hash_table_to_port(s7_scheme *sc, s7_pointer hash, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  s7_int gc_iter, len = hash_table_entries(hash);
  bool too_long = false, hash_cyclic = false, copied = false, immut = false, letd = false;
  s7_pointer iterator;
  int32_t href = -1;

  if (len == 0)
    {
      if (use_write == p_readable)
	hash_table_procedures_to_port(sc, hash, port, true, ci);
      else
	{
	  if (is_weak_hash_table(hash))
	    port_write_string(port)(sc, "(weak-hash-table)", 17, port);
	  else port_write_string(port)(sc, "(hash-table)", 12, port);
	}
      return;
    }

  if (use_write != p_readable)
    {
      s7_int plen = sc->print_length;
      if (plen <= 0)
	{
	  port_write_string(port)(sc, "(hash-table ...)", 16, port);
	  return;
	}
      if (len > plen)
	{
	  too_long = true;
	  len = plen;
	}}

  if ((use_write == p_readable) &&
      (ci))
    {
      href = peek_shared_ref(ci, hash);
      if (href != 0)
	{
	  if (href < 0) href = -href;
	  if ((ci->defined[href]) || (port == ci->cycle_port))
	    {
	      char buf[128];
	      int32_t plen = catstrs_direct(buf, "<", pos_int_to_str_direct(sc, href), ">", (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	      return;
	    }}}

  iterator = s7_make_iterator(sc, hash);
  gc_iter = gc_protect_1(sc, iterator);
  iterator_carrier(iterator) = cons_unchecked(sc, sc->F, sc->F);
  set_has_carrier(iterator);
  hash_cyclic = ((ci) && (is_cyclic(hash)) && ((href = peek_shared_ref(ci, hash)) != 0));

  if (use_write == p_readable)
    {
      if ((is_typed_hash_table(hash)) || (is_pair(hash_table_procedures(hash))) || (hash_chosen(hash)))
	{
	  port_write_string(port)(sc, "(let ((<h> ", 11, port);
	  letd = true;
	}
      else
	if ((is_immutable_hash_table(hash)) && (!hash_cyclic))
	  {
	    port_write_string(port)(sc, "(immutable! ", 12, port);
	    immut = true;
	  }}

  if ((use_write == p_readable) &&
      (hash_cyclic))
    {
      if (href < 0) href = -href;
      if ((!is_typed_hash_table(hash)) && (!is_pair(hash_table_procedures(hash))) && (!hash_chosen(hash)))
	{
	  if (is_weak_hash_table(hash))
	    port_write_string(port)(sc, "(weak-hash-table", 16, port);
	  else port_write_string(port)(sc, "(hash-table", 11, port); /* top level let */
	}
      else
	{
	  hash_table_procedures_to_port(sc, hash, port, true, ci);
	  port_write_character(port)(sc, ')', port);
	}

      /* output here is deferred via ci->cycle_port until later in cyclic_out */
      for (s7_int i = 0; i < len; i++)
	{
	  const s7_pointer key_val = hash_table_iterate(sc, iterator);
	  if (key_val == sc->iterator_at_end_value) break; /* key_val can be #<eof> if hash is a weak-hash-table, and a GC happens during this loop */
	  {
	    const s7_pointer key = car(key_val);
	    const s7_pointer val = cdr(key_val);
	    char buf[128];
	    int32_t eref = peek_shared_ref(ci, val);
	    int32_t kref = peek_shared_ref(ci, key);
	    int32_t plen = catstrs_direct(buf, "  (set! (<", pos_int_to_str_direct(sc, href), "> ", (const char *)NULL);
	    port_write_string(ci->cycle_port)(sc, buf, plen, ci->cycle_port);
	    if (kref != 0)
	      {
		if (kref < 0) kref = -kref;
		plen = catstrs_direct(buf, "<", pos_int_to_str_direct(sc, kref), ">", (const char *)NULL);
		port_write_string(ci->cycle_port)(sc, buf, plen, ci->cycle_port);
	      }
	    else object_to_port(sc, key, ci->cycle_port, p_readable, ci);
	    if (eref != 0)
	      {
		if (eref < 0) eref = -eref;
		plen = catstrs_direct(buf, ") <", pos_int_to_str_direct(sc, eref), ">) ", (const char *)NULL);
		port_write_string(ci->cycle_port)(sc, buf, plen, ci->cycle_port);
	      }
	    else
	      {
		port_write_string(ci->cycle_port)(sc, ") ", 2, ci->cycle_port);
		object_to_port_with_circle_check(sc, val, ci->cycle_port, p_readable, ci);
		port_write_string(ci->cycle_port)(sc, ") ", 2, ci->cycle_port);
	      }}}}
  else
    {
      if (((!is_typed_hash_table(hash)) && (!is_pair(hash_table_procedures(hash))) && (!hash_chosen(hash))) || (use_write != p_readable))
	{
	  if (is_weak_hash_table(hash))
	    port_write_string(port)(sc, "(weak-hash-table", 16, port);
	  else port_write_string(port)(sc, "(hash-table", 11, port);
	}
      else
	{
	  hash_table_procedures_to_port(sc, hash, port, true, ci);
	  port_write_character(port)(sc, ')', port);
	  port_write_string(port)(sc, ") (copy (hash-table", 19, port);
	  copied = true;
	}
      for (s7_int i = 0; i < len; i++)
	{
	  const s7_pointer key_val = hash_table_iterate(sc, iterator);
	  if (key_val == sc->iterator_at_end_value) break; /* key_val can be #<eof> if hash is a weak-hash-table, and a GC happens during this loop */
	  port_write_character(port)(sc, ' ', port);
	  if ((use_write != p_readable) && (use_write != p_code) && (is_normal_symbol(car(key_val))))
	    port_write_character(port)(sc, '\'', port);
	  object_to_port_with_circle_check(sc, car(key_val), port, not_p_display(use_write), ci);
	  port_write_character(port)(sc, ' ', port);
	  object_to_port_with_circle_check(sc, cdr(key_val), port, not_p_display(use_write), ci);
	}
      if (use_write != p_readable)
	{
	  if (too_long)
	    port_write_string(port)(sc, " ...)", 5, port);
	  else port_write_character(port)(sc, ')', port);
	}}

  if (use_write == p_readable)
    {
      if (copied)
	{
	  if (!letd)
	    {
	      char buf[128];
	      int32_t plen = catstrs_direct(buf, ") <", pos_int_to_str_direct(sc, href), ">", (const char *)NULL);
	      port_write_string(port)(sc, buf, plen, port);
	    }
	  else port_write_string(port)(sc, ") <h>))", 7, port);
	}
      else
	if (letd)
	  port_write_string(port)(sc, ") <h>)", 6, port);
	else port_write_character(port)(sc, ')', port);

      if ((is_immutable_hash_table(hash)) && (!hash_cyclic) && (!is_typed_hash_table(hash)))
	port_write_character(port)(sc, ')', port);

      if ((!immut) && (is_immutable_hash_table(hash)) && (!hash_cyclic))
	port_write_string(port)(sc, ") (immutable! <h>))", 19, port);
    }
  s7_gc_unprotect_at(sc, gc_iter);
  iterator_carrier(iterator) = sc->nil;
}

static void slot_list_to_port(s7_scheme *sc, s7_pointer slot, s7_pointer port, shared_info_t *ci, bool bindings) /* bindings=let/inlet choice */
{
  bool first_time = true;
  for (; is_not_slot_end(slot); slot = next_slot(slot))
    {
      if (bindings)
	{
	  if (first_time)
	    {
	      port_write_character(port)(sc, '(', port);
	      first_time = false;
	    }
	  else port_write_string(port)(sc, " (", 2, port);
	}
      else port_write_character(port)(sc, ' ', port);
      symbol_to_port(sc, slot_symbol(slot), port, (bindings) ? p_display : p_key, NULL);  /* (object->string (inlet (symbol "(\")") 1) :readable) */
      port_write_character(port)(sc, ' ', port);
      object_to_port_with_circle_check(sc, slot_value(slot), port, p_readable, ci);
      if (bindings) port_write_character(port)(sc, ')', port);
    }
}

static void slot_list_to_port_with_cycle(s7_scheme *sc, s7_pointer obj, s7_pointer slot, s7_pointer port, shared_info_t *ci, bool bindings)
{
  bool first_time = true;
  for (; is_not_slot_end(slot); slot = next_slot(slot))
    {
      const s7_pointer sym = slot_symbol(slot), val = slot_value(slot);
      if (bindings)
	{
	  if (first_time)
	    {
	      port_write_character(port)(sc, '(', port);
	      first_time = false;
	    }
	  else port_write_string(port)(sc, " (", 2, port);
	}
      else port_write_character(port)(sc, ' ', port);
      symbol_to_port(sc, sym, port, (bindings) ? p_display : p_key, NULL);
      if (has_structure(val))
	{
	  char buf[128];
	  int32_t symref;
	  int32_t len = catstrs_direct(buf, "  (set! (<", pos_int_to_str_direct(sc, -peek_shared_ref(ci, obj)), "> ", (const char *)NULL);
	  port_write_string(port)(sc, " #f", 3, port);
	  port_write_string(ci->cycle_port)(sc, buf, len, ci->cycle_port);
	  symbol_to_port(sc, sym, ci->cycle_port, p_key, NULL);

	  symref = peek_shared_ref(ci, val);
	  if (symref != 0)
	    {
	      if (symref < 0) symref = -symref;
	      len = catstrs_direct(buf, ") <", pos_int_to_str_direct(sc, symref), ">) ", (const char *)NULL);
	      port_write_string(ci->cycle_port)(sc, buf, len, ci->cycle_port);
	    }
	  else
	    {
	      port_write_string(ci->cycle_port)(sc, ") ", 2, ci->cycle_port);
	      object_to_port_with_circle_check(sc, val, ci->cycle_port, p_readable, ci);
	      port_write_string(ci->cycle_port)(sc, ") ", 2, ci->cycle_port);
	    }}
      else
	{
	  port_write_character(port)(sc, ' ', port);
	  object_to_port_with_circle_check(sc, val, port, p_readable, ci);
	}
      if (bindings) port_write_character(port)(sc, ')', port);
      if (is_immutable(obj))
	{
	  char buf[128];
	  int32_t len = catstrs_direct(buf, "  (immutable! <", pos_int_to_str_direct(sc, -peek_shared_ref(ci, obj)), ">) ", (const char *)NULL);
	  port_write_string(ci->cycle_port)(sc, buf, len, ci->cycle_port);
	}}
}

static bool let_has_setter(s7_pointer obj)
{
  for (s7_pointer slot = let_slots(obj); is_not_slot_end(slot); slot = next_slot(slot))
    if ((slot_has_setter(slot)) || (is_immutable_slot(slot)))
      return(true);
  return(false);
}

static bool slot_setters_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, shared_info_t *ci)
{
  bool spaced_out = false;
  for (s7_pointer slot = let_slots(obj); is_not_slot_end(slot); slot = next_slot(slot))
    if (slot_has_setter(slot))
      {
	if (spaced_out) port_write_character(port)(sc, ' ', port); else spaced_out = true;
	port_write_string(port)(sc, "(set! (setter '", 15, port);
	symbol_to_port(sc, slot_symbol(slot), port, p_display, NULL);
	port_write_string(port)(sc, ") ", 2, port);
	object_to_port_with_circle_check(sc, slot_setter(slot), port, p_readable, ci);
	port_write_character(port)(sc, ')', port);
      }
  return(spaced_out);
}

static void immutable_slots_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, bool spaced_out)
{
  for (s7_pointer slot = let_slots(obj); is_not_slot_end(slot); slot = next_slot(slot))
    if (is_immutable_slot(slot))
      {
	if (spaced_out) port_write_character(port)(sc, ' ', port); else spaced_out = true;
	port_write_string(port)(sc, "(immutable! '", 13, port);
	symbol_to_port(sc, slot_symbol(slot), port, p_display, NULL);
	port_write_character(port)(sc, ')', port);
      }
}

static void slot_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  /* the slot symbol might need (symbol...) in which case we don't want the preceding quote */
  symbol_to_port(sc, slot_symbol(obj), port, p_readable, NULL);
  port_write_character(port)(sc, ' ', port);
  object_to_port_with_circle_check(sc, slot_value(obj), port, use_write, ci);
}

static void internal_slot_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  /* here we're displaying a slot in the debugger -- T_SLOT objects are not directly accessible in scheme */
  port_write_string(port)(sc, "#<slot: ", 8, port);
  symbol_to_port(sc, slot_symbol(obj), port, p_display, NULL);
  port_write_character(port)(sc, ' ', port);
  object_to_port_with_circle_check(sc, slot_value(obj), port, use_write, ci);
  port_write_character(port)(sc, '>', port);
}

static void let_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  /* if outer let points to (say) method list, the object needs to specialize object->string itself */
  if ((!sc->short_print) && (has_active_methods(sc, obj))) /* short_print 14-Dec-24 from stacktrace (see below) */
    {
      const s7_pointer print_func = find_method(sc, obj, sc->object_to_string_symbol);
      if (print_func != sc->undefined)
	{
	  s7_pointer str;
	  /* what needs to be protected here? for one, the function might not return a string! */

	  clear_has_methods(obj);
	  if ((use_write == p_write) || (use_write == p_code))
	    str = s7_apply_function(sc, print_func, set_plist_1(sc, obj));
	  else str = s7_apply_function(sc, print_func, set_plist_2(sc, obj, (use_write == p_display) ? sc->F : sc->readable_keyword));
	  set_has_methods(obj);

	  if ((is_string(str)) &&
	      (string_length(str) > 0))
	    port_write_string(port)(sc, string_value(str), string_length(str), port);
	  return;
	}}
  if (obj == sc->rootlet) {port_write_string(port)(sc, "(rootlet)", 9, port); return;}
  if (obj == sc->starlet) {port_write_string(port)(sc, "*s7*", 4, port);      return;}
  /* if (is_unlet(obj))   {port_write_string(port)(sc, "(unlet)", 7, port);   return;} */ /* this is the let created by (unlet), not sc->unlet_entries */
  if (sc->short_print)    {port_write_string(port)(sc, "#<let>", 6, port);    return;}

  /* circles can happen here: (let ((b #f)) (set! b (curlet)) (curlet)): #1=#<let 'b #1#> */
  if (use_write == p_readable)
    {
      int32_t lref;
      if ((ci) &&
	  (is_cyclic(obj)) &&
	  ((lref = peek_shared_ref(ci, obj)) != 0))
	{
	  if (lref < 0) lref = -lref;
	  if ((ci->defined[lref]) || (port == ci->cycle_port))
	    {
	      char buf[128];
	      int32_t len = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, lref), ">", (const char *)NULL);
	      port_write_string(ci->cycle_port)(sc, buf, len, ci->cycle_port);
	      return;
	    }
	  if (let_outlet(obj) != sc->rootlet)
	    {
	      char buf[128];
	      int32_t len = (int32_t)catstrs_direct(buf, "  (set! (outlet <", pos_int_to_str_direct(sc, lref), ">) ", (const char *)NULL);
	      port_write_string(ci->cycle_port)(sc, buf, len, ci->cycle_port);
	      let_to_port(sc, let_outlet(obj), ci->cycle_port, use_write, ci);
	      port_write_string(ci->cycle_port)(sc, ") ", 2, ci->cycle_port);
	    }
	  if (is_openlet(obj))
	    port_write_string(port)(sc, "(openlet ", 9, port);
	  /* not immutable here because we'll need to set the let fields below, then declare it immutable */
	  if (let_has_setter(obj))           /* both explicit setters and immutable slots */
	    {
	      port_write_string(port)(sc, "(let (", 6, port);
	      slot_list_to_port_with_cycle(sc, obj, let_slots(obj), port, ci, true);
	      port_write_string(port)(sc, ") ", 2, port);
	      immutable_slots_to_port(sc, obj, port, slot_setters_to_port(sc, obj, port, ci));
	      port_write_string(port)(sc, " (curlet))", 10, port);
	    }
	  else
	    {
	      port_write_string(port)(sc, "(inlet", 6, port);
	      slot_list_to_port_with_cycle(sc, obj, let_slots(obj), port, ci, false);
	      port_write_character(port)(sc, ')', port);
	    }
	  if (is_openlet(obj))
	    port_write_character(port)(sc, ')', port);
	}
      else
	{
	  if (is_openlet(obj))
	    port_write_string(port)(sc, "(openlet ", 9, port);
	  if (is_immutable_let(obj))
	    port_write_string(port)(sc, "(immutable! ", 12, port);

	  /* this ignores outlet -- but is that a problem? */
	  /* (object->string (let ((i 0)) (set! (setter 'i) integer?) (curlet)) :readable) -> "(let ((i 0)) (set! (setter 'i) #_integer?) (curlet))" */
	  if (let_has_setter(obj))
	    {
	      port_write_string(port)(sc, "(let (", 6, port);
	      slot_list_to_port(sc, let_slots(obj), port, ci, true);
	      port_write_string(port)(sc, ") ", 2, port);
	      immutable_slots_to_port(sc, obj, port, slot_setters_to_port(sc, obj, port, ci));
	      /* perhaps set outlet here?? */
	      port_write_string(port)(sc, " (curlet))", 10, port);
	    }
	  else
	    {
	      if (let_outlet(obj) != sc->rootlet)
		{
		  int32_t ref;
		  port_write_string(port)(sc, "(sublet ", 8, port);
		  if ((ci) && ((ref = peek_shared_ref(ci, let_outlet(obj))) < 0))
		    {
		      char buf[128];
		      int32_t len = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, -ref), ">", (const char *)NULL);
		      port_write_string(port)(sc, buf, len, port);
		    }
		  else
		    {
		      s7_pointer name = let_ref_p_pp(sc, obj, sc->class_name_symbol);
		      if (is_symbol(name))
			symbol_to_port(sc, name, port, p_display, NULL);
		      else let_to_port(sc, let_outlet(obj), port, use_write, ci);
		    }}
	      else port_write_string(port)(sc, "(inlet", 6, port);
	      slot_list_to_port(sc, let_slots(obj), port, ci, false);
	      port_write_character(port)(sc, ')', port);
	    }
	  if (is_immutable_let(obj))
	    port_write_character(port)(sc, ')', port);
	  if (is_openlet(obj))
	    port_write_character(port)(sc, ')', port);
	}}
  else /* not readable write */
    {
      s7_pointer slot = let_slots(obj);
      port_write_string(port)(sc, "(inlet", 6, port);
      for (int32_t i = 1; is_not_slot_end(slot); i++, slot = next_slot(slot))
	{
	  port_write_character(port)(sc, ' ', port);
	  slot_to_port(sc, slot, port, use_write, ci);
	  if ((is_not_slot_end(next_slot(slot))) && (i == sc->print_length))
	    {
	      port_write_string(port)(sc, " ...", 4, port);
	      break;
	    }}
      port_write_character(port)(sc, ')', port);
    }
}

static void write_macro_readably(s7_scheme *sc, s7_pointer obj, s7_pointer port)
{
  const s7_pointer body = closure_body(obj), parlist = closure_pars(obj);
  /* this doesn't handle recursive macros well -- we need letrec or the equivalent as in write_closure_readably */
  /*   (letrec ((m2 (macro (x) `(if (> ,x 0) (m2 (- ,x 1)) 32)))) (object->string m2 :readable)) */

  port_write_string(port)(sc, (is_either_macro(obj)) ? "(macro" : "(bacro", 6, port);
  if ((is_macro_star(obj)) || (is_bacro_star(obj)))
    port_write_character(port)(sc, '*', port);
  if (is_symbol(parlist))
    {
      port_write_character(port)(sc, ' ', port);
      port_write_string(port)(sc, symbol_name(parlist), symbol_name_length(parlist), port);
      port_write_character(port)(sc, ' ', port);
    }
  else
    if (is_pair(parlist))
      {
	s7_pointer pars;
	port_write_string(port)(sc, " (", 2, port);
	for (pars = parlist; is_pair(pars); pars = cdr(pars))
	  {
	    object_to_port(sc, car(pars), port, p_write, NULL);
	    if (is_pair(cdr(pars)))
	      port_write_character(port)(sc, ' ', port);
	  }
	if (!is_null(pars))
	  {
	    port_write_string(port)(sc, " . ", 3, port);
	    object_to_port(sc, pars, port, p_write, NULL);
	  }
	port_write_string(port)(sc, ") ", 2, port);
      }
    else port_write_string(port)(sc, " () ", 4, port);

  for (s7_pointer expr = body; is_pair(expr); expr = cdr(expr))
    object_to_port(sc, car(expr), port, p_write, NULL);
  port_write_character(port)(sc, ')', port);
}


static s7_pointer match_symbol(const s7_pointer symbol, s7_pointer let)
{
  for (s7_pointer le = let; le; le = let_outlet(le))
    for (s7_pointer slot = let_slots(le); is_not_slot_end(slot); slot = next_slot(slot))
      if (slot_symbol(slot) == symbol)
	return(slot);
  return(NULL);
}

static bool slot_memq(const s7_pointer symbol, s7_pointer symbols)
{
  for (s7_pointer syms = symbols; is_pair(syms); syms = cdr(syms))
    if (slot_symbol(car(syms)) == symbol)
      return(true);
  return(false);
}

static bool arg_memq(const s7_pointer symbol, s7_pointer args)
{
  for (s7_pointer p = args; is_pair(p); p = cdr(p))
    if ((car(p) == symbol) ||
	((is_pair(car(p))) &&
	 (caar(p) == symbol)))
      return(true);
  return(false);
}

static void collect_symbol(s7_scheme *sc, s7_pointer sym, s7_pointer let, s7_pointer args, s7_int gc_loc)
{
  if ((!arg_memq(T_Sym(sym), args)) &&
      (!slot_memq(sym, gc_protected_at(sc, gc_loc))))
    {
      s7_pointer slot = match_symbol(sym, let);
      if (slot)
	gc_protected_at(sc, gc_loc) = cons(sc, slot, gc_protected_at(sc, gc_loc));
    }
}

static void collect_locals(s7_scheme *sc, s7_pointer body, s7_pointer let, s7_pointer args, s7_int gc_loc) /* currently called only in write_closure_readably */
{
  if (is_unquoted_pair(sc, body))
    {
      collect_locals(sc, car(body), let, args, gc_loc);
      collect_locals(sc, cdr(body), let, args, gc_loc);
    }
  else
    if (is_symbol(body))
      collect_symbol(sc, body, let, args, gc_loc);
}

static void collect_specials(s7_scheme *sc, s7_pointer let, s7_pointer args, s7_int gc_loc)
{
  collect_symbol(sc, sc->local_signature_symbol, let, args, gc_loc);
  collect_symbol(sc, sc->local_setter_symbol, let, args, gc_loc);
  collect_symbol(sc, sc->local_documentation_symbol, let, args, gc_loc);
  collect_symbol(sc, sc->local_iterator_symbol, let, args, gc_loc);
}

s7_pointer find_closure(s7_scheme *sc, s7_pointer closure, s7_pointer current_let)
{
  for (s7_pointer let = current_let; let; let = let_outlet(let))
    {
      if ((is_funclet(let)) || (is_maclet(let)))
	{
	  s7_pointer sym = funclet_function(let);
	  s7_pointer func = s7_symbol_local_value(sc, sym, let);
	  if (func == closure)
	    return(sym);
	}
      for (s7_pointer slot = let_slots(let); is_not_slot_end(slot); slot = next_slot(slot))
	if (slot_value(slot) == closure)
	  return(slot_symbol(slot));
    }
  if ((is_any_macro(closure)) && /* can't be a c_macro here */
      (has_pair_macro(closure))) /* maybe macro never called, so no maclet exists */
    return(pair_macro(closure_body(closure)));
  return(sc->nil);
}

static void write_closure_name(s7_scheme *sc, s7_pointer closure, s7_pointer port)
{
  {
    s7_pointer sym = find_closure(sc, closure, closure_let(closure));
    if (is_symbol(sym))
      {
	port_write_string(port)(sc, symbol_name(sym), symbol_name_length(sym), port);
	return;
      }}
  switch (type(closure))
    {
    case T_CLOSURE:      port_write_string(port)(sc, "#<lambda ", 9, port);   break;
    case T_CLOSURE_STAR: port_write_string(port)(sc, "#<lambda* ", 10, port); break;
    case T_BACRO:        port_write_string(port)(sc, "#<bacro ", 8, port);    break;
    case T_BACRO_STAR:   port_write_string(port)(sc, "#<bacro* ", 9, port);   break;

    case T_MACRO:
      if (is_expansion(closure))
	port_write_string(port)(sc, "#<expansion ", 12, port);
      else port_write_string(port)(sc, "#<macro ", 8, port);
      break;

    case T_MACRO_STAR:
      if (is_expansion(closure))
	port_write_string(port)(sc, "#<expansion* ", 13, port);
      else port_write_string(port)(sc, "#<macro* ", 9, port);
      break;
    }

  if (is_null(closure_pars(closure)))
    port_write_string(port)(sc, "()>", 3, port);
  else
    {
      s7_pointer pars = closure_pars(closure);
      if (is_symbol(pars))
	{
	  port_write_string(port)(sc, symbol_name(pars), symbol_name_length(pars), port);
	  port_write_character(port)(sc, '>', port);    /* (lambda a a) -> #<lambda a> */
	}
      else
	{
	  s7_pointer sym = car(pars);
	  if (is_pair(sym)) sym = car(sym);
	  port_write_character(port)(sc, '(', port);
	  port_write_string(port)(sc, symbol_name(sym), symbol_name_length(sym), port);
	  if (!is_null(cdr(pars)))
	    {
	      s7_pointer par;
	      port_write_character(port)(sc, ' ', port);
	      if (is_pair(cdr(pars)))
		{
		  par = cadr(pars);
		  if (is_pair(par))
		    par = car(par);
		  else
		    if (par == sc->rest_keyword)
		      {
			port_write_string(port)(sc, ":rest ", 6, port);
			pars = cdr(pars);
			par = cadr(pars);
			if (is_pair(par)) par = car(par);
		      }}
	      else
		{
		  port_write_string(port)(sc, ". ", 2, port);
		  par = cdr(pars);
		}
	      port_write_string(port)(sc, symbol_name(par), symbol_name_length(par), port);
	      if ((is_pair(cdr(pars))) &&
		  (!is_null(cddr(pars))))
		port_write_string(port)(sc, " ...", 4, port);
	    }
	  port_write_string(port)(sc, ")>", 2, port);
	}}
}

s7_pointer closure_name(s7_scheme *sc, s7_pointer closure)
{
  /* this is used by the error handlers to get the current function name */
  s7_pointer sym = find_closure(sc, closure, sc->curlet);
  if (is_symbol(sym))
    return(sym);
  if (is_pair(current_code(sc)))
    return(current_code(sc));
  return(closure); /* desperation -- the parameter list (caar here) will cause endless confusion in OP_APPLY errors! */
}


static void write_closure_readably_1(s7_scheme *sc, s7_pointer obj, s7_pointer arglist, s7_pointer body, s7_pointer port)
{
  const s7_int old_print_length = sc->print_length;

  if (type(obj) == T_CLOSURE_STAR)
    port_write_string(port)(sc, "(lambda* ", 9, port);
  else port_write_string(port)(sc, "(lambda ", 8, port);

  if ((is_pair(arglist)) &&
      (allows_other_keys(arglist)))
    {
      sc->temp7 = (is_null(cdr(arglist))) ? set_plist_2(sc, car(arglist), sc->allow_other_keys_keyword) :
	          ((is_null(cddr(arglist))) ? set_plist_3(sc, car(arglist), cadr(arglist), sc->allow_other_keys_keyword) :
	                                      pair_append(sc, arglist, list_1(sc, sc->allow_other_keys_keyword)));
      object_to_port(sc, sc->temp7, port, p_write, NULL);
      sc->temp7 = sc->unused;
    }
  else object_to_port(sc, arglist, port, p_write, NULL); /* here we just want the straight output (a b) not (list 'a 'b) */

  sc->print_length = 1048576;
  for (s7_pointer p = body; is_pair(p); p = cdr(p))
    {
      port_write_character(port)(sc, ' ', port);
      object_to_port(sc, car(p), port, p_write, NULL);
    }
  port_write_character(port)(sc, ')', port);
  sc->print_length = old_print_length;
}

static void write_closure_readably(s7_scheme *sc, s7_pointer obj, s7_pointer port, shared_info_t *ci)
{
  const s7_pointer body = closure_body(obj);
  s7_pointer parlist = closure_pars(obj);
  s7_pointer pe, local_slots, setter = NULL, obj_slot = NULL;
  s7_int gc_loc;
  bool sent_let = false, sent_letrec = false;

  if (sc->safety > no_safety)
    {
      if (tree_is_cyclic(sc, body))
	{
	  port_write_string(port)(sc, "#<write_closure_readably: body is cyclic>", 41, port); /* not s7_error here! */
	  return;
	}
      if ((!ci) && (is_pair(parlist)))
	{ /* (format #f "~W" (make-hook (let ((cp (list 1))) (set-cdr! cp cp) (list 'quote cp)))) */
	  shared_info_t *new_ci = make_shared_info(sc);
	  clear_shared_info(new_ci);
	  if (collect_shared_info(sc, new_ci, parlist, false))
	    {
	      free_shared_info(new_ci);
	      port_write_string(port)(sc, "#<write_closure_readably: parameter list is cyclic>", 51, port); /* not s7_error here! */
	      return;
	    }
	  free_shared_info(new_ci);
	}}
  if (is_symbol(parlist)) parlist = set_dlist_1(sc, parlist);
  pe = closure_let(obj);

  gc_loc = gc_protect_1(sc, sc->nil);
  collect_locals(sc, body, pe, parlist, gc_loc);   /* collect locals used only here (and below) */
  collect_specials(sc, pe, parlist, gc_loc);

  if (s7_is_dilambda(obj))
    {
      setter = closure_setter(obj);
      if (has_closure_let(setter))                 /* collect args etc so need the parameter list */
	{
	  parlist = closure_pars(setter);
	  if (is_symbol(parlist)) parlist = set_dlist_1(sc, parlist);
	  collect_locals(sc, closure_body(setter), pe, parlist, gc_loc);
	}}

  local_slots = T_Lst(gc_protected_at(sc, gc_loc)); /* possibly a list of slots */
  if (!is_null(local_slots))
    {
      /* if (let|letrec ((f (lambda () f))) (object->string f :readable)), local_slots: ('f f) */
      /* but we can't handle it below because that leads to an infinite loop */
      for (s7_pointer slots = local_slots; is_pair(slots); slots = cdr(slots))
	{
	  const s7_pointer slot = car(slots);
	  if ((!is_any_closure(slot_value(slot))) &&    /* mutually referencing closures? ./snd -l snd-test 24 hits this in the effects dialogs */
	      ((!has_structure(slot_value(slot))) ||    /* see s7test example, vector has closure that refers to vector */
	       (slot_symbol(slot) == sc->local_signature_symbol)))
	    {
	      if (!sent_let)
		{
		  port_write_string(port)(sc, "(let (", 6, port);
		  sent_let = true;
		}
	      port_write_character(port)(sc, '(', port);
	      port_write_string(port)(sc, symbol_name(slot_symbol(slot)), symbol_name_length(slot_symbol(slot)), port);
	      port_write_character(port)(sc, ' ', port);
	      /* (object->string (list (let ((local 1)) (lambda (x) (+ x local)))) :readable) */
	      object_to_port(sc, slot_value(slot), port, p_readable, NULL);
	      if (is_null(cdr(slots)))
		port_write_character(port)(sc, ')', port);
	      else port_write_string(port)(sc, ") ", 2, port);
	    }}
      if (sent_let) port_write_string(port)(sc, ") ", 2, port);
    }

  /* now we need to know if obj is in the closure_let via letrec, and if so, send out letrec+obj name+def below, then close it with obj-name??
   *  the two cases are: (let ((f (lambda () f)))...) which is ok now, and (letrec ((f (lambda () f)))...) which needs the letrec
   */
  if (!is_null(local_slots))
    for (s7_pointer slots = local_slots; is_pair(slots); slots = cdr(slots))
      {
	const s7_pointer slot = car(slots);
	if ((is_any_closure(slot_value(slot))) &&
	    (slot_value(slot) == obj))
	  {
	    port_write_string(port)(sc, "(letrec ((", 10, port); /* (letrec ((f (lambda () f))) f) */
	    sent_letrec = true;
	    port_write_string(port)(sc, symbol_name(slot_symbol(slot)), symbol_name_length(slot_symbol(slot)), port);
	    port_write_character(port)(sc, ' ', port);
	    obj_slot = slot;
	    break;
	  }}

  if (setter)
    port_write_string(port)(sc, "(dilambda ", 10, port);
  write_closure_readably_1(sc, obj, closure_pars(obj), body, port);
  if (setter)
    {
      port_write_character(port)(sc, ' ', port);
      if (has_closure_let(setter))
	write_closure_readably_1(sc, setter, closure_pars(setter), closure_body(setter), port);
      else object_to_port_with_circle_check(sc, setter, port, p_readable, ci);
      port_write_character(port)(sc, ')', port);
    }
  if (sent_letrec)
    {
      port_write_string(port)(sc, ")) ", 3, port);
      port_write_string(port)(sc, symbol_name(slot_symbol(obj_slot)), symbol_name_length(slot_symbol(obj_slot)), port);
      port_write_character(port)(sc, ')', port);
    }
  if (sent_let)
    port_write_character(port)(sc, ')', port);
  s7_gc_unprotect_at(sc, gc_loc);
}

static void iterator_hash_table_to_port(s7_scheme *sc, s7_pointer port, s7_pointer table)
{
  if (is_weak_hash_table(table))
    port_write_string(port)(sc, "(make-iterator (weak-hash-table))", 33, port);
  else port_write_string(port)(sc, "(make-iterator (hash-table))", 28, port);
}

static void iterator_to_port(s7_scheme *sc, s7_pointer iter, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  if (use_write == p_readable)
    {
      if (iterator_is_at_end(iter))
	{
	  switch (type(iterator_sequence(iter)))
	    {
	    case T_NIL:
	    case T_PAIR:           port_write_string(port)(sc, "(make-iterator ())", 18, port);	     break;
	    case T_STRING:         port_write_string(port)(sc, "(make-iterator \"\")", 18, port);    break;
	    case T_BYTE_VECTOR:    port_write_string(port)(sc, "(make-iterator #u())", 20, port);    break;
	    case T_VECTOR:         port_write_string(port)(sc, "(make-iterator #())", 19, port);     break;
	    case T_INT_VECTOR:	   port_write_string(port)(sc, "(make-iterator #i())", 20, port);    break;
	    case T_FLOAT_VECTOR:   port_write_string(port)(sc, "(make-iterator #r())", 20, port);    break;
	    case T_COMPLEX_VECTOR: port_write_string(port)(sc, "(make-iterator #c())", 20, port);    break;
	    case T_LET:	           port_write_string(port)(sc, "(make-iterator (inlet))", 23, port); break;
	    case T_HASH_TABLE:     iterator_hash_table_to_port(sc, port, iterator_sequence(iter));   break;
	    default:
	      port_write_string(port)(sc, "(make-iterator ())", 18, port);
	      break; /* c-object?? function? */
	    }}
      else
	{
	  const s7_pointer seq = iterator_sequence(iter);
	  int32_t iter_ref;
	  if ((ci) &&
	      (is_cyclic(iter)) &&
	      ((iter_ref = peek_shared_ref(ci, iter)) != 0))
	    {
	      /* basically the same as c_pointer_to_port */
	      if (!is_cyclic_set(iter))
		{
		  int32_t nlen;
		  char buf[128];
		  if (iter_ref < 0) iter_ref = -iter_ref;

		  if (ci->init_port == sc->F)
		    {
		      ci->init_port = s7_open_output_string(sc);
		      ci->init_loc = gc_protect_1(sc, ci->init_port);
		    }
		  port_write_string(port)(sc, "#f", 2, port);
		  nlen = (int32_t)catstrs_direct(buf, "  (set! <", pos_int_to_str_direct(sc, iter_ref), "> (make-iterator ", (const char *)NULL);
		  port_write_string(ci->init_port)(sc, buf, nlen, ci->init_port);

		  flip_ref(ci, seq);
		  object_to_port_with_circle_check(sc, seq, ci->init_port, use_write, ci);
		  flip_ref(ci, seq);

		  port_write_string(ci->init_port)(sc, "))\n", 3, ci->init_port);
		  set_cyclic_set(iter);
		  return;
		}}

	  if (is_string(seq))
	    {
	      const s7_int len = string_length(seq) - iterator_position(iter);
	      if (len == 0)
		port_write_string(port)(sc, "(make-iterator \"\")", 18, port);
	      else
		{
		  const char *iter_str = (const char *)(string_value(seq) + iterator_position(iter));
		  port_write_string(port)(sc, "(make-iterator \"", 16, port);
		  if (!string_needs_slashification((const uint8_t *)iter_str, len))
		    port_write_string(port)(sc, iter_str, len, port);
		  else slashify_string_to_port(sc, port, iter_str, len, NOT_IN_QUOTES);
		  port_write_string(port)(sc, "\")", 2, port);
		}}
	  else
	    {
	      if (is_pair(seq))
		{
		  port_write_string(port)(sc, "(make-iterator ", 15, port);
		  object_to_port_with_circle_check(sc, iterator_current(iter), port, use_write, ci);
		  port_write_character(port)(sc, ')', port);
		}
	      else
		{
		  if ((is_let(seq)) && (seq != sc->rootlet) && (seq != sc->starlet))
		    {
		      port_write_string(port)(sc, "(let ((iter (make-iterator ", 27, port);
		      object_to_port_with_circle_check(sc, seq, port, use_write, ci);
		      port_write_string(port)(sc, "))) ", 4, port);
		      for (s7_pointer slot = let_slots(seq); slot != let_iterator_slot(iter); slot = next_slot(slot))
			port_write_string(port)(sc, "(iter) ", 7, port);
		      port_write_string(port)(sc, "iter)", 5, port);
		    }
		  else
		    {
		      if (iterator_position(iter) > 0)
			port_write_string(port)(sc, "(let ((iter (make-iterator ", 27, port);
		      else port_write_string(port)(sc, "(make-iterator ", 15, port);
		      object_to_port_with_circle_check(sc, seq, port, use_write, ci);
		      if (iterator_position(iter) > 0)
			{
			  if (iterator_position(iter) == 1)
			    port_write_string(port)(sc, "))) (iter) iter)", 16, port);
			  else
			    {
			      char str[128];
			      int32_t nlen = (int32_t)catstrs_direct(str, "))) (do ((i 0 (+ i 1))) ((= i ",
								     pos_int_to_str_direct(sc, iterator_position(iter)),
								     ") iter) (iter)))", (const char *)NULL);
			      port_write_string(port)(sc, str, nlen, port);
			    }}
		      else port_write_character(port)(sc, ')', port);
		    }}}}}
  else
    {
      const char *str;
      if ((is_hash_table(iterator_sequence(iter))) && (is_weak_hash_table(iterator_sequence(iter))))
	str = "weak-hash-table";
      else
	if (iterator_sequence(iter) == sc->starlet)
	  str = "*s7*";
	else str = type_name(sc, iterator_sequence(iter), no_article);
      port_write_string(port)(sc, "#<iterator: ", 12, port);
      port_write_string(port)(sc, str, safe_strlen(str), port);
      port_write_character(port)(sc, '>', port);
    }
}

static void c_pointer_to_port(s7_scheme *sc, s7_pointer cptr, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  #define CP_BUFSIZE 128
  char buf[CP_BUFSIZE];
  int32_t nlen;
  if (use_write == p_readable)
    {
      int32_t ref;
      if ((ci) &&
	  (is_cyclic(cptr)) &&
	  ((ref = peek_shared_ref(ci, cptr)) != 0))
	{
	  port_write_string(port)(sc, "#f", 2, port);
	  if (!is_cyclic_set(cptr))
	    {
	      if (ci->init_port == sc->F)
		{
		  ci->init_port = s7_open_output_string(sc);
		  ci->init_loc = gc_protect_1(sc, ci->init_port);
		}
	      nlen = snprintf(buf, CP_BUFSIZE, "  (set! <%d> (c-pointer %" p64, -ref, (intptr_t)c_pointer(cptr));
	      port_write_string(ci->init_port)(sc, buf, nlen, ci->init_port);

	      if ((c_pointer_type(cptr) != sc->F) ||
		  (c_pointer_info(cptr) != sc->F))
		{
		  flip_ref(ci, c_pointer_type(cptr));

		  port_write_character(ci->init_port)(sc, ' ', ci->init_port);
		  object_to_port_with_circle_check(sc, c_pointer_type(cptr), ci->init_port, use_write, ci);

		  flip_ref(ci, c_pointer_type(cptr));
		  flip_ref(ci, c_pointer_info(cptr));

		  port_write_character(ci->init_port)(sc, ' ', ci->init_port);
		  object_to_port_with_circle_check(sc, c_pointer_info(cptr), ci->init_port, use_write, ci);

		  flip_ref(ci, c_pointer_info(cptr));
		}
	      port_write_string(ci->init_port)(sc, "))\n", 3, ci->init_port);
	      set_cyclic_set(cptr);
	    }}
      else
	{
	  nlen = snprintf(buf, CP_BUFSIZE, "(c-pointer %" p64, (intptr_t)c_pointer(cptr));
	  port_write_string(port)(sc, buf, clamp_length(nlen, CP_BUFSIZE), port);
	  if ((c_pointer_type(cptr) != sc->F) ||
	      (c_pointer_info(cptr) != sc->F))
	    {
	      port_write_character(port)(sc, ' ', port);
	      object_to_port_with_circle_check(sc, c_pointer_type(cptr), port, use_write, ci);
	      port_write_character(port)(sc, ' ', port);
	      object_to_port_with_circle_check(sc, c_pointer_info(cptr), port, use_write, ci);
	    }
	  port_write_character(port)(sc, ')', port);
	}}
  else
    {
      if ((is_symbol(c_pointer_type(cptr))) &&
	  (symbol_name_length(c_pointer_type(cptr)) < (CP_BUFSIZE / 2)))
	nlen = snprintf(buf, CP_BUFSIZE, "#<%s %p>", symbol_name(c_pointer_type(cptr)), c_pointer(cptr));
      else nlen = snprintf(buf, CP_BUFSIZE, "#<c_pointer %p>", c_pointer(cptr));
      port_write_string(port)(sc, buf, clamp_length(nlen, CP_BUFSIZE), port);
    }
}

static void random_state_to_port(s7_scheme *sc, s7_pointer rs, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  #define B_BUFSIZE 128
  char buf[B_BUFSIZE];
  int32_t nlen;
  if (use_write == p_readable)
    nlen = snprintf(buf, B_BUFSIZE, "(random-state %" PRIu64 " %" PRIu64 ")", random_seed(rs), random_carry(rs));
  else nlen = snprintf(buf, B_BUFSIZE, "#<random-state %" PRIu64 " %" PRIu64 ">", random_seed(rs), random_carry(rs));
  port_write_string(port)(sc, buf, clamp_length(nlen, B_BUFSIZE), port);
}

static void display_fallback(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
#if S7_DEBUGGING
  print_debugging_state(sc, obj, port);
#else
  if (is_free(obj))
    port_write_string(port)(sc, "<free cell!>", 12, port);
  else port_write_string(port)(sc, "<unknown object!>", 17, port);
#endif
}

static void unique_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
  port_write_string(port)(sc, unique_name(obj), unique_name_length(obj), port);
}

static void undefined_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if ((obj != sc->undefined) && (use_write == p_readable))
    {
      port_write_string(port)(sc, "(with-input-from-string \"", 25, port);
      port_write_string(port)(sc, undefined_name(obj), undefined_name_length(obj), port);
      port_write_string(port)(sc, "\" read)", 7, port);
    }
  else port_write_string(port)(sc, undefined_name(obj), undefined_name_length(obj), port);
}

static void eof_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if (use_write == p_readable)
    port_write_string(port)(sc, "(begin #<eof>)", 14, port);
  else port_write_string(port)(sc, eof_name(obj), eof_name_length(obj), port);
}

static void counter_to_port(s7_scheme *sc, s7_pointer unused_obj, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
  port_write_string(port)(sc, "#<counter>", 10, port);
}

static void integer_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
  /* killer overhead here; breaking it into named/unnamed funcs helps only slightly -- still ridiculous overhead according to callgrind */
  const s7_int num = integer(obj);
  if ((num < 10) && (num >= 0))
    {
      static const char *ones[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
      if (is_string_port(port))
	{
	  if (port_position(port) + 1 < port_data_size(port))
	    {
	      memcpy((void *)(port_data(port) + port_position(port)), (void *)ones[num], 1);
	      port_position(port) += 1;
	    }
	  else string_write_string_resized(sc, ones[num], 1, port);
	}
      else port_write_string(port)(sc, ones[num], 1, port);
    }
  else
    {
      s7_int nlen = 0;
      const char *str = integer_to_string(sc, integer(obj), &nlen);
      port_write_string(port)(sc, str, nlen, port);
    }
}

static void number_to_port(s7_scheme *sc, s7_pointer num, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  s7_int nlen = 0;
  char *str = number_to_string_base_10(sc, num, 0, sc->float_format_precision, 'g', &nlen, use_write); /* was 14 */
  port_write_string(port)(sc, str, nlen, port);
}


static void syntax_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
  if (is_initial_value(obj))
    port_write_string(port)(sc, "#_", 2, port);
  port_display(port)(sc, symbol_name(syntax_symbol(obj)), port);
}

static void character_to_port(s7_scheme *sc, s7_pointer c, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if (use_write == p_display)
    port_write_unicode_char(sc, character(c), port);
  else port_write_string(port)(sc, character_name(c), character_name_length(c), port);
}

static void closure_to_port(s7_scheme *sc, s7_pointer func, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  if (has_active_methods(sc, func))
    {
      /* look for object->string method else fallback on ordinary case.
       * can't use recursion on closure_let here because then the fallback name is #<let>.
       * this is tricky!: (display (openlet (with-let (mock-c-pointer 0) (lambda () 1))))
       *   calls object->string on the closure whose closure_let is the mock-c-pointer;
       *   it has an object->string method that clears mock-c-pointers and tries again...
       *   so, display methods need to use coverlet/openlet.
       */
      const s7_pointer print_func = find_method(sc, closure_let(func), sc->object_to_string_symbol);
      if (print_func != sc->undefined)
	{
	  s7_pointer str = s7_apply_function(sc, print_func, set_plist_1(sc, func));
	  if (string_length(str) > 0)
	    port_write_string(port)(sc, string_value(str), string_length(str), port);
	  return;
	}}
  if (use_write == p_readable)
    write_closure_readably(sc, func, port, ci);
  else write_closure_name(sc, func, port);
}

static void macro_to_port(s7_scheme *sc, s7_pointer func, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if (has_active_methods(sc, func))
    {
      const s7_pointer print_func = find_method(sc, closure_let(func), sc->object_to_string_symbol);
      if (print_func != sc->undefined)
	{
	  s7_pointer str = s7_apply_function(sc, print_func, set_plist_1(sc, func));
	  if (string_length(str) > 0)
	    port_write_string(port)(sc, string_value(str), string_length(str), port);
	  return;
	}}
  if (use_write == p_readable)
    write_macro_readably(sc, func, port);
  else write_closure_name(sc, func, port);
}

static void c_function_to_port(s7_scheme *sc, s7_pointer func, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{                                                          /* includes c_function_star, so c_function_symbol can't be used */
  const s7_int len = c_function_name_length(func);

  if (is_string_port(port)) /* expand port_write_string -> string_write_string, 15 in tauto */
    {
      if (len > 0)
	{
	  if (port_position(port) + len + 2 < port_data_size(port))
	    {
	      if (is_initial_value(func))
		port_write_string(port)(sc, "#_", 2, port);
	      memcpy((void *)(port_data(port) + port_position(port)), (const void *)c_function_name(func), len);
	      port_position(port) += len;
	    }
	  else string_write_string_resized(sc, c_function_name(func), len, port);
	}
      else port_write_string(port)(sc, "#<c-function>", 13, port);
    }
  else
    if (len > 0)
      {
	if (is_initial_value(func))
	  port_write_string(port)(sc, "#_", 2, port);
	port_write_string(port)(sc, c_function_name(func), len, port);
      }
    else port_write_string(port)(sc, "#<c-function>", 13, port);
}

static void c_macro_to_port(s7_scheme *sc, s7_pointer func, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
  if (c_macro_name_length(func) > 0)
    {
      if (is_initial_value(func))
	port_write_string(port)(sc, "#_", 2, port);
      port_write_string(port)(sc, c_macro_name(func), c_macro_name_length(func), port);
    }
  else port_write_string(port)(sc, "#<c-macro>", 10, port);
}

/* (eval-string (object->string (call-with-exit (lambda (go) go)) :readable)) should at least be readable if use_write == p_readable,
 *   but the normal form "#<goto go>" gives a read-error due to the embedded space.  So if :readable, we return "#<goto::go>" which
 *   isn't going to do "the right thing", but at least it doesn't raise a read-error.
 */


static void goto_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *unused_ci)
{
  if (is_symbol(call_exit_name(obj)))
    {
      port_write_string(port)(sc, "#<goto", 6, port);
      port_write_string(port)(sc, (use_write == p_readable) ? "::" : " ", (use_write == p_readable) ? 2 : 1, port);
      symbol_to_port(sc, call_exit_name(obj), port, p_display, NULL);
      port_write_character(port)(sc, '>', port);
    }
  else port_write_string(port)(sc, "#<goto>", 7, port);
}

static void catch_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  port_write_string(port)(sc, "#<catch: ", 9, port);
  object_to_port(sc, catch_tag(obj), port, use_write, ci);
  port_write_character(port)(sc, '>', port);
}

static void dynamic_wind_to_port(s7_scheme *sc, s7_pointer unused_obj, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
  /* this can happen because (*s7* 'stack) can involve dynamic-wind markers */
  port_write_string(port)(sc, "#<dynamic-wind>", 15, port);
}

static void c_object_name_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port)
{
  port_write_string(port)(sc, string_value(c_object_scheme_name(sc, obj)), string_length(c_object_scheme_name(sc, obj)), port);
}

static void c_object_to_port(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
#if !DISABLE_DEPRECATED
  if (c_object_print(sc, obj))
    {
      char *str = ((*(c_object_print(sc, obj)))(sc, c_object_value(obj)));
      port_display(port)(sc, str, port);
      free(str);
      return;
    }
#endif
  if (c_object_to_string(sc, obj)) /* plist here and below can clobber args if SHOW_EVAL_ARGS */
    {
      set_mlist_2(sc, obj, (use_write == p_readable) ? sc->readable_keyword : ((use_write == p_write) ? sc->T : sc->F));
      port_display(port)(sc, s7_string((*(c_object_to_string(sc, obj)))(sc, sc->mlist_2)), port);
    }
  else
    {
      if ((use_write == p_readable) &&
	  (c_object_to_list(sc, obj)) &&  /* to_list and (implicit) set are needed to reconstruct a cyclic c-object, as well as the maker (via type name) */
	  (c_object_set(sc, obj)))
	{
	  int32_t href;
	  const s7_pointer old_w = sc->w;
	  const s7_pointer obj_list = ((*(c_object_to_list(sc, obj)))(sc, set_mlist_1(sc, obj)));
	  sc->w = obj_list;
	  if ((ci) &&
	      (is_cyclic(obj)) &&
	      ((href = peek_shared_ref(ci, obj)) != 0))
	    {
	      s7_pointer p = obj_list;
	      if (href < 0) href = -href;
	      if ((ci->defined[href]) || (port == ci->cycle_port))
		{
		  char buf[128];
		  int32_t nlen = catstrs_direct(buf, "<", pos_int_to_str_direct(sc, href), ">", (const char *)NULL);
		  port_write_string(port)(sc, buf, nlen, port);
		  return;
		}
	      port_write_character(port)(sc, '(', port);
	      c_object_name_to_port(sc, obj, port);
	      for (int32_t i = 0; is_pair(p); i++, p = cdr(p))
		{
		  s7_pointer val = car(p);
		  if (has_structure(val))
		    {
		      char buf[128];
		      int32_t symref;
		      int32_t len = (int32_t)catstrs_direct(buf, "  (set! (<", pos_int_to_str_direct(sc, href), "> ", pos_int_to_str_direct_1(sc, i), ") ", (const char *)NULL);
		      port_write_string(port)(sc, " #f", 3, port);
		      port_write_string(ci->cycle_port)(sc, buf, len, ci->cycle_port);
		      symref = peek_shared_ref(ci, val);
		      if (symref != 0)
			{
			  if (symref < 0) symref = -symref;
			  len = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, symref), ">)\n", (const char *)NULL);
			  port_write_string(ci->cycle_port)(sc, buf, len, ci->cycle_port);
			}
		      else
			{
			  object_to_port_with_circle_check(sc, val, ci->cycle_port, p_readable, ci);
			  port_write_string(ci->cycle_port)(sc, ")\n", 2, ci->cycle_port);
			}}
		  else
		    {
		      port_write_character(port)(sc, ' ', port);
		      object_to_port_with_circle_check(sc, val, port, p_readable, ci);
		    }}}
	  else
	    {
	      port_write_character(port)(sc, '(', port);
	      c_object_name_to_port(sc, obj, port);
	      for (s7_pointer p = obj_list; is_pair(p); p = cdr(p))
		{
		  s7_pointer val = car(p);
		  port_write_character(port)(sc, ' ', port);
		  object_to_port_with_circle_check(sc, val, port, p_readable, ci);
		}}
	  port_write_character(port)(sc, ')', port);
	  sc->w = old_w;
	}
      else
	{
	  char buf[128];
	  int32_t nlen;
	  port_write_string(port)(sc, "#<", 2, port);
	  c_object_name_to_port(sc, obj, port);
	  nlen = snprintf(buf, 128, " %p>", obj);
	  port_write_string(port)(sc, buf, clamp_length(nlen, 128), port);
	}}
}

static void stack_to_port(s7_scheme *sc, const s7_pointer obj, s7_pointer port, use_write_t unused_use_write, shared_info_t *unused_ci)
{
  if (obj == sc->stack)
    port_write_string(port)(sc, "#<current stack>", 16, port);
  else port_write_string(port)(sc, "#<stack>", 8, port);
}

void init_display_functions(void)
{
  for (int32_t i = 0; i < 256; i++) display_functions[i] = display_fallback;
  display_functions[T_BACRO] =           macro_to_port;
  display_functions[T_BACRO_STAR] =      macro_to_port;
  display_functions[T_BOOLEAN] =         unique_to_port;
  display_functions[T_BYTE_VECTOR] =     byte_vector_to_port;
  display_functions[T_CATCH] =           catch_to_port;
  display_functions[T_CHARACTER] =       character_to_port;
  display_functions[T_CLOSURE] =         closure_to_port;
  display_functions[T_CLOSURE_STAR] =    closure_to_port;
  display_functions[T_COMPLEX] =         number_to_port;
  display_functions[T_COMPLEX_VECTOR] =  complex_vector_to_port;
  display_functions[T_CONTINUATION] =    continuation_to_port;
  display_functions[T_COUNTER] =         counter_to_port;
  display_functions[T_C_FUNCTION] =      c_function_to_port;
  display_functions[T_C_FUNCTION_STAR] = c_function_to_port;
  display_functions[T_C_MACRO] =         c_macro_to_port;
  display_functions[T_C_OBJECT] =        c_object_to_port;
  display_functions[T_C_POINTER] =       c_pointer_to_port;
  display_functions[T_C_RST_NO_REQ_FUNCTION] = c_function_to_port;
  display_functions[T_DYNAMIC_WIND] =    dynamic_wind_to_port;
  display_functions[T_EOF] =             eof_to_port;
  display_functions[T_FLOAT_VECTOR] =    float_vector_to_port;
  display_functions[T_GOTO] =            goto_to_port;
  display_functions[T_HASH_TABLE] =      hash_table_to_port;
  display_functions[T_INPUT_PORT] =      input_port_to_port;
  display_functions[T_INTEGER] =         integer_to_port;
  display_functions[T_INT_VECTOR] =      int_vector_to_port;
  display_functions[T_ITERATOR] =        iterator_to_port;
  display_functions[T_LET] =             let_to_port;
  display_functions[T_MACRO] =           macro_to_port;
  display_functions[T_MACRO_STAR] =      macro_to_port;
  display_functions[T_NIL] =             unique_to_port;
  display_functions[T_OUTPUT_PORT] =     output_port_to_port;
  display_functions[T_PAIR] =            pair_to_port;
  display_functions[T_RANDOM_STATE] =    random_state_to_port;
  display_functions[T_RATIO] =           number_to_port;
  display_functions[T_REAL] =            number_to_port;
  display_functions[T_SLOT] =            internal_slot_to_port;
  display_functions[T_STACK] =           stack_to_port;
  display_functions[T_STRING] =          string_to_port;
  display_functions[T_SYMBOL] =          symbol_to_port;
  display_functions[T_SYNTAX] =          syntax_to_port;
  display_functions[T_UNDEFINED] =       undefined_to_port;
  display_functions[T_UNSPECIFIED] =     unique_to_port;
  display_functions[T_UNUSED] =          unique_to_port;
  display_functions[T_VECTOR] =          vector_to_port;
}

static void object_to_port_with_circle_check_1(s7_scheme *sc, s7_pointer obj, s7_pointer port, use_write_t use_write, shared_info_t *ci)
{
  const int32_t ref = (is_collected(obj)) ? shared_ref(ci, obj) : 0;
  if (ref == 0)
    object_to_port(sc, obj, port, use_write, ci);
  else
    {
      char buf[32];
      int32_t nlen;
      if (ref > 0)
	{
	  if (use_write == p_readable)
	    {
	      if (ci->defined[ref])
		{
		  flip_ref(ci, obj);
		  nlen = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, ref), ">", (const char *)NULL);
		  port_write_string(port)(sc, buf, nlen, port);
		  return;
		}
	      object_to_port(sc, obj, port, p_readable, ci);
	    }
	  else
	    { /* "normal" printout involving #n= and #n# */
	      s7_int len = 0;
	      char *p = pos_int_to_str(sc, (s7_int)ref, &len, '=');
	      *--p = '#';
	      port_write_string(port)(sc, p, len, port);
	      object_to_port(sc, obj, port, not_p_display(use_write), ci);
	    }}
      else
	if (use_write == p_readable)
	  {
	    nlen = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, -ref), ">", (const char *)NULL);
	    port_write_string(port)(sc, buf, nlen, port);
	  }
	else
	  {
	    s7_int len = 0;
	    char *p = pos_int_to_str(sc, (s7_int)(-ref), &len, '#');
	    *--p = '#';
	    port_write_string(port)(sc, p, len, port);
	  }}
}

static s7_pointer cyclic_out(s7_scheme *sc, s7_pointer obj, s7_pointer port, shared_info_t *ci)
{
  int32_t ref, len;
  char buf[128];

  ci->cycle_port = s7_open_output_string(sc);
  ci->cycle_loc = gc_protect_1(sc, ci->cycle_port);

  port_write_string(port)(sc, "(let (", 6, port);
  for (int32_t i = 0; i < ci->top; i++)
    {
      ref = peek_shared_ref(ci, ci->objs[i]); /* refs may be in any order */
      if (ref < 0) {ref = -ref; flip_ref(ci, ci->objs[i]);}
      len = (int32_t)catstrs_direct(buf, (i == 0) ? "(<" : "\n      (<", pos_int_to_str_direct(sc, ref), "> ", (const char *)NULL);
      port_write_string(port)(sc, buf, len, port);
      ci->defined[ref] = false;
      object_to_port_with_circle_check(sc, ci->objs[i], port, p_readable, ci);
      port_write_character(port)(sc, ')', port);
      ci->defined[ref] = true;
      if (peek_shared_ref(ci, ci->objs[i]) > 0) flip_ref(ci, ci->objs[i]); /* ref < 0 -> use <%d> in object_to_port */
    }
  port_write_string(port)(sc, ")\n", 2, port);

  if (ci->init_port != sc->F)
    {
      port_write_string(port)(sc, (const char *)(port_data(ci->init_port)), port_position(ci->init_port), port);
      s7_close_output_port(sc, ci->init_port);
      s7_gc_unprotect_at(sc, ci->init_loc);
      ci->init_port = sc->F;
    }

  if (port_position(ci->cycle_port) > 0)     /* 0 if e.g. (object->string (object->let (rootlet)) :readable) */
    port_write_string(port)(sc, (const char *)(port_data(ci->cycle_port)), port_position(ci->cycle_port), port);
  s7_close_output_port(sc, ci->cycle_port);
  s7_gc_unprotect_at(sc, ci->cycle_loc);
  ci->cycle_port = sc->F;

  if ((is_immutable(obj)) && (!is_let(obj)))
    port_write_string(port)(sc, "  (immutable! ", 14, port);
  else port_write_string(port)(sc, "  ", 2, port);

  ref = peek_shared_ref(ci, obj);
  if (ref == 0)
    object_to_port_with_circle_check(sc, obj, port, p_readable, ci);
  else
    {
      len = (int32_t)catstrs_direct(buf, "<", pos_int_to_str_direct(sc, (ref < 0) ? -ref : ref), ">", (const char *)NULL);
      port_write_string(port)(sc, buf, len, port);
    }

  if ((is_immutable(obj)) && (!is_let(obj)))
    port_write_string(port)(sc, "))\n", 3, port);
  else port_write_string(port)(sc, ")\n", 2, port);
  return(obj);
}

static void object_out_1(s7_scheme *sc, s7_pointer obj, s7_pointer strport, use_write_t choice)
{
  if (sc->object_out_locked)
    object_to_port_with_circle_check(sc, T_Pos(obj), strport, choice, sc->circle_info);
  else
    {
      shared_info_t *ci = load_shared_info(sc, T_Pos(obj), choice != p_readable, sc->circle_info);
      if (ci)
	{
	  sc->object_out_locked = true;
	  if (choice == p_readable)
	    cyclic_out(sc, obj, strport, ci);
	  else object_to_port_with_circle_check(sc, T_Pos(obj), strport, choice, ci);
	  sc->object_out_locked = false;
	}
      else object_to_port(sc, obj, strport, choice, NULL);
    }
}

static inline s7_pointer object_out(s7_scheme *sc, s7_pointer obj, s7_pointer strport, use_write_t choice)
{
  if ((has_structure(obj)) && (obj != sc->rootlet))
    object_out_1(sc, obj, strport, choice);
  else object_to_port(sc, obj, strport, choice, NULL);
  return(obj);
}

s7_pointer s7i_object_out(s7_scheme *sc, s7_pointer obj, s7_pointer port, s7i_use_write_t choice)
{
  return object_out(sc, obj, port, (use_write_t)choice);
}


/* -------------------------------- newline -------------------------------- */

void s7_newline(s7_scheme *sc, s7_pointer port)
{
  if (port != s7_f(sc))
    s7i_port_write_character(sc, (uint8_t)'\n', port);
}

s7_pointer g_newline(s7_scheme *sc, s7_pointer args)
{
  #define H_newline "(newline (port (current-output-port))) writes a carriage return to the port"
  #define Q_newline s7_make_signature(sc, 2, sc->is_char_symbol, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))

  const s7_pointer port = (s7_is_pair(args)) ? s7_car(args) : s7_current_output_port(sc);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return s7_make_character(sc, '\n');
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "newline", args);
      return s7_wrong_type_arg_error(sc, "newline", 1, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "newline", 1, port, "an open output port");
  s7_newline(sc, port);
  return s7_make_character(sc, '\n');
}

s7_pointer newline_p(s7_scheme *sc)
{
  s7_newline(sc, s7_current_output_port(sc));
  return s7_make_character(sc, '\n');
}

s7_pointer newline_p_p(s7_scheme *sc, s7_pointer port)
{
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return s7_make_character(sc, '\n');
      return s7i_method_or_bust_p(sc, port, "newline", "an output port");
    }
  s7_newline(sc, port);
  return s7_make_character(sc, '\n');
}


/* -------------------------------- write -------------------------------- */

s7_pointer s7_write(s7_scheme *sc, s7_pointer obj, s7_pointer port)
{
  if (port != s7_f(sc))
    {
      if (s7i_port_is_closed(port))
        return s7_wrong_type_arg_error(sc, "write", 2, port, "an open output port");
      s7i_object_out(sc, obj, port, S7I_P_WRITE);
    }
  return obj;
}

s7_pointer write_p_p(s7_scheme *sc, s7_pointer x)
{
  s7_pointer port = s7_current_output_port(sc);
  return (port == s7_f(sc)) ? x : s7i_object_out(sc, x, port, S7I_P_WRITE);
}

s7_pointer write_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer port)
{
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return x;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write", s7_cons(sc, x, s7_cons(sc, port, s7_nil(sc))));
      return s7_wrong_type_arg_error(sc, "write", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "write", 2, port, "an open output port");
  return s7i_object_out(sc, x, port, S7I_P_WRITE);
}

s7_pointer g_write(s7_scheme *sc, s7_pointer args)
{
  #define H_write "(write obj (port (current-output-port))) writes (object->string obj) to the output port"
  #define Q_write s7_make_signature(sc, 3, sc->T, sc->T, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  IF_METHOD_EXISTS_RETURN_VALUE(sc, s7_car(args), "write", args);
  return write_p_pp(sc, s7_car(args), (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc));
}

s7_pointer g_write_2(s7_scheme *sc, s7_pointer args)
{
  return write_p_pp(sc, s7_car(args), s7_cadr(args));
}


/* -------------------------------- display -------------------------------- */

s7_pointer s7_display(s7_scheme *sc, s7_pointer obj, s7_pointer port)
{
  if (port != s7_f(sc))
    {
      if (s7i_port_is_closed(port))
        return s7_wrong_type_arg_error(sc, "display", 2, port, "an open output port");
      s7i_object_out(sc, obj, port, S7I_P_DISPLAY);
    }
  return obj;
}

s7_pointer display_p_pp(s7_scheme *sc, s7_pointer x, s7_pointer port)
{
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return x;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "display", s7_cons(sc, x, s7_cons(sc, port, s7_nil(sc))));
      return s7_wrong_type_arg_error(sc, "display", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "display", 2, port, "an open output port");
  IF_METHOD_EXISTS_RETURN_VALUE(sc, x, "display", s7_cons(sc, x, s7_cons(sc, port, s7_nil(sc))));
  return s7i_object_out(sc, x, port, S7I_P_DISPLAY);
}

s7_pointer g_display(s7_scheme *sc, s7_pointer args)
{
  #define H_display "(display obj (port (current-output-port))) prints obj"
  #define Q_display s7_make_signature(sc, 3, sc->T, sc->T, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  return display_p_pp(sc, s7_car(args), (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc));
}

s7_pointer g_display_2(s7_scheme *sc, s7_pointer args)
{
  return display_p_pp(sc, s7_car(args), s7_cadr(args));
}

s7_pointer g_display_f(s7_scheme *sc, s7_pointer args)
{
  (void)sc;
  return s7_car(args);
}

s7_pointer display_p_p(s7_scheme *sc, s7_pointer x)
{
  s7_pointer port = s7_current_output_port(sc);
  if (port == s7_f(sc)) return x;
  IF_METHOD_EXISTS_RETURN_VALUE(sc, x, "display", s7_cons(sc, x, s7_nil(sc)));
  return s7i_object_out(sc, x, port, S7I_P_DISPLAY);
}


/* -------------------------------- write-char -------------------------------- */

s7_pointer s7_write_char(s7_scheme *sc, s7_pointer c, s7_pointer port)
{
  if (port != s7_f(sc))
    s7i_port_write_unicode_char(sc, s7_character(c), port);
  return c;
}

s7_pointer write_char_p_pp(s7_scheme *sc, s7_pointer c, s7_pointer port)
{
  if (!s7_is_character(c))
    return s7i_method_or_bust_pp(sc, c, "write-char", c, port, "a character", 1);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return c;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write-char", s7_cons(sc, c, s7_cons(sc, port, s7_nil(sc))));
      return s7_wrong_type_arg_error(sc, "write-char", 2, port, "an output port or #f");
    }
  s7i_port_write_unicode_char(sc, s7_character(c), port);
  return c;
}

s7_pointer write_char_p_p(s7_scheme *sc, s7_pointer c)
{
  if (!s7_is_character(c))
    return s7i_method_or_bust_p(sc, c, "write-char", "a character");
  s7_pointer port = s7_current_output_port(sc);
  if (port == s7_f(sc)) return c;
  s7i_port_write_unicode_char(sc, s7_character(c), port);
  return c;
}

s7_pointer g_write_char(s7_scheme *sc, s7_pointer args)
{
  #define H_write_char "(write-char char (port (current-output-port))) writes char to the output port"
  #define Q_write_char s7_make_signature(sc, 3, sc->is_char_symbol, sc->is_char_symbol, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  if (s7_is_null(sc, s7_cdr(args)))
    return write_char_p_p(sc, s7_car(args));
  return write_char_p_pp(sc, s7_car(args), (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc));
}


/* -------------------------------- write-string -------------------------------- */

s7_pointer g_write_string(s7_scheme *sc, s7_pointer args)
{
  #define H_write_string "(write-string str port start end) writes str to port."
  #define Q_write_string s7_make_circular_signature(sc, 3, 4, \
                           sc->is_string_symbol, sc->is_string_symbol, \
                           s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol),\
                           sc->is_integer_symbol)
  const s7_pointer str = s7_car(args);
  s7_pointer port;
  s7_int start = 0, end;
  if (!s7_is_string(str))
    return s7i_method_or_bust(sc, str, "write-string", args, "a string", 1);
  end = s7_string_length(str);
  if (!s7_is_null(sc, s7_cdr(args)))
    {
      s7_pointer inds = s7_cddr(args);
      port = s7_cadr(args);
      if (!s7_is_null(sc, inds))
        {
          s7_pointer p = s7i_start_and_end(sc, s7_make_symbol(sc, "write-string"), args, 3, inds, &start, &end);
          if (!s7i_is_unused(sc, p)) return p;
        }}
  else port = s7_current_output_port(sc);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc))
        {
          s7_int len;
          if ((start == 0) && (end == s7_string_length(str)))
            return str;
          len = (s7_int)(end - start);
          return s7_make_string_with_length(sc, (const char *)(s7_string(str) + start), len);
        }
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write-string", args);
      return s7_wrong_type_arg_error(sc, "write-string", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "write-string", 2, port, "an open output port");
  if (start == end) return str;
  s7i_port_write_string(sc, (const char *)(s7_string(str) + start), (end - start), port);
  return str;
}

s7_pointer write_string_p_pp(s7_scheme *sc, s7_pointer str, s7_pointer port)
{
  if (!s7_is_string(str))
    return s7i_method_or_bust_pp(sc, str, "write-string", str, port, "a string", 1);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return str;
      return s7i_method_or_bust_pp(sc, port, "write-string", str, port, "an output port", 2);
    }
  if (s7_string_length(str) > 0)
    s7i_port_write_string(sc, s7_string(str), s7_string_length(str), port);
  return str;
}


/* -------------------------------- write-byte -------------------------------- */

s7_pointer g_write_byte(s7_scheme *sc, s7_pointer args)
{
  #define H_write_byte "(write-byte byte (port (current-output-port))): writes byte to the output port"
  #define Q_write_byte s7_make_signature(sc, 3, sc->is_byte_symbol, sc->is_byte_symbol, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))

  s7_pointer port;
  const s7_pointer b = s7_car(args);
  s7_int val;
  if (!s7_is_integer(b))
    return s7i_method_or_bust(sc, b, "write-byte", args, "an integer", 1);

  val = s7_integer(b);
  if ((val < 0) || (val > 255))
    return s7_wrong_type_arg_error(sc, "write-byte", 1, b, "an unsigned byte");

  port = (s7_is_pair(s7_cdr(args))) ? s7_cadr(args) : s7_current_output_port(sc);
  if (!s7_is_output_port(sc, port))
    {
      if (port == s7_f(sc)) return b;
      IF_METHOD_EXISTS_RETURN_VALUE(sc, port, "write-byte", args);
      return s7_wrong_type_arg_error(sc, "write-byte", 2, port, "an output port or #f");
    }
  if (s7i_port_is_closed(port))
    return s7_wrong_type_arg_error(sc, "write-byte", 2, port, "an open output port");

  s7i_port_write_character(sc, (uint8_t)val, port);
  return b;
}


/* -------------------------------- current-input-port -------------------------------- */

s7_pointer g_current_input_port(s7_scheme *sc, s7_pointer unused_args)
{
  #define H_current_input_port "(current-input-port) returns the current input port"
  #define Q_current_input_port s7_make_signature(sc, 1, sc->is_input_port_symbol)
  (void)unused_args;
  return s7_current_input_port(sc);
}


/* -------------------------------- current-output-port -------------------------------- */

s7_pointer g_current_output_port(s7_scheme *sc, s7_pointer unused_args)
{
  #define H_current_output_port "(current-output-port) returns the current output port"
  #define Q_current_output_port s7_make_signature(sc, 1, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  (void)unused_args;
  return s7_current_output_port(sc);
}


/* -------------------------------- current-error-port -------------------------------- */

s7_pointer g_current_error_port(s7_scheme *sc, s7_pointer unused_args)
{
  #define H_current_error_port "(current-error-port) returns the current error port"
  #define Q_current_error_port s7_make_signature(sc, 1, s7_make_signature(sc, 2, sc->is_output_port_symbol, sc->not_symbol))
  (void)unused_args;
  return s7_current_error_port(sc);
}


/* -------------------------------- open-output-string -------------------------------- */

s7_pointer g_open_output_string(s7_scheme *sc, s7_pointer unused_args)
{
  #define H_open_output_string "(open-output-string) opens an output string port"
  #define Q_open_output_string s7_make_signature(sc, 1, sc->is_output_port_symbol)
  (void)unused_args;
  return s7_open_output_string(sc);
}
