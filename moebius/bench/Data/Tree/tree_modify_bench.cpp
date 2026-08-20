/** \file tree_modify_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for correct_node / correct_downwards / simplify_concat
 *  \date   2026
 */

#include "nanobench.h"
#include "tree.hpp"
#include "tree_helper.hpp"
#include "tree_modify.hpp"
#include "tree_observer.hpp"

#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::drd::init_std_drd;
using moebius::drd::the_drd;

// 修改链引用的全局编辑树与 ip 观察者定义在 mogan 主程序侧,
// 基准中给出未挂接的独立桩实现
tree the_et;
path
obtain_ip (tree& ref) {
  (void) ref;
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
  (void) o2;
  return observer ();
}

/** 构造典型文档:500 段,每段 concat 由原子与少量复合节点组成 */
static tree
mk_document () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 500; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 8; j++) {
      par << tree ("word" * as_string (j));
      if (j == 3) par << tree (RIGID, tree ("r" * as_string (j)));
    }
    doc << par;
  }
  return doc;
}

// 生产代码中导出但未写入 hpp(bench 中补声明以复用同一路径)
void correct_concat_node (tree& t, int done);

/** 优化前实现:contains 走字符串名绕行,用于同二进制 A/B 对比 */
static void
old_correct_node (tree& t) {
  if (is_compound (t)) {
    if (the_drd->contains (as_string (L (t))) &&
        !the_drd->correct_arity (L (t), N (t)))
      assign (t, "");
    if (is_concat (t)) correct_concat_node (t, 0);
  }
}

static void
old_correct_downwards (tree& t) {
  if (is_compound (t))
    for (int i= 0; i < N (t); i++)
      old_correct_downwards (t[i]);
  old_correct_node (t);
}

int
main () {
  init_std_drd ();
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (5).unit ("doc");

  tree doc= mk_document ();

  bench.run ("old correct_downwards doc500x8", [&] {
    tree t= copy (doc);
    old_correct_downwards (t);
    ankerl::nanobench::doNotOptimizeAway (t);
  });
  bench.run ("new correct_downwards doc500x8", [&] {
    tree t= copy (doc);
    correct_downwards (t);
    ankerl::nanobench::doNotOptimizeAway (t);
  });
  // 隔离 correct_node 本身:预校正一遍后原地反复全树遍历调用,
  // 树已收敛,每轮只做 contains/arity 判定,不再有修改开销
  tree corrected= copy (doc);
  correct_downwards (corrected);
  bench.run ("old correct_node sweep x100", [&] {
    for (int k= 0; k < 100; k++) {
      old_correct_downwards (corrected);
    }
    ankerl::nanobench::doNotOptimizeAway (corrected);
  });
  bench.run ("new correct_node sweep x100", [&] {
    for (int k= 0; k < 100; k++) {
      correct_downwards (corrected);
    }
    ankerl::nanobench::doNotOptimizeAway (corrected);
  });
  // simplify_correct 全树往返(排版热路径):无变化的子树直接共享原节点
  bench.run ("simplify_correct sweep", [&] {
    ankerl::nanobench::doNotOptimizeAway (simplify_correct (doc));
  });
  return 0;
}
