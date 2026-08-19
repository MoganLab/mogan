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

// 生产代码导出但未写入 hpp,基准中补声明
void raw_split (tree& ref, int pos, int at);
void raw_join (tree& ref, int pos);

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

// 优化前的逐元素搬移 split,用于同二进制 A/B 对比
static void
old_raw_split (tree& ref, int pos, int at) {
  tree t= ref[pos], t1, t2;
  if (is_atomic (ref[pos])) {
    t1= ref[pos]->label (0, at);
    t2= ref[pos]->label (at, N (ref[pos]->label));
  }
  else {
    t1= ref[pos](0, at);
    t2= ref[pos](at, N (ref[pos]));
  }
  int i, n= N (ref);
  AR (ref)->resize (n + 1);
  for (i= n; i > (pos + 1); i--)
    ref[i]= ref[i - 1];
  ref[pos]    = t1;
  ref[pos + 1]= t2;
}

// 优化前的逐元素搬移 join,用于同二进制 A/B 对比
static void
old_raw_join (tree& ref, int pos) {
  tree t1= ref[pos], t2= ref[pos + 1], t;
  if (is_atomic (t1) && is_atomic (t2)) t= t1->label * t2->label;
  else t= t1 * t2;
  ref[pos]= t;
  int i, n= N (ref) - 1;
  for (i= pos + 1; i < n; i++)
    ref[i]= ref[i + 1];
  AR (ref)->resize (n);
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
  // split/join 配平:在宽文档中部反复切分与合并原子段,
  // 逐元素搬移需移动约 500 个孩子句柄,memmove 只搬一次内存块
  bench.run ("old raw_split+join mid x500", [&] {
    tree t= copy (doc);
    for (int i= 0; i < 500; i++)
      old_raw_split (t, 500, 2);
    for (int i= 0; i < 500; i++)
      old_raw_join (t, 500);
  });
  bench.run ("new raw_split+join mid x500", [&] {
    tree t= copy (doc);
    for (int i= 0; i < 500; i++)
      raw_split (t, 500, 2);
    for (int i= 0; i < 500; i++)
      raw_join (t, 500);
  });
  return 0;
}
