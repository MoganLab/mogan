/* s7_liii_string.c - string utility implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_liii_string.h"
#include "s7.h"
#include "s7_internal_helpers.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Externally defined in s7.c - character cache */
extern s7_pointer *chars;

/* Helper function for out-of-range errors */
static s7_pointer string_ref_out_of_range(s7_scheme *sc, s7_int index, bool is_negative)
{
  return s7_out_of_range_error(sc, "string-ref", 2, s7_make_integer(sc, index),
                               is_negative ? "it is negative" : "it is too large");
}

static s7_pointer method_or_bust(s7_scheme *sc, s7_pointer obj, const char *name, const char *type_name)
{
  s7_pointer sym = s7_make_symbol(sc, name);
  s7_pointer func = s7_method(sc, obj, sym);
  if (func != s7_undefined(sc))
    return(s7_apply_function(sc, func, s7_cons(sc, obj, s7_nil(sc))));
  return(s7_wrong_type_arg_error(sc, name, 1, obj, type_name));
}

/* -------------------------------- string? -------------------------------- */

s7_pointer g_is_string(s7_scheme *sc, s7_pointer args)
{
  s7_pointer p = s7_car(args);
  if (s7_is_string(p)) return(s7_t(sc));
  {
    s7_pointer func = s7_method(sc, p, s7_make_symbol(sc, "string?"));
    if (func == s7_undefined(sc)) return(s7_f(sc));
    return(s7_apply_function(sc, func, s7_cons(sc, p, s7_nil(sc))));
  }
}

/* -------------------------------- char-position -------------------------------- */

s7_pointer
g_char_position (s7_scheme* sc, s7_pointer args) {
  const s7_pointer arg1= s7_car (args);
  if ((!s7_is_character (arg1)) && (!s7_is_string (arg1)))
    return s7i_method_or_bust (sc, arg1, "char-position", args, "a character", 1);

  const s7_pointer arg2= s7_cadr (args);
  if (!s7_is_string (arg2)) return s7i_method_or_bust (sc, arg2, "char-position", args, "a string", 2);

  s7_int start= 0;
  if (s7_is_pair (s7_cddr (args))) {
    const s7_pointer arg3= s7_caddr (args);
    if (!s7_is_integer (arg3)) return s7i_method_or_bust (sc, arg3, "char-position", args, "an integer", 3);
    start= s7_number_to_integer_with_caller (sc, arg3, "char-position");
    if (start < 0) return s7_wrong_type_arg_error (sc, "char-position", 3, arg3, "a non-negative integer");
  }

  const char*  porig= s7_string (arg2);
  const s7_int len  = s7_string_length (arg2);
  if (start >= len) return s7_f (sc);

  if (s7_is_character (arg1)) {
    const char  c= (char) s7_character (arg1);
    const char* p= strchr ((const char*) (porig + start), (int) c);
    return p ? s7_make_integer (sc, (s7_int) (p - porig)) : s7_f (sc);
  }

  if (s7_string_length (arg1) == 0) return s7_f (sc);

  const char*  pset= s7_string (arg1);
  const s7_int pos = (s7_int) strcspn ((const char*) (porig + start), (const char*) pset);
  if ((pos + start) < len) return s7_make_integer (sc, pos + start);

  return s7_f (sc);
}

s7_pointer
char_position_p_ppi (s7_scheme* sc, s7_pointer chr, s7_pointer str, s7_int start) {
  if (!s7_is_string (str)) return s7_wrong_type_arg_error (sc, "char-position", 2, str, "a string");
  if (start < 0)
    return s7_wrong_type_arg_error (sc, "char-position", 3, s7_make_integer (sc, start), "a non-negative integer");

  const char*  porig= s7_string (str);
  const s7_int len  = s7_string_length (str);
  if (start >= len) return s7_f (sc);

  const char  c= (char) s7_character (chr);
  const char* p= strchr ((const char*) (porig + start), (int) c);
  return p ? s7_make_integer (sc, (s7_int) (p - porig)) : s7_f (sc);
}

