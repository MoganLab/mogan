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

#include <algorithm>
#include <argh.h>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include "s7.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>

#include <tbox/platform/file.h>
#include <tbox/platform/path.h>
#include <tbox/tbox.h>

#ifdef TB_CONFIG_OS_WINDOWS
#include <io.h>
#include <windows.h>
#elif TB_CONFIG_OS_MACOSX
#include <limits.h>
#include <mach-o/dyld.h>
#elif defined(__EMSCRIPTEN__)
#include <limits.h>
#else
#include <linux/limits.h>
#endif

#if !defined(TB_CONFIG_OS_WINDOWS)
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#if !defined(__EMSCRIPTEN__)
#include <wordexp.h>
#endif
#endif

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

#ifdef GOLDFISH_WITH_REPL
#include <functional>
#include <isocline.h>
#endif

#define GOLDFISH_VERSION "17.11.44"

#define GOLDFISH_PATH_MAXN TB_PATH_MAXN

static std::vector<std::string> command_args= std::vector<std::string> ();

// Declare environ for non-Windows platforms (needed for f_getenvs)
#if !defined(TB_CONFIG_OS_WINDOWS)
extern char **environ;
#endif

namespace goldfish {
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

namespace fs = std::filesystem;

using nlohmann::json;

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional);

static s7_pointer
f_function_libraries (s7_scheme* sc, s7_pointer args);

static bool
split_library_query (const string& query, string& group, string& library);

static vector<string>
find_function_libraries_in_load_path (s7_scheme* sc, const string& function_name);

static const char* NJSON_HANDLE_TAG = "njson-handle";
struct NjsonState {
  std::thread::id owner_thread_id;
  std::vector<std::unique_ptr<json>> handle_store;
  std::vector<s7_int> handle_generations;
  std::vector<s7_int> handle_free_ids;
  std::vector<std::vector<std::string>> keys_cache_values;
  std::vector<bool> keys_cache_valid;

  NjsonState ()
    : owner_thread_id (std::this_thread::get_id ()),
      handle_store (1),
      handle_generations (1, 0),
      keys_cache_values (1),
      keys_cache_valid (1, false) {}
};

static std::mutex njson_state_registry_mutex;
static std::unordered_map<s7_scheme*, std::unique_ptr<NjsonState>> njson_state_registry;

static NjsonState&
njson_get_or_create_state (s7_scheme* sc) {
  std::lock_guard<std::mutex> lock (njson_state_registry_mutex);
  auto it = njson_state_registry.find (sc);
  if (it == njson_state_registry.end ()) {
    auto inserted = njson_state_registry.emplace (sc, std::make_unique<NjsonState> ());
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
    out = s7_string (key);
    return true;
  }
  error_msg = "json object key must be string?";
  return false;
}

static s7_pointer
njson_error (s7_scheme* sc, const char* type_name, const std::string& msg, s7_pointer culprit) {
  return s7_error (sc, s7_make_symbol (sc, type_name),
                   s7_list (sc, 2, s7_make_string (sc, msg.c_str ()), culprit));
}

static s7_pointer
njson_require_owner_thread (s7_scheme* sc, const char* api_name, s7_pointer culprit) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (state.owner_thread_id == std::this_thread::get_id ()) {
    return nullptr;
  }
  return njson_error (sc, "thread-error",
                      std::string (api_name) + ": must be called from the thread that created this VM", culprit);
}

static s7_pointer
make_njson_handle (s7_scheme* sc, s7_int id) {
  NjsonState& state = njson_get_or_create_state (sc);
  s7_int generation = 0;
  if (id > 0) {
    size_t index = static_cast<size_t> (id);
    if (index < state.handle_generations.size ()) {
      generation = state.handle_generations[index];
    }
  }
  return s7_cons (
    sc, s7_make_symbol (sc, NJSON_HANDLE_TAG), s7_cons (sc, s7_make_integer (sc, id), s7_make_integer (sc, generation)));
}

static bool
is_njson_handle (s7_pointer x, s7_int* id_out = nullptr, s7_int* generation_out = nullptr) {
  if (!s7_is_pair (x)) return false;
  s7_pointer tag = s7_car (x);
  s7_pointer payload = s7_cdr (x);
  if (!s7_is_symbol (tag)) return false;
  if (strcmp (s7_symbol_name (tag), NJSON_HANDLE_TAG) != 0) return false;
  if (!s7_is_pair (payload)) return false;
  s7_pointer id = s7_car (payload);
  s7_pointer generation = s7_cdr (payload);
  if (!s7_is_integer (id) || !s7_is_integer (generation)) return false;
  if (id_out) *id_out = s7_integer (id);
  if (generation_out) *generation_out = s7_integer (generation);
  return true;
}

static bool
extract_njson_handle_id (s7_scheme* sc, s7_pointer handle, s7_int& id, std::string& error_msg) {
  NjsonState& state = njson_get_or_create_state (sc);
  s7_int generation = 0;
  if (!is_njson_handle (handle, &id, &generation)) {
    error_msg = "expected njson handle";
    return false;
  }
  if (id <= 0) {
    error_msg = "invalid njson handle id";
    return false;
  }
  if (generation <= 0) {
    error_msg = "invalid njson handle generation";
    return false;
  }
  size_t index = static_cast<size_t> (id);
  if (index >= state.handle_generations.size ()) {
    error_msg = "njson handle does not exist (may have been freed)";
    return false;
  }
  if (state.handle_generations[index] != generation) {
    error_msg = "njson handle generation mismatch (stale handle)";
    return false;
  }
  if (static_cast<size_t> (id) >= state.handle_store.size () || !state.handle_store[static_cast<size_t> (id)]) {
    error_msg = "njson handle does not exist (may have been freed)";
    return false;
  }
  return true;
}

static json*
njson_value_by_id (s7_scheme* sc, s7_int id) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (id <= 0) return nullptr;
  size_t index = static_cast<size_t> (id);
  if (index >= state.handle_store.size ()) return nullptr;
  return state.handle_store[index].get ();
}

static const json*
njson_value_by_id_const (s7_scheme* sc, s7_int id) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (id <= 0) return nullptr;
  size_t index = static_cast<size_t> (id);
  if (index >= state.handle_store.size ()) return nullptr;
  return state.handle_store[index].get ();
}

static void
njson_ensure_keys_cache_size (s7_scheme* sc, size_t n) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (state.keys_cache_values.size () < n) {
    state.keys_cache_values.resize (n);
    state.keys_cache_valid.resize (n, false);
  }
}

static void
njson_ensure_generations_size (s7_scheme* sc, size_t n) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (state.handle_generations.size () < n) {
    state.handle_generations.resize (n, 0);
  }
}

static void
njson_clear_keys_cache_slot (s7_scheme* sc, s7_int id) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (id <= 0) return;
  size_t index = static_cast<size_t> (id);
  if (index >= state.keys_cache_valid.size ()) return;
  state.keys_cache_values[index].clear ();
  state.keys_cache_valid[index] = false;
}

static void
njson_collect_keys (const json& root, std::vector<std::string>& out) {
  out.clear ();
  if (!root.is_object ()) {
    return;
  }
  out.reserve (root.size ());
  for (auto it = root.begin (); it != root.end (); ++it) {
    out.push_back (it.key ());
  }
}

static s7_pointer
njson_build_keys_list (s7_scheme* sc, const std::vector<std::string>& keys) {
  s7_pointer out = s7_nil (sc);
  for (auto it = keys.rbegin (); it != keys.rend (); ++it) {
    const std::string& key = *it;
    out = s7_cons (sc, s7_make_string_with_length (sc, key.data (), static_cast<s7_int> (key.size ())), out);
  }
  return out;
}

static void
njson_store_keys_cache (s7_scheme* sc, s7_int id, std::vector<std::string>&& keys) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (id <= 0) return;
  size_t index = static_cast<size_t> (id);
  njson_ensure_keys_cache_size (sc, index + 1);
  njson_clear_keys_cache_slot (sc, id);
  state.keys_cache_values[index] = std::move (keys);
  state.keys_cache_valid[index] = true;
}

static bool
njson_try_get_keys_cache (s7_scheme* sc, s7_int id, const std::vector<std::string>*& out) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (id <= 0) return false;
  size_t index = static_cast<size_t> (id);
  if (index >= state.keys_cache_valid.size ()) return false;
  if (!state.keys_cache_valid[index]) return false;
  out = &state.keys_cache_values[index];
  return true;
}

static void
njson_invalidate_keys_cache_if_present (s7_scheme* sc, s7_int id) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (id <= 0) return;
  size_t index = static_cast<size_t> (id);
  if (index >= state.keys_cache_valid.size () || !state.keys_cache_valid[index]) return;
  njson_clear_keys_cache_slot (sc, id);
}

static s7_int
store_njson_value (s7_scheme* sc, json&& value) {
  NjsonState& state = njson_get_or_create_state (sc);
  if (!state.handle_free_ids.empty ()) {
    s7_int id = state.handle_free_ids.back ();
    state.handle_free_ids.pop_back ();
    size_t index = static_cast<size_t> (id);
    njson_ensure_generations_size (sc, index + 1);
    s7_int generation = state.handle_generations[index];
    if (generation <= 0 || generation == (std::numeric_limits<s7_int>::max) ()) {
      generation = 1;
    }
    else {
      generation += 1;
    }
    state.handle_generations[index] = generation;
    state.handle_store[index] = std::make_unique<json> (std::move (value));
    njson_ensure_keys_cache_size (sc, state.handle_store.size ());
    return id;
  }

  state.handle_store.push_back (std::make_unique<json> (std::move (value)));
  state.handle_generations.push_back (1);
  njson_ensure_keys_cache_size (sc, state.handle_store.size ());
  s7_int id = static_cast<s7_int> (state.handle_store.size () - 1);
  return id;
}

static s7_int
store_njson_value (s7_scheme* sc, const json& value) {
  json copied = value;
  return store_njson_value (sc, std::move (copied));
}

static bool
scheme_json_index (s7_pointer key, size_t& out, std::string& error_msg) {
  if (!s7_is_integer (key)) {
    error_msg = "array index must be integer?";
    return false;
  }
  s7_int idx = s7_integer (key);
  if (idx < 0) {
    error_msg = "array index must be non-negative";
    return false;
  }
  out = static_cast<size_t> (idx);
  return true;
}

static bool
collect_path_keys (s7_scheme* sc, s7_pointer list, std::vector<s7_pointer>& out, std::string& error_msg) {
  s7_pointer iter = list;
  while (s7_is_pair (iter)) {
    out.push_back (s7_car (iter));
    iter = s7_cdr (iter);
  }
  if (!s7_is_null (sc, iter)) {
    error_msg = "path keys must be a proper list";
    return false;
  }
  return true;
}

template <typename JsonPtr>
static bool
njson_lookup_core (s7_scheme* sc, JsonPtr root, const std::vector<s7_pointer>& path, size_t steps,
                   JsonPtr& out, std::string& error_msg) {
  JsonPtr cur = root;
  for (size_t i = 0; i < steps; i++) {
    s7_pointer key = path[i];
    if (cur->is_object ()) {
      std::string name;
      if (!scheme_json_key_to_string (sc, key, name, error_msg)) {
        return false;
      }
      auto it = cur->find (name);
      if (it == cur->end ()) {
        error_msg = "path not found: missing object key '" + name + "'";
        return false;
      }
      cur = &(*it);
    }
    else if (cur->is_array ()) {
      size_t idx = 0;
      if (!scheme_json_index (key, idx, error_msg)) {
        return false;
      }
      if (idx >= cur->size ()) {
        error_msg = "path not found: array index out of range (index=" + std::to_string (idx)
                  + ", size=" + std::to_string (cur->size ()) + ")";
        return false;
      }
      cur = &(*cur)[idx];
    }
    else {
      char* key_repr_c = s7_object_to_c_string (sc, key);
      if (key_repr_c) {
        std::string key_repr (key_repr_c);
        free (key_repr_c);
        if (key_repr.size () >= 2 && key_repr.front () == '"' && key_repr.back () == '"') {
          key_repr = key_repr.substr (1, key_repr.size () - 2);
        }
        error_msg = "path not found: missing object key '" + key_repr + "'";
      }
      else {
        error_msg = "path not found: missing object key '<unknown>'";
      }
      return false;
    }
  }
  out = cur;
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
    error_msg = "path cannot be empty";
    return false;
  }

  if (!njson_lookup_core (sc, &root, path, path.size () - 1, parent, error_msg)) {
    return false;
  }
  last_key = path.back ();
  return true;
}

static bool
lookup_path_mutable (s7_scheme* sc, json& root, const std::vector<s7_pointer>& path, json*& out, std::string& error_msg) {
  return njson_lookup_core (sc, &root, path, path.size (), out, error_msg);
}

static bool
scheme_to_njson_scalar_or_handle (s7_scheme* sc, s7_pointer value, json& out, std::string& error_msg) {
  if (is_njson_handle (value)) {
    s7_int id = 0;
    if (!extract_njson_handle_id (sc, value, id, error_msg)) {
      return false;
    }
    const json* source = njson_value_by_id_const (sc, id);
    if (!source) {
      error_msg = "njson handle does not exist (may have been freed)";
      return false;
    }
    out = *source;
    return true;
  }

  if (s7_is_string (value)) {
    out = s7_string (value);
    return true;
  }
  if (s7_is_boolean (value)) {
    out = s7_boolean (sc, value);
    return true;
  }
  if (s7_is_integer (value)) {
    out = static_cast<long long> (s7_integer (value));
    return true;
  }
  if (s7_is_real (value)) {
    double real_value = s7_number_to_real (sc, value);
    if (!std::isfinite (real_value)) {
      error_msg = "number must be finite (NaN/Inf are not valid JSON numbers)";
      return false;
    }
    out = real_value;
    return true;
  }
  if (s7_is_number (value)) {
    error_msg = "number must be real and finite";
    return false;
  }
  if (s7_is_symbol (value)) {
    const char* symbol_name = s7_symbol_name (value);
    if (strcmp (symbol_name, "null") == 0) {
      out = nullptr;
      return true;
    }
    error_msg = "symbol value must be null; use boolean? for true/false and string? for text";
    return false;
  }

  error_msg = "value must be njson handle, string?, number?, boolean?, or null symbol";
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
    unsigned long long v = value.get<unsigned long long> ();
    if (v > static_cast<unsigned long long> ((std::numeric_limits<s7_int>::max) ())) {
      return s7_make_real (sc, static_cast<double> (v));
    }
    return s7_make_integer (sc, static_cast<s7_int> (v));
  }
  if (value.is_number_float ()) {
    return s7_make_real (sc, value.get<double> ());
  }
  if (value.is_string ()) {
    const auto& text = value.get_ref<const std::string&> ();
    return s7_make_string (sc, text.c_str ());
  }
  return s7_nil (sc);
}

enum class njson_scheme_tree_mode {
  alist_list,
  hash_vector
};

static s7_pointer njson_value_to_scheme_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode);

static s7_pointer
njson_object_to_alist_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  // Match (liii json): empty object is '(()) so {} stays distinct from [].
  if (value.empty ()) {
    return s7_cons (sc, s7_nil (sc), s7_nil (sc));
  }
  s7_pointer out = s7_nil (sc);
  for (auto it = value.begin (); it != value.end (); ++it) {
    const std::string& key = it.key ();
    s7_pointer key_s7 = s7_make_string_with_length (sc, key.data (), static_cast<s7_int> (key.size ()));
    s7_pointer val_s7 = njson_value_to_scheme_tree (sc, it.value (), mode);
    out = s7_cons (sc, s7_cons (sc, key_s7, val_s7), out);
  }
  return s7_reverse (sc, out);
}

static s7_pointer
njson_array_to_list_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  s7_pointer out = s7_nil (sc);
  for (auto it = value.begin (); it != value.end (); ++it) {
    out = s7_cons (sc, njson_value_to_scheme_tree (sc, *it, mode), out);
  }
  return s7_reverse (sc, out);
}

static s7_pointer
njson_object_to_hash_table_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  s7_pointer out = s7_make_hash_table (sc, static_cast<s7_int> (value.size ()));
  for (auto it = value.begin (); it != value.end (); ++it) {
    const std::string& key = it.key ();
    s7_hash_table_set (sc, out, s7_make_string_with_length (sc, key.data (), static_cast<s7_int> (key.size ())),
                       njson_value_to_scheme_tree (sc, it.value (), mode));
  }
  return out;
}

static s7_pointer
njson_array_to_vector_tree (s7_scheme* sc, const json& value, njson_scheme_tree_mode mode) {
  s7_pointer out = s7_make_vector (sc, static_cast<s7_int> (value.size ()));
  for (size_t i = 0; i < value.size (); i++) {
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

enum class njson_structure_root_kind {
  object,
  array
};

static s7_pointer
njson_run_structure_conversion (s7_scheme* sc, s7_pointer args, const char* api_name, njson_structure_root_kind root_kind,
                                njson_scheme_tree_mode mode) {
  s7_pointer thread_err = njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }

  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (sc, id);
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

  return njson_value_to_scheme_tree (sc, *root, mode);
}

static s7_pointer
njson_value_to_scheme_or_handle (s7_scheme* sc, const json& value) {
  if (value.is_object () || value.is_array ()) {
    s7_int id = store_njson_value (sc, value);
    return make_njson_handle (sc, id);
  }
  return njson_scalar_value_to_scheme (sc, value);
}

static s7_pointer
f_njson_string_to_json (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-string->json", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input = s7_car (args);
  if (!s7_is_string (input)) {
    return njson_error (sc, "type-error", "g_njson-string->json: input must be string", input);
  }

  try {
    json parsed = json::parse (s7_string (input));
    return make_njson_handle (sc, store_njson_value (sc, std::move (parsed)));
  }
  catch (const json::parse_error& err) {
    return njson_error (sc, "parse-error", err.what (), input);
  }
}

static s7_pointer
f_njson_json_to_string (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-json->string", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  input = s7_car (args);
  json        encoded;
  std::string error_msg;
  if (!scheme_to_njson_scalar_or_handle (sc, input, encoded, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-json->string: " + error_msg, input);
  }
  std::string dumped = encoded.dump ();
  return s7_make_string (sc, dumped.c_str ());
}

static s7_pointer
f_njson_format_string (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-format-string", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input = s7_car (args);
  if (!s7_is_string (input)) {
    return njson_error (sc, "type-error", "g_njson-format-string: input must be string", input);
  }

  s7_int     indent = 2;
  s7_pointer rest = s7_cdr (args);
  if (!s7_is_null (sc, rest)) {
    s7_pointer indent_arg = s7_car (rest);
    if (!s7_is_integer (indent_arg)) {
      return njson_error (sc, "type-error", "g_njson-format-string: indent must be integer?", indent_arg);
    }
    indent = s7_integer (indent_arg);
    if (indent < 0) {
      return njson_error (sc, "value-error", "g_njson-format-string: indent must be >= 0", indent_arg);
    }
  }

  try {
    json parsed = json::parse (s7_string (input));
    std::string dumped = parsed.dump (static_cast<int> (indent));
    return s7_make_string (sc, dumped.c_str ());
  }
  catch (const json::parse_error& err) {
    return njson_error (sc, "parse-error", err.what (), input);
  }
}

static s7_pointer
f_njson_handle_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-handle?", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input = s7_car (args);
  return s7_make_boolean (sc, is_njson_handle (input));
}

template <typename HandlePredicate, typename ScalarPredicate>
static s7_pointer
njson_run_value_type_predicate (s7_scheme* sc, s7_pointer args, const char* api_name, HandlePredicate handle_pred,
                                ScalarPredicate scalar_pred) {
  s7_pointer thread_err = njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer input = s7_car (args);
  if (!is_njson_handle (input)) {
    return s7_make_boolean (sc, scalar_pred (input));
  }

  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, input, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, input);
  }
  const json* value = njson_value_by_id_const (sc, id);
  if (!value) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", input);
  }
  return s7_make_boolean (sc, handle_pred (*value));
}

static s7_pointer
f_njson_null_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (
    sc, args, "g_njson-null?", [] (const json& value) { return value.is_null (); }, [] (s7_pointer value) {
      return s7_is_symbol (value) && strcmp (s7_symbol_name (value), "null") == 0;
    });
}

static s7_pointer
f_njson_object_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (sc, args, "g_njson-object?", [] (const json& value) { return value.is_object (); },
                                         [] (s7_pointer value) {
                                           (void) value;
                                           return false;
                                         });
}

static s7_pointer
f_njson_array_p (s7_scheme* sc, s7_pointer args) {
  return njson_run_value_type_predicate (sc, args, "g_njson-array?", [] (const json& value) { return value.is_array (); },
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
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-free", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-free: " + error_msg, handle);
  }
  NjsonState& state = njson_get_or_create_state (sc);
  njson_clear_keys_cache_slot (sc, id);
  state.handle_store[static_cast<size_t> (id)].reset ();
  state.handle_free_ids.push_back (id);
  return s7_t (sc);
}

