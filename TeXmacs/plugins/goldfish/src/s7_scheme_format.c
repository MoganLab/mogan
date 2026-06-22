/* s7_scheme_format.c - format function implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7.h"
#include "s7_internal_helpers.h"
#include "s7_ctables.h"
#include "s7_scheme_format.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

extern const char *ordinal[11];
extern const s7_int ordinal_length[11];

/* -------------------------------- format -------------------------------- */

static no_return void format_error_nr(s7_scheme *sc, const char *ur_msg, s7_int msg_len, const char *str, s7_pointer ur_args, format_data_t *fdat)
{
  s7_pointer err_list;
  const s7_pointer ctrl_str = (fdat->orig_str) ? fdat->orig_str : s7i_wrap_string(sc, str, s7i_safe_strlen(str));
  const s7_pointer args = (s7i_is_elist(sc, ur_args)) ? s7i_copy_proper_list(sc, ur_args) : ur_args;
  const s7_pointer msg = s7i_wrap_string(sc, ur_msg, msg_len);
  if (fdat->loc == 0)
    {
      if (s7_is_pair(args))
	err_list = s7i_set_elist_4(sc, s7i_format_string_1(sc), ctrl_str, args, msg);                                 /* "~S ~{~S~^ ~}: ~A" */
      else err_list = s7i_set_elist_3(sc, s7i_format_string_2(sc), ctrl_str, msg);                                    /* "~S: ~A" */
    }
  else
    if (s7_is_pair(args))
      err_list = s7i_set_elist_5(sc, s7i_format_string_3(sc), ctrl_str, args, s7i_wrap_integer(sc, fdat->loc + 20), msg); /* "~S ~{~S~^ ~}~&~NT^: ~A" */
    else err_list = s7i_set_elist_4(sc, s7i_format_string_4(sc), ctrl_str, s7i_wrap_integer(sc, fdat->loc + 20), msg);    /* "~S~&~NT^: ~A" */
  if (fdat->port)
    {
      s7i_close_format_port(sc, fdat->port);
      fdat->port = NULL;
    }
  s7i_error_nr(sc, s7i_format_error_symbol(sc), err_list);
}

static void format_append_char(s7_scheme *sc, char c, s7_pointer port)
{
  s7i_port_write_character(sc, c, port);
  s7i_inc_format_column(sc);
}

static void format_append_newline(s7_scheme *sc, s7_pointer port)
{
  s7i_port_write_character(sc, '\n', port);
  s7i_set_format_column(sc, 0);
}

static void format_append_string(s7_scheme *sc, format_data_t *fdat, const char *str, s7_int len, s7_pointer port)
{
  s7i_port_write_string(sc, str, len, port);
  fdat->loc += len;
  s7i_add_format_column(sc, len);
}

static void format_append_chars(s7_scheme *sc, format_data_t *fdat, char pad, s7_int chrs, s7_pointer port)
{
  if (s7i_is_string_port(port))
    {
      if ((s7i_port_position(port) + chrs) < s7i_port_data_size(port))
	{
	  memset((char *)s7i_port_data(port) + s7i_port_position(port), pad, chrs); /* unaligned */
	  s7i_set_port_position(port, s7i_port_position(port) + chrs);
	}
      else
	{
	  const s7_int new_len = s7i_port_position(port) + chrs;
	  s7i_resize_port_data(sc, port, new_len * 2);
	  memset((char *)s7i_port_data(port) + s7i_port_position(port), pad, chrs); /* unaligned */
	  s7i_set_port_position(port, new_len);
	}
      fdat->loc += chrs;
      s7i_add_format_column(sc, chrs);
    }
  else
    {
      block_t *b = s7i_mallocate(sc, chrs + 1);
      char *str = (char *)s7i_block_data(b);
      memset((void *)str, pad, chrs);
      str[chrs] = '\0';
      format_append_string(sc, fdat, str, chrs, port);
      s7i_liberate(sc, b);
    }
}

static s7_int format_read_integer(s7_int *cur_i, s7_int str_len, const char *str)
{
  /* we know that str[*cur_i] is a digit */
  s7_int i, new_int = 0;
  for (i = *cur_i; i < str_len - 1; i++)
    {
      int32_t dig = s7i_digits()[(uint8_t)str[i]];
      if (dig < 10)
	{
#if HAVE_OVERFLOW_CHECKS
	  if ((multiply_overflow(new_int, 10, &new_int)) ||
	      (add_overflow(new_int, dig, &new_int)))
	    break;
#else
	  new_int = dig + (new_int * 10);
#endif
	}
      else break;
    }
  *cur_i = i;
  return(new_int);
}

static void format_number(s7_scheme *sc, format_data_t *fdat, int32_t radix, s7_int width, s7_int precision, char float_choice, char pad, s7_pointer port)
{
  if (width < 0) width = 0;
  /* precision choice depends on float_choice if it's -1 */
  if (precision < 0)
    {
      if ((float_choice == 'e') ||
	  (float_choice == 'f') ||
	  (float_choice == 'g'))
	precision = 6;
      else
	{
	  int32_t typ = s7i_type(s7_car(fdat->args)); /* in the "int" cases, precision depends on the arg type */
	  precision = ((typ == s7i_T_INTEGER()) || (typ == s7i_T_RATIO())) ? 0 : 6;
	}}
  /* should (format #f "~F" 1/3) return "1/3"?? in CL it's "0.33333334" */

  if (pad != ' ')
    {
      char *tmp, *padtmp;
      block_t *b = NULL;
      s7_int nlen = 0;
      if (radix == 10)
	tmp = s7i_number_to_string_base_10(sc, s7_car(fdat->args), width, precision, float_choice, &nlen, S7I_P_WRITE);
      else
	{
	  b = s7i_number_to_string_with_radix(sc, s7_car(fdat->args), radix, width, precision, float_choice, &nlen);
	  tmp = (char *)s7i_block_data(b);
	}
      padtmp = tmp;
      while (*padtmp == ' ') (*(padtmp++)) = pad;
      format_append_string(sc, fdat, tmp, nlen, port);
      if (b) s7i_liberate(sc, b);
    }
  else
    {
      char *tmp;
      block_t *b = NULL;
      s7_int nlen = 0;
      if (radix == 10)
	tmp = s7i_number_to_string_base_10(sc, s7_car(fdat->args), width, precision, float_choice, &nlen, S7I_P_WRITE);
      else
	{
	  b = s7i_number_to_string_with_radix(sc, s7_car(fdat->args), radix, width, precision, float_choice, &nlen);
	  tmp = (char *)s7i_block_data(b);
	}
      format_append_string(sc, fdat, tmp, nlen, port);
      if (b) s7i_liberate(sc, b);
    }
  fdat->args = s7_cdr(fdat->args);
  fdat->ctr++;
}