s7_pointer
g_char_position_csi (s7_scheme* sc, s7_pointer args) {
  const s7_pointer arg2= s7_cadr (args);
  if (!s7_is_string (arg2)) return g_char_position (sc, args);

  const s7_int len  = s7_string_length (arg2);
  s7_int       start= 0;

  if (s7_is_pair (s7_cddr (args))) {
    const s7_pointer arg3= s7_caddr (args);
    if (!s7_is_integer (arg3)) return g_char_position (sc, args);
    start= s7_number_to_integer_with_caller (sc, arg3, "char-position");
    if (start < 0) return s7_wrong_type_arg_error (sc, "char-position", 3, arg3, "a non-negative integer");
    if (start >= len) return s7_f (sc);
  }

  if (len == 0) return s7_f (sc);

  const char* porig= s7_string (arg2);
  const char  c    = (char) s7_character (s7_car (args));
  const char* p    = strchr ((const char*) (porig + start), (int) c);
  return p ? s7_make_integer (sc, (s7_int) (p - porig)) : s7_f (sc);
}

/* -------------------------------- string-position -------------------------------- */

s7_pointer g_string_position(s7_scheme *sc, s7_pointer args)
{
  const s7_pointer str1 = s7_car(args), str2 = s7_cadr(args);

  if (!s7_is_string(str1))
    return method_or_bust(sc, str1, "string-position", "a string");
  if (!s7_is_string(str2))
    return method_or_bust(sc, str2, "string-position", "a string");

  s7_int start = 0;
  if (s7_is_pair(s7_cddr(args)))
    {
      const s7_pointer arg3 = s7_caddr(args);
      if (!s7_is_integer(arg3))
        return method_or_bust(sc, arg3, "string-position", "an integer");
      start = s7_integer(arg3);
      if (start < 0)
        return s7_wrong_type_arg_error(sc, "string-position", 3, arg3, "a non-negative integer");
    }

  if (s7_string_length(str1) == 0) return s7_f(sc);
  if (start >= s7_string_length(str2)) return s7_f(sc);

  const char *s1 = s7_string(str1);
  const char *s2 = s7_string(str2);
  const char *p2 = strstr((const char *)(s2 + start), s1);
  return p2 ? s7_make_integer(sc, (s7_int)(p2 - s2)) : s7_f(sc);
}

/* -------------------------------- string-ref -------------------------------- */

s7_pointer string_ref_1(s7_scheme *sc, s7_pointer strng, s7_pointer index)
{
  if (!s7_is_integer(index))
    return s7_wrong_type_arg_error(sc, "string-ref", 2, index, "an integer");

  s7_int ind = s7_integer(index);
  if (ind < 0)
    return string_ref_out_of_range(sc, ind, true);
  if (ind >= s7_string_length(strng))
    return string_ref_out_of_range(sc, ind, false);

  const char *str = s7_string(strng);
  return chars[((uint8_t *)str)[ind]];
}

s7_pointer g_string_ref(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  if (!s7_is_string(str))
    return method_or_bust(sc, str, "string-ref", "a string");
  return string_ref_1(sc, str, s7_cadr(args));
}

/* -------------------------------- string-set! -------------------------------- */

s7_pointer g_string_set(s7_scheme *sc, s7_pointer args)
{
  s7_pointer strng = s7_car(args);
  s7_pointer index = s7_cadr(args);

  if (!s7_is_string(strng))
    return method_or_bust(sc, strng, "string-set!", "a string");
  if (s7_is_immutable(strng))
    return s7_wrong_type_arg_error(sc, "string-set!", 1, strng, "a mutable string");
  if (!s7_is_integer(index))
    return s7_wrong_type_arg_error(sc, "string-set!", 2, index, "an integer");

  s7_int ind = s7_integer(index);
  if (ind < 0)
    return s7_out_of_range_error(sc, "string-set!", 2, index, "it is negative");
  if (ind >= s7_string_length(strng))
    return s7_out_of_range_error(sc, "string-set!", 2, index, "it is too large");

  s7_pointer c = s7_caddr(args);
  if (!s7_is_character(c))
    return s7_wrong_type_arg_error(sc, "string-set!", 3, c, "a character");

  char *str = (char *)s7_string(strng);
  str[ind] = (char)s7_character(c);
  return c;
}

/*---------------------------------string-length---------------------------------*/

s7_pointer g_string_length(s7_scheme *sc, s7_pointer args)
{
  #define H_string_length "(string-length str) returns the length of the string str"
  #define Q_string_length s7_make_signature(sc, 2, sc->is_integer_symbol, sc->is_string_symbol)
  s7_pointer str = s7_car(args);
  if (!s7_is_string(str))
    return(s7i_sole_arg_method_or_bust(sc, str, "string-length", args, "a string"));
  return(s7_make_integer(sc, s7_string_length(str)));
}

