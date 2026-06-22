//
// Copyright (C) 2024 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "s7.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace goldfish {
using nlohmann::json;
using std::string;
using std::vector;

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional) {
  s7_pointer cur_env= s7_curlet (sc);
  s7_pointer func   = s7_make_typed_function (sc, name, f, required, optional, false, desc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

static const char* NJSON_HANDLE_TAG= "njson-handle";
struct NjsonState {
  std::thread::id                       owner_thread_id;
  std::vector<std::unique_ptr<json>>    handle_store;
  std::vector<s7_int>                   handle_generations;
  std::vector<s7_int>                   handle_free_ids;
  std::vector<std::vector<std::string>> keys_cache_values;
  std::vector<bool>                     keys_cache_valid;

  NjsonState ()
      : owner_thread_id (std::this_thread::get_id ()), handle_store (1), handle_generations (1, 0),
        keys_cache_values (1), keys_cache_valid (1, false) {}
};

static std::mutex                                                  njson_state_registry_mutex;
static std::unordered_map<s7_scheme*, std::unique_ptr<NjsonState>> njson_state_registry;

static NjsonState&
njson_get_or_create_state (s7_scheme* sc) {
  std::lock_guard<std::mutex> lock (njson_state_registry_mutex);
  auto                        it= njson_state_registry.find (sc);
  if (it == njson_state_registry.end ()) {
    auto inserted= njson_state_registry.emplace (sc, std::make_unique<NjsonState> ());
    return *(inserted.first->second);
  }
  return *(it->second);
}

static void
njson_register_state (s7_scheme* sc) {
  (void) njson_get_or_create_state (sc);
}

static bool
scheme_json_key_to_string (s7_scheme* sc, s7_pointer key, std::string& out, std::string& error_msg) {
  (void) sc;
  if (s7_is_string (key)) {
    out= s7_string (key);
    return true;
  }
  error_msg= "json object key must be string?";
  return false;
}

static s7_pointer
njson_error (s7_scheme* sc, const char* type_name, const std::string& msg, s7_pointer culprit) {
  return s7_error (sc, s7_make_symbol (sc, type_name), s7_list (sc, 2, s7_make_string (sc, msg.c_str ()), culprit));
}

// 把 nlohmann 抛出的 C++ 异常映射成 goldfish 的 s7_error。从 s7 层唯一稳定可
// 触发的 nlohmann 异常是 parse_error（json::parse 解析非法 JSON 时抛）；其他
// nlohmann 异常已被 njson 自身的输入预校验拦截，落到 json::exception 兜底分支
// 作为防御性 fallback，避免将来 nlohmann 内部行为变化时 C++ 异常逃逸到 s7。
static s7_pointer
njson_map_exception_to_error (s7_scheme* sc, const char* api_name, const std::exception& err, s7_pointer culprit) {
  const char* type_name= "misc-error";
  if (dynamic_cast<const json::parse_error*> (&err) != nullptr) {
    type_name= "parse-error";
  } else if (dynamic_cast<const json::exception*> (&err) != nullptr) {
    type_name= "value-error";
  }
  std::string msg= std::string (api_name) + ": " + err.what ();
  return njson_error (sc, type_name, msg, culprit);
}

// 用法：
//   NJSON_TRY_CATCH (sc, "g_njson-keys", handle, {
//     njson_collect_keys (*root, keys);
//     ...
//     return result;
//   });
//
// 只包裹真正会触碰 nlohmann::json 的代码：参数校验和 handle 查找都放在 try 块
// 外，避免一旦失败时 handle 缓存或 free-list 处于半更新状态。
#define NJSON_TRY_CATCH(sc, api_name, culprit, body)                                              \
  do {                                                                                            \
    try body catch (const std::exception& njson_err_) {                                           \
      return njson_map_exception_to_error (sc, api_name, njson_err_, culprit);                   \
    }                                                                                             \
  } while (0)

static s7_pointer
njson_require_owner_thread (s7_scheme* sc, const char* api_name, s7_pointer culprit) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (state.owner_thread_id == std::this_thread::get_id ()) {
    return nullptr;
  }
  return njson_error (sc, "thread-error",
                      std::string (api_name) + ": must be called from the thread that created this VM", culprit);
}

static s7_pointer
make_njson_handle (s7_scheme* sc, s7_int id) {
  NjsonState& state     = njson_get_or_create_state (sc);
  s7_int      generation= 0;
  if (id > 0) {
    size_t index= static_cast<size_t> (id);
    if (index < state.handle_generations.size ()) {
      generation= state.handle_generations[index];
    }
  }
  return s7_cons (sc, s7_make_symbol (sc, NJSON_HANDLE_TAG),
                  s7_cons (sc, s7_make_integer (sc, id), s7_make_integer (sc, generation)));
}

static bool
is_njson_handle (s7_pointer x, s7_int* id_out= nullptr, s7_int* generation_out= nullptr) {
  if (!s7_is_pair (x)) return false;
  s7_pointer tag    = s7_car (x);
  s7_pointer payload= s7_cdr (x);
  if (!s7_is_symbol (tag)) return false;
  if (strcmp (s7_symbol_name (tag), NJSON_HANDLE_TAG) != 0) return false;
  if (!s7_is_pair (payload)) return false;
  s7_pointer id        = s7_car (payload);
  s7_pointer generation= s7_cdr (payload);
  if (!s7_is_integer (id) || !s7_is_integer (generation)) return false;
  if (id_out) *id_out= s7_integer (id);
  if (generation_out) *generation_out= s7_integer (generation);
  return true;
}

static bool
extract_njson_handle_id (s7_scheme* sc, s7_pointer handle, s7_int& id, std::string& error_msg) {
  NjsonState& state     = njson_get_or_create_state (sc);
  s7_int      generation= 0;
  if (!is_njson_handle (handle, &id, &generation)) {
    error_msg= "expected njson handle";
    return false;
  }
  if (id <= 0) {
    error_msg= "invalid njson handle id";
    return false;
  }
  if (generation <= 0) {
    error_msg= "invalid njson handle generation";
    return false;
  }
  size_t index= static_cast<size_t> (id);
  if (index >= state.handle_generations.size ()) {
    error_msg= "njson handle does not exist (may have been freed)";
    return false;
  }
  if (state.handle_generations[index] != generation) {
    error_msg= "njson handle generation mismatch (stale handle)";
    return false;
  }
  if (static_cast<size_t> (id) >= state.handle_store.size () || !state.handle_store[static_cast<size_t> (id)]) {
    error_msg= "njson handle does not exist (may have been freed)";
    return false;
  }
  return true;
}

static json*
njson_value_by_id (s7_scheme* sc, s7_int id) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (id <= 0) return nullptr;
  size_t index= static_cast<size_t> (id);
  if (index >= state.handle_store.size ()) return nullptr;
  return state.handle_store[index].get ();
}

