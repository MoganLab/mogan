/* s7_ctables.c - character classification tables for s7 Scheme interpreter
 *
 * derived from s7, a Scheme interpreter
 * SPDX-License-Identifier: 0BSD
 */

#include "s7_ctables.h"

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/* Configuration macros - must match s7.c */
#ifndef WITH_PURE_S7
  #define WITH_PURE_S7 0
#endif
#if WITH_PURE_S7
  #define WITH_EXTRA_EXPONENT_MARKERS 0
#endif

#ifndef WITH_R7RS
  #define WITH_R7RS !WITH_PURE_S7
#endif

#ifndef WITH_EXTRA_EXPONENT_MARKERS
  #define WITH_EXTRA_EXPONENT_MARKERS 0
#endif

/* Global character classification tables */
bool *exponent_table;
bool *slashify_table;
bool *char_ok_in_a_name;
bool *white_space;
bool *number_table;
bool *symbol_slashify_table;
int32_t *digits;

void init_ctables(void)
{
  exponent_table = (bool *)calloc(S7_CTABLE_SIZE, sizeof(bool));
  slashify_table = (bool *)calloc(S7_CTABLE_SIZE, sizeof(bool));
  symbol_slashify_table = (bool *)calloc(S7_CTABLE_SIZE, sizeof(bool));
  char_ok_in_a_name = (bool *)malloc(S7_CTABLE_SIZE * sizeof(bool));
  white_space = (bool *)calloc(S7_CTABLE_SIZE + 1, sizeof(bool));
  white_space++;      /* leave white_space[-1] false for white_space[EOF] */
  number_table = (bool *)calloc(S7_CTABLE_SIZE, sizeof(bool));
  digits = (int32_t *)malloc(S7_CTABLE_SIZE * sizeof(int32_t));

  for (int32_t i = 0; i < S7_CTABLE_SIZE; i++)
    {
      char_ok_in_a_name[i] = true;
      /* white_space[i] = false; */
      digits[i] = 256;
      /* number_table[i] = false; */
    }

  char_ok_in_a_name[0] = false;
  char_ok_in_a_name[(uint8_t)'('] = false;  /* cast for C++ */
  char_ok_in_a_name[(uint8_t)')'] = false;
  char_ok_in_a_name[(uint8_t)';'] = false;
  char_ok_in_a_name[(uint8_t)'\t'] = false;
  char_ok_in_a_name[(uint8_t)'\n'] = false;
  char_ok_in_a_name[(uint8_t)'\r'] = false;
  char_ok_in_a_name[(uint8_t)' '] = false;
  char_ok_in_a_name[(uint8_t)'"'] = false;

  white_space[(uint8_t)'\t'] = true;
  white_space[(uint8_t)'\n'] = true;
  white_space[(uint8_t)'\r'] = true;
  white_space[(uint8_t)'\f'] = true;
  white_space[(uint8_t)'\v'] = true;
  white_space[(uint8_t)' '] = true;
  white_space[(uint8_t)'\205'] = true; /* 133 */
  white_space[(uint8_t)'\240'] = true; /* 160 */

  /* surely only 'e' is needed... */
  exponent_table[(uint8_t)'e'] = true; exponent_table[(uint8_t)'E'] = true;
  exponent_table[(uint8_t)'@'] = true;
#if WITH_EXTRA_EXPONENT_MARKERS
  exponent_table[(uint8_t)'s'] = true; exponent_table[(uint8_t)'S'] = true;
  exponent_table[(uint8_t)'f'] = true; exponent_table[(uint8_t)'F'] = true;
  exponent_table[(uint8_t)'d'] = true; exponent_table[(uint8_t)'D'] = true;
  exponent_table[(uint8_t)'l'] = true; exponent_table[(uint8_t)'L'] = true;
#endif
  for (int32_t i = 0; i < 32; i++) slashify_table[i] = true;
  /* for (int32_t i = 127; i < 160; i++) slashify_table[i] = true; */ /* 6-Apr-24 for utf-8, but this has no effect on s7test?? */
  slashify_table[(uint8_t)'\\'] = true;
  slashify_table[(uint8_t)'"'] = true;
#if WITH_R7RS
  /* In R7RS mode, newlines should be escaped to ensure proper serialization */
  slashify_table[(uint8_t)'\n'] = true;
#else
   slashify_table[(uint8_t)'\n'] = false;
#endif

  for (int32_t i = 0; i < S7_CTABLE_SIZE; i++)
    symbol_slashify_table[i] = ((slashify_table[i]) || (!char_ok_in_a_name[i])); /* force use of (symbol ...) for cases like '(ab) as symbol */

  digits[(uint8_t)'0'] = 0; digits[(uint8_t)'1'] = 1; digits[(uint8_t)'2'] = 2; digits[(uint8_t)'3'] = 3; digits[(uint8_t)'4'] = 4;
  digits[(uint8_t)'5'] = 5; digits[(uint8_t)'6'] = 6; digits[(uint8_t)'7'] = 7; digits[(uint8_t)'8'] = 8; digits[(uint8_t)'9'] = 9;
  digits[(uint8_t)'a'] = 10; digits[(uint8_t)'A'] = 10;
  digits[(uint8_t)'b'] = 11; digits[(uint8_t)'B'] = 11;
  digits[(uint8_t)'c'] = 12; digits[(uint8_t)'C'] = 12;
  digits[(uint8_t)'d'] = 13; digits[(uint8_t)'D'] = 13;
  digits[(uint8_t)'e'] = 14; digits[(uint8_t)'E'] = 14;
  digits[(uint8_t)'f'] = 15; digits[(uint8_t)'F'] = 15;

  number_table[(uint8_t)'0'] = true; number_table[(uint8_t)'1'] = true; number_table[(uint8_t)'2'] = true; number_table[(uint8_t)'3'] = true;
  number_table[(uint8_t)'4'] = true; number_table[(uint8_t)'5'] = true; number_table[(uint8_t)'6'] = true; number_table[(uint8_t)'7'] = true;
  number_table[(uint8_t)'8'] = true; number_table[(uint8_t)'9'] = true; number_table[(uint8_t)'.'] = true;
  number_table[(uint8_t)'+'] = true;
  number_table[(uint8_t)'-'] = true;
  number_table[(uint8_t)'#'] = true;
}
