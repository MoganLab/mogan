
/******************************************************************************
 * MODULE     : impl_typesetter.hpp
 * DESCRIPTION: Implementation of the main TeXmacs typesetting routines
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IMPL_TYPESETTER_H
#define IMPL_TYPESETTER_H
#include "Bridge/bridge.hpp"
#include "rectangles.hpp"
#include <memory>

class typesetter_rep {
public:
  edit_env&    env;
  bridge       br;
  array<brush> old_bgs;

  array<page_item> l;  // current lines
  stack_border     sb; // border properties
  array<line_item> a;  // left surroundings
  array<line_item> b;  // right surroundings

  DI                    x1, y1, x2, y2;
  hashmap<string, tree> old_patch;
  bool                  paper;

  // Progressive typesetting state for paginated long documents
  bool   progressive_enabled;        ///< 本文档是否启用流式排版
  int    progressive_cur_child;      ///< 排版前沿：已排到的正文子节点下标
  int    progressive_total_children; ///< 正文子节点总数
  bool   progressive_pending;        ///< 前沿未到文末，后台排版仍在进行
  bool   progressive_bench_active;   ///< 全生命周期基准测试是否已启动
  int    frozen_pages_positioned;    ///< 已执行 position_at 的成熟冻结页面数
  string doc_name;                   ///< 关联的文档路径（用于 benchmark）
  int    chunk_start;                ///< 本轮排版块的起点（= 上轮前沿）
  int    chunk_end;                  ///< 本轮排版块的终点（不含）
  array<page_item> doc_l;
  stack_border     doc_sb;
  array<box>       frozen_page_boxes;
  int              frozen_line_offset;
  int              dynamic_buffer_pages;
  DI               page_text_height;

private:
  std::shared_ptr<rectangle> changed_ptr= std::make_shared<rectangle> ();

public:
  typesetter_rep (edit_env& env, tree et, path ip);

  void insert_stack (array<page_item> l, stack_border sb);
  void insert_parunit (tree t, path ip);
  void insert_paragraph (tree t, path ip);
  void insert_surround (array<line_item> a, array<line_item> b);
  void insert_marker (tree st, path ip);

  void local_start (array<page_item>& l, stack_border& sb);
  void local_end (array<page_item>& l, stack_border& sb);

  void determine_page_references (box b);
  box  typeset ();
  box  typeset (DI& x1, DI& y1, DI& x2, DI& y2);

  /**
   * @brief 流式排版是否仍在进行（排版前沿未到文末）
   * @note  对 Scheme 暴露为 typeset-in-progress?
   */
  bool typeset_in_progress () const { return progressive_pending; }
  /**
   * @brief 将排版前沿向后推进若干子节点
   * @param additional_paragraphs 推进的子节点数
   */
  void progressive_advance (int additional_paragraphs);
  /**
   * @brief 将排版前沿快进到目标子节点附近（含缓冲余量）
   * @param target_child 目标正文子节点下标
   */
  void progressive_fast_forward_until (int target_child);
  /** @brief 清空流式排版累计状态，回到起点重排 */
  void progressive_reset ();
};

#endif // defined IMPL_TYPESETTER_H
