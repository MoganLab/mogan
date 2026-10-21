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
#include <cerrno>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace goldfish {

using std::string;

// 抛出 (liii error) 约定的 type-error，irritant 为出错的参数
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

static s7_pointer
stat_error (s7_scheme* sc, const char* symbol, const char* msg, const char* path_c) {
  return s7_error (sc, s7_make_symbol (sc, symbol),
                   s7_list (sc, 1, s7_make_string (sc, (string (msg) + ": '" + path_c + "'").c_str ())));
}

static s7_pointer
f_path_stat (s7_scheme* sc, s7_pointer args) {
  s7_pointer path_arg= s7_car (args);
  if (!s7_is_string (path_arg)) {
    return string_type_error (sc, "path-stat: path must be a string", path_arg);
  }
  const char* path_c= s7_string (path_arg);
  if (!path_c) {
    return s7_error (sc, s7_make_symbol (sc, "file-not-found-error"),
                     s7_list (sc, 1, s7_make_string (sc, "path-stat: invalid path")));
  }

#if defined(_WIN32)
  int count= MultiByteToWideChar (CP_UTF8, 0, path_c, -1, NULL, 0);
  if (count <= 0) {
    return stat_error (sc, "file-not-found-error", "No such file or directory", path_c);
  }
  std::wstring wpath (count - 1, L'\0');
  MultiByteToWideChar (CP_UTF8, 0, path_c, -1, &wpath[0], count);

  WIN32_FILE_ATTRIBUTE_DATA fad;
  if (!GetFileAttributesExW (wpath.c_str (), GetFileExInfoStandard, &fad)) {
    DWORD err= GetLastError ();
    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
      return stat_error (sc, "file-not-found-error", "No such file or directory", path_c);
    }
    else if (err == ERROR_ACCESS_DENIED) {
      return stat_error (sc, "permission-denied-error", "Permission denied", path_c);
    }
    else {
      return stat_error (sc, "os-error", "stat failed for", path_c);
    }
  }
  DWORD attrs= fad.dwFileAttributes;

  // 116444736000000000ULL 为 FILETIME(1601) 与 Unix epoch(1970) 之间的 100ns 刻度数
  auto filetime_to_unix= [] (const FILETIME& ft) -> double {
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart= ft.dwHighDateTime;
    if (ull.QuadPart < 116444736000000000ULL) return 0.0;
    return (double) (ull.QuadPart - 116444736000000000ULL) / 10000000.0;
  };
  LARGE_INTEGER liSize;
  liSize.LowPart = fad.nFileSizeLow;
  liSize.HighPart= fad.nFileSizeHigh;
  s7_int size    = (s7_int) liSize.QuadPart;
  double ctime   = filetime_to_unix (fad.ftCreationTime);
  double atime   = filetime_to_unix (fad.ftLastAccessTime);
  double mtime   = filetime_to_unix (fad.ftLastWriteTime);

  // st_mode 的权限位只能来自 _wstat64；失败时按目录/普通文件兜底
  s7_int          mode= 0;
  struct __stat64 st;
  if (_wstat64 (wpath.c_str (), &st) == 0) {
    mode= (s7_int) st.st_mode;
  }
  else if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
    mode= 0040000;
  }
  else {
    mode= 0100000;
  }

  if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) {
    mode= (mode & ~0170000) | 0120000;
  }
#else
  struct stat st;
  int         ret= lstat (path_c, &st);
  if (ret != 0) {
    if (errno == ENOENT || errno == ENOTDIR) {
      return stat_error (sc, "file-not-found-error", "No such file or directory", path_c);
    }
    else if (errno == EACCES || errno == EPERM) {
      return stat_error (sc, "permission-denied-error", "Permission denied", path_c);
    }
    else {
      return stat_error (sc, "os-error", "stat failed for", path_c);
    }
  }

  s7_int size= (s7_int) st.st_size;
#if defined(__APPLE__)
  double atime= (double) st.st_atime + (double) st.st_atimespec.tv_nsec / 1e9;
  double mtime= (double) st.st_mtime + (double) st.st_mtimespec.tv_nsec / 1e9;
  double ctime= (double) st.st_ctime + (double) st.st_ctimespec.tv_nsec / 1e9;
#elif defined(__EMSCRIPTEN__)
  double atime= (double) st.st_atime;
  double mtime= (double) st.st_mtime;
  double ctime= (double) st.st_ctime;
#else
  double atime= (double) st.st_atim.tv_sec + (double) st.st_atim.tv_nsec / 1e9;
  double mtime= (double) st.st_mtim.tv_sec + (double) st.st_mtim.tv_nsec / 1e9;
  double ctime= (double) st.st_ctim.tv_sec + (double) st.st_ctim.tv_nsec / 1e9;
#endif
  s7_int mode= (s7_int) st.st_mode;
#endif

  s7_pointer result= s7_make_vector (sc, 5);
  s7_vector_set (sc, result, 0, s7_make_integer (sc, size));
  s7_vector_set (sc, result, 1, s7_make_real (sc, mtime));
  s7_vector_set (sc, result, 2, s7_make_real (sc, atime));
  s7_vector_set (sc, result, 3, s7_make_real (sc, ctime));
  s7_vector_set (sc, result, 4, s7_make_integer (sc, mode));
  return result;
}

static void
glue_path_stat (s7_scheme* sc) {
  const char* name= "g_path-stat";
  const char* desc= "(g_path-stat path) => vector #(size mtime atime ctime mode), stat a file path";
  glue_define (sc, name, desc, f_path_stat, 1, 0);
}

void
glue_liii_path_stat (s7_scheme* sc) {
  glue_path_stat (sc);
}

} // namespace goldfish
