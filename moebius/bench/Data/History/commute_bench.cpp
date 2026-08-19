/** \file commute_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for modification commutation (undo/redo rebase)
 *  \date   2026
 */

#include "modification.hpp"
#include "nanobench.h"
#include "patch.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/tree_label.hpp>

using namespace moebius;

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (1000).unit ("x100");

  // 打字序列的典型换位:同一段内连续 insert
  int  acc= 0;
  bench.run ("commute inserts same para x100", [&] {
    for (int i= 0; i < 100; i++) {
      modification a= mod_insert (path (0, 3), 5, tree ("abc"));
      modification b= mod_insert (path (0, 3), 9, tree ("xy"));
      acc+= commute (a, b) ? 1 : 0;
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  // 不同段落的 insert(不相交路径,swap_basic 快路径)
  bench.run ("commute inserts disjoint x100", [&] {
    for (int i= 0; i < 100; i++) {
      modification a= mod_insert (path (0, 3), 5, tree ("abc"));
      modification b= mod_insert (path (1, 7), 2, tree ("xy"));
      acc+= commute (a, b) ? 1 : 0;
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  // 嵌套路径:同孩子下递归 swap
  bench.run ("commute nested path x100", [&] {
    for (int i= 0; i < 100; i++) {
      modification a= mod_insert (path (0, 3, 1), 5, tree ("abc"));
      modification b= mod_insert (path (0, 3, 2), 2, tree ("xy"));
      acc+= commute (a, b) ? 1 : 0;
    }
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  return 0;
}
