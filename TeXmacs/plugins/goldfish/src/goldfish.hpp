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
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include "s7.h"
#include <string>
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

#define GOLDFISH_VERSION "17.11.28"

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

using nlohmann::json;

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional);

static const char* NJSON_HANDLE_TAG = "njson-handle";
static std::vector<std::unique_ptr<json>> njson_handle_store = std::vector<std::unique_ptr<json>> (1);
static std::vector<s7_int> njson_handle_free_ids;
static std::vector<s7_pointer> njson_keys_cache_values = std::vector<s7_pointer> (1, nullptr);
static std::vector<s7_int> njson_keys_cache_gc_locs = std::vector<s7_int> (1, -1);
static std::vector<bool> njson_keys_cache_valid = std::vector<bool> (1, false);

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
make_njson_handle (s7_scheme* sc, s7_int id) {
  return s7_cons (sc, s7_make_symbol (sc, NJSON_HANDLE_TAG), s7_make_integer (sc, id));
}

static bool
is_njson_handle (s7_pointer x) {
  if (!s7_is_pair (x)) return false;
  s7_pointer tag = s7_car (x);
  s7_pointer id  = s7_cdr (x);
  if (!s7_is_symbol (tag)) return false;
  if (strcmp (s7_symbol_name (tag), NJSON_HANDLE_TAG) != 0) return false;
  return s7_is_integer (id);
}

static bool
extract_njson_handle_id (s7_scheme* sc, s7_pointer handle, s7_int& id, std::string& error_msg) {
  if (!is_njson_handle (handle)) {
    error_msg = "expected njson handle";
    return false;
  }
  id = s7_integer (s7_cdr (handle));
  if (id <= 0) {
    error_msg = "invalid njson handle id";
    return false;
  }
  if (static_cast<size_t> (id) >= njson_handle_store.size () || !njson_handle_store[static_cast<size_t> (id)]) {
    error_msg = "njson handle does not exist (may have been freed)";
    return false;
  }
  return true;
}

static json*
njson_value_by_id (s7_int id) {
  if (id <= 0) return nullptr;
  size_t index = static_cast<size_t> (id);
  if (index >= njson_handle_store.size ()) return nullptr;
  return njson_handle_store[index].get ();
}

static const json*
njson_value_by_id_const (s7_int id) {
  if (id <= 0) return nullptr;
  size_t index = static_cast<size_t> (id);
  if (index >= njson_handle_store.size ()) return nullptr;
  return njson_handle_store[index].get ();
}

static void
njson_ensure_keys_cache_size (size_t n) {
  if (njson_keys_cache_values.size () < n) {
    njson_keys_cache_values.resize (n, nullptr);
    njson_keys_cache_gc_locs.resize (n, -1);
    njson_keys_cache_valid.resize (n, false);
  }
}

static void
njson_clear_keys_cache_slot (s7_scheme* sc, s7_int id) {
  if (id <= 0) return;
  size_t index = static_cast<size_t> (id);
  if (index >= njson_keys_cache_valid.size ()) return;

  if (njson_keys_cache_valid[index] && njson_keys_cache_gc_locs[index] != -1) {
    s7_gc_unprotect_at (sc, njson_keys_cache_gc_locs[index]);
  }
  njson_keys_cache_values[index] = nullptr;
  njson_keys_cache_gc_locs[index] = -1;
  njson_keys_cache_valid[index] = false;
}

static s7_pointer
njson_build_keys_list (s7_scheme* sc, const json& root) {
  s7_pointer out = s7_nil (sc);
  if (!root.is_object ()) {
    return out;
  }

  for (auto it = root.end (); it != root.begin ();) {
    --it;
    const std::string& key = it.key ();
    out = s7_cons (sc, s7_make_string_with_length (sc, key.data (), static_cast<s7_int> (key.size ())), out);
  }
  return out;
}

static void
njson_store_keys_cache (s7_scheme* sc, s7_int id, s7_pointer keys_list) {
  if (id <= 0) return;
  size_t index = static_cast<size_t> (id);
  njson_ensure_keys_cache_size (index + 1);
  njson_clear_keys_cache_slot (sc, id);
  njson_keys_cache_values[index] = keys_list;
  njson_keys_cache_gc_locs[index] = s7_gc_protect (sc, keys_list);
  njson_keys_cache_valid[index] = true;
}

