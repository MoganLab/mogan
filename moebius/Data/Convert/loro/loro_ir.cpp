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
  }
  else {
    int n= N (t);
    if (is_compound (t)) {
      node.kind = LORO_COMPOUND;
      node.label= as_string (L (t)); // op -> 标签名
    }
    else { // is_generic: op<0
      node.kind = LORO_GENERIC;
      node.label= GENERIC_PREFIX * as_string ((int) L (t));
    }
    // 处理子树
    for (int i= 0; i < n; i++)
      node.children << tree_to_loro_ir (t[i]);
  }
  return node;
}

tree
loro_ir_to_tree (const loro_ir_node& node) {
  if (node.kind == LORO_ATOMIC) return tree (node.text);

  int op; // operator，例如 \frac
  if (node.kind == LORO_COMPOUND)
    op= (int) moebius::make_tree_label (node.label); // 标签名 -> op（含扩展）
  else // LORO_GENERIC：label 形如 "generic:<op>"
    op= as_int (node.label (N (GENERIC_PREFIX), N (node.label)));

  // 处理子树
  int  n= N (node.children);
  tree r (op, n);
  for (int i= 0; i < n; i++)
    r[i]= loro_ir_to_tree (node.children[i]);
  return r;
}
