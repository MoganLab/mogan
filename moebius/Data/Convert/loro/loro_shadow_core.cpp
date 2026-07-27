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

path
loro_shadow_rep::node_path_of (mogan_tree_id id) {
  return rev_id_map->contains (id) ? rev_id_map[id] : path ();
}

// 字节串 <-> hex（Cursor 的 postcard 字节需文本帧传输，hex 无空格/冒号）
static string
bytes_to_hex (string b) {
  string r;
  for (int i= 0; i < N (b); i++) {
    unsigned char c = (unsigned char) b[i];
    int           hi= (c >> 4) & 0xf, lo= c & 0xf;
    r << (char) (hi < 10 ? '0' + hi : 'a' + hi - 10);
    r << (char) (lo < 10 ? '0' + lo : 'a' + lo - 10);
  }
  return r;
}

static string
hex_to_bytes (string h) {
  string r;
  int    n= N (h);
  for (int i= 0; i + 1 < n; i+= 2) {
    auto nib= [] (char c) -> int {
      return (c >= '0' && c <= '9')   ? c - '0'
             : (c >= 'a' && c <= 'f') ? c - 'a' + 10
             : (c >= 'A' && c <= 'F') ? c - 'A' + 10
                                      : 0;
    };
    r << (char) ((nib (h[i]) << 4) | nib (h[i + 1]));
  }
  return r;
}

string
loro_shadow_rep::encode_cursor_hex (mogan_tree_id id, int offset) {
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  if (mogan_loro_encode_cursor (doc, id, (uint32_t) offset, &out, &out_len) !=
          0 ||
      out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return "";
  }
  string r ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return bytes_to_hex (r);
}

int
loro_shadow_rep::decode_cursor_hex (string hex) {
  string bytes= hex_to_bytes (hex);
  if (N (bytes) == 0) return -1;
  uint32_t off= 0;
  if (mogan_loro_decode_cursor (
          doc, reinterpret_cast<const uint8_t*> (bytes.begin ()),
          (size_t) N (bytes), &off) != 0)
    return -1; // 容器消失/历史 GC/id 找不到 → 调用方丢弃
  return (int) off;
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
