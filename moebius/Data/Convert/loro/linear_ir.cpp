/** \file linear_ir.cpp
 *  \copyright GPLv3
 *
 * 实现 tree <-> 线性 IR <-> markup 文本的双向映射（统一包裹方案）。标签处理与
 * loro_ir.cpp 一致：导出用 as_string(L(t)) 取名字，导入用 make_tree_label(name)
 * 反查（含扩展），generic 节点（op<0）用 "generic:<op>" 前缀存取。
 *
 * markup 编码（进出单条 LoroText）：
 *   - 哨兵 ESC = \x01 引入一个 token，ESCAPE = \x02 作转义引入符。
 *   - token 形如 ESC <type> <payload> ESC，type 取 O/C/B：
 *       O<label>  OPEN(label)（原子用空 label）
 *       C         CLOSE
 *       B<base64> BINARY
 *   - 原子文本（TEXT）是 token 之间的裸字节，其中 \x01→\x02 '1'、\x02→\x02 '2'
 *     双写转义，故用户文本永不裸含 \x01，必在下一个 token 处终止。
 *   - label / base64 为有限 ASCII，不会含哨兵，无需转义。
 *   - 无 SEP：相邻原子各有 OPEN("")/CLOSE 边界，结构编辑统一为 CLOSE+OPEN
 * 增删。
 *
 *  \author Jim Zhou
 *  \date   2026
 */

#include "linear_ir.hpp"

#include "tree_helper.hpp"

#include <lolly/data/base64.hpp>
#include <moebius/tree_label.hpp>
#include <moebius/vars.hpp>

using lolly::data::decode_base64;
using lolly::data::encode_base64;

static const string GENERIC_PREFIX= "generic:";
static const char   ESC           = '\x01'; // token 引入符 / 终止符
static const char   ESCC          = '\x02'; // 转义引入符

/******************************************************************************
 * UTF-8 结构合法性（区分文本/二进制原子；与 Rust 侧 std::str::from_utf8 对齐）
 *****************************************************************************/
static bool
is_valid_utf8 (string s) {
  int i= 0, n= N (s);
  while (i < n) {
    unsigned char c= (unsigned char) s[i];
    int           extra;
    if (c < 0x80) extra= 0;
    else if ((c >> 5) == 0x06) extra= 1; // 110xxxxx
    else if ((c >> 4) == 0x0e) extra= 2; // 1110xxxx
    else if ((c >> 3) == 0x1e) extra= 3; // 11110xxx
    else return false;                   // 非法前导字节
    if (i + extra >= n) return false;    // 截断
    for (int j= 1; j <= extra; j++)
      if ((((unsigned char) s[i + j]) & 0xc0) != 0x80) return false; // 非续接
    i+= 1 + extra;
  }
  return true;
}

static string
label_of (tree t) {
  if (is_compound (t)) return as_string (L (t));
  return GENERIC_PREFIX * as_string ((int) L (t)); // generic: op<0
}

static int
op_of_label (string label) {
  if (N (label) >= N (GENERIC_PREFIX) &&
      label (0, N (GENERIC_PREFIX)) == GENERIC_PREFIX)
    return as_int (label (N (GENERIC_PREFIX), N (label)));
  if (N (label) == 0) return 0; // 原子（空 label）
  return (int) moebius::make_tree_label (label);
}

static bool
is_atomic_label (string label) {
  return N (label) == 0;
}

/******************************************************************************
 * tree -> 线性 IR（统一包裹）
 *****************************************************************************/
static void
emit_node (array<linear_item>& items, tree t) {
  linear_item open;
  open.kind = LI_OPEN;
  open.label= is_atomic (t) ? "" : label_of (t);
  items << open;
  if (is_atomic (t)) {
    linear_item content;
    if (is_valid_utf8 (t->label)) {
      content.kind= LI_TEXT;
      content.text= t->label;
    }
    else {
      content.kind= LI_BINARY;
      content.text= t->label;
    }
    items << content;
  }
  else {
    int n= N (t);
    for (int i= 0; i < n; i++)
      emit_node (items, t[i]);
  }
  linear_item close;
  close.kind= LI_CLOSE;
  items << close;
}

array<linear_item>
tree_to_linear_ir (tree t) {
  array<linear_item> items;
  emit_node (items, t);
  return items;
}

/******************************************************************************
 * 线性 IR -> tree
 *****************************************************************************/
struct li_frame {
  int         op;
  bool        atomic; // OPEN("") 原子框：仅收一条 TEXT/BINARY
  string      text;   // 原子框的文本
  bool        has_text;
  array<tree> children;
};

static void
attach (array<li_frame>& stack, array<tree>& top, tree node) {
  if (N (stack) > 0) stack[N (stack) - 1].children << node;
  else top << node;
}

