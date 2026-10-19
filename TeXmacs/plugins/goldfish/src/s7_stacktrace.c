/* s7_stacktrace.c - stack trace implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#include "s7_internal.h"
#include "s7_stacktrace.h"
#include "s7_scheme_base.h"
#include "s7_liii_tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define begin_small_symbol_set(Sc)              s7i_begin_small_symbol_set(Sc)
#define end_small_symbol_set(Sc)                s7i_end_small_symbol_set(Sc)
#define add_symbol_to_small_symbol_set(Sc, Sym) s7i_add_symbol_to_small_symbol_set(Sc, Sym)
#define symbol_is_in_small_symbol_set(Sc, Sym)  s7i_symbol_is_in_small_symbol_set(Sc, Sym)

#define block_to_string(sc, b, len)             s7i_inline_block_to_string(sc, b, len)
#define nil_string                              s7i_nil_string()
#define local_strcmp(S1, S2)                    (strcmp(S1, S2) == 0)
#define s7_integer_clamped_if_gmp(Sc, P)        s7i_integer_clamped_if_gmp(Sc, P)
#define mallocate(sc, size)                     s7i_mallocate(sc, size)
#define liberate(sc, b)                         s7i_liberate(sc, b)
#define wrap_integer(sc, n)                     s7i_wrap_integer(sc, n)
#define copy_proper_list(sc, lst)               s7i_copy_proper_list(sc, lst)
#define is_procedure(p)                         s7_is_procedure(p)

/* shared range limits for g_stacktrace and sl_set_stacktrace_defaults */
#define ST_CODE_COLS_MIN 8
#define ST_CODE_COLS_MAX 1024

/* ---------------- stacktrace ---------------- */
static s7_pointer stacktrace_find_caller(s7_scheme *sc, s7_pointer let)
{
  if ((is_let(let)) && (let != sc->rootlet))
    return(((is_funclet(let)) || (is_maclet(let))) ? funclet_function(let) : stacktrace_find_caller(sc, let_outlet(let)));
  return(sc->F);
}

static bool stacktrace_find_let(s7_scheme *sc, s7_int loc, s7_pointer let)
{
  return((loc > 0) &&
	 ((stack_let(sc->stack, loc) == let) ||
	  (stacktrace_find_let(sc, loc - 4, let))));
}

static s7_int stacktrace_find_error_hook_quit(s7_scheme *sc)
{
  for (s7_int op_loc = stack_top(sc) - 1; op_loc >= 3; op_loc -= 4)
    if (stack_op(sc->stack, op_loc) == OP_ERROR_HOOK_QUIT)
      return(op_loc);
  return(-1);
}

static bool stacktrace_in_error_handler(s7_scheme *sc, s7_int loc)
{
  return((let_outlet(sc->owlet) == sc->curlet) ||
	 (stacktrace_find_let(sc, loc * 4, let_outlet(sc->owlet))) ||
	 (stacktrace_find_error_hook_quit(sc) > 0));
}

static bool stacktrace_error_hook_function(s7_scheme *sc, s7_pointer sym)
{
  if (is_symbol(sym))
    {
      s7_pointer func = s7_symbol_value(sc, sym);
      return((is_procedure(func)) &&
	     (hook_has_functions(sc->error_hook)) &&
	     (direct_memq(func, s7_hook_functions(sc, sc->error_hook))));
    }
  return(false);
}

