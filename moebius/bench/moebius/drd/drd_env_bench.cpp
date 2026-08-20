/** \file drd_env_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for drd_info::get_env_child (cursor validation path)
 *  \date   2026
 */

#include "nanobench.h"
#include "tree.hpp"
#include "tree_helper.hpp"

#include <moebius/drd/drd_info.hpp>
#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::drd::drd_decode;
using moebius::drd::drd_env_merge;
using moebius::drd::drd_env_read;
using moebius::drd::init_std_drd;
using moebius::drd::the_drd;

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

/** 优化前实现:一律构造 ATTR、合并、线性读取,用于同二进制 A/B 对比 */
static tree
old_get_env_child (tree t, int i, string var, tree val) {
  tree env (ATTR);
  if (L (t) == WITH && i == N (t) - 1)
    env= drd_env_merge (env, t (0, N (t) - 1));
  else {
    drd::tag_info ti   = the_drd->info[L (t)];
    int           index= ti->get_index (i, N (t));
    if ((index < 0) || (index >= N (ti->ci))) return val;
    tree cenv= drd_decode (ti->ci[index].env);
    for (int k= 1; k < N (cenv); k+= 2)
      if (is_func (cenv[k], ARG, 1) && is_int (cenv[k][0])) {
        cenv  = copy (cenv);
        int j2= as_int (cenv[k][0]);
        if (j2 >= 0 && j2 < N (t)) cenv[k]= copy (t[j2]);
      }
    env= drd_env_merge (env, cenv);
  }
  return drd_env_read (env, var, val);
}

int
main () {
  init_std_drd ();
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (20).unit ("doc-sweep");

  tree doc= mk_document ();
  tree acc ("");

  // 模拟 is_accessible_cursor 的调用形态:逐节点读 mode 环境
  bench.run ("old get_env_child mode sweep", [&] {
    tree r ("");
    for (int i= 0; i < N (doc); i++) {
      r= old_get_env_child (doc, i, "mode", tree (""));
      for (int j= 0; j < N (doc[i]); j++)
        r= old_get_env_child (doc[i], j, "mode", tree (""));
    }
    acc= r;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  bench.run ("new get_env_child mode sweep", [&] {
    tree r ("");
    for (int i= 0; i < N (doc); i++) {
      r= the_drd->get_env_child (doc, i, "mode", tree (""));
      for (int j= 0; j < N (doc[i]); j++)
        r= the_drd->get_env_child (doc[i], j, "mode", tree (""));
    }
    acc= r;
    ankerl::nanobench::doNotOptimizeAway (acc);
  });
  // EXTERN 节点(可执行标记)的光标可达性:每个节点都要
  // 派生 "extern:<name>" 标签
  tree xdoc (DOCUMENT);
  for (int i= 0; i < 200; i++) {
    tree ex (EXTERN, tree ("strong-tag" * as_string (i % 5)));
    for (int j= 0; j < 4; j++)
      ex << tree ("arg" * as_string (j));
    xdoc << ex;
  }
  bench.run ("is_accessible_child extern sweep", [&] {
    int n= 0;
    for (int i= 0; i < N (xdoc); i++)
      for (int j= 0; j < N (xdoc[i]); j++)
        n+= the_drd->is_accessible_child (xdoc[i], j) ? 1 : 0;
    ankerl::nanobench::doNotOptimizeAway (n);
  });
  // 源码模式文档:大量 with "mode" "src" 包裹,
  // is_accessible_cursor 逐节点读 mode 环境
  tree wdoc (DOCUMENT);
  for (int i= 0; i < 500; i++) {
    tree w (WITH, tree ("mode"), tree ("src"), tree ("body" * as_string (i)));
    wdoc << w;
  }
  tree wacc ("");
  bench.run ("get_env_child WITH mode sweep", [&] {
    tree r ("");
    for (int i= 0; i < N (wdoc); i++)
      r= the_drd->get_env_child (wdoc[i], 2, "mode", tree ("text"));
    wacc= r;
    ankerl::nanobench::doNotOptimizeAway (wacc);
  });
  // 环境链下探:深层 WITH 嵌套求 mode(get_env_descendant env 变体)
  tree deep= tree ("leaf");
  for (int i= 0; i < 24; i++)
    deep= tree (WITH, tree ("mode"), tree ("src"), deep);
  tree wdoc2 (DOCUMENT, deep);
  path dp= path (1);
  for (int i= 0; i < 24; i++)
    dp= path (2, dp);
  dp= path (0, dp);
  bench.run ("get_env_descendant WITH chain24", [&] {
    ankerl::nanobench::doNotOptimizeAway (
        the_drd->get_env_descendant (wdoc2, dp, tree (ATTR)));
  });
  // 不同变量链:env 逐层增长,走插入/追加路径
  tree deep2= tree ("leaf");
  for (int i= 0; i < 24; i++)
    deep2=
        tree (WITH, tree ("v" * as_string ((i * 7) % 24)), tree ("src"), deep2);
  tree wdoc3 (DOCUMENT, deep2);
  path dp2= path (1);
  for (int i= 0; i < 24; i++)
    dp2= path (2, dp2);
  dp2= path (0, dp2);
  bench.run ("get_env_descendant distinct vars chain24", [&] {
    ankerl::nanobench::doNotOptimizeAway (
        the_drd->get_env_descendant (wdoc3, dp2, tree (ATTR)));
  });
  return 0;
}
