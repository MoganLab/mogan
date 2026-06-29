//
// Copyright (C) 2024-2026 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "s7.h"
#include <cstring>

namespace goldfish {

static s7_pointer
base_error (s7_scheme* sc, const char* kind, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, kind), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

// 返回 UTF-8 首字节 b 对应的码点字节宽度（1~4）；非法首字节返回 0
static inline int
utf8_seq_len (uint8_t b) {
  if (b < 0x80) return 1;
  if ((b & 0xE0) == 0xC0) return 2;
  if ((b & 0xF0) == 0xE0) return 3;
  if ((b & 0xF8) == 0xF0) return 4;
  return 0;
}

// 统计 UTF-8 字节序列 [buf, buf+byte_len) 中的字符数；同时定位第 char_index 个字符
// 的起始字节偏移（写入 *out_byte_pos）。返回字符总数；遇到非法序列返回 -1。
// 当 char_index >= 字符总数时，*out_byte_pos 写入 byte_len（指向末尾）。
static s7_int
utf8_count_and_locate (const uint8_t* buf, s7_int byte_len, s7_int char_index, s7_int* out_byte_pos) {
  s7_int i   = 0;
  s7_int cnt = 0;
  bool   done= false;

  while (i < byte_len) {
    if (!done && cnt == char_index) {
      if (out_byte_pos) *out_byte_pos= i;
      done= true;
    }
    int len= utf8_seq_len (buf[i]);
    if (len == 0 || i + len > byte_len) return -1;
    i+= len;
    cnt+= 1;
  }

  if (!done && out_byte_pos) *out_byte_pos= byte_len;
  return cnt;
}

static s7_pointer
f_string_to_utf8 (s7_scheme* sc, s7_pointer args) {
  s7_pointer arg= s7_car (args);

  if (!s7_is_string (arg)) {
    return base_error (sc, "type-error", "string->utf8: input must be string", arg);
  }

  const char* str     = s7_string (arg);
  s7_int      byte_len= s7_string_length (arg);

  // 解析可选参数 start / end。args 形如 (str start end ...)，缺省时按 Scheme 默认值处理：
  //   start 缺省 = 0；end 缺省 = #t（取到末尾）
  s7_pointer rest     = s7_cdr (args);
  s7_int     start    = 0;
  bool       end_given= false;
  s7_int     end_val  = 0;

  if (!s7_is_null (sc, rest)) {
    s7_pointer p1= s7_car (rest);
    if (!s7_is_integer (p1)) {
      return base_error (sc, "type-error", "string->utf8: start must be integer", p1);
    }
    start= s7_integer (p1);
    rest = s7_cdr (rest);

    if (!s7_is_null (sc, rest)) {
      s7_pointer p2= s7_car (rest);
      // Scheme 端 end 默认 #t 表示取到末尾；#t 按末尾处理
      if (s7_is_integer (p2)) {
        end_given= true;
        end_val  = s7_integer (p2);
      }
      else if (!s7_is_eq (p2, s7_t (sc))) {
        return base_error (sc, "type-error", "string->utf8: end must be integer", p2);
      }
    }
  }

  // 统计字符总数 N（同时无副作用地遍历校验 UTF-8 合法性）
  s7_int N= utf8_count_and_locate ((const uint8_t*) str, byte_len, 0, NULL);
  if (N < 0) {
    return base_error (sc, "value-error", "string->utf8: Invalid UTF-8 sequence", arg);
  }

  // start 校验：N>0 时 start 必须 [0, N)
  if (N > 0 && (start < 0 || start >= N)) {
    return base_error (sc, "out-of-range", "string->utf8: start out of range", arg);
  }
  // start<0 一律报错（覆盖 N==0 的情况）
  if (start < 0) {
    return base_error (sc, "out-of-range", "string->utf8: start out of range", arg);
  }

  // end 校验：[0, N]
  s7_int end_char;
  if (end_given) {
    if (end_val < 0 || end_val > N) {
      return base_error (sc, "out-of-range", "string->utf8: end out of range", arg);
    }
    end_char= end_val;
  }
  else {
    end_char= N;
  }

  // start > end
  if (start > end_char) {
    return base_error (sc, "out-of-range", "string->utf8: start > end", arg);
  }

  // start == end -> 空 bytevector
  if (start == end_char) {
    return s7_make_byte_vector (sc, 0, 1, NULL);
  }

  // 定位 start / end 对应的字节偏移
  s7_int start_byte= 0;
  s7_int end_byte  = 0;
  utf8_count_and_locate ((const uint8_t*) str, byte_len, start, &start_byte);
  utf8_count_and_locate ((const uint8_t*) str, byte_len, end_char, &end_byte);

  s7_int     out_len= end_byte - start_byte;
  s7_pointer out    = s7_make_byte_vector (sc, out_len, 1, NULL);
  memcpy (s7_byte_vector_elements (out), str + start_byte, out_len);
  return out;
}

static void
glue_string_to_utf8 (s7_scheme* sc) {
  const char* name= "g_string->utf8";
  const char* desc= "(g_string->utf8 str [start [end]]) => bytevector";
  s7_define_function (sc, name, f_string_to_utf8, 1, 0, true, desc);
}

void
glue_scheme_base (s7_scheme* sc) {
  glue_string_to_utf8 (sc);
}

} // namespace goldfish