static s7_pointer
f_njson_size (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-size", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-size: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-size: njson handle does not exist (may have been freed)", handle);
  }

  if (root->is_object () || root->is_array ()) {
    return s7_make_integer (sc, static_cast<s7_int> (root->size ()));
  }
  return s7_make_integer (sc, 0);
}

static s7_pointer
f_njson_empty_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-empty?", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-empty?: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error",
                        "g_njson-empty?: njson handle does not exist (may have been freed)", handle);
  }

  if (root->is_object () || root->is_array ()) {
    return s7_make_boolean (sc, root->empty ());
  }
  return s7_t (sc);
}

static s7_pointer
f_njson_ref (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-ref", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
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

  const json* root = njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-ref: njson handle does not exist (may have been freed)", handle);
  }

  const json* found_value = nullptr;
  if (!lookup_path_const (sc, *root, path, found_value, error_msg)) {
    return njson_error (sc, "key-error", "g_njson-ref: " + error_msg, handle);
  }
  return njson_value_to_scheme_or_handle (sc, *found_value);
}

enum class njson_update_op {
  set,
  append,
  drop
};

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
  handle = s7_car (args);
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, handle);
  }

  std::vector<s7_pointer> tokens;
  if (!collect_path_keys (sc, s7_cdr (args), tokens, error_msg)) {
    return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, handle);
  }

  if (njson_update_needs_value (op)) {
    size_t min_tokens = (op == njson_update_op::append) ? 1 : 2;
    if (tokens.size () < min_tokens) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + njson_update_expected_argv (op), handle);
    }
    path.assign (tokens.begin (), tokens.end () - 1);
    s7_pointer value_token = tokens.back ();
    if (!scheme_to_njson_scalar_or_handle (sc, value_token, value_json, error_msg)) {
      return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, value_token);
    }
  }
  else {
    if (tokens.empty ()) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + njson_update_expected_argv (op), handle);
    }
    path = std::move (tokens);
  }
  return nullptr;
}

static s7_pointer
njson_apply_update_on_root (s7_scheme* sc, json& root, const std::vector<s7_pointer>& path, const json& value_json,
                            njson_update_op op, const char* api_name, s7_pointer handle) {
  if (op == njson_update_op::append) {
    std::string error_msg;
    json* target = &root;
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
  json* parent = nullptr;
  s7_pointer last_key = s7_nil (sc);
  if (!lookup_path_parent_mutable (sc, root, path, parent, last_key, error_msg)) {
    return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, handle);
  }

  if (parent->is_object ()) {
    std::string key_name;
    if (!scheme_json_key_to_string (sc, last_key, key_name, error_msg)) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, last_key);
    }

    if (op == njson_update_op::set) {
      (*parent)[key_name] = value_json;
    }
    else {
      auto it = parent->find (key_name);
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
    size_t idx = 0;
    if (!scheme_json_index (last_key, idx, error_msg)) {
      return njson_error (sc, "key-error", std::string (api_name) + ": " + error_msg, last_key);
    }

    if (op == njson_update_op::set) {
      if (idx < parent->size ()) {
        (*parent)[idx] = value_json;
      }
      else {
        return njson_error (
          sc, "key-error",
          std::string (api_name) + ": array index out of range (index=" + std::to_string (idx)
            + ", size=" + std::to_string (parent->size ()) + ")",
          last_key);
      }
    }
    else {
      if (idx < parent->size ()) {
        parent->erase (parent->begin () + static_cast<json::difference_type> (idx));
      }
      else {
        return njson_error (
          sc, "key-error",
          std::string (api_name) + ": path not found: array index out of range (index=" + std::to_string (idx)
            + ", size=" + std::to_string (parent->size ()) + ")",
          last_key);
      }
    }
    return nullptr;
  }

  if (op == njson_update_op::drop) {
    return njson_error (sc, "key-error", std::string (api_name) + ": path not found: cannot drop from non-container value",
                        last_key);
  }
  if (op == njson_update_op::set) {
    return njson_error (sc, "key-error", std::string (api_name) + ": set target must be array or object", last_key);
  }
  return nullptr;
}

static s7_pointer
njson_run_update (s7_scheme* sc, s7_pointer args, const char* api_name, njson_update_op op, bool in_place) {
  s7_pointer thread_err = njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer handle = s7_nil (sc);
  s7_int id = 0;
  std::vector<s7_pointer> path;
  json value_json;
  s7_pointer err = njson_parse_update_request (sc, args, api_name, op, handle, id, path, value_json);
  if (err) {
    return err;
  }

  if (in_place) {
    json* root = njson_value_by_id (sc, id);
    if (!root) {
      return njson_error (sc, "type-error",
                          std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
    }
    err = njson_apply_update_on_root (sc, *root, path, value_json, op, api_name, handle);
    if (err) {
      return err;
    }
    // Keep write-path fast: only invalidate; keys will be rebuilt lazily on next njson-keys call.
    njson_invalidate_keys_cache_if_present (sc, id);
    return handle;
  }

  const json* root = njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
  }
  json out = *root;
  err = njson_apply_update_on_root (sc, out, path, value_json, op, api_name, handle);
  if (err) {
    return err;
  }
  return make_njson_handle (sc, store_njson_value (sc, std::move (out)));
}

enum class njson_merge_mode {
  shallow,
  deep
};


static s7_pointer
njson_run_merge (s7_scheme* sc, s7_pointer args, const char* api_name, njson_merge_mode mode, bool in_place) {
  s7_pointer thread_err = njson_require_owner_thread (sc, api_name, s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle = s7_car (args);
  s7_pointer  source_input = s7_cadr (args);
  s7_int      id = 0;
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
  bool merge_objects = (mode == njson_merge_mode::deep);

  if (in_place) {
    json* target = njson_value_by_id (sc, id);
    if (!target) {
      return njson_error (sc, "type-error",
                          std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
    }
    if (!target->is_object ()) {
      return njson_error (sc, "type-error", std::string (api_name) + ": merge target must be object", handle);
    }
    try {
      target->update (source_json, merge_objects);
    }
    catch (const std::exception& err) {
      return njson_error (sc, "type-error", std::string (api_name) + ": " + std::string (err.what ()), source_input);
    }
    njson_invalidate_keys_cache_if_present (sc, id);
    return handle;
  }

  const json* target = njson_value_by_id_const (sc, id);
  if (!target) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
  }
  if (!target->is_object ()) {
    return njson_error (sc, "type-error", std::string (api_name) + ": merge target must be object", handle);
  }
  json out = *target;
  try {
    out.update (source_json, merge_objects);
  }
  catch (const std::exception& err) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + std::string (err.what ()), source_input);
  }
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
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-contains-key?", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle = s7_car (args);
  s7_pointer  key = s7_cadr (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-contains-key?: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error",
                        "g_njson-contains-key?: njson handle does not exist (may have been freed)", handle);
  }
  if (!root->is_object ()) {
    return s7_f (sc);
  }

  std::string key_name;
  if (!scheme_json_key_to_string (sc, key, key_name, error_msg)) {
    return njson_error (sc, "key-error", "g_njson-contains-key?: " + error_msg, key);
  }
  return s7_make_boolean (sc, root->contains (key_name));
}

static s7_pointer
f_njson_keys (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-keys", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-keys: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (sc, id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-keys: njson handle does not exist (may have been freed)", handle);
  }
  if (!root->is_object ()) {
    return s7_nil (sc);
  }

  const std::vector<std::string>* cached = nullptr;
  if (njson_try_get_keys_cache (sc, id, cached)) {
    return njson_build_keys_list (sc, *cached);
  }

  std::vector<std::string> keys;
  njson_collect_keys (*root, keys);
  njson_store_keys_cache (sc, id, std::move (keys));
  const std::vector<std::string>* stored = nullptr;
  if (njson_try_get_keys_cache (sc, id, stored)) {
    return njson_build_keys_list (sc, *stored);
  }
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

  void
  error (const json::json_pointer& ptr, const json& instance, const std::string& message) override {
    std::string dumped;
    try {
      dumped = instance.dump ();
    }
    catch (...) {
      dumped = "<failed-to-dump-instance>";
    }
    entries.push_back (njson_schema_error_entry{ptr.to_string (), message, dumped});
  }
};

static s7_pointer
njson_schema_errors_to_scheme (s7_scheme* sc, const std::vector<njson_schema_error_entry>& errors) {
  s7_pointer out = s7_nil (sc);
  for (auto it = errors.rbegin (); it != errors.rend (); ++it) {
    s7_pointer row = s7_make_hash_table (sc, 3);
    s7_hash_table_set (sc, row, s7_make_symbol (sc, "instance-path"), s7_make_string (sc, it->instance_path.c_str ()));
    s7_hash_table_set (sc, row, s7_make_symbol (sc, "message"), s7_make_string (sc, it->message.c_str ()));
    s7_hash_table_set (sc, row, s7_make_symbol (sc, "instance"), s7_make_string (sc, it->instance_dump.c_str ()));
    out = s7_cons (sc, row, out);
  }
  return out;
}

static s7_pointer
njson_run_schema_validation (s7_scheme* sc, const char* api_name, s7_pointer args,
                             std::vector<njson_schema_error_entry>& errors_out) {
  s7_pointer  schema_input = s7_car (args);
  s7_pointer  instance_input = s7_cadr (args);
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
  }
  catch (const std::exception& err) {
    return njson_error (sc, "schema-error", std::string (api_name) + ": " + std::string (err.what ()), schema_input);
  }

  njson_collecting_error_handler err_handler;
  try {
    validator.validate (instance_json, err_handler);
  }
  catch (const std::exception& err) {
    return njson_error (sc, "validation-error", std::string (api_name) + ": " + std::string (err.what ()), instance_input);
  }
  errors_out = std::move (err_handler.entries);
  return nullptr;
}

static s7_pointer
f_njson_schema_report (s7_scheme* sc, s7_pointer args) {
  s7_pointer thread_err = njson_require_owner_thread (sc, "g_njson-schema-report", s7_car (args));
  if (thread_err) {
    return thread_err;
  }
  std::vector<njson_schema_error_entry> errors;
  s7_pointer err = njson_run_schema_validation (sc, "g_njson-schema-report", args, errors);
  if (err) {
    return err;
  }

  s7_pointer report = s7_make_hash_table (sc, 3);
  s7_hash_table_set (sc, report, s7_make_symbol (sc, "valid?"), s7_make_boolean (sc, errors.empty ()));
  s7_hash_table_set (sc, report, s7_make_symbol (sc, "error-count"), s7_make_integer (sc, static_cast<s7_int> (errors.size ())));
  s7_hash_table_set (sc, report, s7_make_symbol (sc, "errors"), njson_schema_errors_to_scheme (sc, errors));
  return report;
}

inline void
glue_njson (s7_scheme* sc) {
  njson_register_state (sc);
  const char* parse_name = "g_njson-string->json";
  const char* parse_desc = "(g_njson-string->json json-string) => njson-handle";
  const char* dump_name  = "g_njson-json->string";
  const char* dump_desc  = "(g_njson-json->string handle-or-scalar) => strict-json-string";
  const char* format_name = "g_njson-format-string";
  const char* format_desc = "(g_njson-format-string json-string :optional indent) => strict-json-string";
  const char* handlep_name = "g_njson-handle?";
  const char* handlep_desc = "(g_njson-handle? x) => boolean?";
  const char* nullp_name = "g_njson-null?";
  const char* nullp_desc = "(g_njson-null? x) => boolean?";
  const char* objectp_name = "g_njson-object?";
  const char* objectp_desc = "(g_njson-object? x) => boolean?";
  const char* arrayp_name = "g_njson-array?";
  const char* arrayp_desc = "(g_njson-array? x) => boolean?";
  const char* stringp_name = "g_njson-string?";
  const char* stringp_desc = "(g_njson-string? x) => boolean?";
  const char* numberp_name = "g_njson-number?";
  const char* numberp_desc = "(g_njson-number? x) => boolean?";
  const char* integerp_name = "g_njson-integer?";
  const char* integerp_desc = "(g_njson-integer? x) => boolean?";
  const char* booleanp_name = "g_njson-boolean?";
  const char* booleanp_desc = "(g_njson-boolean? x) => boolean?";
  const char* size_name = "g_njson-size";
  const char* size_desc = "(g_njson-size handle) => integer?";
  const char* emptyp_name = "g_njson-empty?";
  const char* emptyp_desc = "(g_njson-empty? handle) => boolean?";
  const char* free_name = "g_njson-free";
  const char* free_desc = "(g_njson-free handle) => boolean?";
  const char* ref_name = "g_njson-ref";
  const char* ref_desc = "(g_njson-ref handle key ...) => scalar-or-handle";
  const char* set_name = "g_njson-set";
  const char* set_desc = "(g_njson-set handle key ... value) => new-handle";
  const char* append_name = "g_njson-append";
  const char* append_desc = "(g_njson-append handle [key ...] value) => new-handle";
  const char* set_x_name = "g_njson-set!";
  const char* set_x_desc = "(g_njson-set! handle key ... value) => same-handle";
  const char* append_x_name = "g_njson-append!";
  const char* append_x_desc = "(g_njson-append! handle [key ...] value) => same-handle";
  const char* drop_name = "g_njson-drop";
  const char* drop_desc = "(g_njson-drop handle key ...) => new-handle";
  const char* drop_x_name = "g_njson-drop!";
  const char* drop_x_desc = "(g_njson-drop! handle key ...) => same-handle";
  const char* merge_name = "g_njson-merge";
  const char* merge_desc = "(g_njson-merge handle other-object) => new-handle";
  const char* merge_x_name = "g_njson-merge!";
  const char* merge_x_desc = "(g_njson-merge! handle other-object) => same-handle";
  const char* deep_merge_name = "g_njson-deep-merge";
  const char* deep_merge_desc = "(g_njson-deep-merge handle other-object) => new-handle";
  const char* deep_merge_x_name = "g_njson-deep-merge!";
  const char* deep_merge_x_desc = "(g_njson-deep-merge! handle other-object) => same-handle";
  const char* has_key_name = "g_njson-contains-key?";
  const char* has_key_desc = "(g_njson-contains-key? handle key) => boolean?";
  const char* keys_name = "g_njson-keys";
  const char* keys_desc = "(g_njson-keys handle) => (list-of string?)";
  const char* object_alist_name = "g_njson-object->alist";
  const char* object_alist_desc = "(g_njson-object->alist object-handle) => alist";
  const char* object_hash_name = "g_njson-object->hash-table";
  const char* object_hash_desc = "(g_njson-object->hash-table object-handle) => hash-table";
  const char* array_list_name = "g_njson-array->list";
  const char* array_list_desc = "(g_njson-array->list array-handle) => list";
  const char* array_vector_name = "g_njson-array->vector";
  const char* array_vector_desc = "(g_njson-array->vector array-handle) => vector";
  const char* schema_report_name = "g_njson-schema-report";
  const char* schema_report_desc = "(g_njson-schema-report schema-handle instance) => hash-table";
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

static s7_pointer
response2hashtable (s7_scheme* sc, cpr::Response r) {
  s7_pointer ht= s7_make_hash_table (sc, 8);
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "status-code"), s7_make_integer (sc, r.status_code));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "url"), s7_make_string (sc, r.url.c_str()));
  s7_hash_table_set (sc, ht, s7_make_symbol(sc, "elapsed"), s7_make_real (sc, r.elapsed));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "text"), s7_make_string (sc, r.text.c_str ()));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "reason"), s7_make_string (sc, r.reason.c_str ()));
  s7_pointer headers= s7_make_hash_table(sc, r.header.size());
  for (const auto &header : r.header) {
    const auto key= header.first.c_str ();
    std::string key_lower = header.first;
    std::transform(key_lower.begin(), key_lower.end(),
                   key_lower.begin(), ::tolower);
    const auto value= header.second.c_str ();
    s7_hash_table_set(sc, headers,
                      s7_make_string(sc, key_lower.c_str()),
                      s7_make_string(sc, value));
  }
  s7_hash_table_set (sc, ht, s7_make_symbol(sc, "headers"), headers);

  return ht;
}

inline cpr::Parameters
to_cpr_parameters (s7_scheme* sc, s7_pointer args) {
  cpr::Parameters params = cpr::Parameters{};
  s7_pointer iter = args;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair = s7_car (iter);
    params.Add (cpr::Parameter (s7_string (s7_car (pair)),
                                s7_string (s7_cdr (pair))));
    iter = s7_cdr (iter);
  }
  return params;
}

inline cpr::Header
to_cpr_headers (s7_scheme* sc, s7_pointer args) {
  cpr::Header headers = cpr::Header{};
  s7_pointer iter = args;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair = s7_car (iter);
    headers.insert ({s7_string (s7_car (pair)),
                     s7_string (s7_cdr (pair))});
    iter = s7_cdr (iter);
  }
  return headers;
}

inline cpr::Proxies
to_cpr_proxies (s7_scheme* sc, s7_pointer args) {
  std::map<std::string, std::string> proxy_map;
  s7_pointer iter = args;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair = s7_car (iter);
    proxy_map[s7_string (s7_car (pair))] = s7_string (s7_cdr (pair));
    iter = s7_cdr (iter);
  }
  return cpr::Proxies (proxy_map);
}

static cpr::Part
to_cpr_multipart_part (s7_scheme* sc, s7_pointer part_spec) {
  std::string name;
  std::string value;
  std::string file_path;
  std::string filename;
  std::string content_type;
  bool has_file = false;

  s7_pointer iter = part_spec;
  while (!s7_is_null (sc, iter)) {
    s7_pointer entry = s7_car (iter);
    s7_pointer raw_key = s7_car (entry);
    const char* key = s7_is_symbol (raw_key)
                      ? s7_symbol_name (raw_key)
                      : s7_string (raw_key);
    const char* raw_value = s7_string (s7_cdr (entry));

    if (strcmp (key, "name") == 0) {
      name = raw_value;
    } else if (strcmp (key, "value") == 0) {
      value = raw_value;
    } else if (strcmp (key, "file") == 0) {
      file_path = raw_value;
      has_file = true;
    } else if (strcmp (key, "filename") == 0) {
      filename = raw_value;
    } else if (strcmp (key, "content-type") == 0) {
      content_type = raw_value;
    }

    iter = s7_cdr (iter);
  }

  if (has_file) {
    cpr::Files files;
    if (filename.empty ()) {
      files.push_back (cpr::File (file_path));
    } else {
      files.push_back (cpr::File (file_path, filename));
    }
    return cpr::Part (name, files, content_type);
  }

  return cpr::Part (name, value, content_type);
}

static void
append_cpr_multipart_file_parts (s7_scheme* sc, s7_pointer files, std::vector<cpr::Part>& parts) {
  s7_pointer iter = files;
  while (!s7_is_null (sc, iter)) {
    parts.push_back (to_cpr_multipart_part (sc, s7_car (iter)));
    iter = s7_cdr (iter);
  }
}

static void
append_cpr_multipart_form_parts (s7_scheme* sc, s7_pointer data, std::vector<cpr::Part>& parts) {
  s7_pointer iter = data;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair = s7_car (iter);
    parts.push_back (cpr::Part (s7_string (s7_car (pair)),
                                s7_string (s7_cdr (pair))));
    iter = s7_cdr (iter);
  }
}

static cpr::Multipart
to_cpr_post_multipart (s7_scheme* sc, s7_pointer data, s7_pointer files) {
  std::vector<cpr::Part> parts;
  append_cpr_multipart_form_parts (sc, data, parts);
  append_cpr_multipart_file_parts (sc, files, parts);
  return cpr::Multipart (parts);
}

static s7_pointer
f_http_head (s7_scheme* sc, s7_pointer args) {
  const char* url= s7_string (s7_car (args));
  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  cpr::Response r= session.Head ();
  return response2hashtable (sc, r);
}

inline void
glue_http_head (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);
  const char* s_http_head = "g_http-head";
  const char* d_http_head = "(g_http-head url ...) => hash-table?";
  auto func_http_head= s7_make_typed_function (sc, s_http_head, f_http_head, 1, 0, false, d_http_head, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, s_http_head), func_http_head);
}