static bool
njson_try_get_keys_cache (s7_int id, s7_pointer& out) {
  if (id <= 0) return false;
  size_t index = static_cast<size_t> (id);
  if (index >= njson_keys_cache_valid.size ()) return false;
  if (!njson_keys_cache_valid[index]) return false;
  out = njson_keys_cache_values[index];
  return true;
}

static void
njson_invalidate_keys_cache_if_present (s7_scheme* sc, s7_int id) {
  if (id <= 0) return;
  size_t index = static_cast<size_t> (id);
  if (index >= njson_keys_cache_valid.size () || !njson_keys_cache_valid[index]) return;
  njson_clear_keys_cache_slot (sc, id);
}

static s7_int
store_njson_value (json&& value) {
  if (!njson_handle_free_ids.empty ()) {
    s7_int id = njson_handle_free_ids.back ();
    njson_handle_free_ids.pop_back ();
    njson_handle_store[static_cast<size_t> (id)] = std::make_unique<json> (std::move (value));
    njson_ensure_keys_cache_size (njson_handle_store.size ());
    return id;
  }

  njson_handle_store.push_back (std::make_unique<json> (std::move (value)));
  njson_ensure_keys_cache_size (njson_handle_store.size ());
  s7_int id = static_cast<s7_int> (njson_handle_store.size () - 1);
  return id;
}

