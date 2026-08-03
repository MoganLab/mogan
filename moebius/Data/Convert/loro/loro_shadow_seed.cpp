/** \file loro_shadow_seed.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 *
 * body 用单条 LoroText（markup 流）：seed = tree → 线性 IR → markup →
 * body_seed。 body 节点不再有 TreeID 身份（id_map 留空；多光标身份由 Phase 4
 * Cursor 化补回）。
 */

#include "loro_ir.hpp"
#include "loro_shadow.hpp"
#include "tree_helper.hpp"

// body LoroText 的 markup 读写（FFI 包装）
string
loro_shadow_rep::body_markup () {
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_body_get_text (doc, &out, &out_len) != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return "";
  }
  string s ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return s;
}

void
loro_shadow_rep::body_seed_markup (string markup) {
  mogan_loro_body_seed (doc, reinterpret_cast<const uint8_t*> (markup.begin ()),
                        (size_t) N (markup));
}

void
loro_shadow_rep::seed (tree root) {
  id_map    = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  rev_id_map= hashmap<mogan_tree_id, path> (path ());
  root_id   = mogan_tree_id{0, 0};
  body_seed_markup (linear_ir_to_markup (tree_to_linear_ir (root)));
}

bool
loro_shadow_rep::sync_id_map_from_shadow (tree buffer) {
  (void) buffer; // body 无 TreeID 身份，无需按结构绑定
  id_map    = hashmap<tree_rep*, mogan_tree_id> (mogan_tree_id{0, 0});
  rev_id_map= hashmap<mogan_tree_id, path> (path ());
  root_id   = mogan_tree_id{0, 0};
  // body 非空 = shadow 已有远端内容（本端加入者复用，不另 seed 新根）
  return mogan_loro_body_len_utf8 (doc) > 0;
}
