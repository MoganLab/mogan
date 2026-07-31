/** \file loro_shadow_codec.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 *
 * body 经单条 LoroText（markup 流）编解码：to_tree = body markup → 线性 IR →
 * tree；import_and_build = import 远端 → to_tree 重建 buffer（body 无 TreeID，
 * id_map 留空）。
 */

#include "loro_shadow.hpp"
#include "tree_helper.hpp"

bool
loro_shadow_rep::import_and_build (string bytes, tree& out_buffer) {
  if (!import_data (bytes)) return false;
  out_buffer= to_tree ();
  return mogan_loro_body_len_utf8 (doc) > 0;
}

tree
loro_shadow_rep::to_tree () {
  string markup= body_markup ();
  if (N (markup) == 0) return tree ("");
  return linear_ir_to_tree (markup_to_linear_ir (markup));
}