static s7_pointer
f_http_get (s7_scheme* sc, s7_pointer args) {
  const char* url= s7_string (s7_car (args));
  s7_pointer params= s7_cadr (args);
  s7_pointer headers= s7_caddr (args);
  s7_pointer proxy= s7_cadddr (args);
  s7_pointer callback= s7_car (s7_cddddr (args));
  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header cpr_headers = to_cpr_headers (sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies (sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  session.SetHeader (cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session.SetProxies(cpr_proxies);
  }

  if (s7_is_procedure (callback)) {
    session.SetWriteCallback (cpr::WriteCallback{[sc, callback](const std::string_view& data, intptr_t) -> bool {
      s7_pointer data_str = s7_make_string_with_length (sc, data.data (), data.length ());
      s7_pointer call_args = s7_cons (sc, data_str, s7_nil (sc));

      s7_pointer ret = s7_call (sc, callback, call_args);
      if (s7_is_boolean (ret)) {
        return s7_boolean (sc, ret);
      }

      return true;
    }});

    try {
      cpr::Response response = session.Get ();
    } catch (const std::exception& e) {
      return s7_make_integer (sc, 500);
    }
    return s7_undefined (sc);
  }

  cpr::Response r= session.Get ();
  return response2hashtable (sc, r);
}

inline void
glue_http_get (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);
  const char* s_http_get= "g_http-get";
  const char* d_http_get= "(g_http-get url params headers proxy callback) => hash-table? | undefined";
  auto func_http_get= s7_make_typed_function (sc, s_http_get, f_http_get, 5, 0, false, d_http_get, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, s_http_get), func_http_get);
}

