
/******************************************************************************
 * MODULE     : example_dynamic_library.cpp
 * DESCRIPTION: an example dynamic library for testing
 * COPYRIGHT  : (C) 2024  jingkaimori
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

extern "C" {
double
square_div_2 (int arg) {
  return arg * arg / 2.0;
};
}
