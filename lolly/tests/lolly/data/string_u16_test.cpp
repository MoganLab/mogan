/** \file lolly_string_test.cpp
 *  \copyright GPLv3
 *  \details Test for string
 *  \author jingkaimori
 *  \date   2024
 */

#include "a_lolly_test.hpp"
#include "lolly/data/string_u16.hpp"
#include <doctest/doctest.h>

using namespace lolly::data;

TEST_CASE ("equality of string") {
  CHECK_EQ (string_u16 () == string_u16 (), true);

  CHECK_EQ (string_u16 (u"abc") == u"abc", true);
  CHECK_EQ (u"abc" == string_u16 (u"abc"), true);

  CHECK_EQ (string_u16 (u"abc") == u"", false);
  CHECK_EQ (u"" == string_u16 (u"abc"), false);

  CHECK_EQ (string_u16 (u"abc") != u"abc", false);
  CHECK_EQ (u"abc" != string_u16 (u"abc"), false);

  CHECK_EQ (string_u16 (u"abc") != u"", true);
  CHECK_EQ (u"" != string_u16 (u"abc"), true);

  CHECK_EQ (string_u16 (u"abc") == string_u16 (u"abc"), true);
  CHECK_EQ (string_u16 (u"abc") == string_u16 (), false);
  CHECK_EQ (string_u16 (u"abc") != string_u16 (u"abc"), false);
  CHECK_EQ (string_u16 (u"abc") != string_u16 (), true);
}

TEST_CASE ("compare string") {
  CHECK (string_u16 (u"ab") < string_u16 (u"b"));
  CHECK (!(string_u16 (u"b") < string_u16 (u"ab")));
  CHECK (string_u16 () < string_u16 (u"0"));
  CHECK (!(string_u16 (u"0") < string_u16 ()));
  CHECK (string_u16 (u"a") <= string_u16 (u"a"));
  CHECK (!(string_u16 (u"ab") <= string_u16 (u"a")));
  CHECK (string_u16 (u"ab") <= string_u16 (u"b"));
  CHECK (!(string_u16 (u"b") <= string_u16 (u"ab")));
  CHECK (string_u16 () <= string_u16 ());
  CHECK (string_u16 () <= string_u16 (u"0"));
  CHECK (!(string_u16 (u"0") <= string_u16 ()));
}

TEST_CASE ("test slice") {
  auto abcde= string_u16 (u"abcde");
  CHECK_EQ (abcde (0, 0) == string_u16 (), true);
  CHECK_EQ (abcde (0, 1) == string_u16 (u"a"), true);
  CHECK_EQ (abcde (1, 3) (0, 1) == string_u16 (u"b"), true);
  CHECK_EQ (abcde (0, 10) == string_u16 (u"abcde"), true);
  CHECK_EQ (abcde (-1, 1) == string_u16 (u"a"), true);
  CHECK_EQ (abcde (3, 2) == string_u16 (), true);
  CHECK_EQ (abcde (3, -2) == string_u16 (), true);
  CHECK_EQ (abcde (10, 11) == string_u16 (), true);
  CHECK_EQ (abcde (-3, -2) == string_u16 (), true);

  // string_view == string
  CHECK_EQ (string_u16 (u"a") == abcde (0, 1), true);
}

TEST_CASE ("test concat") {
  CHECK_EQ (string_u16 (u"abc") * u"de" == string_u16 (u"abcde"), true);
  CHECK_EQ (string_u16 (u"abc") * string_u16 (u"de") == string_u16 (u"abcde"),
            true);
  CHECK_EQ (string_u16 (u"abc") * string_u16 (u"Hello") (0, 2) ==
                string_u16 (u"abcHe"),
            true);
}

/******************************************************************************
 * Modifications
 ******************************************************************************/

TEST_CASE ("test append") {
  auto str= string_u16 ();
  str << u'x';
  CHECK (str == u"x");
  str << string_u16 (u"yz");
  CHECK (str == u"xyz");
}

TEST_CASE ("test reserve along with append") {

  SUBCASE ("reserved more space") {
    auto str= string_u16 ();
    str->reserve (6);
    str << u'x';
    CHECK_EQ (str == u"x", true);
    str << string_u16 (u"yz");
    CHECK_EQ (str == u"xyz", true);
    str << string_u16 (u": larger than reserved space");
    CHECK_EQ (str == u"xyz: larger than reserved space", true);
  }
  SUBCASE ("reserved the same space") {
    auto str= string_u16 (u"abc");
    str->reserve (3);
    CHECK_EQ (str == u"abc", true);
  }
  SUBCASE ("reserved less space should take no effect") {
    auto str= string_u16 (u"abc");
    str->reserve (2);
    CHECK_EQ (str == u"abc", true);
  }
}

TEST_CASE ("test expand_or_shrink_by along with sub index") {

  SUBCASE ("expand") {
    auto str       = string_u16 (u"abc");
    int  previous_n= str->expand_or_shrink_by (1);
    CHECK_EQ (previous_n, 3);
    str[3]= u'd';
    CHECK_EQ (str == u"abcd", true);
  }
  SUBCASE ("shrink") {
    auto str       = string_u16 (u"abc");
    int  previous_n= str->expand_or_shrink_by (-1);
    CHECK_EQ (previous_n, 3);
    CHECK_EQ (str == u"ab", true);
  }
  SUBCASE ("delta 0 takes no effect") {
    auto str       = string_u16 (u"abc");
    int  previous_n= str->expand_or_shrink_by (0);
    CHECK_EQ (previous_n, 3);
    CHECK_EQ (str == u"abc", true);
  }
}

TEST_CASE ("test resize") {

  SUBCASE ("expand") {
    auto str= string_u16 (u"abc");
    str->resize (4);
    str[3]= u'd';
    CHECK_EQ (str == u"abcd", true);
  }
  SUBCASE ("shrink") {
    auto str= string_u16 (u"abc");
    str->resize (2);
    CHECK_EQ (str == u"ab", true);
  }
  SUBCASE ("delta 0 takes no effect") {
    auto str= string_u16 (u"abc");
    str->resize (3);
    CHECK_EQ (str == u"abc", true);
  }
}

TEST_CASE ("test copy") {
  string_u16 str1= u"str1";
  string_u16 str2= copy (str1);
  CHECK (str1 == str2);
  CHECK (str1.buffer () != str2.buffer ());

  string_u16 str3= copy (str1 (0, 3));
  CHECK (str3 == u"str");
}

TEST_CASE ("test iteration") {
  string_u16 str1         = u"s\0tr1";
  int        expectedIndex= 0;
  for (const auto element : str1) {
    CHECK_EQ (element, str1[expectedIndex]);
    ++expectedIndex;
  }
  CHECK_EQ (expectedIndex, N (str1));
}