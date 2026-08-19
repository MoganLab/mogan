/** \file tree_helper_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for tree_helper predicates on the typesetting path
 *  \date   2026
 */

#include "nanobench.h"
#include "tree_helper.hpp"

using namespace moebius;

/** 构造典型文档：100 段，每段 concat 由原子与少量复合节点组成 */
static tree
mk_document () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 100; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 10; j++) {
      par << tree ("word" * as_string (j));
      if (j == 4)
        par << tree (make_tree_label ("with"), tree ("color"), tree ("red"),
                     tree ("inner" * as_string (j)));
    }
    doc << par;
  }
  return doc;
}

/** 递归统计 is_empty 为真的节点数，模拟排版期的逐节点调用 */
static int
count_empty (tree t) {
  int r= is_empty (t) ? 1 : 0;
  if (is_compound (t))
    for (int i= 0; i < N (t); i++)
      r+= count_empty (t[i]);
  return r;
}

int
main () {
  tree                     doc= mk_document ();
  int                      n  = 0;
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (100).unit ("op");
  bench.run ("is_empty whole doc", [&] { n+= count_empty (doc); });
  bench.run ("is_snippet", [&] { n+= is_snippet (doc) ? 1 : 0; });
  bench.run ("is_multi_paragraph",
             [&] { n+= is_multi_paragraph (doc) ? 1 : 0; });
  ankerl::nanobench::doNotOptimizeAway (n);
  return 0;
}
