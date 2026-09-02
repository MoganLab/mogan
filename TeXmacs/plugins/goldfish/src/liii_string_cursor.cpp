//
// Copyright (C) 2026 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// (liii string-cursor) 的核心游标原语的 C++ 实现
//
// 游标表示：负整数 -(byte_offset+2)，即字节 0 对应 -2。-1 不是合法游标，
// 保留给"负索引"错误语义（见 validate-start-end）。
// 与字符索引（非负整数）天然不相交，满足 SRFI 130 的要求，
// 且比较、前进、后退都是 O(1) 或 O(字符宽度) 的整数/字节运算，无分配。

#include "s7.h"
#include <cstdint>
#include <cstring>

namespace goldfish {

static s7_pointer
liii_string_cursor_type_error (s7_scheme* sc, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, "type-error"), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

static s7_pointer
liii_string_cursor_value_error (s7_scheme* sc, const char* msg) {
  return s7_error (sc, s7_make_symbol (sc, "value-error"), s7_list (sc, 1, s7_make_string (sc, msg)));
}

// 返回 UTF-8 首字节 b 对应的码点字节宽度（1~4）；非法首字节返回 1
static inline s7_int
utf8_seq_len (uint8_t b) {
  if (b < 0x80) return 1;
  if ((b & 0xE0) == 0xC0) return 2;
  if ((b & 0xF0) == 0xE0) return 3;
  if ((b & 0xF8) == 0xF0) return 4;
  return 1;
}

// 游标 <-> 字节偏移
static inline s7_int
cursor_to_offset (s7_pointer cursor) {
  return -s7_integer (cursor) - 2;
}

static inline s7_pointer
offset_to_cursor (s7_scheme* sc, s7_int offset) {
  return s7_make_integer (sc, -offset - 2);
}

// 校验第一个参数是字符串；失败时抛 type-error 并返回 NULL
static const char*
check_string_arg (s7_scheme* sc, s7_pointer arg, const char* who) {
  if (!s7_is_string (arg)) {
    liii_string_cursor_type_error (sc, who, arg);
    return NULL;
  }
  return s7_string (arg);
}

// 校验 cursor 参数：负整数（游标）或非负整数（索引）；返回 true 表示是游标
static bool
parse_cursor_arg (s7_scheme* sc, s7_pointer arg, const char* who, s7_int* value) {
  if (!s7_is_integer (arg)) {
    liii_string_cursor_type_error (sc, who, arg);
    return false;
  }
  *value= s7_integer (arg);
  return *value < -1;
}

// ---- string-cursor-start / string-cursor-end ----

static s7_pointer
f_string_cursor_start (s7_scheme* sc, s7_pointer args) {
  s7_pointer str= s7_car (args);
  if (!check_string_arg (sc, str, "string-cursor-start: first parameter must be string")) return NULL;
  return s7_make_integer (sc, -2);
}

static s7_pointer
f_string_cursor_end (s7_scheme* sc, s7_pointer args) {
  s7_pointer str= s7_car (args);
  if (!check_string_arg (sc, str, "string-cursor-end: first parameter must be string")) return NULL;
  return offset_to_cursor (sc, (s7_int) s7_string_length (str));
}

// ---- 字节偏移上的前进/后退 ----

static s7_int
utf8_advance (const char* s, s7_int off) {
  return off + utf8_seq_len ((uint8_t) s[off]);
}

static s7_int
utf8_retreat (const char* s, s7_int off) {
  do {
    off--;
  } while (off > 0 && ((s[off] & 0xC0) == 0x80));
  return off;
}

// ---- string-cursor-next / string-cursor-prev ----

static s7_pointer
f_string_cursor_next (s7_scheme* sc, s7_pointer args) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, "string-cursor-next: first parameter must be string");
  if (!s) return NULL;
  s7_int cur;
  bool   is_cursor=
      parse_cursor_arg (sc, s7_cadr (args), "string-cursor-next: second parameter must be integer or cursor", &cur);
  s7_int len= (s7_int) s7_string_length (str);

  if (!is_cursor) {
    // 索引语义：返回索引 +1
    if (cur < 0 || cur >= len) return liii_string_cursor_value_error (sc, "string-cursor-next: already at end cursor");
    return s7_make_integer (sc, cur + 1);
  }
  s7_int off= cursor_to_offset (s7_car (s7_cdr (args)));
  if (off >= len) return liii_string_cursor_value_error (sc, "string-cursor-next: already at end cursor");
  return offset_to_cursor (sc, utf8_advance (s, off));
}

