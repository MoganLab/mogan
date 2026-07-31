/** \file loro_ir.cpp
 *  \copyright GPLv3
 *
 * 实现 tree <-> loro_ir_node 的依赖无关双向映射。
 * 标签走 moebius 的 name<->op 注册表：导出用 as_string(tree_label)
 * 取名字， 导入用 make_tree_label(name) 反查（含扩展标签）。generic
 * 节点（op<0） 用十进制存取（as_string 支持 int、as_int
 * 支持负号）
 *
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"

#include "tree_helper.hpp"

#include <moebius/tree_label.hpp>

#include <cstdint>

/******************************************************************************
 * moebius tree <-> loro_ir_node
 ******************************************************************************/

// generic 节点 label 前缀，op 以十进制存于其后
static const string GENERIC_PREFIX= "generic:";

// PARA(+) 且只有 1 个原子子节点：可合并进共享 LoroText 的「段落文本块」
static bool
is_simple_para (tree t) {
  return is_compound (t) && (int) L (t) == (int) moebius::PARA && N (t) == 1 &&
         is_atomic (t[0]);
}

loro_ir_node
tree_to_loro_ir (tree t) {
  loro_ir_node node;
  if (is_atomic (t)) {
    node.kind= LORO_ATOMIC;
    node.text= t->label;
    return node;
  }
  int n= N (t);
  if (is_compound (t)) {
    node.kind = LORO_COMPOUND;
    node.label= as_string (L (t));
  }
  else {
    node.kind = LORO_GENERIC;
    node.label= GENERIC_PREFIX * as_string ((int) L (t));
  }
  // 合并连续「简单 PARA」为 1 IR 原子 + splits：所有段落文本进同一 LoroText，
  // 段落边界用 marker 表达。跨段落 JOIN = 删 marker（字符身份不变）。
  // 同时保留原有的「连续裸原子」合并（CONCAT 场景）。
  int i= 0;
  while (i < n) {
    if (is_simple_para (t[i])) {
      string     merged= t[i][0]->label;
      array<int> splits;
      int        j= i + 1;
      while (j < n && is_simple_para (t[j])) {
        splits << N (merged);
        merged= merged * t[j][0]->label;
        j++;
      }
      loro_ir_node child;
      child.kind  = LORO_ATOMIC;
      child.text  = merged;
      child.splits= splits;
      node.children << child;
      i= j;
    }
    else if (is_atomic (t[i])) {
      string     merged= t[i]->label;
      array<int> splits;
      int        j= i + 1;
      while (j < n && is_atomic (t[j])) {
        splits << N (merged);
        merged= merged * t[j]->label;
        j++;
      }
      loro_ir_node child;
      child.kind  = LORO_ATOMIC;
      child.text  = merged;
      child.splits= splits;
      node.children << child;
      i= j;
    }
    else {
      node.children << tree_to_loro_ir (t[i]);
      i++;
    }
  }
  return node;
}

tree
loro_ir_to_tree (const loro_ir_node& node) {
  if (node.kind == LORO_ATOMIC) return tree (node.text);

  int op;
  if (node.kind == LORO_COMPOUND)
    op= (int) moebius::make_tree_label (node.label);
  else op= as_int (node.label (N (GENERIC_PREFIX), N (node.label)));

  // DOCUMENT 的 IR 原子子节点 = 合并段落文本块：按 splits 拆回 PARA。
  // 非 DOCUMENT（如 CONCAT）的 IR 原子子节点 = 合并裸原子：按 splits
  // 拆回裸原子。
  bool        wrap_para= (op == (int) moebius::DOCUMENT);
  array<tree> children;
  int         n= N (node.children);
  for (int i= 0; i < n; i++) {
    if (node.children[i].kind == LORO_ATOMIC) {
      string text = node.children[i].text;
      int    ns   = N (node.children[i].splits);
      int    start= 0;
      for (int j= 0; j <= ns; j++) {
        int end= (j < ns) ? node.children[i].splits[j] : N (text);
        if (wrap_para) {
          tree para ((tree_label) moebius::PARA, 1);
          para[0]= tree (text (start, end));
          children << para;
        }
        else {
          children << tree (text (start, end));
        }
        start= end;
      }
    }
    else children << loro_ir_to_tree (node.children[i]);
  }
  tree r (op, N (children));
  for (int i= 0; i < N (children); i++)
    r[i]= children[i];
  return r;
}