static void format_ordinal_number(s7_scheme *sc, format_data_t *fdat, s7_pointer port)
{
  s7_int num = s7i_integer_clamped_if_gmp(sc, s7_car(fdat->args));
  if (num < 11)
    format_append_string(sc, fdat, ordinal[num], ordinal_length[num], port);
  else
    {
      s7_int nlen = 0;
      const char *tmp = s7i_integer_to_string(sc, num, &nlen);
      format_append_string(sc, fdat, tmp, nlen, port);
      num = num % 100;
      if ((num >= 11) && (num <= 13))
	format_append_string(sc, fdat, "th", 2, port);
      else
	{
	  num = num % 10;
	  if (num == 1) format_append_string(sc, fdat, "st", 2, port);
	  else
	    if (num == 2) format_append_string(sc, fdat, "nd", 2, port);
	    else
	      if (num == 3) format_append_string(sc, fdat, "rd", 2, port);
	      else format_append_string(sc, fdat, "th", 2, port);
	}}
  fdat->args = s7_cdr(fdat->args);
  fdat->ctr++;
}

static s7_int format_nesting(const char *str, s7_int start, s7_int end)   /* start=i, end=str_len-1, assume ~{...~} */
{
  for (s7_int k = start + 2, nesting = 1; k < end; k++)
    if (str[k] == '~')
      {
	if (str[k + 1] == '}')
	  {
	    nesting--;
	    if (nesting == 0)
	      return(k - start - 1);
	  }
	else
	  if (str[k + 1] == '{')
	    nesting++;
      }
  return(-1);
}

static bool format_method(s7_scheme *sc, const char *str, format_data_t *fdat, s7_pointer port)
{
  s7_pointer func;
  const s7_pointer obj = s7_car(fdat->args);
  char ctrl_str[3];

  if ((!s7i_has_active_methods(sc, obj)) ||
      ((func = s7i_find_method_with_let(sc, obj, s7i_format_symbol(sc))) == s7i_undefined(sc)))
    return(false);

  ctrl_str[0] = '~';
  ctrl_str[1] = str[0];
  ctrl_str[2] = '\0';

  if (port == obj)    /* a problem! we need the openlet port for format, but that's an infinite loop when it calls format again as obj */
    s7_apply_function(sc, func, s7i_set_plist_3(sc, port, s7i_wrap_string(sc, ctrl_str, 2), s7i_wrap_string(sc, "#<format port>", 14)));
  else s7_apply_function(sc, func, s7i_set_plist_3(sc, port, s7i_wrap_string(sc, ctrl_str, 2), obj));

  fdat->args = s7_cdr(fdat->args);
  fdat->ctr++;
  return(true);
}

static s7_int format_n_arg(s7_scheme *sc, const char *str, format_data_t *fdat, s7_pointer args)
{
  if (s7_is_null(sc, fdat->args))          /* (format #f "~nT") */
    format_error_nr(sc, "~N: missing argument", 20, str, args, fdat);
  if (!s7_is_integer(s7_car(fdat->args)))
    format_error_nr(sc, "~N: integer argument required", 29, str, args, fdat);
  {
    const s7_int n = s7i_integer_clamped_if_gmp(sc, s7_car(fdat->args));
    if (n < 0)
      format_error_nr(sc, "~N value is negative?", 21, str, args, fdat);
    if (n > s7i_max_string_length(sc))
      { /* desperation -- we need some string that will stay around long enough to be reported */
	int bytes = snprintf(s7i_strbuf(sc), s7i_strbuf_size(sc), "~N value is too big; (*s7* 'max-string-length) is %" ld64, s7i_max_string_length(sc));
	format_error_nr(sc, s7i_strbuf(sc), bytes, str, args, fdat);
      }
    fdat->args = s7_cdr(fdat->args);    /* I don't think fdat->ctr should be incremented here -- it's for (*s7* 'print-length) etc */
    return(n);
  }
}

static s7_int format_numeric_arg(s7_scheme *sc, const char *str, s7_int str_len, format_data_t *fdat, s7_int *i)
{
  s7_int old_i = *i;
  const s7_int width = format_read_integer(i, str_len, str);
  if (width < 0)
    {
      if (str[old_i - 1] != ',') /* need branches here, not if-expr because format_error creates the permanent string */
	format_error_nr(sc, "width is negative?", 18, str, fdat->args, fdat);
      format_error_nr(sc, "precision is negative?", 22, str, fdat->args, fdat);
    }
  if (width > s7i_max_string_length(sc))
    {
      int bytes;
      if (str[old_i - 1] != ',')
	bytes = snprintf(s7i_strbuf(sc), s7i_strbuf_size(sc), "width is too big; (*s7* 'max-string-length) is %" ld64, s7i_max_string_length(sc));
      else bytes = snprintf(s7i_strbuf(sc), s7i_strbuf_size(sc), "precision is too big; (*s7* 'max-string-length) is %" ld64, s7i_max_string_length(sc));
      format_error_nr(sc, s7i_strbuf(sc), bytes, str, fdat->args, fdat);
    }
  return(width);
}

