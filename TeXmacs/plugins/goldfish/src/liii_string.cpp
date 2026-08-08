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

#include "s7.h"
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace goldfish {

static s7_pointer
liii_string_type_error (s7_scheme* sc, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, "type-error"), s7_list (sc, 2, s7_make_string (sc, msg), arg));
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

// 将码点编码为 UTF-8 写入 out，返回字节数（1~4）
static int
utf8_encode (uint32_t cp, char* out) {
  if (cp < 0x80) {
    out[0]= (char) cp;
    return 1;
  }
  if (cp < 0x800) {
    out[0]= (char) (0xC0 | (cp >> 6));
    out[1]= (char) (0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    out[0]= (char) (0xE0 | (cp >> 12));
    out[1]= (char) (0x80 | ((cp >> 6) & 0x3F));
    out[2]= (char) (0x80 | (cp & 0x3F));
    return 3;
  }
  out[0]= (char) (0xF0 | (cp >> 18));
  out[1]= (char) (0x80 | ((cp >> 12) & 0x3F));
  out[2]= (char) (0x80 | ((cp >> 6) & 0x3F));
  out[3]= (char) (0x80 | (cp & 0x3F));
  return 4;
}

static s7_pointer
f_string_split (s7_scheme* sc, s7_pointer args) {
  s7_pointer str_arg= s7_car (args);
  s7_pointer sep_arg= s7_cadr (args);

  if (!s7_is_string (str_arg)) {
    return liii_string_type_error (sc, "string-split: first parameter must be string", str_arg);
  }

  std::string sep;
  if (s7_is_string (sep_arg)) {
    sep.assign (s7_string (sep_arg), (size_t) s7_string_length (sep_arg));
  }
  else if (s7_is_character (sep_arg)) {
    char buf[4];
    int  n= utf8_encode (s7_character (sep_arg), buf);
    sep.assign (buf, (size_t) n);
  }
  else {
    return liii_string_type_error (sc, "string-split: second parameter must be string or char", sep_arg);
  }

  const char* s  = s7_string (str_arg);
  size_t      len= (size_t) s7_string_length (str_arg);

  std::vector<std::pair<size_t, size_t>> parts;
  if (sep.empty ()) {
    // 空分隔符：按 UTF-8 字符拆分；非法字节按单字节处理
    size_t i= 0;
    while (i < len) {
      int n= utf8_seq_len ((uint8_t) s[i]);
      if (n == 0 || i + (size_t) n > len) n= 1;
      parts.emplace_back (i, (size_t) n);
      i+= (size_t) n;
    }
  }
  else {
    std::string_view sv (s, len);
    size_t           start= 0;
    while (true) {
      size_t pos= sv.find (sep, start);
      if (pos == std::string_view::npos) {
        parts.emplace_back (start, len - start);
        break;
      }
      parts.emplace_back (start, pos - start);
      start= pos + sep.size ();
    }
  }

  /* no Scheme callbacks here, so args stay put; only the result being built
   * needs a GC anchor, with each new pair linked in right after s7_cons */
  s7_pointer head= s7_cons (sc, s7_nil (sc), s7_nil (sc));
  s7_gc_protect_via_stack (sc, head);
  s7_pointer tail= head;
  for (const auto& part : parts) {
    s7_pointer str= s7_make_string_with_length (sc, s + part.first, (s7_int) part.second);
    s7_set_cdr (tail, s7_cons (sc, str, s7_nil (sc)));
    tail= s7_cdr (tail);
  }
  s7_gc_unprotect_via_stack (sc, head);
  return s7_cdr (head);
}

enum class string_join_grammar { infix, strict_infix, suffix, prefix };

static s7_pointer
f_string_join (s7_scheme* sc, s7_pointer args) {
  s7_pointer l   = s7_car (args);
  s7_pointer rest= s7_cdr (args);

  std::string delim;
  if (!s7_is_null (sc, rest)) {
    s7_pointer delim_arg= s7_car (rest);
    if (!s7_is_string (delim_arg)) {
      return liii_string_type_error (sc, "optional params in string-join", delim_arg);
    }
    delim.assign (s7_string (delim_arg), (size_t) s7_string_length (delim_arg));
    rest= s7_cdr (rest);
  }

  string_join_grammar grammar      = string_join_grammar::infix;
  bool                grammar_valid= true;
  if (!s7_is_null (sc, rest)) {
    s7_pointer grammar_arg= s7_car (rest);
    if (!s7_is_symbol (grammar_arg)) {
      return liii_string_type_error (sc, "optional params in string-join", grammar_arg);
    }
    const char* name= s7_symbol_name (grammar_arg);
    if (std::strcmp (name, "infix") == 0) grammar= string_join_grammar::infix;
    else if (std::strcmp (name, "strict-infix") == 0) grammar= string_join_grammar::strict_infix;
    else if (std::strcmp (name, "suffix") == 0) grammar= string_join_grammar::suffix;
    else if (std::strcmp (name, "prefix") == 0) grammar= string_join_grammar::prefix;
    else grammar_valid= false;
  }

  // 第一趟：校验元素均为字符串并累计总字节数，与旧实现一样在 grammar 分支之前报错
  size_t     count    = 0;
  size_t     total_len= 0;
  s7_pointer p        = l;
  while (s7_is_pair (p)) {
    s7_pointer elem= s7_car (p);
    if (!s7_is_string (elem)) {
      return liii_string_type_error (sc, "string-join: elements must be strings", elem);
    }
    total_len+= (size_t) s7_string_length (elem);
    count++;
    p= s7_cdr (p);
  }
  if (!s7_is_null (sc, p)) {
    return liii_string_type_error (sc, "string-join: first parameter must be a proper list", l);
  }

  if (!grammar_valid) {
    return s7_error (sc, s7_make_symbol (sc, "value-error"), s7_list (sc, 1, s7_make_string (sc, "invalid grammer")));
  }

  if (grammar == string_join_grammar::strict_infix && count == 0) {
    return s7_error (sc, s7_make_symbol (sc, "value-error"),
                     s7_list (sc, 1, s7_make_string (sc, "empty list not allowed")));
  }

  const size_t delim_len  = delim.size ();
  size_t       delim_count= 0;
  switch (grammar) {
  case string_join_grammar::infix:
  case string_join_grammar::strict_infix:
    delim_count= (count > 0) ? count - 1 : 0;
    break;
  case string_join_grammar::suffix:
  case string_join_grammar::prefix:
    delim_count= count;
    break;
  }

  std::string result;
  result.reserve (total_len + delim_count * delim_len);
  size_t i= 0;
  for (p= l; s7_is_pair (p); p= s7_cdr (p), i++) {
    if (grammar == string_join_grammar::prefix || (i > 0 && grammar != string_join_grammar::suffix)) {
      result.append (delim);
    }
    s7_pointer elem= s7_car (p);
    result.append (s7_string (elem), (size_t) s7_string_length (elem));
    if (grammar == string_join_grammar::suffix) result.append (delim);
  }

  /* no Scheme callbacks here, so args (and the strings reachable from the
   * input list) stay put; the result is built in a C++ buffer first and
   * copied into the Scheme heap in a single allocation */
  return s7_make_string_with_length (sc, result.data (), (s7_int) result.size ());
}

static s7_pointer
f_string_replace (s7_scheme* sc, s7_pointer args) {
  s7_pointer str_arg= s7_car (args);
  s7_pointer old_arg= s7_cadr (args);
  s7_pointer new_arg= s7_caddr (args);
  s7_pointer rest   = s7_cdddr (args);

  if (!s7_is_string (str_arg)) {
    return liii_string_type_error (sc, "string-replace: str must be a string", str_arg);
  }
  if (!s7_is_string (old_arg)) {
    return liii_string_type_error (sc, "string-replace: old must be a string", old_arg);
  }
  if (!s7_is_string (new_arg)) {
    return liii_string_type_error (sc, "string-replace: new must be a string", new_arg);
  }

  s7_int count= -1;
  if (!s7_is_null (sc, rest)) {
    s7_pointer count_arg= s7_car (rest);
    // 与旧实现 (integer? count) 的契约一致：整数或整数值的浮点数均可
    if (s7_is_integer (count_arg)) {
      count= s7_integer (count_arg);
    }
    else if (s7_is_real (count_arg) && std::floor (s7_real (count_arg)) == s7_real (count_arg)) {
      count= (s7_int) s7_real (count_arg);
    }
    else {
      return liii_string_type_error (sc, "string-replace: count must be an integer", count_arg);
    }
  }

  /* 先全部拷入 C++ 缓冲区，之后只在最后做一次 Scheme 堆分配，
   * 因此无需额外的 GC anchor（且全程没有 Scheme 回调） */
  const std::string_view str (s7_string (str_arg), (size_t) s7_string_length (str_arg));
  const std::string_view old_v (s7_string (old_arg), (size_t) s7_string_length (old_arg));
  const std::string_view new_v (s7_string (new_arg), (size_t) s7_string_length (new_arg));

  if (count == 0) {
    return s7_make_string_with_length (sc, str.data (), (s7_int) str.size ());
  }

  std::string result;
  if (old_v.empty ()) {
    // 空 pattern：在每个字节之间插入 new（Python 兼容行为）
    if (str.empty ()) {
      result.assign (new_v);
    }
    else {
      const size_t max_insert= str.size () + 1;
      size_t       remaining = (count < 0) ? max_insert : std::min ((size_t) count, max_insert);
      result.reserve (str.size () + remaining * new_v.size ());
      size_t i= 0;
      while (i < str.size () && remaining > 0) {
        result.append (new_v);
        result.append (str, i, 1);
        i++;
        remaining--;
      }
      if (i == str.size ()) {
        if (remaining > 0) result.append (new_v);
      }
      else {
        result.append (str, i, str.size () - i);
      }
    }
  }
  else {
    // 非空 pattern：从左到右、非重叠替换；UTF-8 字节级匹配是精确的
    size_t remaining= (count < 0) ? std::string_view::npos : (size_t) count;
    size_t start    = 0;
    while (remaining > 0) {
      size_t pos= str.find (old_v, start);
      if (pos == std::string_view::npos) break;
      result.append (str, start, pos - start);
      result.append (new_v);
      start= pos + old_v.size ();
      remaining--;
    }
    if (start == 0) {
      // 无匹配：返回原内容的副本
      return s7_make_string_with_length (sc, str.data (), (s7_int) str.size ());
    }
    result.append (str, start, str.size () - start);
  }

  return s7_make_string_with_length (sc, result.data (), (s7_int) result.size ());
}

static s7_pointer
f_string_starts_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer str_arg   = s7_car (args);
  s7_pointer prefix_arg= s7_cadr (args);

  if (!s7_is_string (str_arg) || !s7_is_string (prefix_arg)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 1, s7_make_string (sc, "string-starts? parameter is not a string")));
  }

  // UTF-8 字节级前缀比较是精确的：前缀字节序列必然落在码点边界上
  const size_t str_len   = (size_t) s7_string_length (str_arg);
  const size_t prefix_len= (size_t) s7_string_length (prefix_arg);
  if (prefix_len > str_len) return s7_f (sc);
  return s7_make_boolean (sc, std::memcmp (s7_string (str_arg), s7_string (prefix_arg), prefix_len) == 0);
}