static s7_pointer
f_http_post (s7_scheme* sc, s7_pointer args) {
  const char* url= s7_string (s7_car (args));
  s7_pointer params= s7_cadr (args);
  s7_pointer body_or_data = s7_caddr (args);
  s7_pointer headers= s7_cadddr (args);
  s7_pointer proxy= s7_car (s7_cddddr (args));
  s7_pointer files= s7_cadr (s7_cddddr (args));
  s7_pointer callback= s7_list_ref (sc, args, 6);
  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header cpr_headers = to_cpr_headers (sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies (sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  session.SetHeader (cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session.SetProxies(cpr_proxies);
  }

  if (s7_is_list(sc, files) && !s7_is_null(sc, files)) {
    session.SetMultipart (to_cpr_post_multipart (sc, body_or_data, files));
  }
  else {
    const char* body= s7_string (body_or_data);
    cpr::Body cpr_body= cpr::Body (body);
    session.SetBody (cpr_body);
  }

  if (s7_is_procedure (callback)) {
    session.SetWriteCallback (cpr::WriteCallback{[sc, callback](const std::string_view& data, intptr_t) -> bool {
      s7_pointer data_str = s7_make_string_with_length (sc, data.data (), data.length ());
      s7_pointer call_args = s7_cons (sc, data_str, s7_nil (sc));

      s7_pointer ret = s7_call (sc, callback, call_args);
      if (s7_is_boolean (ret)) {
        return s7_boolean (sc, ret);
      }

      return true;
    }});

    try {
      cpr::Response response = session.Post ();
    } catch (const std::exception& e) {
      return s7_make_integer (sc, 500);
    }
    return s7_undefined (sc);
  }

  cpr::Response r= session.Post ();
  return response2hashtable (sc, r);
}

inline void
glue_http_post (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);
  const char* name= "g_http-post";
  const char* doc= "(g_http-post url params body-or-data headers proxy files callback) => hash-table? | undefined";
  auto func_http_post= s7_make_typed_function (sc, name, f_http_post, 7, 0, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func_http_post);
}

inline void
glue_http (s7_scheme* sc) {
  glue_http_head (sc);
  glue_http_get (sc);
  glue_http_post (sc);
}

// -------------------------------- Async HTTP --------------------------------
// Data structure to store async HTTP request state
struct AsyncHttpRequest {
  s7_scheme* sc;
  s7_pointer callback;
  int gc_loc;
  std::shared_ptr<cpr::Session> session;  // Keep session alive
  cpr::AsyncResponse async_response;
  bool completed;
  cpr::Response response;
  std::mutex mutex;
  
  AsyncHttpRequest(s7_scheme* scheme, s7_pointer cb, int gc_protect_loc, 
                   std::shared_ptr<cpr::Session> sess, cpr::AsyncResponse&& ar)
    : sc(scheme), callback(cb), gc_loc(gc_protect_loc), 
      session(std::move(sess)), async_response(std::move(ar)), completed(false) {}
};

// Global list of pending async requests
static std::mutex g_async_requests_mutex;
static std::vector<std::shared_ptr<AsyncHttpRequest>> g_async_requests;

// Check if any async requests have completed and process their callbacks
// This function should be called periodically from the main thread
// Returns the number of callbacks executed
static int
process_async_http_callbacks () {
  std::vector<std::shared_ptr<AsyncHttpRequest>> completed_requests;
  
  // Find completed requests
  {
    std::lock_guard<std::mutex> lock(g_async_requests_mutex);
    for (auto it = g_async_requests.begin(); it != g_async_requests.end(); ) {
      bool is_ready = false;
      {
        std::lock_guard<std::mutex> req_lock((*it)->mutex);
        if (!(*it)->completed) {
          // Check if the future is ready (non-blocking)
          if ((*it)->async_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            (*it)->response = (*it)->async_response.get();
            (*it)->completed = true;
            is_ready = true;
          }
        }
      }
      
      if (is_ready) {
        completed_requests.push_back(*it);
        it = g_async_requests.erase(it);
      } else {
        ++it;
      }
    }
  }
  
  // Execute callbacks for completed requests (outside the lock)
  for (auto& req : completed_requests) {
    s7_pointer ht = response2hashtable(req->sc, req->response);
    s7_call(req->sc, req->callback, s7_cons(req->sc, ht, s7_nil(req->sc)));
    s7_gc_unprotect_at(req->sc, req->gc_loc);
  }
  
  return static_cast<int>(completed_requests.size());
}

// Start an async HTTP GET request
static s7_pointer
f_http_async_get (s7_scheme* sc, s7_pointer args) {
  const char* url = s7_string(s7_car(args));
  s7_pointer params = s7_cadr(args);
  s7_pointer headers = s7_caddr(args);
  s7_pointer proxy = s7_cadddr(args);
  s7_pointer callback = s7_car(s7_cddddr(args));
  
  if (!s7_is_procedure(callback)) {
    return s7_error(sc, s7_make_symbol(sc, "type-error"),
                    s7_list(sc, 2, s7_make_string(sc, "http-async-get: callback must be a procedure"), callback));
  }
  
  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header cpr_headers = to_cpr_headers (sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies (sc, proxy);
  
  // Protect callback from GC
  int gc_loc = s7_gc_protect(sc, callback);
  
  // Create session on heap with shared_ptr to keep it alive
  auto session = std::make_shared<cpr::Session>();
  session->SetUrl(cpr::Url(url));
  session->SetParameters(cpr_params);
  session->SetHeader(cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session->SetProxies(cpr_proxies);
  }
  
  // Start async request using libcpr's built-in thread pool
  // Session is captured by shared_ptr, so it stays alive until async operation completes
  auto async_resp = session->GetAsync();
  
  // Store the request (session is also stored to keep reference)
  auto req = std::make_shared<AsyncHttpRequest>(sc, callback, gc_loc, session, std::move(async_resp));
  {
    std::lock_guard<std::mutex> lock(g_async_requests_mutex);
    g_async_requests.push_back(req);
  }
  
  return s7_make_boolean(sc, true);
}

inline void
glue_http_async_get (s7_scheme* sc) {
  s7_pointer cur_env = s7_curlet(sc);
  const char* name = "g_http-async-get";
  const char* doc = "(g_http-async-get url params headers proxy callback) => boolean, start async http get. callback receives response hashtable. Use g_http-poll to check for completion.";
  auto func = s7_make_typed_function(sc, name, f_http_async_get, 5, 0, false, doc, NULL);
  s7_define(sc, cur_env, s7_make_symbol(sc, name), func);
}

// Start an async HTTP POST request
static s7_pointer
f_http_async_post (s7_scheme* sc, s7_pointer args) {
  const char* url = s7_string(s7_car(args));
  s7_pointer params = s7_cadr(args);
  const char* body = s7_string(s7_caddr(args));
  s7_pointer headers = s7_cadddr(args);
  s7_pointer proxy = s7_car(s7_cddddr(args));
  s7_pointer callback = s7_cadr(s7_cddddr(args));
  
  if (!s7_is_procedure(callback)) {
    return s7_error(sc, s7_make_symbol(sc, "type-error"),
                    s7_list(sc, 2, s7_make_string(sc, "http-async-post: callback must be a procedure"), callback));
  }
  
  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header cpr_headers = to_cpr_headers (sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies (sc, proxy);
  
  // Protect callback from GC
  int gc_loc = s7_gc_protect(sc, callback);
  
  // Create session on heap with shared_ptr to keep it alive
  auto session = std::make_shared<cpr::Session>();
  session->SetUrl(cpr::Url(url));
  session->SetParameters(cpr_params);
  session->SetBody(cpr::Body(body));
  session->SetHeader(cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session->SetProxies(cpr_proxies);
  }
  
  // Start async request using libcpr's built-in thread pool
  auto async_resp = session->PostAsync();
  
  // Store the request (session is also stored to keep reference)
  auto req = std::make_shared<AsyncHttpRequest>(sc, callback, gc_loc, session, std::move(async_resp));
  {
    std::lock_guard<std::mutex> lock(g_async_requests_mutex);
    g_async_requests.push_back(req);
  }
  
  return s7_make_boolean(sc, true);
}

inline void
glue_http_async_post (s7_scheme* sc) {
  s7_pointer cur_env = s7_curlet(sc);
  const char* name = "g_http-async-post";
  const char* doc = "(g_http-async-post url params body headers proxy callback) => boolean, start async http post. callback receives response hashtable. Use g_http-poll to check for completion.";
  auto func = s7_make_typed_function(sc, name, f_http_async_post, 6, 0, false, doc, NULL);
  s7_define(sc, cur_env, s7_make_symbol(sc, name), func);
}

// Start an async HTTP HEAD request
static s7_pointer
f_http_async_head (s7_scheme* sc, s7_pointer args) {
  const char* url = s7_string(s7_car(args));
  s7_pointer params = s7_cadr(args);
  s7_pointer headers = s7_caddr(args);
  s7_pointer proxy = s7_cadddr(args);
  s7_pointer callback = s7_car(s7_cddddr(args));
  
  if (!s7_is_procedure(callback)) {
    return s7_error(sc, s7_make_symbol(sc, "type-error"),
                    s7_list(sc, 2, s7_make_string(sc, "http-async-head: callback must be a procedure"), callback));
  }
  
  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header cpr_headers = to_cpr_headers (sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies (sc, proxy);
  
  // Protect callback from GC
  int gc_loc = s7_gc_protect(sc, callback);
  
  // Create session on heap with shared_ptr to keep it alive
  auto session = std::make_shared<cpr::Session>();
  session->SetUrl(cpr::Url(url));
  session->SetParameters(cpr_params);
  session->SetHeader(cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session->SetProxies(cpr_proxies);
  }
  
  // Start async request using libcpr's built-in thread pool
  auto async_resp = session->HeadAsync();
  
  // Store the request (session is also stored to keep reference)
  auto req = std::make_shared<AsyncHttpRequest>(sc, callback, gc_loc, session, std::move(async_resp));
  {
    std::lock_guard<std::mutex> lock(g_async_requests_mutex);
    g_async_requests.push_back(req);
  }
  
  return s7_make_boolean(sc, true);
}

inline void
glue_http_async_head (s7_scheme* sc) {
  s7_pointer cur_env = s7_curlet(sc);
  const char* name = "g_http-async-head";
  const char* doc = "(g_http-async-head url params headers proxy callback) => boolean, start async http head. callback receives response hashtable. Use g_http-poll to check for completion.";
  auto func = s7_make_typed_function(sc, name, f_http_async_head, 5, 0, false, doc, NULL);
  s7_define(sc, cur_env, s7_make_symbol(sc, name), func);
}

// Poll for completed async HTTP requests and execute their callbacks
static s7_pointer
f_http_poll (s7_scheme* sc, s7_pointer args) {
  int executed = process_async_http_callbacks();
  return s7_make_integer(sc, executed);
}

inline void
glue_http_poll (s7_scheme* sc) {
  s7_pointer cur_env = s7_curlet(sc);
  const char* name = "g_http-poll";
  const char* doc = "(g_http-poll) => integer, check for completed async http requests and execute their callbacks. Returns number of callbacks executed.";
  auto func = s7_make_typed_function(sc, name, f_http_poll, 0, 0, false, doc, NULL);
  s7_define(sc, cur_env, s7_make_symbol(sc, name), func);
}

// Wait for all pending async HTTP requests to complete (blocking)
static s7_pointer
f_http_wait_all (s7_scheme* sc, s7_pointer args) {
  s7_double timeout_sec = -1.0; // -1 means wait forever
  if (s7_is_real(s7_car(args))) {
    timeout_sec = s7_real(s7_car(args));
  }
  
  auto start = std::chrono::steady_clock::now();
  bool has_pending = true;
  int total_executed = 0;
  
  while (has_pending) {
    int executed = process_async_http_callbacks();
    total_executed += executed;
    
    // Check if there are still pending requests
    {
      std::lock_guard<std::mutex> lock(g_async_requests_mutex);
      has_pending = !g_async_requests.empty();
    }
    
    if (has_pending) {
      // Check timeout
      if (timeout_sec >= 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - start).count() / 1000.0;
        if (elapsed >= timeout_sec) {
          break; // Timeout
        }
      }
      // Small sleep to avoid busy waiting
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  
  return s7_make_integer(sc, total_executed);
}

inline void
glue_http_wait_all (s7_scheme* sc) {
  s7_pointer cur_env = s7_curlet(sc);
  const char* name = "g_http-wait-all";
  const char* doc = "(g_http-wait-all [timeout-seconds]) => integer, wait for all pending async http requests to complete. timeout < 0 means wait forever. Returns number of callbacks executed.";
  auto func = s7_make_typed_function(sc, name, f_http_wait_all, 0, 1, false, doc, NULL);
  s7_define(sc, cur_env, s7_make_symbol(sc, name), func);
}

inline void
glue_http_async (s7_scheme* sc) {
  glue_http_async_get(sc);
  glue_http_async_post(sc);
  glue_http_async_head(sc);
  glue_http_poll(sc);
  glue_http_wait_all(sc);
}


inline s7_pointer
string_vector_to_s7_vector (s7_scheme* sc, vector<string> v) {
  int        N  = v.size ();
  s7_pointer ret= s7_make_vector (sc, N);
  for (int i= 0; i < N; i++) {
    s7_vector_set (sc, ret, i, s7_make_string (sc, v[i].c_str ()));
  }
  return ret;
}

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional) {
  s7_pointer cur_env= s7_curlet (sc);
  s7_pointer func   = s7_make_typed_function (sc, name, f, required, optional, false, desc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

static s7_pointer
f_version (s7_scheme* sc, s7_pointer args) {
  return s7_make_string (sc, GOLDFISH_VERSION);
}

static s7_pointer
f_delete_file (s7_scheme* sc, s7_pointer args) {
  const char* path_c= s7_string (s7_car (args));
  return s7_make_boolean (sc, tb_file_remove (path_c));
}

inline void
glue_goldfish (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);

  const char* s_version           = "version";
  const char* d_version           = "(version) => string";
  const char* s_delete_file       = "g_delete-file";
  const char* d_delete_file       = "(g_delete-file string) => boolean";
  const char* s_function_libraries= "g_function-libraries";
  const char* d_function_libraries=
    "(g_function-libraries function-name) => list, returns visible library names such as '((liii string)) that "
    "export function-name in the current *load-path*";

  s7_define (sc, cur_env, s7_make_symbol (sc, s_version),
             s7_make_typed_function (sc, s_version, f_version, 0, 0, false, d_version, NULL));

  s7_define (sc, cur_env, s7_make_symbol (sc, s_delete_file),
             s7_make_typed_function (sc, s_delete_file, f_delete_file, 1, 0, false, d_delete_file, NULL));

  s7_define (sc, cur_env, s7_make_symbol (sc, s_function_libraries),
             s7_make_typed_function (sc, s_function_libraries, f_function_libraries, 1, 0, false, d_function_libraries, NULL));
}

// old `f_current_second` TODO: use std::chrono::tai_clock::now() when using C++ 20
//                        NOTE(jinser): use a new name for tai
// `current-second` impl by g_get-time-of-day now
static s7_pointer
f_get_time_of_day (s7_scheme* sc, s7_pointer args) {
  using namespace std::chrono;
  auto now = time_point_cast<microseconds>(system_clock::now());
  auto since_epoch = now.time_since_epoch();
  auto sec = duration_cast<seconds>(since_epoch);

  s7_pointer vs = s7_list(sc, 2,
                          s7_make_integer(sc, sec.count()),
                          s7_make_integer(sc, (since_epoch - sec).count()));
  return s7_values(sc, vs);
}

static s7_pointer
f_monotonic_nanosecond (s7_scheme* sc, s7_pointer args) {
  using namespace std::chrono;
  auto now = steady_clock::now();
  auto duration = now.time_since_epoch();
  auto count = duration_cast<std::chrono::nanoseconds>(duration).count();
  return s7_make_integer(sc, count);
}

template<typename Clock>
constexpr int64_t clock_resolution_ns() {
  typedef std::chrono::duration<double, std::nano> NS;
  NS ns = typename Clock::duration(1);
  return ns.count();
}

inline void
glue_scheme_time (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);

  const char* s_get_time_of_day= "g_get-time-of-day";
  const char* d_get_time_of_day= "(g_get-time-of-day): () => (integer, integer), return the "
                                "current second and microsecond in integer";
  s7_define (sc, cur_env, s7_make_symbol (sc, s_get_time_of_day),
             s7_make_typed_function (sc, s_get_time_of_day, f_get_time_of_day, 0, 0, false, d_get_time_of_day, NULL));

  const char* s_monotonic_nanosecond= "g_monotonic-nanosecond";
  const char* d_monotonic_nanosecond= "(g_monotonic-nanosecond): () => integer, returns the steady clock's monotonic nanoseconds since an unspecified epoch";
  s7_define (sc, cur_env, s7_make_symbol (sc, s_monotonic_nanosecond),
             s7_make_typed_function (sc, s_monotonic_nanosecond, f_monotonic_nanosecond, 0, 0, false, d_monotonic_nanosecond, NULL));

  s7_define_constant_with_environment (sc, cur_env, "g_system-clock-resolution",
                                       s7_make_integer(sc, clock_resolution_ns<std::chrono::system_clock>()));
  s7_define_constant_with_environment (sc, cur_env, "g_steady-clock-resolution",
                                       s7_make_integer(sc, clock_resolution_ns<std::chrono::steady_clock>()));
}


static s7_pointer
f_get_environment_variable (s7_scheme* sc, s7_pointer args) {
#ifdef _MSC_VER
  std::string path_sep= ";";
#else
  std::string path_sep= ":";
#endif
  std::string          ret;
  tb_size_t            size       = 0;
  const char*          key        = s7_string (s7_car (args));
  tb_environment_ref_t environment= tb_environment_init ();
  if (environment) {
    size= tb_environment_load (environment, key);
    if (size >= 1) {
      tb_for_all_if (tb_char_t const*, value, environment, value) { ret.append (value).append (path_sep); }
    }
  }
  tb_environment_exit (environment);
  if (size == 0) { // env key not found
    return s7_make_boolean (sc, false);
  }
  else {
    return s7_make_string (sc, ret.substr (0, ret.size () - 1).c_str ());
  }
}

static s7_pointer
f_command_line (s7_scheme* sc, s7_pointer args) {
  s7_pointer ret = s7_nil (sc);
  int        size= command_args.size ();
  for (int i= size - 1; i >= 0; i--) {
    ret= s7_cons (sc, s7_make_string (sc, command_args[i].c_str ()), ret);
  }
  return ret;
}

static s7_pointer
f_unset_environment_variable (s7_scheme* sc, s7_pointer args) {
  const char* env_name= s7_string (s7_car (args));
  return s7_make_boolean (sc, tb_environment_remove (env_name));
}

static s7_pointer
f_getenvs (s7_scheme* sc, s7_pointer args) {
  s7_pointer p = s7_nil(sc);

#ifdef TB_CONFIG_OS_WINDOWS
  // Windows: use GetEnvironmentStrings
  LPCH env_strings = GetEnvironmentStrings();
  if (env_strings) {
    LPCH env = env_strings;
    while (*env) {
      const char* eq = strchr(env, '=');
      if (eq && eq != env) { // skip empty variable names
        s7_pointer name = s7_make_string_with_length(sc, env, eq - env);
        s7_pointer value = s7_make_string(sc, eq + 1);
        p = s7_cons(sc, s7_cons(sc, name, value), p);
      }
      env += strlen(env) + 1;
    }
    FreeEnvironmentStrings(env_strings);
  }
#else
  // Unix/Linux/macOS: use environ (declared at global scope)
  for (int32_t i = 0; environ[i]; i++) {
    const char* eq = strchr(environ[i], '=');
    if (eq) {
      s7_pointer name = s7_make_string_with_length(sc, environ[i], eq - environ[i]);
      s7_pointer value = s7_make_string(sc, eq + 1);
      p = s7_cons(sc, s7_cons(sc, name, value), p);
    }
  }
#endif

  return p;
}

inline void
glue_scheme_process_context (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);

  const char* s_get_environment_variable= "g_get-environment-variable";
  const char* d_get_environment_variable= "(g_get-environemt-variable string) => string";
  const char* s_command_line            = "g_command-line";
  const char* d_command_line            = "(g_command-line) => string";
  const char* s_getenvs                 = "g_getenvs";
  const char* d_getenvs                 = "(g_getenvs) => alist, returns all environment variables as an alist";

  s7_define (sc, cur_env, s7_make_symbol (sc, s_get_environment_variable),
             s7_make_typed_function (sc, s_get_environment_variable, f_get_environment_variable, 1, 0, false,
                                     d_get_environment_variable, NULL));
  s7_define (sc, cur_env, s7_make_symbol (sc, s_command_line),
             s7_make_typed_function (sc, s_command_line, f_command_line, 0, 0, false, d_command_line, NULL));
  s7_define (sc, cur_env, s7_make_symbol (sc, s_getenvs),
             s7_make_typed_function (sc, s_getenvs, f_getenvs, 0, 0, false, d_getenvs, NULL));
}

string
goldfish_exe () {
#ifdef TB_CONFIG_OS_WINDOWS
  char buffer[GOLDFISH_PATH_MAXN];
  GetModuleFileName (NULL, buffer, GOLDFISH_PATH_MAXN);
  return string (buffer);
#elif TB_CONFIG_OS_MACOSX
  char        buffer[PATH_MAX];
  uint32_t    size= sizeof (buffer);
  if (_NSGetExecutablePath (buffer, &size) == 0) {
    char real_path[GOLDFISH_PATH_MAXN];
    if (realpath (buffer, real_path) != NULL) {
      return string (real_path);
    }
  }
#elif TB_CONFIG_OS_LINUX
  char    buffer[GOLDFISH_PATH_MAXN];
  ssize_t len= readlink ("/proc/self/exe", buffer, sizeof (buffer) - 1);
  if (len != -1) {
    buffer[len]= '\0';
    return std::string (buffer);
  }
#endif
  return "";
}

static s7_pointer
f_executable (s7_scheme* sc, s7_pointer args) {
  string exe_path= goldfish_exe ();
  return s7_make_string (sc, exe_path.c_str ());
}

inline void
glue_executable (s7_scheme* sc) {
  const char* name= "g_executable";
  const char* desc= "(g_executable) => string";
  glue_define (sc, name, desc, f_executable, 0, 0);
}

inline void
glue_liii_sys (s7_scheme* sc) {
  glue_executable (sc);
}

static s7_pointer
f_os_arch (s7_scheme* sc, s7_pointer args) {
  return s7_make_string (sc, TB_ARCH_STRING);
}

inline void
glue_os_arch (s7_scheme* sc) {
  const char* name= "g_os-arch";
  const char* desc= "(g_os-arch) => string";
  glue_define (sc, name, desc, f_os_arch, 0, 0);
}

static s7_pointer
f_os_type (s7_scheme* sc, s7_pointer args) {
#ifdef TB_CONFIG_OS_LINUX
  return s7_make_string (sc, "Linux");
#endif
#ifdef TB_CONFIG_OS_MACOSX
  return s7_make_string (sc, "Darwin");
#endif
#ifdef TB_CONFIG_OS_WINDOWS
  return s7_make_string (sc, "Windows");
#endif
  return s7_make_boolean (sc, false);
}

inline void
glue_os_type (s7_scheme* sc) {
  const char* name= "g_os-type";
  const char* desc= "(g_os-type) => string";
  glue_define (sc, name, desc, f_os_type, 0, 0);
}

static s7_pointer
f_os_call (s7_scheme* sc, s7_pointer args) {
  const char*       cmd_c= s7_string (s7_car (args));
  tb_process_attr_t attr = {tb_null};
  attr.flags             = TB_PROCESS_FLAG_NO_WINDOW;
  int ret;

#if (defined(_MSC_VER) || defined(__MINGW32__))
  ret= (int) std::system (cmd_c);
#elif defined(__EMSCRIPTEN__)
  tb_char_t* argv[]= {(tb_char_t*) cmd_c, tb_null};
  ret              = (int) tb_process_run (argv[0], (tb_char_t const**) argv, &attr);
#else
  wordexp_t p;
  ret= wordexp (cmd_c, &p, 0);
  if (ret != 0) {
    // failed after calling wordexp
  }
  else if (p.we_wordc == 0) {
    wordfree (&p);
    ret= EINVAL;
  }
  else {
    ret= (int) tb_process_run (p.we_wordv[0], (tb_char_t const**) p.we_wordv, &attr);
    wordfree (&p);
  }
#endif
  return s7_make_integer (sc, ret);
}

inline void
glue_os_call (s7_scheme* sc) {
  const char* name= "g_os-call";
  const char* desc= "(g_os-call string) => int, execute a shell command and return the exit code";
  glue_define (sc, name, desc, f_os_call, 1, 0);
}

static s7_pointer
f_system (s7_scheme* sc, s7_pointer args) {
  const char* cmd_c= s7_string (s7_car (args));
  int         ret  = (int) std::system (cmd_c);
  return s7_make_integer (sc, ret);
}

inline void
glue_system (s7_scheme* sc) {
  const char* name= "g_system";
  const char* desc= "(g_system string) => int, execute a shell command and return the exit code";
  glue_define (sc, name, desc, f_system, 1, 0);
}

static s7_pointer
f_access (s7_scheme* sc, s7_pointer args) {
  const char* path_c= s7_string (s7_car (args));
  int         mode  = s7_integer ((s7_cadr (args)));
  bool        ret   = false;
  if (mode == 0) {
    tb_file_info_t info;
    ret= tb_file_info (path_c, &info);
  }
  else {
    ret= tb_file_access (path_c, mode);
  }

  return s7_make_boolean (sc, ret);
}

inline void
glue_access (s7_scheme* sc) {
  const char* name= "g_access";
  const char* desc= "(g_access string integer) => boolean, check file access permissions";
  glue_define (sc, name, desc, f_access, 2, 0);
}

// 实现 putenv 功能
static s7_pointer
f_set_environment_variable (s7_scheme* sc, s7_pointer args) {
  const char* key  = s7_string (s7_car (args));
  const char* value= s7_string (s7_cadr (args));
  return s7_make_boolean (sc, tb_environment_set (key, value));
}

inline void
glue_setenv (s7_scheme* sc) {
  const char* name= "g_setenv";
  const char* desc= "(g_setenv key value) => boolean, set an environment variable";
  glue_define (sc, name, desc, f_set_environment_variable, 2, 0);
}

inline void
glue_unsetenv (s7_scheme* sc) {
  const char* name= "g_unsetenv";
  const char* desc= "(g_unsetenv string): string => boolean";
  glue_define (sc, name, desc, f_unset_environment_variable, 1, 0);
}

static s7_pointer
f_os_temp_dir (s7_scheme* sc, s7_pointer args) {
  tb_char_t path[GOLDFISH_PATH_MAXN];
  tb_directory_temporary (path, GOLDFISH_PATH_MAXN);
  return s7_make_string (sc, path);
}

inline void
glue_os_temp_dir (s7_scheme* sc) {
  const char* name= "g_os-temp-dir";
  const char* desc= "(g_os-temp-dir) => string, get the temporary directory path";
  glue_define (sc, name, desc, f_os_temp_dir, 0, 0);
}

static s7_pointer
f_mkdir (s7_scheme* sc, s7_pointer args) {
  const char* dir_c= s7_string (s7_car (args));
  return s7_make_boolean (sc, tb_directory_create (dir_c));
}

inline void
glue_mkdir (s7_scheme* sc) {
  const char* name= "g_mkdir";
  const char* desc= "(g_mkdir string) => boolean, create a directory";
  glue_define (sc, name, desc, f_mkdir, 1, 0);
}

static s7_pointer
f_rmdir (s7_scheme* sc, s7_pointer args) {
  const char* dir_c= s7_string (s7_car (args));
  return s7_make_boolean (sc, tb_directory_remove (dir_c));
}

inline void
glue_rmdir (s7_scheme* sc) {
  const char* name= "g_rmdir";
  const char* desc= "(g_rmdir string) => boolean, remove a directory";
  glue_define (sc, name, desc, f_rmdir, 1, 0);
}

static s7_pointer
f_remove_file (s7_scheme* sc, s7_pointer args) {
  const char* path   = s7_string (s7_car (args));
  bool        success= tb_file_remove (path); // 直接调用 TBOX 删除文件
  return s7_make_boolean (sc, success);
}

inline void
glue_remove_file (s7_scheme* sc) {
  const char* name= "g_remove-file";
  const char* desc= "(g_remove-file path) => boolean, delete a file";
  glue_define (sc, name, desc, f_remove_file, 1, 0);
}

static s7_pointer
f_rename (s7_scheme* sc, s7_pointer args) {
  const char* src = s7_string (s7_car (args));
  const char* dst = s7_string (s7_cadr (args));
  try {
    fs::rename (src, dst);
    return s7_make_boolean (sc, true);
  }
  catch (const fs::filesystem_error& e) {
    return s7_make_boolean (sc, false);
  }
}

inline void
glue_rename (s7_scheme* sc) {
  const char* name= "g_rename";
  const char* desc= "(g_rename src dst) => boolean, rename file or directory from src to dst";
  glue_define (sc, name, desc, f_rename, 2, 0);
}

static s7_pointer
f_chdir (s7_scheme* sc, s7_pointer args) {
  const char* dir_c= s7_string (s7_car (args));
  return s7_make_boolean (sc, tb_directory_current_set (dir_c));
}

inline void
glue_chdir (s7_scheme* sc) {
  const char* name= "g_chdir";
  const char* desc= "(g_chdir string) => boolean, change the current working directory";
  glue_define (sc, name, desc, f_chdir, 1, 0);
}

static tb_long_t
tb_directory_walk_func (tb_char_t const* path, tb_file_info_t const* info, tb_cpointer_t priv) {
  // check
  tb_assert_and_check_return_val (path && info, TB_DIRECTORY_WALK_CODE_END);

  vector<string>* p_v_result= (vector<string>*) priv;
  p_v_result->push_back (string (path));
  return TB_DIRECTORY_WALK_CODE_CONTINUE;
}

static s7_pointer
f_listdir (s7_scheme* sc, s7_pointer args) {
  const char*    path_c= s7_string (s7_car (args));
  vector<string> entries;
  s7_pointer     ret= s7_make_vector (sc, 0);
  tb_directory_walk (path_c, 0, tb_false, tb_directory_walk_func, &entries);

  int    entries_N   = entries.size ();
  string path_s      = string (path_c);
  int    path_N      = path_s.size ();
  int    path_slash_N= path_N;
  char   last_ch     = path_s[path_N - 1];
#if defined(TB_CONFIG_OS_WINDOWS)
  if (last_ch != '/' && last_ch != '\\') {
    path_slash_N= path_slash_N + 1;
  }
#else
  if (last_ch != '/') {
    path_slash_N= path_slash_N + 1;
  }
#endif
  for (int i= 0; i < entries_N; i++) {
    entries[i]= entries[i].substr (path_slash_N);
  }
  return string_vector_to_s7_vector (sc, entries);
}

inline void
glue_listdir (s7_scheme* sc) {
  const char* name= "g_listdir";
  const char* desc= "(g_listdir string) => vector, list the contents of a directory";
  glue_define (sc, name, desc, f_listdir, 1, 0);
}

static s7_pointer
f_getcwd (s7_scheme* sc, s7_pointer args) {
  tb_char_t path[GOLDFISH_PATH_MAXN];
  tb_directory_current (path, GOLDFISH_PATH_MAXN);
  return s7_make_string (sc, path);
}

inline void
glue_getcwd (s7_scheme* sc) {
  const char* name= "g_getcwd";
  const char* desc= "(g_getcwd) => string, get the current working directory";
  glue_define (sc, name, desc, f_getcwd, 0, 0);
}

static s7_pointer
f_getlogin (s7_scheme* sc, s7_pointer args) {
#ifdef TB_CONFIG_OS_WINDOWS
  return s7_make_boolean (sc, false);
#else
  uid_t          uid= getuid ();
  struct passwd* pwd= getpwuid (uid);
  return s7_make_string (sc, pwd->pw_name);
#endif
}

inline void
glue_getlogin (s7_scheme* sc) {
  const char* name= "g_getlogin";
  const char* desc= "(g_getlogin) => string, get the current user's login name";
  glue_define (sc, name, desc, f_getlogin, 0, 0);
}

static s7_pointer
f_getpid (s7_scheme* sc, s7_pointer args) {
#ifdef TB_CONFIG_OS_WINDOWS
  return s7_make_integer (sc, (int) GetCurrentProcessId ());
#else
  return s7_make_integer (sc, getpid ());
#endif
}

inline void
glue_getpid (s7_scheme* sc) {
  const char* name= "g_getpid";
  const char* desc= "(g_getpid) => integer";
  glue_define (sc, name, desc, f_getpid, 0, 0);
}

static s7_pointer
f_sleep(s7_scheme* sc, s7_pointer args) {
  s7_double seconds = s7_real(s7_car(args));
  
  // 使用 tbox 的 tb_sleep 函数，参数是毫秒
  tb_msleep((tb_long_t)(seconds * 1000));

  return s7_nil(sc);
}

inline void
glue_sleep(s7_scheme* sc) {
  const char* name = "g_sleep";
  const char* desc = "(g_sleep seconds) => nil, sleep for the specified number of seconds";
  glue_define(sc, name, desc, f_sleep, 1, 0);
}



inline void
glue_liii_os (s7_scheme* sc) {
  glue_os_arch (sc);
  glue_os_type (sc);
  glue_os_call (sc);
  glue_system (sc);
  glue_access (sc);
  glue_setenv (sc);
  glue_unsetenv (sc);
  glue_getcwd (sc);
  glue_os_temp_dir (sc);
  glue_mkdir (sc);
  glue_rmdir (sc);
  glue_remove_file (sc);
  glue_rename (sc);
  glue_chdir (sc);
  glue_listdir (sc);
  glue_getlogin (sc);
  glue_getpid (sc);
}

static s7_pointer
f_uuid4 (s7_scheme* sc, s7_pointer args) {
  tb_char_t        uuid[37];
  const tb_char_t* ret= tb_uuid4_make_cstr (uuid, tb_null);
  return s7_make_string (sc, ret);
}

inline void
glue_uuid4 (s7_scheme* sc) {
  const char* name= "g_uuid4";
  const char* desc= "(g_uuid4) => string";
  glue_define (sc, name, desc, f_uuid4, 0, 0);
}

inline void
glue_liii_uuid (s7_scheme* sc) {
  glue_uuid4 (sc);
}

inline void
hash_bytes_to_hex (const tb_byte_t* bytes, tb_size_t length, tb_char_t* hex_output) {
  static const tb_char_t hex_digits[]= "0123456789abcdef";
  for (tb_size_t i= 0; i < length; ++i) {
    hex_output[i * 2]    = hex_digits[bytes[i] >> 4];
    hex_output[i * 2 + 1]= hex_digits[bytes[i] & 0x0f];
  }
  hex_output[length * 2]= '\0';
}

static bool
md5_file_to_hex (const char* path, tb_char_t* hex_output) {
  if (!path) {
    return false;
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return false;
  }

  tb_md5_t md5;
  tb_md5_init (&md5, 0);

  tb_size_t size  = tb_file_size (file);
  tb_size_t offset= 0;
  tb_byte_t buffer[4096];
  while (offset < size) {
    tb_size_t want     = ((size - offset) > sizeof (buffer)) ? sizeof (buffer) : (size - offset);
    tb_size_t real_size= tb_file_read (file, buffer, want);
    if (real_size == 0) {
      tb_file_exit (file);
      return false;
    }
    tb_md5_spak (&md5, buffer, real_size);
    offset += real_size;
  }

  tb_file_exit (file);

  tb_byte_t digest[16];
  tb_md5_exit (&md5, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return true;
}

static bool
sha_file_to_hex (const char* path, tb_size_t mode, tb_size_t digest_size, tb_char_t* hex_output) {
  if (!path) {
    return false;
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return false;
  }

  tb_sha_t sha;
  tb_sha_init (&sha, mode);

  tb_size_t size  = tb_file_size (file);
  tb_size_t offset= 0;
  tb_byte_t buffer[4096];
  while (offset < size) {
    tb_size_t want     = ((size - offset) > sizeof (buffer)) ? sizeof (buffer) : (size - offset);
    tb_size_t real_size= tb_file_read (file, buffer, want);
    if (real_size == 0) {
      tb_file_exit (file);
      return false;
    }
    tb_sha_spak (&sha, buffer, real_size);
    offset += real_size;
  }

  tb_file_exit (file);

  tb_byte_t digest[32];
  tb_sha_exit (&sha, digest, digest_size);
  hash_bytes_to_hex (digest, digest_size, hex_output);
  return true;
}

static s7_pointer
f_md5 (s7_scheme* sc, s7_pointer args) {
  const char* search_string= s7_string (s7_car (args));
  tb_size_t   len          = tb_strlen (search_string);
  tb_byte_t   digest[16];
  tb_char_t   hex_output[33]= {0};
  tb_md5_t    md5;

  tb_md5_init (&md5, 0);
  if (len > 0) {
    tb_md5_spak (&md5, (tb_byte_t const*) search_string, len);
  }
  tb_md5_exit (&md5, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return s7_make_string (sc, hex_output);
}

inline void
glue_md5 (s7_scheme* sc) {
  const char* name= "g_md5";
  const char* desc= "(g_md5 str) => string";
  glue_define (sc, name, desc, f_md5, 1, 0);
}

static s7_pointer
f_md5_file (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  tb_char_t   hex_output[33]= {0};
  if (!md5_file_to_hex (path, hex_output)) {
    return s7_make_boolean (sc, false);
  }
  return s7_make_string (sc, hex_output);
}

inline void
glue_md5_file (s7_scheme* sc) {
  const char* name= "g_md5-by-file";
  const char* desc= "(g_md5-by-file path) => string|#f";
  glue_define (sc, name, desc, f_md5_file, 1, 0);
}

static s7_pointer
f_sha1 (s7_scheme* sc, s7_pointer args) {
  const char* search_string= s7_string (s7_car (args));
  tb_size_t   len          = tb_strlen (search_string);
  tb_byte_t   digest[20];
  tb_char_t   hex_output[41]= {0};
  tb_sha_t    sha;

  tb_sha_init (&sha, 160);
  if (len > 0) {
    tb_sha_spak (&sha, (tb_byte_t const*) search_string, len);
  }
  tb_sha_exit (&sha, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return s7_make_string (sc, hex_output);
}

inline void
glue_sha1 (s7_scheme* sc) {
  const char* name= "g_sha1";
  const char* desc= "(g_sha1 str) => string";
  glue_define (sc, name, desc, f_sha1, 1, 0);
}

static s7_pointer
f_sha1_file (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  tb_char_t   hex_output[41]= {0};
  if (!sha_file_to_hex (path, 160, 20, hex_output)) {
    return s7_make_boolean (sc, false);
  }
  return s7_make_string (sc, hex_output);
}

inline void
glue_sha1_file (s7_scheme* sc) {
  const char* name= "g_sha1-by-file";
  const char* desc= "(g_sha1-by-file path) => string|#f";
  glue_define (sc, name, desc, f_sha1_file, 1, 0);
}

static s7_pointer
f_sha256 (s7_scheme* sc, s7_pointer args) {
  const char* search_string= s7_string (s7_car (args));
  tb_size_t   len          = tb_strlen (search_string);
  tb_byte_t   digest[32];
  tb_char_t   hex_output[65]= {0};
  tb_sha_t    sha;

  tb_sha_init (&sha, 256);
  if (len > 0) {
    tb_sha_spak (&sha, (tb_byte_t const*) search_string, len);
  }
  tb_sha_exit (&sha, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return s7_make_string (sc, hex_output);
}

inline void
glue_sha256 (s7_scheme* sc) {
  const char* name= "g_sha256";
  const char* desc= "(g_sha256 str) => string";
  glue_define (sc, name, desc, f_sha256, 1, 0);
}

static s7_pointer
f_sha256_file (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  tb_char_t   hex_output[65]= {0};
  if (!sha_file_to_hex (path, 256, 32, hex_output)) {
    return s7_make_boolean (sc, false);
  }
  return s7_make_string (sc, hex_output);
}

inline void
glue_sha256_file (s7_scheme* sc) {
  const char* name= "g_sha256-by-file";
  const char* desc= "(g_sha256-by-file path) => string|#f";
  glue_define (sc, name, desc, f_sha256_file, 1, 0);
}

inline void
glue_liii_hashlib (s7_scheme* sc) {
  glue_md5 (sc);
  glue_md5_file (sc);
  glue_sha1 (sc);
  glue_sha1_file (sc);
  glue_sha256 (sc);
  glue_sha256_file (sc);
}



static s7_pointer
f_isdir (s7_scheme* sc, s7_pointer args) {
  const char*    dir_c= s7_string (s7_car (args));
  tb_file_info_t info;
  bool           ret= false;
  if (tb_file_info (dir_c, &info)) {
    switch (info.type) {
    case TB_FILE_TYPE_DIRECTORY:
    case TB_FILE_TYPE_DOT:
    case TB_FILE_TYPE_DOT2:
      ret= true;
    }
  }
  return s7_make_boolean (sc, ret);
}

inline void
glue_isdir (s7_scheme* sc) {
  const char* name= "g_isdir";
  const char* desc= "(g_isdir string) => boolean";
  glue_define (sc, name, desc, f_isdir, 1, 0);
}

static s7_pointer
f_isfile (s7_scheme* sc, s7_pointer args) {
  const char*    dir_c= s7_string (s7_car (args));
  tb_file_info_t info;
  bool           ret= false;
  if (tb_file_info (dir_c, &info)) {
    switch (info.type) {
    case TB_FILE_TYPE_FILE:
      ret= true;
    }
  }
  return s7_make_boolean (sc, ret);
}

inline void
glue_isfile (s7_scheme* sc) {
  const char* name= "g_isfile";
  const char* desc= "(g_isfile string) => boolean";
  glue_define (sc, name, desc, f_isfile, 1, 0);
}

static s7_pointer
f_path_getsize (s7_scheme* sc, s7_pointer args) {
  const char*    path_c= s7_string (s7_car (args));
  tb_file_info_t info;
  if (tb_file_info (path_c, &info)) {
    return s7_make_integer (sc, (int) info.size);
  }
  else {
    return s7_make_integer (sc, (int) -1);
  }
}

inline void
glue_path_getsize (s7_scheme* sc) {
  const char* name= "g_path-getsize";
  const char* desc= "(g_path_getsize string): string => integer";
  glue_define (sc, name, desc, f_path_getsize, 1, 0);
}

static s7_pointer
f_path_read_text (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_boolean (sc, false);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    // TODO: warning on the tb_file_init failure
    return s7_make_boolean (sc, false);
  }

  tb_file_sync (file);

  tb_size_t size= tb_file_size (file);
  if (size == 0) {
    tb_file_exit (file);
    return s7_make_string (sc, "");
  }

  tb_byte_t* buffer   = new tb_byte_t[size + 1];
  tb_size_t  real_size= tb_file_read (file, buffer, size);
  buffer[real_size]   = '\0';

  tb_file_exit (file);
  std::string content (reinterpret_cast<char*> (buffer), real_size);
  delete[] buffer;

  // Normalize line endings: convert \r\n to \n (also handles standalone \r)
  std::string normalized;
  normalized.reserve (content.size ());
  for (size_t i= 0; i < content.size (); ++i) {
    if (content[i] == '\r') {
      // Skip \r, but if next char is \n, we'll output just \n below
      if (i + 1 < content.size () && content[i + 1] == '\n') {
        // \r\n sequence: skip \r, output \n on next iteration
        continue;
      }
      // Standalone \r: treat as \n
      normalized.push_back ('\n');
    }
    else {
      normalized.push_back (content[i]);
    }
  }

  return s7_make_string (sc, normalized.c_str ());
}

inline void
glue_path_read_text (s7_scheme* sc) {
  const char* name= "g_path-read-text";
  const char* desc= "(g_path-read-text path) => string, read the content of the file at the given path";
  s7_define_function (sc, name, f_path_read_text, 1, 0, false, desc);
}

static s7_pointer
f_path_read_bytes (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_boolean (sc, false);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return s7_make_boolean (sc, false);
  }

  tb_file_sync (file);
  tb_size_t size= tb_file_size (file);

  if (size == 0) {
    tb_file_exit (file);
    // Create an empty bytevector with correct parameters
    return s7_make_byte_vector (sc, 0, 1, NULL); // 1 dimension, no dimension info
  }

  // Allocate buffer similar to f_path_read_text
  tb_byte_t* buffer   = new tb_byte_t[size];
  tb_size_t  real_size= tb_file_read (file, buffer, size);
  tb_file_exit (file);

  if (real_size != size) {
    delete[] buffer;
    return s7_make_boolean (sc, false); // Read failed
  }

  // Create a Scheme bytevector and copy data
  s7_pointer bytevector     = s7_make_byte_vector (sc, real_size, 1, NULL); // 1 dimension, no dimension info
  tb_byte_t* bytevector_data= s7_byte_vector_elements (bytevector);
  memcpy (bytevector_data, buffer, real_size);

  delete[] buffer;
  return bytevector; // Return the bytevector
}

inline void
glue_path_read_bytes (s7_scheme* sc) {
  const char* name= "g_path-read-bytes";
  const char* desc= "(g_path-read-bytes path) => bytevector, read the binary content of the file at the given path";
  s7_define_function (sc, name, f_path_read_bytes, 1, 0, false, desc);
}

static s7_pointer
f_path_write_text (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_integer (sc, -1);
  }

  const char* content= s7_string (s7_cadr (args));
  if (!content) {
    return s7_make_integer (sc, -1);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC);
  if (file == tb_null) {
    return s7_make_integer (sc, -1);
  }

  tb_filelock_ref_t lock= tb_filelock_init (file);
  if (tb_filelock_enter (lock, TB_FILELOCK_MODE_EX) == tb_false) {
    tb_filelock_exit (lock);
    tb_file_exit (file);
    return s7_make_integer (sc, -1);
  }

  tb_size_t content_size= strlen (content);
  tb_size_t written_size= tb_file_writ (file, reinterpret_cast<const tb_byte_t*> (content), content_size);

  bool release_success= tb_filelock_leave (lock);
  tb_filelock_exit (lock);
  bool exit_success= tb_file_exit (file);

  if (written_size == content_size && release_success && exit_success) {
    return s7_make_integer (sc, written_size);
  }
  else {
    return s7_make_integer (sc, -1);
  }
}

inline void
glue_path_write_text (s7_scheme* sc) {
  const char* name= "g_path-write-text";
  const char* desc= "(g_path-write-text path content) => integer,\
write content to the file at the given path and return the number of bytes written, or -1 on failure";
  s7_define_function (sc, name, f_path_write_text, 2, 0, false, desc);
}

static s7_pointer
f_path_append_text (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_integer (sc, -1);
  }

  const char* content= s7_string (s7_cadr (args));
  if (!content) {
    return s7_make_integer (sc, -1);
  }

  // 以追加模式打开文件
  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_APPEND);
  if (file == tb_null) {
    return s7_make_integer (sc, -1);
  }

  tb_filelock_ref_t lock= tb_filelock_init (file);
  if (tb_filelock_enter (lock, TB_FILELOCK_MODE_EX) == tb_false) {
    tb_filelock_exit (lock);
    tb_file_exit (file);
    return s7_make_integer (sc, -1);
  }

  tb_size_t content_size= strlen (content);
  tb_size_t written_size= tb_file_writ (file, reinterpret_cast<const tb_byte_t*> (content), content_size);

  bool release_success= tb_filelock_leave (lock);
  tb_filelock_exit (lock);
  bool exit_success= tb_file_exit (file);

  if (written_size == content_size && release_success && exit_success) {
    return s7_make_integer (sc, written_size);
  }
  else {
    return s7_make_integer (sc, -1);
  }
}

inline void
glue_path_append_text (s7_scheme* sc) {
  const char* name= "g_path-append-text";
  const char* desc= "(g_path-append-text path content) => integer,\
append content to the file at the given path and return the number of bytes written, or -1 on failure";
  s7_define_function (sc, name, f_path_append_text, 2, 0, false, desc);
}

static s7_pointer
f_path_touch (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_boolean (sc, false);
  }

  tb_bool_t success= tb_file_touch (path, 0, 0);

  if (success == tb_true) {
    return s7_make_boolean (sc, true);
  }
  else {
    return s7_make_boolean (sc, false);
  }
}

inline void
glue_path_touch (s7_scheme* sc) {
  const char* name= "g_path-touch";
  const char* desc= "(g_path-touch path) => boolean, create empty file or update modification time";
  s7_define_function (sc, name, f_path_touch, 1, 0, false, desc);
}

inline void
glue_liii_path (s7_scheme* sc) {
  glue_isfile (sc);
  glue_isdir (sc);
  glue_path_getsize (sc);
  glue_path_read_text (sc);
  glue_path_read_bytes (sc);
  glue_path_write_text (sc);
  glue_path_append_text (sc);
  glue_path_touch (sc);
}

static s7_pointer
f_datetime_now (s7_scheme* sc, s7_pointer args) {
  // Get current time using tbox for year, month, day, etc.
  tb_time_t now= tb_time ();

  // Get local time
  tb_tm_t lt= {0};
  if (!tb_localtime (now, &lt)) {
    return s7_f (sc);
  }

  // Use C++ chrono to get microseconds
  std::uint64_t micros= 0;
#ifdef TB_CONFIG_OS_WINDOWS
  // On Windows, ensure we properly handle chrono
  FILETIME       ft;
  ULARGE_INTEGER uli;
  GetSystemTimeAsFileTime (&ft);
  uli.LowPart = ft.dwLowDateTime;
  uli.HighPart= ft.dwHighDateTime;
  // Convert to microseconds and get modulo
  micros= (uli.QuadPart / 10) % 1000000; // Convert from 100-nanosecond intervals to microseconds
#else
  // Standard approach for other platforms
  auto now_chrono= std::chrono::system_clock::now ();
  auto duration  = now_chrono.time_since_epoch ();
  micros         = std::chrono::duration_cast<std::chrono::microseconds> (duration).count () % 1000000;
#endif

  // Create a vector with the time components - vector is easier to index than list in Scheme
  s7_pointer time_vec= s7_make_vector (sc, 7);

  // Fill the vector with values
  s7_vector_set (sc, time_vec, 0, s7_make_integer (sc, lt.year));   // year
  s7_vector_set (sc, time_vec, 1, s7_make_integer (sc, lt.month));  // month
  s7_vector_set (sc, time_vec, 2, s7_make_integer (sc, lt.mday));   // day
  s7_vector_set (sc, time_vec, 3, s7_make_integer (sc, lt.hour));   // hour
  s7_vector_set (sc, time_vec, 4, s7_make_integer (sc, lt.minute)); // minute
  s7_vector_set (sc, time_vec, 5, s7_make_integer (sc, lt.second)); // second
  s7_vector_set (sc, time_vec, 6, s7_make_integer (sc, micros));    // micro-second

  return time_vec;
}

inline void
glue_datetime_now (s7_scheme* sc) {
  const char* name= "g_datetime-now";
  const char* desc= "(g_datetime-now) => datetime, create a datetime object with current time";
  s7_define_function (sc, name, f_datetime_now, 0, 0, false, desc);
}

static s7_pointer
f_date_now (s7_scheme* sc, s7_pointer args) {
  // Get current time using tbox for year, month, day, etc.
  tb_time_t now= tb_time ();

  // Get local time
  tb_tm_t lt= {0};
  if (!tb_localtime (now, &lt)) {
    return s7_f (sc);
  }

  // Create a vector with the time components - vector is easier to index than list in Scheme
  s7_pointer time_vec= s7_make_vector (sc, 3);

  // Fill the vector with values
  s7_vector_set (sc, time_vec, 0, s7_make_integer (sc, lt.year));  // year
  s7_vector_set (sc, time_vec, 1, s7_make_integer (sc, lt.month)); // month
  s7_vector_set (sc, time_vec, 2, s7_make_integer (sc, lt.mday));  // day

  return time_vec;
}

inline void
glue_date_now (s7_scheme* sc) {
  const char* name= "g_date-now";
  const char* desc= "(g_date-now) => date, create a date object with current date";
  s7_define_function (sc, name, f_date_now, 0, 0, false, desc);
}

inline void
glue_liii_time (s7_scheme* sc) {
  glue_sleep (sc);
}

inline void
glue_liii_datetime (s7_scheme* sc) {
  glue_datetime_now (sc);
  glue_date_now (sc);
}

// -------------------------------- iota --------------------------------
static inline s7_pointer
iota_list (s7_scheme* sc, s7_int count, s7_pointer start, s7_int step) {
  s7_pointer res= s7_nil (sc);
  s7_int     val;
  for (val= s7_integer (start) + step * (count - 1); count > 0; count--) {
    res= s7_cons (sc, s7_make_integer (sc, val), res);
    val-= step;
  }
  return res;
}

static s7_pointer
iota_list_p_ppp (s7_scheme* sc, s7_pointer count, s7_pointer start, s7_pointer step) {
  if (!s7_is_integer (count)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 2, s7_make_string (sc, "iota: count must be an integer"), count));
  }
  if (!s7_is_integer (start)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 2, s7_make_string (sc, "iota: start must be an integer"), start));
  }
  if (!s7_is_integer (step)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 2, s7_make_string (sc, "iota: step must be an integer"), step));
  }
  s7_int cnt= s7_integer (count);
  if (cnt < 0) {
    return s7_error (sc, s7_make_symbol (sc, "value-error"),
                     s7_list (sc, 2, s7_make_string (sc, "iota: count is negative"), count));
  }
  s7_int st = s7_integer (start);
  s7_int stp= s7_integer (step);
  return iota_list (sc, cnt, start, stp);
}