static const json*
njson_value_by_id_const (s7_scheme* sc, s7_int id) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (id <= 0) return nullptr;
  size_t index= static_cast<size_t> (id);
  if (index >= state.handle_store.size ()) return nullptr;
  return state.handle_store[index].get ();
}

static void
njson_ensure_keys_cache_size (s7_scheme* sc, size_t n) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (state.keys_cache_values.size () < n) {
    state.keys_cache_values.resize (n);
    state.keys_cache_valid.resize (n, false);
  }
}

static void
njson_ensure_generations_size (s7_scheme* sc, size_t n) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (state.handle_generations.size () < n) {
    state.handle_generations.resize (n, 0);
  }
}

static void
njson_clear_keys_cache_slot (s7_scheme* sc, s7_int id) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (id <= 0) return;
  size_t index= static_cast<size_t> (id);
  if (index >= state.keys_cache_valid.size ()) return;
  state.keys_cache_values[index].clear ();
  state.keys_cache_valid[index]= false;
}

static void
njson_collect_keys (const json& root, std::vector<std::string>& out) {
  out.clear ();
  if (!root.is_object ()) {
    return;
  }
  out.reserve (root.size ());
  for (auto it= root.begin (); it != root.end (); ++it) {
    out.push_back (it.key ());
  }
}

static s7_pointer
njson_build_keys_list (s7_scheme* sc, const std::vector<std::string>& keys) {
  s7_pointer out= s7_nil (sc);
  for (auto it= keys.rbegin (); it != keys.rend (); ++it) {
    const std::string& key= *it;
    out= s7_cons (sc, s7_make_string_with_length (sc, key.data (), static_cast<s7_int> (key.size ())), out);
  }
  return out;
}

static void
njson_store_keys_cache (s7_scheme* sc, s7_int id, std::vector<std::string>&& keys) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (id <= 0) return;
  size_t index= static_cast<size_t> (id);
  njson_ensure_keys_cache_size (sc, index + 1);
  njson_clear_keys_cache_slot (sc, id);
  state.keys_cache_values[index]= std::move (keys);
  state.keys_cache_valid[index] = true;
}

static bool
njson_try_get_keys_cache (s7_scheme* sc, s7_int id, const std::vector<std::string>*& out) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (id <= 0) return false;
  size_t index= static_cast<size_t> (id);
  if (index >= state.keys_cache_valid.size ()) return false;
  if (!state.keys_cache_valid[index]) return false;
  out= &state.keys_cache_values[index];
  return true;
}

static void
njson_invalidate_keys_cache_if_present (s7_scheme* sc, s7_int id) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (id <= 0) return;
  size_t index= static_cast<size_t> (id);
  if (index >= state.keys_cache_valid.size () || !state.keys_cache_valid[index]) return;
  njson_clear_keys_cache_slot (sc, id);
}

static s7_int
store_njson_value (s7_scheme* sc, json&& value) {
  NjsonState& state= njson_get_or_create_state (sc);
  if (!state.handle_free_ids.empty ()) {
    s7_int id= state.handle_free_ids.back ();
    state.handle_free_ids.pop_back ();
    size_t index= static_cast<size_t> (id);
    njson_ensure_generations_size (sc, index + 1);
    s7_int generation= state.handle_generations[index];
    if (generation <= 0 || generation == (std::numeric_limits<s7_int>::max) ()) {
      generation= 1;
    }
    else {
      generation+= 1;
    }
    state.handle_generations[index]= generation;
    state.handle_store[index]      = std::make_unique<json> (std::move (value));
    njson_ensure_keys_cache_size (sc, state.handle_store.size ());
    return id;
  }

  state.handle_store.push_back (std::make_unique<json> (std::move (value)));
  state.handle_generations.push_back (1);
  njson_ensure_keys_cache_size (sc, state.handle_store.size ());
  s7_int id= static_cast<s7_int> (state.handle_store.size () - 1);
  return id;
}

static s7_int
store_njson_value (s7_scheme* sc, const json& value) {
  json copied= value;
  return store_njson_value (sc, std::move (copied));
}

static bool
scheme_json_index (s7_pointer key, size_t& out, std::string& error_msg) {
  if (!s7_is_integer (key)) {
    error_msg= "array index must be integer?";
    return false;
  }
  s7_int idx= s7_integer (key);
  if (idx < 0) {
    error_msg= "array index must be non-negative";
    return false;
  }
  out= static_cast<size_t> (idx);
  return true;
}

static bool
collect_path_keys (s7_scheme* sc, s7_pointer list, std::vector<s7_pointer>& out, std::string& error_msg) {
  s7_pointer iter= list;
  while (s7_is_pair (iter)) {
    out.push_back (s7_car (iter));
    iter= s7_cdr (iter);
  }
  if (!s7_is_null (sc, iter)) {
    error_msg= "path keys must be a proper list";
    return false;
  }
  return true;
}

template <typename JsonPtr>
static bool
njson_lookup_core (s7_scheme* sc, JsonPtr root, const std::vector<s7_pointer>& path, size_t steps, JsonPtr& out,
                   std::string& error_msg) {
  JsonPtr cur= root;
  for (size_t i= 0; i < steps; i++) {
    s7_pointer key= path[i];
    if (cur->is_object ()) {
      std::string name;
      if (!scheme_json_key_to_string (sc, key, name, error_msg)) {
        return false;
      }
      auto it= cur->find (name);
      if (it == cur->end ()) {
        error_msg= "path not found: missing object key '" + name + "'";
        return false;
      }
      cur= &(*it);
    }
    else if (cur->is_array ()) {
      size_t idx= 0;
      if (!scheme_json_index (key, idx, error_msg)) {
        return false;
      }
      if (idx >= cur->size ()) {
        error_msg= "path not found: array index out of range (index=" + std::to_string (idx) +
                   ", size=" + std::to_string (cur->size ()) + ")";
        return false;
      }
      cur= &(*cur)[idx];
    }
    else {
      char* key_repr_c= s7_object_to_c_string (sc, key);
      if (key_repr_c) {
        std::string key_repr (key_repr_c);
        free (key_repr_c);
        if (key_repr.size () >= 2 && key_repr.front () == '"' && key_repr.back () == '"') {
          key_repr= key_repr.substr (1, key_repr.size () - 2);
        }
        error_msg= "path not found: missing object key '" + key_repr + "'";
      }
      else {
        error_msg= "path not found: missing object key '<unknown>'";
      }
      return false;
    }
  }
  out= cur;
  return true;
}

