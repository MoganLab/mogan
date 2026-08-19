/** \file tree_cursor_bench.cpp
 *  \copyright GPLv3
 *  \details Benchmark for cursor movement (move_any via next_any/next_valid)
 *  \date   2026
 */

#include "cork.hpp"
#include "nanobench.h"
#include "tree.hpp"
#include "tree_cursor.hpp"
#include "tree_helper.hpp"
#include "tree_traverse.hpp"

#include <moebius/drd/drd_std.hpp>
#include <moebius/tree_label.hpp>

using namespace moebius;
using moebius::drd::init_std_drd;
using moebius::drd::the_drd;

/** 构造典型文档:100 段,每段 concat 由单词与少量复合节点组成 */
static tree
mk_document () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 100; i++) {
    tree par (CONCAT);
    for (int j= 0; j < 8; j++) {
      par << tree ("word" * as_string (j));
      if (j == 3) par << tree (RIGID, tree ("r" * as_string (j)));
    }
    doc << par;
  }
  return doc;
}

// 优化前实现:每次移动三次 subtree 全树下探,用于同二进制 A/B 对比
static path
old_move_any (tree t, path p, bool forward) {
  path q = path_up (p);
  int  l = last_item (p);
  tree st= subtree (t, q);
  if (!is_nil (q) && is_func (subtree (t, path_up (q)), RAW_DATA)) {
    if (forward) return path_up (q) * 1;
    else return path_up (q) * 0;
  }
  if (is_atomic (st)) {
    string s= st->label;
    l       = max (min (l, N (s)), 0);
    if (forward) {
      if (l < N (s)) {
        tm_char_forwards (s, l);
        return q * l;
      }
    }
    else {
      if (l > 0) {
        tm_char_backwards (s, l);
        return q * l;
      }
    }
  }
  else if ((forward && l == 0) || (!forward && l == 1)) {
    int i, n= N (st);
    if (forward) {
      for (i= 0; i < n; i++)
        if (the_drd->is_accessible_child (st, i)) return q * path (i, 0);
    }
    else {
      for (i= n - 1; i >= 0; i--)
        if (the_drd->is_accessible_child (st, i))
          return q * path (i, right_index (st[i]));
    }
    return q * (1 - l);
  }
  else if (is_nil (q)) return p;
  l = last_item (q);
  q = path_up (q);
  st= subtree (t, q);
  int i, n= N (st);
  if (forward) {
    for (i= l + 1; i < n; i++)
      if (the_drd->is_accessible_child (st, i)) return q * path (i, 0);
  }
  else {
    for (i= l - 1; i >= 0; i--)
      if (the_drd->is_accessible_child (st, i)) {
        return q * path (i, right_index (st[i]));
      }
  }
  return q * (forward ? 1 : 0);
}

// 生产代码导出但未写入 hpp,基准中补声明
path next_any (tree t, path p);

/** 构造单词移动基准文档:100 段含普通词/连字符词/数字词 */
static tree
mk_word_doc () {
  tree doc (DOCUMENT);
  for (int i= 0; i < 100; i++) {
    tree par (CONCAT);
    par << tree ("alpha beta") << tree ("gamma-delta. ") << tree ("x1 x2 y3");
    doc << par;
  }
  return doc;
}

int
main () {
  init_std_drd ();
  ankerl::nanobench::Bench bench;
  bench.minEpochIterations (50).unit ("sweep");

  tree doc= mk_document ();
  path p0 = start (doc);

  // 从文档头逐字符推进到尾,模拟光标移动/选择的完整扫
  bench.run ("old next_any full sweep", [&] {
    path p= p0;
    int  n= 0;
    while (true) {
      path r= old_move_any (doc, p, true);
      if (r == p) break;
      p= r;
      n++;
    }
    ankerl::nanobench::doNotOptimizeAway (n);
  });
  bench.run ("new next_any full sweep", [&] {
    path p= p0;
    int  n= 0;
    while (true) {
      path r= next_any (doc, p);
      if (r == p) break;
      p= r;
      n++;
    }
    ankerl::nanobench::doNotOptimizeAway (n);
  });
  // Ctrl+Right:逐词扫全文档
  ankerl::nanobench::Bench wbench;
  wbench.minEpochIterations (20).unit ("sweep");
  tree wdoc= mk_word_doc ();
  path wp0 = start (wdoc);
  wbench.run ("next_word full sweep", [&] {
    path p= wp0;
    int  n= 0;
    while (true) {
      path r= next_word (wdoc, p);
      if (r == p) break;
      p= r;
      n++;
    }
    ankerl::nanobench::doNotOptimizeAway (n);
  });
  // 光标校正:keep_positive + pre_correct + left/right_correct
  ankerl::nanobench::Bench cbench2;
  cbench2.minEpochIterations (200).unit ("sweep");
  cbench2.run ("correct_cursor sweep100", [&] {
    path pp= wp0;
    int  n = 0;
    for (int i= 0; i < 100; i++) {
      path r= correct_cursor (wdoc, pp, true);
      if (r != pp) n++;
      pp= r;
    }
    ankerl::nanobench::doNotOptimizeAway (n);
  });
  return 0;
}
