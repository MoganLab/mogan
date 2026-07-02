
/******************************************************************************
 * MODULE     : basic.cpp
 * DESCRIPTION: fast global new and delete
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "basic.hpp"
#include "analyze.hpp"
#include "string.hpp"

int
hash (pointer ptr) {
  return ((int) ((intptr_t) ptr)) + (((int) ((intptr_t) ptr)) % 19);
}

#ifdef USE_EXCEPTIONS
string the_exception;
string the_report;
// string get_crash_report (const char* msg);

void
tm_throw (const char* msg) {
  the_exception= msg;
  // the_report   = get_crash_report (msg);
  cout << "Throwing " << msg << LF;
  cout << "-------------------------------------------------\n";
  cout << the_report << LF;
  cout << "-------------------------------------------------\n";
  throw string (msg);
}

void
handle_exceptions () {
  if (N (the_exception) != 0) {
    // formatted arg (verbatim_to_tree (the_report, false, "utf-8"));
    // failed_error << "Exception, " << the_exception << arg << LF;
    the_exception= "";
    the_report   = "";
  }
}
#endif

/******************************************************************************
 * miscellaneous routines
 ******************************************************************************/

int
new_type_identifier () {
  static int id= 0;
  id--;
  return id;
}

static int current_indent= 0;

tm_ostream&
operator<< (tm_ostream& out, display_control ctrl) {
  int i;
  switch (ctrl) {
  case INDENT:
    out << "  ";
    current_indent+= 2;
    break;
  case UNINDENT:
    out << "\b\b";
    current_indent-= 2;
    break;
  case HRULE:
    for (i= current_indent; i < 78; i++)
      out << "-";
  case LF:
    out << "\n";
    for (i= 0; i < current_indent; i++)
      out << " ";
    break;
  }
  return out;
}