static bool
lookup_path_const (s7_scheme* sc, const json& root, const std::vector<s7_pointer>& path, const json*& out,
                   std::string& error_msg) {
  return njson_lookup_core (sc, &root, path, path.size (), out, error_msg);
}

static bool
lookup_path_parent_mutable (s7_scheme* sc, json& root, const std::vector<s7_pointer>& path, json*& parent,
                            s7_pointer& last_key, std::string& error_msg) {
  if (path.empty ()) {
    error_msg= "path cannot be empty";
    return false;
  }

  if (!njson_lookup_core (sc, &root, path, path.size () - 1, parent, error_msg)) {
    return false;
  }
  last_key= path.back ();
  return true;
}

static bool
lookup_path_mutable (s7_scheme* sc, json& root, const std::vector<s7_pointer>& path, json*& out,
                     std::string& error_msg) {
  return njson_lookup_core (sc, &root, path, path.size (), out, error_msg);
}

static bool
scheme_to_njson_scalar_or_handle (s7_scheme* sc, s7_pointer value, json& out, std::string& error_msg) {
  if (is_njson_handle (value)) {
    s7_int id= 0;
    if (!extract_njson_handle_id (sc, value, id, error_msg)) {
      return false;
    }
    const json* source= njson_value_by_id_const (sc, id);
    if (!source) {
      error_msg= "njson handle does not exist (may have been freed)";
      return false;
    }
    out= *source;
    return true;
  }

  if (s7_is_string (value)) {
    out= s7_string (value);
    return true;
  }
  if (s7_is_boolean (value)) {
    out= s7_boolean (sc, value);
    return true;
  }
  if (s7_is_integer (value)) {
    out= static_cast<long long> (s7_integer (value));
    return true;
  }
  if (s7_is_real (value)) {
    double real_value= s7_number_to_real (sc, value);
    if (!std::isfinite (real_value)) {
      error_msg= "number must be finite (NaN/Inf are not valid JSON numbers)";
      return false;
    }
    out= real_value;
    return true;
  }
  if (s7_is_number (value)) {
    error_msg= "number must be real and finite";
    return false;
  }
  if (s7_is_symbol (value)) {
    const char* symbol_name= s7_symbol_name (value);
    if (strcmp (symbol_name, "null") == 0) {
      out= nullptr;
      return true;
    }
    error_msg= "symbol value must be null; use boolean? for true/false and string? for text";
    return false;
  }

  if (s7_is_vector (value)) {
    s7_int      len     = s7_vector_length (value);
    s7_pointer* elements= s7_vector_elements (value);
    out                 = json::array ();
    for (s7_int i= 0; i < len; ++i) {
      json element_json;
      if (!scheme_to_njson_scalar_or_handle (sc, elements[i], element_json, error_msg)) {
        error_msg= "vector element " + std::to_string (i) + ": " + error_msg;
        return false;
      }
      out.push_back (element_json);
    }
    return true;
  }

  error_msg= "value must be njson handle, string?, number?, boolean?, vector?, or null symbol";
  return false;
}

static s7_pointer
njson_scalar_value_to_scheme (s7_scheme* sc, const json& value) {
  if (value.is_null ()) {
    return s7_make_symbol (sc, "null");
  }
  if (value.is_boolean ()) {
    return s7_make_boolean (sc, value.get<bool> ());
  }
  if (value.is_number_integer ()) {
    return s7_make_integer (sc, static_cast<s7_int> (value.get<long long> ()));
  }
  if (value.is_number_unsigned ()) {
    unsigned long long v= value.get<unsigned long long> ();
    if (v > static_cast<unsigned long long> ((std::numeric_limits<s7_int>::max) ())) {
      return s7_make_real (sc, static_cast<double> (v));
    }
    return s7_make_integer (sc, static_cast<s7_int> (v));
  }
  if (value.is_number_float ()) {
    return s7_make_real (sc, value.get<double> ());
  }
  if (value.is_string ()) {
    const auto& text= value.get_ref<const std::string&> ();
    return s7_make_string (sc, text.c_str ());
  }
  return s7_nil (sc);
}

enum class njson_scheme_tree_mode { alist_list, hash_vector };

static s7_pointer njson_value_to_scheme_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode);

static s7_pointer
njson_object_to_alist_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  // Match (liii json): empty object is '(()) so {} stays distinct from [].
  if (value.empty ()) {
    return s7_cons (sc, s7_nil (sc), s7_nil (sc));
  }
  s7_pointer out= s7_nil (sc);
  for (auto it= value.begin (); it != value.end (); ++it) {
    const std::string& key   = it.key ();
    s7_pointer         key_s7= s7_make_string_with_length (sc, key.data (), static_cast<s7_int> (key.size ()));
    s7_pointer         val_s7= njson_value_to_scheme_tree (sc, it.value (), mode);
    out                      = s7_cons (sc, s7_cons (sc, key_s7, val_s7), out);
  }
  return s7_reverse (sc, out);
}

static s7_pointer
njson_array_to_list_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  s7_pointer out= s7_nil (sc);
  for (auto it= value.begin (); it != value.end (); ++it) {
    out= s7_cons (sc, njson_value_to_scheme_tree (sc, *it, mode), out);
  }
  return s7_reverse (sc, out);
}