static s7_pointer
f_string_cursor_prev (s7_scheme* sc, s7_pointer args) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, "string-cursor-prev: first parameter must be string");
  if (!s) return NULL;
  s7_pointer cur_arg= s7_cadr (args);
  s7_int     cur;
  bool       is_cursor=
      parse_cursor_arg (sc, cur_arg, "string-cursor-prev: second parameter must be integer or cursor", &cur);
  if (is_cursor) {
    s7_int off= cursor_to_offset (cur_arg);
    if (off <= 0) return liii_string_cursor_value_error (sc, "string-cursor-prev: already at start cursor");
    return offset_to_cursor (sc, utf8_retreat (s, off));
  }
  if (cur <= 0) return liii_string_cursor_value_error (sc, "string-cursor-prev: already at start cursor");
  return s7_make_integer (sc, cur - 1);
}

// ---- string-cursor-forward / string-cursor-back ----

static s7_pointer
string_cursor_move (s7_scheme* sc, s7_pointer args, bool forward, const char* who) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, who);
  if (!s) return NULL;
  s7_pointer cur_arg= s7_cadr (args);
  s7_int     cur, nchars;
  bool       is_cursor= parse_cursor_arg (sc, cur_arg, who, &cur);
  s7_pointer n_arg    = s7_cadr (s7_cdr (args));
  if (!s7_is_integer (n_arg)) return liii_string_cursor_type_error (sc, who, n_arg);
  nchars= s7_integer (n_arg);
  if (!forward) nchars= -nchars;
  s7_int len= (s7_int) s7_string_length (str);

  if (!is_cursor) return s7_make_integer (sc, cur + nchars);

  s7_int off= cursor_to_offset (cur_arg);
  if (nchars >= 0) {
    for (s7_int i= 0; i < nchars; i++) {
      if (off >= len)
        return liii_string_cursor_value_error (sc, "string-cursor-forward: result would be invalid cursor");
      off= utf8_advance (s, off);
    }
  }
  else {
    for (s7_int i= 0; i < -nchars; i++) {
      if (off <= 0) return liii_string_cursor_value_error (sc, "string-cursor-back: result would be invalid cursor");
      off= utf8_retreat (s, off);
    }
  }
  return offset_to_cursor (sc, off);
}

static s7_pointer
f_string_cursor_forward (s7_scheme* sc, s7_pointer args) {
  return string_cursor_move (sc, args, true, "string-cursor-forward");
}

static s7_pointer
f_string_cursor_back (s7_scheme* sc, s7_pointer args) {
  return string_cursor_move (sc, args, false, "string-cursor-back");
}

// ---- 游标比较 ----
// 索引空间：非负整数直接比较；游标空间：字节偏移越大，负整数越小，方向取反。
// 混用游标与索引是类型错误。

enum cursor_cmp { CMP_EQ, CMP_LT, CMP_GT, CMP_LE, CMP_GE };

static s7_pointer
string_cursor_compare (s7_scheme* sc, s7_pointer args, cursor_cmp op, const char* who) {
  s7_pointer a= s7_car (args), b= s7_cadr (args);
  s7_int     ia, ib;
  bool       ca= parse_cursor_arg (sc, a, who, &ia);
  bool       cb= parse_cursor_arg (sc, b, who, &ib);
  if (ca != cb) return liii_string_cursor_type_error (sc, "string-cursor compare: cannot mix cursor and index", b);

  s7_int x, y;
  if (ca) {
    x= -ia;
    y= -ib;
  } // 取反后恢复字节偏移的升序
  else {
    x= ia;
    y= ib;
  }

  bool result;
  switch (op) {
  case CMP_EQ:
    result= (x == y);
    break;
  case CMP_LT:
    result= (x < y);
    break;
  case CMP_GT:
    result= (x > y);
    break;
  case CMP_LE:
    result= (x <= y);
    break;
  default:
    result= (x >= y);
    break;
  }
  return s7_make_boolean (sc, result);
}

static s7_pointer
f_string_cursor_eq (s7_scheme* sc, s7_pointer args) {
  return string_cursor_compare (sc, args, CMP_EQ, "string-cursor=?");
}
static s7_pointer
f_string_cursor_lt (s7_scheme* sc, s7_pointer args) {
  return string_cursor_compare (sc, args, CMP_LT, "string-cursor<?");
}
static s7_pointer
f_string_cursor_gt (s7_scheme* sc, s7_pointer args) {
  return string_cursor_compare (sc, args, CMP_GT, "string-cursor>?");
}
static s7_pointer
f_string_cursor_le (s7_scheme* sc, s7_pointer args) {
  return string_cursor_compare (sc, args, CMP_LE, "string-cursor<=?");
}
static s7_pointer
f_string_cursor_ge (s7_scheme* sc, s7_pointer args) {
  return string_cursor_compare (sc, args, CMP_GE, "string-cursor>=?");
}

