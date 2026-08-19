/** \file tmu_read_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for TMU document parsing (tmu_to_tree)
 *  \date   2026
 */

#include "nanobench.h"
#include "tmu.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::CONCAT;
using moebius::DOCUMENT;

/** 构造典型文档:500 段,每段 10 个单词与少量标记 */
static tree
mk_document () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 500; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 10; j++) {
      par << tree ("word" * as_string (j));
      if (j == 4) par << tree (RIGID, tree ("r" * as_string (j)));
    }
    doc << par;
  }
  return doc;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (5).unit ("doc");

  tree   doc  = mk_document ();
  string tmu  = tree_to_tmu (doc);
  size_t bytes= N (tmu);

  bench.run ("tmu_to_tree doc500x10",
             [&] { ankerl::nanobench::doNotOptimizeAway (tmu_to_tree (tmu)); });
  bench.run ("tree_to_tmu doc500x10",
             [&] { ankerl::nanobench::doNotOptimizeAway (tree_to_tmu (doc)); });
  ankerl::nanobench::doNotOptimizeAway (bytes);
  return 0;
}