static char *stacktrace_walker(s7_scheme *sc, s7_pointer code, s7_pointer let, char *notes,
			       s7_int code_cols, s7_int total_cols, s7_int notes_start_col,
			       bool as_comment, int32_t depth)
{
  if (is_symbol(code))
    {
      if ((!symbol_is_in_small_symbol_set(sc, code)) &&
	  (!is_slot(global_slot(code))))
	{
	  const s7_pointer val = s7_symbol_local_value(sc, code, let);
	  add_symbol_to_small_symbol_set(sc, code);
	  if ((val) &&
	      (val != sc->undefined) &&
	      (!is_any_macro(val)) &&
	      (type(val) < T_CONTINUATION))
	    {
	      char *objstr;
	      s7_pointer objp;
	      s7_int new_note_len, notes_max;
	      bool new_notes_line = false;
	      const bool old_short_print = sc->short_print;
	      const s7_int old_len = sc->print_length;
	      s7_int objlen;

	      if (notes_start_col < 0) notes_start_col = 50;
	      if (notes_start_col > total_cols) notes_start_col = 0;
	      notes_max = total_cols - notes_start_col;
	      sc->short_print = true;
	      if (sc->print_length > 4) sc->print_length = 4;
	      objp = s7_object_to_string(sc, val, true);
	      objstr = string_value(objp);
	      objlen = string_length(objp);
	      if ((objlen > notes_max) &&
		  (notes_max > 5))
		{
		  objstr[notes_max - 4] = '.';
		  objstr[notes_max - 3] = '.';
		  objstr[notes_max - 2] = '.';
		  objstr[notes_max - 1] = '\0';
		  objlen = notes_max;
		}
	      sc->short_print = old_short_print;
	      sc->print_length = old_len;
	      new_note_len = symbol_name_length(code) + 3 + objlen;
	      /* we want to append this much info to the notes, but does it need a new line? */
	      if (notes_start_col < code_cols)
		new_notes_line = true;
	      else
		if (notes)
		  {
		    const char *last_newline = strrchr(notes, (int)'\n'); /* returns ptr to end if none = nil if not found? */
		    s7_int cur_line_len = (last_newline) ? (strlen(notes) - strlen(last_newline)) : strlen(notes);
		    new_notes_line = ((cur_line_len + new_note_len) > notes_max);
		  }
	      {
		char *str;
		if (new_notes_line)
		  {
		    const char *spaces = "                                                                                ";
		    const s7_int spaces_len = 80;
		    new_note_len += (4 + notes_start_col + ((notes) ? strlen(notes) : 0));
		    str = (char *)Malloc(new_note_len);   /* str[0] = '\0'; */
		    catstrs_direct(str,
				   (notes) ? notes : "",
				   "\n",
				   (as_comment) ? "; " : "",
				   (spaces_len >= notes_start_col) ? (const char *)(spaces + spaces_len - notes_start_col) : "",
				   (as_comment) ? "" : " ; ",
				   symbol_name(code),
				   ": ",
				   objstr, (const char *)NULL);
		  }
		else
		  {
		    new_note_len += ((notes) ? strlen(notes) : 0) + 4;
		    str = (char *)Malloc(new_note_len);   /* str[0] = '\0'; */
		    catstrs_direct(str,
				   (notes) ? notes : "",
				   (notes) ? ", " : " ; ",
				   symbol_name(code),
				   ": ",
				   objstr, (const char *)NULL);
		  }
		if (notes) free(notes);
		return(str);
	      }}}
      return(notes);
    }
  if ((is_pair(code)) &&
      (s7_list_length(sc, code) > 0) &&
      (depth < 32))
    {
      notes = stacktrace_walker(sc, car(code), let, notes, code_cols, total_cols, notes_start_col, as_comment, depth + 1);
      return(stacktrace_walker(sc, cdr(code), let, notes, code_cols, total_cols, notes_start_col, as_comment, depth + 2));
    }
  return(notes);
}

static bool find_location_in_tree(s7_pointer tree, s7_int *ret_line, s7_int *ret_file, s7_int *ret_pos)
{
  if (!is_pair(tree)) return false;
  if (has_location(tree))
    {
      *ret_line = (s7_int)pair_line_number(tree);
      *ret_file = (s7_int)pair_file_number(tree);
      *ret_pos = (s7_int)pair_position(tree);
      return true;
    }
  for (s7_pointer p = tree; is_pair(p); p = cdr(p))
    {
      if (find_location_in_tree(car(p), ret_line, ret_file, ret_pos))
	return true;
    }
  return false;
}

bool s7i_find_error_location(s7_scheme *sc, s7_pointer cur_code, s7_int *ret_line, s7_int *ret_file, s7_int *ret_pos)
{
  *ret_line = -1;
  *ret_file = -1;
  *ret_pos = 0;
  if ((!is_pair(cur_code)) ||
      (!find_location_in_tree(cur_code, ret_line, ret_file, ret_pos)))
    {
      const s7_int top = (sc->stack_end - sc->stack_start) / 4;
      for (s7_int loc = top - 1; loc > 0; loc--)
	{
	  const s7_int true_loc = (loc + 1) * 4 - 1;
	  if (find_location_in_tree(stack_code(sc->stack, true_loc), ret_line, ret_file, ret_pos))
	    break;
	}
    }
  return((*ret_line > 0) && (*ret_file >= 0) && (*ret_file <= (s7_int)sc->file_names_top) && (is_string(sc->file_names[*ret_file])));
}

