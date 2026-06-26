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
  string_eq (tm_encode (""), "");
  string_eq (tm_encode ("abc"), "abc");
  string_eq (tm_encode ("<>"), "<less><gtr>");
  string_eq (tm_encode ("<#ABCD>"), "<less>#ABCD<gtr>");
  string_eq (tm_encode ("a<b>c"), "a<less>b<gtr>c");
}

TEST_CASE ("tm_var_encode") {
  string_eq (tm_var_encode (""), "");
  string_eq (tm_var_encode ("abc"), "abc");
  string_eq (tm_var_encode ("<>"), "<less><gtr>");
  string_eq (tm_var_encode ("<#ABCD>"), "<#ABCD>");
  string_eq (tm_var_encode ("<#AB> <#CD>"), "<#AB> <#CD>");
  string_eq (tm_var_encode ("a<b>c"), "a<less>b<gtr>c");
}

TEST_CASE ("tm_decode") {
  string_eq (tm_decode (""), "");
  string_eq (tm_decode ("abc"), "abc");
  string_eq (tm_decode ("<less><gtr>"), "<>");
  string_eq (tm_decode ("<#ABCD>"), "<#ABCD>");
  string_eq (tm_decode ("a<less>b<gtr>c"), "a<b>c");
}

TEST_CASE ("tm_correct") {
  string_eq (tm_correct (""), "");
  string_eq (tm_correct ("abc"), "abc");
  string_eq (tm_correct ("<less>"), "<less>");
  string_eq (tm_correct ("<less"), "");
  string_eq (tm_correct ("a<bc>d"), "a<bc>d");
  string_eq (tm_correct ("a<<b>>c"), "ac");
  string_eq (tm_correct (">a<"), "a");
}

TEST_CASE ("tm_char_forwards") {
  string s    = "a<#ABCD>bc";
  int    pos  = 0;
  int    prev = pos;
  tm_char_forwards (s, pos);
  string_eq (s (prev, pos), "a");
  prev= pos;
  tm_char_forwards (s, pos);
  string_eq (s (prev, pos), "<#ABCD>");
  prev= pos;
  tm_char_forwards (s, pos);
  string_eq (s (prev, pos), "b");
  prev= pos;
  tm_char_forwards (s, pos);
  string_eq (s (prev, pos), "c");
  CHECK_EQ (pos, N (s));
}

TEST_CASE ("tm_char_backwards") {
  string s    = "a<#ABCD>bc";
  int    pos  = N (s);
  int    prev = pos;
  tm_char_backwards (s, pos);
  string_eq (s (pos, prev), "c");
  prev= pos;
  tm_char_backwards (s, pos);
  string_eq (s (pos, prev), "b");
  prev= pos;
  tm_char_backwards (s, pos);
  string_eq (s (pos, prev), "<#ABCD>");
  prev= pos;
  tm_char_backwards (s, pos);
  string_eq (s (pos, prev), "a");
  CHECK_EQ (pos, 0);
}

TEST_CASE ("tm_char_next and tm_char_previous") {
  string s= "a<#ABCD>bc";
  CHECK_EQ (tm_char_next (s, 0), 1);
  CHECK_EQ (tm_char_next (s, 1), 8);
  CHECK_EQ (tm_char_next (s, 8), 9);
  CHECK_EQ (tm_char_next (s, 9), 10);
  CHECK_EQ (tm_char_previous (s, 10), 9);
  CHECK_EQ (tm_char_previous (s, 9), 8);
  CHECK_EQ (tm_char_previous (s, 8), 1);
  CHECK_EQ (tm_char_previous (s, 1), 0);
}

TEST_CASE ("tm_forward_access") {
  string s= "a<#ABCD>bc";
  string_eq (tm_forward_access (s, 0), "a");
  string_eq (tm_forward_access (s, 1), "<#ABCD>");
  string_eq (tm_forward_access (s, 2), "b");
  string_eq (tm_forward_access (s, 3), "c");
}

