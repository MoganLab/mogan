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

#endif // defined LORO_H
