/** \file tree_observer_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for raw tree modifications on the editing path
 *  \date   2026
 */

#include "nanobench.h"
#include "observers.hpp"
#include "tree_helper.hpp"
#include "tree_observer.hpp"

#include <moebius/drd/drd_std.hpp>

using namespace moebius;

// 修改链引用的全局编辑树与 ip 观察者定义在 mogan 主程序侧，
// 基准中给出未挂接的独立桩实现
tree the_et;
path
obtain_ip (tree& ref) {
  (void) ref; // 未挂接的树没有 ip
  return path ();
}
bool
ip_attached (path ip) {
  (void) ip;
  return false;
}
observer
list_observer (observer o1, observer o2) {
  (void) o1;
  (void) o2; // 基准树未挂观察者，不会被调用
  return observer ();
}

/** 构造有 1000 个孩子的宽文档 */
static tree
mk_wide_doc () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 1000; i++)
    doc << tree ("para" * as_string (i));
  return doc;
}

int
main () {
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (10).unit ("op");

  // 单个 op 内插删配平，避免 epoch 叠加导致文档规模无界增长
  tree doc= mk_wide_doc ();
  bench.run ("raw_insert+remove head x1000", [&] {
    for (int i= 0; i < 1000; i++)
      raw_insert (doc, 0, tree (DOCUMENT, tree ("x")));
    for (int i= 0; i < 1000; i++)
      raw_remove (doc, 0, 1);
  });
  return 0;
}
