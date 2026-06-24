/** \file lolly_tree_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for tree
 *  \author jingkaimori
 *  \date   2024
 */

#include "lolly/data/lolly_tree.hpp"
#include <nanobench.h>

static ankerl::nanobench::Bench bench;

namespace lolly {
extern void init_tbox ();
} // namespace lolly

using test_tree= lolly::data::lolly_tree<int>;

static test_tree
generate_test_tree (int i= 0, int d= 3) {
  if (d == 0) return test_tree (as_string (i));
  else {
    int       n= 6 + ((int) (2 * sin (1.0 * i * d)));
    test_tree t (2, n);
    for (int j= 0; j < n; i++, j++)
      t[j]= test_tree (i, d - 1);
    return t;
  }
}

int
main () {
  lolly::init_tbox ();
#ifdef OS_WASM
  bench.minEpochIterations (2000);
#else
  bench.minEpochIterations (200000);
#endif
  bench.complexityN (1).run ("construct atomic tree",
                             [&] { test_tree ("abc"); });
  bench.run ("construct compound tree of depth from argument",
             [&] { test_tree (10, "abc", "def", "ghi"); });
  bench.run ("construct compound tree from array",
             [&] { test_tree (10, array<test_tree> ("abc", "def", "ghi")); });
  for (int d= 1; d < 6; d++) {
    bench.complexityN (d);
    bench.run ("construct compound tree of given depth",
               [&] { generate_test_tree (0, d); });
  }
  for (int d= 1; d < 6; d++) {
    static test_tree tr= generate_test_tree (0, d);
    bench.complexityN (d).run ("type of tree",
                               [&] { lolly::data::is_compound (tr); });
  }
  for (int d= 1; d < 6; d++) {
    static test_tree tr= generate_test_tree (0, d);
    bench.complexityN (d).run ("size of tree", [&] { lolly::data::N (tr); });
  }
  for (int d= 1; d < 6; d++) {
    static test_tree tr= generate_test_tree (0, d);
    bench.complexityN (d).run ("immutable index of tree",
                               [&] { lolly::data::A (tr)[0]; });
  }
  for (int d= 1; d < 6; d++) {
    static test_tree tr= generate_test_tree (0, d);
    bench.complexityN (d).run ("mutable index of tree",
                               [&] { lolly::data::AR (tr)[0]= "abcdef"; });
  }
  for (int d= 1; d < 6; d++) {
    static test_tree tr= generate_test_tree (0, d);
    bench.complexityN (d).run ("copy tree", [&] { lolly::data::copy (tr); });
  }
  for (int d= 1; d < 6; d++) {
    static test_tree tr1= generate_test_tree (0, d),
                     tr2= generate_test_tree (0, d);
    bench.complexityN (d).run ("equality of tree", [&] { tr1 == tr2; });
  }
}
