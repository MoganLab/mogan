/** \file tree_traverse_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for word-based cursor traversal
 *  \date   2026
 */

#include "nanobench.h"
#include "tree_helper.hpp"
#include "tree_traverse.hpp"

#include <moebius/drd/drd_std.hpp>

using namespace moebius;

/** 构造一段含 TeXmacs 十六进制中文字符的长文本 */
static tree
mk_cjk_doc () {
  string s;
  for (int i= 0; i < 200; i++) {
    s << "<#4e2d>";
    s << "<#6587>";
    if (i % 5 == 0) s << " ";
    if (i % 7 == 0) s << "abc";
  }
  tree doc (DOCUMENT, tree (CONCAT, tree (s)));
  return doc;
}

int
main () {
  tree                     doc= mk_cjk_doc ();
  path                     p (0, 0, 0);
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (1).unit ("op");
  bench.run ("next_word x1000", [&] {
    path q= p;
    for (int i= 0; i < 1000; i++)
      q= next_word (doc, q);
    ankerl::nanobench::doNotOptimizeAway (q);
  });
  return 0;
}