static void location_from_let(s7_scheme *sc, s7_pointer let, const char **ret_file, s7_int *ret_line)
{
  if ((has_let_file(let)) && (let_file(let) <= (s7_int)sc->file_names_top) && (let_line(let) > 0))
    {
      *ret_line = let_line(let);
      if (is_string(sc->file_names[let_file(let)]))
	*ret_file = string_value(sc->file_names[let_file(let)]);
    }
}

static void get_code_or_let_location(s7_scheme *sc, s7_pointer code, s7_pointer let, const char **ret_file, s7_int *ret_line)
{
  *ret_file = NULL;
  *ret_line = -1;

  if ((is_let(let)) && ((is_funclet(let)) || (is_maclet(let))))
    location_from_let(sc, let, ret_file, ret_line);

  if (*ret_line <= 0)
    {
      s7_int line = -1, file = -1, pos = 0;
      if (find_location_in_tree(code, &line, &file, &pos))
	{
	  *ret_line = line;
	  if ((file >= 0) && (file <= (s7_int)sc->file_names_top) && (is_string(sc->file_names[file])))
	    *ret_file = string_value(sc->file_names[file]);
	}
    }

  if ((*ret_line <= 0) && (is_let(let)))
    location_from_let(sc, let, ret_file, ret_line);

  if ((*ret_line <= 0) && (is_string(slot_value(sc->error_file))) && (s7_is_integer(slot_value(sc->error_line))))
    {
      *ret_file = string_value(slot_value(sc->error_file));
      *ret_line = integer(slot_value(sc->error_line));
    }
}

static block_t *stacktrace_add_func(s7_scheme *sc, s7_pointer caller, s7_pointer code, const char *loc_file, s7_int loc_line, const char *errstr, char *notes, s7_int code_max, bool as_comment)
{
  s7_int newlen, errlen = strlen(errstr);
  char *newstr;
  block_t *newp;
  char loc_buf[256];
  if ((loc_file) && (loc_line > 0))
    snprintf(loc_buf, sizeof(loc_buf), "  %s:%lld: ", loc_file, (long long)loc_line);
  else
    snprintf(loc_buf, sizeof(loc_buf), "  ");
  s7_int loc_len = strlen(loc_buf);

  if ((is_symbol(caller)) &&
      (caller != car(code)))
    {
      newlen = loc_len + symbol_name_length(caller) + errlen + 10;
      newp = mallocate(sc, newlen);
      newstr = (char *)block_data(newp);      /* newstr[0] = '\0'; */
      errlen = catstrs_direct(newstr, loc_buf, symbol_name(caller), ": ", errstr, (const char *)NULL);
    }
  else
    {
      newlen = loc_len + errlen + 8;
      newp = mallocate(sc, newlen);
      newstr = (char *)block_data(newp);      /* newstr[0] = '\0'; */
      if ((errlen > 2) && (errstr[2] == '('))
        errlen = catstrs_direct(newstr, loc_buf, "  ", errstr, (const char *)NULL);
      else
	errlen = catstrs_direct(newstr, loc_buf, errstr, (const char *)NULL);
    }
  s7_int eff_code_max = code_max + loc_len;
  newlen = eff_code_max + 8 + ((notes) ? strlen(notes) : 0);
  {
    block_t *b = mallocate(sc, newlen);
    char *str = (char *)block_data(b);        /* str[0] = '\0'; */
    if (errlen >= eff_code_max)
      {
	newstr[eff_code_max - 4] = '.';
	newstr[eff_code_max - 3] = '.';
	newstr[eff_code_max - 2] = '.';
	newstr[eff_code_max - 1] = '\0';
	catstrs_direct(str, (as_comment) ? "; " : "", newstr, (notes) ? notes : "", "\n", (const char *)NULL);
      }
    else
      {
	/* send out newstr, pad with spaces to eff_code_max, then notes */
	const s7_int len = catstrs_direct(str, (as_comment) ? "; " : "", newstr, (const char *)NULL);
	if (notes)
	  {
	    s7_int i;
	    for (i = len; i < eff_code_max - 1; i++) str[i] = ' ';
	    str[i] = '\0';
	    catstrs(str, newlen, notes, "\n", (char *)NULL);
	  }
	else catstrs(str, newlen, "\n", (char *)NULL);
      }
    liberate(sc, newp);
    return(b);
  }
}

