/** \file loro.cpp
 *  \copyright GPLv3
 *  \details tree <-> Loro CRDT snapshot 的 FFI 胶水层。body 用单条 LoroText
 *            （markup 流）：tree → 线性 IR → markup → body_seed → snapshot；
 *            反向 snapshot → import → body_get_text → markup → tree。
 *            LORO_ENABLED 关时为空桩（见 loro.hpp）。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro.hpp"

#include "linear_ir.hpp"

#include <cstddef>
#include <cstdint>

string
tree_to_loro (tree t) {
  void* doc= mogan_loro_doc_new ();
  if (doc == nullptr) return string ();
  string markup= linear_ir_to_markup (tree_to_linear_ir (t));
  mogan_loro_body_seed (doc, reinterpret_cast<const uint8_t*> (markup.begin ()),
                        (size_t) N (markup));
  mogan_loro_doc_commit (doc);
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  string   snapshot;
  if (mogan_loro_doc_export (doc, &out, &out_len) == 0 && out != nullptr) {
    snapshot= string ((const char*) out, (int) out_len);
    mogan_loro_free (out, out_len);
  }
  mogan_loro_doc_free (doc);
  return snapshot;
}

tree
loro_to_tree (string snapshot) {
  void* doc= mogan_loro_doc_new ();
  if (doc == nullptr) return tree ("");
  mogan_loro_doc_import (doc,
                         reinterpret_cast<const uint8_t*> (snapshot.begin ()),
                         (size_t) N (snapshot));
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  tree     r ("");
  if (mogan_loro_body_get_text (doc, &out, &out_len) == 0 && out != nullptr) {
    string markup ((const char*) out, (int) out_len);
    mogan_loro_free (out, out_len);
    if (N (markup) > 0) r= linear_ir_to_tree (markup_to_linear_ir (markup));
  }
  mogan_loro_doc_free (doc);
  return r;
}
