/** \file loro_ir.cpp
 *  \copyright GPLv3
 *  \details 实现 tree <-> LoroIRNode 的依赖无关双向映射。
 *            标签走 moebius 的 name<->op 注册表：导出用 as_string(tree_label)
 * 取名字， 导入用 make_tree_label(name) 反查（含扩展标签）。generic
 * 节点（op<0） 用十进制存取（as_string 支持 int、as_int
 * 支持负号），保证忠实还原。
 *
 *            本文件不依赖 loro-c。后续 loro.cpp 的 FFI 薄层只需把 LoroIRNode
 * <-> LoroDoc 字节。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"

#include "tree_helper.hpp"

#include <moebius/tree_label.hpp>

#include <cstdint>

// generic 节点 label 前缀，op 以十进制存于其后
static const string GENERIC_PREFIX= "generic:";

loro_ir_node
tree_to_loro_ir (tree t) {
  loro_ir_node node;
  if (is_atomic (t)) {
    node.kind= LORO_ATOMIC;
    node.text= t->label;
  }
  else {
    int n= N (t);
    if (is_compound (t)) {
      node.kind = LORO_COMPOUND;
      node.label= as_string (L (t)); // op -> 标签名
    }
    else { // is_generic: op<0
      node.kind = LORO_GENERIC;
      node.label= GENERIC_PREFIX * as_string ((int) L (t));
    }
    for (int i= 0; i < n; i++)
      node.children << tree_to_loro_ir (t[i]);
  }
  return node;
}

tree
loro_ir_to_tree (loro_ir_node node) {
  if (node.kind == LORO_ATOMIC) return tree (node.text);

  int op;
  if (node.kind == LORO_COMPOUND)
    op= (int) moebius::make_tree_label (node.label); // 标签名 -> op（含扩展）
  else // LORO_GENERIC：label 形如 "generic:<op>"
    op= as_int (node.label (N (GENERIC_PREFIX), N (node.label)));

  int  n= N (node.children);
  tree r (op, n);
  for (int i= 0; i < n; i++)
    r[i]= loro_ir_to_tree (node.children[i]);
  return r;
}

/******************************************************************************
 * 扁平二进制编解码（与 3rdparty/mogan-loro-ffi/src/lib.rs 逐字节一致，小端）
 ******************************************************************************/

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