static s7_pointer stacktrace_1(s7_scheme *sc, s7_int frames_max, s7_int code_cols, s7_int total_cols, s7_int notes_start_col, bool as_comment)
{
  char *str = NULL;
  block_t *strp = NULL;
  s7_int loc, frames = 0;
  s7_int top = (sc->stack_end - sc->stack_start) / 4; /* (*s7* 'stack_top), not stack_top(sc)! */
  begin_small_symbol_set(sc);

  if (stacktrace_in_error_handler(sc, top))
    {
      const s7_pointer err_code = slot_value(sc->error_code);
      if ((is_pair(err_code)) &&
	  (!tree_is_cyclic(sc, err_code)))
	{
	  char *notes = NULL;
	  const s7_pointer current_let = let_outlet(sc->owlet);
	  const s7_pointer errstr = s7_object_to_string(sc, err_code, false);
	  const s7_pointer caller = stacktrace_find_caller(sc, current_let); /* this is a symbol */
	  const char *loc_file = NULL;
	  s7_int loc_line = -1;
	  if ((is_let(current_let)) &&
	      (current_let != sc->rootlet))
	    notes = stacktrace_walker(sc, err_code, current_let, NULL, code_cols, total_cols, notes_start_col, as_comment, 0);
	  get_code_or_let_location(sc, err_code, current_let, &loc_file, &loc_line);
	  strp = stacktrace_add_func(sc, caller, err_code, loc_file, loc_line, string_value(errstr), notes, code_cols, as_comment);
	  str = (char *)block_data(strp);
	  if ((S7_DEBUGGING) && (notes == str)) fprintf(stderr, "%s[%d]: notes==str\n", __func__, __LINE__);
	  if (notes) free(notes); /* copied into strp, 29-Sep-23 -- see below: maybe check that notes!=str? */
	}
      loc = stacktrace_find_error_hook_quit(sc); /* if OP_ERROR_HOOK_QUIT is in the stack, jump past it! */
      if (loc > 0) top = (loc + 1) / 4;
    }
  for (loc = top - 1; loc > 0; loc--)
    {
      const s7_int true_loc = (loc + 1) * 4 - 1;
      const s7_pointer code = stack_code(sc->stack, true_loc);
      const s7_pointer let = stack_let(sc->stack, true_loc); /* might not be let (gc stack protection etc) */
      const s7_pointer func = stacktrace_find_caller(sc, let);
      const char *loc_file = NULL;
      s7_int loc_line = -1;
      s7_pointer codep;
      const char *codestr;
      char *notes = NULL, *newstr, *catstr;
      block_t *newp, *catp;
      s7_int newlen;

      if ((!is_pair(code)) || (tree_is_cyclic(sc, code))) continue;
      if (stacktrace_error_hook_function(sc, func)) continue;
      get_code_or_let_location(sc, code, let, &loc_file, &loc_line);
      if ((loc_file) && (strstr(loc_file, "gfproject.scm") != NULL)) continue;

      codep = s7_object_to_string(sc, code, false);
      if (string_length(codep) == 0) continue;
      codestr = string_value(codep);
      if ((local_strcmp(codestr, "(result)")) ||
	  (local_strcmp(codestr, "(#f)")) ||
	  (strstr(codestr, "(stacktrace)")) ||
	  (strstr(codestr, "(stacktrace ")))
	continue;

      frames++;
      if (frames > frames_max)
	{
	  end_small_symbol_set(sc);
	  return(block_to_string(sc, strp, safe_strlen((char *)block_data(strp))));
	}
      if ((is_let(let)) && (let != sc->rootlet))
	notes = stacktrace_walker(sc, code, let, NULL, code_cols, total_cols, notes_start_col, as_comment, 0);
      newp = stacktrace_add_func(sc, func, code, loc_file, loc_line, codestr, notes, code_cols, as_comment);
      newstr = (char *)block_data(newp);
      if ((S7_DEBUGGING) && (notes == newstr)) fprintf(stderr, "%s[%d]: notes==newstr\n", __func__, __LINE__);
      if ((notes) && (notes != newstr) && (is_let(let)) && (let != sc->rootlet))
	free(notes);
      newlen = strlen(newstr) + 1 + ((str) ? strlen(str) : 0);
      catp = mallocate(sc, newlen);
      catstr = (char *)block_data(catp);
      catstrs_direct(catstr, (str) ? str : "", newstr, (const char *)NULL);
      liberate(sc, newp);
      if (strp) liberate(sc, strp);
      strp = catp;
      str = (char *)block_data(strp);
    }
  end_small_symbol_set(sc);
  return((strp) ? block_to_string(sc, strp, safe_strlen((char *)block_data(strp))) : nil_string);
}