static s7_int
store_njson_value (const json& value) {
  json copied = value;
  return store_njson_value (std::move (copied));
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

static bool
lookup_path_const (s7_scheme* sc, const json& root, const std::vector<s7_pointer>& path, const json*& out,
                   bool& found, std::string& error_msg) {
  const json* cur = &root;
  for (size_t i = 0; i < path.size (); i++) {
    s7_pointer key = path[i];
    if (cur->is_object ()) {
      std::string name;
      if (!scheme_json_key_to_string (sc, key, name, error_msg)) {
        return false;
      }
      auto it = cur->find (name);
      if (it == cur->end ()) {
        found = false;
        return true;
      }
      cur = &(*it);
    }
    else if (cur->is_array ()) {
      size_t idx = 0;
      if (!scheme_json_index (key, idx, error_msg)) {
        return false;
      }
      if (idx >= cur->size ()) {
        found = false;
        return true;
      }
      cur = &(*cur)[idx];
    }
    else {
      found = false;
      return true;
    }
  }
  out = cur;
  found = true;
  return true;
}

static bool
lookup_path_parent_mutable (s7_scheme* sc, json& root, const std::vector<s7_pointer>& path, json*& parent,
                            s7_pointer& last_key, bool& found, std::string& error_msg) {
  if (path.empty ()) {
    error_msg = "path cannot be empty";
    return false;
  }

  json* cur = &root;
  for (size_t i = 0; i + 1 < path.size (); i++) {
    s7_pointer key = path[i];
    if (cur->is_object ()) {
      std::string name;
      if (!scheme_json_key_to_string (sc, key, name, error_msg)) {
        return false;
      }
      auto it = cur->find (name);
      if (it == cur->end ()) {
        found = false;
        return true;
      }
      cur = &(*it);
    }
    else if (cur->is_array ()) {
      size_t idx = 0;
      if (!scheme_json_index (key, idx, error_msg)) {
        return false;
      }
      if (idx >= cur->size ()) {
        found = false;
        return true;
      }
      cur = &(*cur)[idx];
    }
    else {
      found = false;
      return true;
    }
  }

  parent = cur;
  last_key = path.back ();
  found = true;
  return true;
}

static bool
scheme_to_njson_scalar_or_handle (s7_scheme* sc, s7_pointer value, json& out, std::string& error_msg) {
  if (is_njson_handle (value)) {
    s7_int id = 0;
    if (!extract_njson_handle_id (sc, value, id, error_msg)) {
      return false;
    }
    const json* source = njson_value_by_id_const (id);
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
  if (s7_is_real (value) || s7_is_number (value)) {
    out = s7_number_to_real (sc, value);
    return true;
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
njson_value_to_scheme_or_handle (s7_scheme* sc, const json& value) {
  if (value.is_object () || value.is_array ()) {
    s7_int id = store_njson_value (value);
    return make_njson_handle (sc, id);
  }
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

static s7_pointer
f_njson_string_to_json (s7_scheme* sc, s7_pointer args) {
  s7_pointer input = s7_car (args);
  if (!s7_is_string (input)) {
    return njson_error (sc, "type-error", "g_njson-string->json: input must be string", input);
  }

  try {
    json parsed = json::parse (s7_string (input));
    return make_njson_handle (sc, store_njson_value (std::move (parsed)));
  }
  catch (const json::parse_error& err) {
    return njson_error (sc, "parse-error", err.what (), input);
  }
}

static s7_pointer
f_njson_json_to_string (s7_scheme* sc, s7_pointer args) {
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
f_njson_handle_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer input = s7_car (args);
  return s7_make_boolean (sc, is_njson_handle (input));
}

template <typename HandlePredicate, typename ScalarPredicate>
static s7_pointer
njson_run_value_type_predicate (s7_scheme* sc, s7_pointer args, const char* api_name, HandlePredicate handle_pred,
                                ScalarPredicate scalar_pred) {
  s7_pointer input = s7_car (args);
  if (!is_njson_handle (input)) {
    return s7_make_boolean (sc, scalar_pred (input));
  }

  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, input, id, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, input);
  }
  const json* value = njson_value_by_id_const (id);
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
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-free: " + error_msg, handle);
  }
  njson_clear_keys_cache_slot (sc, id);
  njson_handle_store[static_cast<size_t> (id)].reset ();
  njson_handle_free_ids.push_back (id);
  return s7_t (sc);
}

static s7_pointer
f_njson_size (s7_scheme* sc, s7_pointer args) {
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-size: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (id);
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
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-empty?: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (id);
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
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-ref: " + error_msg, handle);
  }

  std::vector<s7_pointer> path;
  if (!collect_path_keys (sc, s7_cdr (args), path, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-ref: " + error_msg, handle);
  }
  if (path.empty ()) {
    return njson_error (sc, "value-error", "g_njson-ref: missing key arguments", handle);
  }

  const json* root = njson_value_by_id_const (id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-ref: njson handle does not exist (may have been freed)", handle);
  }

  const json* found_value = nullptr;
  bool found = false;
  if (!lookup_path_const (sc, *root, path, found_value, found, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-ref: " + error_msg, handle);
  }
  if (!found) {
    return s7_nil (sc);
  }
  return njson_value_to_scheme_or_handle (sc, *found_value);
}

enum class njson_update_op {
  set,
  push,
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
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, handle);
  }

  if (njson_update_needs_value (op)) {
    if (tokens.size () < 2) {
      return njson_error (sc, "value-error", std::string (api_name) + ": " + njson_update_expected_argv (op), handle);
    }
    path.assign (tokens.begin (), tokens.end () - 1);
    s7_pointer value_token = tokens.back ();
    if (!scheme_to_njson_scalar_or_handle (sc, value_token, value_json, error_msg)) {
      return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, value_token);
    }
  }
  else {
    if (tokens.empty ()) {
      return njson_error (sc, "value-error", std::string (api_name) + ": " + njson_update_expected_argv (op), handle);
    }
    path = std::move (tokens);
  }
  return nullptr;
}

static s7_pointer
njson_apply_update_on_root (s7_scheme* sc, json& root, const std::vector<s7_pointer>& path, const json& value_json,
                            njson_update_op op, const char* api_name, s7_pointer handle) {
  std::string error_msg;
  json* parent = nullptr;
  s7_pointer last_key = s7_nil (sc);
  bool found = false;
  if (!lookup_path_parent_mutable (sc, root, path, parent, last_key, found, error_msg)) {
    return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, handle);
  }
  if (!found) {
    return nullptr;
  }

  if (parent->is_object ()) {
    std::string key_name;
    if (!scheme_json_key_to_string (sc, last_key, key_name, error_msg)) {
      return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, last_key);
    }

    if (op == njson_update_op::set) {
      auto it = parent->find (key_name);
      if (it != parent->end ()) {
        (*parent)[key_name] = value_json;
      }
    }
    else if (op == njson_update_op::push) {
      (*parent)[key_name] = value_json;
    }
    else {
      parent->erase (key_name);
    }
    return nullptr;
  }

  if (parent->is_array ()) {
    size_t idx = 0;
    if (!scheme_json_index (last_key, idx, error_msg)) {
      return njson_error (sc, "type-error", std::string (api_name) + ": " + error_msg, last_key);
    }

    if (op == njson_update_op::set) {
      if (idx < parent->size ()) {
        (*parent)[idx] = value_json;
      }
    }
    else if (op == njson_update_op::push) {
      if (idx <= parent->size ()) {
        parent->insert (parent->begin () + static_cast<json::difference_type> (idx), value_json);
      }
      else {
        parent->push_back (value_json);
      }
    }
    else {
      if (idx < parent->size ()) {
        parent->erase (parent->begin () + static_cast<json::difference_type> (idx));
      }
    }
  }
  return nullptr;
}

