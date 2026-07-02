#include "a_lolly_test.hpp"
#include "string.hpp"

TEST_CASE ("equality of string") {
  CHECK_EQ (string ("abc") == "abc", true);
  CHECK_EQ (string ("abc") == "", false);

  CHECK_EQ (string ("abc") != "abc", false);
  CHECK_EQ (string ("abc") != "", true);

  CHECK_EQ (string ("abc") == string ("abc"), true);
  CHECK_EQ (string ("abc") == string (), false);
  CHECK_EQ (string ("abc") != string ("abc"), false);
  CHECK_EQ (string ("abc") != string (), true);

  CHECK_EQ (string () == string (), true);
}

TEST_CASE ("compare string") {
  CHECK (string ("ab") < string ("b"));
  CHECK (string () < string ("0"));
  CHECK (string ("a") <= string ("a"));
  CHECK (string ("ab") <= string ("b"));
  CHECK (string () <= string ());
  CHECK (string () <= string ("0"));
}

TEST_CASE ("test slice") {
  CHECK_EQ (string ("abcde") (0, 0) == string (), true);
  CHECK_EQ (string ("abcde") (0, 1) == string ("a"), true);
  CHECK_EQ (string ("abcde") (0, 10) == string ("abcde"), true);
  CHECK_EQ (string ("abcde") (-1, 1) == string ("a"), true);
  CHECK_EQ (string ("abcde") (3, 2) == string (), true);
  CHECK_EQ (string ("abcde") (3, -2) == string (), true);
  CHECK_EQ (string ("abcde") (10, 11) == string (), true);
  CHECK_EQ (string ("abcde") (-3, -2) == string (), true);
}

TEST_CASE ("test concat") {
  CHECK_EQ (string ("abc") * "de" == string ("abcde"), true);
  CHECK_EQ (string ("abc") * string ("de") == string ("abcde"), true);
  CHECK_EQ ("abc" * string ("de") == string ("abcde"), true);
}

TEST_CASE ("test iteration") {
  string str1         = "s\0tr1";
  int    expectedIndex= 0;
  for (const auto element : str1) {
    CHECK_EQ (element, str1[expectedIndex]);
    ++expectedIndex;
  }
  CHECK_EQ (expectedIndex, N (str1));
}

/******************************************************************************
 * Modifications
 ******************************************************************************/

TEST_CASE ("test append") {
  auto str= string ();
  str << 'x';
  CHECK_EQ (str == string ("x"), true);
  str << string ("yz");
  CHECK_EQ (str == string ("xyz"), true);
}

/******************************************************************************
 * Conversions
 ******************************************************************************/

TEST_CASE ("test as bool") {
  CHECK_EQ (as_bool (string ("true")), true);
  CHECK_EQ (as_bool (string ("#t")), true);
  CHECK_EQ (as_bool (string ("false")), false);

  CHECK_EQ (as_bool (string ("true")), true);
  CHECK_EQ (as_bool (string ("#t")), true);
  CHECK_EQ (!as_bool (string ("false")), true);
  // implicit conversion from char*
}

TEST_CASE ("as_int") {
  SUBCASE ("normal conversion") {
    CHECK_EQ (as_int (string ("1")), 1);
    CHECK_EQ (as_int (string ("-1")), -1);
    CHECK_EQ (as_int (string ("0")), 0);
    CHECK_EQ (as_int (string ("+1")), 1);
  }

  // assuming int is 32bit
  SUBCASE ("min and max") {
    CHECK_EQ (as_int (string ("-2147483648")), -2147483648);
    CHECK_EQ (as_int (string ("2147483647")), 2147483647);
  }

  SUBCASE ("int overflow") {
    CHECK_EQ (as_int (string ("-2147483649")), 2147483647);
    CHECK_EQ (as_int (string ("2147483648")), -2147483648);
  }

  SUBCASE ("invalid int") {
    CHECK_EQ (as_int (string ("not a int")), 0);
    CHECK_EQ (as_int (string ("++1")), 0);
    CHECK_EQ (as_int (string ("1pt")), 1);
    CHECK_EQ (as_int (string ("1.23")), 1);
  }
}

TEST_CASE ("is_int") {
  SUBCASE ("normal conversion") {
    CHECK (is_int (string ("1")));
    CHECK (is_int (string ("-1")));
    CHECK (is_int (string ("0")));
    CHECK (is_int (string ("+1")));
  }

  // assuming int is 32bit
  SUBCASE ("min and max") {
    CHECK (is_int (string ("-2147483648")));
    CHECK (is_int (string ("2147483647")));
  }

  SUBCASE ("int overflow") {
    CHECK (is_int (string ("-2147483649")));
    CHECK (is_int (string ("2147483648")));
  }

  SUBCASE ("invalid int") {
    CHECK_FALSE (is_int (string ("not a int")));
    CHECK_FALSE (is_int (string ("++1")));
    CHECK_FALSE (is_int (string ("1pt")));
    CHECK_FALSE (is_int (string ("1.23")));
  }
}

TEST_CASE ("test as strig bool") {
  CHECK_EQ (as_string_bool (true) == string ("true"), true);
  CHECK_EQ (as_string_bool (false) == string ("false"), true);
}

/******************************************************************************
 * Predicates
 ******************************************************************************/

TEST_CASE ("test is empty") {
  CHECK_EQ (is_empty (""), true);
  CHECK_EQ (!is_empty (" "), true);
  CHECK_EQ (!is_empty ("nonempty"), true);
}

TEST_CASE ("test is bool") {
  CHECK_EQ (is_bool (string ("true")), true);
  CHECK_EQ (is_bool (string ("false")), true);
  CHECK_EQ (is_bool (string ("true")), true);
  CHECK_EQ (is_bool (string ("false")), true);

  CHECK_EQ (!is_bool (string ("100")), true);
  CHECK_EQ (!is_bool (string ("nil")), true);
}

TEST_CASE ("test is int") {
  // Empty string is not an int
  CHECK_EQ (!is_int (string ("")), true);

  // Only 0-9 in chars are int
  for (auto i= 0; i < 256; i++) {
    char iter= (char) i;
    if (iter >= '0' && iter <= '9') CHECK_EQ (is_int (string (iter)), true);
    else CHECK_EQ (!is_int (string (iter)), true);
  }

  // Random tests
  CHECK_EQ (is_int (string ("-100")), true);
  CHECK_EQ (is_int (string ("+100")), true);
  CHECK_EQ (is_int (string ("100")), true);
  CHECK_EQ (!is_int (string (".0")), true);
  CHECK_EQ (!is_int (string ("0x09")), true);
}

TEST_CASE ("test is quoted") {
  CHECK_EQ (is_quoted ("\"\""), true);
  CHECK_EQ (is_quoted ("\"Hello TeXmacs\""), true);
  CHECK_EQ (is_quoted ("\"Hello\"TeXmacs\""), true);

  CHECK_EQ (!is_quoted ("\""), true);
  CHECK_EQ (!is_quoted ("A"), true);
  CHECK_EQ (!is_quoted ("9"), true);
  CHECK_EQ (!is_quoted ("Hello TeXmacs"), true);
  CHECK_EQ (!is_quoted ("\"Hello TeXmac\"s"), true);
  CHECK_EQ (!is_quoted ("H\"ello TeXmacs\""), true);
  // is_quoted only checks if a string starts with a double quote
  // and ends with another double quote, regardless the validity
  // of the raw string
}
