/* s7_scheme_char.c - character implementations for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 *
 * Bill Schottstaedt, bil@ccrma.stanford.edu
 */

#include "s7_scheme_char.h"
#include "s7_internal_helpers.h"
#include <ctype.h>
#include <string.h>
#include <wctype.h>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

extern uint8_t     uppers[256];
extern uint8_t     lowers[256];
extern s7_pointer* chars;

#define S7_NUM_CHARS 256

void
init_scheme_char_tables (void) {
}

static s7_pointer
list1 (s7_scheme* sc, s7_pointer a) {
  return s7_cons (sc, a, s7_nil (sc));
}

static s7_pointer
list2 (s7_scheme* sc, s7_pointer a, s7_pointer b) {
  return s7_cons (sc, a, s7_cons (sc, b, s7_nil (sc)));
}

static bool
is_character_via_method (s7_scheme* sc, s7_pointer p) {
  if (s7_is_character (p)) return true;
  {
    s7_pointer sym = s7_make_symbol (sc, "char?");
    s7_pointer func= s7_method (sc, p, sym);
    if (func == s7_undefined (sc)) return false;
    return (s7_apply_function (sc, func, list1 (sc, p)) != s7_f (sc));
  }
}

static s7_pointer
char_with_error_check (s7_scheme* sc, s7_pointer args_left, s7_int pos, const char* caller) {
  s7_int arg_pos= pos + 1;
  for (s7_pointer chrs= s7_cdr (args_left); s7_is_pair (chrs); chrs= s7_cdr (chrs), arg_pos++) {
    s7_pointer cur= s7_car (chrs);
    if (!is_character_via_method (sc, cur)) return s7_wrong_type_arg_error (sc, caller, arg_pos, cur, "a character");
  }
  return s7_f (sc);
}

static int32_t
charcmp (uint32_t c1, uint32_t c2) {
  return (c1 == c2) ? 0 : (c1 < c2) ? -1 : 1;
}

/* -------------------------------- char<->integer -------------------------------- */

s7_pointer
g_char_to_integer (s7_scheme* sc, s7_pointer args) {
  s7_pointer arg= s7_car (args);
  if (!s7_is_character (arg)) return s7i_method_or_bust (sc, arg, "char->integer", args, "a character", 1);
  return s7_make_integer (sc, (s7_int) s7_character (arg));
}

s7_int
char_to_integer_i_7p (s7_scheme* sc, s7_pointer c) {
  if (!s7_is_character (c)) {
    s7_pointer result= s7i_method_or_bust (sc, c, "char->integer", list1 (sc, c), "a character", 1);
    return s7_integer (result);
  }
  return (s7_int) s7_character (c);
}

s7_pointer
char_to_integer_p_p (s7_scheme* sc, s7_pointer c) {
  if (!s7_is_character (c)) return s7i_method_or_bust (sc, c, "char->integer", list1 (sc, c), "a character", 1);
  return s7_make_integer (sc, (s7_int) s7_character (c));
}

s7_pointer
integer_to_char_p_p (s7_scheme* sc, s7_pointer x) {
  s7_int ind;
  if (!s7_is_integer (x)) return s7i_method_or_bust (sc, x, "integer->char", list1 (sc, x), "an integer", 1);

  ind= s7_number_to_integer_with_caller (sc, x, "integer->char");

  if ((ind < 0) || (ind > 0x10FFFF))
    return s7_out_of_range_error (sc, "integer->char", 1, x, "it doesn't fit in an unsigned byte");
  return s7_make_character (sc, (uint32_t) ind);
}

s7_pointer
g_integer_to_char (s7_scheme* sc, s7_pointer args) {
  return integer_to_char_p_p (sc, s7_car (args));
}

s7_pointer
integer_to_char_p_i (s7_scheme* sc, s7_int ind) {
  if ((ind < 0) || (ind > 0x10FFFF))
    return s7_out_of_range_error (sc, "integer->char", 1, s7_make_integer (sc, ind),
                                  "it doesn't fit in an unsigned byte");
  return s7_make_character (sc, (uint32_t) ind);
}

/* -------------------------------- char? -------------------------------- */

s7_pointer
g_is_char (s7_scheme* sc, s7_pointer args) {
  s7_pointer obj= s7_car (args);
  if (s7_is_character (obj)) return s7_t (sc);
  {
    s7_pointer sym = s7_make_symbol (sc, "char?");
    s7_pointer func= s7_method (sc, obj, sym);
    if (func == s7_undefined (sc)) return s7_f (sc);
    return s7_apply_function (sc, func, list1 (sc, obj));
  }
}