s7_pointer g_make_string(s7_scheme *sc, s7_pointer args)
{
  s7_pointer n = s7_car(args);
  s7_int len;
  if (!s7_is_integer(n))
    return(s7i_method_or_bust(sc, n, "make-string", args, "an integer", 1));
  if (s7_is_pair(s7_cdr(args)) && !s7_is_character(s7_cadr(args)))
    return(s7i_method_or_bust(sc, s7_cadr(args), "make-string", args, "a character", 2));

  len = s7_integer(n);
  if (len == 0) return(s7i_nil_string());
  if (len < 0)
    return(s7_out_of_range_error(sc, "make-string", 1, n, "it is negative"));
  if (len > s7i_max_string_length(sc))
    return(s7_out_of_range_error(sc, "make-string", 1, n, "it is too large"));
  if (s7_is_null(sc, s7_cdr(args)))
    return(s7i_make_empty_string(sc, len, '\0'));
  {
    s7_pointer c = s7_cadr(args);
    if (s7_character(c) > 0xFF)
      return(s7_out_of_range_error(sc, "make-string", 2, c,
                                   "make-string only accepts characters in range #x00..#xFF; use utf8-make-string for Unicode characters"));
    char fill = (char)s7_character(c);
    s7_pointer result = s7i_make_empty_string(sc, len, fill);
    if (fill == '\0')
      {
        char *str = (char *)s7_string(result);
        memset(str, 0, len);
      }
    return(result);
  }
}

s7_pointer s7i_string_to_number(s7_scheme *sc, char *str, int32_t radix)
{
  s7_pointer x = make_atom(sc, str, radix, false, false);
  return((s7_is_number(x)) ? x : s7_f(sc));
}

s7_pointer g_string_to_number(s7_scheme *sc, s7_pointer args)
{
  s7_int radix;
  char *str;
  if (!s7_is_string(s7_car(args)))
    return(s7i_method_or_bust(sc, s7_car(args), "string->number", args, "a string", 1));

  if (s7_is_pair(s7_cdr(args)))
    {
      s7_pointer rad = s7_cadr(args);
      if (!s7_is_integer(rad))
	return(s7i_method_or_bust(sc, rad, "string->number", args, "an integer", 2));
      radix = (int32_t)s7_integer(rad);
      if ((radix < 2) || (radix > 16))
	return(s7_out_of_range_error(sc, "string->number", 2, rad, "a valid radix"));
    }
  else radix = 10;
  str = (char *)s7_string(s7_car(args));
  if ((!str) || (!*str))
    return(s7_f(sc));
  return(s7i_string_to_number(sc, str, radix));
}

s7_pointer string_to_number_p_p(s7_scheme *sc, s7_pointer str1)
{
  char *str;
  if (!s7_is_string(str1))
    return(s7_wrong_type_arg_error(sc, "string->number", 1, str1, "a string"));
  str = (char *)s7_string(str1);
  return(((!str) || (!*str)) ? s7_f(sc) : s7i_string_to_number(sc, str, 10));
}

s7_pointer string_to_number_p_pp(s7_scheme *sc, s7_pointer str1, s7_pointer radix1)
{
  s7_int radix;
  char *str;
  if (!s7_is_string(str1))
    return(s7_wrong_type_arg_error(sc, "string->number", 1, str1, "a string"));

  if (!s7_is_integer(radix1))
    return(s7_wrong_type_arg_error(sc, "string->number", 2, radix1, "an integer"));
  radix = s7_integer(radix1);
  if ((radix < 2) || (radix > 16))
    return(s7_out_of_range_error(sc, "string->number", 2, radix1, "a valid radix"));

  str = (char *)s7_string(str1);
  if ((!str) || (!*str))
    return(s7_f(sc));
  return(s7i_string_to_number(sc, str, radix));
}

s7_pointer g_substring(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  s7_int start = 0, end, len;
  const char *s;

  if (!s7_is_string(str))
    return(s7i_method_or_bust(sc, str, "substring", args, "a string", 1));
  end = s7_string_length(str);
  if (!s7_is_null(sc, s7_cdr(args)))
    {
      s7_pointer p = s7i_start_and_end(sc, s7_make_symbol(sc, "substring"), args, 2, s7_cdr(args), &start, &end);
      if (!s7i_is_unused(sc, p)) return(p);
    }
  s = s7_string(str);
  len = end - start;
  if (len == 0) return(s7i_nil_string());
  return(s7_make_string_with_length(sc, s + start, len));
}

