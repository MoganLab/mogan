/** \file loro_ir_codex.cpp
 *  \copyright GPLv3
 *  \author Jim Zhou
 *  \date   2026
 *
 * 扁平二进制编解码
 *
 * 将 loro_ir_node 编码为紧凑二进制字符串，用于跨语言传输。
 *
 * 编码格式：
 *
 *   Node {
 *     uint8   kind;              // 节点类型
 *     string  label;             // label，长度由前缀编码
 *     string  text;              // atomic 文本，长度由前缀编码
 *     uint32  child_count;       // 子节点数量
 *     Node[]  children;          // 递归编码子节点
 *   }
 *
 * 字符串格式：
 *
 *   uint32 length;               // 字符串字节长度，小端序
 *   char[] data;                 // 原始字符串数据
 *
 * 整数统一采用 little-endian 编码，保证 C++ 与 Rust/WASM 等环境
 * 之间的数据表示一致。
 *
 * 后续 FFI 层只需传递该二进制数据即可。
 */

#include "loro_ir.hpp"
#include "string.hpp"

static void
put_u32 (string& s, uint32_t v) {
  s << (char) (v & 0xff);
  s << (char) ((v >> 8) & 0xff);
  s << (char) ((v >> 16) & 0xff);
  s << (char) ((v >> 24) & 0xff);
}

static void
put_str (string& s, string t) {
  int n= N (t);
  put_u32 (s, (uint32_t) n);
  for (int i= 0; i < n; i++)
    s << t[i];
}

static void
encode_node_into (string& s, loro_ir_node node) {
  s << (char) node.kind;
  put_str (s, node.label);
  put_str (s, node.text);
  int n= N (node.children);
  put_u32 (s, (uint32_t) n);
  for (int i= 0; i < n; i++)
    encode_node_into (s, node.children[i]);
}

string
loro_ir_encode (loro_ir_node node) {
  string s;
  encode_node_into (s, node);
  return s;
}

static uint32_t
get_u32_dec (string& b, int& pos) {
  uint32_t v= (uint32_t) (unsigned char) b[pos] |
              ((uint32_t) (unsigned char) b[pos + 1] << 8) |
              ((uint32_t) (unsigned char) b[pos + 2] << 16) |
              ((uint32_t) (unsigned char) b[pos + 3] << 24);
  pos+= 4;
  return v;
}

static string
get_str_dec (string& b, int& pos) {
  uint32_t n= get_u32_dec (b, pos);
  string   r;
  for (uint32_t i= 0; i < n; i++)
    r << b[pos + i];
  pos+= n;
  return r;
}

static loro_ir_node
decode_node_from (string& b, int& pos) {
  loro_ir_node node;
  node.kind = (loro_node_kind) (unsigned char) b[pos++];
  node.label= get_str_dec (b, pos);
  node.text = get_str_dec (b, pos);
  uint32_t n= get_u32_dec (b, pos);
  for (uint32_t i= 0; i < n; i++)
    node.children << decode_node_from (b, pos);
  return node;
}

loro_ir_node
loro_ir_decode (string bytes) {
  int pos= 0;
  return decode_node_from (bytes, pos);
}