static s7_pointer
njson_object_to_hash_table_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  s7_pointer out= s7_make_hash_table (sc, static_cast<s7_int> (value.size ()));
  for (auto it= value.begin (); it != value.end (); ++it) {
    const std::string& key= it.key ();
    s7_hash_table_set (sc, out, s7_make_string_with_length (sc, key.data (), static_cast<s7_int> (key.size ())),
                       njson_value_to_scheme_tree (sc, it.value (), mode));
  }
  return out;
}

static s7_pointer
njson_array_to_vector_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  s7_pointer out= s7_make_vector (sc, static_cast<s7_int> (value.size ()));
  for (size_t i= 0; i < value.size (); i++) {
    s7_vector_set (sc, out, static_cast<s7_int> (i), njson_value_to_scheme_tree (sc, value[i], mode));
  }
  return out;
}

static s7_pointer
njson_value_to_scheme_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  if (value.is_object ()) {
    return (mode == njson_scheme_tree_mode::alist_list) ? njson_object_to_alist_tree (sc, value, mode)
                                                        : njson_object_to_hash_table_tree (sc, value, mode);
  }
  if (value.is_array ()) {
    return (mode == njson_scheme_tree_mode::alist_list) ? njson_array_to_list_tree (sc, value, mode)
                                                        : njson_array_to_vector_tree (sc, value, mode);
  }
  return njson_scalar_value_to_scheme (sc, value);
}

enum class njson_structure_root_kind { object, array };

static s7_pointer
njson_run_structure_conversion (s7_scheme* sc, s7_pointer args, const char* api_name,
                                njson_structure_root_kind root_kind, njson_scheme_tree_mode mode) {
  s7_pointer thread_err= njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }

  s7_pointer  handle= s7_car (args);
  s7_int      id    = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, handle);
  }

  const json* root= njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
  }

  if ((root_kind == njson_structure_root_kind::object) && !root->is_object ()) {
    return njson_error (sc, "type-error", std::string (api_name) + ": json root must be object", handle);
  }
  if ((root_kind == njson_structure_root_kind::array) && !root->is_array ()) {
    return njson_error (sc, "type-error", std::string (api_name) + ": json root must be array", handle);
  }

  NJSON_TRY_CATCH (sc, api_name, handle, {
    return njson_value_to_scheme_tree (sc, *root, mode);
  });
}

static s7_pointer
njson_value_to_scheme_or_handle (s7_scheme* sc, const json& value) {
  if (value.is_object () || value.is_array ()) {
    s7_int id= store_njson_value (sc, value);
    return make_njson_handle (sc, id);
  }
  return njson_scalar_value_to_scheme (sc, value);
}

static s7_pointer
f_njson_string_to_json (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-string->json", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input= s7_car (args);
  if (!s7_is_string (input)) {
    return njson_error (sc, "type-error", "g_njson-string->json: input must be string", input);
  }

  NJSON_TRY_CATCH (sc, "g_njson-string->json", input, {
    json parsed= json::parse (s7_string (input));
    return make_njson_handle (sc, store_njson_value (sc, std::move (parsed)));
  });
}

static s7_pointer
f_njson_json_to_string (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-json->string", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  input= s7_car (args);
  json        encoded;
  std::string error_msg;
  if (!scheme_to_njson_scalar_or_handle (sc, input, encoded, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-json->string: " + error_msg, input);
  }
  NJSON_TRY_CATCH (sc, "g_njson-json->string", input, {
    std::string dumped= encoded.dump ();
    return s7_make_string (sc, dumped.c_str ());
  });
}

static s7_pointer
f_njson_format_string (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-format-string", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input= s7_car (args);
  if (!s7_is_string (input)) {
    return njson_error (sc, "type-error", "g_njson-format-string: input must be string", input);
  }

  s7_int     indent= 2;
  s7_pointer rest  = s7_cdr (args);
  if (!s7_is_null (sc, rest)) {
    s7_pointer indent_arg= s7_car (rest);
    if (!s7_is_integer (indent_arg)) {
      return njson_error (sc, "type-error", "g_njson-format-string: indent must be integer?", indent_arg);
    }
    indent= s7_integer (indent_arg);
    if (indent < 0) {
      return njson_error (sc, "value-error", "g_njson-format-string: indent must be >= 0", indent_arg);
    }
  }

  NJSON_TRY_CATCH (sc, "g_njson-format-string", input, {
    json        parsed= json::parse (s7_string (input));
    std::string dumped= parsed.dump (static_cast<int> (indent));
    return s7_make_string (sc, dumped.c_str ());
  });
}

static s7_pointer
f_njson_handle_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-handle?", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input= s7_car (args);
  return s7_make_boolean (sc, is_njson_handle (input));
}

template <typename HandlePredicate, typename ScalarPredicate>
static s7_pointer
njson_run_value_type_predicate (s7_scheme* sc, s7_pointer args, const char* api_name, HandlePredicate handle_pred,
                                ScalarPredicate scalar_pred) {
  s7_pointer thread_err= njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input= s7_car (args);
  if (!is_njson_handle (input)) {
    return s7_make_boolean (sc, scalar_pred (input));
  }

  s7_int      id= 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, input, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, input);
  }
  const json* value= njson_value_by_id_const (sc, id);
  if (!value) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", input);
  }
  NJSON_TRY_CATCH (sc, api_name, input, {
    return s7_make_boolean (sc, handle_pred (*value));
  });
}

static s7_pointer
f_njson_null_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
      sc, args, "g_njson-null?", [] (const json& value) { return value.is_null (); },
      [] (s7_pointer value) { return s7_is_symbol (value) && strcmp (s7_symbol_name (value), "null") == 0; });
}

static s7_pointer
f_njson_object_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
      sc, args, "g_njson-object?", [] (const json& value) { return value.is_object (); },
      [] (s7_pointer value) {
        (void) value;
        return false;
      });
}

static s7_pointer
f_njson_array_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
      sc, args, "g_njson-array?", [] (const json& value) { return value.is_array (); },
      [] (s7_pointer value) {
        (void) value;
        return false;
      });
}

static s7_pointer
f_njson_string_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
      sc, args, "g_njson-string?", [] (const json& value) { return value.is_string (); },
      [] (s7_pointer value) { return s7_is_string (value); });
}

static s7_pointer
f_njson_number_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
      sc, args, "g_njson-number?", [] (const json& value) { return value.is_number (); },
      [] (s7_pointer value) { return s7_is_number (value); });
}