s7_pointer g_string_copy(s7_scheme *sc, s7_pointer args)
{
  s7_pointer source = s7_car(args);
  s7_pointer p, dest;
  s7_int start, end;

  if (!s7_is_string(source))
    return(s7i_method_or_bust(sc, source, "string-copy", args, "a string", 1));
  if (s7_is_null(sc, s7_cdr(args)))
    {
      if (s7_string_length(source) == 0) return(s7i_nil_string());
      return(s7_make_string_with_length(sc, s7_string(source), s7_string_length(source)));
    }
  dest = s7_cadr(args);
  if (!s7_is_string(dest))
    return(s7i_method_or_bust(sc, dest, "string-copy", args, "a string", 2));
  if (s7_is_immutable(dest))
    return(s7_wrong_type_arg_error(sc, "string-copy", 2, dest, "a mutable string"));

  end = s7_string_length(dest);
  p = s7_cddr(args);
  if (s7_is_null(sc, p))
    start = 0;
  else
    {
      if (!s7_is_integer(s7_car(p)))
	return(s7i_method_or_bust(sc, s7_car(p), "string-copy", args, "an integer", 3));
      start = s7_integer(s7_car(p));
      if (start < 0) start = 0;
      p = s7_cdr(p);
      if (s7_is_null(sc, p))
	end = start + s7_string_length(source);
      else
	{
	  if (!s7_is_integer(s7_car(p)))
	    return(s7i_method_or_bust(sc, s7_car(p), "string-copy", args, "an integer", 4));
	  end = s7_integer(s7_car(p));
	  if (end < 0) end = start;
	}}
  if (end > s7_string_length(dest)) end = s7_string_length(dest);
  if (end <= start) return(dest);
  if ((end - start) > s7_string_length(source)) end = start + s7_string_length(source);
  memmove((void *)((char *)s7_string(dest) + start), (const void *)s7_string(source), end - start);
  return(dest);
}

s7_pointer g_string_fill(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  s7_pointer chr;
  s7_int start = 0, end;

  if (!s7_is_string(str))
    return(s7i_method_or_bust(sc, str, "string-fill!", args, "a string", 1));
  if (s7_is_immutable(str))
    return(s7_wrong_type_arg_error(sc, "string-fill!", 1, str, "a mutable string"));

  chr = s7_cadr(args);
  if (!s7_is_character(chr))
    return(s7i_method_or_bust(sc, chr, "string-fill!", args, "a character", 2));

  end = s7_string_length(str);
  if (!s7_is_null(sc, s7_cddr(args)))
    {
      s7_pointer p = s7i_start_and_end(sc, s7_make_symbol(sc, "string-fill!"), args, 3, s7_cddr(args), &start, &end);
      if (!s7i_is_unused(sc, p)) return(p);
      if (start == end) return(chr);
    }
  if (end == 0) return(chr);
  memset((void *)((char *)s7_string(str) + start), (int)s7_character(chr), (size_t)(end - start));
  return(chr);
}

s7_pointer g_string_to_list(s7_scheme *sc, s7_pointer args)
{
  s7_int start = 0, end;
  s7_pointer str = s7_car(args);

  if (!s7_is_string(str))
    return(s7i_sole_arg_method_or_bust(sc, str, "string->list", args, "a string"));
  end = s7_string_length(str);
  if (!s7_is_null(sc, s7_cdr(args)))
    {
      s7_pointer p = s7i_start_and_end(sc, s7_make_symbol(sc, "string->list"), args, 2, s7_cdr(args), &start, &end);
      if (!s7i_is_unused(sc, p)) return(p);
      if (start == end) return(s7_nil(sc));
    }
  else
    if (end == 0) return(s7_nil(sc));
  if ((end - start) > s7i_max_list_length(sc))
    return(s7_out_of_range_error(sc, "string->list", 2, s7_make_integer(sc, end - start), "it is too large"));
  {
    s7_pointer result = s7_nil(sc);
    for (s7_int i = end - 1; i >= start; i--)
      result = s7_cons(sc, chars[((uint8_t)s7_string(str)[i])], result);
    return(result);
  }
}

/* g_string_append is now defined below with the full string-append implementation */

s7_pointer g_string(s7_scheme *sc, s7_pointer args)
{
  if (s7_is_null(sc, args)) return(s7i_nil_string());
  return(s7i_string_1(sc, args, s7_make_symbol(sc, "string")));
}

