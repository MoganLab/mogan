/** \file linear_ir_ops_test.cpp
 *  \copyright GPLv3
 *  \details 验证 linear_ir_apply_mod（clean_apply 等价）与 compute_markup_edit
 *            （body LoroText 最小字节 splice）。compute_markup_edit 的 offset
 *            正确性用 round-trip 性质独立校验：splice 应用到「操作前 markup」后
 *            应等于 clean_apply 后树的 markup。纯逻辑，不依赖 loro-ffi。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "linear_ir.hpp"
#include "linear_ir_ops.hpp"
#include "modification.hpp"
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

static bool
apply_matches_clean (tree t, modification mod) {
  tree               expected= clean_apply (t, mod);
  array<linear_item> ir      = tree_to_linear_ir (t);
  array<linear_item> ir2     = linear_ir_apply_mod (ir, mod);
  tree               got     = linear_ir_to_tree (ir2);
  return got == expected;
}

/******************************************************************************
 * linear_ir_apply_mod：与 clean_apply 等价
 *****************************************************************************/

TEST_CASE ("linear_ir_ops: SPLIT atomic matches clean_apply") {
  ensure_labels ();
  tree t (CONCAT, 1);
  t[0]= tree ("hello");
  CHECK_EQ (apply_matches_clean (t, mod_split (path (), 0, 3)), true);
}

TEST_CASE ("linear_ir_ops: SPLIT compound matches clean_apply") {
  ensure_labels ();
  tree inner (CONCAT, 3);
  inner[0]= tree ("a");
  inner[1]= tree ("b");
  inner[2]= tree ("c");
  tree t (CONCAT, 1);
  t[0]= inner;
  CHECK_EQ (apply_matches_clean (t, mod_split (path (), 0, 1)), true);
}

TEST_CASE ("linear_ir_ops: JOIN atomic matches clean_apply") {
  ensure_labels ();
  tree t (CONCAT, 2);
  t[0]= tree ("hel");
  t[1]= tree ("lo");
  CHECK_EQ (apply_matches_clean (t, mod_join (path (), 0)), true);
}

TEST_CASE ("linear_ir_ops: JOIN compound matches clean_apply") {
  ensure_labels ();
  tree a (CONCAT, 1);
  a[0]= tree ("a");
  tree b (CONCAT, 2);
  b[0]= tree ("b");
  b[1]= tree ("c");
  tree t (CONCAT, 2);
  t[0]= a;
  t[1]= b;
  CHECK_EQ (apply_matches_clean (t, mod_join (path (), 0)), true);
}

TEST_CASE ("linear_ir_ops: SPLIT then JOIN round-trips to original tree") {
  ensure_labels ();
  tree inner (CONCAT, 3);
  inner[0]= tree ("a");
  inner[1]= tree ("b");
  inner[2]= tree ("c");
  tree t (CONCAT, 1);
  t[0]                 = inner;
  array<linear_item> ir= tree_to_linear_ir (t);
  array<linear_item> s = linear_ir_apply_mod (ir, mod_split (path (), 0, 1));
  array<linear_item> j = linear_ir_apply_mod (s, mod_join (path (), 0));
  CHECK_EQ (linear_ir_to_tree (j) == t, true);
}

TEST_CASE ("linear_ir_ops: INSERT_NODE matches clean_apply") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]= tree ("hello");
  CHECK_EQ (
      apply_matches_clean (t, mod_insert_node (path (0), 0, tree (CONCAT, 0))),
      true);
}

TEST_CASE ("linear_ir_ops: REMOVE_NODE matches clean_apply") {
  ensure_labels ();
  tree inner (CONCAT, 1);
  inner[0]= tree ("hello");
  tree t (DOCUMENT, 1);
  t[0]= inner;
  CHECK_EQ (apply_matches_clean (t, mod_remove_node (path (0), 0)), true);
}

TEST_CASE ("linear_ir_ops: INSERT_NODE then REMOVE_NODE round-trips") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]                 = tree ("hello");
  array<linear_item> ir= tree_to_linear_ir (t);
  array<linear_item> w=
      linear_ir_apply_mod (ir, mod_insert_node (path (0), 0, tree (CONCAT, 0)));
  array<linear_item> u= linear_ir_apply_mod (w, mod_remove_node (path (0), 0));
  CHECK_EQ (linear_ir_to_tree (u) == t, true);
}

TEST_CASE ("linear_ir_ops: ASSIGN is passed through unchanged") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]                 = tree ("x");
  array<linear_item> ir= tree_to_linear_ir (t);
  tree               neu (DOCUMENT, 1);
  neu[0]                = tree ("y");
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod_assign (path (), neu));
  CHECK_EQ (N (ir2) == N (ir), true);
}

/******************************************************************************
 * compute_markup_edit：body LoroText 最小字节 splice
 *****************************************************************************/

