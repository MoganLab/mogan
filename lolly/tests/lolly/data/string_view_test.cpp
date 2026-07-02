
/** \file string_view_test.cpp
 *  \copyright GPLv3
 *  \details Test for string_view
 *  \author jingkaimori
 *  \date   2024
 */

#include "a_lolly_test.hpp"
#include "lolly/data/string_u16.hpp"
#include "lolly/data/string_view.hpp"
#include <doctest/doctest.h>

using lolly::data::string_u16_view;

TEST_CASE ("construct string_view") {
  const char16_t arr[6]= u"hello";
  CHECK (string_u16_view (arr) == u"hello");
  const char16_t arr3[6]= {u'5', u'c', u'h', u'a', u'r', u's'};
  CHECK (string_u16_view (arr3) == u"5char");

  const char16_t  arr11[12]= u"hello";
  string_u16_view view11   = string_u16_view (arr11);
  CHECK (view11.N == 11);
  CHECK (view11 (0, 5) == u"hello");

  CHECK (string_u16_view (u"abc") == u"abc");

  const char16_t* str= u"def";
  CHECK (string_u16_view (str, 3) == u"def");

  const std::u16string std_str (u"ghij");
  CHECK (string_u16_view (std_str) == u"ghij");
}