s7_pointer g_substring_uncopied(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  s7_int start = 0, end;

  if (!s7_is_string(str))
    return(s7i_method_or_bust(sc, str, "substring-uncopied", args, "a string", 1));
  end = s7_string_length(str);
  if (!s7_is_null(sc, s7_cdr(args)))
    {
      s7_pointer p = s7i_start_and_end(sc, s7_make_symbol(sc, "substring-uncopied"), args, 2, s7_cdr(args), &start, &end);
      if (!s7i_is_unused(sc, p)) return(p);
    }
  return(s7_make_string_with_length(sc, s7_string(str) + start, end - start));
}

#if !WITH_PURE_S7
s7_pointer g_list_to_string(s7_scheme *sc, s7_pointer args)
{
  if (s7_is_null(sc, s7_car(args)))
    return(s7i_nil_string());
  if (!s7_is_proper_list(sc, s7_car(args)))
    return(s7i_method_or_bust_p(sc, s7_car(args), "list->string",
                                "a (proper, non-circular) list of characters"));
  return(s7i_string_1(sc, s7_car(args), s7_make_symbol(sc, "list->string")));
}
#endif

/* -------------------------------- string-append -------------------------------- */

static void string_append_2(s7_scheme *sc, s7_pointer newstr, s7_pointer args, const s7_pointer stop_arg, s7_pointer caller)
{
  s7_int len;
  char *pos = s7i_string_value_ptr(newstr);
  for (s7_pointer strs = args; strs != stop_arg; strs = s7_cdr(strs))
    {
      s7_pointer str = s7_car(strs);
      if (s7_is_string(str))
        {
          len = s7_string_length(str);
          if (len > 0)
            {
              memcpy(pos, s7_string(str), len);
              pos += len;
            }}
      else
        if (!s7i_sequence_is_empty(sc, str))
          {
            char *old_str = s7i_string_value_ptr(newstr);
            s7i_set_string_value(newstr, pos);
            len = s7i_sequence_length(sc, str);
            s7i_copy_1(sc, caller, s7i_set_plist_2(sc, str, newstr));
            s7i_set_string_value(newstr, old_str);
            pos += len;
          }}
}

s7_pointer s7i_string_append_1(s7_scheme *sc, s7_pointer args, s7_pointer caller)
{
  s7_int len = 0;
  s7_pointer newstr;
  bool just_strings = true;

  if (s7_is_null(sc, args))
    return(s7i_nil_string());

  s7_gc_protect_via_stack(sc, args);
  /* get length for new string */
  for (s7_pointer strs = args; s7_is_pair(strs); strs = s7_cdr(strs))
    {
      const s7_pointer str = s7_car(strs);
      if (s7_is_string(str))
        len += s7_string_length(str);
      else
        {
          s7_int newlen;
          if (!s7i_is_sequence(str))
            {
              s7_gc_unprotect_via_stack(sc, args);
              s7i_wrong_type_error_nr(sc, caller, s7i_position_of(strs, args), str, s7_name_to_value(sc, "string"));
            }
          if (s7i_has_active_methods(sc, str))
            {
              const s7_pointer func = s7i_find_method_with_let(sc, str, caller);
              if (func != s7_name_to_value(sc, "undefined"))
                {
                  if (len == 0)
                    {
                      s7_gc_unprotect_via_stack(sc, args);
                      return(s7_apply_function(sc, func, strs));
                    }
                  newstr = s7i_make_empty_string(sc, len, '\0');
                  string_append_2(sc, newstr, args, strs, caller);
                  s7_gc_unprotect_via_stack(sc, args);
                  return(s7_apply_function(sc, func, s7i_set_ulist_1(sc, newstr, strs)));
                }}
          if (s7i_is_string_append_or_symbol_caller(sc, caller))
            {
              s7_gc_unprotect_via_stack(sc, args);
              s7i_wrong_type_error_nr(sc, caller, s7i_position_of(strs, args), str, s7_name_to_value(sc, "string"));
            }
          newlen = s7i_sequence_length(sc, str);
          if (newlen < 0)
            {
              s7_gc_unprotect_via_stack(sc, args);
              s7i_wrong_type_error_nr(sc, caller, s7i_position_of(strs, args), str, s7_name_to_value(sc, "string"));
            }
          just_strings = false;
          len += newlen;
        }}
  if (len == 0)
    {
      s7_gc_unprotect_via_stack(sc, args);
      return(s7i_nil_string());
    }
  if (len > s7i_max_string_length(sc))
    {
      s7_gc_unprotect_via_stack(sc, args);
      s7i_string_append_length_error(sc, caller, len);
    }
  newstr = s7i_make_empty_string(sc, len, '\0');
  if (just_strings)
    {
      s7_pointer strs = args;
      for (char *pos = (char *)s7_string(newstr); s7_is_pair(strs); strs = s7_cdr(strs))
        {
          len = s7_string_length(s7_car(strs));
          if (len > 0)
            {
              memcpy(pos, s7_string(s7_car(strs)), len);
              pos += len;
            }}}
  else string_append_2(sc, newstr, args, s7_nil(sc), caller);
  s7_gc_unprotect_via_stack(sc, args);
  return(newstr);
}