static s7_pointer
f_njson_integer_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
      sc, args, "g_njson-integer?",
      [] (const json& value) { return value.is_number_integer () || value.is_number_unsigned (); },
      [] (s7_pointer value) { return s7_is_integer (value); });
}

static s7_pointer
f_njson_boolean_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
      sc, args, "g_njson-boolean?", [] (const json& value) { return value.is_boolean (); },
      [] (s7_pointer value) { return s7_is_boolean (value); });
}

static s7_pointer
f_njson_free (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-free", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle= s7_car (args);
  s7_int      id    = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-free: " + error_msg, handle);
  }
  NjsonState& state= njson_get_or_create_state (sc);
  njson_clear_keys_cache_slot (sc, id);
  state.handle_store[static_cast<size_t> (id)].reset ();
  state.handle_free_ids.push_back (id);
  return s7_t (sc);
}

static s7_pointer
f_njson_size (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-size", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle= s7_car (args);
  s7_int      id    = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-size: " + error_msg, handle);
  }

  const json* root= njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-size: njson handle does not exist (may have been freed)", handle);
  }

  if (root->is_object () || root->is_array ()) {
    NJSON_TRY_CATCH (sc, "g_njson-size", handle, {
      return s7_make_integer (sc, static_cast<s7_int> (root->size ()));
    });
  }
  return s7_make_integer (sc, 0);
}

static s7_pointer
f_njson_empty_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-empty?", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle= s7_car (args);
  s7_int      id    = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-empty?: " + error_msg, handle);
  }

  const json* root= njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-empty?: njson handle does not exist (may have been freed)", handle);
  }

  if (root->is_object () || root->is_array ()) {
    NJSON_TRY_CATCH (sc, "g_njson-empty?", handle, {
      return s7_make_boolean (sc, root->empty ());
    });
  }
  return s7_t (sc);
}

static s7_pointer
f_njson_ref (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-ref", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle= s7_car (args);
  s7_int      id    = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-ref: " + error_msg, handle);
  }

  std::vector<s7_pointer> path;
  if (!collect_path_keys (sc, s7_cdr (args), path, error_msg)) {
    return njson_error (sc, "key-error", "g_njson-ref: " + error_msg, handle);
  }
  if (path.empty ()) {
    return njson_error (sc, "key-error", "g_njson-ref: missing key arguments", handle);
  }

  const json* root= njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-ref: njson handle does not exist (may have been freed)", handle);
  }

  const json* found_value= nullptr;
  if (!lookup_path_const (sc, *root, path, found_value, error_msg)) {
    return njson_error (sc, "key-error", "g_njson-ref: " + error_msg, handle);
  }
  NJSON_TRY_CATCH (sc, "g_njson-ref", handle, {
    return njson_value_to_scheme_or_handle (sc, *found_value);
  });
}

enum class njson_update_op { set, append, drop };

static bool
njson_update_needs_value (njson_update_op op) {
  return op != njson_update_op::drop;
}

static const char*
njson_update_expected_argv (njson_update_op op) {
  if (op == njson_update_op::drop) {
    return "expected (json key ...)";
  }
  if (op == njson_update_op::append) {
    return "expected (json [key ...] value)";
  }
  return "expected (json key ... value)";
}

static s7_pointer
njson_parse_update_request (s7_scheme* sc, s7_pointer args, const char* api_name, njson_update_op op,
                            s7_pointer& handle, s7_int& id, std::vector<s7_pointer>& path, json& value_json) {
  handle= s7_car (args);
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, handle);
  }

  std::vector<s7_pointer> tokens;
  if (!collect_path_keys (sc, s7_cdr (args), tokens, error_msg)) {
    return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, handle);
  }

  if (njson_update_needs_value (op)) {
    size_t min_tokens= (op == njson_update_op::append) ? 1 : 2;
    if (tokens.size () < min_tokens) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + njson_update_expected_argv (op), handle);
    }
    path.assign (tokens.begin (), tokens.end () - 1);
    s7_pointer value_token= tokens.back ();
    if (!scheme_to_njson_scalar_or_handle (sc, value_token, value_json, error_msg)) {
      return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, value_token);
    }
  }
  else {
    if (tokens.empty ()) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + njson_update_expected_argv (op), handle);
    }
    path= std::move (tokens);
  }
  return nullptr;
}

static s7_pointer
njson_apply_update_on_root (s7_scheme* sc, json& root, const std::vector<s7_pointer>& path, const json& value_json,
                            njson_update_op op, const char* api_name, s7_pointer handle) {
  if (op == njson_update_op::append) {
    std::string error_msg;
    json*       target= &root;
    if (!path.empty ()) {
      if (!lookup_path_mutable (sc, root, path, target, error_msg)) {
        return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, handle);
      }
    }
    if (!target->is_array ()) {
      return njson_error (sc, "key-error", std::string (api_name) + ": append target must be array", handle);
    }
    target->push_back (value_json);
    return nullptr;
  }

  std::string error_msg;
  json*       parent  = nullptr;
  s7_pointer  last_key= s7_nil (sc);
  if (!lookup_path_parent_mutable (sc, root, path, parent, last_key, error_msg)) {
    return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, handle);
  }

  if (parent->is_object ()) {
    std::string key_name;
    if (!scheme_json_key_to_string (sc, last_key, key_name, error_msg)) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, last_key);
    }

    if (op == njson_update_op::set) {
      (*parent)[key_name]= value_json;
    }
    else {
      auto it= parent->find (key_name);
      if (it == parent->end ()) {
        return njson_error (sc, "key-error",
                            std::string (api_name) + ": path not found: missing object key '" + key_name + "'",
                            last_key);
      }
      parent->erase (it);
    }
    return nullptr;
  }

  if (parent->is_array ()) {
    size_t idx= 0;
    if (!scheme_json_index (last_key, idx, error_msg)) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, last_key);
    }

    if (op == njson_update_op::set) {
      if (idx < parent->size ()) {
        (*parent)[idx]= value_json;
      }
      else {
        return njson_error (sc, "key-error",
                            std::string (api_name) + ": array index out of range (index=" + std::to_string (idx) +
                                ", size=" + std::to_string (parent->size ()) + ")",
                            last_key);
      }
    }
    else {
      if (idx < parent->size ()) {
        parent->erase (parent->begin () + static_cast<json::difference_type> (idx));
      }
      else {
        return njson_error (sc, "key-error",
                            std::string (api_name) + ": path not found: array index out of range (index=" +
                                std::to_string (idx) + ", size=" + std::to_string (parent->size ()) + ")",
                            last_key);
      }
    }
    return nullptr;
  }

  if (op == njson_update_op::drop) {
    return njson_error (sc, "key-error",
                        std::string (api_name) + ": path not found: cannot drop from non-container value", last_key);
  }
  if (op == njson_update_op::set) {
    return njson_error (sc, "key-error", std::string (api_name) + ": set target must be array or object", last_key);
  }
  return nullptr;
}

