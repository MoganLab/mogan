/** \file linear_ir_test.cpp
 *  \copyright GPLv3
 *  \details tree <-> 线性 IR <-> markup 往返相等性测试。覆盖 atomic、compound
 *            （内置与扩展标签）、generic（op<0）、嵌套与子节点顺序、相邻原子、
 *            二进制原子、含哨兵字符的用户文本，以及 MARKER(SPLIT) 的展开语义。
 *            本用例不依赖 loro-ffi（linear_ir 为纯 C++），故不以 LORO_ENABLED
 *            门控，始终编译运行，提供秒级反馈。compound 往复依赖标签 name<->op
 *            表，由 init_std_drd() 填充（幂等）。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "linear_ir.hpp"
#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

// 填充标签 name<->op 表（init_std_drd 自带幂等 guard）
static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

// ---- 构造 item 的小工具 ----
static linear_item
mk_open (string label) {
  linear_item it;
  it.kind = LI_OPEN;
  it.label= label;
  return it;
}
static linear_item
mk_close () {
  linear_item it;
  it.kind= LI_CLOSE;
  return it;
}
static linear_item
mk_text (string s) {
  linear_item it;
  it.kind= LI_TEXT;
  it.text= s;
  return it;
}
static linear_item
mk_split () {
  linear_item it;
  it.kind  = LI_MARKER;
  it.marker= LM_SPLIT;
  return it;
}

/******************************************************************************
 * tree <-> 线性 IR 往返
 *****************************************************************************/

