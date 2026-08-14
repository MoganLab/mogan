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

namespace goldfish {

// (g_list-sorted? less-p lis) => boolean
// 判定标准与 SRFI-132 参考实现一致：对所有相邻元素对，
// (less-p next prev) 为真（非 #f）即视为逆序。
static s7_pointer
f_list_sorted_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer less_p= s7_car (args);
  s7_pointer lis   = s7_cadr (args);

  if (!s7_is_procedure (less_p)) {
    return s7_wrong_type_arg_error (sc, "list-sorted?", 1, less_p, "a procedure");
  }
  if (!s7_is_pair (lis)) {
    if (s7_is_null (sc, lis)) return s7_t (sc);
    return s7_wrong_type_arg_error (sc, "list-sorted?", 2, lis, "a proper list");
  }

  /* args 的 cons 单元在 less_p 回调期间可能被求值器复用，
   * 因此把 less_p、列表头和我们自建的二元调用列表放进同一个
   * 栈上保护的 anchor，保证回调触发 GC 时全部可达 */
  s7_pointer call_args= s7_cons (sc, s7_f (sc), s7_cons (sc, s7_f (sc), s7_nil (sc)));
  s7_pointer anchor   = s7_cons (sc, less_p, s7_cons (sc, lis, call_args));
  s7_gc_protect_via_stack (sc, anchor);
  s7_pointer call_args_second= s7_cdr (call_args);

  bool       sorted= true;
  s7_pointer prev  = s7_car (lis);
  s7_pointer p     = s7_cdr (lis);
  while (s7_is_pair (p)) {
    s7_pointer cur= s7_car (p);
    s7_set_car (call_args, cur);
    s7_set_car (call_args_second, prev);
    if (s7_apply_function (sc, less_p, call_args) != s7_f (sc)) {
      sorted= false;
      break;
    }
    prev= cur;
    p   = s7_cdr (p);
  }
  s7_gc_unprotect_via_stack (sc, anchor);

  if (!sorted) return s7_f (sc);
  if (!s7_is_null (sc, p)) {
    return s7_wrong_type_arg_error (sc, "list-sorted?", 2, lis, "a proper list");
  }
  return s7_t (sc);
}

static void
glue_list_sorted_p (s7_scheme* sc) {
  const char* name= "g_list-sorted?";
  const char* desc= "(g_list-sorted? less-p lis) => boolean";
  s7_define_function (sc, name, f_list_sorted_p, 2, 0, false, desc);
}

void
glue_liii_sort (s7_scheme* sc) {
  glue_list_sorted_p (sc);
}

} // namespace goldfish