format_data_t *make_fdat(s7_scheme *sc)
{
  format_data_t *fdat = (format_data_t *)calloc(1, sizeof(format_data_t)); /* not Malloc here! */
  fdat->curly_arg = s7i_nil(sc);
  return(fdat);
}

static format_data_t *open_format_data(s7_scheme *sc)
{
  format_data_t *fdat;
  s7i_inc_format_depth(sc);
  if (s7i_format_depth(sc) >= s7i_num_fdats(sc))
    {
      int32_t new_num_fdats = s7i_format_depth(sc) * 2;
      s7i_set_fdats(sc, (format_data_t **)realloc(s7i_fdats(sc), sizeof(format_data_t *) * new_num_fdats));
      for (int32_t k = s7i_num_fdats(sc); k < new_num_fdats; k++) s7i_fdats(sc)[k] = make_fdat(sc);
      s7i_set_num_fdats(sc, new_num_fdats);
    }
  fdat = s7i_fdats(sc)[s7i_format_depth(sc)];
#if 1
  if (fdat->port) /* happens a lot in tform */
    {
      s7i_close_format_port(sc, fdat->port);
      fdat->port = NULL;
    }
#endif
#if 0
  /* can happen but requires a lot of effort and is never repeatable! only fdat->curly_arg is GC protected?  */
  if (fdat->strport)
    {
      s7i_close_format_port(sc, fdat->strport);
      fdat->strport = NULL;
    }
#endif
  fdat->loc = 0;
  fdat->curly_arg = s7i_nil(sc);
  return(fdat);
}

#define s7i_is_one_or_big_one(Sc, Num) s7i_is_one(Num)

s7_pointer s7i_object_to_list(s7_scheme *sc, s7_pointer obj);

