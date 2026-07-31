/** \file linear_ir_ops.hpp
 *  \copyright GPLv3
 *  \details 结构 modification ↔ 线性 IR / markup 字节编辑的纯逻辑层。
 *            linear_ir_apply_mod 按 clean_apply 语义等价变换 item 序列；
 *            compute_markup_edit 把 mod 翻译成 body LoroText 上的一条最小字节
 *            编辑（splice），是 Phase 3 mirror 的核心——offset 正确性可独立
 *            round-trip 测试，无需 Loro。统一包裹方案下，SPLIT/JOIN（原子与
 *            复合）同为 CLOSE+OPEN token 的插入/删除，存活字符 op-id 不变。
 *  \author Jim Zhou
 *  \date   2026
 */

#ifndef LINEAR_IR_OPS_H
#define LINEAR_IR_OPS_H

#include "linear_ir.hpp"
#include "modification.hpp"

/** 把一个结构 modification 作用于线性 IR（按 clean_apply 语义等价返回新序列）。
 *  覆盖 SPLIT/JOIN/INSERT_NODE/REMOVE_NODE；其余原样返回。 */
array<linear_item> linear_ir_apply_mod (array<linear_item> items,
                                        modification       mod);

/** 一条 markup 字节 splice 编辑（相对操作前 markup 的 utf-8 字节坐标）：
 *  在 offset 处删除 delete_len 字节，再插入 insert_bytes。
 *  ok=false 表示无可精确计算的编辑（复合子树插入/删除、INSERT_NODE/REMOVE_NODE/
 *  ASSIGN），调用方应 coarse 重 seed。 */
struct markup_edit {
  bool   ok;
  int    offset;
  int    delete_len;
  string insert_bytes;
};

/** 把 modification 翻译成 body LoroText 上的一条最小字节 splice（基于操作前
 *  items）。覆盖：文本 INSERT/REMOVE、SPLIT（原子/复合）、JOIN（原子/复合）。
 *  统一方案下 SPLIT=插 CLOSE+OPEN(label)、JOIN=删 CLOSE+OPEN(label)。其余返回
 *  ok=false。 */
markup_edit compute_markup_edit (array<linear_item> items, modification mod);

#endif // LINEAR_IR_OPS_H