// ---- string-cursor-diff ----

static s7_pointer
f_string_cursor_diff (s7_scheme* sc, s7_pointer args) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, "string-cursor-diff: first parameter must be string");
  if (!s) return NULL;
  s7_pointer a= s7_cadr (args), b= s7_caddr (args);
  s7_int     ia, ib;
  bool       ca= parse_cursor_arg (sc, a, "string-cursor-diff: start must be integer or cursor", &ia);
  bool       cb= parse_cursor_arg (sc, b, "string-cursor-diff: end must be integer or cursor", &ib);
  if (ca != cb) return liii_string_cursor_type_error (sc, "string-cursor-diff: cannot mix cursor and index", b);

  if (!ca) return s7_make_integer (sc, ib - ia);

  s7_int off1= cursor_to_offset (a), off2= cursor_to_offset (b);
  s7_int count= 0;
  while (off1 < off2) {
    off1= utf8_advance (s, off1);
    count++;
  }
  return s7_make_integer (sc, count);
}

// ---- string-cursor->index / string-index->cursor ----

static s7_pointer
f_string_cursor_to_index (s7_scheme* sc, s7_pointer args) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, "string-cursor->index: first parameter must be string");
  if (!s) return NULL;
  s7_pointer cur_arg= s7_cadr (args);
  s7_int     cur;
  bool       is_cursor=
      parse_cursor_arg (sc, cur_arg, "string-cursor->index: second parameter must be integer or cursor", &cur);
  if (!is_cursor) return s7_make_integer (sc, cur);

  s7_int off= cursor_to_offset (cur_arg);
  s7_int len= (s7_int) s7_string_length (str);
  if (off > len) return liii_string_cursor_value_error (sc, "string-cursor->index: cursor out of range");
  s7_int count= 0;
  while (off > 0) {
    off= utf8_retreat (s, off);
    count++;
  }
  return s7_make_integer (sc, count);
}

static s7_pointer
f_string_index_to_cursor (s7_scheme* sc, s7_pointer args) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, "string-index->cursor: first parameter must be string");
  if (!s) return NULL;
  s7_pointer idx_arg= s7_cadr (args);
  s7_int     idx;
  bool       is_cursor=
      parse_cursor_arg (sc, idx_arg, "string-index->cursor: second parameter must be integer or cursor", &idx);
  if (is_cursor) return idx_arg;

  if (idx < 0) return liii_string_cursor_value_error (sc, "string-index->cursor: index out of range");
  s7_int off= 0, len= (s7_int) s7_string_length (str);
  for (s7_int i= 0; i < idx; i++) {
    if (off >= len) return liii_string_cursor_value_error (sc, "string-index->cursor: index out of range");
    off= utf8_advance (s, off);
  }
  return offset_to_cursor (sc, off);
}

// ---- string-ref/cursor ----

static s7_pointer
f_string_ref_cursor (s7_scheme* sc, s7_pointer args) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, "string-ref/cursor: first parameter must be string");
  if (!s) return NULL;
  s7_pointer cur_arg= s7_cadr (args);
  s7_int     cur;
  bool is_cursor= parse_cursor_arg (sc, cur_arg, "string-ref/cursor: second parameter must be integer or cursor", &cur);
  if (!is_cursor && cur < 0)
    return liii_string_cursor_value_error (sc, "string-ref/cursor: cursor at or past end of string");

  s7_int len= (s7_int) s7_string_length (str);
  s7_int off;
  if (is_cursor) {
    off= cursor_to_offset (cur_arg);
    if (off >= len) return liii_string_cursor_value_error (sc, "string-ref/cursor: cursor at or past end of string");
  }
  else {
    off= 0;
    for (s7_int i= 0; i < cur; i++) {
      if (off >= len) return liii_string_cursor_value_error (sc, "string-ref/cursor: cursor at or past end of string");
      off= utf8_advance (s, off);
    }
    if (off >= len) return liii_string_cursor_value_error (sc, "string-ref/cursor: cursor at or past end of string");
  }

  // 解码 UTF-8 码点
  uint32_t cp;
  uint8_t  b0= (uint8_t) s[off];
  s7_int   n = utf8_seq_len (b0);
  if (n == 1) cp= b0;
  else if (n == 2) cp= ((b0 & 0x1F) << 6) | (s[off + 1] & 0x3F);
  else if (n == 3) cp= ((b0 & 0x0F) << 12) | ((s[off + 1] & 0x3F) << 6) | (s[off + 2] & 0x3F);
  else cp= ((b0 & 0x07) << 18) | ((s[off + 1] & 0x3F) << 12) | ((s[off + 2] & 0x3F) << 6) | (s[off + 3] & 0x3F);
  return s7_make_character (sc, cp);
}