s7_pointer s7_stacktrace(s7_scheme *sc)
{
  return(stacktrace_1(sc,
		      s7_integer_clamped_if_gmp(sc, car(sc->stacktrace_defaults)),
		      s7_integer_clamped_if_gmp(sc, cadr(sc->stacktrace_defaults)),
		      s7_integer_clamped_if_gmp(sc, caddr(sc->stacktrace_defaults)),
		      s7_integer_clamped_if_gmp(sc, cadddr(sc->stacktrace_defaults)),
		      s7_boolean(sc, s7_list_ref(sc, sc->stacktrace_defaults, 4))));
}

s7_pointer g_stacktrace(s7_scheme *sc, s7_pointer args)
{
  s7_int max_frames = s7_integer_clamped_if_gmp(sc, car(sc->stacktrace_defaults));
  s7_int code_cols = s7_integer_clamped_if_gmp(sc, cadr(sc->stacktrace_defaults));
  s7_int total_cols = s7_integer_clamped_if_gmp(sc, caddr(sc->stacktrace_defaults));
  s7_int notes_start_col = s7_integer_clamped_if_gmp(sc, cadddr(sc->stacktrace_defaults));
  bool as_comment = s7_boolean(sc, s7_list_ref(sc, sc->stacktrace_defaults, 4));

  if (!is_null(args))
    {
      if (!s7_is_integer(car(args)))
	return(method_or_bust(sc, car(args), sc->stacktrace_symbol, args, sc->type_names[T_INTEGER], 1));
      max_frames = s7_integer_clamped_if_gmp(sc, car(args));
      if ((max_frames <= 0) || (max_frames > S7_INT32_MAX))
	max_frames = ST_MAX_FRAMES;
      args = cdr(args);
      if (!is_null(args))
	{
	  if (!s7_is_integer(car(args)))
	    wrong_type_error_nr(sc, sc->stacktrace_symbol, 2, car(args), sc->type_names[T_INTEGER]);
	  code_cols = s7_integer_clamped_if_gmp(sc, car(args));
	  if ((code_cols <= ST_CODE_COLS_MIN) || (code_cols > ST_CODE_COLS_MAX))
	    code_cols = ST_CODE_COLS;
	  args = cdr(args);
	  if (!is_null(args))
	    {
	      if (!s7_is_integer(car(args)))
		wrong_type_error_nr(sc, sc->stacktrace_symbol, 3, car(args), sc->type_names[T_INTEGER]);
	      total_cols = s7_integer_clamped_if_gmp(sc, car(args));
	      if ((total_cols <= code_cols) || (total_cols > S7_INT32_MAX))
		total_cols = ST_TOTAL_COLS;
	      args = cdr(args);
	      if (!is_null(args))
		{
		  if (!s7_is_integer(car(args)))
		    wrong_type_error_nr(sc, sc->stacktrace_symbol, 4, car(args), sc->type_names[T_INTEGER]);
		  notes_start_col = s7_integer_clamped_if_gmp(sc, car(args));
		  if ((notes_start_col <= 0) || (notes_start_col > S7_INT32_MAX))
		    notes_start_col = ST_NOTES_START_COL;
		  args = cdr(args);
		  if (!is_null(args))
		    {
		      if (!is_boolean(car(args)))
			wrong_type_error_nr(sc, sc->stacktrace_symbol, 5, car(args), sc->type_names[T_BOOLEAN]);
		      as_comment = s7_boolean(sc, car(args));
		    }}}}}
  return(stacktrace_1(sc, max_frames, code_cols, total_cols, notes_start_col, as_comment));
}

