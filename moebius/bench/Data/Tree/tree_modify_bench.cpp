/** \file tree_modify_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for tree correction on the editing path
 *  \date   2026
 */

#include "nanobench.h"
#include "observers.hpp"
#include "tree_helper.hpp"
#include "tree_modify.hpp"

#include <moebius/drd/drd_std.hpp>

using namespace moebius;

// tree_modify 的 assign/correct 链引用全局编辑树与 ip 观察者，
// 两者定义在 mogan 主程序侧，基准中给出未挂接的独立桩实现
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
  (void) o1; (void) o2; // 基准树未挂观察者，不会被调用
  return observer ();
}

/** 构造典型文档：100 段，每段 concat 由原子与少量复合节点组成 */
static tree
mk_document () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 100; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 10; j++) {
      par << tree ("word" * as_string (j));
      if (j == 4)
        par << tree (make_tree_label ("with"), tree ("color"),
                     tree ("red"), tree ("inner"));
    }
    doc << par;
  }
  return doc;
}

int
main () {
  tree doc = mk_document ();
  tree doc2= mk_document ();
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (100).unit ("op");
  bench.run ("correct_node", [&] {
    correct_node (doc);
    for (int i= 0; i < N (doc); i++)
      correct_node (doc[i]);
  });
  bench.run ("correct_downwards", [&] { correct_downwards (doc2); });
  return 0;
}