// ---- substring/cursors ----

static s7_pointer
f_substring_cursors (s7_scheme* sc, s7_pointer args) {
  s7_pointer  str= s7_car (args);
  const char* s  = check_string_arg (sc, str, "substring/cursors: first parameter must be string");
  if (!s) return NULL;
  s7_pointer a= s7_cadr (args), b= s7_caddr (args);
  s7_int     ia, ib;
  bool       ca= parse_cursor_arg (sc, a, "substring/cursors: start must be integer or cursor", &ia);
  bool       cb= parse_cursor_arg (sc, b, "substring/cursors: end must be integer or cursor", &ib);
  if (ca != cb) return liii_string_cursor_type_error (sc, "substring/cursors: cannot mix cursor and index", b);

  s7_int len= (s7_int) s7_string_length (str);
  s7_int byte_start, byte_end;
  if (ca) {
    byte_start= cursor_to_offset (a);
    byte_end  = cursor_to_offset (b);
  }
  else {
    byte_end= 0;
    for (s7_int i= 0; i < ib; i++) {
      if (byte_end >= len) return liii_string_cursor_value_error (sc, "substring/cursors: end index out of range");
      byte_end= utf8_advance (s, byte_end);
    }
    byte_start= 0;
    for (s7_int i= 0; i < ia; i++)
      byte_start= utf8_advance (s, byte_start);
  }
  if (byte_start > byte_end || byte_end > len)
    return liii_string_cursor_value_error (sc, "substring/cursors: end index out of range");

  s7_pointer result= s7_make_string_with_length (sc, "", byte_end - byte_start);
  memcpy ((char*) s7_string (result), s + byte_start, byte_end - byte_start);
  return result;
}

// ---- 注册 ----

void
glue_liii_string_cursor (s7_scheme* sc) {
  s7_define_function (sc, "g_string-cursor-start", f_string_cursor_start, 1, 0, false,
                      "(g_string-cursor-start str) => start cursor");
  s7_define_function (sc, "g_string-cursor-end", f_string_cursor_end, 1, 0, false,
                      "(g_string-cursor-end str) => post-end cursor");
  s7_define_function (sc, "g_string-cursor-next", f_string_cursor_next, 2, 0, false,
                      "(g_string-cursor-next str cur) => next cursor");
  s7_define_function (sc, "g_string-cursor-prev", f_string_cursor_prev, 2, 0, false,
                      "(g_string-cursor-prev str cur) => previous cursor");
  s7_define_function (sc, "g_string-cursor-forward", f_string_cursor_forward, 3, 0, false,
                      "(g_string-cursor-forward str cur nchars) => cursor");
  s7_define_function (sc, "g_string-cursor-back", f_string_cursor_back, 3, 0, false,
                      "(g_string-cursor-back str cur nchars) => cursor");
  s7_define_function (sc, "g_string-cursor=?", f_string_cursor_eq, 2, 0, false, "(g_string-cursor=? c1 c2) => boolean");
  s7_define_function (sc, "g_string-cursor<?", f_string_cursor_lt, 2, 0, false, "(g_string-cursor<? c1 c2) => boolean");
  s7_define_function (sc, "g_string-cursor>?", f_string_cursor_gt, 2, 0, false, "(g_string-cursor>? c1 c2) => boolean");
  s7_define_function (sc, "g_string-cursor<=?", f_string_cursor_le, 2, 0, false,
                      "(g_string-cursor<=? c1 c2) => boolean");
  s7_define_function (sc, "g_string-cursor>=?", f_string_cursor_ge, 2, 0, false,
                      "(g_string-cursor>=? c1 c2) => boolean");
  s7_define_function (sc, "g_string-cursor-diff", f_string_cursor_diff, 3, 0, false,
                      "(g_string-cursor-diff str start end) => nchars");
  s7_define_function (sc, "g_string-cursor->index", f_string_cursor_to_index, 2, 0, false,
                      "(g_string-cursor->index str cur) => index");
  s7_define_function (sc, "g_string-index->cursor", f_string_index_to_cursor, 2, 0, false,
                      "(g_string-index->cursor str idx) => cursor");
  s7_define_function (sc, "g_string-ref/cursor", f_string_ref_cursor, 2, 0, false,
                      "(g_string-ref/cursor str cur) => char");
  s7_define_function (sc, "g_substring/cursors", f_substring_cursors, 3, 0, false,
                      "(g_substring/cursors str start end) => string");
}

} // namespace goldfish
