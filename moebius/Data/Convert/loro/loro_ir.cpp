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
  // 合并连续裸原子为 1 IR 原子 + splits。
  // 不合并 PARA-wrapped 原子：IR 无法区分 DOCUMENT 下是裸原子还是 PARA，
  // 强行合并+包装会破坏 round-trip。
  int i= 0;
  while (i < n) {
    if (is_atomic (t[i])) {
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

  // 物化：原子子节点有 splits 时展开为 N+1 个裸原子直接子节点。
  array<tree> children;
  int         n= N (node.children);
  for (int i= 0; i < n; i++) {
    if (node.children[i].kind == LORO_ATOMIC &&
        N (node.children[i].splits) > 0) {
      string text = node.children[i].text;
      int    start= 0;
      for (int j= 0; j < N (node.children[i].splits); j++) {
        children << tree (text (start, node.children[i].splits[j]));
        start= node.children[i].splits[j];
      }
      children << tree (text (start, N (text)));
    }
    else children << loro_ir_to_tree (node.children[i]);
  }
  tree r (op, N (children));
  for (int i= 0; i < N (children); i++)
    r[i]= children[i];
  return r;
}
