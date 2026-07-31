/** \file linear_ir.hpp
 *  \copyright GPLv3
 *  \details 线性文档流 IR：把 moebius 文档树展开成一条线性的 item 序列，作为
 *            body ↔ Loro 单条 LoroText 标记流转换的核心。**统一包裹**：每个节点
 *            （复合/泛型/原子）都表示为 OPEN(label) ... CLOSE；原子的 label
 * 为空， 其间恰好一条 TEXT/BINARY。这样相邻原子各有显式 OPEN/CLOSE 边界， 无需
 * SEP 分隔符——结构编辑（SPLIT/JOIN/INSERT_NODE/REMOVE_NODE） 统一退化为
 * CLOSE+OPEN token 的插入/删除，存活字符 op-id 不变。 本层不依赖
 * loro-ffi，可独立编译与 round-trip 测试。
 *  \author Jim Zhou
 *  \date   2026
 */

#ifndef LINEAR_IR_H
#define LINEAR_IR_H

#include "array.hpp"
#include "string.hpp"
#include "tree.hpp"

/**
 * 线性 item 类别。
 * - LI_OPEN / LI_CLOSE：开启/关闭一个节点。OPEN 的 label：复合为标签名、泛型为
 *   "generic:<op>"、**原子为空串**。原子节点形如 OPEN("") TEXT(...) CLOSE。
 * - LI_TEXT：原子文本内容（合法 UTF-8），text 存原文。
 * - LI_BINARY：原子二进制（图片等非法 UTF-8），text 存原始字节。
 */
enum linear_item_kind { LI_OPEN, LI_CLOSE, LI_TEXT, LI_BINARY };

struct linear_item {
  linear_item_kind kind;
  string           label; // OPEN: 标签名 / "generic:<op>" / 空（atomic）
  string           text;  // TEXT: 原文；BINARY: 原始字节
};

/** moebius tree -> 线性 IR（先序展开）。每个节点 → OPEN(label) ...children...
 *  CLOSE；原子 → OPEN("") TEXT/BINARY CLOSE。 */
array<linear_item> tree_to_linear_ir (tree t);

/** 线性 IR -> moebius tree（栈式解析）。OPEN("") 视为原子。解析器自愈：未配对
 *  OPEN 末尾补 CLOSE、孤立 CLOSE 跳过、栈空时裸 TEXT 视为根原子。空序列返回
 *  空 (document)。 */
tree linear_ir_to_tree (array<linear_item> items);

/** 线性 IR -> markup 文本（\x01 哨兵 + \x02 转义），用于进出单条 LoroText。 */
string linear_ir_to_markup (array<linear_item> items);

/** markup 文本 -> 线性 IR（自愈解析，与 linear_ir_to_markup 互逆）。 */
array<linear_item> markup_to_linear_ir (string s);

#endif // LINEAR_IR_H
