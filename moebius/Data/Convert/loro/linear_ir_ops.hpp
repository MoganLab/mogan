/** \file linear_ir_ops.hpp
 *  \copyright GPLv3
 *  \details 把结构 modification（SPLIT/JOIN/INSERT_NODE/REMOVE_NODE）直接作用于
 *            线性 IR，以最小 item 编辑完成，保留所有存活内容的身份（Phase 3 里
 *            Loro 字符 op-id 不变的纯逻辑基础）。语义与 moebius 的 clean_apply
 *            一致，可用
 *              linear_ir_to_tree(linear_ir_apply_mod(ir, mod)) ==
 * clean_apply(tree, mod) 校验。
 *
 *            编辑形式（均在 item 序列上插入/删除，不重建存活内容）：
 *            - SPLIT 原子：一个 TEXT 拆成 TEXT(head)+TEXT(tail)（markup 层即
 * SEP 边界）。
 *            - SPLIT 复合：在切分点插入 CLOSE + OPEN(同
 * label)，原子树一字不动。
 *            - JOIN  原子：合并两个相邻 TEXT 为一个。
 *            - JOIN  复合：删除中间的 CLOSE + OPEN 边界。
 *            - INSERT_NODE：用 OPEN/CLOSE 包裹目标，并把 wrapper
 * 的既有子节点就位。
 *            - REMOVE_NODE：删去 wrapper 的 OPEN/CLOSE，仅保留被提升的子节点。
 *
 *            本层不依赖 loro-ffi，可与 linear_ir 一起无 LORO_ENABLED 编译测试。
 *  \author Jim Zhou
 *  \date   2026
 */

#ifndef LINEAR_IR_OPS_H
#define LINEAR_IR_OPS_H

#include "linear_ir.hpp"
#include "modification.hpp"

/** 把一个结构 modification 作用于线性 IR（返回新的 item 序列）。
 *  ASSIGN 等非结构操作不在本次范围，原样返回 items 副本。 */
array<linear_item> linear_ir_apply_mod (array<linear_item> items,
                                        modification       mod);

#endif // LINEAR_IR_OPS_H
