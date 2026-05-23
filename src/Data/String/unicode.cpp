
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

#include "unicode.hpp"
#include "converter.hpp"

#include <lolly/data/unicode.hpp>

static hashmap<string, string> unicode_range_cache ("");

string
get_unicode_range (string c) {
  if (unicode_range_cache->contains (c)) return unicode_range_cache[c];
  string uc= strict_cork_to_utf8 (c);
  if (N (uc) == 0) {
    unicode_range_cache (c)= "";
    return "";
  }
  int      pos  = 0;
  uint32_t code = lolly::data::decode_from_utf8 (uc, pos);
  string   range= lolly::data::unicode_get_range (code);
  if (pos != N (uc)) range= "";
  unicode_range_cache (c)= range;
  return range;
}

static hashmap<string, int> utf8_code_cache (-1);

int
get_utf8_code_cached (string c) {
  if (utf8_code_cache->contains (c)) return utf8_code_cache[c];
  int c_N= N (c);
  if (c_N <= 2 || c_N > 6) {
    utf8_code_cache (c)= -1;
    return -1;
  }
  string uc  = strict_cork_to_utf8 (c);
  int    pos = 0;
  int    code= lolly::data::decode_from_utf8 (uc, pos);
  if (pos == c_N) {
    utf8_code_cache (c)= code;
    return code;
  }
  utf8_code_cache (c)= -1;
  return -1;
}

bool
is_emoji_character (int uc) {
  return lolly::data::unicode_get_range (uc) == "emoji";
}
