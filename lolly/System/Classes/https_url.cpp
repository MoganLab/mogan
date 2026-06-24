
/******************************************************************************
 * MODULE     : https_url.cpp
 * DESCRIPTION: url rooted with https://
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <https://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "url.hpp"

https_url::https_url (string name)
    : url (url_root ("https") * url_get_name (name)) {}

https_url::https_url (const char* name)
    : url (url_root ("https") * url_get_name (string (name))) {}