static s7_pointer
f_string_ends_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer str_arg   = s7_car (args);
  s7_pointer suffix_arg= s7_cadr (args);

  if (!s7_is_string (str_arg) || !s7_is_string (suffix_arg)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 1, s7_make_string (sc, "string-ends? parameter is not a string")));
  }

  const size_t str_len   = (size_t) s7_string_length (str_arg);
  const size_t suffix_len= (size_t) s7_string_length (suffix_arg);
  if (suffix_len > str_len) return s7_f (sc);
  const char* tail= s7_string (str_arg) + (str_len - suffix_len);
  return s7_make_boolean (sc, std::memcmp (tail, s7_string (suffix_arg), suffix_len) == 0);
}

static void
glue_string_join (s7_scheme* sc) {
  const char* name= "g_string-join";
  const char* desc= "(g_string-join string-list . delim+grammar) => string";
  s7_define_function (sc, name, f_string_join, 1, 2, false, desc);
}

static void
glue_string_starts_p (s7_scheme* sc) {
  const char* name= "g_string-starts?";
  const char* desc= "(g_string-starts? str prefix) => boolean";
  s7_define_function (sc, name, f_string_starts_p, 2, 0, false, desc);
}

static void
glue_string_ends_p (s7_scheme* sc) {
  const char* name= "g_string-ends?";
  const char* desc= "(g_string-ends? str suffix) => boolean";
  s7_define_function (sc, name, f_string_ends_p, 2, 0, false, desc);
}

static void
glue_string_replace (s7_scheme* sc) {
  const char* name= "g_string-replace";
  const char* desc= "(g_string-replace str old new . count) => string";
  s7_define_function (sc, name, f_string_replace, 3, 1, false, desc);
}

static void
glue_string_split (s7_scheme* sc) {
  const char* name= "g_string-split";
  const char* desc= "(g_string-split str sep) => list of strings";
  s7_define_function (sc, name, f_string_split, 2, 0, false, desc);
}

void
glue_liii_string (s7_scheme* sc) {
  glue_string_join (sc);
  glue_string_replace (sc);
  glue_string_starts_p (sc);
  glue_string_ends_p (sc);
  glue_string_split (sc);
}

} // namespace goldfish
