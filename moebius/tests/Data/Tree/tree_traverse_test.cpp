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
    string  r= label_of (tree_utf8_to_herk (t));
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

// 光标移动测试需要 drd 与 tree_cursor 声明
#include "tree_cursor.hpp"
#include <moebius/drd/drd_std.hpp>

using moebius::drd::init_std_drd;

// 生产代码导出但未写入 hpp,测试中补声明
path next_any (tree t, path p);
path previous_any (tree t, path p);

static tree
mk_cursor_doc () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 10; i++) {
    tree par (CONCAT);
    par << tree ("ab") << tree ("cd");
    doc << par;
  }
  return doc;
}

TEST_CASE ("next_any sweep reaches end and terminates") {
  init_std_drd ();
  tree doc  = mk_cursor_doc ();
  path p    = start (doc);
  int  steps= 0;
  while (steps < 1000) {
    path r= next_any (doc, p);
    if (r == p) break;
    p= r;
    steps++;
  }
  CHECK (steps < 1000);           // 必须收敛
  CHECK (steps > 10);             // 确实逐字符推进了
  CHECK (next_any (doc, p) == p); // 停在不动点
}

TEST_CASE ("next_any then previous_any roundtrip") {
  init_std_drd ();
  tree doc= mk_cursor_doc ();
  path p  = start (doc);
  path mid= p;
  for (int i= 0; i < 20; i++)
    mid= next_any (doc, mid);
  // 从中途倒退相同步数,应回到起点
  path back= mid;
  for (int i= 0; i < 20; i++)
    back= previous_any (doc, back);
  CHECK (back == p);
}

TEST_CASE ("next_any steps into first paragraph") {
  init_std_drd ();
  tree doc= mk_cursor_doc ();
  path p  = start (doc);
  path r1 = next_any (doc, p);
  CHECK (!is_nil (r1));
  CHECK (r1 != p);            // 第一步必有推进
  CHECK (!is_nil (r1->next)); // 深入文档内部而非停在顶层
}

TEST_CASE ("next_word skips whole words") {
  init_std_drd ();
  tree doc (DOCUMENT);
  tree par (CONCAT);
  par << tree ("hello world") << tree ("x");
  doc << par;
  path p= start (doc);
  path q= next_word (doc, p);
  CHECK (q != p);
  // 再前进若干步应收敛到文档尾
  int steps= 0;
  while (steps < 100) {
    path r= next_word (doc, q);
    if (r == q) break;
    q= r;
    steps++;
  }
  CHECK (steps < 100);
  CHECK (next_word (doc, q) == q);
}

TEST_CASE ("next_word previous_word roundtrip") {
  init_std_drd ();
  tree doc (DOCUMENT);
  tree par (CONCAT);
  par << tree ("alpha beta") << tree ("gamma delta");
  doc << par;
  path p  = start (doc);
  path mid= p;
  for (int i= 0; i < 6; i++)
    mid= next_word (doc, mid);
  path back= mid;
  for (int i= 0; i < 6; i++)
    back= previous_word (doc, back);
  CHECK (back == p);
}

TEST_CASE ("word boundary respects punctuation and hex escapes") {
  init_std_drd ();
  // <#4E2D> 形式的非 ASCII 字符按词分隔符块处理,不崩溃即可
  tree doc (DOCUMENT);
  tree par (CONCAT);
  par << tree ("a <#4E2D> b");
  doc << par;
  path p    = start (doc);
  int  steps= 0;
  while (steps < 100) {
    path r= next_word (doc, p);
    if (r == p) break;
    p= r;
    steps++;
  }
  CHECK (steps < 100);
}

TEST_CASE ("correct_cursor drops negative prefix path") {
  init_std_drd ();
  tree doc (DOCUMENT);
  tree par (CONCAT);
  par << tree ("ab") << tree ("cd");
  doc << par;
  // 含负索引的路径被 keep_positive 截断后再校正
  path bad = path (-1, path (0, path (0)));
  path good= correct_cursor (doc, bad, true);
  CHECK (!is_nil (good));
  // 校正结果应是文档内的合法光标(首元素非负)
  CHECK (good->item >= 0);
}

TEST_CASE ("correct_cursor keeps valid path stable") {
  init_std_drd ();
  tree doc (DOCUMENT);
  tree par (CONCAT);
  par << tree ("ab") << tree ("cd");
  doc << par;
  path p    = start (doc);
  path fixed= correct_cursor (doc, p, true);
  CHECK (fixed == p); // 起点已是合法前向光标
}
