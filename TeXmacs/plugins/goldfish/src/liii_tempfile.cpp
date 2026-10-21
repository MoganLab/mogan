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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "s7.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include <tbox/tbox.h>

#if !defined(TB_CONFIG_OS_WINDOWS)
#include <sys/stat.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace goldfish {

namespace fs= std::filesystem;

inline s7_pointer
string_type_error (s7_scheme* sc, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, "type-error"), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional) {
  s7_pointer cur_env= s7_curlet (sc);
  s7_pointer func   = s7_make_typed_function (sc, name, f, required, optional, false, desc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

static std::string
generate_random_alphanumeric (std::size_t len) {
  static const char charset[]= "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  // 生成器用函数内 static 单例：避免重试循环里反复构造 random_device/mt19937
  static std::mt19937                gen (std::random_device{}());
  std::uniform_int_distribution<int> dis (0, sizeof (charset) - 2);

  std::string s;
  s.reserve (len);
  for (std::size_t i= 0; i < len; ++i) {
    s+= charset[dis (gen)];
  }
  return s;
}

static std::string
temp_base_dir (const char* dir_c) {
  std::string base= dir_c;
  if (!base.empty () && base.back () != '/' && base.back () != '\\') {
#if defined(TB_CONFIG_OS_WINDOWS)
    base+= '\\';
#else
    base+= '/';
#endif
  }
  return base;
}

// 按 errno / GetLastError 映射为 (liii error) 约定的错误类别
#if !defined(TB_CONFIG_OS_WINDOWS)
static s7_pointer
temp_file_error (s7_scheme* sc, int err, const char* path) {
  const char* tag= "os-error";
  std::string msg;
  if (err == ENOENT || err == ENOTDIR) {
    tag= "file-not-found-error";
    msg= std::string ("No such file or directory: '") + path + "'";
  }
  else if (err == EACCES || err == EPERM) {
    tag= "permission-denied-error";
    msg= std::string ("Permission denied: '") + path + "'";
  }
  else {
    msg= strerror (err);
  }
  return s7_error (sc, s7_make_symbol (sc, tag),
                   s7_list (sc, 2, s7_make_string (sc, msg.c_str ()), s7_make_string (sc, path)));
}
#else
static s7_pointer
temp_file_error (s7_scheme* sc, DWORD err, const char* path) {
  const char* tag= "os-error";
  std::string msg;
  if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
    tag= "file-not-found-error";
    msg= std::string ("No such file or directory: '") + path + "'";
  }
  else if (err == ERROR_ACCESS_DENIED) {
    tag= "permission-denied-error";
    msg= std::string ("Permission denied: '") + path + "'";
  }
  else {
    msg= std::string ("system error ") + std::to_string ((unsigned long) err);
  }
  return s7_error (sc, s7_make_symbol (sc, tag),
                   s7_list (sc, 2, s7_make_string (sc, msg.c_str ()), s7_make_string (sc, path)));
}
#endif

// 以 prefix+6 位随机字符+suffix 生成候选名并调用 try_create 尝试创建，最多 100 次。
// try_create 返回 0=创建成功，1=名字冲突重试，-1=致命错误（错误码经 errno/GetLastError 取）。
// 返回 0=成功(chosen 已填)，1=冲突耗尽，-1=致命错误；chosen 始终记录最后一次候选名。
template <typename F>
static int
create_temp_unique (const std::string& base, const char* prefix_c, const char* suffix_c, std::string& chosen,
                    F&& try_create) {
  for (int i= 0; i < 100; ++i) {
    chosen= base + prefix_c + generate_random_alphanumeric (6) + suffix_c;
    int r = try_create (chosen);
    if (r != 1) return r;
  }
  return 1;
}

static s7_pointer
temp_exhausted_error (s7_scheme* sc, const char* what) {
  return s7_error (
      sc, s7_make_symbol (sc, "os-error"),
      s7_list (sc, 1,
               s7_make_string (
                   sc, (std::string (what) + ": failed to create unique temporary name after 100 attempts").c_str ())));
}

static s7_pointer
f_mkstemp (s7_scheme* sc, s7_pointer args) {
  s7_pointer prefix_arg= s7_car (args);
  s7_pointer suffix_arg= s7_cadr (args);
  s7_pointer dir_arg   = s7_caddr (args);

  if (!s7_is_string (prefix_arg)) {
    return string_type_error (sc, "make-temp-file: prefix must be a string", prefix_arg);
  }
  if (!s7_is_string (suffix_arg)) {
    return string_type_error (sc, "make-temp-file: suffix must be a string", suffix_arg);
  }
  if (!s7_is_string (dir_arg)) {
    return string_type_error (sc, "make-temp-file: dir must be a string", dir_arg);
  }

  const char* prefix_c= s7_string (prefix_arg);
  const char* suffix_c= s7_string (suffix_arg);
  const char* dir_c   = s7_string (dir_arg);

  std::string base= temp_base_dir (dir_c);

#if !defined(TB_CONFIG_OS_WINDOWS)
  std::string tpl       = base + prefix_c + "XXXXXX" + suffix_c;
  int         suffix_len= static_cast<int> (std::strlen (suffix_c));
  int         fd        = mkstemps (&tpl[0], suffix_len);
  if (fd < 0) {
    return temp_file_error (sc, errno, tpl.c_str ());
  }
  close (fd);
  s7_pointer port= s7_open_output_file (sc, tpl.c_str (), "w");
  return s7_values (sc, s7_list (sc, 2, port, s7_make_string (sc, tpl.c_str ())));
#else
  DWORD       last_err= 0;
  std::string chosen_path;
  int         r= create_temp_unique (base, prefix_c, suffix_c, chosen_path, [&last_err] (const std::string& cand) {
    HANDLE hFile= CreateFileA (cand.c_str (), GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_NEW,
                                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
      CloseHandle (hFile);
      return 0;
    }
    last_err= GetLastError ();
    return (last_err == ERROR_FILE_EXISTS || last_err == ERROR_ALREADY_EXISTS) ? 1 : -1;
  });
  if (r == 1) return temp_exhausted_error (sc, "make-temp-file");
  if (r < 0) return temp_file_error (sc, last_err, chosen_path.c_str ());
  s7_pointer port= s7_open_output_file (sc, chosen_path.c_str (), "w");
  return s7_values (sc, s7_list (sc, 2, port, s7_make_string (sc, chosen_path.c_str ())));
#endif
}

static s7_pointer
f_mkdtemp (s7_scheme* sc, s7_pointer args) {
  s7_pointer prefix_arg= s7_car (args);
  s7_pointer suffix_arg= s7_cadr (args);
  s7_pointer dir_arg   = s7_caddr (args);

  if (!s7_is_string (prefix_arg)) {
    return string_type_error (sc, "make-temp-dir: prefix must be a string", prefix_arg);
  }
  if (!s7_is_string (suffix_arg)) {
    return string_type_error (sc, "make-temp-dir: suffix must be a string", suffix_arg);
  }
  if (!s7_is_string (dir_arg)) {
    return string_type_error (sc, "make-temp-dir: dir must be a string", dir_arg);
  }

  const char* prefix_c= s7_string (prefix_arg);
  const char* suffix_c= s7_string (suffix_arg);
  const char* dir_c   = s7_string (dir_arg);

  std::string base= temp_base_dir (dir_c);

#if !defined(TB_CONFIG_OS_WINDOWS)
  std::string chosen_path;
  int         r= create_temp_unique (base, prefix_c, suffix_c, chosen_path, [] (const std::string& cand) {
    if (mkdir (cand.c_str (), 0700) == 0) return 0;
    return errno == EEXIST ? 1 : -1;
  });
  if (r == 1) return temp_exhausted_error (sc, "make-temp-dir");
  if (r < 0) return temp_file_error (sc, errno, chosen_path.c_str ());
  return s7_make_string (sc, chosen_path.c_str ());
#else
  DWORD       last_err= 0;
  std::string chosen_path;
  int         r= create_temp_unique (base, prefix_c, suffix_c, chosen_path, [&last_err] (const std::string& cand) {
    if (CreateDirectoryA (cand.c_str (), NULL)) return 0;
    last_err= GetLastError ();
    return last_err == ERROR_ALREADY_EXISTS ? 1 : -1;
  });
  if (r == 1) return temp_exhausted_error (sc, "make-temp-dir");
  if (r < 0) return temp_file_error (sc, last_err, chosen_path.c_str ());
  return s7_make_string (sc, chosen_path.c_str ());
#endif
}

static s7_pointer
f_rmtree (s7_scheme* sc, s7_pointer args) {
  s7_pointer path_arg= s7_car (args);
  if (!s7_is_string (path_arg)) {
    return string_type_error (sc, "rmtree: path must be a string", path_arg);
  }
  const char*     path_c= s7_string (path_arg);
  std::error_code ec;
  fs::remove_all (fs::u8path (path_c), ec);
  return s7_make_boolean (sc, !ec);
}

static s7_pointer
f_file_mode (s7_scheme* sc, s7_pointer args) {
  s7_pointer path_arg= s7_car (args);
  if (!s7_is_string (path_arg)) {
    return string_type_error (sc, "file-mode: path must be a string", path_arg);
  }
  const char* path_c= s7_string (path_arg);
#if !defined(TB_CONFIG_OS_WINDOWS)
  struct stat st;
  if (stat (path_c, &st) != 0) {
    return s7_make_integer (sc, -1);
  }
  return s7_make_integer (sc, st.st_mode & 0777);
#else
  return s7_make_integer (sc, 0600);
#endif
}

static s7_pointer
f_chmod (s7_scheme* sc, s7_pointer args) {
  s7_pointer path_arg= s7_car (args);
  if (!s7_is_string (path_arg)) {
    return string_type_error (sc, "chmod: path must be a string", path_arg);
  }
  s7_pointer mode_arg= s7_cadr (args);
  if (!s7_is_integer (mode_arg)) {
    return string_type_error (sc, "chmod: mode must be an integer", mode_arg);
  }
  const char* path_c= s7_string (path_arg);
  int         mode  = s7_integer (mode_arg);
#if !defined(TB_CONFIG_OS_WINDOWS)
  return s7_make_boolean (sc, chmod (path_c, static_cast<mode_t> (mode)) == 0);
#else
  return s7_make_boolean (sc, true);
#endif
}

void
glue_liii_tempfile (s7_scheme* sc) {
  glue_define (sc, "g_mkstemp", "(g_mkstemp prefix suffix dir) => (values port path)", f_mkstemp, 3, 0);
  glue_define (sc, "g_mkdtemp", "(g_mkdtemp prefix suffix dir) => string", f_mkdtemp, 3, 0);
  glue_define (sc, "g_rmtree", "(g_rmtree path) => boolean", f_rmtree, 1, 0);
  glue_define (sc, "g_file-mode", "(g_file-mode path) => integer", f_file_mode, 1, 0);
  glue_define (sc, "g_chmod", "(g_chmod path mode) => boolean", f_chmod, 2, 0);
}

} // namespace goldfish
