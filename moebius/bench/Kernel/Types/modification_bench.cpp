/** \file modification_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for is_applicable / can_* applicability checks
 *  \date   2026
 */

#include "modification.hpp"
#include "nanobench.h"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/tree_label.hpp>

using namespace moebius;

// 生产代码导出但未写入 hpp,基准中补声明
bool can_insert (tree t, path p, int pos, tree u);
bool can_remove (tree t, path p, int pos, int nr);

/** 构造嵌套文档:10 层 document 嵌套,每层 10 个孩子 */
static tree
mk_deep_doc () {
  tree t (DOCUMENT, tree ("leaf"));
  for (int i= 0; i < 10; i++) {
    tree outer (DOCUMENT);
    for (int j= 0; j < 10; j++)
      outer << tree ("filler" * as_string (j));
    outer[9]= t;
    t= outer;
  }
  return t;
}

static path
mk_deep_path (int n) {
  path p (9);
  for (int i= 1; i < n; i++)
    p= path (9, p);
  return p;
}

// 优化前实现:has_subtree + subtree 两趟遍历,用于同二进制 A/B 对比
static bool
old_can_insert (tree t, path p, int pos, tree u) {
  if (!has_subtree (t, p)) return false;
  tree st= subtree (t, p);
  if (is_atomic (st)) return pos >= 0 && pos <= N (st->label) && is_atomic (u);
  else return pos >= 0 && pos <= N (st) && is_compound (u);
}

static bool
old_can_remove (tree t, path p, int pos, int nr) {
  if (!has_subtree (t, p)) return false;
  tree st= subtree (t, p);
  if (is_atomic (st)) return pos >= 0 && pos + nr <= N (st->label);
  else return pos >= 0 && pos + nr <= N (st);
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (10000).unit ("op");

  tree doc= mk_deep_doc ();
  path deep= mk_deep_path (9);
  tree  atomic_ins ("xyz");
  tree  compound_ins (DOCUMENT, tree ("x"));
  int   acc        = 0;

  bench.run ("old can_insert depth9", [&] {
    acc+= old_can_insert (doc, deep, 0, compound_ins) ? 1 : 0;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new can_insert depth9", [&] {
    acc+= can_insert (doc, deep, 0, compound_ins) ? 1 : 0;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("old can_remove depth9", [&] {
    acc+= old_can_remove (doc, deep, 0, 1) ? 1 : 0;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new can_remove depth9", [&] {
    acc+= can_remove (doc, deep, 0, 1) ? 1 : 0;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  // 命中原子节点后继续下探(失败路径)
  path over= mk_deep_path (12);
  bench.run ("old can_insert miss depth12", [&] {
    acc+= old_can_insert (doc, over, 0, atomic_ins) ? 1 : 0;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new can_insert miss depth12", [&] {
    acc+= can_insert (doc, over, 0, atomic_ins) ? 1 : 0;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  return 0;
}
