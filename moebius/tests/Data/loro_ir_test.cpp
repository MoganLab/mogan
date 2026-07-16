/** \file loro_ir_test.cpp
 *  \copyright GPLv3
 *  \details tree <-> LoroIRNode 往返相等性测试。覆盖 atomic、compound（内置与扩展
 *            标签）、generic（op<0）、嵌套与子节点顺序、空复合节点。
 *            compound 往复依赖标签 name<->op 表，由 init_std_drd() 填充（幂等）。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "loro_ir.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

// 填充标签 name<->op 表（init_std_drd 自带幂等 guard），compound 往返依赖它
static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

TEST_CASE ("loro_ir: atomic string round-trips with special chars") {
  tree t ("hello 世界 <with>|tag\\n");
  tree back = loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: compound with known label round-trips") {
  ensure_labels ();
  // (document "a" (para "b"))
  tree t (DOCUMENT, 2);
  t[0]    = tree ("a");
  t[1]    = tree (PARA, 1);
  t[1][0] = tree ("b");
  tree back = loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
  CHECK_EQ (as_string (L (back)) == "document", true);
}

TEST_CASE ("loro_ir: compound with extension label round-trips") {
  ensure_labels ();
  tree_label ext = make_tree_label ("my-custom-extension-tag");
  tree       t (ext, 1);
  t[0]       = tree ("x");
  tree back  = loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
  CHECK_EQ (as_string (L (back)) == "my-custom-extension-tag", true);
}

TEST_CASE ("loro_ir: generic op<0 round-trips") {
  tree       t (-7, 1);
  t[0]       = tree ("g");
  loro_ir_node ir = tree_to_loro_ir (t);
  CHECK_EQ (ir.kind == LORO_GENERIC, true);
  CHECK_EQ (ir.label == "generic:-7", true);
  tree back = loro_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: nested children and order preserved") {
  ensure_labels ();
  // (concat "a" (with "b") "c")
  tree t (CONCAT, 3);
  t[0]    = tree ("a");
  t[1]    = tree (WITH, 1);
  t[1][0] = tree ("b");
  t[2]    = tree ("c");
  loro_ir_node ir = tree_to_loro_ir (t);
  CHECK_EQ (N (ir.children) == 3, true);
  CHECK_EQ (ir.children[0].kind == LORO_ATOMIC, true);
  CHECK_EQ (ir.children[0].text == "a", true);
  CHECK_EQ (ir.children[1].label == "with", true);
  tree back = loro_ir_to_tree (ir);
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir: empty compound round-trips") {
  ensure_labels ();
  tree t (DOCUMENT, 0);
  tree back = loro_ir_to_tree (tree_to_loro_ir (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro_ir codec: flat encode/decode self round-trip + bytes") {
  // (compound "document" (atomic "hi") (atomic "x"))
  loro_ir_node orig;
  orig.kind  = LORO_COMPOUND;
  orig.label = "document";
  loro_ir_node c0;
  c0.kind = LORO_ATOMIC;
  c0.text = "hi";
  loro_ir_node c1;
  c1.kind = LORO_ATOMIC;
  c1.text = "x";
  orig.children << c0;
  orig.children << c1;

  string bytes= loro_ir_encode (orig);
  // 打印字节（十进制），用于与 Rust 侧 Writer 输出逐字节比对格式
  cout << "[codec] " << N (bytes) << " bytes:";
  for (int i = 0; i < N (bytes); i++)
    cout << " " << (int) (unsigned char) bytes[i];
  cout << LF;

  loro_ir_node back= loro_ir_decode (bytes);
  CHECK_EQ (back.kind == LORO_COMPOUND, true);
  CHECK_EQ (back.label == "document", true);
  CHECK_EQ (N (back.children) == 2, true);
  CHECK_EQ (back.children[0].text == "hi", true);
  CHECK_EQ (back.children[1].text == "x", true);
}
