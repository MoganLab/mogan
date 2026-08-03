/** \file linear_ir.hpp
 *  \copyright GPLv3
 *  \details 线性文档流 IR：把 moebius 文档树展开成一条线性的 item 序列，作为
 *            body ↔ Loro 单条 LoroText 标记流转换的核心。复合/泛型节点以
 *            OPEN/CLOSE 包裹，原子以裸 TEXT/BINARY 表示，结构操作以 MARKER
 *            （当前仅 SPLIT）表示。本层不依赖 loro-ffi，可独立编译与
 * round-trip。
 *
 *            设计目标：SPLIT/JOIN/INSERT_NODE/REMOVE_NODE 退化为 item 序列上的
 *            插入/删除，保留所有存活内容的身份（Loro 字符 op-id）。例如
 *            (para "hello") 在 offset 3 处 split 后线性 IR 为：
 *              [OPEN(para), TEXT("hel"), MARKER(SPLIT), TEXT("lo"), CLOSE]
 *            仅在 "hel"/"lo" 之间插入一个 marker，原字符一字不动。
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
 * - LI_OPEN / LI_CLOSE：开启/关闭一个复合或泛型节点（OPEN 的 label 存标签名或
 *   "generic:<op>"）。
 * - LI_TEXT：原子文本内容（合法 UTF-8），text 存原文。
 * - LI_BINARY：原子二进制（图片等非法 UTF-8），text 存原始字节。
 * - LI_MARKER：结构标记（marker 存子类型）。LM_SPLIT 表示「关闭当前最内层
 *   复合节点，并以相同 label 开新兄弟」——段落切分即此。
 *
 * 原子以裸 TEXT/BINARY 表示（不额外包裹 OPEN/CLOSE），同一复合内的相邻原子
 * 即多个 atomic 子节点（如 (concat "a" "b")）。
 */
enum linear_item_kind { LI_OPEN, LI_CLOSE, LI_TEXT, LI_BINARY, LI_MARKER };
enum linear_marker_kind { LM_SPLIT };

struct linear_item {
  linear_item_kind   kind;
  string             label;  // OPEN: 标签名 / "generic:<op>"
  string             text;   // TEXT: 原文；BINARY: 原始字节
  linear_marker_kind marker; // MARKER 子类型
};

/** moebius tree -> 线性 IR（先序展开）。复合/泛型 → OPEN ...children... CLOSE；
 *  原子 → 裸 TEXT（合法 UTF-8）或 BINARY（否则）。根原子产出单条 TEXT/BINARY。
 */
array<linear_item> tree_to_linear_ir (tree t);

/** 线性 IR -> moebius tree（栈式解析）。MARKER(SPLIT) 关闭当前最内层复合并以
 *  相同 label 重开兄弟。解析器自愈：未配对 OPEN 末尾自动补 CLOSE、孤立 CLOSE
 *  跳过、栈空时裸 TEXT 视为根原子。空序列返回空 (document)。 */
tree linear_ir_to_tree (array<linear_item> items);

/** 线性 IR -> markup 文本（\x01 哨兵 + \x02 转义），用于进出单条 LoroText。
 *  相邻原子之间自动插入 SEP 哨兵以免文本相连。 */
string linear_ir_to_markup (array<linear_item> items);

/** markup 文本 -> 线性 IR（自愈解析，与 linear_ir_to_markup 互逆）。 */
array<linear_item> markup_to_linear_ir (string s);

#endif // LINEAR_IR_H
