
/******************************************************************************
 * MODULE     : hash_utils.cpp
 * DESCRIPTION: Binary-safe hashing utilities (MD5 etc.)
 * COPYRIGHT  : (C) 2026  Mogan STEM authors
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "hash_utils.hpp"

#include "lolly/data/numeral.hpp"
#include <tbox/tbox.h>

string
md5_binary (string s) {
  tb_byte_t o_buffer[16];
  tb_size_t o_size=
      tb_md5_make ((tb_byte_t*) as_charp (s), N (s), o_buffer, 16);
  if (o_size != 16) {
    return string ("");
  }

  string md5_hex;
  for (int i= 0; i < 16; ++i) {
    md5_hex << lolly::data::to_padded_hex (o_buffer[i]);
  }
  return md5_hex;
}
