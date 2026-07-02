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
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "s7.h"
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <tbox/platform/file.h>
#include <tbox/platform/path.h>
#include <tbox/tbox.h>

#ifdef TB_CONFIG_OS_WINDOWS
#include <windows.h>
#elif TB_CONFIG_OS_MACOSX
#include <limits.h>
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

#define GOLDFISH_PATH_MAXN TB_PATH_MAXN

namespace goldfish {

using std::string;
using std::vector;

namespace fs= std::filesystem;

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional) {
  s7_pointer cur_env= s7_curlet (sc);
  s7_pointer func   = s7_make_typed_function (sc, name, f, required, optional, false, desc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
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

static s7_pointer
f_os_arch (s7_scheme* sc, s7_pointer args) {
  return s7_make_string (sc, TB_ARCH_STRING);
}

static void
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

static void
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

static void
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

static void
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

static void
glue_access (s7_scheme* sc) {
  const char* name= "g_access";
  const char* desc= "(g_access string integer) => boolean, check file access permissions";
  glue_define (sc, name, desc, f_access, 2, 0);
}

static s7_pointer
f_set_environment_variable (s7_scheme* sc, s7_pointer args) {
  const char* key  = s7_string (s7_car (args));
  const char* value= s7_string (s7_cadr (args));
  return s7_make_boolean (sc, tb_environment_set (key, value));
}

static void
glue_setenv (s7_scheme* sc) {
  const char* name= "g_setenv";
  const char* desc= "(g_setenv key value) => boolean, set an environment variable";
  glue_define (sc, name, desc, f_set_environment_variable, 2, 0);
}

static s7_pointer
f_unset_environment_variable (s7_scheme* sc, s7_pointer args) {
  const char* env_name= s7_string (s7_car (args));
  return s7_make_boolean (sc, tb_environment_remove (env_name));
}

static void
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

static void
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

static void
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

static void
glue_rmdir (s7_scheme* sc) {
  const char* name= "g_rmdir";
  const char* desc= "(g_rmdir string) => boolean, remove a directory";
  glue_define (sc, name, desc, f_rmdir, 1, 0);
}

static s7_pointer
f_remove_file (s7_scheme* sc, s7_pointer args) {
  const char* path   = s7_string (s7_car (args));
  bool        success= tb_file_remove (path);
  return s7_make_boolean (sc, success);
}

static void
glue_remove_file (s7_scheme* sc) {
  const char* name= "g_remove-file";
  const char* desc= "(g_remove-file path) => boolean, delete a file";
  glue_define (sc, name, desc, f_remove_file, 1, 0);
}

static s7_pointer
f_rename (s7_scheme* sc, s7_pointer args) {
  const char* src= s7_string (s7_car (args));
  const char* dst= s7_string (s7_cadr (args));
  try {
    fs::rename (src, dst);
    return s7_make_boolean (sc, true);
  } catch (const fs::filesystem_error& e) {
    return s7_make_boolean (sc, false);
  }
}

static void
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

static void
glue_chdir (s7_scheme* sc) {
  const char* name= "g_chdir";
  const char* desc= "(g_chdir string) => boolean, change the current working directory";
  glue_define (sc, name, desc, f_chdir, 1, 0);
}

static tb_long_t
tb_directory_walk_func (tb_char_t const* path, tb_file_info_t const* info, tb_cpointer_t priv) {
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

static void
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

static void
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

static void
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

static void
glue_getpid (s7_scheme* sc) {
  const char* name= "g_getpid";
  const char* desc= "(g_getpid) => integer";
  glue_define (sc, name, desc, f_getpid, 0, 0);
}

void
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

} // namespace goldfish
