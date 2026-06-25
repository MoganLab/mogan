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
#include <argh.h>
#include <cctype>
#include <chrono>
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
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

#include <nlohmann/json.hpp>

#ifdef GOLDFISH_WITH_REPL
#include <functional>
#include <isocline.h>
#endif

#define GOLDFISH_VERSION "18.11.13"

#define GOLDFISH_PATH_MAXN TB_PATH_MAXN

static std::vector<std::string> command_args= std::vector<std::string> ();

// Declare environ for non-Windows platforms (needed for f_getenvs)
#if !defined(TB_CONFIG_OS_WINDOWS)
extern char** environ;
#endif

namespace goldfish {
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

namespace fs= std::filesystem;

using nlohmann::json;

inline void glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required,
                         s7_int optional);

static s7_pointer f_function_libraries (s7_scheme* sc, s7_pointer args);

static s7_pointer f_gfproject_load_config (s7_scheme* sc, s7_pointer args);

static s7_pointer f_project_root (s7_scheme* sc, s7_pointer args);

static bool split_library_query (const string& query, string& group, string& library);

static string find_goldfish_library ();

static vector<string> find_function_libraries_in_load_path (s7_scheme* sc, const string& function_name);

void glue_njson (s7_scheme* sc);
void glue_http (s7_scheme* sc);
void glue_http_async (s7_scheme* sc);
void glue_liii_hashlib (s7_scheme* sc);
void glue_liii_os (s7_scheme* sc);
void glue_liii_path (s7_scheme* sc);
void glue_subprocess_run_values (s7_scheme* sc);

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
  const char* s_gfproject_load_config= "g_gfproject-load-config";
  const char* d_gfproject_load_config= "(g_gfproject-load-config) => string, returns merged gfproject.json";
  const char* s_project_root         = "g_project-root";
  const char* d_project_root=
      "(g_project-root) => string or #f, returns the directory containing the local gfproject.json, "
      "or #f if none is found in the current working directory";

  s7_define (sc, cur_env, s7_make_symbol (sc, s_version),
             s7_make_typed_function (sc, s_version, f_version, 0, 0, false, d_version, NULL));

  s7_define (sc, cur_env, s7_make_symbol (sc, s_delete_file),
             s7_make_typed_function (sc, s_delete_file, f_delete_file, 1, 0, false, d_delete_file, NULL));

  s7_define (
      sc, cur_env, s7_make_symbol (sc, s_function_libraries),
      s7_make_typed_function (sc, s_function_libraries, f_function_libraries, 1, 0, false, d_function_libraries, NULL));

  s7_define (sc, cur_env, s7_make_symbol (sc, s_gfproject_load_config),
             s7_make_typed_function (sc, s_gfproject_load_config, f_gfproject_load_config, 0, 0, false,
                                     d_gfproject_load_config, NULL));

  s7_define (sc, cur_env, s7_make_symbol (sc, s_project_root),
             s7_make_typed_function (sc, s_project_root, f_project_root, 0, 0, false, d_project_root, NULL));
}

// old `f_current_second` TODO: use std::chrono::tai_clock::now() when using C++ 20
//                        NOTE(jinser): use a new name for tai
// `current-second` impl by g_get-time-of-day now
static s7_pointer
f_get_time_of_day (s7_scheme* sc, s7_pointer args) {
  using namespace std::chrono;
  auto now        = time_point_cast<microseconds> (system_clock::now ());
  auto since_epoch= now.time_since_epoch ();
  auto sec        = duration_cast<seconds> (since_epoch);

  s7_pointer vs=
      s7_list (sc, 2, s7_make_integer (sc, sec.count ()), s7_make_integer (sc, (since_epoch - sec).count ()));
  return s7_values (sc, vs);
}

static s7_pointer
f_monotonic_nanosecond (s7_scheme* sc, s7_pointer args) {
  using namespace std::chrono;
  auto now     = steady_clock::now ();
  auto duration= now.time_since_epoch ();
  auto count   = duration_cast<std::chrono::nanoseconds> (duration).count ();
  return s7_make_integer (sc, count);
}

