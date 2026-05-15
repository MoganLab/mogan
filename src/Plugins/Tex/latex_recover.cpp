
/******************************************************************************
 * MODULE     : latex_recover.cpp
 * DESCRIPTION: Error recovery for TeXmacs -> LaTeX exportation
 * COPYRIGHT  : (C) 2015  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Tex/convert_tex.hpp"
#include "Tex/tex.hpp"
#include "analyze.hpp"
#include "file.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"

/******************************************************************************
 * Getting information out of log files
 ******************************************************************************/

int
number_latex_pages (url log) {
  string s;
  if (load_string (log, s, false)) return -1;
  int pos= search_backwards ("Output written on ", s);
  if (pos < 0) return -1;
  pos= search_forwards (" pages, ", pos, s);
  if (pos < 0) return -1;
  int end= pos;
  while (pos > 0 && is_numeric (s[pos - 1]))
    pos--;
  return as_int (s (pos, end));
}
