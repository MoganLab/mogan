

/******************************************************************************
 * MODULE     : tm_locale.cpp
 * DESCRIPTION: Locale related routines
 * COPYRIGHT  : (C) 1999-2019  Joris van der Hoeven, Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tm_locale.hpp"
#include "locale.hpp"

#ifdef QTTEXMACS
#include "Qt/qt_utilities.hpp"
#endif

/******************************************************************************
 * Getting a formatted date
 ******************************************************************************/

string
get_date (string lan, string fm, int year, int month, int day) {
  return lolly::locale::get_date (lan, fm, year, month, day);
}

#ifdef QTTEXMACS
string
pretty_time (int t) {
  return qt_pretty_time (t);
}
#endif
