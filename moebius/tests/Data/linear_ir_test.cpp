/** \file linear_ir_test.cpp
 *  \copyright GPLv3
 *  \details tree <-> 线性 IR <-> markup 往返相等性测试（统一包裹方案：每个节点
 *            OPEN(label)...CLOSE，原子 label 为空）。覆盖
 * atomic、compound（内置
 *            与扩展标签）、generic（op<0）、嵌套与子节点顺序、相邻原子、二进制
 *            原子、含哨兵字符的用户文本。本用例不依赖 loro-ffi（linear_ir 为纯
 *            C++），不以 LORO_ENABLED 门控，始终编译运行。compound 往复依赖标签
 *            name<->op 表，由 init_std_drd() 填充（幂等）。
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

static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

/******************************************************************************
 * tree <-> 线性 IR 往返
 *****************************************************************************/

TEST_CASE ("linear_ir: atomic string round-trips") {
  tree t ("hello 世界 <with>|tag");
  tree back= linear_ir_to_tree (tree_to_linear_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: atomic is wrapped OPEN('')+TEXT+CLOSE") {
  tree               t ("hi");
  array<linear_item> ir= tree_to_linear_ir (t);
  CHECK_EQ (N (ir) == 3, true);
  CHECK_EQ (ir[0].kind == LI_OPEN && N (ir[0].label) == 0, true);
  CHECK_EQ (ir[1].kind == LI_TEXT && ir[1].text == "hi", true);
  CHECK_EQ (ir[2].kind == LI_CLOSE, true);
}

TEST_CASE ("linear_ir: compound with known label round-trips") {
  ensure_labels ();
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

TEST_CASE ("linear_ir: adjacent atomics round-trip") {
  ensure_labels ();
  // (concat "a" "b" "c") —— 三个相邻原子，各有 OPEN('')/CLOSE 边界
  tree t (CONCAT, 3);
  t[0]     = tree ("a");
  t[1]     = tree ("b");
  t[2]     = tree ("c");
  tree back= linear_ir_to_tree (tree_to_linear_ir (t));
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
  string bin;
  bin << (char) 0x89;
  bin << (char) 0x50;
  bin << (char) 0x4e;
  bin << (char) 0x47;
  bin << (char) 0x0d;
  bin << (char) 0x0a;
  tree               t (bin);
  array<linear_item> ir= tree_to_linear_ir (t);
  // OPEN('') BINARY CLOSE
  CHECK_EQ (N (ir) == 3, true);
  CHECK_EQ (ir[1].kind == LI_BINARY, true);
  CHECK_EQ (ir[1].text == bin, true);
  tree back= linear_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

/******************************************************************************
 * markup 编解码往返
 *****************************************************************************/

TEST_CASE ("linear_ir: markup round-trips a compound document") {
  ensure_labels ();
  tree t (DOCUMENT, 2);
  t[0]                 = tree (PARA, 1);
  t[0][0]              = tree ("hello");
  t[1]                 = tree (PARA, 1);
  t[1][0]              = tree ("world");
  array<linear_item> ir= tree_to_linear_ir (t);
  tree t2= linear_ir_to_tree (markup_to_linear_ir (linear_ir_to_markup (ir)));
  CHECK_EQ (t2 == t, true);
}

TEST_CASE ("linear_ir: markup escapes sentinel bytes in user text") {
  string s;
  s << "a";
  s << (char) 0x01;
  s << "b";
  s << (char) 0x02;
  s << "c";
  tree t (s);
  tree back= linear_ir_to_tree (
      markup_to_linear_ir (linear_ir_to_markup (tree_to_linear_ir (t))));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("linear_ir: markup round-trips binary atomic") {
  string bin;
  bin << (char) 0x89;
  bin << (char) 0x50;
  bin << (char) 0x4e;
  bin << (char) 0x47;
  tree t (bin);
  tree back= linear_ir_to_tree (
      markup_to_linear_ir (linear_ir_to_markup (tree_to_linear_ir (t))));
  CHECK_EQ (back == t, true);
}

/******************************************************************************
 * 自愈解析（并发下可能产生暂时不配对的 markup）
 *****************************************************************************/

TEST_CASE ("linear_ir: unclosed OPEN auto-heals at end") {
  ensure_labels ();
  array<linear_item> ir;
  linear_item        o;
  o.kind = LI_OPEN;
  o.label= "document";
  ir << o;
  o.label= "para";
  ir << o;
  linear_item txt;
  txt.kind= LI_TEXT;
  txt.text= "x";
  ir << txt;
  tree t= linear_ir_to_tree (ir); // 不崩即可
  CHECK_EQ (N (t) == 1, true);
  CHECK_EQ (as_string (L (t)) == "document", true);
}

TEST_CASE ("linear_ir: stray CLOSE is skipped") {
  ensure_labels ();
  array<linear_item> ir;
  linear_item        c;
  c.kind= LI_CLOSE;
  ir << c;
  linear_item o;
  o.kind = LI_OPEN;
  o.label= "document";
  ir << o;
  ir << c;
  tree t= linear_ir_to_tree (ir);
  CHECK_EQ (as_string (L (t)) == "document", true);
}

TEST_CASE ("linear_ir: empty markup yields empty document") {
  tree t= linear_ir_to_tree (markup_to_linear_ir (""));
  CHECK_EQ (N (t) == 0, true);
  CHECK_EQ (as_string (L (t)) == "document", true);
}
