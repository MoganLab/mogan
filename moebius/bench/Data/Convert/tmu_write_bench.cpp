/** \file tmu_write_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for TMU document serialization (tree_to_tmu)
 *  \date   2026
 */

#include "nanobench.h"
#include "tmu.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::COLLECTION;
using moebius::CONCAT;
using moebius::DOCUMENT;

/** 构造含大量连续空段落的文档:每次 cr() 时行尾只剩上一轮的
 * 缩进空格,触发尾随空格改写路径(原实现整段前缀拷贝) */
static tree
mk_empty_paras (int n) {
  tree doc (DOCUMENT);
  for (int i= 0; i < n; i++)
    doc << tree ("");
  return doc;
}

/** 常规文档对照 */
static tree
mk_document () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 500; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 10; j++)
      par << tree ("word" * as_string (j));
    doc << par;
  }
  return doc;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (5).unit ("doc");

  tree empties= mk_empty_paras (2000);
  tree doc    = mk_document ();
  bench.run ("tree_to_tmu empty paras x2000", [&] {
    ankerl::nanobench::doNotOptimizeAway (tree_to_tmu (empties));
  });
  bench.run ("tree_to_tmu regular doc500x10", [&] {
    ankerl::nanobench::doNotOptimizeAway (tree_to_tmu (doc));
  });
  return 0;
}