s7_pointer format_to_port_1(s7_scheme *sc, s7_pointer port, const char *str, s7_pointer args,
				   s7_pointer *next_arg, bool with_result, bool columnized, s7_int len, s7_pointer orig_str)
{
  s7_int i, str_len;
  format_data_t *fdat;
  s7_pointer deferred_port;
  if (len <= 0)
    {
      str_len = s7i_safe_strlen(str);
      if (str_len == 0)
	{
	  if (s7_is_pair(args))
	    s7i_error_nr(sc, s7i_format_error_symbol(sc),
		     s7i_set_elist_2(sc, s7i_wrap_string(sc, "format control string is null, but there are arguments: ~S", 58), args));
	  return(s7i_nil_string());
	}}
  else str_len = len;

  fdat = open_format_data(sc);
  fdat->args = args;
  fdat->orig_str = orig_str;

  if (with_result)
    {
      deferred_port = port;
      port = s7i_open_format_port(sc);
      fdat->port = port;
    }
  else deferred_port = s7i_F(sc);

  for (i = 0; i < str_len - 1; i++)
    {
      if ((uint8_t)(str[i]) == (uint8_t)'~')
	{
	  s7i_use_write_t use_write;
	  switch (str[i + 1])
	    {
	    case '%':                           /* -------- newline -------- */
	      /* sbcl apparently accepts numeric args here (including 0); use ~NC in s7: (format #f "~NC" 3 #\newline) */
	      if ((s7i_port_data(port)) &&
		  (s7i_port_position(port) < s7i_port_data_size(port)))
		{
		  s7_int pos = s7i_port_position(port);
		  s7i_port_data(port)[pos] = '\n';
		  s7i_set_port_position(port, pos + 1);
		  s7i_set_format_column(sc, 0);
		}
	      else format_append_newline(sc, port);
	      i++;
	      break;

	    case '&':                           /* -------- conditional newline -------- */
	      /* this only works if all output goes through format -- display/write for example do not update format_column */
	      if (s7i_format_column(sc) > 0)
		format_append_newline(sc, port);
	      i++;
	      break;

	    case '~':                           /* -------- tilde -------- */
	      format_append_char(sc, '~', port);
	      i++;
	      break;

	    case '\n':                          /* -------- trim white-space --------  so (format #f "hiho~\n") -> "hiho"! */
	      for (i = i + 2; i <str_len - 1; i++)
		if (!s7i_white_space()[(uint8_t)(str[i])])
		  {
		    i--;
		    break;
		  }
	      break;

	    case '*':                           /* -------- ignore arg -------- */
	      i++;
	      if (s7_is_null(sc, fdat->args))          /* (format #f "~*~A") */
		format_error_nr(sc, "can't skip argument!", 20, str, args, fdat);
	      fdat->args = s7_cdr(fdat->args);
	      break;

	    case '|':                           /* -------- exit if args nil or ctr > (*s7* 'print-length) -------- */
	      if ((s7_is_pair(fdat->args)) &&
		  (fdat->ctr >= s7i_print_length(sc)))
		{
		  format_append_string(sc, fdat, " ...", 4, port);
		  fdat->args = s7i_nil(sc);
		}
	      /* fall through */

	    case '^':                           /* -------- exit -------- */
	      if (s7_is_null(sc, fdat->args))
		{
		  i = str_len;
		  goto ALL_DONE;
		}
	      i++;
	      break;

	    case '@':                           /* -------- plural, 'y' or 'ies' -------- */
	      i += 2;
	      if ((str[i] != 'P') && (str[i] != 'p'))
		format_error_nr(sc, "unknown '@' directive", 21, str, args, fdat);
	      if (!s7_is_pair(fdat->args))
		format_error_nr(sc, "'@' directive argument missing", 30, str, args, fdat);
	      if (!s7_is_real(s7_car(fdat->args)))        /* CL accepts non numbers here */
		format_error_nr(sc, "'@P' directive argument is not a real number", 44, str, args, fdat);

	      if (!s7i_is_one_or_big_one(sc, s7_car(fdat->args)))
		format_append_string(sc, fdat, "ies", 3, port);
	      else format_append_char(sc, 'y', port);

	      fdat->args = s7_cdr(fdat->args);
	      break;

	    case 'P': case 'p':                 /* -------- plural in 's' -------- */
	      if (!s7_is_pair(fdat->args))
		format_error_nr(sc, "'P' directive argument missing", 30, str, args, fdat);
	      if (!s7_is_real(s7_car(fdat->args)))
		format_error_nr(sc, "'P' directive argument is not a real number", 43, str, args, fdat);
	      if (!s7i_is_one_or_big_one(sc, s7_car(fdat->args)))
		format_append_char(sc, 's', port);
	      i++;
	      fdat->args = s7_cdr(fdat->args);
	      break;

	    case '{':                           /* -------- iteration -------- */
	      {
		s7_int curly_len;

		if (s7_is_null(sc, fdat->args))
		  format_error_nr(sc, "missing argument", 16, str, args, fdat);

		if ((s7_is_pair(s7_car(fdat->args))) &&               /* any sequence is possible here */
		    (s7_list_length(sc, s7_car(fdat->args)) < 0))  /* (format #f "~{~a~e~}" (cons 1 2)) */
		  /* we can't use !s7_is_proper_list(sc, s7_car(fdat->args)) because cyclic lists are ok here */
		  format_error_nr(sc, "~{ argument is a dotted list", 28, str, args, fdat);

		curly_len = format_nesting(str, i, str_len - 1);

		if (curly_len == -1)
		  format_error_nr(sc, "'{' directive, but no matching '}'", 34, str, args, fdat);
		if (curly_len == 1)
		  format_error_nr(sc, "~{~}' doesn't consume any arguments!", 36, str, args, fdat);

		/* what about cons's here?  I can't see any way to specify the car or cdr of a cons within the format string */
		if (!s7_is_null(sc, s7_car(fdat->args)))               /* (format #f "~{~A ~}" ()) -> "" */
		  {
		    s7_pointer curly_arg = s7i_object_to_list(sc, s7_car(fdat->args)); /* if a pair (or non-sequence), this simply returns the original */
		    /* perhaps use an iterator here -- rootlet->list is expensive! */
		    if (s7_is_pair(curly_arg))                    /* (format #f "~{~A ~}" #()) -> "" */
		      {
			char *curly_str = NULL;                /* this is the local (nested) format control string */
			s7_pointer cycle_arg;

			fdat->curly_arg = curly_arg;
			if (curly_len > fdat->curly_len)
			  {
			    if (fdat->curly_str) free(fdat->curly_str);
			    fdat->curly_len = curly_len;
			    fdat->curly_str = (char *)malloc(curly_len);
			  }
			curly_str = fdat->curly_str;
			memcpy((void *)curly_str, (const void *)(str + i + 2), curly_len - 1);
			curly_str[curly_len - 1] = '\0';

			if ((s7i_format_depth(sc) < s7i_num_fdats(sc) - 1) &&
			    (s7i_fdats(sc)[s7i_format_depth(sc) + 1]))
			  s7i_fdats(sc)[s7i_format_depth(sc) + 1]->ctr = 0;

			/* it's not easy to use an iterator here instead of a list (so object->list isn't needed above),
			 *   because the curly brackets may enclose multiple arguments -- we would need to use
			 *   iterators throughout this function.
			 */
			cycle_arg = curly_arg;
			while (s7_is_pair(curly_arg))
			  {
			    s7_pointer new_arg = s7i_nil(sc);
			    format_to_port_1(sc, port, curly_str, curly_arg, &new_arg, false, columnized, curly_len - 1, NULL);
			    if (curly_arg == new_arg)
			      {
				if (s7_cdr(curly_arg) == curly_arg) break;
				fdat->curly_arg = s7i_nil(sc);
				format_error_nr(sc, "'{...}' doesn't consume any arguments!", 38, str, args, fdat);
			      }
			    curly_arg = new_arg;
			    if ((!s7_is_pair(curly_arg)) || (curly_arg == cycle_arg))
			      break;
			    cycle_arg = s7_cdr(cycle_arg);
			    format_to_port_1(sc, port, curly_str, curly_arg, &new_arg, false, columnized, curly_len - 1, NULL);
			    curly_arg = new_arg;
			  }
			fdat->curly_arg = s7i_nil(sc);
		      }
		    else
		      if (!s7_is_null(sc, curly_arg))
			format_error_nr(sc, "'{' directive argument should be a list or something we can turn into a list", 76, str, args, fdat);
		  }
		i += (curly_len + 2); /* jump past the ending '}' too */
		fdat->args = s7_cdr(fdat->args);
		fdat->ctr++;
	      }
	      break;

	    case '}':
	      format_error_nr(sc, "unmatched '}'", 13, str, args, fdat);

	    case '$':
	      use_write = S7I_P_CODE; /* affects when symbols but not keywords are quoted (symbol_to_port and hash_table_to_port) */
	      goto OBJSTR;

	    case 'W': case 'w':
	      use_write = S7I_P_READABLE;
	      goto OBJSTR;

	    case 'S': case 's':
	      use_write = S7I_P_WRITE;
	      goto OBJSTR;

	    case 'A': case 'a':
	      use_write = S7I_P_DISPLAY;
	    OBJSTR:                        /* object->string */
	      {
		s7_pointer obj;
		if (s7_is_null(sc, fdat->args))
		  format_error_nr(sc, "missing argument", 16, str, args, fdat);
		i++;
		obj = s7_car(fdat->args);
		if ((use_write == S7I_P_READABLE) ||
		    (!s7i_has_active_methods(sc, obj)) ||
		    (!format_method(sc, (const char *)(str + i), fdat, port)))
		  {
		    s7_pointer strport;
		    const bool old_openlets = s7i_has_openlets(sc);
		    /* for the column check, we need to know the length of the object->string output */
		    if (columnized)
		      {
			strport = s7i_open_format_port(sc);
			fdat->strport = strport;
		      }
		    else strport = port;
		    if (use_write == S7I_P_READABLE)
		      s7i_set_has_openlets(sc, false);
		    s7i_object_out(sc, obj, strport, use_write);
		    if (use_write == S7I_P_READABLE)
		      s7i_set_has_openlets(sc, old_openlets);
		    if (columnized)
		      {
			if (s7i_port_position(strport) >= s7i_port_data_size(strport))
			  s7i_resize_port_data(sc, strport, s7i_port_data_size(strport) * 2);
			s7i_port_data(strport)[s7i_port_position(strport)] = '\0';
			if (s7i_port_position(strport) > 0)
			  format_append_string(sc, fdat, (const char *)s7i_port_data(strport), s7i_port_position(strport), port);
			s7i_close_format_port(sc, strport);
			fdat->strport = NULL;
		      }
		    fdat->args = s7_cdr(fdat->args);
		    fdat->ctr++;
		  }}
	      break;

	      /* -------- numeric args -------- */
	    case ':':
	      i += 2;
	      if ((str[i] != 'D') && (str[i] != 'd'))
		format_error_nr(sc, "unknown ':' directive", 21, str, args, fdat);
	      if (!s7_is_pair(fdat->args))
		format_error_nr(sc, "':D' directive argument missing", 31, str, args, fdat);
	      if (!s7_is_integer(s7_car(fdat->args)))
		format_error_nr(sc, "':D' directive argument is not an integer", 41, str, args, fdat);
	      if (s7i_integer_clamped_if_gmp(sc, s7_car(fdat->args)) < 0)
		format_error_nr(sc, "':D' directive argument can't be negative", 41, str, args, fdat);
	      format_ordinal_number(sc, fdat, port);
	      break;

	    case '0': case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7': case '8': case '9': case ',':
	    case 'N': case 'n':

	    case 'B': case 'b':
	    case 'D': case 'd':
	    case 'E': case 'e':
	    case 'F': case 'f':
	    case 'G': case 'g':
	    case 'O': case 'o':
	    case 'X': case 'x':

	    case 'T': case 't':
	    case 'C': case 'c':
	      {
		s7_int width = -1, precision = -1;
		char pad = ' ';
		i++;                                      /* str[i] == '~' */

		if (s7i_digitp((int32_t)(str[i])))
		  width = format_numeric_arg(sc, str, str_len, fdat, &i);
		else
		  if ((str[i] == 'N') || (str[i] == 'n'))
		    {
		      i++;
		      width = format_n_arg(sc, str, fdat, args);
		    }
		if (str[i] == ',')
		  {
		    i++;                                  /* is (format #f "~12,12D" 1) an error?  The precision (or is it the width?) has no use here */
		    if (s7i_digitp((int32_t)(str[i])))
		      precision = format_numeric_arg(sc, str, str_len, fdat, &i);
		    else
		      if ((str[i] == 'N') || (str[i] == 'n'))
			{
			  i++;
			  precision = format_n_arg(sc, str, fdat, args);
			}
		      else
			if (str[i] == '\'')              /* (format #f "~12,'xD" 1) -> "xxxxxxxxxxx1" */
			  {
			    pad = str[i + 1];
			    i += 2;
			    if (i >= str_len)            /* (format #f "~,'") */
			      format_error_nr(sc, "incomplete numeric argument", 27, str, args, fdat);
			  }}  /* is (let ((str "~12,'xD")) (set! (str 5) #\null) (format #f str 1)) an error? */

		switch (str[i])
		  {
		    /* -------- pad to column --------
		     *   are columns numbered from 1 or 0?  there seems to be disagreement about this directive, does "space over to" mean including?
		     */
		  case 'T': case 't':
		    if (width == -1) width = 0;
		    if (precision == -1) precision = 0;
		    if ((width > 0) || (precision > 0))         /* (format #f "a~8Tb") */
		      {
			/* (length (substring (format #f "~%~10T.") 1)) == (length (format #f "~10T."))
			 * (length (substring (format #f "~%-~10T.~%") 1)) == (length (format #f "-~10T.~%"))
			 */
			if (precision > 0)
			  {
			    int32_t mult = (int32_t)(ceil((s7_double)(s7i_format_column(sc) + 1 - width) / (s7_double)precision)); /* CLtL2 ("least positive int") */
			    if (mult < 1) mult = 1;
			    width += (precision * mult);
			  }
			width -= (s7i_format_column(sc) + 1);
			if (width > 0)
			  format_append_chars(sc, fdat, pad, width, port);
		      }
		    break;

		  case 'C': case 'c':
		    {
		      s7_pointer obj;

		      if (s7_is_null(sc, fdat->args))
			format_error_nr(sc, "~~C: missing argument", 21, str, args, fdat);
		      /* the "~~" here and below protects against "~C" being treated as a directive */
		      obj = s7_car(fdat->args);
		      if (!s7_is_character(obj))
			{
			  if (!format_method(sc, (const char *)(str + i), fdat, port)) /* i stepped forward above */
			    format_error_nr(sc, "'C' directive requires a character argument", 43, str, args, fdat);
			}
		      else
			{
			  /* here use_write is false, so we just add the char, not its name */
			  if (width == -1)
			    format_append_char(sc, s7_character(obj), port);
			  else
			    if (width > 0)
			      format_append_chars(sc, fdat, s7_character(obj), width, port);

			  fdat->args = s7_cdr(fdat->args);
			  fdat->ctr++;
			}}
		    break;

		    /* -------- numbers -------- */
		  case 'F': case 'f':
		    if (s7_is_null(sc, fdat->args))
		      format_error_nr(sc, "~~F: missing argument", 21, str, args, fdat);
		    if (!s7_is_number(s7_car(fdat->args)))
		      {
			if (!format_method(sc, (const char *)(str + i), fdat, port))
			  format_error_nr(sc, "~~F: numeric argument required", 30, str, args, fdat);
		      }
		    else format_number(sc, fdat, 10, width, precision, 'f', pad, port);
		    break;

		  case 'G': case 'g':
		    if (s7_is_null(sc, fdat->args))
		      format_error_nr(sc, "~~G: missing argument", 21, str, args, fdat);
		    if (!s7_is_number(s7_car(fdat->args)))
		      {
			if (!format_method(sc, (const char *)(str + i), fdat, port))
			  format_error_nr(sc, "~~G: numeric argument required", 30, str, args, fdat);
		      }
		    else format_number(sc, fdat, 10, width, precision, 'g', pad, port);
		    break;

		  case 'E': case 'e':
		    if (s7_is_null(sc, fdat->args))
		      format_error_nr(sc, "~~E: missing argument", 21, str, args, fdat);
		    if (!s7_is_number(s7_car(fdat->args)))
		      {
			if (!format_method(sc, (const char *)(str + i), fdat, port))
			  format_error_nr(sc, "~~E: numeric argument required", 30, str, args, fdat);
		      }
		    else format_number(sc, fdat, 10, width, precision, 'e', pad, port);
		    break;

		    /* how to handle non-integer arguments in the next 4 cases?  clisp just returns
		     *   the argument: (format nil "~X" 1.25) -> "1.25" which is perverse (ClTl2 p 581:
		     *   "if arg is not an integer, it is printed in ~A format and decimal base")!!
		     *   I think I'll use the type of the number to choose the output format.
		     */
		  case 'D': case 'd':
		    if (s7_is_null(sc, fdat->args))
		      format_error_nr(sc, "~~D: missing argument", 21, str, args, fdat);
		    if (!s7_is_number(s7_car(fdat->args)))
		      {
			/* (let () (require mockery.scm) (format #f "~D" ((*mock-number* 'mock-number) 123)))
			 *    port here is a string-port, str has the width/precision data if the caller wants it,
			 *    args is the current arg.  But format_number handles fdat->args and so on, so
			 *    I think I'll pass the format method the current control string (str), the
			 *    current object (s7_car(fdat->args)), and the arglist (args), and assume it will
			 *    return a (scheme) string.
			 */
			if (!format_method(sc, (const char *)(str + i), fdat, port))
			  format_error_nr(sc, "~~D: numeric argument required", 30, str, args, fdat);
		      }
		    else format_number(sc, fdat, 10, width, precision, 'd', pad, port);
		    break;

		  case 'O': case 'o':
		    if (s7_is_null(sc, fdat->args))
		      format_error_nr(sc, "~~O: missing argument", 21, str, args, fdat);
		    if (!s7_is_number(s7_car(fdat->args)))
		      {
			if (!format_method(sc, (const char *)(str + i), fdat, port))
			  format_error_nr(sc, "~~O: numeric argument required", 30, str, args, fdat);
		      }
		    else format_number(sc, fdat, 8, width, precision, 'o', pad, port);
		    break;

		  case 'X': case 'x':
		    if (s7_is_null(sc, fdat->args))
		      format_error_nr(sc, "~~X: missing argument", 21, str, args, fdat);
		    if (!s7_is_number(s7_car(fdat->args)))
		      {
			if (!format_method(sc, (const char *)(str + i), fdat, port))
			  format_error_nr(sc, "~~X: numeric argument required", 30, str, args, fdat);
		      }
		    else format_number(sc, fdat, 16, width, precision, 'x', pad, port);
		    break;

		  case 'B': case 'b':
		    if (s7_is_null(sc, fdat->args))
		      format_error_nr(sc, "~~B: missing argument", 21, str, args, fdat);
		    if (!s7_is_number(s7_car(fdat->args)))
		      {
			if (!format_method(sc, (const char *)(str + i), fdat, port))
			  format_error_nr(sc, "~~B: numeric argument required", 30, str, args, fdat);
		      }
		    else format_number(sc, fdat, 2, width, precision, 'b', pad, port);
		    break;

		  default:
		    if (width > 0)
		      format_error_nr(sc, "unused numeric argument", 23, str, args, fdat);
		    format_error_nr(sc, "unimplemented format directive", 30, str, args, fdat);
		  }}
	      break;

	    default:
	      format_error_nr(sc, "unimplemented format directive", 30, str, args, fdat);
	    }}
      else /* str[i] is not #\~ */
	{
	  const char *p = (char *)strchr((const char *)(str + i + 1), (int)'~');
	  s7_int j = (p) ? p - str : str_len;
	  s7_int new_len = j - i;

	  if ((s7i_port_data(port)) &&
	      ((s7i_port_position(port) + new_len) < s7i_port_data_size(port)))
	    {
	      memcpy((void *)(s7i_port_data(port) + s7i_port_position(port)), (const void *)(str + i), new_len);
	      s7i_set_port_position(port, s7i_port_position(port) + new_len);
	    }
	  else s7i_port_write_string(sc, (const char *)(str + i), new_len, port);
	  fdat->loc += new_len;
	  s7i_add_format_column(sc, new_len);
	  i = j - 1;
	}}

 ALL_DONE:
  if (next_arg)
    (*next_arg) = fdat->args;
  else
    if (!s7_is_null(sc, fdat->args))
      format_error_nr(sc, "too many arguments", 18, str, args, fdat);

  if (i < str_len)
    {
      if (str[i] == '~')
	format_error_nr(sc, "control string ends in tilde", 28, str, args, fdat);
      format_append_char(sc, str[i], port);
    }
  s7i_dec_format_depth(sc);
  if (with_result)
    {
      s7_pointer result;
      if ((s7_is_output_port(sc, deferred_port)) &&
	  (s7i_port_position(port) > 0))
	{
	  if (s7i_port_position(port) < s7i_port_data_size(port))
	    s7i_port_data(port)[s7i_port_position(port)] = '\0';
	  s7i_port_write_string(sc, (const char *)s7i_port_data(port), s7i_port_position(port), deferred_port);
	}
      if (s7i_port_position(port) < s7i_port_data_size(port))
	{
	  if (s7i_port_position(port) == 0)
	    result = s7i_nil_string();
	  else
	    {
	      block_t *block = s7i_inline_mallocate(sc, s7i_FORMAT_PORT_LENGTH()); /* for format port after turning current format block into a string */
	      result = s7i_inline_block_to_string(sc, s7i_port_data_block(port), s7i_port_position(port));
	      s7i_set_port_data_size(port, s7i_FORMAT_PORT_LENGTH());
	      s7i_set_port_data_block(port, block);
	      s7i_set_port_data(port, (uint8_t *)(s7i_block_data(block)));
	      s7i_port_data(port)[0] = '\0';
	      s7i_set_port_position(port, 0);
	    }}
      else result = s7i_make_string_with_length(sc, (char *)s7i_port_data(port), s7i_port_position(port)); /* this can happen (s7test, pos/size=128) */
      s7i_close_format_port(sc, port); /* i.e. return it to the fdat free list */
      fdat->port = NULL;
      return(result);
    }
  return(s7i_nil_string());
}

