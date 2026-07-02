
/******************************************************************************
 * MODULE     : blank_url.cpp
 * DESCRIPTION: url rooted with //
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "url.hpp"

blank_url::blank_url (string name)
    : url (url_root ("blank") * url_get_name (name)) {}

blank_url::blank_url (const char* name)
    : url (url_root ("blank") * url_get_name (string (name))) {}

bool
is_root_blank (url u) {
  return is_root (u, "blank");
}

bool
is_rooted_blank (url u) {
  return is_root_blank (u) || (is_concat (u) && is_rooted_blank (u[1])) ||
         (is_or (u) && is_rooted_blank (u[1]) && is_rooted_blank (u[2]));
}
