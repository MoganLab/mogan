/******************************************************************************
 * MODULE     : cork_test.cpp
 * DESCRIPTION: tests for the Cork encoding
 * COPYRIGHT  : (C) 2024    Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "cork.hpp"

TEST_CASE ("tm_encode") {
  string_eq (tm_encode ("<>"), "<less><gtr>");
  string_eq (tm_encode ("<#ABCD>"), "<less>#ABCD<gtr>");
  string_eq (tm_encode ("abc"), "abc");
  string_eq (tm_encode (""), "");
}

TEST_CASE ("tm_var_encode") {
  string_eq (tm_var_encode ("<>"), "<less><gtr>");
  string_eq (tm_var_encode ("<#ABCD>"), "<#ABCD>");
  string_eq (tm_encode ("abc"), "abc");
  string_eq (tm_encode (""), "");
}

TEST_CASE ("tm_decode") {
  string_eq (tm_decode ("<less><gtr>"), "<>");
  string_eq (tm_decode ("<#ABCD>"), "<#ABCD>");
  string_eq (tm_decode ("abc"), "abc");
  string_eq (tm_decode (""), "");
}

TEST_CASE ("tm_length") {
  string_eq (tm_string_length (""), 0);
  string_eq (tm_string_length ("<#ABCD>"), 1);
  string_eq (tm_string_length ("<less>1"), 2);
}