TEST_CASE ("linear_ir: atomic string round-trips") {
  tree t ("hello 世界 <with>|tag");
  tree back= linear_ir_to_tree (tree_to_linear_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: atomic becomes single bare TEXT item") {
  tree               t ("hi");
  array<linear_item> ir= tree_to_linear_ir (t);
  CHECK_EQ (N (ir) == 1, true);
  CHECK_EQ (ir[0].kind == LI_TEXT, true);
  CHECK_EQ (ir[0].text == "hi", true);
}

TEST_CASE ("linear_ir: compound with known label round-trips") {
  ensure_labels ();
  // (document "a" (para "b"))
  tree t (DOCUMENT, 2);
  t[0]     = tree ("a");
  t[1]     = tree (PARA, 1);
  t[1][0]  = tree ("b");
  tree back= linear_ir_to_tree (tree_to_linear_ir (t));
  CHECK_EQ (back == t, true);
  CHECK_EQ (as_string (L (back)) == "document", true);
}

TEST_CASE ("linear_ir: compound with extension label round-trips") {
  ensure_labels ();
  tree_label ext= make_tree_label ("my-custom-extension-tag");
  tree       t (ext, 1);
  t[0]     = tree ("x");
  tree back= linear_ir_to_tree (tree_to_linear_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: generic op<0 round-trips") {
  tree t (-7, 1);
  t[0]                 = tree ("g");
  array<linear_item> ir= tree_to_linear_ir (t);
  CHECK_EQ (ir[0].kind == LI_OPEN, true);
  CHECK_EQ (ir[0].label == "generic:-7", true);
  tree back= linear_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: adjacent atomics are distinct children") {
  ensure_labels ();
  // (concat "a" "b" "c") —— 三个相邻原子子节点
  tree t (CONCAT, 3);
  t[0]                 = tree ("a");
  t[1]                 = tree ("b");
  t[2]                 = tree ("c");
  array<linear_item> ir= tree_to_linear_ir (t);
  // OPEN(concat) TEXT TEXT TEXT CLOSE
  CHECK_EQ (N (ir) == 5, true);
  CHECK_EQ (ir[1].kind == LI_TEXT && ir[1].text == "a", true);
  CHECK_EQ (ir[2].kind == LI_TEXT && ir[2].text == "b", true);
  CHECK_EQ (ir[3].kind == LI_TEXT && ir[3].text == "c", true);
  tree back= linear_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: empty compound round-trips") {
  ensure_labels ();
  tree t (DOCUMENT, 0);
  tree back= linear_ir_to_tree (tree_to_linear_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: deeply nested tree round-trips") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]         = tree (PARA, 1);
  t[0][0]      = tree (WITH, 1);
  t[0][0][0]   = tree (CONCAT, 1);
  t[0][0][0][0]= tree ("deep");
  tree back    = linear_ir_to_tree (tree_to_linear_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: large number of children round-trips") {
  ensure_labels ();
  int  n= 1000;
  tree t (CONCAT, n);
  for (int i= 0; i < n; ++i)
    t[i]= tree ("x");
  tree back= linear_ir_to_tree (tree_to_linear_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: binary atomic (non-UTF-8) round-trips") {
  // PNG 头：0x89 0x50 0x4E 0x47 ... —— 非法 UTF-8 → BINARY
  string bin;
  bin << (char) 0x89;
  bin << (char) 0x50;
  bin << (char) 0x4e;
  bin << (char) 0x47;
  bin << (char) 0x0d;
  bin << (char) 0x0a;
  tree               t (bin);
  array<linear_item> ir= tree_to_linear_ir (t);
  CHECK_EQ (N (ir) == 1, true);
  CHECK_EQ (ir[0].kind == LI_BINARY, true);
  CHECK_EQ (ir[0].text == bin, true);
  tree back= linear_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

/******************************************************************************
 * markup 编解码往返
 *****************************************************************************/

TEST_CASE ("linear_ir: markup round-trips a compound document") {
  ensure_labels ();
  tree t (DOCUMENT, 2);
  t[0]                     = tree (PARA, 1);
  t[0][0]                  = tree ("hello");
  t[1]                     = tree (PARA, 1);
  t[1][0]                  = tree ("world");
  array<linear_item> ir    = tree_to_linear_ir (t);
  string             markup= linear_ir_to_markup (ir);
  array<linear_item> back  = markup_to_linear_ir (markup);
  tree               t2    = linear_ir_to_tree (back);
  CHECK_EQ (t2 == t, true);
}

TEST_CASE ("linear_ir: markup escapes sentinel bytes in user text") {
  // 用户文本含 \x01/\x02 哨兵字节，不得与 token 冲突
  string s;
  s << "a";
  s << (char) 0x01;
  s << "b";
  s << (char) 0x02;
  s << "c";
  tree   t (s);
  string markup= linear_ir_to_markup (tree_to_linear_ir (t));
  tree   back  = linear_ir_to_tree (markup_to_linear_ir (markup));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: markup round-trips binary atomic") {
  string bin;
  bin << (char) 0x89;
  bin << (char) 0x50;
  bin << (char) 0x4e;
  bin << (char) 0x47;
  tree   t (bin);
  string markup= linear_ir_to_markup (tree_to_linear_ir (t));
  tree   back  = linear_ir_to_tree (markup_to_linear_ir (markup));
  CHECK_EQ (back == t, true);
}

/******************************************************************************
 * MARKER(SPLIT) 展开语义（结构操作的基础）
 *****************************************************************************/

TEST_CASE ("linear_ir: SPLIT marker yields same-label siblings") {
  ensure_labels ();
  // [OPEN(document), OPEN(para), TEXT("hel"), MARKER(SPLIT), TEXT("lo"), CLOSE,
  // CLOSE] → (document (para "hel")(para "lo"))
  array<linear_item> ir;
  ir << mk_open ("document");
  ir << mk_open ("para");
  ir << mk_text ("hel");
  ir << mk_split ();
  ir << mk_text ("lo");
  ir << mk_close ();
  ir << mk_close ();
  tree t= linear_ir_to_tree (ir);

  tree expected (DOCUMENT, 2);
  expected[0]   = tree (PARA, 1);
  expected[0][0]= tree ("hel");
  expected[1]   = tree (PARA, 1);
  expected[1][0]= tree ("lo");
  CHECK_EQ (t == expected, true);
}

TEST_CASE ("linear_ir: removing SPLIT (JOIN) keeps two atomics in one node") {
  ensure_labels ();
  // 同上但去掉 SPLIT → (document (para "hel" "lo"))：两段原子成为同一 para
  // 的子节点
  array<linear_item> ir;
  ir << mk_open ("document");
  ir << mk_open ("para");
  ir << mk_text ("hel");
  ir << mk_text ("lo");
  ir << mk_close ();
  ir << mk_close ();
  tree t= linear_ir_to_tree (ir);

  tree expected (DOCUMENT, 1);
  expected[0]   = tree (PARA, 2);
  expected[0][0]= tree ("hel");
  expected[0][1]= tree ("lo");
  CHECK_EQ (t == expected, true);
}

TEST_CASE ("linear_ir: SPLIT marker survives markup round-trip") {
  ensure_labels ();
  array<linear_item> ir;
  ir << mk_open ("document");
  ir << mk_open ("para");
  ir << mk_text ("hel");
  ir << mk_split ();
  ir << mk_text ("lo");
  ir << mk_close ();
  ir << mk_close ();
  string             markup= linear_ir_to_markup (ir);
  array<linear_item> back  = markup_to_linear_ir (markup);
  CHECK_EQ (N (back) == N (ir), true);
  CHECK_EQ (back[3].kind == LI_MARKER && back[3].marker == LM_SPLIT, true);
  tree expected (DOCUMENT, 2);
  expected[0]   = tree (PARA, 1);
  expected[0][0]= tree ("hel");
  expected[1]   = tree (PARA, 1);
  expected[1][0]= tree ("lo");
  CHECK_EQ (linear_ir_to_tree (back) == expected, true);
}

/******************************************************************************
 * 自愈解析（并发下可能产生暂时不配对的 markup）
 *****************************************************************************/

TEST_CASE ("linear_ir: unclosed OPEN auto-heals at end") {
  ensure_labels ();
  // 缺少末尾 CLOSE：OPEN(document) OPEN(para) TEXT("x")
  array<linear_item> ir;
  ir << mk_open ("document");
  ir << mk_open ("para");
  ir << mk_text ("x");
  tree t= linear_ir_to_tree (ir); // 不崩即可
  CHECK_EQ (N (t) == 1, true);
  CHECK_EQ (as_string (L (t)) == "document", true);
}

TEST_CASE ("linear_ir: stray CLOSE is skipped") {
  ensure_labels ();
  // 起始即 CLOSE，随后正常 OPEN/CLOSE
  array<linear_item> ir;
  ir << mk_close ();
  ir << mk_open ("document");
  ir << mk_close ();
  tree t= linear_ir_to_tree (ir);
  CHECK_EQ (as_string (L (t)) == "document", true);
}

TEST_CASE ("linear_ir: empty markup yields empty document") {
  tree t= linear_ir_to_tree (markup_to_linear_ir (""));
  CHECK_EQ (N (t) == 0, true);
  CHECK_EQ (as_string (L (t)) == "document", true);
}
