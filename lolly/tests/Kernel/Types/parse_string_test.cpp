#include "a_lolly_test.hpp"
#include "parse_string.hpp"

#include <cassert>
#include <iostream>

TEST_CASE ("test for test()") {
  parse_string s1, s2 ("abc"), s3 ("  ");
  parse_string s4 ("\t\b\0");
  CHECK_EQ (test (s1, ""), true);
  CHECK_EQ (test (s2, "ab"), true);
  CHECK_EQ (test (s3, " "), true);
  CHECK_EQ (test (s4, "\t\b"), true);
  CHECK_EQ (test (s3, ""), true);

  CHECK_EQ (!test (s3, "asdf"), true);
  CHECK_EQ (!test (s4, "1234"), true);
}

TEST_CASE ("test for operator[] and getchar()") {
  parse_string s1;
  parse_string s2 (" 123456789abcd~!@#$");
  string       str ('0', 300);
  str << "1";
  parse_string s3 (str);
  CHECK_EQ (s2[0] == ' ', true);
  CHECK_EQ (s2[9] == '9', true);
  CHECK_EQ (s2[14] == '~', true);
  CHECK_EQ (s3[300] == '1', true);
  CHECK_EQ (s2[-1] == '\0', true);

  CHECK_EQ (s1[2] != '2', true);
  CHECK_EQ (s2[18] != '#', true);
  CHECK_EQ (s2[0] != '0', true);
  CHECK_EQ (s2[100] != '$', true);
}

TEST_CASE ("test for get_string()") {
  parse_string s1 ("\t\b"), s2;
  s1->write ("!@#$");
  s1->write ("0123");
  s1->write ("");
  CHECK_EQ (s2->get_string (0) == "", true);
  CHECK_EQ (s1->get_string (0) == "", true);
  CHECK_EQ (s1->get_string (1) == "0", true);
  CHECK_EQ (s1->get_string (2) == "01", true);
  CHECK_EQ (s1->get_string (6) == "0123!@", true);
  CHECK_EQ (s1->get_string (9) == "0123!@#$\t", true);
  CHECK_EQ (s1->get_string (12) == "0123!@#$\t\b", true);

  CHECK_EQ (s2->get_string (0) != " ", true);
  CHECK_EQ (s1->get_string (0) != "0", true);
  CHECK_EQ (s1->get_string (1) != "", true);
  CHECK_EQ (s1->get_string (2) != "!@", true);
  CHECK_EQ (s1->get_string (15) != "", true);
}

TEST_CASE ("test for write() and read()") {
  parse_string s1 ("\t\b");
  s1->write ("!@#$");
  s1->write ("0123");
  s1->write ("");
  CHECK_EQ (s1[4] == '!', true);
  CHECK_EQ (s1[7] == '$', true);
  CHECK_EQ (test (s1, "0123!@#$\t\b"), true);

  CHECK_EQ (s1->read (3) == "012", true);
  CHECK_EQ (s1->read (1) == "3", true);
  CHECK_EQ (s1->read (5) == "!@#$\t", true);
  CHECK_EQ (s1->read (2) == "\b", true);
  CHECK_EQ (s1->read (2) == "", true);
}

TEST_CASE ("test for bool") {
  parse_string s1, s2 ("a)3"), s3 (" ");
  parse_string s4 ("\0"), s5 ("\t");
  CHECK_EQ (s2 == true, true);
  CHECK_EQ (s3 == true, true);
  CHECK_EQ (s4 == true, true);
  CHECK_EQ (s5 == true, true);

  CHECK_EQ (s1 == false, true);
}

TEST_CASE ("test for operator+= and advance()") {
  parse_string s1 ("\t\b");
  s1->write ("!@#$");
  s1->write ("0123");
  s1->write ("");
  s1+= 1;
  CHECK_EQ (s1[0] == '1', true);
  CHECK_EQ (s1[9] == '\0', true);
  s1+= 3;
  CHECK_EQ (s1[0] == '!', true);
  CHECK_EQ (s1[4] == '\t', true);
  s1+= 6;
  CHECK_EQ (s1[0] == '\0', true);
  CHECK_EQ (s1[-1] != '\b', true);
}