bool s7i_is_columnizing(const char *str)  /* look for ~t ~,<int>T ~<int>,<int>t */
{
  const char *p = (const char *)str;
  while (*p)
    {
      if (*p++ == '~') /* this is faster than strchr */
	{
	  char c = *p++;
	  if ((c == 't') || (c == 'T')) return(true);
	  if (!c) return(false);
	  if ((c == ',') || ((c >= '0') && (c <= '9')) || (c == 'n') || (c == 'N'))
	    {
	      while (((c >= '0') && (c <= '9')) || (c == 'n') || (c == 'N')) c = *p++;
	      if ((c == 't') || (c == 'T')) return(true);
	      if (!c) return(false);                       /* ~,1 for example */
	      if (c == ',')
		{
		  c = *p++;
		  while (((c >= '0') && (c <= '9')) || (c == 'n') || (c == 'N')) c = *p++;
		  if ((c == 't') || (c == 'T')) return(true);
		  if (!c) return(false);
		}}}}
  return(false);
}

s7_pointer g_format(s7_scheme *sc, s7_pointer args)
{
  #define H_format "(format out str . args) substitutes args into str sending the result to out. Most of \
s7's format directives are taken from CL: ~% = newline, ~& = newline if the preceding output character was \
no a newline, ~~ = ~, ~<newline> trims white space, ~* skips an argument, ~^ exits {} iteration if the arg list is exhausted, \
~nT spaces over to column n, ~A prints a representation of any object, ~S is the same, but puts strings in double quotes, \
~C prints a character, numbers are handled by ~F, ~E, ~G, ~B, ~O, ~D, and ~X with preceding numbers giving \
spacing (and spacing character) and precision.  ~{ starts an embedded format directive which is ended by ~}: \n\
\n\
  >(format #f \"dashed: ~{~A~^-~}\" '(1 2 3))\n\
  \"dashed: 1-2-3\"\n\
\n\
~P inserts \"s\" if the current it is not 1 or 1.0 (use ~@P for \"ies\" or \"y\").\n\
~B is number->string in base 2, ~O in base 8, ~D base 10, ~X base 16,\n\
~E: (format #f \"~E\" 100.1) -&gt; \"1.001000e+02\" (%e in C)\n\
~F: (format #f \"~F\" 100.1) -&gt; \"100.100000\"   (%f in C)\n\
~G: (format #f \"~G\" 100.1) -&gt; \"100.1\"        (%g in C)\n\
\n\
If the 'out' argument is not an output port (i.e. #f, #t, or ()), the resultant string is returned.  If it \
is #t, the string is also sent to the current-output-port."

  #define Q_format s7_make_circular_signature(sc, 2, 3, \
                     s7i_is_string_symbol(sc), s7_make_signature(sc, 3, s7i_is_output_port_symbol(sc), s7i_s7_is_boolean_symbol(sc), s7i_is_null_symbol(sc)), s7i_T(sc))

  s7_pointer port = s7_car(args);
  if (s7_is_null(sc, port))
    {
      port = s7i_current_output_port(sc);          /* () -> (current-output-port) */
      if (port == s7i_F(sc))                       /*   otherwise () -> #f so we get a returned string, which is confusing */
	return(s7i_nil_string());                  /*   was #f 18-Mar-24 */
    }
  s7i_set_format_column(sc, 0);
  if (!((s7_is_boolean(port)) ||                  /* #f or #t */
	((s7_is_output_port(sc, port)) &&             /* (current-output-port) or call-with-open-file arg, etc */
	 (!s7i_port_is_closed(port)))))
    return(s7i_method_or_bust_sym(sc, port, s7i_format_symbol(sc), args, s7i_an_output_port_string(), 1));
  {
    s7_pointer str = s7_cadr(args);
    if (!s7_is_string(str))
      return(s7i_method_or_bust_sym(sc, str, s7i_format_symbol(sc), args, s7i_string_type_name(sc), 2));
    return(format_to_port_1(sc, (port == s7i_T(sc)) ? s7i_current_output_port(sc) : port,
			    s7i_string_value(str), s7_cddr(args), NULL, !s7_is_output_port(sc, port), true, s7i_string_length(str), str));
  }
}