static s7_pointer
njson_run_update (s7_scheme* sc, s7_pointer args, const char* api_name, njson_update_op op, bool in_place) {
  s7_pointer handle = s7_nil (sc);
  s7_int id = 0;
  std::vector<s7_pointer> path;
  json value_json;
  s7_pointer err = njson_parse_update_request (sc, args, api_name, op, handle, id, path, value_json);
  if (err) {
    return err;
  }

  if (in_place) {
    json* root = njson_value_by_id (id);
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

  const json* root = njson_value_by_id_const (id);
  if (!root) {
    return njson_error (sc, "type-error",
                        std::string (api_name) + ": njson handle does not exist (may have been freed)", handle);
  }
  json out = *root;
  err = njson_apply_update_on_root (sc, out, path, value_json, op, api_name, handle);
  if (err) {
    return err;
  }
  return make_njson_handle (sc, store_njson_value (std::move (out)));
}

static s7_pointer
f_njson_set (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-set", njson_update_op::set, false);
}

static s7_pointer
f_njson_push (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-push", njson_update_op::push, false);
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
f_njson_push_x (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-push!", njson_update_op::push, true);
}

static s7_pointer
f_njson_drop_x (s7_scheme* sc, s7_pointer args) {
  return njson_run_update (sc, args, "g_njson-drop!", njson_update_op::drop, true);
}

static s7_pointer
f_njson_contains_key_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer  handle = s7_car (args);
  s7_pointer  key = s7_cadr (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-contains-key?: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (id);
  if (!root) {
    return njson_error (sc, "type-error",
                        "g_njson-contains-key?: njson handle does not exist (may have been freed)", handle);
  }
  if (!root->is_object ()) {
    return s7_f (sc);
  }

  std::string key_name;
  if (!scheme_json_key_to_string (sc, key, key_name, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-contains-key?: " + error_msg, key);
  }
  return s7_make_boolean (sc, root->contains (key_name));
}

static s7_pointer
f_njson_keys (s7_scheme* sc, s7_pointer args) {
  s7_pointer  handle = s7_car (args);
  s7_int      id = 0;
  std::string error_msg;
  if (!extract_njson_handle_id (sc, handle, id, error_msg)) {
    return njson_error (sc, "type-error", "g_njson-keys: " + error_msg, handle);
  }

  const json* root = njson_value_by_id_const (id);
  if (!root) {
    return njson_error (sc, "type-error", "g_njson-keys: njson handle does not exist (may have been freed)", handle);
  }
  if (!root->is_object ()) {
    return s7_nil (sc);
  }

  s7_pointer cached = nullptr;
  if (njson_try_get_keys_cache (id, cached)) {
    return cached;
  }

  s7_pointer out = njson_build_keys_list (sc, *root);
  njson_store_keys_cache (sc, id, out);
  return out;
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
  const char* parse_name = "g_njson-string->json";
  const char* parse_desc = "(g_njson-string->json json-string) => njson-handle";
  const char* dump_name  = "g_njson-json->string";
  const char* dump_desc  = "(g_njson-json->string handle-or-scalar) => strict-json-string";
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
  const char* set_x_name = "g_njson-set!";
  const char* set_x_desc = "(g_njson-set! handle key ... value) => same-handle";
  const char* push_name = "g_njson-push";
  const char* push_desc = "(g_njson-push handle key ... value) => new-handle";
  const char* push_x_name = "g_njson-push!";
  const char* push_x_desc = "(g_njson-push! handle key ... value) => same-handle";
  const char* drop_name = "g_njson-drop";
  const char* drop_desc = "(g_njson-drop handle key ...) => new-handle";
  const char* drop_x_name = "g_njson-drop!";
  const char* drop_x_desc = "(g_njson-drop! handle key ...) => same-handle";
  const char* has_key_name = "g_njson-contains-key?";
  const char* has_key_desc = "(g_njson-contains-key? handle key) => boolean?";
  const char* keys_name = "g_njson-keys";
  const char* keys_desc = "(g_njson-keys handle) => (list-of string?)";
  const char* schema_report_name = "g_njson-schema-report";
  const char* schema_report_desc = "(g_njson-schema-report schema-handle instance) => hash-table";
  glue_define (sc, parse_name, parse_desc, f_njson_string_to_json, 1, 0);
  glue_define (sc, dump_name, dump_desc, f_njson_json_to_string, 1, 0);
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
  glue_define (sc, set_x_name, set_x_desc, f_njson_set_x, 3, 32);
  glue_define (sc, push_name, push_desc, f_njson_push, 3, 32);
  glue_define (sc, push_x_name, push_x_desc, f_njson_push_x, 3, 32);
  glue_define (sc, drop_name, drop_desc, f_njson_drop, 2, 32);
  glue_define (sc, drop_x_name, drop_x_desc, f_njson_drop_x, 2, 32);
  glue_define (sc, has_key_name, has_key_desc, f_njson_contains_key_p, 2, 0);
  glue_define (sc, keys_name, keys_desc, f_njson_keys, 1, 0);
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
  cpr::Parameters params= cpr::Parameters{};
  if (s7_is_list(sc, args)) {
    s7_pointer iter= args;
    while (!s7_is_null (sc, iter)) {
      s7_pointer pair= s7_car (iter);
      if (s7_is_pair (pair)) {
        const char* key= s7_string (s7_car (pair));
        const char* value= s7_string (s7_cdr (pair));
        params.Add (cpr::Parameter (string (key), string (value)));
      }
      iter= s7_cdr (iter);
    }
  }
  return params;
}

inline cpr::Header
to_cpr_headers (s7_scheme* sc, s7_pointer args) {
  cpr::Header headers= cpr::Header{};
  if (s7_is_list(sc, args)) {
    s7_pointer iter= args;
    while (!s7_is_null (sc, iter)) {
      s7_pointer pair= s7_car (iter);
      if (s7_is_pair (pair)) {
        const char* key= s7_string (s7_car (pair));
        const char* value= s7_string (s7_cdr (pair));
        headers.insert (std::make_pair (key, value));
      }
      iter= s7_cdr (iter);
    }
  }
  return headers;
}

inline cpr::Proxies
to_cpr_proxies (s7_scheme* sc, s7_pointer args) {
  std::map<std::string, std::string> proxy_map;
  if (s7_is_list(sc, args)) {
    s7_pointer iter= args;
    while (!s7_is_null (sc, iter)) {
      s7_pointer pair= s7_car (iter);
      if (s7_is_pair (pair)) {
        const char* key= s7_string (s7_car (pair));
        const char* value= s7_string (s7_cdr (pair));
        proxy_map[key] = value;
      }
      iter= s7_cdr (iter);
    }
  }
  return cpr::Proxies(proxy_map);
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
  cpr::Parameters cpr_params= to_cpr_parameters(sc, params);
  s7_pointer headers= s7_caddr (args);
  cpr::Header cpr_headers= to_cpr_headers (sc, headers);
  s7_pointer proxy= s7_cadddr (args);
  cpr::Proxies cpr_proxies= to_cpr_proxies(sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  session.SetHeader (cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session.SetProxies(cpr_proxies);
  }

  cpr::Response r= session.Get ();
  return response2hashtable (sc, r);
}

inline void
glue_http_get (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);
  const char* s_http_get= "g_http-get";
  const char* d_http_get= "(g_http-get url params headers proxy) => hash-table?";
  auto func_http_get= s7_make_typed_function (sc, s_http_get, f_http_get, 4, 0, false, d_http_get, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, s_http_get), func_http_get);
}

static s7_pointer
f_http_post (s7_scheme* sc, s7_pointer args) {
  const char* url= s7_string (s7_car (args));
  s7_pointer params= s7_cadr (args);
  cpr::Parameters cpr_params= to_cpr_parameters(sc, params);
  const char* body= s7_string (s7_caddr (args));
  cpr::Body cpr_body= cpr::Body (body);
  s7_pointer headers= s7_cadddr (args);
  cpr::Header cpr_headers= to_cpr_headers (sc, headers);
  s7_pointer proxy= s7_car (s7_cddddr (args));
  cpr::Proxies cpr_proxies= to_cpr_proxies (sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  session.SetBody (cpr_body);
  session.SetHeader (cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session.SetProxies(cpr_proxies);
  }

  cpr::Response r= session.Post ();
  return response2hashtable (sc, r);
}

inline void
glue_http_post (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);
  const char* name= "g_http-post";
  const char* doc= "(g_http-post url params body headers proxy) => hash-table?";
  auto func_http_post= s7_make_typed_function (sc, name, f_http_post, 5, 0, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func_http_post);
}

static s7_pointer
f_http_stream_get (s7_scheme* sc, s7_pointer args) {
  const char* url = s7_string (s7_car (args));
  s7_pointer params = s7_cadr (args);
  s7_pointer proxy = s7_caddr (args);
  s7_pointer userdata = s7_cadddr (args);
  s7_pointer callback = s7_car(s7_cddddr(args));

  cpr::Parameters cpr_params = to_cpr_parameters(sc, params);
  cpr::Proxies cpr_proxies = to_cpr_proxies(sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session.SetProxies (cpr_proxies);
  }

  session.SetWriteCallback(cpr::WriteCallback{[sc, callback](const std::string_view& data, intptr_t cpr_userdata) -> bool {
    // Retrieve userdata from intptr_t
    s7_pointer userdata_ptr = (s7_pointer)cpr_userdata;

    // Call the scheme callback inline
    s7_pointer data_str = s7_make_string_with_length(sc, data.data(), data.length());
    s7_pointer args = s7_cons(sc, data_str, s7_cons(sc, userdata_ptr, s7_nil(sc)));

    s7_pointer ret = s7_call(sc, callback, args);
    if (s7_is_boolean(ret)) {
      return s7_boolean(sc, ret);
    }

    return true; // Continue receiving
  }, reinterpret_cast<intptr_t>(userdata)});

  try {
    cpr::Response response = session.Get();
  } catch (const std::exception& e) {
    return s7_make_integer(sc, 500); // Error case
  }
  return s7_undefined(sc);
}

inline void
glue_http_stream_get (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);
  const char* s_stream_get = "g_http-stream-get";
  const char* d_stream_get = "(g_http-stream-get url params proxy userdata callback) => undefined";
  auto func_stream_get = s7_make_typed_function (sc, s_stream_get, f_http_stream_get, 5, 0, false, d_stream_get, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, s_stream_get), func_stream_get);
}