static s7_pointer
njson_run_update (s7_scheme* sc, s7_pointer args, const char* api_name, njson_update_op op, bool in_place) {
  s7_pointer thread_err= njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer              handle= s7_nil (sc);
  s7_int                  id    = 0;
  std::vector<s7_pointer> path;
  json                    value_json;
  s7_pointer              err= njson_parse_update_request (sc, args, api_name, op, handle, id, path, value_json);
  if (err) {
    return err;
  }

  if (in_place) {
    json* root= njson_value_by_id (sc, id);
    if (!root) {
      return njson_error (sc, "type-error",
                          std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
    }
    NJSON_TRY_CATCH (sc, api_name, handle, {
      err= njson_apply_update_on_root (sc, *root, path, value_json, op, api_name, handle);
    });
    if (err) {
      return err;
    }
    // Keep write-path fast: only invalidate; keys will be rebuilt lazily on next njson-keys call.
    njson_invalidate_keys_cache_if_present (sc, id);
    return handle;
  }

  const json* root= njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
  }
  json out= *root;
  NJSON_TRY_CATCH (sc, api_name, handle, {
    err= njson_apply_update_on_root (sc, out, path, value_json, op, api_name, handle);
  });
  if (err) {
    return err;
  }
  return make_njson_handle (sc, store_njson_value (sc, std::move (out)));
}

enum class njson_merge_mode { shallow, deep };

static s7_pointer
njson_run_merge (s7_scheme* sc, s7_pointer args, const char* api_name, njson_merge_mode mode, bool in_place) {
  s7_pointer thread_err= njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle      = s7_car (args);
  s7_pointer  source_input= s7_cadr (args);
  s7_int      id          = 0;
  json        source_json;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, handle);
  }
  if (!scheme_to_njson_scalar_or_handle (sc, source_input, source_json, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, source_input);
  }
  if (!source_json.is_object ()) {
    return njson_error (sc, "type-error", std::string (api_name) + ": merge source must be object", source_input);
  }
  bool merge_objects= (mode == njson_merge_mode::deep);

  if (in_place) {
    json* target= njson_value_by_id (sc, id);
    if (!target) {
      return njson_error (sc, "type-error",
                          std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
    }
    if (!target->is_object ()) {
      return njson_error (sc, "type-error", std::string (api_name) + ": merge target must be object", handle);
    }
    NJSON_TRY_CATCH (sc, api_name, source_input, {
      target->update (source_json, merge_objects);
    });
    njson_invalidate_keys_cache_if_present (sc, id);
    return handle;
  }

  const json* target= njson_value_by_id_const (sc, id);
  if (!target) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
  }
  if (!target->is_object ()) {
    return njson_error (sc, "type-error", std::string (api_name) + ": merge target must be object", handle);
  }
  json out= *target;
  NJSON_TRY_CATCH (sc, api_name, source_input, {
    out.update (source_json, merge_objects);
  });
  return make_njson_handle (sc, store_njson_value (sc, std::move (out)));
}

static s7_pointer
f_njson_set (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-set", njson_update_op::set, false);
}

static s7_pointer
f_njson_append (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-append", njson_update_op::append, false);
}

static s7_pointer
f_njson_append_x (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-append!", njson_update_op::append, true);
}

static s7_pointer
f_njson_drop (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-drop", njson_update_op::drop, false);
}

static s7_pointer
f_njson_set_x (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-set!", njson_update_op::set, true);
}

static s7_pointer
f_njson_drop_x (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-drop!", njson_update_op::drop, true);
}

static s7_pointer
f_njson_merge (s7_scheme* sc, s7_pointer args) {
  return njson_run_merge (sc, args, "g_njson-merge", njson_merge_mode::shallow, false);
}

static s7_pointer
f_njson_merge_x (s7_scheme* sc, s7_pointer args) {
  return njson_run_merge (sc, args, "g_njson-merge!", njson_merge_mode::shallow, true);
}

static s7_pointer
f_njson_deep_merge (s7_scheme* sc, s7_pointer args) {
  return njson_run_merge (sc, args, "g_njson-deep-merge", njson_merge_mode::deep, false);
}

static s7_pointer
f_njson_deep_merge_x (s7_scheme* sc, s7_pointer args) {
  return njson_run_merge (sc, args, "g_njson-deep-merge!", njson_merge_mode::deep, true);
}

static s7_pointer
f_njson_contains_key_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-contains-key?", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle= s7_car (args);
  s7_pointer  key   = s7_cadr (args);
  s7_int      id    = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-contains-key?: " + error_msg, handle);
  }

  const json* root= njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-contains-key?: njson handle does not exist (may have been freed)",
                        handle);
  }
  if (!root->is_object ()) {
    return s7_f (sc);
  }

  std::string key_name;
  if (!scheme_json_key_to_string (sc, key, key_name, error_msg)) {
    return njson_error (sc, "key-error", "g_njson-contains-key?: " + error_msg, key);
  }
  NJSON_TRY_CATCH (sc, "g_njson-contains-key?", handle, {
    return s7_make_boolean (sc, root->contains (key_name));
  });
}

static s7_pointer
f_njson_keys (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-keys", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle= s7_car (args);
  s7_int      id    = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-keys: " + error_msg, handle);
  }

  const json* root= njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-keys: njson handle does not exist (may have been freed)", handle);
  }
  if (!root->is_object ()) {
    return s7_nil (sc);
  }

  const std::vector<std::string>* cached= nullptr;
  if (njson_try_get_keys_cache (sc, id, cached)) {
    return njson_build_keys_list (sc, *cached);
  }

  NJSON_TRY_CATCH (sc, "g_njson-keys", handle, {
    std::vector<std::string> keys;
    njson_collect_keys (*root, keys);
    njson_store_keys_cache (sc, id, std::move (keys));
    const std::vector<std::string>* stored= nullptr;
    if (njson_try_get_keys_cache (sc, id, stored)) {
      return njson_build_keys_list (sc, *stored);
    }
  });
  return s7_nil (sc);
}