static s7_pointer
g_iota_list (s7_scheme* sc, s7_pointer args) {
  s7_pointer arg1 = s7_car (args); // count
  s7_pointer rest1= s7_cdr (args);
  s7_pointer arg2 = (s7_is_pair (rest1)) ? s7_car (rest1) : s7_make_integer (sc, 0); // start value, default 0
  s7_pointer rest2= s7_cdr (rest1);
  s7_pointer arg3 = (s7_is_pair (rest2)) ? s7_car (rest2) : s7_make_integer (sc, 1); // step size, default 1
  return iota_list_p_ppp (sc, arg1, arg2, arg3);
}

inline void
glue_iota_list (s7_scheme* sc) {
  const char* name= "iota";
  const char* desc= "(iota count [start [step]]) => list, returns a list of count elements starting from start "
                    "(default 0) with step (default 1)";
  s7_define_function (sc, name, g_iota_list, 1, 2, false, desc);
}

inline void
glue_liii_list (s7_scheme* sc) {
  glue_iota_list (sc);
}

void
glue_for_community_edition (s7_scheme* sc) {
  glue_goldfish (sc);
  glue_scheme_time (sc);
  glue_scheme_process_context (sc);
  glue_liii_sys (sc);
  glue_liii_os (sc);
  glue_liii_path (sc);
  glue_liii_list (sc);
  glue_liii_time (sc);
  glue_liii_datetime (sc);
  glue_liii_uuid (sc);
  glue_liii_hashlib (sc);
  glue_njson (sc);
  glue_http (sc);
  glue_http_async (sc);
}

static void
display_help () {
  cout << "Goldfish Scheme " << GOLDFISH_VERSION << " by LiiiLabs" << endl;
  cout << endl;
  cout << "Commands:" << endl;
  cout << "  help               Display this help message" << endl;
  cout << "  version            Display version" << endl;
  cout << "  eval CODE          Evaluate Scheme code" << endl;
  cout << "                     Example: gf eval '(+ 1 2)'" << endl;
  cout << "                     Prefer single quotes so double quotes inside Scheme strings usually do not need escaping" << endl;
  cout << "  load FILE          Load Scheme code from FILE, then enter REPL" << endl;
  cout << "  fix [options] PATH Format PATH (PATH can be a .scm file or directory)" << endl;
  cout << "                     Options:" << endl;
  cout << "                       --dry-run  Print formatted result to stdout" << endl;
  cout << "  source ORG/LIB     Print the exact source of ORG/LIB from current *load-path*" << endl;
  cout << "                     Reads the real library file, not tests/ or generated docs" << endl;
  cout << "                     Example: gf source liii/path" << endl;
  cout << "  doc ORG/LIB        Show the library overview for ORG/LIB from tests/" << endl;
  cout << "                     Usually reads tests/ORG/LIB-test.scm" << endl;
  cout << "                     Example: gf doc liii/path" << endl;
  cout << "  doc ORG/LIB FUNC   Show the function doc/test file for FUNC under a specific library" << endl;
  cout << "                     Best when you already know the library, or the name is ambiguous" << endl;
  cout << "                     Example: gf doc liii/path \"path-read-text\"" << endl;
  cout << "                     Quote FUNC for names like \"bag-delete!\", \"path?\", \"alist->fxmapping\", or \"bag<=?\"" << endl;
  cout << "                     This preserves symbols such as ! ? > < and keeps FUNC as one shell argument" << endl;
  cout << "  doc FUNC           Search visible libraries for exported FUNC, then show its doc/test file" << endl;
  cout << "                     If multiple libraries export it, candidates are listed" << endl;
  cout << "                     Example: gf doc \"string-split\"" << endl;
  cout << "                     Quote FUNC for names like \"bag-delete!\", \"path?\", \"alist->fxmapping\", or \"bag<=?\"" << endl;
  cout << "                     This keeps shell-sensitive symbols intact and makes it clear FUNC is one argument" << endl;
  cout << "  doc --build-json   Rebuild tests/function-library-index.json for global gf doc FUNC lookup" << endl;
  cout << "                     Needed by function-name search and fuzzy suggestions" << endl;
  cout << "                     Run this after changing exports, or before packaging" << endl;
  cout << "  test [PATTERN]     Run tests (all *-test.scm files under tests/)" << endl;
  cout << "                     PATTERN can be:" << endl;
  cout << "                       (none)          Run all tests" << endl;
  cout << "                       FILE.scm        Run specific test file" << endl;
  cout << "                       DIR/            Run tests in directory" << endl;
  cout << "                       name-test.scm   Match by file name" << endl;
  cout << "                       substring       Match by path substring" << endl;
  cout << "  run TARGET         Run main function from TARGET" << endl;
  cout << "                     TARGET can be:" << endl;
  cout << "                       FILE.scm       Load file and run main" << endl;
  cout << "                       x/y/z.scm      Load file and run main" << endl;
  cout << "                       module.name    Import (module name) and run main" << endl;
#ifdef GOLDFISH_WITH_REPL
  cout << "  repl               Enter interactive REPL mode" << endl;
#endif
  cout << "  FILE               Load and evaluate Scheme code from FILE" << endl;
  cout << endl;
  cout << "Options:" << endl;
  cout << "  --mode, -m MODE    Set mode: default, liii, sicp, r7rs, s7" << endl;
  cout << "  -I DIR             Prepend DIR to library search path" << endl;
  cout << "  -A DIR             Append DIR to library search path" << endl;
  cout << endl;
  cout << "If no command is specified, help is displayed by default." << endl;
}

static void
display_version () {
  cout << "Goldfish Scheme " << GOLDFISH_VERSION << " by LiiiLabs" << endl;
  cout << "based on S7 Scheme " << S7_VERSION << " (" << S7_DATE << ")" << endl;
}

static void
display_for_invalid_options (const std::vector<std::string>& invalid_opts) {
  for (const auto& opt : invalid_opts) {
    std::cerr << "Invalid option: " << opt << "\n";
  }
  std::cerr << "\n";
  display_help ();
}

static void
goldfish_eval_file (s7_scheme* sc, string path, bool quiet) {
  s7_pointer result= s7_load (sc, path.c_str ());
  if (!result) {
    cerr << "Failed to load " << path << endl;
    exit (-1);
  }
  if (!quiet) {
    cout << path << " => " << s7_object_to_c_string (sc, result) << endl;
  }
}

static string
goldfish_cli_program_name () {
  if (!command_args.empty ()) {
    string program= fs::path (command_args.front ()).filename ().string ();
    if (!program.empty ()) {
      return program;
    }
  }
  return "gf";
}

static bool
goldfish_is_fix_hint_candidate_error (const string& errmsg) {
  return errmsg.find ("unexpected close paren") != string::npos || errmsg.find ("missing close paren") != string::npos;
}

static string
goldfish_extract_scheme_path_from_error (const string& errmsg) {
  size_t marker= errmsg.find (".scm[");
  while (marker != string::npos) {
    size_t start= marker;
    while (start > 0) {
      unsigned char ch= static_cast<unsigned char> (errmsg[start - 1]);
      if (std::isspace (ch) || ch == '"' || ch == '\'' || ch == '`' || ch == '(' || ch == ')' || ch == ',' || ch == ';') {
        break;
      }
      --start;
    }

    string candidate= errmsg.substr (start, marker + 4 - start);
    if (!candidate.empty ()) {
      return candidate;
    }

    marker= errmsg.find (".scm[", marker + 1);
  }

  return "";
}