const char *s7_format(s7_scheme *sc, s7_pointer args)
{
  s7_pointer result = g_format(sc, args);
  return((s7_is_string(result)) ? s7i_string_value(result) : NULL);
}

s7_pointer g_format_f(s7_scheme *sc, s7_pointer args)  /* port == #f, there are other args */
{
  s7_pointer str = s7_cadr(args);
  s7i_set_format_column(sc, 0);
  if (!s7_is_string(str))
    return(s7i_method_or_bust_sym(sc, str, s7i_format_symbol(sc), args, s7i_string_type_name(sc), 2));
  return(format_to_port_1(sc, s7i_F(sc), s7i_string_value(str), s7_cddr(args), NULL, true, true, s7i_string_length(str), str));
}

/* g_format_nr is now defined in s7_scheme_predicate.c */

s7_pointer g_format_just_control_string(s7_scheme *sc, s7_pointer args)
{
  s7_pointer port = s7_car(args);
  const s7_pointer str = s7_cadr(args);

  if (port == s7i_F(sc))
    return(str);
  if (s7_is_null(sc, port))
    {
      port = s7i_current_output_port(sc);
      if (port == s7i_F(sc))
	return(s7i_nil_string());
    }
  if (port == s7i_T(sc))
    {
      if ((s7i_current_output_port(sc) != s7i_F(sc)) && (s7i_string_length(str) != 0))
	s7i_port_write_string(sc, s7i_string_value(str), s7i_string_length(str), s7i_current_output_port(sc));
      return(str);
    }
  if ((!s7_is_output_port(sc, port)) ||
      (s7i_port_is_closed(port)))
    return(s7i_method_or_bust_sym(sc, port, s7i_format_symbol(sc), args, s7i_a_format_port_string(), 1));

  if (s7i_string_length(str) == 0)
    return(s7i_nil_string());

  s7i_port_write_string(sc, s7i_string_value(str), s7i_string_length(str), port);
  return(s7i_nil_string());
}

