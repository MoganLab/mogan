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
#include <cstdio>
#include <cstring>
#include <string>
#include <tbox/tbox.h>
#include <vector>

#if !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(__EMSCRIPTEN__)
#include <wordexp.h>
#endif

namespace goldfish {

using std::string;
using std::vector;

enum class redirect_mode { tee, capture, inherit, discard, file };

s7_pointer
f_subprocess_run_values (s7_scheme* sc, s7_pointer args) {
  s7_pointer cmd_arg= s7_car (args);
  args              = s7_cdr (args);

  const char* cwd= nullptr;
  if (s7_is_pair (args) && s7_is_string (s7_car (args))) {
    cwd = s7_string (s7_car (args));
    args= s7_cdr (args);
  }
  else if (s7_is_pair (args)) {
    args= s7_cdr (args);
  }

  vector<string>      env_storage;
  vector<const char*> envp;
  if (s7_is_pair (args) && s7_is_pair (s7_car (args))) {
    s7_pointer env_alist= s7_car (args);
    while (s7_is_pair (env_alist)) {
      s7_pointer item= s7_car (env_alist);
      if (s7_is_pair (item)) {
        const char* key  = s7_string (s7_car (item));
        s7_pointer  val  = s7_cdr (item);
        const char* val_c= s7_is_string (val) ? s7_string (val) : "";
        env_storage.push_back (string (key) + "=" + val_c);
      }
      env_alist= s7_cdr (env_alist);
    }
    for (auto& s : env_storage) {
      envp.push_back (s.c_str ());
    }
    envp.push_back (nullptr);
    args= s7_cdr (args);
  }
  else if (s7_is_pair (args)) {
    args= s7_cdr (args);
  }

  const char* input    = nullptr;
  size_t      input_len= 0;
  if (s7_is_pair (args) && s7_is_string (s7_car (args))) {
    input    = s7_string (s7_car (args));
    input_len= strlen (input);
    args     = s7_cdr (args);
  }
  else if (s7_is_pair (args)) {
    args= s7_cdr (args);
  }

  tb_long_t timeout_ms= -1;
  if (s7_is_pair (args) && s7_is_integer (s7_car (args))) {
    timeout_ms= s7_integer (s7_car (args)) * 1000;
    args      = s7_cdr (args);
  }
  else if (s7_is_pair (args)) {
    args= s7_cdr (args);
  }

  redirect_mode stdout_mode= redirect_mode::inherit;
  const char*   stdout_path= nullptr;
  if (s7_is_pair (args)) {
    s7_pointer stdout_val= s7_car (args);
    if (s7_is_symbol (stdout_val)) {
      const char* sym= s7_symbol_name (stdout_val);
      if (strcmp (sym, "capture") == 0) {
        stdout_mode= redirect_mode::capture;
      }
      else if (strcmp (sym, "discard") == 0) {
        stdout_mode= redirect_mode::discard;
      }
      else if (strcmp (sym, "inherit") == 0) {
        stdout_mode= redirect_mode::inherit;
      }
    }
    else if (s7_is_string (stdout_val)) {
      stdout_mode= redirect_mode::file;
      stdout_path= s7_string (stdout_val);
    }
    args= s7_cdr (args);
  }

  bool stdout_append= false;
  if (s7_is_pair (args)) {
    s7_pointer stdout_mode_val= s7_car (args);
    if (s7_is_symbol (stdout_mode_val) && strcmp (s7_symbol_name (stdout_mode_val), "append") == 0) {
      stdout_append= true;
    }
    args= s7_cdr (args);
  }

  redirect_mode stderr_mode     = redirect_mode::inherit;
  bool          stderr_to_stdout= false;
  const char*   stderr_path     = nullptr;
  if (s7_is_pair (args)) {
    s7_pointer stderr_val= s7_car (args);
    if (s7_is_symbol (stderr_val)) {
      const char* sym= s7_symbol_name (stderr_val);
      if (strcmp (sym, "stdout") == 0) {
        stderr_to_stdout= true;
      }
      else if (strcmp (sym, "capture") == 0) {
        stderr_mode= redirect_mode::capture;
      }
      else if (strcmp (sym, "discard") == 0) {
        stderr_mode= redirect_mode::discard;
      }
      else if (strcmp (sym, "inherit") == 0) {
        stderr_mode= redirect_mode::inherit;
      }
    }
    else if (s7_is_string (stderr_val)) {
      stderr_mode= redirect_mode::file;
      stderr_path= s7_string (stderr_val);
    }
    args= s7_cdr (args);
  }

  bool stderr_append= false;
  if (s7_is_pair (args)) {
    s7_pointer stderr_mode_val= s7_car (args);
    if (s7_is_symbol (stderr_mode_val) && strcmp (s7_symbol_name (stderr_mode_val), "append") == 0) {
      stderr_append= true;
    }
    args= s7_cdr (args);
  }

  const char* stdin_path= nullptr;
  bool        stdin_null= false;
  if (s7_is_pair (args)) {
    s7_pointer stdin_val= s7_car (args);
    if (s7_is_symbol (stdin_val) && strcmp (s7_symbol_name (stdin_val), "null") == 0) {
      stdin_null= true;
    }
    else if (s7_is_string (stdin_val)) {
      stdin_path= s7_string (stdin_val);
    }
  }

  tb_process_attr_t attr= {tb_null};
  attr.flags            = TB_PROCESS_FLAG_NO_WINDOW;

  if (cwd) attr.curdir= cwd;
  if (!envp.empty ()) attr.envp= (tb_char_t const**) envp.data ();

  bool need_stdout_pipe=
      (stdout_mode == redirect_mode::tee || stdout_mode == redirect_mode::capture) ||
      (stderr_to_stdout && stdout_mode != redirect_mode::file && stdout_mode != redirect_mode::discard);

  tb_pipe_file_ref_t out_pipe[2]= {tb_null};
  if (stdout_mode == redirect_mode::file) {
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.out.path= stdout_path;
    attr.outmode = TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | (stdout_append ? TB_FILE_MODE_APPEND : TB_FILE_MODE_TRUNC);
  }
  else if (stdout_mode == redirect_mode::discard) {
    attr.outtype= TB_PROCESS_REDIRECT_TYPE_FILEPATH;
#ifdef TB_CONFIG_OS_WINDOWS
    attr.out.path= "NUL";
#else
    attr.out.path= "/dev/null";
#endif
    attr.outmode= TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC;
  }
  else if (need_stdout_pipe) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO};
    tb_pipe_file_init_pair (out_pipe, mode, 0);
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.out.pipe= out_pipe[1];
  }

  tb_pipe_file_ref_t err_pipe[2]= {tb_null};
  if (stderr_mode == redirect_mode::file) {
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.err.path= stderr_path;
    attr.errmode = TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | (stderr_append ? TB_FILE_MODE_APPEND : TB_FILE_MODE_TRUNC);
  }
  else if (stderr_mode == redirect_mode::discard) {
    attr.errtype= TB_PROCESS_REDIRECT_TYPE_FILEPATH;
#ifdef TB_CONFIG_OS_WINDOWS
    attr.err.path= "NUL";
#else
    attr.err.path= "/dev/null";
#endif
    attr.errmode= TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC;
  }
  else if (stderr_to_stdout && out_pipe[1]) {
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.err.pipe= out_pipe[1];
  }
  else if (stderr_mode == redirect_mode::tee || stderr_mode == redirect_mode::capture) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO};
    tb_pipe_file_init_pair (err_pipe, mode, 0);
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.err.pipe= err_pipe[1];
  }

  tb_pipe_file_ref_t in_pipe[2]= {tb_null};
  if (stdin_path) {
    attr.intype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.in.path= stdin_path;
    attr.inmode = TB_FILE_MODE_RO;
  }
  else if (stdin_null) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO};
    tb_pipe_file_init_pair (in_pipe, mode, 0);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.in.pipe= in_pipe[0];
    tb_pipe_file_exit (in_pipe[1]);
  }
  else if (input) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO};
    tb_pipe_file_init_pair (in_pipe, mode, 0);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.in.pipe= in_pipe[0];
    tb_pipe_file_write (in_pipe[1], (tb_byte_t*) input, input_len);
    tb_pipe_file_exit (in_pipe[1]);
  }

  tb_process_ref_t process= tb_null;
  if (s7_is_string (cmd_arg)) {
    const char* cmd_c= s7_string (cmd_arg);
#ifdef TB_CONFIG_OS_WINDOWS
    process= tb_process_init_cmd (cmd_c, &attr);
#elif !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(__EMSCRIPTEN__)
    wordexp_t p;
    int       ret= wordexp (cmd_c, &p, 0);
    if (ret == 0 && p.we_wordc > 0) {
      process= tb_process_init (p.we_wordv[0], (tb_char_t const**) p.we_wordv, &attr);
      wordfree (&p);
    }
#else
    process= tb_process_init_cmd (cmd_c, &attr);
#endif
  }
  else if (s7_is_pair (cmd_arg)) {
    vector<const char*> argv;
    s7_pointer          p= cmd_arg;
    while (s7_is_pair (p)) {
      s7_pointer item= s7_car (p);
      if (s7_is_string (item)) {
        argv.push_back (s7_string (item));
      }
      p= s7_cdr (p);
    }
    argv.push_back (nullptr);
    if (!argv.empty ()) {
      process= tb_process_init (argv[0], argv.data (), &attr);
    }
  }

  if (out_pipe[1]) tb_pipe_file_exit (out_pipe[1]);
  if (err_pipe[1]) tb_pipe_file_exit (err_pipe[1]);

  string    stdout_str;
  string    stderr_str;
  tb_long_t status= -1;

  if (process) {
    tb_long_t wait_result= tb_process_wait (process, &status, timeout_ms);

    if (wait_result == 0) {
      tb_process_kill (process);
      tb_process_wait (process, &status, -1);
      status= -1;
    }

    if (out_pipe[0]) {
      char      buf[4096];
      tb_long_t n;
      while ((n= tb_pipe_file_read (out_pipe[0], (tb_byte_t*) buf, sizeof (buf) - 1)) > 0) {
        buf[n]= '\0';
        stdout_str.append (buf);
        if (stdout_mode == redirect_mode::tee) {
          fwrite (buf, 1, n, stdout);
          fflush (stdout);
        }
      }
      tb_pipe_file_exit (out_pipe[0]);
    }

    if (err_pipe[0]) {
      char      buf[4096];
      tb_long_t n;
      while ((n= tb_pipe_file_read (err_pipe[0], (tb_byte_t*) buf, sizeof (buf) - 1)) > 0) {
        buf[n]= '\0';
        stderr_str.append (buf);
        if (stderr_mode == redirect_mode::tee) {
          fwrite (buf, 1, n, stderr);
          fflush (stderr);
        }
      }
      tb_pipe_file_exit (err_pipe[0]);
    }

    tb_process_exit (process);
  }

  s7_pointer out_s7 = s7_make_string (sc, stdout_str.c_str ());
  s7_pointer err_s7 = s7_make_string (sc, stderr_str.c_str ());
  s7_pointer code_s7= s7_make_integer (sc, (s7_int) status);
  return s7_values (sc, s7_cons (sc, out_s7, s7_cons (sc, err_s7, s7_cons (sc, code_s7, s7_nil (sc)))));
}

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional) {
  s7_pointer cur_env= s7_curlet (sc);
  s7_pointer func   = s7_make_typed_function (sc, name, f, required, optional, false, desc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

void
glue_subprocess_run_values (s7_scheme* sc) {
  const char* name= "g_subprocess-run-values";
  const char* desc= "(g_subprocess-run-values command cwd env input timeout stdout stdout-mode stderr stderr-mode "
                    "stdin) => (values stdout stderr exit-code)";
  glue_define (sc, name, desc, f_subprocess_run_values, 1, 9);
}

} // namespace goldfish
