/** \file loro.hpp
 *  \copyright GPLv3
 *  \details body 树 <-> Loro CRDT snapshot 的公开接口。
 *
 *            链路：tree --(loro_ir)--> loro_ir_node --(扁平编码)--> bytes
 *            --(mogan-loro-ffi)--> LoroDoc::export(snapshot)。反向同理。
 *            依赖 mogan-loro-ffi 静态库（见 3rdparty/mogan-loro-ffi）。
 *
 *            编译开关 LORO_ENABLED（xmake option libloro）：
 *            - 开启：调用真实 FFI（mogan_loro_encode/decode），链
 * mogan-loro-ffi。
 *            - 关闭（默认）：tree_to_loro/loro_to_tree 为空桩，返回空，无 Rust
 * 依赖， 保证主构建不被未接好的 FFI 拖垮。
 *  \author Jim Zhou
 *  \date   2026
 */

#ifndef LORO_H
#define LORO_H

#include "string.hpp"
#include "tree.hpp"

/** @brief body 树 -> Loro CRDT snapshot 字节（包成 lolly
 * string）。失败返回空串。 */
string tree_to_loro (tree t);

/** @brief Loro CRDT snapshot 字节 -> body 树。失败返回空树。 */
tree loro_to_tree (string snapshot);

/******************************************************************************
 * live-doc API（Phase 2：增量 op 镜像）
 *
 * 仅在 LORO_ENABLED 下声明。对应 3rdparty/mogan-loro-ffi/src/lib.rs 的 extern
 *"C"。
 *****************************************************************************/
#ifdef LORO_ENABLED
#include <cstddef>
#include <cstdint>

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
void    mogan_loro_doc_advance_export_vv (void* doc);
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
void    mogan_loro_free (uint8_t* ptr, size_t len);
}
#endif // LORO_ENABLED

#endif // defined LORO_H
