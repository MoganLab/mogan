/** \file scheme_load_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for .tm document loading (scheme parse + tree convert)
 *  \date   2026
 */

#include "nanobench.h"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/data/scheme.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::data::scheme_document_to_tree;
using moebius::data::scheme_to_tree;

/** 构造典型 .tm 文档文本:500 段,每段 concat 若干词与标记 */
static string
mk_tm_document () {
  string s= "(document (TeXmacs \"2.1.2\") ";
  for (int i= 0; i < 500; i++) {
    s << "(concat \"word0\" \"word1\" (rigid \"r2\") \"word3\")";
  }
  s << ")";
  return s;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (5).unit ("doc");

  string buf= mk_tm_document ();
  bench.run ("scheme_document_to_tree doc500", [&] {
    ankerl::nanobench::doNotOptimizeAway (scheme_document_to_tree (buf));
  });
  bench.run ("scheme_to_tree doc500", [&] {
    ankerl::nanobench::doNotOptimizeAway (scheme_to_tree (buf));
  });
  return 0;
}