s7_pointer g_string_append(s7_scheme *sc, s7_pointer args)
{
  return(s7i_string_append_1(sc, args, s7_make_symbol(sc, "string-append")));
}

static s7_pointer string_append_1(s7_scheme *sc, s7_pointer s1, s7_pointer s2)
{
  if ((s7_is_string(s1)) && (s7_is_string(s2)))
    {
      s7_int len;
      const s7_int pos = s7_string_length(s1);
      s7_pointer newstr;
      if (pos == 0) return(s7_make_string_with_length(sc, s7_string(s2), s7_string_length(s2)));
      len = pos + s7_string_length(s2);
      if (len == pos) return(s7_make_string_with_length(sc, s7_string(s1), s7_string_length(s1)));
      if (len > s7i_max_string_length(sc))
        s7i_string_append_length_error(sc, s7_make_symbol(sc, "string-append"), len);
      s7_gc_protect_via_stack(sc, s2);
      newstr = s7i_make_empty_string(sc, len, '\0');
      memcpy((char *)s7_string(newstr), s7_string(s1), pos);
      memcpy((char *)(s7_string(newstr) + pos), s7_string(s2), s7_string_length(s2));
      s7_gc_unprotect_via_stack(sc, s2);
      return(newstr);
    }
  return(s7i_string_append_1(sc, s7_list(sc, 2, s1, s2), s7_make_symbol(sc, "string-append")));
}

s7_pointer string_append_p_pp(s7_scheme *sc, s7_pointer s1, s7_pointer s2) {return(string_append_1(sc, s1, s2));}

s7_pointer g_string_append_2(s7_scheme *sc, s7_pointer args) {return(string_append_1(sc, s7_car(args), s7_cadr(args)));}

/* -------------------------------- string comparisons -------------------------------- */

static s7_pointer g_string_cmp(s7_scheme *sc, s7_pointer args, int32_t val, s7_pointer sym)
{
  s7_pointer str = s7_car(args);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(str))
    return(s7i_method_or_bust_sym(sc, str, sym, args, tn, 1));
  for (s7_pointer strs = s7_cdr(args); s7_is_pair(strs); str = s7_car(strs), strs = s7_cdr(strs))
    {
      if (!s7_is_string(s7_car(strs)))
        return(s7i_method_or_bust_sym(sc, s7_car(strs), sym, s7i_set_ulist_1(sc, str, strs), tn, s7i_position_of(strs, args)));
      if (s7i_scheme_strcmp(str, s7_car(strs)) != val)
        {
          for (str = s7_cdr(strs); s7_is_pair(str); str = s7_cdr(str))
            if (!s7i_is_string_via_method(sc, s7_car(str)))
              s7i_wrong_type_error_nr(sc, sym, s7i_position_of(str, args), s7_car(str), tn);
          return(s7_f(sc));
        }}
  return(s7_t(sc));
}

static s7_pointer g_string_cmp_not(s7_scheme *sc, s7_pointer args, int32_t val, s7_pointer sym)
{
  s7_pointer str = s7_car(args);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(str))
    return(s7i_method_or_bust_sym(sc, str, sym, args, tn, 1));
  for (s7_pointer strs = s7_cdr(args); s7_is_pair(strs); str = s7_car(strs), strs = s7_cdr(strs))
    {
      if (!s7_is_string(s7_car(strs)))
        return(s7i_method_or_bust_sym(sc, s7_car(strs), sym, s7i_set_ulist_1(sc, str, strs), tn, s7i_position_of(strs, args)));
      if (s7i_scheme_strcmp(str, s7_car(strs)) == val)
        {
          for (str = s7_cdr(strs); s7_is_pair(str); str = s7_cdr(str))
            if (!s7i_is_string_via_method(sc, s7_car(str)))
              s7i_wrong_type_error_nr(sc, sym, s7i_position_of(str, args), s7_car(str), tn);
          return(s7_f(sc));
        }}
  return(s7_t(sc));
}

