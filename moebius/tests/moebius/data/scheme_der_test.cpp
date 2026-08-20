/** \file scheme_der_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for scheme source parsing (string_to_scheme_tree)
 *  \date   2026
 */

#include "moe_doctests.hpp"

#include "tree_helper.hpp"

#include <moebius/data/scheme.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;

using namespace moebius::data;

static string
atom_of (scheme_tree t) {
  return t->label;
}

TEST_SUITE ("scheme_der") {

  TEST_CASE ("parse simple list") {
    scheme_tree t= string_to_scheme_tree ("(a b c)");
    CHECK (is_tuple (t));
    CHECK_EQ (N (t), 3);
    CHECK (atom_of (t[0]) == "a");
    CHECK (atom_of (t[2]) == "c");
  }

  TEST_CASE ("parse token at buffer end") {
    // 词元恰在缓冲区末尾结束(原实现越界读末尾一字节)
    scheme_tree t= string_to_scheme_tree ("(abc)");
    CHECK_EQ (N (t), 1);
    CHECK (atom_of (t[0]) == "abc");
  }

  TEST_CASE ("parse trailing backslash at end") {
    // 转义字符位于缓冲区末尾(原实现 unslash 越界读)
    scheme_tree t= string_to_scheme_tree ("abc\\");
    CHECK (atom_of (t) == "abc\\");
  }

  TEST_CASE ("parse quoted string with escapes") {
    scheme_tree t= string_to_scheme_tree ("\"a\\nb\\tc\\\\d\"");
    string      s= atom_of (t);
    CHECK (s == "\"a\nb\tc\\d\"");
  }

  TEST_CASE ("parse unclosed quote at end") {
    // 未闭合引号在缓冲区末尾截断,补上收尾引号
    scheme_tree t= string_to_scheme_tree ("\"abc");
    CHECK (atom_of (t) == "\"abc\"");
  }

  TEST_CASE ("parse skips comments") {
    scheme_tree t= string_to_scheme_tree ("; comment\n(foo)");
    CHECK (is_tuple (t));
    CHECK_EQ (N (t), 1);
    CHECK (atom_of (t[0]) == "foo");
  }

  TEST_CASE ("parse quote sugar") {
    scheme_tree t= string_to_scheme_tree ("'x");
    CHECK (is_tuple (t));
    CHECK_EQ (N (t), 2);
    CHECK (atom_of (t[0]) == "'");
    CHECK (atom_of (t[1]) == "x");
  }

  TEST_CASE ("parse with carriage returns") {
    // CR 字符被整串剔除,词元内 CR 直接拼接
    scheme_tree t= string_to_scheme_tree ("(a\015b c\015)");
    CHECK_EQ (N (t), 2);
    CHECK (atom_of (t[0]) == "ab");
    CHECK (atom_of (t[1]) == "c");
  }

  TEST_CASE ("block parse concatenated expressions") {
    scheme_tree t= block_to_scheme_tree ("(a) (b) (c)");
    CHECK (is_tuple (t));
    CHECK_EQ (N (t), 3);
    CHECK_EQ (N (t[1]), 1);
    CHECK (atom_of (t[1][0]) == "b");
  }

} // TEST_SUITE