s7_pointer g_format_as_objstr(s7_scheme *sc, s7_pointer args)
{
  s7_pointer func, obj = s7_caddr(args);
  if ((!s7i_has_active_methods(sc, obj)) ||
      ((func = s7i_find_method_with_let(sc, obj, s7i_format_symbol(sc))) == s7i_undefined(sc)))
    return(s7_object_to_string(sc, obj, false));
  return(s7_apply_function(sc, func, s7i_set_plist_3(sc, s7i_F(sc), s7_cadr(args), obj)));
}

s7_pointer g_format_no_column(s7_scheme *sc, s7_pointer args)
{
  s7_pointer port = s7_car(args);
  if (s7_is_null(sc, port))
    {
      port = s7i_current_output_port(sc);
      if (port == s7i_F(sc))
	return(s7i_nil_string());
    }
  if (!((s7_is_boolean(port)) ||
	((s7_is_output_port(sc, port)) &&             /* (current-output-port) or call-with-open-file arg, etc */
	 (!s7i_port_is_closed(port)))))
    return(s7i_method_or_bust_sym(sc, port, s7i_format_symbol(sc), args, s7i_a_format_port_string(), 1));
  {
    s7_pointer str = s7_cadr(args);
    s7i_set_format_column(sc, 0);
    return(format_to_port_1(sc, (port == s7i_T(sc)) ? s7i_current_output_port(sc) : port,
			    s7i_string_value(str), s7_cddr(args), NULL,
			    !s7_is_output_port(sc, port),   /* i.e. is boolean as port so we're returning a string */
			    false,                 /* we checked in advance that it is not columnized */
			    s7i_string_length(str), str));
  }
}