static no_return void sl_stacktrace_wrong_type_error_nr(s7_scheme *sc, s7_int num, s7_pointer arg, s7_pointer typ, s7_pointer val)
{
  set_elist_6(sc, wrap_string(sc, "(set! (*s7* 'stacktrace-defaults) '~S): the ~:D list element ~S is ~A but should be ~A", 86),
	      val, wrap_integer(sc, num), arg, object_type_name(sc, arg), typ);
  error_nr(sc, sc->wrong_type_arg_symbol, sc->elist_6);
}

static no_return void sl_stacktrace_out_of_range_error_nr(s7_scheme *sc, s7_pointer accessor, s7_pointer lst, s7_pointer arg, s7_pointer descr)
{
  error_nr(sc, sc->out_of_range_symbol,
	   set_elist_5(sc, wrap_string(sc, "(set! (*s7* 'stacktrace-defaults) '~S): ~S => ~S is out of range (~A)", 69), lst, accessor, arg, descr));
}

s7_pointer sl_set_stacktrace_defaults(s7_scheme *sc, s7_pointer sym, s7_pointer val)
{
  s7_int code_cols;
  if (!is_pair(val))
    starlet_wrong_type_error_nr(sc, sym, val, sc->type_names[T_PAIR]);
  if (s7_list_length(sc, val) != 5)
    starlet_wrong_type_error_nr(sc, sym, val, wrap_string(sc, "a list with 5 entries", 21));

  if (!s7_is_integer(car(val)))          /* max_frames, default 30 */
    sl_stacktrace_wrong_type_error_nr(sc, 1, car(val), wrap_string(sc, "an integer (max stack frames)", 29), val);
  {
    s7_int i = s7_integer_clamped_if_gmp(sc, car(val));
    if ((i <= 0) || (i > S7_INT32_MAX))
      sl_stacktrace_out_of_range_error_nr(sc, sc->car_symbol, val, car(val), wrap_string(sc, "should be 0 < max-frames <= 2^32", 32));
  }
  if (!s7_is_integer(cadr(val)))         /* code_cols, default 45 */
    sl_stacktrace_wrong_type_error_nr(sc, 2, cadr(val), wrap_string(sc, "an integer (code columns)", 25), val);
  code_cols = s7_integer_clamped_if_gmp(sc, cadr(val));
  if ((code_cols <= ST_CODE_COLS_MIN) || (code_cols > ST_CODE_COLS_MAX))
    sl_stacktrace_out_of_range_error_nr(sc, sc->cadr_symbol, val, cadr(val), wrap_string(sc, "should be 8 < code-columns <= 1024", 34));

  if (!s7_is_integer(caddr(val)))        /* total_cols, default 80 */
    sl_stacktrace_wrong_type_error_nr(sc, 3, caddr(val), wrap_string(sc, "an integer (total columns)", 26), val);
  {
    s7_int i = s7_integer_clamped_if_gmp(sc, caddr(val));
    if ((i <= code_cols) || (i > S7_INT32_MAX))
      {
	int bytes = snprintf(sc->strbuf, sc->strbuf_size, "should be %" ld64 " < total-columns <= 2^32", code_cols);
	sl_stacktrace_out_of_range_error_nr(sc, sc->caddr_symbol, val, caddr(val), wrap_string(sc, sc->strbuf, bytes));
      }}

  if (!s7_is_integer(cadddr(val)))       /* notes_start_col, default max(45, code_cols) */
    sl_stacktrace_wrong_type_error_nr(sc, 4, cadddr(val), wrap_string(sc, "an integer (comment position)", 29), val);
  {
    s7_int i = s7_integer_clamped_if_gmp(sc, cadddr(val));
    if ((i <= 0) || (i > S7_INT32_MAX))
      sl_stacktrace_out_of_range_error_nr(sc, sc->cadddr_symbol, val, cadddr(val), wrap_string(sc, "should be 0 < comment-position <= 2^32", 38));
  }
  if (!is_boolean(s7_list_ref(sc, val, 4)))  /* as_comment, default #f */
    sl_stacktrace_wrong_type_error_nr(sc, 5, s7_list_ref(sc, val, 4), wrap_string(sc, "a boolean (output-as-comment)", 29), val);

  sc->stacktrace_defaults = copy_proper_list(sc, val);
  return(val);
}