s7_pointer g_strings_are_equal(s7_scheme *sc, s7_pointer args)
{
  s7_pointer str = s7_car(args);
  s7_pointer eq_sym = s7i_string_eq_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);

  if (!s7_is_string(str))
    return(s7i_method_or_bust_sym(sc, str, eq_sym, args, tn, 1));
  for (s7_pointer arglist = s7_cdr(args); s7_is_pair(arglist); arglist = s7_cdr(arglist))
    {
      s7_pointer p = s7_car(arglist);
      if (!s7_is_string(p))
        return(s7i_method_or_bust_sym(sc, p, eq_sym, s7i_set_ulist_1(sc, str, arglist), tn, s7i_position_of(arglist, args)));
      if (!s7i_scheme_strings_are_equal(p, str))
        {
          for (str = s7_cdr(arglist); s7_is_pair(str); str = s7_cdr(str))
            if (!s7i_is_string_via_method(sc, s7_car(str)))
              s7i_wrong_type_error_nr(sc, eq_sym, s7i_position_of(str, args), s7_car(str), tn);
          return(s7_f(sc));
        }}
  return(s7_t(sc));
}

s7_pointer g_strings_are_less(s7_scheme *sc, s7_pointer args)
{
  return(g_string_cmp(sc, args, -1, s7i_string_lt_symbol(sc)));
}

s7_pointer g_strings_are_greater(s7_scheme *sc, s7_pointer args)
{
  return(g_string_cmp(sc, args, 1, s7i_string_gt_symbol(sc)));
}

s7_pointer g_strings_are_geq(s7_scheme *sc, s7_pointer args)
{
  return(g_string_cmp_not(sc, args, -1, s7i_string_geq_symbol(sc)));
}

s7_pointer g_strings_are_leq(s7_scheme *sc, s7_pointer args)
{
  return(g_string_cmp_not(sc, args, 1, s7i_string_leq_symbol(sc)));
}

s7_pointer g_string_equal_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer eq_sym = s7i_string_eq_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(s7_car(args)))
    return(s7i_method_or_bust_sym(sc, s7_car(args), eq_sym, args, tn, 1));
  if (!s7_is_string(s7_cadr(args)))
    return(s7i_method_or_bust_sym(sc, s7_cadr(args), eq_sym, args, tn, 2));
  return(s7_make_boolean(sc, s7i_scheme_strings_are_equal(s7_car(args), s7_cadr(args))));
}

s7_pointer g_string_equal_2c(s7_scheme *sc, s7_pointer args)
{
  s7_pointer eq_sym = s7i_string_eq_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(s7_car(args)))
    return(s7i_method_or_bust_sym(sc, s7_car(args), eq_sym, args, tn, 1));
  return(s7_make_boolean(sc, s7i_scheme_strings_are_equal(s7_car(args), s7_cadr(args))));
}

s7_pointer string_eq_p_pp(s7_scheme *sc, s7_pointer str1, s7_pointer str2)
{
  s7_pointer eq_sym = s7i_string_eq_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(str1))
    return(s7i_method_or_bust_sym(sc, str1, eq_sym, s7i_set_plist_2(sc, str1, str2), tn, 1));
  if (!s7_is_string(str2))
    return(s7i_method_or_bust_sym(sc, str2, eq_sym, s7i_set_plist_2(sc, str1, str2), tn, 2));
  return(s7_make_boolean(sc, s7i_scheme_strings_are_equal(str1, str2)));
}

s7_pointer g_string_less_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer lt_sym = s7i_string_lt_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(s7_car(args)))
    return(s7i_method_or_bust_sym(sc, s7_car(args), lt_sym, args, tn, 1));
  if (!s7_is_string(s7_cadr(args)))
    return(s7i_method_or_bust_sym(sc, s7_cadr(args), lt_sym, args, tn, 2));
  return(s7_make_boolean(sc, s7i_scheme_strcmp(s7_car(args), s7_cadr(args)) == -1));
}