static string
goldfish_extract_error_expression (const string& errmsg, size_t search_start) {
  const string infix= " in ";
  size_t       start= errmsg.find (infix, search_start);
  if (start == string::npos) {
    return "";
  }

  start += infix.size ();
  size_t end= errmsg.find ('\n', start);
  if (end == string::npos) {
    end= errmsg.size ();
  }

  return errmsg.substr (start, end - start);
}

static bool
goldfish_form_contains_called_symbol (s7_scheme* sc, s7_pointer form, const string& function_name) {
  if (s7_is_pair (form)) {
    s7_pointer operator_form= s7_car (form);
    if (s7_is_symbol (operator_form) && (function_name == s7_symbol_name (operator_form))) {
      return true;
    }

    for (s7_pointer iter= form; s7_is_pair (iter); iter= s7_cdr (iter)) {
      if (goldfish_form_contains_called_symbol (sc, s7_car (iter), function_name)) {
        return true;
      }
    }

    s7_pointer tail= form;
    while (s7_is_pair (tail)) {
      tail= s7_cdr (tail);
    }
    if ((!s7_is_null (sc, tail)) && goldfish_form_contains_called_symbol (sc, tail, function_name)) {
      return true;
    }
  }

  return false;
}

static bool
goldfish_error_expression_contains_function_call (s7_scheme* sc, const string& expression, const string& function_name) {
  if (expression.empty ()) {
    return false;
  }

  s7_pointer port     = s7_open_input_string (sc, expression.c_str ());
  s7_pointer eof_object= s7_eof_object (sc);
  s7_pointer form     = s7_read (sc, port);
  s7_close_input_port (sc, port);

  if ((form == eof_object) || (!form)) {
    return expression.find ("(" + function_name) != string::npos;
  }

  return goldfish_form_contains_called_symbol (sc, form, function_name);
}

static string
goldfish_extract_unbound_function_name_from_error (s7_scheme* sc, const string& errmsg) {
  const string prefix= "unbound variable ";
  size_t       start = errmsg.find (prefix);
  if (start == string::npos) {
    return "";
  }

  start += prefix.size ();
  size_t end= start;
  while (end < errmsg.size ()) {
    unsigned char ch= static_cast<unsigned char> (errmsg[end]);
    if (std::isspace (ch) || ch == '(' || ch == ')' || ch == ';') {
      break;
    }
    ++end;
  }

  if (end == start) {
    return "";
  }

  string function_name= errmsg.substr (start, end - start);
  if (errmsg.find ("in (" + function_name, end) != string::npos) {
    return function_name;
  }

  string error_expression= goldfish_extract_error_expression (errmsg, end);
  if (!goldfish_error_expression_contains_function_call (sc, error_expression, function_name)) {
    return "";
  }

  return function_name;
}

static string
goldfish_format_scheme_error_message (const char* errmsg) {
  if ((!errmsg) || (!*errmsg)) {
    return "";
  }

  string formatted= errmsg;
  if (formatted.find ("Hint: try `") != string::npos) {
    return formatted;
  }
  if (!goldfish_is_fix_hint_candidate_error (formatted)) {
    return formatted;
  }

  string path= goldfish_extract_scheme_path_from_error (formatted);
  if (path.empty ()) {
    return formatted;
  }

  if ((!formatted.empty ()) && (formatted.back () != '\n')) {
    formatted += '\n';
  }
  formatted += "Hint: try `" + goldfish_cli_program_name () + " fix " + path + "` to repair common parenthesis issues.\n";
  return formatted;
}

static string
goldfish_shell_double_quote (const string& value) {
  string quoted= "\"";
  for (char ch : value) {
    switch (ch) {
      case '\\':
        quoted += "\\\\";
        break;
      case '"':
        quoted += "\\\"";
        break;
      case '$':
        quoted += "\\$";
        break;
      case '`':
        quoted += "\\`";
        break;
      default:
        quoted += ch;
        break;
    }
  }
  quoted += "\"";
  return quoted;
}

static string
goldfish_library_display_name (const string& library_query) {
  string group;
  string library;
  if (!split_library_query (library_query, group, library)) {
    return library_query;
  }
  return "(" + group + " " + library + ")";
}

static string
goldfish_library_import_form (const string& library_query) {
  string group;
  string library;
  if (!split_library_query (library_query, group, library)) {
    return "";
  }
  return "(import (" + group + " " + library + "))";
}

static string
goldfish_library_doc_command (const string& library_query, const string& function_name) {
  return goldfish_cli_program_name () + " doc " + library_query + " " + goldfish_shell_double_quote (function_name);
}

static string
goldfish_append_doc_hint_if_needed (s7_scheme* sc, const string& errmsg) {
  if (errmsg.find ("Hint: try `") != string::npos) {
    return errmsg;
  }

  string function_name= goldfish_extract_unbound_function_name_from_error (sc, errmsg);
  if (function_name.empty ()) {
    return errmsg;
  }

  string formatted= errmsg;
  if ((!formatted.empty ()) && (formatted.back () != '\n')) {
    formatted += '\n';
  }

  vector<string> library_queries;
  try {
    library_queries= find_function_libraries_in_load_path (sc, function_name);
  }
  catch (const std::exception&) {
    library_queries.clear ();
  }

  if (library_queries.empty ()) {
    formatted += "Hint: try `" + goldfish_cli_program_name () + " doc " + goldfish_shell_double_quote (function_name) +
                 "`\n";
    formatted += "`" + goldfish_cli_program_name () + " doc` may show similarly named functions when there is no exact match.\n";
    formatted += "If it finds nothing similar, try searching the codebase with `git grep "
                 + goldfish_shell_double_quote (function_name)
                 + "`, implement that function yourself, or stop using it.\n";
    return formatted;
  }

  if (library_queries.size () == 1) {
    string import_form= goldfish_library_import_form (library_queries.front ());
    formatted += "Hint: function `" + function_name + "` exists in library `" +
                 goldfish_library_display_name (library_queries.front ()) + "`.\n";
    if (!import_form.empty ()) {
      formatted += "Please import that library first: `" + import_form + "`.\n";
    }
    return formatted;
  }

  formatted += "Hint: function `" + function_name + "` exists in multiple visible libraries:\n";
  for (const auto& library_query : library_queries) {
    formatted += "  " + goldfish_library_display_name (library_query) + "\n";
  }
  formatted += "Try one of these commands to decide which library to use:\n";
  for (const auto& library_query : library_queries) {
    formatted += "  " + goldfish_library_doc_command (library_query, function_name) + "\n";
  }
  return formatted;
}

static void
goldfish_render_scheme_error_message (s7_scheme* sc, const char* errmsg, string& rendered) {
  rendered= goldfish_append_doc_hint_if_needed (sc, goldfish_format_scheme_error_message (errmsg));
  if ((!rendered.empty ()) && (rendered.back () != '\n')) {
    rendered += '\n';
  }
}

static void
goldfish_print_scheme_error_message (s7_scheme* sc, const char* errmsg) {
  if ((errmsg) && (*errmsg)) {
    string rendered;
    goldfish_render_scheme_error_message (sc, errmsg, rendered);
    cout << rendered;
  }
}

static void
goldfish_print_prefixed_scheme_error_message (s7_scheme* sc, const string& prefix, const char* errmsg) {
  if ((errmsg) && (*errmsg)) {
    string rendered;
    goldfish_render_scheme_error_message (sc, errmsg, rendered);
    cerr << prefix;
    if ((!prefix.empty ()) && (prefix.back () != '\n')) {
      cerr << '\n';
    }
    cerr << rendered;
  }
}

static void
goldfish_eval_code (s7_scheme* sc, string code) {
  string wrapped_code = "(begin " + code + " )";
  s7_pointer x= s7_eval_c_string (sc, wrapped_code.c_str ());
  cout << s7_object_to_c_string (sc, x) << endl;
}

struct GoldfixCliOptions {
  bool   enabled= false;
  bool   dry_run= false;
  string path;
  string error;
};

static bool
string_starts_with (const string& value, const string& prefix) {
  return value.rfind (prefix, 0) == 0;
}

static GoldfixCliOptions
parse_goldfix_cli_options (int argc, char** argv) {
  GoldfixCliOptions opts;

  // Look for 'fix' subcommand
  int fix_index= -1;
  for (int i= 1; i < argc; ++i) {
    string arg= argv[i];
    if (arg == "fix") {
      fix_index= i;
      break;
    }
  }

  if (fix_index == -1) {
    return opts; // No fix subcommand found
  }

  opts.enabled= true;

  // Parse options after 'fix' subcommand
  bool path_set= false;
  for (int i= fix_index + 1; i < argc; ++i) {
    string arg= argv[i];

    if (arg == "--dry-run") {
      if (opts.dry_run) {
        opts.error= "Error: '--dry-run' can only be specified once.";
        return opts;
      }
      opts.dry_run= true;
      continue;
    }

    // Check for options that start with '-' but are not recognized
    if (arg.length () > 0 && arg[0] == '-') {
      // This is an unrecognized option, will be caught by invalid flag check
      continue;
    }

    // This should be the PATH argument
    if (!path_set) {
      opts.path  = arg;
      path_set   = true;
    }
  }

  if (!path_set || opts.path.empty ()) {
    opts.error= "Error: 'fix' requires a PATH argument.";
    return opts;
  }

  return opts;
}

static bool
is_goldfix_option_flag (const string& flag) {
  return flag == "fix" || flag == "--dry-run" || flag == "dry-run";
}

static vector<string>
filter_invalid_options_for_goldfix (const vector<string>& flags) {
  vector<string> filtered;
  for (const auto& flag : flags) {
    if (!is_goldfix_option_flag (flag)) {
      filtered.push_back (flag);
    }
  }
  return filtered;
}

static string
find_goldfix_tool_root (const char* gf_lib) {
  std::error_code ec;
  vector<fs::path> candidates= {fs::path (gf_lib) / "tools" / "goldfix", fs::path (gf_lib).parent_path () / "tools" / "goldfix"};

  for (const auto& candidate : candidates) {
    if (fs::is_directory (candidate, ec)) {
      return candidate.string ();
    }
    ec.clear ();
  }

  return "";
}

static string
find_goldtest_tool_root (const char* gf_lib) {
  std::error_code ec;
  vector<fs::path> candidates= {fs::path (gf_lib) / "tools" / "goldtest", fs::path (gf_lib).parent_path () / "tools" / "goldtest"};

  for (const auto& candidate : candidates) {
    if (fs::is_directory (candidate, ec)) {
      return candidate.string ();
    }
    ec.clear ();
  }

  return "";
}

static string
find_golddoc_tool_root (const char* gf_lib) {
  std::error_code ec;
  vector<fs::path> candidates= {fs::path (gf_lib) / "tools" / "golddoc", fs::path (gf_lib).parent_path () / "tools" / "golddoc"};

  for (const auto& candidate : candidates) {
    if (fs::is_directory (candidate, ec)) {
      return candidate.string ();
    }
    ec.clear ();
  }

  return "";
}

static string
find_goldsource_tool_root (const char* gf_lib) {
  std::error_code ec;
  vector<fs::path> candidates= {fs::path (gf_lib) / "tools" / "goldsource", fs::path (gf_lib).parent_path () / "tools" / "goldsource"};

  for (const auto& candidate : candidates) {
    if (fs::is_directory (candidate, ec)) {
      return candidate.string ();
    }
    ec.clear ();
  }

  return "";
}

static void
add_goldfix_load_path_if_present (s7_scheme* sc, const char* gf_lib) {
  string tool_root= find_goldfix_tool_root (gf_lib);
  if (!tool_root.empty ()) {
    s7_add_to_load_path (sc, tool_root.c_str ());
  }
}

static string
current_scheme_error_output (s7_scheme* sc) {
  const char* errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
  if ((errmsg) && (*errmsg)) {
    return string (errmsg);
  }
  return "";
}

static string
read_text_file_exact (const fs::path& path) {
  std::ifstream input (path, std::ios::binary);
  if (!input.is_open ()) {
    throw std::runtime_error ("Failed to open file for reading: " + path.string ());
  }

  std::ostringstream buffer;
  buffer << input.rdbuf ();
  if (input.bad ()) {
    throw std::runtime_error ("Failed to read file: " + path.string ());
  }

  return buffer.str ();
}

static void
write_text_file_exact (const fs::path& path, const string& content) {
  std::ofstream output (path, std::ios::binary | std::ios::trunc);
  if (!output.is_open ()) {
    throw std::runtime_error ("Failed to open file for writing: " + path.string ());
  }

  output.write (content.data (), static_cast<std::streamsize> (content.size ()));
  if (!output) {
    throw std::runtime_error ("Failed to write file: " + path.string ());
  }
}

static bool
is_scheme_source_file (const fs::path& path) {
  return path.has_extension () && path.extension () == ".scm";
}

static vector<fs::path>
collect_goldfix_targets (const fs::path& input_path, bool dry_run) {
  std::error_code ec;
  if (!fs::exists (input_path, ec)) {
    throw std::runtime_error ("Path does not exist: " + input_path.string ());
  }

  if (fs::is_regular_file (input_path, ec)) {
    return {input_path};
  }

  if (dry_run) {
    throw std::runtime_error ("'fix --dry-run' only supports files.");
  }

  if (!fs::is_directory (input_path, ec)) {
    throw std::runtime_error ("Unsupported path: " + input_path.string ());
  }

  vector<fs::path> files;
  for (fs::recursive_directory_iterator it (input_path, fs::directory_options::skip_permission_denied, ec), end; it != end;
       it.increment (ec)) {
    if (ec) {
      throw std::runtime_error ("Failed to walk directory: " + input_path.string ());
    }
    if (it->is_regular_file (ec) && is_scheme_source_file (it->path ())) {
      files.push_back (it->path ());
    }
    ec.clear ();
  }

  std::sort (files.begin (), files.end (), [] (const fs::path& lhs, const fs::path& rhs) { return lhs.string () < rhs.string (); });
  return files;
}

static s7_pointer
require_goldfix_fix_content (s7_scheme* sc, const char* gf_lib) {
  string tool_root= find_goldfix_tool_root (gf_lib);
  if (tool_root.empty ()) {
    throw std::runtime_error ("Goldfix module directory does not exist.");
  }

  s7_add_to_load_path (sc, tool_root.c_str ());
  s7_eval_c_string (sc, "(import (liii goldfix))");
  string scheme_error= current_scheme_error_output (sc);
  if (!scheme_error.empty ()) {
    throw std::runtime_error ("Failed to import (liii goldfix).");
  }

  s7_pointer fix_content= s7_name_to_value (sc, "fix-content");
  if ((!fix_content) || (!s7_is_procedure (fix_content))) {
    throw std::runtime_error ("Failed to resolve fix-content from (liii goldfix).");
  }

  return fix_content;
}

static string
goldfix_fix_content (s7_scheme* sc, s7_pointer fix_content, const string& content) {
  s7_pointer result= s7_call (sc, fix_content, s7_list (sc, 1, s7_make_string (sc, content.c_str ())));
  if (!result || !s7_is_string (result)) {
    throw std::runtime_error ("(liii goldfix) fix-content did not return a string.");
  }
  return string (s7_string (result));
}

static string
goldfix_progress_prefix (std::size_t index, std::size_t total) {
  std::ostringstream out;
  out << "[" << index << "/" << total << "]";
  return out.str ();
}

static int
goldfish_run_fix_mode (s7_scheme* sc, const char* gf_lib, const GoldfixCliOptions& opts) {
  try {
    s7_pointer      fix_content= require_goldfix_fix_content (sc, gf_lib);
    vector<fs::path> files     = collect_goldfix_targets (fs::path (opts.path), opts.dry_run);
    std::ostream&    status_out= std::cerr;

    if (opts.dry_run) {
      if (files.empty ()) {
        throw std::runtime_error ("No input file provided for 'fix --dry-run'.");
      }
      const fs::path& file  = files.front ();
      string          prefix= goldfix_progress_prefix (1, 1);
      status_out << prefix << " Processing " << file.string () << std::endl;
      cout << goldfix_fix_content (sc, fix_content, read_text_file_exact (file));
      status_out << prefix << " Dry-run complete " << file.string () << std::endl;
      return current_scheme_error_output (sc).empty () ? 0 : -1;
    }

    std::size_t changed_count= 0;
    for (std::size_t i= 0; i < files.size (); ++i) {
      const fs::path& file  = files[i];
      string          prefix= goldfix_progress_prefix (i + 1, files.size ());
      status_out << prefix << " Processing " << file.string () << std::endl;

      try {
        string original= read_text_file_exact (file);
        string fixed   = goldfix_fix_content (sc, fix_content, original);
        if (!current_scheme_error_output (sc).empty ()) {
          status_out << prefix << " Failed " << file.string () << std::endl;
          return -1;
        }
        if (fixed != original) {
          write_text_file_exact (file, fixed);
          ++changed_count;
          status_out << prefix << " Updated " << file.string () << std::endl;
        }
        else {
          status_out << prefix << " Unchanged " << file.string () << std::endl;
        }
      }
      catch (const std::exception& err) {
        status_out << prefix << " Failed " << file.string () << ": " << err.what () << std::endl;
        return 1;
      }
    }

    if (fs::is_directory (fs::path (opts.path))) {
      status_out << "Processed " << files.size () << " .scm file(s), updated " << changed_count << std::endl;
    }
    return 0;
  }
  catch (const std::exception& err) {
    cerr << err.what () << endl;
    return 1;
  }
}

s7_scheme*
init_goldfish_scheme (const char* gf_lib) {
  s7_scheme* sc= s7_init ();
  s7_add_to_load_path (sc, gf_lib);
  add_goldfix_load_path_if_present (sc, gf_lib);

  if (!tb_init (tb_null, tb_null)) exit (-1);

  glue_for_community_edition (sc);
  return sc;
}

void
customize_goldfish_by_mode (s7_scheme* sc, string mode, const char* boot_file_path) {
  if (mode != "s7") {
    s7_load (sc, boot_file_path);
  }

  if (mode == "default" || mode == "liii") {
    s7_eval_c_string (sc, "(import (liii base) (liii error) (liii string))");
  }
  else if (mode == "scheme") {
    s7_eval_c_string (sc, "(import (liii base) (liii error))");
  }
  else if (mode == "sicp") {
    s7_eval_c_string (sc, "(import (scheme base) (srfi sicp))");
  }
  else if (mode == "r7rs") {
    s7_eval_c_string (sc, "(import (scheme base))");
  }
  else if (mode == "s7") {
  }
  else {
    cerr << "No such mode: " << mode << endl;
    exit (-1);
  }
}

string
find_goldfish_library () {
  string exe_path= goldfish_exe ();

  tb_char_t        data_bin[TB_PATH_MAXN]= {0};
  tb_char_t const* ret_bin               = tb_path_directory (exe_path.c_str (), data_bin, sizeof (data_bin));

  tb_char_t        data_root[TB_PATH_MAXN]= {0};
  tb_char_t const* gf_root                = tb_path_directory (ret_bin, data_root, sizeof (data_root));

  tb_char_t        data_lib[TB_PATH_MAXN]= {0};
  tb_char_t const* gf_lib                = tb_path_absolute_to (gf_root, "share/goldfish", data_lib, sizeof (data_lib));
#ifdef TB_CONFIG_OS_LINUX
  if (strcmp (gf_root, "/") == 0) {
    gf_lib= "/usr/share/goldfish";
  }
#endif

  if (!tb_file_access (gf_lib, TB_FILE_MODE_RO)) {
    gf_lib= tb_path_absolute_to (gf_root, "goldfish", data_lib, sizeof (data_lib));
    if (!tb_file_access (gf_lib, TB_FILE_MODE_RO)) {
      cerr << "The load path for Goldfish standard library does not exist" << endl;
      exit (-1);
    }
  }

  return string (gf_lib);
}

string
find_goldfish_boot (const char* gf_lib) {
  tb_char_t        data_boot[TB_PATH_MAXN]= {0};
  tb_char_t const* gf_boot= tb_path_absolute_to (gf_lib, "scheme/boot.scm", data_boot, sizeof (data_boot));

  if (!tb_file_access (gf_boot, TB_FILE_MODE_RO)) {
    cerr << "The boot.scm for Goldfish Scheme does not exist" << endl;
    exit (-1);
  }
  return string (gf_boot);
}

#ifdef GOLDFISH_WITH_REPL
struct SymbolInfo {
  std::string name;
  std::string doc;
};
static std::vector<SymbolInfo> cached_symbols;

// UNLIMITED history
// TODO(jinser): 1. programatic value-history procedure api in scheme
//               2. `,option value-history` meta command
static std::vector<s7_pointer> history_values;