s7_pointer
is_char_p_p (s7_scheme* sc, s7_pointer p) {
  return s7_is_character (p) ? s7_t (sc) : s7_f (sc);
}

/* -------------------------------- char<? char<=? char>? char>=? char=? -------------------------------- */

static s7_pointer
g_char_cmp (s7_scheme* sc, s7_pointer args, int32_t val, const char* name) {
  s7_pointer chr= s7_car (args);
  if (!s7_is_character (chr)) return s7i_method_or_bust (sc, chr, name, args, "a character", 1);

  s7_int pos= 2;
  for (s7_pointer chrs= s7_cdr (args); s7_is_pair (chrs); chrs= s7_cdr (chrs), pos++) {
    s7_pointer cur= s7_car (chrs);
    if (!s7_is_character (cur)) return s7i_method_or_bust (sc, cur, name, s7_cons (sc, chr, chrs), "a character", pos);
    if (charcmp (s7_character (chr), s7_character (cur)) != val) return char_with_error_check (sc, chrs, pos, name);
    chr= cur;
  }
  return s7_t (sc);
}

static s7_pointer
g_char_cmp_not (s7_scheme* sc, s7_pointer args, int32_t val, const char* name) {
  s7_pointer chr= s7_car (args);
  if (!s7_is_character (chr)) return s7i_method_or_bust (sc, chr, name, args, "a character", 1);

  s7_int pos= 2;
  for (s7_pointer chrs= s7_cdr (args); s7_is_pair (chrs); chrs= s7_cdr (chrs), pos++) {
    s7_pointer cur= s7_car (chrs);
    if (!s7_is_character (cur)) return s7i_method_or_bust (sc, cur, name, s7_cons (sc, chr, chrs), "a character", pos);
    if (charcmp (s7_character (chr), s7_character (cur)) == val) return char_with_error_check (sc, chrs, pos, name);
    chr= cur;
  }
  return s7_t (sc);
}

s7_pointer
g_chars_are_equal (s7_scheme* sc, s7_pointer args) {
  s7_pointer chr= s7_car (args);
  if (!s7_is_character (chr)) return s7i_method_or_bust (sc, chr, "char=?", args, "a character", 1);

  s7_int pos= 2;
  for (s7_pointer chrs= s7_cdr (args); s7_is_pair (chrs); chrs= s7_cdr (chrs), pos++) {
    s7_pointer cur= s7_car (chrs);
    if (!s7_is_character (cur))
      return s7i_method_or_bust (sc, cur, "char=?", s7_cons (sc, chr, chrs), "a character", pos);
    if (cur != chr) return char_with_error_check (sc, chrs, pos, "char=?");
  }
  return s7_t (sc);
}

s7_pointer
g_chars_are_less (s7_scheme* sc, s7_pointer args) {
  return g_char_cmp (sc, args, -1, "char<?");
}

s7_pointer
g_chars_are_greater (s7_scheme* sc, s7_pointer args) {
  return g_char_cmp (sc, args, 1, "char>?");
}

s7_pointer
g_chars_are_geq (s7_scheme* sc, s7_pointer args) {
  return g_char_cmp_not (sc, args, -1, "char>=?");
}

s7_pointer
g_chars_are_leq (s7_scheme* sc, s7_pointer args) {
  return g_char_cmp_not (sc, args, 1, "char<=?");
}

s7_pointer
g_simple_char_eq (s7_scheme* sc, s7_pointer args) {
  return s7_make_boolean (sc, s7_car (args) == s7_cadr (args));
}

s7_pointer
g_simple_char_eq1 (s7_scheme* sc, s7_pointer args) {
  s7_pointer c1= s7_car (args);
  s7_pointer c2= s7_cadr (args);
  if (!s7_is_character (c2)) return s7i_method_or_bust (sc, c2, "char=?", args, "a character", 2);
  return s7_make_boolean (sc, c1 == c2);
}

s7_pointer
g_simple_char_eq2 (s7_scheme* sc, s7_pointer args) {
  s7_pointer c1= s7_car (args);
  s7_pointer c2= s7_cadr (args);
  if (!s7_is_character (c1)) return s7i_method_or_bust (sc, c1, "char=?", args, "a character", 1);
  return s7_make_boolean (sc, c1 == c2);
}

bool
char_lt_b_unchecked (s7_pointer c1, s7_pointer c2) {
  return c1 < c2;
}

