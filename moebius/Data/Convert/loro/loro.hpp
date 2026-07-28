/** \file loro.hpp
 *  \copyright GPLv3
 *  \details body 树 <-> Loro CRDT snapshot 的公开接口。
 *
 *            链路：tree --(loro_ir)--> loro_ir_node --(扁平编码)--> bytes
 *            --(mogan-loro-ffi)--> LoroDoc::export(snapshot)。反向同理。
 *            依赖 mogan-loro-ffi 静态库（见 3rdparty/mogan-loro-ffi）。
 *
 *            编译开关 LORO_ENABLED (xmake option loro)
 *  \author Jim Zhou
 *  \date   2026
 */

#ifndef LORO_H
#define LORO_H

#include "string.hpp"
#include "tree.hpp"
#include <cstddef>
#include <cstdint>

/******************************************************************************
 * doc API（全量树镜像）
 *
 * 对应 3rdparty/mogan-loro-ffi/src/lib.rs 的 extern "C"。
 *****************************************************************************/
// body 树 -> Loro CRDT snapshot 字节（包成 lolly string)。失败返回空串.
string tree_to_loro (tree t);

// Loro CRDT snapshot 字节 -> body 树。失败返回空树。
tree loro_to_tree (string snapshot);

/******************************************************************************
 * live-doc API（增量 op 镜像）
 *****************************************************************************/

/// TreeID 的 C 表示（与 Rust 侧 #[repr(C)] MoganTreeId 布局一致，按值跨界）
struct mogan_tree_id {
  uint64_t peer;
  int32_t  counter;
};

/// local-update 事件回调：本地编辑产生增量 delta 时回调 cb(user_data, bytes,
/// len)
typedef void (*mogan_local_update_cb) (void* user_data, const uint8_t* bytes,
                                       size_t len);

extern "C" {
// doc 句柄为不透明 void*（即 Rust 的 *mut LoroDoc）
void*   mogan_loro_doc_new ();
void    mogan_loro_doc_free (void* doc);
int32_t mogan_loro_doc_seed (void* doc, const uint8_t* ir, size_t ir_len);
int32_t mogan_loro_doc_export (void* doc, uint8_t** out, size_t* out_len);
int32_t mogan_loro_doc_to_ir (void* doc, uint8_t** out, size_t* out_len);
int32_t mogan_loro_doc_to_ir_with_ids (void* doc, uint8_t** out,
                                       size_t* out_len);
int32_t mogan_loro_doc_import (void* doc, const uint8_t* bytes, size_t len);
void    mogan_loro_doc_commit (void* doc);
// 导出"自上次本函数调用以来"的本地增量 update（内部推进 vv 水位），
// 用于 seed 后主动广播初始状态。
int32_t mogan_loro_doc_export_local_update (void* doc, uint8_t** out,
                                            size_t* out_len);
// 把 export vv 水位推进到当前（不导出字节），用于 import 远端数据后避免回传。
void mogan_loro_doc_advance_export_vv (void* doc);

// 在 LoroTree 下创建一个带 __section__=name 标签的新 root，把 IR 子树提升进该
// root（用于把 body 之外的文档部分作为独立 root 纳入同一 LoroDoc）。返回新 root
// 的 TreeID（失败返回 {0,0}）。
mogan_tree_id mogan_loro_doc_seed_section (void* doc, const uint8_t* name,
                                           size_t name_len, const uint8_t* ir,
                                           size_t ir_len);
// 从指定 root TreeID 读出 section 子树为 IR 扁平字节。
int32_t mogan_loro_doc_section_to_ir (void* doc, mogan_tree_id root_id,
                                      uint8_t** out, size_t* out_len);
// 枚举所有带 __section__ 标签的 root：输出若干条 (name_len:name, peer,
// counter)。
int32_t mogan_loro_doc_list_sections (void* doc, uint8_t** out,
                                      size_t* out_len);
int32_t mogan_loro_doc_on_local_update (void* doc, mogan_local_update_cb cb,
                                        void* user_data);
mogan_tree_id mogan_loro_node_create (void* doc, mogan_tree_id parent,
                                      uint32_t index, uint8_t kind,
                                      const uint8_t* label, size_t label_len);
int32_t       mogan_loro_node_delete (void* doc, mogan_tree_id id);
int32_t       mogan_loro_node_children (void* doc, mogan_tree_id parent,
                                        uint8_t** out, size_t* out_len);
int32_t       mogan_loro_node_mov (void* doc, mogan_tree_id target,
                                   mogan_tree_id parent, uint32_t index);
int32_t       mogan_loro_node_set_label (void* doc, mogan_tree_id id,
                                         const uint8_t* label, size_t len);
int32_t       mogan_loro_node_set_binary (void* doc, mogan_tree_id id,
                                          const uint8_t* bytes, size_t len);
int32_t mogan_loro_node_text_insert (void* doc, mogan_tree_id id, uint32_t pos,
                                     const uint8_t* bytes, size_t len);
int32_t mogan_loro_node_text_delete (void* doc, mogan_tree_id id, uint32_t pos,
                                     uint32_t len);
int32_t mogan_loro_node_join_text (void* doc, mogan_tree_id x_id,
                                   mogan_tree_id y_id);
// 稳定光标位置（Cursor，op-id 锚定）：把节点 LoroText 在 offset
// 处的稳定位置编码 为 postcard 字节；反向按当前 doc 解析为 unicode
// 偏移（并发编辑下自动跟随）。
int32_t mogan_loro_encode_cursor (void* doc, mogan_tree_id tree_id,
                                  uint32_t offset, uint8_t** out,
                                  size_t* out_len);
int32_t mogan_loro_decode_cursor (void* doc, const uint8_t* bytes, size_t len,
                                  uint32_t* out_offset);
void    mogan_loro_free (uint8_t* ptr, size_t len);
}

#endif // defined LORO_H