inline void
update_symbol_cache (s7_scheme* sc) {
  cached_symbols.clear ();
  s7_pointer cur_env = s7_curlet (sc);
  s7_pointer sym_list= s7_let_to_list (sc, cur_env);
  int        n       = s7_list_length (sc, sym_list);
  for (int i= 0; i < n; ++i) {
    s7_pointer  pair= s7_list_ref (sc, sym_list, i);
    s7_pointer  sym = s7_car (pair);
    s7_pointer  val = s7_cdr (pair);
    const char* name= s7_symbol_name (sym);
    const char* doc = s7_documentation (sc, val);
    cached_symbols.push_back ({name, doc ? doc : ""});
  }
}

inline void
ic_goldfish_eval (s7_scheme* sc, const char* code) {
  int        err_gc_loc= -1, out_gc_loc= -1;
  s7_pointer old_err_port= s7_set_current_error_port (sc, s7_open_output_string (sc));
  if (old_err_port != s7_nil (sc)) err_gc_loc= s7_gc_protect (sc, old_err_port);

  s7_pointer out_port    = s7_open_output_string (sc);
  s7_pointer old_out_port= s7_set_current_output_port (sc, out_port);
  if (old_err_port != s7_nil (sc)) out_gc_loc= s7_gc_protect (sc, old_out_port);

  s7_pointer result= s7_eval_c_string (sc, code);

  const char* display_out= s7_get_output_string (sc, out_port);
  if (display_out && *display_out) {
    std::string out_str= display_out;
    if (!out_str.empty () && out_str.back () == '\n') {
      ic_printf ("%s", display_out);
    }
    else {
      // 用以表示换行符由 REPL 添加
      ic_printf ("%s↩\n", display_out);
    }
  }

  const char* errmsg= s7_get_output_string (sc, s7_current_error_port (sc));

  if (errmsg && *errmsg) {
    string rendered;
    goldfish_render_scheme_error_message (sc, errmsg, rendered);
    ic_printf ("[error]%s[/]", rendered.c_str ());
  }
  if (result) {
    history_values.push_back (result);
    s7_gc_protect (sc, result);
    std::string name   = "$" + std::to_string (history_values.size ());
    s7_pointer  cur_env= s7_curlet (sc);
    s7_define (sc, cur_env, s7_make_symbol (sc, name.c_str ()), result);

    char* result_str= s7_object_to_c_string (sc, result);
    if (result_str) {
      ic_printf ("%s [gray]=[/] %s\n", name.c_str (), result_str);
      free (result_str);
    }
  }

  s7_close_output_port (sc, s7_current_error_port (sc));
  s7_set_current_error_port (sc, old_err_port);

  if (err_gc_loc != -1) s7_gc_unprotect_at (sc, err_gc_loc);
  if (out_gc_loc != -1) s7_gc_unprotect_at (sc, out_gc_loc);

  update_symbol_cache (sc);
}

inline std::string
get_history_path () {
#ifdef TB_CONFIG_OS_WINDOWS
  const char* appdata= getenv ("APPDATA");
  std::string dir    = appdata ? std::string (appdata) + "\\goldfish" : ".";
  tb_directory_create (dir.c_str ());
  std::string path= dir + "\\history";
#else
  const char* xdg_state= getenv ("XDG_STATE_HOME");
  const char* xdg_data = getenv ("XDG_DATA_HOME");
  const char* home     = getenv ("HOME");
  std::string dir;
  if (xdg_data) {
    dir= std::string (xdg_data) + "/goldfish";
  }
  else if (home) {
    dir= std::string (home) + "/.local/share/goldfish";
  }
  else {
    dir= ".";
  }
  // 可选：创建目录
  tb_directory_create (dir.c_str ());
  std::string path= dir + "/history";
#endif
  return path;
}

inline bool
is_symbol_char (const char* s, long len) {
  int c= (unsigned char) *s;
  return isalnum (c) || strchr ("!$%&*/:<=>?^_~+-.", c);
}

inline void
symbol_completer (ic_completion_env_t* cenv, const char* symbol) {
  constexpr size_t MAXLEN   = 79;
  size_t           input_len= strlen (symbol);
  for (const auto& info : cached_symbols) {
    if (strncmp (info.name.c_str (), symbol, input_len) == 0) {
      const char* doc= nullptr;
      std::string short_doc;
      if (!info.doc.empty ()) {
        if (info.doc.length () > MAXLEN) {
          short_doc= info.doc.substr (0, MAXLEN) + "...";
          doc      = short_doc.c_str ();
        }
        else {
          doc= info.doc.c_str ();
        }
      }
      ic_add_completion_ex (cenv, info.name.c_str (), info.name.c_str (), doc);
    }
  }
}

inline void
goldfish_completer (ic_completion_env_t* cenv, const char* input) {
  ic_complete_word (cenv, input, &symbol_completer, is_symbol_char);
}

inline void
goldfish_highlighter (ic_highlight_env_t* henv, const char* input, void* arg) {
  static const char* keywords[]= {"define",
                                  "lambda",
                                  "if",
                                  "else",
                                  "let",
                                  "let*",
                                  "letrec",
                                  "begin",
                                  "quote",
                                  "set!",
                                  "cond",
                                  "case",
                                  "and",
                                  "or",
                                  "do",
                                  "delay",
                                  "quasiquote",
                                  "unquote",
                                  "unquote-splicing",
                                  NULL};
  long               len       = (long) strlen (input);
  for (long i= 0; i < len;) {
    long tlen;
    if ((tlen= ic_match_any_token (input, i, &ic_char_is_idletter, keywords)) > 0) {
      // 关键字
      ic_highlight (henv, i, tlen, "keyword");
      i+= tlen;
    }
    else if ((tlen= ic_is_token (input, i, &is_symbol_char)) > 0) {
      // 已定义符号

      std::string token (input + i, tlen);
      if (std::any_of (cached_symbols.begin (), cached_symbols.end (),
                       [&] (const SymbolInfo& info) { return info.name == token; })) {
        ic_highlight (henv, i, tlen, "symbol");
      }
      else {
        ic_highlight (henv, i, tlen, nullptr);
      }
      i+= tlen;
    }
    else if ((tlen= ic_is_token (input, i, &ic_char_is_digit)) > 0) {
      // 数字
      ic_highlight (henv, i, tlen, "number");
      i+= tlen;
    }
    else if (input[i] == '#' && (input[i + 1] == 't' || input[i + 1] == 'f')) {
      // 布尔值
      ic_highlight (henv, i, 2, "constant");
      i+= 2;
    }
    else if (input[i] == '"') {
      long start= i;
      i++;
      while (i < len && input[i] != '"') {
        if (input[i] == '\\' && i + 1 < len) i++; // 跳过转义
        i++;
      }
      if (i < len) i++; // 包含结尾引号
      ic_highlight (henv, start, i - start, "string");
    }
    else if (input[i] == ';') {
      // 注释
      long start= i;
      while (i < len && input[i] != '\n')
        i++;
      ic_highlight (henv, start, i - start, "comment");
    }
    else {
      // 其它
      ic_highlight (henv, i, 1, nullptr);
      i++;
    }
  }
}

struct MetaCommand {
  const char* name;
  const char* help;
  bool        exact;

  std::function<bool (const char* input, s7_scheme* sc, const char* arg)> handler;
};

inline bool meta_quit (const char*, s7_scheme*, const char*);
inline bool meta_help (const char*, s7_scheme*, const char*);
inline bool meta_import (const char*, s7_scheme*, const char*);
inline bool meta_apropos (const char*, s7_scheme* sc, const char* arg);
inline bool meta_describe (const char*, s7_scheme* sc, const char* arg);

const MetaCommand commands[]= {
    {",quit", "exit REPL", true, meta_quit},
    {",q", "exit REPL", true, meta_quit},
    {",help", "show this help", true, meta_help},
    {",?", "show this help", true, meta_help},
    {",import", "import Scheme module", false, meta_import},
    {",apropos", "search symbols by substring", false, meta_apropos},
    {",a", "search symbols by substring", false, meta_apropos},
    {",describe", "describe symbol", false, meta_describe},
    {",d", "describe symbol", false, meta_describe},
};
const size_t commands_count= sizeof (commands) / sizeof (commands[0]);

inline bool
meta_quit (const char*, s7_scheme*, const char*) {
  return true;
}

// TODO: ,help <command>
inline bool
meta_help (const char*, s7_scheme*, const char*) {
  ic_printf ("[b]Meta commands:[/]\n");
  for (const auto& cmd : commands) {
    ic_printf ("[b]%-16s[/] %s\n", cmd.name, cmd.help);
  }
  return false;
}

inline bool
meta_import (const char*, s7_scheme* sc, const char* arg) {
  if (!arg || *arg == 0) {
    ic_printf ("[red]Usage:[/] ,import <module>\n");
    return false;
  }
  std::string mod = arg;
  std::string code= "(import " + mod + ")";

  ic_goldfish_eval (sc, code.c_str ());

  return false;
}

inline bool
meta_apropos (const char*, s7_scheme*, const char* arg) {
  if (!arg || !*arg) {
    ic_printf ("[b]Usage:[/] ,apropos <substring>\n");
    return false;
  }
  int found= false;
  for (const auto& info : cached_symbols) {
    if (strstr (info.name.c_str (), arg)) {
      ic_printf ("[b cyan]%s[/] [dim](procedure)[/] %s\n", info.name.c_str (),
                 info.doc.empty () ? "" : info.doc.c_str ());
      found= true;
    }
  }
  if (!found) ic_printf ("[dim]No symbol matches '%s'[/]\n", arg);
  return false;
}

inline bool
meta_describe (const char*, s7_scheme* sc, const char* arg) {
  if (!arg || !*arg) {
    ic_printf ("[b]Usage:[/] ,describe <symbol>\n");
    return false;
  }
  // 查找符号
  s7_pointer sym= s7_make_symbol (sc, arg);

  // 检查是否已定义
  if (!s7_is_defined (sc, s7_symbol_name (sym))) {
    ic_printf ("[dim]Symbol not defined: %s[/]\n", arg);
    return false;
  }
  s7_pointer  val = s7_symbol_value (sc, sym);
  const char* type= s7_object_to_c_string (sc, s7_type_of (sc, val));
  ic_printf ("[b]%s[/] [dim](%s)[/]\n", arg, type);

  if (s7_is_procedure (val)) {
    // 参数信息
    s7_pointer arity   = s7_arity (sc, val);
    s7_int     min_args= s7_integer (s7_car (arity));
    s7_int     max_args= s7_integer (s7_cdr (arity));

    std::string max_str= (max_args >= 0x20000000) ? "any" : std::to_string (max_args);
    ic_printf ("  [gray]Arity:[/] min [number]%d[/], max [number]%s[/]\n", min_args, max_str.c_str ());

    s7_pointer sig= s7_signature (sc, val);
    if (sig && !s7_is_null (sc, sig)) {
      char* sig_str= s7_object_to_c_string (sc, sig);
      if (sig_str) {
        ic_printf ("  [gray]Signature:[/] %s\n", sig_str);
        free (sig_str);
      }
    }

    // 文档
    const char* doc= s7_documentation (sc, val);
    if (doc && *doc) {
      ic_printf ("  [gray]Doc:[/] %s\n", doc);
    }
  }
  else {
    char*       val_str= s7_object_to_c_string (sc, val);
    std::string preview;
    if (val_str) {
      preview= std::string (val_str).substr (0, 80);
      if (strlen (val_str) > 80) preview+= "...";
    }
    else {
      preview= "";
    }
    ic_printf ("  [gray]Value:[/] %s\n", preview.c_str ());
    if (val_str) free (val_str);
  }
  return false;
}

inline bool
handle_meta_command (const char* input, s7_scheme* sc) {
  for (const auto& cmd : commands) {
    size_t len= strlen (cmd.name);
    if (cmd.exact) {
      if (strcmp (input, cmd.name) == 0) return cmd.handler (input, sc, nullptr);
    }
    else {
      if (strncmp (input, cmd.name, len) == 0) {
        // 跳过空格
        const char* arg= input + len + 1;
        while (*arg == ' ')
          ++arg;
        return cmd.handler (input, sc, input + len + 1);
      }
    }
  }
  ic_printf ("[red]Unknown meta command:[/] %s\n", input);
  return false;
}

inline void
goldfish_repl (s7_scheme* sc, const string& mode) {
  setlocale (LC_ALL, "C.UTF-8");
  ic_style_def ("kbd", "gray underline");
  ic_style_def ("ic-prompt", "gold");

  // 自定义样式
  ic_style_def ("error", "red");
  ic_style_def ("symbol", "cyan");

  ic_printf ("[b gold]Goldfish Scheme[/] [b plum]%s[/] by LiiiLabs\n"
             "[i]Based on S7 Scheme %s [dim](%s)[/][/]\n",
             GOLDFISH_VERSION, S7_VERSION, S7_DATE);
  // Display mode info; liii mode shows extra imported libraries
  if (mode == "liii" || mode == "default") {
    ic_printf ("[b]Mode:[/] [b]%s[/] (additionally imports: (liii base) (liii error) (liii string) compared to r7rs)\n\n",
               mode.c_str ());
  }
  else {
    ic_printf ("[b]Mode:[/] [b]%s[/]\n\n", mode.c_str ());
  }
  ic_printf ("- Type ',quit' or ',q' to quit. (or use [kbd]ctrl-d[/]).\n"
             "- Type ',help' for REPL commands help.\n"
             "- Press [kbd]F1[/] for help on editing commands.\n"
             "- Use [kbd]shift-tab[/] for multiline input. (or [kbd]ctrl-enter[/], or [kbd]ctrl-j[/])\n"
             "- Use [kbd]ctrl-r[/] to search the history.\n\n");

  auto history_path= get_history_path ();
  ic_set_history (history_path.c_str (), -1);

  ic_set_default_completer (&goldfish_completer, sc);
  ic_set_default_highlighter (&goldfish_highlighter, nullptr);

  //  prompt_marker, continuation_prompt_marker
  ic_set_prompt_marker ("> ", "... ");
  ic_enable_auto_tab (true);
  // 缓存的符号向量，只需要查表，没有必要延迟
  ic_set_hint_delay (0);

  update_symbol_cache (sc);

  while (true) {
    char* input= ic_readline ("gf");
    if (!input) break;
    if (strlen (input) == 0) {
      free (input);
      continue;
    }
    if (input[0] == ',') {
      bool quit= handle_meta_command (input, sc);
      free (input);
      if (quit) break;
      continue;
    }

    ic_goldfish_eval (sc, input);
  }
}
#endif

struct StartupCliOptions {
  string         mode= "default";
  vector<string> prepend_dirs;
  vector<string> append_dirs;
  string         command;
  int            command_index= -1;
  string         error;
};

static std::string
parse_mode_option (int argc, char** argv, const std::string& default_mode= "default") {
  std::string mode= default_mode;
  for (int i= 1; i < argc; ++i) {
    string arg= argv[i];
    if ((arg == "--mode" || arg == "-m") && (i + 1) < argc) {
      mode= argv[++i];
    }
    else if (arg.rfind ("--mode=", 0) == 0) {
      mode= arg.substr (7);
    }
    else if (arg.rfind ("-m=", 0) == 0) {
      mode= arg.substr (3);
    }
  }
  return mode;
}

static bool
is_legacy_cli_command (const string& arg) {
  return arg == "--help" || arg == "-h" || arg == "--version" || arg == "-v";
}

static string
normalize_load_path_dir (const string& raw_dir) {
  fs::path path (raw_dir);
  string   normalized= path.lexically_normal ().string ();
  return normalized.empty () ? raw_dir : normalized;
}

static bool
load_path_directory_exists (const string& raw_dir) {
  std::error_code ec;
  fs::path        path (normalize_load_path_dir (raw_dir));
  return fs::exists (path, ec) && fs::is_directory (path, ec);
}

static bool
append_unique_string (vector<string>& items, const string& raw_item) {
  string item= normalize_load_path_dir (raw_item);
  if (item.empty ()) return false;
  if (std::find (items.begin (), items.end (), item) != items.end ()) return false;
  items.push_back (item);
  return true;
}

