/** \file tree_traverse_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for tree_utf8_to_herk / tree_herk_to_utf8
 *  \date   2026
 */

#include "nanobench.h"
#include "tree.hpp"
#include "tree_traverse.hpp"

#include <lolly/data/herk.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;

/** 构造典型文档:1000 段,90% 纯 ASCII 段,10% 含非 ASCII 字符的段 */
static tree
mk_document () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 1000; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 10; j++) {
      if (i % 10 == 0 && j == 5)
        par << tree ("w\xC3\xA9rds " * as_string (j)); // é
      else par << tree ("word number " * as_string (j));
    }
    doc << par;
  }
  return doc;
}

/** 优化前实现:原子一律走 lolly 逐字符转换,用于同二进制 A/B 对比 */
static tree
old_utf8_to_herk (tree t) {
  if (is_atomic (t)) return tree (lolly::data::utf8_to_herk (t->label));
  else if (!is_func (t, RAW_DATA)) {
    int  n= N (t);
    tree t2 (t, n);
    for (int i= 0; i < n; i++)
      t2[i]= old_utf8_to_herk (t[i]);
    return t2;
  }
  else return t;
}

/** 优化前实现,用于同二进制 A/B 对比 */
static tree_u8
old_herk_to_utf8 (tree t) {
  if (is_atomic (t)) return tree (lolly::data::herk_to_utf8 (t->label));
  else if (!is_func (t, RAW_DATA)) {
    int  n= N (t);
    tree t2 (t, n);
    for (int i= 0; i < n; i++)
      t2[i]= old_herk_to_utf8 (t[i]);
    return t2;
  }
  else return t;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (10).unit ("doc");

  tree doc     = mk_document ();
  tree herk_doc= tree_utf8_to_herk (doc);

  bench.run ("old utf8->herk doc1000x10", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_utf8_to_herk (doc));
  });
  bench.run ("new utf8->herk doc1000x10", [&] {
    ankerl::nanobench::doNotOptimizeAway (tree_utf8_to_herk (doc));
  });
  bench.run ("old herk->utf8 doc1000x10", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_herk_to_utf8 (herk_doc));
  });
  bench.run ("new herk->utf8 doc1000x10", [&] {
    ankerl::nanobench::doNotOptimizeAway (tree_herk_to_utf8 (herk_doc));
  });

  // 纯 ASCII 长文档:快路径全覆盖的理想场景
  tree plain (DOCUMENT);
  for (int i= 0; i < 1000; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 10; j++)
      par << tree ("word number " * as_string (j));
    plain << par;
  }
  bench.run ("old utf8->herk ascii-only", [&] {
    ankerl::nanobench::doNotOptimizeAway (old_utf8_to_herk (plain));
  });
  bench.run ("new utf8->herk ascii-only", [&] {
    ankerl::nanobench::doNotOptimizeAway (tree_utf8_to_herk (plain));
  });
  return 0;
}
