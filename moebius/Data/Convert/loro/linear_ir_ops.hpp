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

/** 一条 markup 字节 splice（相对操作前 markup 的 utf-8 字节坐标）：在 offset
 *  处先删 delete_len 字节，再插 insert_bytes。 */
struct markup_splice {
  int    offset;
  int    delete_len;
  string insert_bytes;
};

/** 一个 modification 对应的 markup 编辑（一到多条 splice，须按 offset
 * 降序应用， 使低 offset 不受高 offset 编辑影响）。ok=false
 * 表示无可精确计算的编辑（复合 子树增删的某些畸形形态、ASSIGN），调用方应
 * coarse 重 seed。 注意：coarse 重 seed = 清空 body + 重灌，会 clobber
 * 并发的对端编辑，故常规 结构操作必须精确翻译。 */
struct markup_edit {
  bool                 ok;
  array<markup_splice> ops;
};

/** 把 modification 翻译成 body LoroText 上的最小字节 splice 序列（基于操作前
 *  items）。覆盖：文本 INSERT/REMOVE、SPLIT（原子/复合）、JOIN（原子/复合）、
 *  INSERT_NODE（包裹：插 OPEN+CLOSE）、REMOVE_NODE（脱壳：删 OPEN+CLOSE）。
 *  ASSIGN 等返回 ok=false。 */
markup_edit compute_markup_edit (array<linear_item> items, modification mod);

/** body markup 字节偏移 ↔ buffer 树位置（Phase 4 光标映射的纯逻辑核心）。
 *  atomic_path 指向某原子节点（其 OPEN('')），char_off 为其内字符偏移；返回该
 *  位置在 markup 中的 utf-8 字节偏移，失败 -1。 */
int linear_ir_offset_of_atomic (array<linear_item> items, path atomic_path,
                                int char_off);

/** body markup 字节偏移 -> 树位置路径（原子路径 * 字符偏移）。落在原子内容外
 *  （token 区）时就近吸附到相邻原子；无原子返回 nil。 */
path linear_ir_path_at_offset (array<linear_item> items, int byte_off);

int  linear_ir_text_index_of_offset (const array<linear_item>& items,
                                     int byte_off, bool& prefer_start);
path linear_ir_path_at_text_index (const array<linear_item>& items,
                                   int                       target_text_idx,
                                   bool prefer_start= false);

#endif // LINEAR_IR_OPS_H
