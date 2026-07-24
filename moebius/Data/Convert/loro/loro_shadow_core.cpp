/** \file loro_shadow_core.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 */

#include "basic.hpp"
#include "loro_shadow.hpp"

// tree_rep* 的 hash（hashmap key 用），仿 observer::hash((pointer)rep)
inline int
hash (tree_rep* p) {
  return hash ((pointer) p);
}

/******************************************************************************
 * 构造与析构
 ******************************************************************************/

loro_shadow_rep::loro_shadow_rep ()
    : doc (mogan_loro_doc_new ()), id_map (mogan_tree_id{0, 0}),
      root_id (mogan_tree_id{0, 0}), _update_cb (nullptr),
      _update_user_data (nullptr) {}

loro_shadow_rep::~loro_shadow_rep () {
  if (doc) mogan_loro_doc_free (doc);
}

loro_shadow::loro_shadow () : rep (tm_new<loro_shadow_rep> ()) {}

bool
loro_shadow_rep::has_id (tree t) {
  return id_map->contains (inside (t));
}

mogan_tree_id
loro_shadow_rep::get_id (tree t) {
  return id_map->contains (inside (t)) ? id_map (inside (t))
                                       : mogan_tree_id{0, 0};
}

path
loro_shadow_rep::cursor_path_of (mogan_tree_id id, int offset) {
  // O(1) 反查：rev_id_map 与 id_map 同处维护，始终反映当前 buffer。
  if (!rev_id_map->contains (id)) return path (); // 节点未同步到/已被删除
  return rev_id_map[id] * path (offset);          // 节点 path + 偏移
}

string
loro_shadow_rep::export_snapshot () {
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_doc_export (doc, &out, &out_len) != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return string ();
  }
  string s ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return s;
}

bool
loro_shadow_rep::import_data (string bytes) {
  return mogan_loro_doc_import (
             doc, reinterpret_cast<const uint8_t*> (bytes.begin ()),
             (size_t) N (bytes)) == 0;
}

void
loro_shadow_rep::on_local_update (mogan_local_update_cb cb, void* user_data) {
  _update_cb       = cb;
  _update_user_data= user_data;
  mogan_loro_doc_on_local_update (doc, cb, user_data);
}

void
loro_shadow_rep::broadcast_update () {
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  mogan_loro_doc_commit (doc);
  if (mogan_loro_doc_export_local_update (doc, &out, &out_len) == 0 &&
      out != nullptr && _update_cb != nullptr) {
    _update_cb (_update_user_data, out, out_len);
  }
  if (out) mogan_loro_free (out, out_len);
}

void
loro_shadow_rep::advance_export_vv () {
  mogan_loro_doc_advance_export_vv (doc);
}
