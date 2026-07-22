/** \file loro_ir_codec_test.cpp
 *  \copyright GPLv3
 *  \details loro_ir_node <-> bytes (string) 往返测试。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro_ir.hpp"
#include "loro_ir_codec.hpp"
#include "moe_doctests.hpp"
#include <moebius/vars.hpp>

using namespace moebius;

#ifdef LORO_ENABLED

TEST_CASE ("loro_ir_codec: flat encode/decode self round-trip + bytes") {
  // (compound "document" (atomic "hi") (atomic "x"))
  loro_ir_node orig;
  orig.kind = LORO_COMPOUND;
  orig.label= "document";
  loro_ir_node c0;
  c0.kind= LORO_ATOMIC;
  c0.text= "hi";
  loro_ir_node c1;
  c1.kind= LORO_ATOMIC;
  c1.text= "x";
  orig.children << c0;
  orig.children << c1;

  string bytes= loro_ir_encode (orig);
  // 打印字节（十进制），用于与 Rust 侧 Writer 输出逐字节比对格式
  cout << "[codec] " << N (bytes) << " bytes:";
  for (int i= 0; i < N (bytes); i++)
    cout << " " << (int) (unsigned char) bytes[i];
  cout << LF;

  loro_ir_node back= loro_ir_decode (bytes);
  CHECK_EQ (back.kind == LORO_COMPOUND, true);
  CHECK_EQ (back.label == "document", true);
  CHECK_EQ (N (back.children) == 2, true);
  CHECK_EQ (back.children[0].text == "hi", true);
  CHECK_EQ (back.children[1].text == "x", true);
}

TEST_CASE ("loro_ir_codec: flat encode/decode atomic node") {
  loro_ir_node orig;
  orig.kind= LORO_ATOMIC;
  orig.text= "hello";

  string       bytes= loro_ir_encode (orig);
  loro_ir_node back = loro_ir_decode (bytes);
  CHECK_EQ (back.kind == LORO_ATOMIC, true);
  CHECK_EQ (back.text == "hello", true);
}

TEST_CASE ("loro_ir_codec: generic node encode/decode") {
  loro_ir_node orig;
  orig.kind = LORO_GENERIC;
  orig.label= "generic:-99";
  loro_ir_node c;
  c.kind= LORO_ATOMIC;
  c.text= "test";
  orig.children << c;

  string       bytes= loro_ir_encode (orig);
  loro_ir_node back = loro_ir_decode (bytes);
  CHECK_EQ (back.kind == LORO_GENERIC, true);
  CHECK_EQ (back.label == "generic:-99", true);
  CHECK_EQ (N (back.children) == 1, true);
  CHECK_EQ (back.children[0].text == "test", true);
}

TEST_CASE ("loro_ir_codec: deeply nested node encode/decode") {
  loro_ir_node root;
  root.kind = LORO_COMPOUND;
  root.label= "document";

  loro_ir_node l1;
  l1.kind = LORO_COMPOUND;
  l1.label= "para";

  loro_ir_node l2;
  l2.kind= LORO_ATOMIC;
  l2.text= "deep";

  l1.children << l2;
  root.children << l1;

  string       bytes= loro_ir_encode (root);
  loro_ir_node back = loro_ir_decode (bytes);

  CHECK_EQ (back.kind == LORO_COMPOUND, true);
  CHECK_EQ (back.label == "document", true);
  CHECK_EQ (N (back.children) == 1, true);
  CHECK_EQ (back.children[0].kind == LORO_COMPOUND, true);
  CHECK_EQ (back.children[0].label == "para", true);
  CHECK_EQ (N (back.children[0].children) == 1, true);
  CHECK_EQ (back.children[0].children[0].kind == LORO_ATOMIC, true);
  CHECK_EQ (back.children[0].children[0].text == "deep", true);
}

TEST_CASE ("loro_ir_codec: empty compound node") {
  loro_ir_node orig;
  orig.kind = LORO_COMPOUND;
  orig.label= "empty";

  string       bytes= loro_ir_encode (orig);
  loro_ir_node back = loro_ir_decode (bytes);
  CHECK_EQ (back.kind == LORO_COMPOUND, true);
  CHECK_EQ (back.label == "empty", true);
  CHECK_EQ (N (back.children) == 0, true);
}

TEST_CASE ("loro_ir_codec: empty string atomic node") {
  loro_ir_node orig;
  orig.kind= LORO_ATOMIC;
  orig.text= "";

  string       bytes= loro_ir_encode (orig);
  loro_ir_node back = loro_ir_decode (bytes);
  CHECK_EQ (back.kind == LORO_ATOMIC, true);
  CHECK_EQ (back.text == "", true);
}

#endif // LORO_ENABLED
