/** \file loro_ir_codex.hpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 * 扁平二进制编解码
 *
 * 格式（小端序）：
 *   node := kind:u8  label_len:u32 label:bytes  text_len:u32 text:bytes
 *           n_children:u32  node × n_children
 */

#ifndef LORO_IR_CODEC_H
#define LORO_IR_CODEC_H

#include "loro_ir.hpp"
#include "string.hpp"

string       loro_ir_encode (loro_ir_node node);
loro_ir_node loro_ir_decode (string bytes);

#endif // LORO_IR_CODEC_H