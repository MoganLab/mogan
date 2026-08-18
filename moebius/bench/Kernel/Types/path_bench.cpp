/** \file path_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for path operations
 *  \date   2026
 */

#include "nanobench.h"
#include "path.hpp"

/** build a left-leaning tree: compound(op=1, [child]) wrapping a leaf n times
 */
static void
mk_deep_tree (tree& t, int depth) {
  t= tree ("leaf");
  for (int i= 0; i < depth; i++)
    t= tree (1, t);
}

/** build an all-zero path of length n pointing at the innermost leaf */
static path
mk_deep_path (int n) {
  path p (0);
  for (int i= 1; i < n; i++)
    p= path (0, p);
  return p;
}

/** build a wide compound with 100 children, each a 3-deep compound */
static tree
mk_wide_tree () {
  tree t (1);
  for (int i= 0; i < 100; i++)
    t << tree (2, tree ("a"), tree ("b"), tree ("c"));
  return t;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (100000).unit ("op");

  tree deep;
  mk_deep_tree (deep, 16);
  path deep_p    = mk_deep_path (16);
  path deep_p_up = path_up (deep_p, 1);
  path deep_p_bad= mk_deep_path (16);
  deep_p_bad     = path_add (deep_p_bad, 1, 15); // last index out of range

  tree wide= mk_wide_tree ();
  path wide_p (50, 1);

  tree* r= nullptr;
  bool  b= false;
  bench.run ("subtree deep hit", [&] { r= &subtree (deep, deep_p); });
  bench.run ("subtree shallow", [&] { r= &subtree (wide, wide_p); });
  bench.run ("subtree parent", [&] { r= &parent_subtree (deep, deep_p); });
  bench.run ("subtree miss", [&] { r= &subtree (deep, deep_p_bad); });
  bench.run ("has_subtree", [&] { b= has_subtree (deep, deep_p); });

  ankerl::nanobench::doNotOptimizeAway (r);
  ankerl::nanobench::doNotOptimizeAway (b);
  return 0;
}