s7_pointer format_chooser(s7_scheme *sc, s7_pointer func, int32_t args, s7_pointer expr)
{
  if (args > 1)
    {
      const s7_pointer port = s7_cadr(expr);
      const s7_pointer str_arg = s7_caddr(expr);
      if (s7_is_string(str_arg))
	{
	  if ((args == 2) || (args == 3))
	    {
	      s7_int len;
	      char *orig = s7i_string_value_ptr(str_arg);
	      const char *p = strchr((const char *)orig, (int)'~');
	      if (!p)
		return((args == 2) ? s7i_format_just_control_string(sc) : func);

	      len = s7i_string_length(str_arg);
	      if ((args == 2) &&
		  (len > 1) &&
		  (orig[len - 1] == '%') &&
		  ((p - orig) == len - 2))
		{
		  orig[len - 2] = '\n';
		  orig[len - 1] = '\0';
		  s7i_set_string_length(str_arg, len - 1);
		  return(s7i_format_just_control_string(sc));
		}
	      if ((args == 3) && /* (format #f "~a" obj) */
		  (port == s7i_F(sc)) &&
		  (len == 2) &&  /*            "~a"      */
		  (orig[0] == '~') && ((orig[1] == 'A') || (orig[1] == 'a')))
		return(s7i_format_as_objstr(sc));
	    }
	  /* this used to worry about optimized expr and particular cases -- why? I can't find a broken case */
	  if (!s7i_is_columnizing(s7i_string_value(str_arg)))
	    return(s7i_format_no_column(sc));
	}
      if (port == s7i_F(sc)) return(s7i_format_f(sc));
    }
  return(func);
}