bool
char_lt_b_7pp (s7_scheme* sc, s7_pointer c1, s7_pointer c2) {
  if (!s7_is_character (c1)) return s7i_method_or_bust_bool (sc, c1, "char<?", list2 (sc, c1, c2), "a character", 1);
  if (!s7_is_character (c2)) return s7i_method_or_bust_bool (sc, c2, "char<?", list2 (sc, c1, c2), "a character", 2);
  return c1 < c2;
}

bool
char_leq_b_unchecked (s7_pointer c1, s7_pointer c2) {
  return c1 <= c2;
}

bool
char_leq_b_7pp (s7_scheme* sc, s7_pointer c1, s7_pointer c2) {
  if (!s7_is_character (c1)) return s7i_method_or_bust_bool (sc, c1, "char<=?", list2 (sc, c1, c2), "a character", 1);
  if (!s7_is_character (c2)) return s7i_method_or_bust_bool (sc, c2, "char<=?", list2 (sc, c1, c2), "a character", 2);
  return c1 <= c2;
}

bool
char_gt_b_unchecked (s7_pointer c1, s7_pointer c2) {
  return c1 > c2;
}

bool
char_gt_b_7pp (s7_scheme* sc, s7_pointer c1, s7_pointer c2) {
  if (!s7_is_character (c1)) return s7i_method_or_bust_bool (sc, c1, "char>?", list2 (sc, c1, c2), "a character", 1);
  if (!s7_is_character (c2)) return s7i_method_or_bust_bool (sc, c2, "char>?", list2 (sc, c1, c2), "a character", 2);
  return c1 > c2;
}

bool
char_geq_b_unchecked (s7_pointer c1, s7_pointer c2) {
  return c1 >= c2;
}

bool
char_geq_b_7pp (s7_scheme* sc, s7_pointer c1, s7_pointer c2) {
  if (!s7_is_character (c1)) return s7i_method_or_bust_bool (sc, c1, "char>=?", list2 (sc, c1, c2), "a character", 1);
  if (!s7_is_character (c2)) return s7i_method_or_bust_bool (sc, c2, "char>=?", list2 (sc, c1, c2), "a character", 2);
  return c1 >= c2;
}

bool
char_eq_b_unchecked (s7_pointer c1, s7_pointer c2) {
  return c1 == c2;
}

bool
char_eq_b_7pp (s7_scheme* sc, s7_pointer c1, s7_pointer c2) {
  if (!s7_is_character (c1)) return s7i_method_or_bust_bool (sc, c1, "char=?", list2 (sc, c1, c2), "a character", 1);
  if (c1 == c2) return true;
  if (!s7_is_character (c2)) return s7i_method_or_bust_bool (sc, c2, "char=?", list2 (sc, c1, c2), "a character", 2);
  return false;
}

s7_pointer
char_eq_p_pp (s7_scheme* sc, s7_pointer c1, s7_pointer c2) {
  if (!s7_is_character (c1)) return s7i_method_or_bust (sc, c1, "char=?", list2 (sc, c1, c2), "a character", 1);
  if (c1 == c2) return s7_t (sc);
  if (!s7_is_character (c2)) return s7i_method_or_bust (sc, c2, "char=?", list2 (sc, c1, c2), "a character", 2);
  return s7_f (sc);
}

s7_pointer
g_char_equal_2 (s7_scheme* sc, s7_pointer args) {
  s7_pointer c1= s7_car (args);
  s7_pointer c2= s7_cadr (args);
  if (!s7_is_character (c1)) return s7i_method_or_bust (sc, c1, "char=?", args, "a character", 1);
  if (c1 == c2) return s7_t (sc);
  if (!s7_is_character (c2)) return s7i_method_or_bust (sc, c2, "char=?", args, "a character", 2);
  return s7_f (sc);
}

s7_pointer
g_char_less_2 (s7_scheme* sc, s7_pointer args) {
  s7_pointer c1= s7_car (args);
  s7_pointer c2= s7_cadr (args);
  if (!s7_is_character (c1)) return s7i_method_or_bust (sc, c1, "char<?", args, "a character", 1);
  if (!s7_is_character (c2)) return s7i_method_or_bust (sc, c2, "char<?", args, "a character", 2);
  return s7_make_boolean (sc, s7_character (c1) < s7_character (c2));
}

s7_pointer
g_char_greater_2 (s7_scheme* sc, s7_pointer args) {
  s7_pointer c1= s7_car (args);
  s7_pointer c2= s7_cadr (args);
  if (!s7_is_character (c1)) return s7i_method_or_bust (sc, c1, "char>?", args, "a character", 1);
  if (!s7_is_character (c2)) return s7i_method_or_bust (sc, c2, "char>?", args, "a character", 2);
  return s7_make_boolean (sc, s7_character (c1) > s7_character (c2));
}