static s7_pointer
f_http_stream_post (s7_scheme* sc, s7_pointer args) {
  s7_pointer url_arg = s7_car (args);
  s7_pointer params = s7_cadr (args);
  s7_pointer body_arg = s7_caddr (args);
  s7_pointer headers = s7_cadddr (args);
  s7_pointer proxy = s7_car(s7_cddddr(args));
  s7_pointer userdata = s7_cadr(s7_cddddr(args));
  s7_pointer callback = s7_list_ref(sc, args, 6);

  const char* url = s7_string(url_arg);
  const char* body = s7_string(body_arg);

  cpr::Parameters cpr_params = to_cpr_parameters(sc, params);
  cpr::Header cpr_headers = to_cpr_headers(sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies(sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  session.SetBody (cpr::Body (body));
  session.SetHeader (cpr_headers);
  if (s7_is_list(sc, proxy) && !s7_is_null(sc, proxy)) {
    session.SetProxies (cpr_proxies);
  }


  // Store userdata in s7 managed memory to prevent GC
  s7_pointer userdata_loc = s7_make_c_pointer(sc, (void*)userdata);

  session.SetWriteCallback(cpr::WriteCallback{[sc, callback](const std::string_view& data, intptr_t cpr_userdata) -> bool {
    // Retrieve userdata from intptr_t
    s7_pointer userdata_ptr = (s7_pointer)cpr_userdata;

    // Call the scheme callback inline
    s7_pointer data_str = s7_make_string_with_length(sc, data.data(), data.length());
    s7_pointer args = s7_cons(sc, data_str, s7_cons(sc, userdata_ptr, s7_nil(sc)));

    s7_pointer ret = s7_call(sc, callback, args);
    if (s7_is_boolean(ret)) {
      return s7_boolean(sc, ret);
    }

    return true; // Continue receiving
  }, reinterpret_cast<intptr_t>(userdata)});

  try {
    cpr::Response response = session.Post();
  } catch (const std::exception& e) {
    return s7_make_integer(sc, 500); // Error case
  }
  return s7_undefined(sc);
}

inline void
glue_http_stream_post (s7_scheme* sc) {
  s7_pointer cur_env= s7_curlet (sc);

  const char* s_stream_post = "g_http-stream-post";
  const char* d_stream_post = "(g_http-stream-post url params body headers proxy userdata callback) => undefined";
  auto func_stream_post = s7_make_typed_function (sc, s_stream_post, f_http_stream_post, 7, 0, false, d_stream_post, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, s_stream_post), func_stream_post);
}

inline void
glue_http (s7_scheme* sc) {
  glue_http_head (sc);
  glue_http_get (sc);
  glue_http_post (sc);
}

inline void
glue_http_stream (s7_scheme* sc) {
  glue_http_stream_get(sc);
  glue_http_stream_post(sc);
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
  
  cpr::Parameters cpr_params = to_cpr_parameters(sc, params);
  cpr::Header cpr_headers = to_cpr_headers(sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies(sc, proxy);
  
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
  
  cpr::Parameters cpr_params = to_cpr_parameters(sc, params);
  cpr::Header cpr_headers = to_cpr_headers(sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies(sc, proxy);
  
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
  
  cpr::Parameters cpr_params = to_cpr_parameters(sc, params);
  cpr::Header cpr_headers = to_cpr_headers(sc, headers);
  cpr::Proxies cpr_proxies = to_cpr_proxies(sc, proxy);
  
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

  const char* s_version    = "version";
  const char* d_version    = "(version) => string";
  const char* s_delete_file= "g_delete-file";
  const char* d_delete_file= "(g_delete-file string) => boolean";

  s7_define (sc, cur_env, s7_make_symbol (sc, s_version),
             s7_make_typed_function (sc, s_version, f_version, 0, 0, false, d_version, NULL));

  s7_define (sc, cur_env, s7_make_symbol (sc, s_delete_file),
             s7_make_typed_function (sc, s_delete_file, f_delete_file, 1, 0, false, d_delete_file, NULL));
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

  return s7_make_string (sc, content.c_str ());
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
  glue_http_stream (sc);
  glue_http_async (sc);
}

static void
display_help () {
  cout << "Goldfish Scheme " << GOLDFISH_VERSION << " by LiiiLabs" << endl;
  cout << "--help, -h    \tDisplay this help message" << endl;
  cout << "--version, -v \tDisplay version" << endl;
  cout << "--mode, -m    \tAllowed mode: default, liii, sicp, r7rs, s7" << endl;
  cout << "--eval, -e    \tLoad and evaluate Scheme code from the command line" << endl
       << "\t\t  e.g. -e '(begin (display `Hello) (+ 1 2))'" << endl;
  cout << "--load, -l FILE\tLoad Scheme code from FILE" << endl;
#ifdef GOLDFISH_WITH_REPL
  cout << "--repl, -i    \tEnter interactive REPL mode" << endl;
#else
  cout << "--repl, -i    \t*Interactive REPL is not available in this build*" << endl;
#endif
  cout << "FILE [FILE...]\tLoad and evaluate Scheme code from one or more files" << endl
       << "\t\t  (all non-option arguments are treated as files to load)" << endl;
  cout << endl << "If no FILE is specified, REPL is entered by default (if available)." << endl;
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

static void
goldfish_eval_code (s7_scheme* sc, string code) {
  s7_pointer x= s7_eval_c_string (sc, code.c_str ());
  cout << s7_object_to_c_string (sc, x) << endl;
}

s7_scheme*
init_goldfish_scheme (const char* gf_lib) {
  s7_scheme* sc= s7_init ();
  s7_add_to_load_path (sc, gf_lib);

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
    s7_eval_c_string (sc, "(import (liii base) (liii error) (liii oop))");
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
    ic_printf ("[error]%s[/]\n", errmsg); // 美化输出
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
             "[i]Based on S7 Scheme %s [dim](%s)[/][/]\n"
             "[b]Mode:[/] [b]%s[/]\n\n",
             GOLDFISH_VERSION, S7_VERSION, S7_DATE, mode.c_str ());
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

int
repl_for_community_edition (s7_scheme* sc, int argc, char** argv) {
  string      gf_lib_dir  = find_goldfish_library ();
  const char* gf_lib      = gf_lib_dir.c_str ();
  string      gf_boot_path= find_goldfish_boot (gf_lib);
  const char* gf_boot     = gf_boot_path.c_str ();

  // 供 goldfish `g_command-line` procedure 查询
  command_args.assign (argv, argv + argc);

  // params: 如 `--mode r7rs` 或 `-m=r7rs`，使用 operator() 取值
  // flag  : 如 `--mode`      或 `-m`，     使用 operator[] 取存在与否

  const std::vector<std::pair<std::string, std::string>> reg_params_pairs= {
      {"--mode", "-m"}, {"--eval", "-e"}, {"--load", "-l"}};

  // 初始化解析器（使用预定义的向量来注册 params）
  argh::parser cmdl;
  for (const auto& fp : reg_params_pairs) {
    cmdl.add_params ({fp.first.c_str (), fp.second.c_str ()});
  }
  cmdl.parse (argc, argv);

  // 只要存在 help 或 version 参数，忽略其他所有参数，打印相应信息后正常退出
  if (cmdl[{"--help", "-h"}]) {
    display_help ();
    exit (0);
  }
  if (cmdl[{"--version", "-v"}]) {
    display_version ();
    exit (0);
  }

  // --mode / -m: Load the standard library by mode
  std::string mode    = "default";
  auto        mode_arg= cmdl ({"--mode", "-m"});
  auto        eval_arg= cmdl ({"--eval", "-e"});
  auto        load_arg= cmdl ({"--load", "-l"});

  for (const auto& reg_params : reg_params_pairs) {
    // 使用 operator[] 检查是否是 flag，即非 params
    if (cmdl[{reg_params.first.c_str (), reg_params.second.c_str ()}]) {
      std::cerr << "Error: '" << reg_params.first << "' or '" << reg_params.second << "' requires a parameter."
                << std::endl;
      exit (1);
    }
  }

  auto repl_flag=
      cmdl[{"--repl", "-i"}] || (cmdl.get_unaccessed_flags ().empty () && cmdl.get_unaccessed_params ().empty ());

  // 没有注册为 params 的默认都是 flag，因此只需要检查未访问的 flag
  if (!cmdl.get_unaccessed_flags ().empty ()) {
    display_for_invalid_options (cmdl.get_unaccessed_flags ());
    exit (1);
  }

  if (mode_arg) mode= mode_arg.str ();
  customize_goldfish_by_mode (sc, mode, gf_boot);

  // start capture error output
  const char* errmsg  = NULL;
  s7_pointer  old_port= s7_set_current_error_port (sc, s7_open_output_string (sc));
  int         gc_loc  = -1;
  if (old_port != s7_nil (sc)) gc_loc= s7_gc_protect (sc, old_port);

  if (eval_arg) {
    std::string code= eval_arg.str ();
    goldfish_eval_code (sc, code);
  }
  if (load_arg) {
    std::string file= load_arg.str ();
    goldfish_eval_file (sc, file, true);
  }

  // eval only the first file passed as positional argument
  auto& files= cmdl.pos_args ();
  if (files.size () > 1) {
    auto it= files.begin () + 1;
    goldfish_eval_file (sc, *it, true);
    // 如果有指定文件且没有显示传入 --repl, -i 参数，不进入 repl
    if (repl_flag && !cmdl[{"--repl", "-i"}]) repl_flag= false;
  }

  if (repl_flag) {
#ifdef GOLDFISH_WITH_REPL
    errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
    if ((errmsg) && (*errmsg)) ic_printf ("[red]%s[/]\n", errmsg);
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);

    goldfish_repl (sc, mode);
    return 0;
#else
    std::cerr << "Interactive REPL is not available in this build.\n" << std::endl;
    // 如果没有指定 --repl, -i，额外打印 help 消息
    if (!cmdl[{"--repl", "-i"}]) display_help ();
    exit (-1);
#endif
  }

  errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
  if ((errmsg) && (*errmsg)) cout << errmsg;
  s7_close_output_port (sc, s7_current_error_port (sc));
  s7_set_current_error_port (sc, old_port);
  if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);

  if ((errmsg) && (*errmsg)) return -1;
  else return 0;
}

} // namespace goldfish