s7_pointer string_lt_p_pp(s7_scheme *sc, s7_pointer str1, s7_pointer str2)
{
  s7_pointer lt_sym = s7i_string_lt_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(str1))
    return(s7i_method_or_bust_sym(sc, str1, lt_sym, s7i_set_plist_2(sc, str1, str2), tn, 1));
  if (!s7_is_string(str2))
    return(s7i_method_or_bust_sym(sc, str2, lt_sym, s7i_set_plist_2(sc, str1, str2), tn, 2));
  return(s7_make_boolean(sc, s7i_scheme_strcmp(str1, str2) == -1));
}

s7_pointer g_string_greater_2(s7_scheme *sc, s7_pointer args)
{
  s7_pointer gt_sym = s7i_string_gt_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(s7_car(args)))
    return(s7i_method_or_bust_sym(sc, s7_car(args), gt_sym, args, tn, 1));
  if (!s7_is_string(s7_cadr(args)))
    return(s7i_method_or_bust_sym(sc, s7_cadr(args), gt_sym, args, tn, 2));
  return(s7_make_boolean(sc, s7i_scheme_strcmp(s7_car(args), s7_cadr(args)) == 1));
}

s7_pointer string_gt_p_pp(s7_scheme *sc, s7_pointer str1, s7_pointer str2)
{
  s7_pointer gt_sym = s7i_string_gt_symbol(sc);
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(str1))
    return(s7i_method_or_bust_sym(sc, str1, gt_sym, s7i_set_plist_2(sc, str1, str2), tn, 1));
  if (!s7_is_string(str2))
    return(s7i_method_or_bust_sym(sc, str2, gt_sym, s7i_set_plist_2(sc, str1, str2), tn, 2));
  return(s7_make_boolean(sc, s7i_scheme_strcmp(str1, str2) == 1));
}

/* boolean unchecked/checked variants for optimizer */
bool string_lt_b_unchecked(s7_pointer s1, s7_pointer s2) {return(s7i_scheme_strcmp(s1, s2) == -1);}
bool string_leq_b_unchecked(s7_pointer s1, s7_pointer s2) {return(s7i_scheme_strcmp(s1, s2) != 1);}
bool string_gt_b_unchecked(s7_pointer s1, s7_pointer s2) {return(s7i_scheme_strcmp(s1, s2) == 1);}
bool string_geq_b_unchecked(s7_pointer s1, s7_pointer s2) {return(s7i_scheme_strcmp(s1, s2) != -1);}
bool string_eq_b_unchecked(s7_pointer s1, s7_pointer s2) {return(s7i_scheme_strings_are_equal(s1, s2));}

static void check_string2_args(s7_scheme *sc, s7_pointer caller, s7_pointer str1, s7_pointer str2)
{
  s7_pointer tn = s7i_string_type_name(sc);
  if (!s7_is_string(str1))
    { s7i_method_or_bust_sym(sc, str1, caller, s7i_set_plist_2(sc, str1, str2), tn, 1); }
  if (!s7_is_string(str2))
    { s7i_method_or_bust_sym(sc, str2, caller, s7i_set_plist_2(sc, str1, str2), tn, 2); }
}

bool string_lt_b_7pp(s7_scheme *sc, s7_pointer s1, s7_pointer s2)
{
  check_string2_args(sc, s7i_string_lt_symbol(sc), s1, s2);
  return(s7i_scheme_strcmp(s1, s2) == -1);
}

bool string_leq_b_7pp(s7_scheme *sc, s7_pointer s1, s7_pointer s2)
{
  check_string2_args(sc, s7i_string_leq_symbol(sc), s1, s2);
  return(s7i_scheme_strcmp(s1, s2) != 1);
}

bool string_gt_b_7pp(s7_scheme *sc, s7_pointer s1, s7_pointer s2)
{
  check_string2_args(sc, s7i_string_gt_symbol(sc), s1, s2);
  return(s7i_scheme_strcmp(s1, s2) == 1);
}

bool string_geq_b_7pp(s7_scheme *sc, s7_pointer s1, s7_pointer s2)
{
  check_string2_args(sc, s7i_string_geq_symbol(sc), s1, s2);
  return(s7i_scheme_strcmp(s1, s2) != -1);
}

bool string_eq_b_7pp(s7_scheme *sc, s7_pointer s1, s7_pointer s2)
{
  check_string2_args(sc, s7i_string_eq_symbol(sc), s1, s2);
  return(s7i_scheme_strings_are_equal(s1, s2));
}
