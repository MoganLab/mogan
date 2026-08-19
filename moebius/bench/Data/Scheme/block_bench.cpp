/** \file block_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for scheme parser
 *  \author jingkaimori
 *  \date   2024
 */

#include "file.hpp"
#include "moebius/data/scheme.hpp"
#include "string.hpp"
#include "sys_utils.hpp"
#include <nanobench.h>

using namespace moebius::data;

static ankerl::nanobench::Bench bench;

int
main () {
  lolly::init_tbox ();
  string buffer;
  url    u= url_pwd () * url ("moebius/bench/Data/Scheme/dictionary.scm");
  load_string (u, buffer, false);
  bench.minEpochIterations (10)
      .batch (N (buffer))
      .unit ("character")
      .run ("parsing large group of simple element",
            [&] { block_to_scheme_tree (buffer); });
  scheme_tree parsed_tree= block_to_scheme_tree (buffer);
  bench.run ("serializing large group of simple element",
             [&] { scheme_tree_to_block (parsed_tree); });

  u= url_pwd () * url ("moebius/bench/Data/Scheme/virtual-font.scm");
  load_string (u, buffer, false);
  bench.batch (N (buffer))
      .run ("parsing group of complex tree",
            [&] { block_to_scheme_tree (buffer); })
      .run ("parsing single tree with complex structure",
            [&] { string_to_scheme_tree (buffer); });
  parsed_tree= block_to_scheme_tree (buffer);
  bench.run ("serializing group of complex tree",
             [&] { scheme_tree_to_block (parsed_tree); });
  bench.run ("serializing single tree with complex structure",
             [&] { scheme_tree_to_string (parsed_tree); });
  return 0;
}
