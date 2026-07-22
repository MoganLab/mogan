/** \file loro_ir.hpp
 *  \copyright GPLv3
 *  \details 依赖无关的中层表示（IR）：把 moebius 文档树映射到一个中性的
 *            Loro 风格节点树，作为 tree <-> Loro CRDT 转换的核心。本层不依赖
 *            loro-ffi，可独立编译与 round-trip 测试.
 *  \author Jim Zhou
 *  \date   2026
 */

#ifndef LORO_IR_H
#define LORO_IR_H

#include "array.hpp"
#include "tree.hpp"

/**
 * Loro 风格的中性节点表示，对应一个未来的 LoroTree 节点。
 *
 * 数值必须与 3rdparty/mogan-loro-ffi/src/lib.rs 的 KIND_* 常量逐字一致
 * （扁平编码里 kind 直接以裸字节跨 FFI 传递）。
 *
 * - LORO_ATOMIC：对应 moebius 原子节点（op==0，字符串叶子）。text 存其内容。
 * - LORO_COMPOUND：对应 moebius 复合节点（op>0）。label 存标签名字字符串
 *   （如 "document"、"paragraph"），children 为有序子节点。
 * - LORO_GENERIC：对应 moebius generic 节点（op<0，罕见）。label 形如
 *   "generic:<op>"，children 为有序子节点。
 */
enum loro_node_kind { LORO_ATOMIC, LORO_COMPOUND, LORO_GENERIC };

struct loro_ir_node {
  loro_node_kind      kind;     // 节点类别
  string              label;    // COMPOUND: 标签名；GENERIC: "generic:<op>"
  string              text;     // ATOMIC: 字符串叶子内容
  array<loro_ir_node> children; // COMPOUND / GENERIC: 有序子节点
};

// moebius tree -> loro_ir_node
loro_ir_node tree_to_loro_ir (tree t);

// loro_ir_node -> moebius tree
tree loro_ir_to_tree (const loro_ir_node& node);

#endif // defined LORO_IR_H
