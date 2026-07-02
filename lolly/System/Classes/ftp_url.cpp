
/******************************************************************************
 * MODULE     : ftp_url.cpp
 * DESCRIPTION: url rooted with ftp://
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "url.hpp"

ftp_url::ftp_url (string name) : url (url_root ("ftp") * url_get_name (name)) {}

ftp_url::ftp_url (const char* name)
    : url (url_root ("ftp") * url_get_name (string (name))) {}