static bool
is_plugin_name_part (const string& value) {
  if (value.empty ()) return false;
  return std::all_of (value.begin (), value.end (), [] (unsigned char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'); });
}

static bool
is_auto_goldfish_plugin_dir_name (const string& name) {
  size_t dash_pos= name.find ('-');
  if (dash_pos == string::npos || dash_pos == 0 || dash_pos == name.length () - 1) {
    return false;
  }
  if (name.find ('-', dash_pos + 1) != string::npos) {
    return false;
  }
  return is_plugin_name_part (name.substr (0, dash_pos)) && is_plugin_name_part (name.substr (dash_pos + 1));
}

static bool
directory_contains_scheme_sources (const fs::path& dir) {
  std::error_code ec;
  for (fs::recursive_directory_iterator it (dir, fs::directory_options::skip_permission_denied, ec), end; it != end;
       it.increment (ec)) {
    if (ec) {
      ec.clear ();
      continue;
    }
    if (it->is_regular_file (ec) && it->path ().extension () == ".scm") {
      return true;
    }
    ec.clear ();
  }
  return false;
}

static vector<string>
discover_auto_goldfish_library_dirs () {
  vector<string> dirs;
  const char*    home= getenv ("HOME");
  if ((!home) || (!*home)) {
    return dirs;
  }

  std::error_code ec;
  fs::path        root= fs::path (home) / ".local" / "goldfish";
  if (!fs::exists (root, ec) || !fs::is_directory (root, ec)) {
    return dirs;
  }

  for (fs::directory_iterator it (root, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment (ec)) {
    if (ec) {
      ec.clear ();
      continue;
    }
    if (!it->is_directory (ec)) {
      ec.clear ();
      continue;
    }

    string name= it->path ().filename ().string ();
    if (!is_auto_goldfish_plugin_dir_name (name)) {
      ec.clear ();
      continue;
    }
    if (!directory_contains_scheme_sources (it->path ())) {
      ec.clear ();
      continue;
    }

    append_unique_string (dirs, it->path ().string ());
    ec.clear ();
  }

  std::sort (dirs.begin (), dirs.end ());
  return dirs;
}

static vector<string>
current_load_path_entries (s7_scheme* sc) {
  vector<string> entries;
  for (s7_pointer rest= s7_load_path (sc); s7_is_pair (rest); rest= s7_cdr (rest)) {
    s7_pointer entry= s7_car (rest);
    if (s7_is_string (entry)) {
      append_unique_string (entries, string (s7_string (entry)));
    }
  }
  return entries;
}

static void
set_load_path_entries (s7_scheme* sc, const vector<string>& entries) {
  s7_pointer list= s7_nil (sc);
  for (auto it= entries.rbegin (); it != entries.rend (); ++it) {
    list= s7_cons (sc, s7_make_string (sc, it->c_str ()), list);
  }
  s7_symbol_set_value (sc, s7_make_symbol (sc, "*load-path*"), list);
}

static void
prepend_load_path_entries (s7_scheme* sc, const vector<string>& prepend_dirs) {
  vector<string> seen= current_load_path_entries (sc);
  for (auto it= prepend_dirs.rbegin (); it != prepend_dirs.rend (); ++it) {
    string dir= normalize_load_path_dir (*it);
    if (dir.empty ()) continue;
    if (std::find (seen.begin (), seen.end (), dir) != seen.end ()) continue;
    s7_add_to_load_path (sc, dir.c_str ());
    seen.insert (seen.begin (), dir);
  }
}

static void
append_load_path_entries (s7_scheme* sc, const vector<string>& append_dirs) {
  vector<string> entries= current_load_path_entries (sc);
  bool           changed= false;
  for (const auto& raw_dir : append_dirs) {
    string dir= normalize_load_path_dir (raw_dir);
    if (dir.empty ()) continue;
    if (std::find (entries.begin (), entries.end (), dir) != entries.end ()) continue;
    entries.push_back (dir);
    changed= true;
  }
  if (changed) {
    set_load_path_entries (sc, entries);
  }
}

static bool
append_unique_exact_string (vector<string>& items, const string& value) {
  if (value.empty ()) return false;
  if (std::find (items.begin (), items.end (), value) != items.end ()) return false;
  items.push_back (value);
  return true;
}

static bool
string_is_decimal_integer (const string& value) {
  if (value.empty ()) return false;

  size_t index= 0;
  if (value[0] == '+' || value[0] == '-') {
    index= 1;
  }
  if (index >= value.size ()) return false;

  return std::all_of (value.begin () + static_cast<std::ptrdiff_t> (index), value.end (),
                      [] (unsigned char ch) { return std::isdigit (ch) != 0; });
}

static s7_pointer
make_library_name_part (s7_scheme* sc, const string& part) {
  if (string_is_decimal_integer (part)) {
    try {
      return s7_make_integer (sc, std::stoll (part));
    }
    catch (const std::exception&) {
    }
  }
  return s7_make_symbol (sc, part.c_str ());
}

static bool
split_library_query (const string& query, string& group, string& library) {
  size_t slash_pos= query.find ('/');
  if (slash_pos == string::npos || slash_pos == 0 || slash_pos == query.size () - 1) {
    return false;
  }
  if (query.find ('/', slash_pos + 1) != string::npos) {
    return false;
  }
  group  = query.substr (0, slash_pos);
  library= query.substr (slash_pos + 1);
  return true;
}

static bool
is_named_symbol (s7_pointer value, const char* name) {
  return s7_is_symbol (value) && (strcmp (s7_symbol_name (value), name) == 0);
}

static bool
library_name_part_to_string (s7_pointer value, string& out) {
  if (s7_is_symbol (value)) {
    out= s7_symbol_name (value);
    return true;
  }
  if (s7_is_integer (value)) {
    out= std::to_string (s7_integer (value));
    return true;
  }
  return false;
}

static bool
extract_library_name_from_form (s7_scheme* sc, s7_pointer library_name_form, string& group, string& library) {
  if ((!s7_is_list (sc, library_name_form)) || (s7_list_length (sc, library_name_form) != 2)) {
    return false;
  }

  return library_name_part_to_string (s7_car (library_name_form), group) &&
         library_name_part_to_string (s7_cadr (library_name_form), library);
}

static bool
export_spec_name_matches (s7_scheme* sc, s7_pointer export_spec, const string& function_name) {
  if (s7_is_symbol (export_spec)) {
    return function_name == s7_symbol_name (export_spec);
  }

  if (s7_is_list (sc, export_spec) && (s7_list_length (sc, export_spec) == 3) && is_named_symbol (s7_car (export_spec), "rename")) {
    string renamed_export;
    if (library_name_part_to_string (s7_caddr (export_spec), renamed_export)) {
      return function_name == renamed_export;
    }
  }

  return false;
}

static bool
define_library_form_exports_function (s7_scheme* sc, s7_pointer form, const string& function_name, string& group, string& library) {
  if ((!s7_is_list (sc, form)) || s7_is_null (sc, form) || (!is_named_symbol (s7_car (form), "define-library"))) {
    return false;
  }

  string form_group;
  string form_library;
  if (!extract_library_name_from_form (sc, s7_cadr (form), form_group, form_library)) {
    return false;
  }

  for (s7_pointer declarations= s7_cddr (form); s7_is_pair (declarations); declarations= s7_cdr (declarations)) {
    s7_pointer declaration= s7_car (declarations);
    if ((!s7_is_list (sc, declaration)) || s7_is_null (sc, declaration) || (!is_named_symbol (s7_car (declaration), "export"))) {
      continue;
    }

    for (s7_pointer export_specs= s7_cdr (declaration); s7_is_pair (export_specs); export_specs= s7_cdr (export_specs)) {
      if (export_spec_name_matches (sc, s7_car (export_specs), function_name)) {
        group  = form_group;
        library= form_library;
        return true;
      }
    }
  }

  return false;
}

static bool
source_file_exports_function (s7_scheme* sc, const fs::path& source_file, const string& function_name, string& group, string& library) {
  string     source_text= read_text_file_exact (source_file);
  s7_pointer port       = s7_open_input_string (sc, source_text.c_str ());
  s7_pointer eof_object = s7_eof_object (sc);

  while (true) {
    s7_pointer form= s7_read (sc, port);
    if (form == eof_object) break;
    if (define_library_form_exports_function (sc, form, function_name, group, library)) {
      s7_close_input_port (sc, port);
      return true;
    }
  }

  s7_close_input_port (sc, port);
  return false;
}

static s7_pointer
make_library_name_list (s7_scheme* sc, const string& group, const string& library) {
  return s7_list (sc, 2, make_library_name_part (sc, group), make_library_name_part (sc, library));
}

static vector<fs::path>
sorted_child_directories (const fs::path& root) {
  vector<fs::path> directories;
  std::error_code  ec;

  for (fs::directory_iterator it (root, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment (ec)) {
    if (ec) {
      ec.clear ();
      continue;
    }
    if (it->is_directory (ec)) {
      directories.push_back (it->path ());
    }
    ec.clear ();
  }

  std::sort (directories.begin (), directories.end (),
             [] (const fs::path& lhs, const fs::path& rhs) { return lhs.string () < rhs.string (); });
  return directories;
}

static vector<fs::path>
sorted_scheme_source_files (const fs::path& dir) {
  vector<fs::path> files;
  std::error_code  ec;

  for (fs::directory_iterator it (dir, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment (ec)) {
    if (ec) {
      ec.clear ();
      continue;
    }
    if (it->is_regular_file (ec) && (it->path ().extension () == ".scm")) {
      files.push_back (it->path ());
    }
    ec.clear ();
  }

  std::sort (files.begin (), files.end (), [] (const fs::path& lhs, const fs::path& rhs) { return lhs.string () < rhs.string (); });
  return files;
}

static vector<string>
find_function_libraries_in_load_path (s7_scheme* sc, const string& function_name) {
  vector<string> library_queries;
  std::error_code ec;

  for (const auto& load_root_string : current_load_path_entries (sc)) {
    fs::path load_root (load_root_string);
    if ((!fs::exists (load_root, ec)) || (!fs::is_directory (load_root, ec))) {
      ec.clear ();
      continue;
    }
    ec.clear ();

    for (const auto& group_dir : sorted_child_directories (load_root)) {
      for (const auto& source_file : sorted_scheme_source_files (group_dir)) {
        string group;
        string library;
        if (source_file_exports_function (sc, source_file, function_name, group, library)) {
          append_unique_exact_string (library_queries, group + "/" + library);
        }
      }
    }
  }

  std::sort (library_queries.begin (), library_queries.end ());
  return library_queries;
}

static s7_pointer
make_library_name_list_list (s7_scheme* sc, const vector<string>& library_queries) {
  s7_pointer result= s7_nil (sc);
  for (auto it= library_queries.rbegin (); it != library_queries.rend (); ++it) {
    string group;
    string library;
    if (!split_library_query (*it, group, library)) continue;
    result= s7_cons (sc, make_library_name_list (sc, group, library), result);
  }
  return result;
}

static s7_pointer
f_function_libraries (s7_scheme* sc, s7_pointer args) {
  s7_pointer function_name_arg= s7_car (args);
  if (!s7_is_string (function_name_arg)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 2, s7_make_string (sc, "g_function-libraries: function-name must be string?"),
                              function_name_arg));
  }

  string         function_name= s7_string (function_name_arg);
  vector<string> visible_library_queries;

  try {
    visible_library_queries= find_function_libraries_in_load_path (sc, function_name);
  }
  catch (const std::exception& ex) {
    return s7_error (sc, s7_make_symbol (sc, "read-error"),
                     s7_list (sc, 2,
                              s7_make_string (sc, (string ("g_function-libraries: failed to inspect libraries: ") + ex.what ()).c_str ()),
                              function_name_arg));
  }

  return make_library_name_list_list (sc, visible_library_queries);
}

static StartupCliOptions
parse_startup_cli_options (int argc, char** argv) {
  StartupCliOptions opts;

  for (int i= 1; i < argc; ++i) {
    string arg= argv[i];

    if (arg == "--mode" || arg == "-m") {
      if ((i + 1) >= argc) {
        opts.error= "Error: '--mode' requires a MODE argument.";
        return opts;
      }
      opts.mode= argv[++i];
      continue;
    }
    if (arg.rfind ("--mode=", 0) == 0) {
      opts.mode= arg.substr (7);
      continue;
    }
    if (arg.rfind ("-m=", 0) == 0) {
      opts.mode= arg.substr (3);
      continue;
    }

    if (arg == "-I" || arg == "-A") {
      if ((i + 1) >= argc) {
        opts.error= "Error: '" + arg + "' requires a DIRECTORY argument.";
        return opts;
      }
      string dir= argv[++i];
      if (!load_path_directory_exists (dir)) {
        opts.error= "Error: directory does not exist: " + dir;
        return opts;
      }
      if (arg == "-I") {
        append_unique_string (opts.prepend_dirs, dir);
      }
      else {
        append_unique_string (opts.append_dirs, dir);
      }
      continue;
    }

    if (is_legacy_cli_command (arg) || arg.empty () || arg[0] != '-') {
      opts.command      = arg;
      opts.command_index= i;
      break;
    }

    opts.error= "Invalid option: " + arg;
    return opts;
  }

  return opts;
}

static void
apply_startup_load_path_options (s7_scheme* sc, const StartupCliOptions& opts) {
  vector<string> prepend_dirs= opts.prepend_dirs;
  for (const auto& dir : discover_auto_goldfish_library_dirs ()) {
    append_unique_string (prepend_dirs, dir);
  }
  prepend_load_path_entries (sc, prepend_dirs);
  append_load_path_entries (sc, opts.append_dirs);
}

int
repl_for_community_edition (s7_scheme* sc, int argc, char** argv) {
  string      gf_lib_dir  = find_goldfish_library ();
  const char* gf_lib      = gf_lib_dir.c_str ();
  string      gf_boot_path= find_goldfish_boot (gf_lib);
  const char* gf_boot     = gf_boot_path.c_str ();

  // 供 goldfish `g_command-line` procedure 查询
  command_args.assign (argv, argv + argc);

  StartupCliOptions startup_opts= parse_startup_cli_options (argc, argv);
  if (!startup_opts.error.empty ()) {
    std::cerr << startup_opts.error << "\n\n";
    display_help ();
    exit (1);
  }

  string command      = startup_opts.command;
  int    command_index= startup_opts.command_index;

  // 如果没有找到命令或没有参数，显示帮助
  if (argc <= 1 || command.empty ()) {
    display_help ();
    return 0;
  }

  // 根据命令类型确定默认模式：
  // - repl/load 命令默认使用 liii 模式
  // - 其他命令（eval, run, 直接执行脚本）默认使用 r7rs 模式
  string default_mode= "r7rs";
  if (command == "repl" || command == "load") {
    default_mode= "liii";
  }
  string mode= parse_mode_option (argc, argv, default_mode);

  // 处理旧版的 --help, -h, --version, -v（为了向后兼容）
  if (command == "--help" || command == "-h") {
    display_help ();
    return 0;
  }
  if (command == "--version" || command == "-v") {
    display_version ();
    return 0;
  }

  // 解析 fix 子命令（它有自己的参数解析）
  GoldfixCliOptions goldfix_opts= parse_goldfix_cli_options (argc, argv);
  if (!goldfix_opts.error.empty ()) {
    cerr << goldfix_opts.error << endl;
    exit (1);
  }

  apply_startup_load_path_options (sc, startup_opts);
  customize_goldfish_by_mode (sc, mode, gf_boot);

  // start capture error output
  const char* errmsg  = NULL;
  s7_pointer  old_port= s7_set_current_error_port (sc, s7_open_output_string (sc));
  int         gc_loc  = -1;
  if (old_port != s7_nil (sc)) gc_loc= s7_gc_protect (sc, old_port);

  // 处理 fix 子命令
  if (goldfix_opts.enabled) {
    int fix_ret= goldfish_run_fix_mode (sc, gf_lib, goldfix_opts);
    errmsg     = s7_get_output_string (sc, s7_current_error_port (sc));
    goldfish_print_scheme_error_message (sc, errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);

    if ((errmsg) && (*errmsg) && (fix_ret == 0)) return -1;
    return fix_ret;
  }

  // 处理 help 子命令
  if (command == "help") {
    display_help ();
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    return 0;
  }

  // 处理 version 子命令
  if (command == "version") {
    display_version ();
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    return 0;
  }

  // 处理 eval 子命令
  if (command == "eval") {
    if (argc < command_index + 1) {
      std::cerr << "Error: 'eval' requires CODE argument.\n" << std::endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    // 查找 CODE 参数（跳过 mode 选项，从命令位置之后开始）
    string code;
    for (int i= command_index + 1; i < argc; ++i) {
      string arg= argv[i];
      if (arg == "--mode" || arg == "-m") {
        i++; // skip mode value
        continue;
      }
      if (arg.rfind ("--mode=", 0) == 0 || arg.rfind ("-m=", 0) == 0) {
        continue;
      }
      code= arg;
      break;
    }
    if (code.empty ()) {
      std::cerr << "Error: 'eval' requires CODE argument.\n" << std::endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    goldfish_eval_code (sc, code);
    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    goldfish_print_scheme_error_message (sc, errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    if ((errmsg) && (*errmsg)) return -1;
    return 0;
  }

  // 处理 load 子命令（加载文件后进入 REPL）
  if (command == "load") {
    if (argc < command_index + 1) {
      std::cerr << "Error: 'load' requires FILE argument.\n" << std::endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    // 查找 FILE 参数（跳过 mode 选项，从命令位置之后开始）
    string file;
    for (int i= command_index + 1; i < argc; ++i) {
      string arg= argv[i];
      if (arg == "--mode" || arg == "-m") {
        i++; // skip mode value
        continue;
      }
      if (arg.rfind ("--mode=", 0) == 0 || arg.rfind ("-m=", 0) == 0) {
        continue;
      }
      file= arg;
      break;
    }
    if (file.empty ()) {
      std::cerr << "Error: 'load' requires FILE argument.\n" << std::endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    // 加载文件
    goldfish_eval_file (sc, file, true);
    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) {
      goldfish_print_scheme_error_message (sc, errmsg);
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      return -1;
    }
    // 加载成功后进入 REPL
#ifdef GOLDFISH_WITH_REPL
    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) {
      string rendered;
      goldfish_render_scheme_error_message (sc, errmsg, rendered);
      ic_printf ("[red]%s[/]", rendered.c_str ());
    }
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);

    goldfish_repl (sc, mode);
    return 0;
#else
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    std::cerr << "Interactive REPL is not available in this build.\n" << std::endl;
    exit (-1);
#endif
  }

  // 处理 repl 子命令
  if (command == "repl") {
#ifdef GOLDFISH_WITH_REPL
    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) {
      string rendered;
      goldfish_render_scheme_error_message (sc, errmsg, rendered);
      ic_printf ("[red]%s[/]", rendered.c_str ());
    }
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);

    goldfish_repl (sc, mode);
    return 0;
#else
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    std::cerr << "Interactive REPL is not available in this build.\n" << std::endl;
    exit (-1);
#endif
  }

  // 处理 test 子命令
  if (command == "test") {
    // 添加 tools/goldtest 目录到 load path (用于加载 (liii goldtest) 模块)
    string goldtest_root = find_goldtest_tool_root (gf_lib);
    if (goldtest_root.empty ()) {
      cerr << "Error: tools/goldtest directory not found." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    s7_add_to_load_path (sc, goldtest_root.c_str ());

    // Import (liii goldtest) module
    s7_pointer import_result = s7_eval_c_string (sc, "(import (liii goldtest))");
    if (!import_result) {
      cerr << "Error: Failed to import (liii goldtest) module." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    errmsg = s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) {
      goldfish_print_prefixed_scheme_error_message (sc, "Error importing (liii goldtest):", errmsg);
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }

    // Get and call the main function from (liii goldtest)
    s7_pointer main_func = s7_name_to_value (sc, "main");
    if ((!main_func) || (!s7_is_procedure (main_func))) {
      cerr << "Error: Failed to find main function in (liii goldtest)." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    s7_pointer result = s7_call (sc, main_func, s7_nil (sc));
    errmsg = s7_get_output_string (sc, s7_current_error_port (sc));
    goldfish_print_scheme_error_message (sc, errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    if (s7_is_integer (result)) {
      return static_cast<int> (s7_integer (result));
    }
    return 0;
  }

  // 处理 doc 子命令
  if (command == "doc") {
    string golddoc_root = find_golddoc_tool_root (gf_lib);
    if (golddoc_root.empty ()) {
      cerr << "Error: tools/golddoc directory not found." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    s7_add_to_load_path (sc, golddoc_root.c_str ());

    s7_pointer import_result = s7_eval_c_string (sc, "(import (liii golddoc))");
    if (!import_result) {
      cerr << "Error: Failed to import (liii golddoc) module." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    errmsg = s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) {
      goldfish_print_prefixed_scheme_error_message (sc, "Error importing (liii golddoc):", errmsg);
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }

    s7_pointer main_func = s7_name_to_value (sc, "main");
    if ((!main_func) || (!s7_is_procedure (main_func))) {
      cerr << "Error: Failed to find main function in (liii golddoc)." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    s7_pointer result = s7_call (sc, main_func, s7_nil (sc));
    errmsg = s7_get_output_string (sc, s7_current_error_port (sc));
    goldfish_print_scheme_error_message (sc, errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    if (s7_is_integer (result)) {
      return static_cast<int> (s7_integer (result));
    }
    return 0;
  }

  // 处理 source 子命令
  if (command == "source") {
    string goldsource_root = find_goldsource_tool_root (gf_lib);
    if (goldsource_root.empty ()) {
      cerr << "Error: tools/goldsource directory not found." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    s7_add_to_load_path (sc, goldsource_root.c_str ());

    s7_pointer import_result = s7_eval_c_string (sc, "(import (liii goldsource))");
    if (!import_result) {
      cerr << "Error: Failed to import (liii goldsource) module." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    errmsg = s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) {
      goldfish_print_prefixed_scheme_error_message (sc, "Error importing (liii goldsource):", errmsg);
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }

    s7_pointer main_func = s7_name_to_value (sc, "main");
    if ((!main_func) || (!s7_is_procedure (main_func))) {
      cerr << "Error: Failed to find main function in (liii goldsource)." << endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }
    s7_pointer result = s7_call (sc, main_func, s7_nil (sc));
    errmsg = s7_get_output_string (sc, s7_current_error_port (sc));
    goldfish_print_scheme_error_message (sc, errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    if (s7_is_integer (result)) {
      return static_cast<int> (s7_integer (result));
    }
    return 0;
  }

  // 处理 run 子命令
  if (command == "run") {
    // 获取 TARGET 参数
    string target;
    for (int i= command_index + 1; i < argc; ++i) {
      string arg= argv[i];
      if (arg == "--mode" || arg == "-m") {
        i++; // skip mode value
        continue;
      }
      if (arg.rfind ("--mode=", 0) == 0 || arg.rfind ("-m=", 0) == 0) {
        continue;
      }
      target= arg;
      break;
    }
    if (target.empty ()) {
      std::cerr << "Error: 'run' requires TARGET argument.\n" << std::endl;
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      exit (1);
    }

    // 判断类型并处理
    if (target.find ('/') != string::npos || target.rfind (".scm") == target.length () - 4) {
      // 包含 / 或以 .scm 结尾，按文件路径处理
      // 检查文件是否存在
      std::error_code ec;
      if (!fs::exists (target, ec) || !fs::is_regular_file (target, ec)) {
        s7_close_output_port (sc, s7_current_error_port (sc));
        s7_set_current_error_port (sc, old_port);
        if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
        std::cerr << "Error: File not found: " << target << std::endl;
        return 1;
      }
      goldfish_eval_file (sc, target, true);
    }
    else {
      // 按模块名处理，例如: liii.string -> (liii string)
      string import_expr= "(import (" + target + "))";
      // 将 . 替换为空格
      for (size_t i= 0; i < import_expr.length (); ++i) {
        if (import_expr[i] == '.') import_expr[i]= ' ';
      }
      s7_eval_c_string (sc, import_expr.c_str ());
    }

    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) {
      goldfish_print_scheme_error_message (sc, errmsg);
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      return 1;
    }

    // 检查并调用 main 函数
    s7_pointer main_func= s7_name_to_value (sc, "main");
    if ((!main_func) || (!s7_is_procedure (main_func))) {
      s7_close_output_port (sc, s7_current_error_port (sc));
      s7_set_current_error_port (sc, old_port);
      if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
      std::cerr << "Error: No main function found in target: " << target << std::endl;
      return 1;
    }

    // 调用 main 函数
    s7_call (sc, main_func, s7_nil (sc));

    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    goldfish_print_scheme_error_message (sc, errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    return 0;
  }

  // 处理直接执行文件（以 .scm 结尾或存在的文件）
  // 检查是否是文件
  std::error_code ec;
  if (fs::exists (command, ec) && fs::is_regular_file (command, ec)) {
    goldfish_eval_file (sc, command, true);
    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    goldfish_print_scheme_error_message (sc, errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    if ((errmsg) && (*errmsg)) return -1;
    return 0;
  }

  // 未知命令
  std::cerr << "Unknown command: " << command << "\n\n";
  display_help ();
  s7_close_output_port (sc, s7_current_error_port (sc));
  s7_set_current_error_port (sc, old_port);
  if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
  return 1;
}

} // namespace goldfish