template <typename Clock>
constexpr int64_t
clock_resolution_ns () {
  typedef std::chrono::duration<double, std::nano> NS;
  NS                                               ns= typename Clock::duration (1);
  return ns.count ();
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
  const char* d_monotonic_nanosecond= "(g_monotonic-nanosecond): () => integer, returns the steady clock's monotonic "
                                      "nanoseconds since an unspecified epoch";
  s7_define (sc, cur_env, s7_make_symbol (sc, s_monotonic_nanosecond),
             s7_make_typed_function (sc, s_monotonic_nanosecond, f_monotonic_nanosecond, 0, 0, false,
                                     d_monotonic_nanosecond, NULL));

  s7_define_constant_with_environment (sc, cur_env, "g_system-clock-resolution",
                                       s7_make_integer (sc, clock_resolution_ns<std::chrono::system_clock> ()));
  s7_define_constant_with_environment (sc, cur_env, "g_steady-clock-resolution",
                                       s7_make_integer (sc, clock_resolution_ns<std::chrono::steady_clock> ()));
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
f_getenvs (s7_scheme* sc, s7_pointer args) {
  s7_pointer p= s7_nil (sc);

#ifdef TB_CONFIG_OS_WINDOWS
  // Windows: use GetEnvironmentStrings
  LPCH env_strings= GetEnvironmentStrings ();
  if (env_strings) {
    LPCH env= env_strings;
    while (*env) {
      const char* eq= strchr (env, '=');
      if (eq && eq != env) { // skip empty variable names
        s7_pointer name = s7_make_string_with_length (sc, env, eq - env);
        s7_pointer value= s7_make_string (sc, eq + 1);
        p               = s7_cons (sc, s7_cons (sc, name, value), p);
      }
      env+= strlen (env) + 1;
    }
    FreeEnvironmentStrings (env_strings);
  }
#else
  // Unix/Linux/macOS: use environ (declared at global scope)
  for (int32_t i= 0; environ[i]; i++) {
    const char* eq= strchr (environ[i], '=');
    if (eq) {
      s7_pointer name = s7_make_string_with_length (sc, environ[i], eq - environ[i]);
      s7_pointer value= s7_make_string (sc, eq + 1);
      p               = s7_cons (sc, s7_cons (sc, name, value), p);
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
  char     buffer[PATH_MAX];
  uint32_t size= sizeof (buffer);
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

static bool
which_access_check (const char* path) {
#ifdef TB_CONFIG_OS_WINDOWS
  tb_file_info_t info;
  return tb_file_info (path, &info) && info.type == TB_FILE_TYPE_FILE;
#else
  return tb_file_access (path, TB_FILE_MODE_EXEC);
#endif
}

static s7_pointer
f_which (s7_scheme* sc, s7_pointer args) {
  const char* cmd_c        = s7_string (s7_car (args));
  s7_pointer  path_arg     = s7_cdr (args);
  const char* path_override= nullptr;

  if (s7_is_pair (path_arg)) {
    path_override= s7_string (s7_car (path_arg));
  }

  string         cmd_str (cmd_c);
  vector<string> search_dirs;
  string         cmd_name;

  bool has_dir_sep= (cmd_str.find ('/') != string::npos) || (cmd_str.find ('\\') != string::npos);

  if (has_dir_sep) {
    size_t last_sep= cmd_str.find_last_of ("/\\");
    string dir     = cmd_str.substr (0, last_sep);
    cmd_name       = cmd_str.substr (last_sep + 1);
    if (dir.empty ()) {
      dir= ".";
    }
    search_dirs.push_back (dir);
  }
  else {
    cmd_name= cmd_str;

    string path_env;
    if (path_override != nullptr) {
      path_env= path_override;
    }
    else {
      const char* env= getenv ("PATH");
      if (env != nullptr) {
        path_env= env;
      }
    }

    if (path_env.empty ()) {
      return s7_make_boolean (sc, false);
    }

    char path_sep= ':';
#ifdef TB_CONFIG_OS_WINDOWS
    path_sep= ';';
#endif

    size_t start= 0;
    size_t end  = path_env.find (path_sep);
    while (end != string::npos) {
      string dir= path_env.substr (start, end - start);
      if (!dir.empty ()) {
        search_dirs.push_back (dir);
      }
      start= end + 1;
      end  = path_env.find (path_sep, start);
    }
    string last_dir= path_env.substr (start);
    if (!last_dir.empty ()) {
      search_dirs.push_back (last_dir);
    }
  }

  vector<string> files_to_check;

#ifdef TB_CONFIG_OS_WINDOWS
  vector<string> exts;
  const char*    pathext= getenv ("PATHEXT");
  if (pathext != nullptr) {
    string ext_str (pathext);
    size_t start= 0;
    size_t end  = ext_str.find (';');
    while (end != string::npos) {
      string ext= ext_str.substr (start, end - start);
      if (!ext.empty ()) {
        if (ext[0] == '.') ext= ext.substr (1);
        exts.push_back (ext);
      }
      start= end + 1;
      end  = ext_str.find (';', start);
    }
    string last_ext= ext_str.substr (start);
    if (!last_ext.empty ()) {
      if (last_ext[0] == '.') last_ext= last_ext.substr (1);
      exts.push_back (last_ext);
    }
  }

  files_to_check.push_back (cmd_name);
  for (const string& ext : exts) {
    files_to_check.push_back (cmd_name + "." + ext);
  }
#else
  files_to_check.push_back (cmd_name);
#endif

  for (const string& dir : search_dirs) {
    for (const string& file : files_to_check) {
#ifdef TB_CONFIG_OS_WINDOWS
      string full_path= dir + "\\" + file;
#else
      string full_path= dir + "/" + file;
#endif
      if (which_access_check (full_path.c_str ())) {
        return s7_make_string (sc, full_path.c_str ());
      }
    }
  }

  return s7_make_boolean (sc, false);
}

inline void
glue_which (s7_scheme* sc) {
  const char* name= "g_which";
  const char* desc= "(g_which cmd [path]) => string or #f, locate a command in PATH or given search path";
  glue_define (sc, name, desc, f_which, 1, 1);
}

inline void
glue_executable (s7_scheme* sc) {
  const char* name= "g_executable";
  const char* desc= "(g_executable) => string";
  glue_define (sc, name, desc, f_executable, 0, 0);
}

inline void
glue_liii_sys (s7_scheme* sc) {
  glue_which (sc);
  glue_executable (sc);
}

static s7_pointer
f_sleep (s7_scheme* sc, s7_pointer args) {
  s7_double seconds= s7_real (s7_car (args));

  // 使用 tbox 的 tb_sleep 函数，参数是毫秒
  tb_msleep ((tb_long_t) (seconds * 1000));

  return s7_nil (sc);
}

inline void
glue_sleep (s7_scheme* sc) {
  const char* name= "g_sleep";
  const char* desc= "(g_sleep seconds) => nil, sleep for the specified number of seconds";
  glue_define (sc, name, desc, f_sleep, 1, 0);
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
  glue_subprocess_run_values (sc);
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
  cout
      << "                     Prefer single quotes so double quotes inside Scheme strings usually do not need escaping"
      << endl;
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
  cout << "                     Quote FUNC for names like \"bag-delete!\", \"path?\", \"alist->fxmapping\", or "
          "\"bag<=?\""
       << endl;
  cout << "                     This preserves symbols such as ! ? > < and keeps FUNC as one shell argument" << endl;
  cout << "  doc FUNC           Search visible libraries for exported FUNC, then show its doc/test file" << endl;
  cout << "                     If multiple libraries export it, candidates are listed" << endl;
  cout << "                     Example: gf doc \"string-split\"" << endl;
  cout << "                     Quote FUNC for names like \"bag-delete!\", \"path?\", \"alist->fxmapping\", or "
          "\"bag<=?\""
       << endl;
  cout << "                     This keeps shell-sensitive symbols intact and makes it clear FUNC is one argument"
       << endl;
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
  cout << "  -e CODE            Alias for eval CODE" << endl;
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
      if (std::isspace (ch) || ch == '"' || ch == '\'' || ch == '`' || ch == '(' || ch == ')' || ch == ',' ||
          ch == ';') {
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

  start+= infix.size ();
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
goldfish_error_expression_contains_function_call (s7_scheme* sc, const string& expression,
                                                  const string& function_name) {
  if (expression.empty ()) {
    return false;
  }

  s7_pointer port      = s7_open_input_string (sc, expression.c_str ());
  s7_pointer eof_object= s7_eof_object (sc);
  s7_pointer form      = s7_read (sc, port);
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

  start+= prefix.size ();
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
    formatted+= '\n';
  }
  formatted+=
      "Hint: try `" + goldfish_cli_program_name () + " fix " + path + "` to repair common parenthesis issues.\n";
  return formatted;
}

static string
goldfish_shell_double_quote (const string& value) {
  string quoted= "\"";
  for (char ch : value) {
    switch (ch) {
    case '\\':
      quoted+= "\\\\";
      break;
    case '"':
      quoted+= "\\\"";
      break;
    case '$':
      quoted+= "\\$";
      break;
    case '`':
      quoted+= "\\`";
      break;
    default:
      quoted+= ch;
      break;
    }
  }
  quoted+= "\"";
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
    formatted+= '\n';
  }

  vector<string> library_queries;
  try {
    library_queries= find_function_libraries_in_load_path (sc, function_name);
  } catch (const std::exception&) {
    library_queries.clear ();
  }

  if (library_queries.empty ()) {
    formatted+=
        "Hint: try `" + goldfish_cli_program_name () + " doc " + goldfish_shell_double_quote (function_name) + "`\n";
    formatted+=
        "`" + goldfish_cli_program_name () + " doc` may show similarly named functions when there is no exact match.\n";
    formatted+= "If it finds nothing similar, try searching the codebase with `git grep " +
                goldfish_shell_double_quote (function_name) +
                "`, implement that function yourself, or stop using it.\n";
    return formatted;
  }

  if (library_queries.size () == 1) {
    string import_form= goldfish_library_import_form (library_queries.front ());
    formatted+= "Hint: function `" + function_name + "` exists in library `" +
                goldfish_library_display_name (library_queries.front ()) + "`.\n";
    if (!import_form.empty ()) {
      formatted+= "Please import that library first: `" + import_form + "`.\n";
    }
    return formatted;
  }

  formatted+= "Hint: function `" + function_name + "` exists in multiple visible libraries:\n";
  for (const auto& library_query : library_queries) {
    formatted+= "  " + goldfish_library_display_name (library_query) + "\n";
  }
  formatted+= "Try one of these commands to decide which library to use:\n";
  for (const auto& library_query : library_queries) {
    formatted+= "  " + goldfish_library_doc_command (library_query, function_name) + "\n";
  }
  return formatted;
}

static void
goldfish_render_scheme_error_message (s7_scheme* sc, const char* errmsg, string& rendered) {
  rendered= goldfish_append_doc_hint_if_needed (sc, goldfish_format_scheme_error_message (errmsg));
  if ((!rendered.empty ()) && (rendered.back () != '\n')) {
    rendered+= '\n';
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
  string     wrapped_code= "(begin " + code + " )";
  s7_pointer x           = s7_eval_c_string (sc, wrapped_code.c_str ());
  cout << s7_object_to_c_string (sc, x) << endl;
}

static string
find_golddoc_tool_root (const char* gf_lib) {
  std::error_code  ec;
  vector<fs::path> candidates= {fs::path (gf_lib) / "tools" / "doc",
                                fs::path (gf_lib).parent_path () / "tools" / "doc"};

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
  std::error_code  ec;
  vector<fs::path> candidates= {fs::path (gf_lib) / "tools" / "source",
                                fs::path (gf_lib).parent_path () / "tools" / "source"};

  for (const auto& candidate : candidates) {
    if (fs::is_directory (candidate, ec)) {
      return candidate.string ();
    }
    ec.clear ();
  }

  return "";
}

static string
find_goldhelp_tool_root (const char* gf_lib) {
  std::error_code  ec;
  vector<fs::path> candidates= {fs::path (gf_lib) / "tools" / "help",
                                fs::path (gf_lib).parent_path () / "tools" / "help"};

  for (const auto& candidate : candidates) {
    if (fs::is_directory (candidate, ec)) {
      return candidate.string ();
    }
    ec.clear ();
  }

  return "";
}

// Tool registration system - dynamically load tools from gfproject.json

static fs::path
find_local_gfproject_json () {
  std::error_code ec;
  fs::path        cwd= fs::current_path (ec);
  if (!ec) {
    fs::path local_path= cwd / "gfproject.json";
    if (fs::exists (local_path, ec) && fs::is_regular_file (local_path, ec)) {
      return local_path;
    }
  }
  return "";
}

static fs::path
find_lib_gfproject_json (const char* gf_lib) {
  std::error_code ec;
  fs::path        lib_path= fs::path (gf_lib) / "gfproject.json";
  if (fs::exists (lib_path, ec) && fs::is_regular_file (lib_path, ec)) {
    return lib_path;
  }
  ec.clear ();
  fs::path parent_path= fs::path (gf_lib).parent_path () / "gfproject.json";
  if (fs::exists (parent_path, ec) && fs::is_regular_file (parent_path, ec)) {
    return parent_path;
  }
  return "";
}

static json
load_json_file_or_empty (const fs::path& path) {
  if (path.empty ()) {
    return json::object ();
  }
  try {
    std::ifstream file (path);
    if (!file.is_open ()) {
      return json::object ();
    }
    json config;
    file >> config;
    if (config.is_object ()) {
      return config;
    }
    return json::object ();
  } catch (...) {
    return json::object ();
  }
}

static json
gfproject_extract_tools (const json& config) {
  if (config.is_object () && config.contains ("tools") && config["tools"].is_object ()) {
    return config["tools"];
  }
  return json::object ();
}

static json
gfproject_deep_merge_value (const json& base, const json& overlay) {
  if (base.is_object () && overlay.is_object ()) {
    json merged= base;
    for (const auto& [key, overlay_value] : overlay.items ()) {
      if (merged.contains (key)) {
        merged[key]= gfproject_deep_merge_value (merged[key], overlay_value);
      }
      else {
        merged[key]= overlay_value;
      }
    }
    return merged;
  }
  return overlay;
}

struct gfproject_config_bundle {
  json lib_config;
  json local_config;
  json merged_config;
};

static gfproject_config_bundle
load_gfproject_config_bundle (const char* gf_lib) {
  gfproject_config_bundle bundle;
  bundle.lib_config  = load_json_file_or_empty (find_lib_gfproject_json (gf_lib));
  bundle.local_config= load_json_file_or_empty (find_local_gfproject_json ());

  json merged      = bundle.lib_config.is_object () ? bundle.lib_config : json::object ();
  json merged_tools= gfproject_extract_tools (bundle.lib_config);
  json local_tools = gfproject_extract_tools (bundle.local_config);

  for (const auto& [command, local_tool] : local_tools.items ()) {
    if (merged_tools.contains (command)) {
      merged_tools[command]= gfproject_deep_merge_value (merged_tools[command], local_tool);
    }
    else {
      merged_tools[command]= local_tool;
    }
  }

  merged["tools"]     = merged_tools;
  bundle.merged_config= merged;
  return bundle;
}

static json
load_gfproject_config (const char* gf_lib) {
  return load_gfproject_config_bundle (gf_lib).merged_config;
}

struct gfproject_tool_resolution {
  bool has_local_override= false;
  bool has_merged_tool   = false;
  bool has_lib_tool      = false;
  json merged_tool       = json::object ();
  json lib_tool          = json::object ();
};

static gfproject_tool_resolution
resolve_gfproject_tool (const char* gf_lib, const string& command) {
  gfproject_config_bundle bundle      = load_gfproject_config_bundle (gf_lib);
  json                    local_tools = gfproject_extract_tools (bundle.local_config);
  json                    lib_tools   = gfproject_extract_tools (bundle.lib_config);
  json                    merged_tools= gfproject_extract_tools (bundle.merged_config);

  gfproject_tool_resolution resolved;
  resolved.has_local_override= local_tools.contains (command);
  resolved.has_merged_tool   = merged_tools.contains (command);
  resolved.has_lib_tool      = lib_tools.contains (command);

  if (resolved.has_merged_tool) {
    resolved.merged_tool= merged_tools[command];
  }
  if (resolved.has_lib_tool) {
    resolved.lib_tool= lib_tools[command];
  }
  return resolved;
}

static void
goldfish_reset_captured_error_port (s7_scheme* sc) {
  s7_close_output_port (sc, s7_current_error_port (sc));
  s7_set_current_error_port (sc, s7_open_output_string (sc));
}

enum class gfproject_tool_prepare_error {
  none,
  incomplete_config,
  invalid_config_value,
  missing_tool_root,
  import_failed,
  missing_main,
};

struct gfproject_tool_prepare_result {
  gfproject_tool_prepare_error error= gfproject_tool_prepare_error::none;
  string                       message;
  s7_pointer                   main_func= nullptr;
};

static string find_tool_root_by_command (const char* gf_lib, const string& command);

static gfproject_tool_prepare_result
goldfish_prepare_tool_main (s7_scheme* sc, const char* gf_lib, const string& command, const json& tool_config) {
  gfproject_tool_prepare_result result;

  if (!tool_config.is_object ()) {
    result.error  = gfproject_tool_prepare_error::invalid_config_value;
    result.message= "Error: Tool '" + command + "' config must be a JSON object.";
    return result;
  }

  if (!tool_config.contains ("organization") || !tool_config.contains ("module")) {
    result.error  = gfproject_tool_prepare_error::incomplete_config;
    result.message= "Error: Tool '" + command + "' is not fully implemented (missing organization or module).";
    return result;
  }

  if (!tool_config["organization"].is_string () || !tool_config["module"].is_string ()) {
    result.error  = gfproject_tool_prepare_error::invalid_config_value;
    result.message= "Error: Tool '" + command + "' organization/module must be strings.";
    return result;
  }

  string org      = tool_config["organization"];
  string module   = tool_config["module"];
  string tool_root= find_tool_root_by_command (gf_lib, command);

  if (tool_root.empty ()) {
    result.error  = gfproject_tool_prepare_error::missing_tool_root;
    result.message= "Error: tools/" + command + "/" + org + " directory not found.";
    return result;
  }

  s7_add_to_load_path (sc, tool_root.c_str ());

  string      import_expr  = "(import (" + org + " " + module + "))";
  s7_pointer  import_result= s7_eval_c_string (sc, import_expr.c_str ());
  const char* errmsg       = s7_get_output_string (sc, s7_current_error_port (sc));
  if (!import_result || ((errmsg) && (*errmsg))) {
    result.error  = gfproject_tool_prepare_error::import_failed;
    result.message= "Error importing (" + org + " " + module + "):";
    return result;
  }

  s7_pointer main_func= s7_name_to_value (sc, "main");
  if ((!main_func) || (!s7_is_procedure (main_func))) {
    result.error  = gfproject_tool_prepare_error::missing_main;
    result.message= "Error: Failed to find main function in (" + org + " " + module + ").";
    return result;
  }

  result.main_func= main_func;
  return result;
}

static int
goldfish_finish_tool_error (s7_scheme* sc, const string& message, const char*& errmsg, s7_pointer old_port, int gc_loc,
                            bool include_scheme_error) {
  errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
  if (!message.empty ()) {
    if (include_scheme_error && (errmsg) && (*errmsg)) {
      goldfish_print_prefixed_scheme_error_message (sc, message, errmsg);
    }
    else {
      cerr << message << endl;
    }
  }
  else if ((errmsg) && (*errmsg)) {
    goldfish_print_scheme_error_message (sc, errmsg);
  }
  s7_close_output_port (sc, s7_current_error_port (sc));
  s7_set_current_error_port (sc, old_port);
  if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
  return 1;
}

static int
goldfish_finish_tool_success (s7_scheme* sc, s7_pointer result, const char*& errmsg, s7_pointer old_port, int gc_loc) {
  errmsg= s7_get_output_string (sc, s7_current_error_port (sc));
  goldfish_print_scheme_error_message (sc, errmsg);
  s7_close_output_port (sc, s7_current_error_port (sc));
  s7_set_current_error_port (sc, old_port);
  if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
  if (s7_is_integer (result)) {
    return static_cast<int> (s7_integer (result));
  }
  return 0;
}

static int
goldfish_run_tool_with_config (s7_scheme* sc, const char* gf_lib, const string& command, const json& tool_config,
                               const char*& errmsg, s7_pointer old_port, int gc_loc, bool allow_fallback) {
  gfproject_tool_prepare_result prepared= goldfish_prepare_tool_main (sc, gf_lib, command, tool_config);
  if (prepared.error != gfproject_tool_prepare_error::none) {
    if (allow_fallback) {
      goldfish_reset_captured_error_port (sc);
      return -1;
    }

    bool include_scheme_error= prepared.error == gfproject_tool_prepare_error::import_failed;
    return goldfish_finish_tool_error (sc, prepared.message, errmsg, old_port, gc_loc, include_scheme_error);
  }

  s7_pointer result= s7_call (sc, prepared.main_func, s7_nil (sc));
  return goldfish_finish_tool_success (sc, result, errmsg, old_port, gc_loc);
}

static s7_pointer
f_gfproject_load_config (s7_scheme* sc, s7_pointer args) {
  (void) args;
  string gf_lib_dir= find_goldfish_library ();
  json   config    = load_gfproject_config (gf_lib_dir.c_str ());
  return s7_make_string (sc, config.dump ().c_str ());
}

static s7_pointer
f_project_root (s7_scheme* sc, s7_pointer args) {
  (void) args;
  fs::path local= find_local_gfproject_json ();
  if (local.empty ()) {
    return s7_f (sc);
  }
  // local points at gfproject.json; its parent is the project root.
  std::string s= local.parent_path ().generic_string ();
  if (s.empty ()) {
    return s7_f (sc);
  }
  return s7_make_string (sc, s.c_str ());
}

static int
goldfish_run_tool (s7_scheme* sc, const char* gf_lib, const string& command, const char*& errmsg, s7_pointer old_port,
                   int gc_loc) {
  gfproject_tool_resolution resolved= resolve_gfproject_tool (gf_lib, command);
  if (!resolved.has_merged_tool) {
    return -1;
  }

  bool allow_builtin_fallback= command == "help" || command == "version" || command == "eval" || command == "load" ||
                               command == "repl" || command == "run";

  if (resolved.has_local_override && resolved.has_lib_tool) {
    int merged_ret=
        goldfish_run_tool_with_config (sc, gf_lib, command, resolved.merged_tool, errmsg, old_port, gc_loc, true);
    if (merged_ret != -1) {
      return merged_ret;
    }
    return goldfish_run_tool_with_config (sc, gf_lib, command, resolved.lib_tool, errmsg, old_port, gc_loc,
                                          allow_builtin_fallback);
  }

  return goldfish_run_tool_with_config (sc, gf_lib, command, resolved.merged_tool, errmsg, old_port, gc_loc,
                                        allow_builtin_fallback);
}

static string
find_tool_root_by_command (const char* gf_lib, const string& command) {
  std::error_code  ec;
  fs::path         cwd= fs::current_path (ec);
  vector<fs::path> candidates;
  if (!ec) {
    candidates.push_back (cwd / "tools" / command);
  }
  candidates.push_back (fs::path (gf_lib) / "tools" / command);
  candidates.push_back (fs::path (gf_lib).parent_path () / "tools" / command);

  for (const auto& candidate : candidates) {
    if (fs::is_directory (candidate, ec)) {
      return candidate.string ();
    }
    ec.clear ();
  }
  return "";
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
    s7_eval_c_string (sc, "(import (scheme base) (liii base) (liii error) (liii string))");
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
    ic_printf ("[b]Mode:[/] [b]%s[/] (additionally imports: (scheme base) (liii base) (liii error) (liii string) "
               "compared to r7rs)\n\n",
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
  return arg == "--help" || arg == "-h" || arg == "--version" || arg == "-v" || arg == "-e";
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
  return std::all_of (value.begin (), value.end (),
                      [] (unsigned char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'); });
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

  for (fs::directory_iterator it (root, fs::directory_options::skip_permission_denied, ec), end; it != end;
       it.increment (ec)) {
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
    } catch (const std::exception&) {
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

  if (s7_is_list (sc, export_spec) && (s7_list_length (sc, export_spec) == 3) &&
      is_named_symbol (s7_car (export_spec), "rename")) {
    string renamed_export;
    if (library_name_part_to_string (s7_caddr (export_spec), renamed_export)) {
      return function_name == renamed_export;
    }
  }

  return false;
}

static bool
define_library_form_exports_function (s7_scheme* sc, s7_pointer form, const string& function_name, string& group,
                                      string& library) {
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
    if ((!s7_is_list (sc, declaration)) || s7_is_null (sc, declaration) ||
        (!is_named_symbol (s7_car (declaration), "export"))) {
      continue;
    }

    for (s7_pointer export_specs= s7_cdr (declaration); s7_is_pair (export_specs);
         export_specs           = s7_cdr (export_specs)) {
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
source_file_exports_function (s7_scheme* sc, const fs::path& source_file, const string& function_name, string& group,
                              string& library) {
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

  for (fs::directory_iterator it (root, fs::directory_options::skip_permission_denied, ec), end; it != end;
       it.increment (ec)) {
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

  for (fs::directory_iterator it (dir, fs::directory_options::skip_permission_denied, ec), end; it != end;
       it.increment (ec)) {
    if (ec) {
      ec.clear ();
      continue;
    }
    if (it->is_regular_file (ec) && (it->path ().extension () == ".scm")) {
      files.push_back (it->path ());
    }
    ec.clear ();
  }

  std::sort (files.begin (), files.end (),
             [] (const fs::path& lhs, const fs::path& rhs) { return lhs.string () < rhs.string (); });
  return files;
}

static vector<string>
find_function_libraries_in_load_path (s7_scheme* sc, const string& function_name) {
  vector<string>  library_queries;
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
    return s7_error (
        sc, s7_make_symbol (sc, "type-error"),
        s7_list (sc, 2, s7_make_string (sc, "g_function-libraries: function-name must be string?"), function_name_arg));
  }

  string         function_name= s7_string (function_name_arg);
  vector<string> visible_library_queries;

  try {
    visible_library_queries= find_function_libraries_in_load_path (sc, function_name);
  } catch (const std::exception& ex) {
    return s7_error (
        sc, s7_make_symbol (sc, "read-error"),
        s7_list (
            sc, 2,
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

  // 如果没有找到命令或没有参数，使用 help 命令
  if (argc <= 1 || command.empty ()) {
    command= "help";
  }
  if (command == "-e") {
    command= "eval";
    if (command_index >= 0 && command_index < static_cast<int> (command_args.size ())) {
      command_args[command_index]= "eval";
    }
  }

  // 自动路由：如果参数是目录且第一级文件夹是 tests，自动视为 test 命令
  if (!command.empty () && command != "help" && command != "version" && command != "eval" && command != "load" &&
      command != "repl" && command != "run" && command != "test" && command != "-e") {
    std::error_code ec;
    if (fs::is_directory (command, ec)) {
      fs::path p (command);
      auto     it= p.begin ();
      if (it != p.end () && *it == "tests") {
        if (command_index >= 0 && command_index <= static_cast<int> (command_args.size ())) {
          command_args.insert (command_args.begin () + command_index, "test");
        }
        command= "test";
        std::cerr << "[gf] Auto-routing: detected tests directory, routing to test command" << "\n";
        std::cerr << "[gf] Executing: ";
        for (size_t i= 0; i < command_args.size (); ++i) {
          if (i > 0) std::cerr << " ";
          std::cerr << command_args[i];
        }
        std::cerr << std::endl;
      }
    }
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

  apply_startup_load_path_options (sc, startup_opts);
  customize_goldfish_by_mode (sc, mode, gf_boot);

  // start capture error output
  const char* errmsg  = NULL;
  s7_pointer  old_port= s7_set_current_error_port (sc, s7_open_output_string (sc));
  int         gc_loc  = -1;
  if (old_port != s7_nil (sc)) gc_loc= s7_gc_protect (sc, old_port);

  // 处理动态注册的工具（从 gfproject.json 加载）
  json gfproject_config= load_gfproject_config (gf_lib);
  if (gfproject_config.contains ("tools") && gfproject_config["tools"].contains (command)) {
    int tool_ret= goldfish_run_tool (sc, gf_lib, command, errmsg, old_port, gc_loc);
    if (tool_ret != -1) {
      // Tool was found and executed (or failed with an error)
      return tool_ret;
    }
    // If tool_ret == -1, tool config exists but execution failed, fall through to check other commands
  }

  if (command == "help") {
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    display_help ();
    return 0;
  }

  if (command == "version") {
    s7_close_output_port (sc, s7_current_error_port (sc));
    s7_set_current_error_port (sc, old_port);
    if (gc_loc != -1) s7_gc_unprotect_at (sc, gc_loc);
    display_version ();
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