static void
close_top (array<li_frame>& stack, array<tree>& top) {
  if (N (stack) == 0) return;
  li_frame f= stack[N (stack) - 1];
  stack->resize (N (stack) - 1);
  tree node;
  if (f.atomic) node= tree (f.text);
  else {
    node= tree (f.op, N (f.children));
    for (int j= 0; j < N (f.children); j++)
      node[j]= f.children[j];
  }
  attach (stack, top, node);
}

tree
linear_ir_to_tree (array<linear_item> items) {
  array<li_frame> stack;
  array<tree>     top;
  int             n= N (items);
  for (int i= 0; i < n; i++) {
    linear_item& it= items[i];
    if (it.kind == LI_OPEN) {
      li_frame f;
      f.atomic  = is_atomic_label (it.label);
      f.op      = op_of_label (it.label);
      f.has_text= false;
      stack << f;
    }
    else if (it.kind == LI_CLOSE) {
      close_top (stack, top); // 孤立 CLOSE（空栈）跳过
    }
    else if (it.kind == LI_TEXT || it.kind == LI_BINARY) {
      if (N (stack) > 0 && stack[N (stack) - 1].atomic) {
        // 收进原子框（已有则覆盖，自愈）
        stack[N (stack) - 1].text    = it.text;
        stack[N (stack) - 1].has_text= true;
      }
      else {
        // 裸原子内容（栈空或复合内）：视为即时原子子节点
        attach (stack, top, tree (it.text));
      }
    }
  }
  while (N (stack) > 0)
    close_top (stack, top); // 末尾自愈：未配对 OPEN 自动闭合
  if (N (top) == 0) return tree (moebius::DOCUMENT, 0);
  if (N (top) == 1) return top[0];
  tree doc (moebius::DOCUMENT, N (top));
  for (int j= 0; j < N (top); j++)
    doc[j]= top[j];
  return doc;
}

/******************************************************************************
 * 线性 IR -> markup
 *****************************************************************************/
static void
emit_escaped (string& out, string s) {
  int n= N (s);
  for (int i= 0; i < n; i++) {
    char c= s[i];
    if (c == ESC) {
      out << ESCC;
      out << '1';
    }
    else if (c == ESCC) {
      out << ESCC;
      out << '2';
    }
    else out << c;
  }
}

static void
emit_token (string& out, char type, string payload) {
  out << ESC;
  out << type;
  out << payload;
  out << ESC;
}

string
linear_ir_to_markup (array<linear_item> items) {
  string out;
  int    n= N (items);
  for (int i= 0; i < n; i++) {
    linear_item& it= items[i];
    if (it.kind == LI_OPEN) emit_token (out, 'O', it.label);
    else if (it.kind == LI_CLOSE) emit_token (out, 'C', "");
    else if (it.kind == LI_BINARY)
      emit_token (out, 'B', encode_base64 (it.text));
    else emit_escaped (out, it.text); // LI_TEXT
  }
  return out;
}

/******************************************************************************
 * markup -> 线性 IR
 *****************************************************************************/
array<linear_item>
markup_to_linear_ir (string s) {
  array<linear_item> items;
  int                n= N (s);
  int                i= 0;
  string             buf;
  bool               has_buf= false;
  while (i < n) {
    char c= s[i];
    if (c == ESC) {
      // token：ESC <type> <payload> ESC
      i++;
      if (i >= n) break;
      char type= s[i];
      i++;
      string payload;
      while (i < n && s[i] != ESC) {
        payload << s[i];
        i++;
      }
      if (i < n) i++; // 终止 ESC
      if (has_buf) {  // token 之前累积的裸文本
        linear_item it;
        it.kind= LI_TEXT;
        it.text= buf;
        items << it;
        has_buf= false;
        buf    = "";
      }
      if (type == 'O') {
        linear_item it;
        it.kind = LI_OPEN;
        it.label= payload;
        items << it;
      }
      else if (type == 'C') {
        linear_item it;
        it.kind= LI_CLOSE;
        items << it;
      }
      else if (type == 'B') {
        linear_item it;
        it.kind= LI_BINARY;
        it.text= decode_base64 (payload);
        items << it;
      }
      // 未知 type 忽略（前向兼容）
    }
    else if (c == ESCC) {
      // 转义：ESCAPE <code>
      i++;
      if (i >= n) break;
      char nx= s[i];
      i++;
      buf << (nx == '1' ? ESC : nx == '2' ? ESCC : nx);
      has_buf= true;
    }
    else {
      buf << c;
      i++;
      has_buf= true;
    }
  }
  if (has_buf) {
    linear_item it;
    it.kind= LI_TEXT;
    it.text= buf;
    items << it;
  }
  return items;
}