static s7_pointer
f_njson_object_to_alist (s7_scheme* sc, s7_pointer args) {
  return njson_run_structure_conversion (sc, args, "g_njson-object->alist", njson_structure_root_kind::object,
                                         njson_scheme_tree_mode::alist_list);
}

static s7_pointer
f_njson_object_to_hash_table (s7_scheme* sc, s7_pointer args) {
  return njson_run_structure_conversion (sc, args, "g_njson-object->hash-table", njson_structure_root_kind::object,
                                         njson_scheme_tree_mode::hash_vector);
}

static s7_pointer
f_njson_array_to_list (s7_scheme* sc, s7_pointer args) {
  return njson_run_structure_conversion (sc, args, "g_njson-array->list", njson_structure_root_kind::array,
                                         njson_scheme_tree_mode::alist_list);
}

static s7_pointer
f_njson_array_to_vector (s7_scheme* sc, s7_pointer args) {
  return njson_run_structure_conversion (sc, args, "g_njson-array->vector", njson_structure_root_kind::array,
                                         njson_scheme_tree_mode::hash_vector);
}

struct njson_schema_error_entry {
  std::string instance_path;
  std::string message;
  std::string instance_dump;
};

class njson_collecting_error_handler : public nlohmann::json_schema::error_handler {
public:
  std::vector<njson_schema_error_entry> entries;

  void error (const json::json_pointer& ptr, const json& instance, const std::string& message) override {
    std::string dumped;
    try {
      dumped= instance.dump ();
    } catch (...) {
      dumped= "<failed-to-dump-instance>";
    }
    entries.push_back (njson_schema_error_entry{ptr.to_string (), message, dumped});
  }
};

static s7_pointer
njson_schema_errors_to_scheme (s7_scheme* sc, const std::vector<njson_schema_error_entry>& errors) {
  s7_pointer out= s7_nil (sc);
  for (auto it= errors.rbegin (); it != errors.rend (); ++it) {
    s7_pointer row= s7_make_hash_table (sc, 3);
    s7_hash_table_set (sc, row, s7_make_symbol (sc, "instance-path"), s7_make_string (sc, it->instance_path.c_str ()));
    s7_hash_table_set (sc, row, s7_make_symbol (sc, "message"), s7_make_string (sc, it->message.c_str ()));
    s7_hash_table_set (sc, row, s7_make_symbol (sc, "instance"), s7_make_string (sc, it->instance_dump.c_str ()));
    out= s7_cons (sc, row, out);
  }
  return out;
}

static s7_pointer
njson_run_schema_validation (s7_scheme* sc, const char* api_name, s7_pointer args,
                             std::vector<njson_schema_error_entry>& errors_out) {
  s7_pointer  schema_input  = s7_car (args);
  s7_pointer  instance_input= s7_cadr (args);
  json        schema_json;
  json        instance_json;
  std::string error_msg;
  if (!scheme_to_njson_scalar_or_handle (sc, schema_input, schema_json, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": schema " + error_msg, schema_input);
  }
  if (!scheme_to_njson_scalar_or_handle (sc, instance_input, instance_json, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": instance " + error_msg, instance_input);
  }

  nlohmann::json_schema::json_validator validator;
  try {
    validator.set_root_schema (schema_json);
  } catch (const std::exception& err) {
    return njson_error (sc, "schema-error", std::string (api_name) + ": " + std::string (err.what ()), schema_input);
  }

  njson_collecting_error_handler err_handler;
  try {
    validator.validate (instance_json, err_handler);
  } catch (const std::exception& err) {
    return njson_error (sc, "validation-error", std::string (api_name) + ": " + std::string (err.what ()),
                        instance_input);
  }
  errors_out= std::move (err_handler.entries);
  return nullptr;
}

