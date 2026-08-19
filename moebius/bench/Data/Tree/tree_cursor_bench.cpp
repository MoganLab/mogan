/** \file tree_cursor_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for cursor validation and movement hot paths
 *  \date   2026
 */

#include "nanobench.h"
#include "tree_cursor.hpp"
#include "tree_helper.hpp"
#include "tree_traverse.hpp"

#include <moebius/drd/drd_std.hpp>

using namespace moebius;

/** 构造一篇有代表性的文档：document 下 100 个段落，
 * 每段是 concat，由 10 个原子片段和少量复合节点构成
 */
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

/** 收集一批典型光标路径：每段中部原子片段的起点/终点 */
static array<path>
mk_paths (tree& doc) {
  array<path> ps;
  for (int i= 0; i < N (doc); i++)
    for (int j= 0; j < N (doc[i]); j++)
      if (is_atomic (doc[i][j])) {
        ps << path (i, j, 0);
        ps << path (i, j, N (doc[i][j]->label));
      }
  return ps;
}

int
main () {
  tree        doc= mk_document ();
  array<path> ps = mk_paths (doc);
  path        mid= ps[N (ps) >> 1];

  bool                     b= false;
  path                     r= path ();
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (1000).unit ("op");
  bench.run ("valid_cursor", [&] {
    for (int i= 0; i < N (ps); i++)
      b= b || valid_cursor (doc, ps[i]);
  });
  bench.run ("is_accessible_cursor", [&] {
    for (int i= 0; i < N (ps); i++)
      b= b || is_accessible_cursor (doc, ps[i]);
  });
  bench.run ("next_valid", [&] {
    path q= mid;
    for (int i= 0; i < 200; i++)
      q= next_valid (doc, q);
    r= q;
  });
  bench.run ("correct_cursor", [&] { r= correct_cursor (doc, mid, true); });

  ankerl::nanobench::doNotOptimizeAway (b);
  ankerl::nanobench::doNotOptimizeAway (r);
  return 0;
}