TEST_CASE ("tm_backward_access") {
  string s= "a<#ABCD>bc";
  string_eq (tm_backward_access (s, 0), "c");
  string_eq (tm_backward_access (s, 1), "b");
  string_eq (tm_backward_access (s, 2), "<#ABCD>");
  string_eq (tm_backward_access (s, 3), "a");
}

TEST_CASE ("tm_string_length") {
  CHECK_EQ (tm_string_length (""), 0);
  CHECK_EQ (tm_string_length ("abc"), 3);
  CHECK_EQ (tm_string_length ("<#ABCD>"), 1);
  CHECK_EQ (tm_string_length ("<less>1"), 2);
  CHECK_EQ (tm_string_length ("a<#ABCD>bc"), 4);
}

TEST_CASE ("tm_tokenize") {
  array<string> tokens= tm_tokenize ("a<#ABCD>bc");
  CHECK_EQ (N (tokens), 4);
  string_eq (tokens[0], "a");
  string_eq (tokens[1], "<#ABCD>");
  string_eq (tokens[2], "b");
  string_eq (tokens[3], "c");
  string_eq (tm_recompose (tokens), "a<#ABCD>bc");
}

TEST_CASE ("tm_recompose") {
  array<string> a;
  a << string ("a") << string ("<#ABCD>") << string ("b") << string ("c");
  string_eq (tm_recompose (a), "a<#ABCD>bc");
  string_eq (tm_recompose (array<string> ()), "");
}

TEST_CASE ("tm_search_forwards") {
  string s= "ab<#ABCD>cdab";
  CHECK_EQ (tm_search_forwards ("ab", 0, s), 0);
  CHECK_EQ (tm_search_forwards ("ab", 1, s), 11);
  CHECK_EQ (tm_search_forwards ("<#ABCD>", 0, s), 2);
  CHECK_EQ (tm_search_forwards ("<#ABCD>", 3, s), -1);
  CHECK_EQ (tm_search_forwards ("xyz", 0, s), -1);
  CHECK_EQ (tm_search_forwards ("", 0, s), 0);
}

TEST_CASE ("tm_search_backwards") {
  string s= "ab<#ABCD>cdab";
  CHECK_EQ (tm_search_backwards ("ab", 12, s), 11);
  CHECK_EQ (tm_search_backwards ("ab", 9, s), 0);
  CHECK_EQ (tm_search_backwards ("<#ABCD>", 12, s), 2);
  CHECK_EQ (tm_search_backwards ("xyz", 12, s), -1);
}

TEST_CASE ("tm_string_split") {
  array<string> r1= tm_string_split ("hello world");
  CHECK_EQ (N (r1), 2);
  string_eq (r1[0], "hello");
  string_eq (r1[1], "world");

  array<string> r2= tm_string_split ("hello world!");
  CHECK_EQ (N (r2), 3);
  string_eq (r2[0], "hello");
  string_eq (r2[1], " ");
  string_eq (r2[2], "world!");

  array<string> r3= tm_string_split ("abc123def");
  CHECK_EQ (N (r3) >= 2, true);
}

TEST_CASE ("contains_unicode_char") {
  CHECK_EQ (contains_unicode_char (""), false);
  CHECK_EQ (contains_unicode_char ("abc"), false);
  CHECK_EQ (contains_unicode_char ("<less><gtr>"), false);
  CHECK_EQ (contains_unicode_char ("<#ABCD>"), true);
  CHECK_EQ (contains_unicode_char ("a<#ABCD>b"), true);
}

TEST_CASE ("downgrade_math_letters") {
  string_eq (downgrade_math_letters ("abc"), "abc");
  string_eq (downgrade_math_letters ("<b-a>"), "a");
  string_eq (downgrade_math_letters ("<up-A>"), "A");
  string_eq (downgrade_math_letters ("<cal-X>"), "X");
  string_eq (downgrade_math_letters ("<bbb-Y>"), "Y");
  string_eq (downgrade_math_letters ("<frak-Z>"), "Z");
  string_eq (downgrade_math_letters ("a<b-b><cal-C>"), "abc");
}
