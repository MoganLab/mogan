/** \file tree_traverse_test.cpp
 *  \copyright GPLv3
 *  \details Unit tests for tree_utf8_to_herk / tree_herk_to_utf8
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_traverse.hpp"

#include <moebius/tree_label.hpp>

using namespace moebius;

static string
label_of (tree_u8 t) {
  return t->label;
}

TEST_SUITE ("tree_traverse") {

TEST_CASE ("utf8_to_herk plain ascii is identity") {
  tree_u8 t ("Hello, world! [42] #{foo}");
  CHECK (label_of (tree_utf8_to_herk (t)) == "Hello, world! [42] #{foo}");
}

TEST_CASE ("utf8_to_herk escapes backtick") {
  // 反引号 0x60 是 herk 恒等例外,快路径必须放行给慢路径;
  // 其映射目标 herk 字节 0 经 herk_to_utf8 还原回反引号,用往返验证
  tree_u8 t ("a`b");
  CHECK (label_of (tree_herk_to_utf8 (tree_utf8_to_herk (t))) == "a`b");
}

TEST_CASE ("utf8_to_herk converts non-ascii") {
  // U+00E9 (é) 的 utf8 字节为 C3 A9,herk 中直接映射为单个高位字节 0xE9
  tree_u8 t ("\xC3\xA9");
  string r= label_of (tree_utf8_to_herk (t));
  CHECK (r == string ("\xE9", 1));
}

TEST_CASE ("utf8_to_herk roundtrip compound") {
  tree doc (DOCUMENT);
  doc << tree ("plain");
  doc << tree ("\xC3\xA9"); // é
  doc << tree ("more");
  tree h= tree_utf8_to_herk (doc);
  REQUIRE (N (h) == 3);
  CHECK (label_of (h[0]) == "plain");
  CHECK (label_of (h[1]) == string ("\xE9", 1));
  tree_u8 back= tree_herk_to_utf8 (h);
  REQUIRE (N (back) == 3);
  CHECK (label_of (back[0]) == "plain");
  CHECK (label_of (back[1]) == "\xC3\xA9");
}

TEST_CASE ("herk_to_utf8 keeps literal ascii") {
  tree t ("abc XYZ 123");
  CHECK (label_of (tree_herk_to_utf8 (t)) == "abc XYZ 123");
}

TEST_CASE ("herk_to_utf8 expands hex escape") {
  tree t ("a<#41>b");
  CHECK (label_of (tree_herk_to_utf8 (t)) == "aAb");
}

TEST_CASE ("herk_to_utf8 keeps lone lt sign") {
  // '<' 不后跟 '#' 时不构成转义,恒等快路径应正确放行
  tree t ("a<b");
  CHECK (label_of (tree_herk_to_utf8 (t)) == "a<b");
  tree t2 ("a<<#41>");
  CHECK (label_of (tree_herk_to_utf8 (t2)) == "a<A");
}

TEST_CASE ("herk_to_utf8 maps high bytes") {
  // herk 字节 0xE9 -> U+00E9 -> utf8 C3 A9
  tree t (string ("\xE9", 1));
  CHECK (label_of (tree_herk_to_utf8 (t)) == "\xC3\xA9");
}

TEST_CASE ("conversion preserves raw_data") {
  tree rd (RAW_DATA, tree ("payload"));
  tree h= tree_utf8_to_herk (rd);
  CHECK (is_func (h, RAW_DATA));
  CHECK (N (h) == 1);
  CHECK (h[0] == tree ("payload"));
}

} // TEST_SUITE