static s7_pointer
f_njson_schema_report (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err= njson_require_owner_thread (sc, "g_njson-schema-report", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  std::vector<njson_schema_error_entry> errors;
  s7_pointer                            err= njson_run_schema_validation (sc, "g_njson-schema-report", args, errors);
  if (err) {
    return err;
  }

  s7_pointer report= s7_make_hash_table (sc, 3);
  s7_hash_table_set (sc, report, s7_make_symbol (sc, "valid?"), s7_make_boolean (sc, errors.empty ()));
  s7_hash_table_set (sc, report, s7_make_symbol (sc, "error-count"),
                     s7_make_integer (sc, static_cast<s7_int> (errors.size ())));
  s7_hash_table_set (sc, report, s7_make_symbol (sc, "errors"), njson_schema_errors_to_scheme (sc, errors));
  return report;
}

void
glue_njson (s7_scheme* sc) {
  njson_register_state (sc);
  const char* parse_name        = "g_njson-string->json";
  const char* parse_desc        = "(g_njson-string->json json-string) => njson-handle";
  const char* dump_name         = "g_njson-json->string";
  const char* dump_desc         = "(g_njson-json->string handle-or-scalar) => strict-json-string";
  const char* format_name       = "g_njson-format-string";
  const char* format_desc       = "(g_njson-format-string json-string :optional indent) => strict-json-string";
  const char* handlep_name      = "g_njson-handle?";
  const char* handlep_desc      = "(g_njson-handle? x) => boolean?";
  const char* nullp_name        = "g_njson-null?";
  const char* nullp_desc        = "(g_njson-null? x) => boolean?";
  const char* objectp_name      = "g_njson-object?";
  const char* objectp_desc      = "(g_njson-object? x) => boolean?";
  const char* arrayp_name       = "g_njson-array?";
  const char* arrayp_desc       = "(g_njson-array? x) => boolean?";
  const char* stringp_name      = "g_njson-string?";
  const char* stringp_desc      = "(g_njson-string? x) => boolean?";
  const char* numberp_name      = "g_njson-number?";
  const char* numberp_desc      = "(g_njson-number? x) => boolean?";
  const char* integerp_name     = "g_njson-integer?";
  const char* integerp_desc     = "(g_njson-integer? x) => boolean?";
  const char* booleanp_name     = "g_njson-boolean?";
  const char* booleanp_desc     = "(g_njson-boolean? x) => boolean?";
  const char* size_name         = "g_njson-size";
  const char* size_desc         = "(g_njson-size handle) => integer?";
  const char* emptyp_name       = "g_njson-empty?";
  const char* emptyp_desc       = "(g_njson-empty? handle) => boolean?";
  const char* free_name         = "g_njson-free";
  const char* free_desc         = "(g_njson-free handle) => boolean?";
  const char* ref_name          = "g_njson-ref";
  const char* ref_desc          = "(g_njson-ref handle key ...) => scalar-or-handle";
  const char* set_name          = "g_njson-set";
  const char* set_desc          = "(g_njson-set handle key ... value) => new-handle";
  const char* append_name       = "g_njson-append";
  const char* append_desc       = "(g_njson-append handle [key ...] value) => new-handle";
  const char* set_x_name        = "g_njson-set!";
  const char* set_x_desc        = "(g_njson-set! handle key ... value) => same-handle";
  const char* append_x_name     = "g_njson-append!";
  const char* append_x_desc     = "(g_njson-append! handle [key ...] value) => same-handle";
  const char* drop_name         = "g_njson-drop";
  const char* drop_desc         = "(g_njson-drop handle key ...) => new-handle";
  const char* drop_x_name       = "g_njson-drop!";
  const char* drop_x_desc       = "(g_njson-drop! handle key ...) => same-handle";
  const char* merge_name        = "g_njson-merge";
  const char* merge_desc        = "(g_njson-merge handle other-object) => new-handle";
  const char* merge_x_name      = "g_njson-merge!";
  const char* merge_x_desc      = "(g_njson-merge! handle other-object) => same-handle";
  const char* deep_merge_name   = "g_njson-deep-merge";
  const char* deep_merge_desc   = "(g_njson-deep-merge handle other-object) => new-handle";
  const char* deep_merge_x_name = "g_njson-deep-merge!";
  const char* deep_merge_x_desc = "(g_njson-deep-merge! handle other-object) => same-handle";
  const char* has_key_name      = "g_njson-contains-key?";
  const char* has_key_desc      = "(g_njson-contains-key? handle key) => boolean?";
  const char* keys_name         = "g_njson-keys";
  const char* keys_desc         = "(g_njson-keys handle) => (list-of string?)";
  const char* object_alist_name = "g_njson-object->alist";
  const char* object_alist_desc = "(g_njson-object->alist object-handle) => alist";
  const char* object_hash_name  = "g_njson-object->hash-table";
  const char* object_hash_desc  = "(g_njson-object->hash-table object-handle) => hash-table";
  const char* array_list_name   = "g_njson-array->list";
  const char* array_list_desc   = "(g_njson-array->list array-handle) => list";
  const char* array_vector_name = "g_njson-array->vector";
  const char* array_vector_desc = "(g_njson-array->vector array-handle) => vector";
  const char* schema_report_name= "g_njson-schema-report";
  const char* schema_report_desc= "(g_njson-schema-report schema-handle instance) => hash-table";
  glue_define (sc, parse_name, parse_desc, f_njson_string_to_json, 1, 0);
  glue_define (sc, dump_name, dump_desc, f_njson_json_to_string, 1, 0);
  glue_define (sc, format_name, format_desc, f_njson_format_string, 1, 1);
  glue_define (sc, handlep_name, handlep_desc, f_njson_handle_p, 1, 0);
  glue_define (sc, nullp_name, nullp_desc, f_njson_null_p, 1, 0);
  glue_define (sc, objectp_name, objectp_desc, f_njson_object_p, 1, 0);
  glue_define (sc, arrayp_name, arrayp_desc, f_njson_array_p, 1, 0);
  glue_define (sc, stringp_name, stringp_desc, f_njson_string_p, 1, 0);
  glue_define (sc, numberp_name, numberp_desc, f_njson_number_p, 1, 0);
  glue_define (sc, integerp_name, integerp_desc, f_njson_integer_p, 1, 0);
  glue_define (sc, booleanp_name, booleanp_desc, f_njson_boolean_p, 1, 0);
  glue_define (sc, size_name, size_desc, f_njson_size, 1, 0);
  glue_define (sc, emptyp_name, emptyp_desc, f_njson_empty_p, 1, 0);
  glue_define (sc, free_name, free_desc, f_njson_free, 1, 0);
  glue_define (sc, ref_name, ref_desc, f_njson_ref, 2, 32);
  glue_define (sc, set_name, set_desc, f_njson_set, 3, 32);
  glue_define (sc, append_name, append_desc, f_njson_append, 2, 32);
  glue_define (sc, set_x_name, set_x_desc, f_njson_set_x, 3, 32);
  glue_define (sc, append_x_name, append_x_desc, f_njson_append_x, 2, 32);
  glue_define (sc, drop_name, drop_desc, f_njson_drop, 2, 32);
  glue_define (sc, drop_x_name, drop_x_desc, f_njson_drop_x, 2, 32);
  glue_define (sc, merge_name, merge_desc, f_njson_merge, 2, 0);
  glue_define (sc, merge_x_name, merge_x_desc, f_njson_merge_x, 2, 0);
  glue_define (sc, deep_merge_name, deep_merge_desc, f_njson_deep_merge, 2, 0);
  glue_define (sc, deep_merge_x_name, deep_merge_x_desc, f_njson_deep_merge_x, 2, 0);
  glue_define (sc, has_key_name, has_key_desc, f_njson_contains_key_p, 2, 0);
  glue_define (sc, keys_name, keys_desc, f_njson_keys, 1, 0);
  glue_define (sc, object_alist_name, object_alist_desc, f_njson_object_to_alist, 1, 0);
  glue_define (sc, object_hash_name, object_hash_desc, f_njson_object_to_hash_table, 1, 0);
  glue_define (sc, array_list_name, array_list_desc, f_njson_array_to_list, 1, 0);
  glue_define (sc, array_vector_name, array_vector_desc, f_njson_array_to_vector, 1, 0);
  glue_define (sc, schema_report_name, schema_report_desc, f_njson_schema_report, 2, 0);
}

} // namespace goldfish