static bool
markup_edit_roundtrip (tree t, modification mod) {
  array<linear_item> pre   = tree_to_linear_ir (t);
  string             pre_mk= linear_ir_to_markup (pre);
  markup_edit        ed    = compute_markup_edit (pre, mod);
  if (!ed.ok) return false;
  string result= pre_mk (0, ed.offset) * ed.insert_bytes *
                 pre_mk (ed.offset + ed.delete_len, N (pre_mk));
  string expected=
      linear_ir_to_markup (tree_to_linear_ir (clean_apply (t, mod)));
  if (result != expected) {
    cout << "pre_mk  : " << pre_mk << LF;
    cout << "result  : " << result << LF;
    cout << "expected: " << expected << LF;
  }
  return result == expected;
}

// CLOSE + OPEN("") 的 markup 字节：\x01C\x01 \x01O\x01
static string
close_open_empty_bytes () {
  string co;
  co << (char) 0x01;
  co << 'C';
  co << (char) 0x01;
  co << (char) 0x01;
  co << 'O';
  co << (char) 0x01;
  return co;
}

TEST_CASE ("markup_edit: text insert offset") {
  ensure_labels ();
  tree t (CONCAT, 1);
  t[0]            = tree ("hel");
  modification mod= mod_insert (path (0), 1, tree ("x"));
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
  markup_edit ed= compute_markup_edit (tree_to_linear_ir (t), mod);
  CHECK_EQ (ed.delete_len == 0 && ed.insert_bytes == "x", true);
}

TEST_CASE ("markup_edit: text remove offset") {
  ensure_labels ();
  tree t (CONCAT, 1);
  t[0]            = tree ("hello");
  modification mod= mod_remove (path (0), 1, 2);
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
  markup_edit ed= compute_markup_edit (tree_to_linear_ir (t), mod);
  CHECK_EQ (ed.delete_len == 2 && N (ed.insert_bytes) == 0, true);
}

TEST_CASE ("markup_edit: text insert into empty atomic") {
  ensure_labels ();
  tree t (CONCAT, 1);
  t[0]            = tree ("");
  modification mod= mod_insert (path (0), 0, tree ("z"));
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
}

TEST_CASE ("markup_edit: text insert escapes sentinel byte") {
  ensure_labels ();
  tree t (CONCAT, 1);
  t[0]= tree ("he");
  string s;
  s << (char) 0x01;
  modification mod= mod_insert (path (0), 1, tree (s));
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
  markup_edit ed= compute_markup_edit (tree_to_linear_ir (t), mod);
  string      exp;
  exp << (char) 0x02;
  exp << '1';
  CHECK_EQ (ed.insert_bytes == exp, true);
}

TEST_CASE ("markup_edit: SPLIT atomic inserts only CLOSE+OPEN('')") {
  ensure_labels ();
  tree t (CONCAT, 1);
  t[0]            = tree ("hello");
  modification mod= mod_split (path (), 0, 3);
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
  markup_edit ed= compute_markup_edit (tree_to_linear_ir (t), mod);
  CHECK_EQ (ed.delete_len == 0 && ed.insert_bytes == close_open_empty_bytes (),
            true);
}

TEST_CASE ("markup_edit: SPLIT compound inserts CLOSE+OPEN(label)") {
  ensure_labels ();
  tree inner (CONCAT, 3);
  inner[0]= tree ("a");
  inner[1]= tree ("b");
  inner[2]= tree ("c");
  tree t (CONCAT, 1);
  t[0]            = inner;
  modification mod= mod_split (path (), 0, 1);
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
}

TEST_CASE ("markup_edit: JOIN atomic deletes only CLOSE+OPEN('')") {
  ensure_labels ();
  tree t (CONCAT, 2);
  t[0]            = tree ("he");
  t[1]            = tree ("llo");
  modification mod= mod_join (path (), 0);
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
  markup_edit ed= compute_markup_edit (tree_to_linear_ir (t), mod);
  CHECK_EQ (ed.delete_len == 6 && N (ed.insert_bytes) == 0, true);
}

TEST_CASE ("markup_edit: JOIN compound deletes CLOSE+OPEN(label)") {
  ensure_labels ();
  tree a (CONCAT, 1);
  a[0]= tree ("a");
  tree b (CONCAT, 1);
  b[0]= tree ("b");
  tree t (CONCAT, 2);
  t[0]            = a;
  t[1]            = b;
  modification mod= mod_join (path (), 0);
  CHECK_EQ (markup_edit_roundtrip (t, mod), true);
}

TEST_CASE ("markup_edit: INSERT_NODE falls back to coarse (ok=false)") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]          = tree ("hello");
  markup_edit ed= compute_markup_edit (
      tree_to_linear_ir (t), mod_insert_node (path (0), 0, tree (CONCAT, 0)));
  CHECK_EQ (ed.ok, false); // v1：INSERT_NODE 暂走 coarse 重 seed
}
