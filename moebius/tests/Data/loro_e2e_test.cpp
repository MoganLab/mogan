/** \file loro_e2e_test.cpp
 *  \copyright GPLv3
 *  \details tree <-> Loro CRDT snapshot 端到端往返测试。
 *
 *            仅在 LORO_ENABLED（xmake option libloro=y）下编译用例；关闭时本文件为
 *            空二进制（无 TEST_CASE），不影响默认构建。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "moe_doctests.hpp"
#include "loro.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using namespace moebius;

#ifdef LORO_ENABLED

static void
ensure_labels () {
  moebius::drd::init_std_drd ();
}

TEST_CASE ("loro e2e: atomic string round-trips through Loro snapshot") {
  tree t ("hello 世界 <tag>|\\n");
  tree back = loro_to_tree (tree_to_loro (t));
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro e2e: nested document round-trips through Loro") {
  ensure_labels ();
  // (document (para (concat "Hello " (with "world"))) "trailing text")
  tree t (DOCUMENT, 2);
  t[0]       = tree (PARA, 1);
  t[0][0]    = tree (CONCAT, 2);
  t[0][0][0] = tree ("Hello ");
  t[0][0][1] = tree (WITH, 1);
  t[0][0][1][0] = tree ("world");
  t[1]       = tree ("trailing text");

  string snap = tree_to_loro (t);
  CHECK_EQ (N (snap) > 0, true); // 非空 snapshot
  tree back = loro_to_tree (snap);
  CHECK_EQ (back == t, true);
}

TEST_CASE ("loro e2e: empty compound round-trips") {
  ensure_labels ();
  tree t (DOCUMENT, 0);
  tree back = loro_to_tree (tree_to_loro (t));
  CHECK_EQ (back == t, true);
}

#endif // LORO_ENABLED
