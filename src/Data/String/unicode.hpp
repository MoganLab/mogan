
/******************************************************************************
 * MODULE     : unicode.hpp
 * DESCRIPTION: Unicode related routines
 * COPYRIGHT  : (C) 2013  Joris van der Hoeven
 *                  2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef UNICODE_H
#define UNICODE_H

#include "lolly/data/unicode.hpp"
#include "string.hpp"

string get_unicode_range (string c);
int    get_utf8_code_cached (string c);
bool   is_emoji_character (int uc);

#endif
