/** \file loro_shadow_mod.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 *
 * 一个 modification 镜像到 body 单 LoroText（markup 流）。优先走
 * compute_markup_edit 给出的最小字节 splice（文本增删、SPLIT/JOIN——存活字符
 * op-id 不变）；不可精确 翻译的（ASSIGN / INSERT_NODE / REMOVE_NODE /
 * 复合子树增删）退化为整 body 重 seed， 保证 shadow 始终与 buffer 一致。
 */

#include "loro_shadow.hpp"
#include "tree_helper.hpp"

void
loro_shadow_rep::mirror_mod (tree doc_root, modification mod) {
  string             pre_mk= body_markup (); // 操作前 body 状态
  array<linear_item> pre   = markup_to_linear_ir (pre_mk);
  markup_edit        ed    = compute_markup_edit (pre, mod);
  if (ed.ok) {
    // 按 offset 降序应用各 splice（先高后低，使低 offset 不受高 offset
    // 编辑影响）
    array<markup_splice> ops= ed.ops;
    for (int i= 0; i < N (ops); i++)
      for (int j= i + 1; j < N (ops); j++)
        if (ops[j].offset > ops[i].offset) {
          markup_splice t= ops[i];
          ops[i]         = ops[j];
          ops[j]         = t;
        }
    for (int i= 0; i < N (ops); i++) {
      markup_splice& s= ops[i];
      if (s.delete_len > 0)
        mogan_loro_body_text_delete (doc, (size_t) s.offset,
                                     (size_t) s.delete_len);
      if (N (s.insert_bytes) > 0)
        mogan_loro_body_text_insert (
            doc, (size_t) s.offset,
            reinterpret_cast<const uint8_t*> (s.insert_bytes.begin ()),
            (size_t) N (s.insert_bytes));
    }
  }
  else {
    // coarse：整 body 按 post-edit buffer 重新 seed（仅 ASSIGN 等罕见情形；
    // 并发下会 clobber 对端编辑，常规结构操作均已精确翻译）
    body_seed_markup (linear_ir_to_markup (tree_to_linear_ir (doc_root)));
  }
  mogan_loro_doc_commit (doc);
}
