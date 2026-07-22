/** \file loro_ir_test.cpp
 *  \copyright GPLv3
 *  \details tree <-> LoroIRNode 往返相等性测试。覆盖
 * atomic、compound（内置与扩展
 *            标签）、generic（op<0）、嵌套与子节点顺序、空复合节点。
 *            compound 往复依赖标签 name<->op 表，由 init_std_drd()
 * 填充（幂等）。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "moe_doctests.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

#ifdef LORO_ENABLED

// 填充标签 name<->op 表（init_std_drd 自带幂等 guard），compound 往返依赖它
static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

TEST_CASE ("loro_ir: atomic string round-trips with special chars") {
  tree t ("hello 世界 <with>|tag\\n");
  tree back= loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: compound with known label round-trips") {
  ensure_labels ();
  // (document "a" (para "b"))
  tree t (DOCUMENT, 2);
  t[0]     = tree ("a");
  t[1]     = tree (PARA, 1);
  t[1][0]  = tree ("b");
  tree back= loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
  CHECK_EQ (as_string (L (back)) == "document", true);
}

TEST_CASE ("loro_ir: compound with extension label round-trips") {
  ensure_labels ();
  tree_label ext= make_tree_label ("my-custom-extension-tag");
  tree       t (ext, 1);
  t[0]     = tree ("x");
  tree back= loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
  CHECK_EQ (as_string (L (back)) == "my-custom-extension-tag", true);
}

TEST_CASE ("loro_ir: generic op<0 round-trips") {
  tree t (-7, 1);
  t[0]           = tree ("g");
  loro_ir_node ir= tree_to_loro_ir (t);
  CHECK_EQ (ir.kind == LORO_GENERIC, true);
  CHECK_EQ (ir.label == "generic:-7", true);
  tree back= loro_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: nested children and order preserved") {
  ensure_labels ();
  // (concat "a" (with "b") "c")
  tree t (CONCAT, 3);
  t[0]           = tree ("a");
  t[1]           = tree (WITH, 1);
  t[1][0]        = tree ("b");
  t[2]           = tree ("c");
  loro_ir_node ir= tree_to_loro_ir (t);
  CHECK_EQ (N (ir.children) == 3, true);
  CHECK_EQ (ir.children[0].kind == LORO_ATOMIC, true);
  CHECK_EQ (ir.children[0].text == "a", true);
  CHECK_EQ (ir.children[1].label == "with", true);
  tree back= loro_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: empty compound round-trips") {
  ensure_labels ();
  tree t (DOCUMENT, 0);
  tree back= loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: deeply nested tree") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]         = tree (PARA, 1);
  t[0][0]      = tree (WITH, 1);
  t[0][0][0]   = tree (CONCAT, 1);
  t[0][0][0][0]= tree ("deep");
  tree back    = loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: tree with large number of children") {
  ensure_labels ();
  int  n= 1000;
  tree t (CONCAT, n);
  for (int i= 0; i < n; ++i) {
    t[i]= tree ("x");
  }
  tree back= loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: long text node") {
  string s= "a";
  for (int i= 0; i < 10; ++i)
    s= s * 2; // length 1024
  tree t (s);
  tree back= loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
}

#endif // LORO_ENABLED