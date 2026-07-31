/** \file linear_ir_ops_test.cpp
 *  \copyright GPLv3
 *  \details 验证 linear_ir_apply_mod 对四种结构 modification 的作用结果与
 *            clean_apply（moebius 权威语义）一致，且以最小 item 编辑完成
 *            （SPLIT 仅增 item、JOIN 仅减 item 等），不重建存活内容。
 *            纯逻辑，不依赖 loro-ffi，不以 LORO_ENABLED 门控。
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

// 用 clean_apply 作真值，校验线性 IR 上的 apply 等价
static bool
apply_matches_clean (tree t, modification mod) {
  tree               expected= clean_apply (t, mod);
  array<linear_item> ir      = tree_to_linear_ir (t);
  array<linear_item> ir2     = linear_ir_apply_mod (ir, mod);
  tree               got     = linear_ir_to_tree (ir2);
  return got == expected;
}

/******************************************************************************
 * SPLIT
 *****************************************************************************/

TEST_CASE ("linear_ir_ops: SPLIT atomic matches clean_apply") {
  ensure_labels ();
  // (concat "hello") —— 切 root 的 child 0 在 offset 3
  tree t (CONCAT, 1);
  t[0]            = tree ("hello");
  modification mod= mod_split (path (), 0, 3);
  CHECK_EQ (apply_matches_clean (t, mod), true);
  // 原子切分：一个 TEXT 拆成两个，item 数 +1，无 CLOSE/OPEN 新增
  array<linear_item> ir = tree_to_linear_ir (t);
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod);
  CHECK_EQ (N (ir2) == N (ir) + 1, true);
}

TEST_CASE ("linear_ir_ops: SPLIT compound matches clean_apply") {
  ensure_labels ();
  // (concat (concat "a" "b" "c")) —— 切 root 的 child 0（内层 concat）在 child
  // 1
  tree inner (CONCAT, 3);
  inner[0]= tree ("a");
  inner[1]= tree ("b");
  inner[2]= tree ("c");
  tree t (CONCAT, 1);
  t[0]            = inner;
  modification mod= mod_split (path (), 0, 1);
  CHECK_EQ (apply_matches_clean (t, mod), true);
  // 复合切分：插入 CLOSE + OPEN，item 数 +2
  array<linear_item> ir = tree_to_linear_ir (t);
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod);
  CHECK_EQ (N (ir2) == N (ir) + 2, true);
}

TEST_CASE ("linear_ir_ops: SPLIT at boundary (at == N(children)) matches "
           "clean_apply") {
  ensure_labels ();
  tree inner (CONCAT, 2);
  inner[0]= tree ("a");
  inner[1]= tree ("b");
  tree t (CONCAT, 1);
  t[0]= inner;
  // 在末尾切（at == 子节点数）→ 第二段为空复合
  modification mod= mod_split (path (), 0, 2);
  CHECK_EQ (apply_matches_clean (t, mod), true);
}

/******************************************************************************
 * JOIN
 *****************************************************************************/

TEST_CASE ("linear_ir_ops: JOIN atomic matches clean_apply") {
  ensure_labels ();
  // (concat "hel" "lo") —— join child 0,1
  tree t (CONCAT, 2);
  t[0]            = tree ("hel");
  t[1]            = tree ("lo");
  modification mod= mod_join (path (), 0);
  CHECK_EQ (apply_matches_clean (t, mod), true);
  array<linear_item> ir = tree_to_linear_ir (t);
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod);
  CHECK_EQ (N (ir2) == N (ir) - 1, true); // 两个 TEXT 合一
}

TEST_CASE ("linear_ir_ops: JOIN compound matches clean_apply") {
  ensure_labels ();
  // (concat (concat "a")(concat "b" "c")) —— join child 0,1（两复合）
  tree a (CONCAT, 1);
  a[0]= tree ("a");
  tree b (CONCAT, 2);
  b[0]= tree ("b");
  b[1]= tree ("c");
  tree t (CONCAT, 2);
  t[0]            = a;
  t[1]            = b;
  modification mod= mod_join (path (), 0);
  CHECK_EQ (apply_matches_clean (t, mod), true);
  array<linear_item> ir = tree_to_linear_ir (t);
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod);
  CHECK_EQ (N (ir2) == N (ir) - 2, true); // 删去 CLOSE + OPEN
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
  // 切再合：语义上回到原树（join 是 split 的逆）
  array<linear_item> split= linear_ir_apply_mod (ir, mod_split (path (), 0, 1));
  // split 后 root 仍是 (concat)，内层切成两段；join root 的 child 0,1
  array<linear_item> joined= linear_ir_apply_mod (split, mod_join (path (), 0));
  CHECK_EQ (linear_ir_to_tree (joined) == t, true);
}

/******************************************************************************
 * INSERT_NODE / REMOVE_NODE
 *****************************************************************************/

TEST_CASE ("linear_ir_ops: INSERT_NODE wraps child matches clean_apply") {
  ensure_labels ();
  // (document "hello") —— 把 child 0 包进 (concat)
  tree t (DOCUMENT, 1);
  t[0]= tree ("hello");
  tree         wrapper (CONCAT, 0);
  modification mod= mod_insert_node (path (0), 0, wrapper);
  CHECK_EQ (apply_matches_clean (t, mod), true);
  // 空 wrapper：仅 +OPEN +CLOSE
  array<linear_item> ir = tree_to_linear_ir (t);
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod);
  CHECK_EQ (N (ir2) == N (ir) + 2, true);
}

TEST_CASE ("linear_ir_ops: REMOVE_NODE unwraps matches clean_apply") {
  ensure_labels ();
  // (document (concat "hello")) —— 脱去 concat，提升 child 0
  tree inner (CONCAT, 1);
  inner[0]= tree ("hello");
  tree t (DOCUMENT, 1);
  t[0]            = inner;
  modification mod= mod_remove_node (path (0), 0);
  CHECK_EQ (apply_matches_clean (t, mod), true);
  array<linear_item> ir = tree_to_linear_ir (t);
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod);
  CHECK_EQ (N (ir2) == N (ir) - 2, true);
}

TEST_CASE ("linear_ir_ops: INSERT_NODE then REMOVE_NODE round-trips") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]= tree ("hello");
  tree               wrapper (CONCAT, 0);
  array<linear_item> ir= tree_to_linear_ir (t);
  array<linear_item> wrapped=
      linear_ir_apply_mod (ir, mod_insert_node (path (0), 0, wrapper));
  // wrapped: (document (concat "hello"))；脱去 child 0(child0 of document) 的
  // concat
  array<linear_item> unwrapped=
      linear_ir_apply_mod (wrapped, mod_remove_node (path (0), 0));
  CHECK_EQ (linear_ir_to_tree (unwrapped) == t, true);
}

TEST_CASE ("linear_ir_ops: ASSIGN is passed through unchanged") {
  ensure_labels ();
  tree t (DOCUMENT, 1);
  t[0]                 = tree ("x");
  array<linear_item> ir= tree_to_linear_ir (t);
  tree               neu (DOCUMENT, 1);
  neu[0]                = tree ("y");
  array<linear_item> ir2= linear_ir_apply_mod (ir, mod_assign (path (), neu));
  CHECK_EQ (N (ir2) == N (ir), true); // ASSIGN 本次不动
}
