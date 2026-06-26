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

TEST_CASE ("tm_search_forwards") {
  // empty pattern: returns the starting position directly
  CHECK_EQ (tm_search_forwards ("", 0, "abc"), 0);
  CHECK_EQ (tm_search_forwards ("", 2, "abc"), 2);
  // no match: returns -1
  CHECK_EQ (tm_search_forwards ("xyz", 0, "abc"), -1);
  // simple substring match
  CHECK_EQ (tm_search_forwards ("b", 0, "abc"), 1);
  CHECK_EQ (tm_search_forwards ("bc", 0, "abc"), 1);
  // pos past the only match: returns -1
  CHECK_EQ (tm_search_forwards ("bc", 2, "abc"), -1);
  // pattern not fitting in the remaining tail
  CHECK_EQ (tm_search_forwards ("abcd", 0, "abc"), -1);
  // multi-byte Cork sequence counts as one logical char
  CHECK_EQ (tm_search_forwards ("<less>", 0, "a<less>b"), 1);
  CHECK_EQ (tm_search_forwards ("<#ABCD>", 0, "x<#ABCD>y"), 1);
  // returns byte offset into the source string
  CHECK_EQ (tm_search_forwards ("b", 0, "a<less>b"), 7);
}
