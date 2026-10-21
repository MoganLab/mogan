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

#include "liii_subprocess.hpp"
#include "s7.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <tbox/tbox.h>
#include <utility>
#include <vector>

#ifdef TB_CONFIG_OS_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef tb_pipefile2fd
#define tb_pipefile2fd(file) (tb_int_t) ((file) ? (((tb_long_t) (file)) - 1) : -1)
#endif

static void
set_pipe_cloexec (tb_pipe_file_ref_t p) {
  if (p) {
    int fd= tb_pipefile2fd (p);
    if (fd >= 0) {
      fcntl (fd, F_SETFD, FD_CLOEXEC);
    }
  }
}

static void
set_pipe_pair_cloexec (tb_pipe_file_ref_t pipe[2]) {
  set_pipe_cloexec (pipe[0]);
  set_pipe_cloexec (pipe[1]);
}
#endif

#if !defined(TB_CONFIG_OS_WINDOWS) && !defined(__EMSCRIPTEN__)
#include <wordexp.h>
#endif

namespace goldfish {

using std::string;
using std::to_string;
using std::vector;

enum class redirect_mode { tee, capture, inherit, discard, file };

static s7_int g_process_type_tag= 0;

inline s7_pointer
subprocess_type_error (s7_scheme* sc, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, "type-error"), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

#ifdef TB_CONFIG_OS_WINDOWS
static tb_int_t
get_process_pid (tb_process_ref_t process) {
  if (!process) return -1;
  struct win_proc {
    STARTUPINFOEX       si;
    STARTUPINFO*        psi;
    PROCESS_INFORMATION pi;
  };
  return (tb_int_t) (((win_proc*) process)->pi.dwProcessId);
}

static HANDLE
create_process_job () {
  HANDLE h_job= CreateJobObject (NULL, NULL);
  if (h_job) {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
    memset (&jeli, 0, sizeof (jeli));
    jeli.BasicLimitInformation.LimitFlags= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject (h_job, JobObjectExtendedLimitInformation, &jeli, sizeof (jeli));
  }
  return h_job;
}

static void
assign_process_to_job (HANDLE h_job, tb_process_ref_t process) {
  if (h_job && process) {
    struct win_proc {
      STARTUPINFOEX       si;
      STARTUPINFO*        psi;
      PROCESS_INFORMATION pi;
    };
    HANDLE h_proc= ((win_proc*) process)->pi.hProcess;
    if (h_proc && h_proc != INVALID_HANDLE_VALUE) {
      AssignProcessToJobObject (h_job, h_proc);
    }
  }
}
#else
extern "C" tb_int_t tb_process_pid (tb_process_ref_t self);
static tb_int_t
get_process_pid (tb_process_ref_t process) {
  if (!process) return -1;
  return tb_process_pid (process);
}
#endif

struct Subprocess {
  tb_process_ref_t process  = tb_null;
  int              pid      = -1;
  bool             exited   = false;
  tb_long_t        exit_code= -1;
  bool             closed   = false;

#ifdef TB_CONFIG_OS_WINDOWS
  HANDLE h_in_write= NULL;
  HANDLE h_out_read= NULL;
  HANDLE h_err_read= NULL;
  HANDLE h_job     = NULL;
#else
  tb_pipe_file_ref_t in_pipe = tb_null;
  tb_pipe_file_ref_t out_pipe= tb_null;
  tb_pipe_file_ref_t err_pipe= tb_null;
#endif

  string stdout_buffer;
  string stderr_buffer;
  bool   stdout_eof= false;
  bool   stderr_eof= false;

  tb_hong_t last_activity_time= 0;

  void update_activity () { last_activity_time= tb_mclock (); }

  double get_last_activity_time_sec () const { return (double) last_activity_time / 1000.0; }

  double get_idle_seconds () const {
    tb_hong_t now= tb_mclock ();
    if (now > last_activity_time) {
      return (double) (now - last_activity_time) / 1000.0;
    }
    return 0.0;
  }

  ~Subprocess () { close_all (); }

  void close_all () {
    close_pipes ();
    if (process) {
      if (!exited) {
#ifdef TB_CONFIG_OS_WINDOWS
        if (h_job) {
          TerminateJobObject (h_job, 1);
        }
        tb_process_kill (process);
        if (pid > 0) {
          string cmd= "taskkill /F /T /PID " + std::to_string (pid) + " >NUL 2>&1";
          system (cmd.c_str ());
        }
#else
        if (pid > 1) {
          kill (-pid, SIGKILL);
          kill (pid, SIGKILL);
        }
        else if (pid > 0) {
          kill (pid, SIGKILL);
        }
        else {
          tb_process_kill (process);
        }
#endif
        tb_process_wait (process, &exit_code, -1);
        exited= true;
      }
      tb_process_exit (process);
      process= tb_null;
    }
#ifdef TB_CONFIG_OS_WINDOWS
    if (h_job) {
      CloseHandle (h_job);
      h_job= NULL;
    }
#endif
  }

  void close_pipes () {
    close_stdin ();
    close_stdout ();
    close_stderr ();
    closed= true;
  }

  void close_stdin () {
#ifdef TB_CONFIG_OS_WINDOWS
    if (h_in_write) {
      CloseHandle (h_in_write);
      h_in_write= NULL;
    }
#else
    if (in_pipe) {
      tb_pipe_file_exit (in_pipe);
      in_pipe= tb_null;
    }
#endif
  }

  void close_stdout () {
    stdout_eof= true;
#ifdef TB_CONFIG_OS_WINDOWS
    if (h_out_read) {
      CloseHandle (h_out_read);
      h_out_read= NULL;
    }
#else
    if (out_pipe) {
      tb_pipe_file_exit (out_pipe);
      out_pipe= tb_null;
    }
#endif
  }

  void close_stderr () {
    stderr_eof= true;
#ifdef TB_CONFIG_OS_WINDOWS
    if (h_err_read) {
      CloseHandle (h_err_read);
      h_err_read= NULL;
    }
#else
    if (err_pipe) {
      tb_pipe_file_exit (err_pipe);
      err_pipe= tb_null;
    }
#endif
  }

  bool update_status () {
    if (exited) return true;
    if (!process) return false;
    tb_long_t status= -1;
    tb_long_t r     = tb_process_wait (process, &status, 0);
    if (r > 0) {
      exited   = true;
      exit_code= status;
      return true;
    }
    return false;
  }

  bool alive () {
    if (exited) return false;
    update_status ();
    return !exited;
  }

  int poll () {
    if (exited) return 1;
    if (update_status ()) return 1;
    return 0;
  }

  int wait (tb_long_t timeout_ms) {
    if (exited) return 1;
    if (!process) return -1;
    tb_long_t status= -1;
    tb_long_t r     = tb_process_wait (process, &status, timeout_ms);
    if (r > 0) {
      exited   = true;
      exit_code= status;
      return 1;
    }
    else if (r == 0) {
      return 0;
    }
    else {
      return -1;
    }
  }

  void terminate () {
    if (exited || !process) return;
#ifdef TB_CONFIG_OS_WINDOWS
    tb_process_kill (process);
#else
    if (pid > 1) {
      kill (-pid, SIGTERM);
      kill (pid, SIGTERM);
    }
    else if (pid > 0) {
      kill (pid, SIGTERM);
    }
#endif
  }

  void kill_proc () {
    if (exited || !process) return;
#ifdef TB_CONFIG_OS_WINDOWS
    if (h_job) {
      TerminateJobObject (h_job, 1);
    }
    tb_process_kill (process);
    if (pid > 0) {
      string cmd= "taskkill /F /T /PID " + std::to_string (pid) + " >NUL 2>&1";
      system (cmd.c_str ());
    }
#else
    if (pid > 1) {
      kill (-pid, SIGKILL);
      kill (pid, SIGKILL);
    }
    else if (pid > 0) {
      kill (pid, SIGKILL);
    }
    else {
      tb_process_kill (process);
    }
#endif
  }

  size_t write_stdin (const char* data, size_t len) {
#ifdef TB_CONFIG_OS_WINDOWS
    if (!h_in_write) return 0;
    size_t total  = 0;
    DWORD  written= 0;
    while (len > 0) {
      if (!WriteFile (h_in_write, data, (DWORD) len, &written, NULL) || written == 0) break;
      data+= written;
      len-= written;
      total+= written;
    }
    return total;
#else
    if (!in_pipe) return 0;
    int    fd   = tb_pipefile2fd (in_pipe);
    size_t total= 0;
    while (len > 0) {
      ssize_t n= write (fd, data, len);
      if (n <= 0) {
        if (errno == EINTR) continue;
        break;
      }
      data+= n;
      len-= n;
      total+= n;
    }
    return total;
#endif
  }

  int fill_buffer (bool is_err, int timeout_ms= 100) {
    string& buf= is_err ? stderr_buffer : stdout_buffer;
    bool&   eof= is_err ? stderr_eof : stdout_eof;
    if (eof) return 0;

#ifdef TB_CONFIG_OS_WINDOWS
    HANDLE h= is_err ? h_err_read : h_out_read;
    if (!h) {
      eof= true;
      return 0;
    }
    DWORD avail= 0;
    if (!PeekNamedPipe (h, NULL, 0, NULL, &avail, NULL)) {
      eof= true;
      return 0;
    }
    if (avail > 0) {
      char  tmp[65536];
      DWORD to_read= (std::min) ((DWORD) sizeof (tmp), avail);
      DWORD n      = 0;
      if (ReadFile (h, tmp, to_read, &n, NULL) && n > 0) {
        buf.append (tmp, n);
        update_activity ();
        return (int) n;
      }
      else {
        eof= true;
        return 0;
      }
    }
    else {
      if (update_status ()) {
        eof= true;
        return 0;
      }
      return -1;
    }
#else
    tb_pipe_file_ref_t p= is_err ? err_pipe : out_pipe;
    if (!p) {
      eof= true;
      return 0;
    }
    int           fd= tb_pipefile2fd (p);
    struct pollfd pfd;
    pfd.fd     = fd;
    pfd.events = POLLIN;
    pfd.revents= 0;
    int pret   = ::poll (&pfd, 1, timeout_ms);
    if (pret > 0) {
      char    tmp[65536];
      ssize_t n= read (fd, tmp, sizeof (tmp));
      if (n > 0) {
        buf.append (tmp, n);
        update_activity ();
        return (int) n;
      }
      else if (n == 0) {
        eof= true;
        return 0;
      }
      else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return -1;
      }
      else {
        eof= true;
        return 0;
      }
    }
    else if (pret == 0) {
      if (update_status ()) {
        char tmp[65536];
        fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK);
        ssize_t n= read (fd, tmp, sizeof (tmp));
        if (n > 0) {
          buf.append (tmp, n);
          update_activity ();
          return (int) n;
        }
        eof= true;
        return 0;
      }
      return -1;
    }
    else {
      if (errno == EINTR) return -1;
      eof= true;
      return 0;
    }
#endif
  }

  std::pair<string, bool> read_chunk (bool is_err, size_t max_bytes) {
    string& buf= is_err ? stderr_buffer : stdout_buffer;
    bool&   eof= is_err ? stderr_eof : stdout_eof;

    while (buf.empty () && !eof) {
      int r= fill_buffer (is_err, 200);
      if (r == 0) break;
      if (r < 0) {
        if (update_status ()) {
          int r2= fill_buffer (is_err, 50);
          if (r2 <= 0) {
            eof= true;
            break;
          }
        }
      }
    }

    if (!buf.empty ()) {
      size_t take= (std::min) (max_bytes, buf.size ());
      string res = buf.substr (0, take);
      buf.erase (0, take);
      update_activity ();
      return {res, false};
    }
    return {"", true};
  }

  std::pair<string, bool> read_line (bool is_err) {
    string& buf= is_err ? stderr_buffer : stdout_buffer;
    bool&   eof= is_err ? stderr_eof : stdout_eof;

    while (true) {
      size_t pos= buf.find ('\n');
      if (pos != string::npos) {
        string line= buf.substr (0, pos);
        if (!line.empty () && line.back () == '\r') {
          line.pop_back ();
        }
        buf.erase (0, pos + 1);
        update_activity ();
        return {line, false};
      }
      if (eof) {
        if (!buf.empty ()) {
          string line= buf;
          if (!line.empty () && line.back () == '\r') line.pop_back ();
          buf.clear ();
          update_activity ();
          return {line, false};
        }
        return {"", true};
      }
      int r= fill_buffer (is_err, 200);
      if (r == 0) {
        if (!buf.empty ()) {
          string line= buf;
          if (!line.empty () && line.back () == '\r') line.pop_back ();
          buf.clear ();
          update_activity ();
          return {line, false};
        }
        return {"", true};
      }
    }
  }

  bool is_eof (bool is_err) {
    string& buf= is_err ? stderr_buffer : stdout_buffer;
    bool&   eof= is_err ? stderr_eof : stdout_eof;
    return eof && buf.empty ();
  }

  int poll_io (int timeout_ms) {
    if (is_eof (false) && is_eof (true)) return -1;
    if (!stdout_buffer.empty () && !stderr_buffer.empty ()) return 3;
    if (!stdout_buffer.empty ()) return 1;
    if (!stderr_buffer.empty ()) return 2;

#ifdef TB_CONFIG_OS_WINDOWS
    bool out_avail= false, err_avail= false;
    if (!stdout_eof && h_out_read) {
      DWORD avail= 0;
      if (PeekNamedPipe (h_out_read, NULL, 0, NULL, &avail, NULL)) {
        if (avail > 0) out_avail= true;
      }
      else {
        stdout_eof= true;
      }
    }
    if (!stderr_eof && h_err_read) {
      DWORD avail= 0;
      if (PeekNamedPipe (h_err_read, NULL, 0, NULL, &avail, NULL)) {
        if (avail > 0) err_avail= true;
      }
      else {
        stderr_eof= true;
      }
    }
    if (out_avail || err_avail) update_activity ();
    if (out_avail && err_avail) return 3;
    if (out_avail) return 1;
    if (err_avail) return 2;
    if (stdout_eof && stderr_eof) return -1;
    return 0;
#else
    struct pollfd pfds[2];
    int           nfds   = 0;
    int           out_idx= -1, err_idx= -1;
    if (!stdout_eof && out_pipe) {
      out_idx              = nfds++;
      pfds[out_idx].fd     = tb_pipefile2fd (out_pipe);
      pfds[out_idx].events = POLLIN;
      pfds[out_idx].revents= 0;
    }
    if (!stderr_eof && err_pipe) {
      err_idx              = nfds++;
      pfds[err_idx].fd     = tb_pipefile2fd (err_pipe);
      pfds[err_idx].events = POLLIN;
      pfds[err_idx].revents= 0;
    }
    if (nfds == 0) return -1;

    int pret= ::poll (pfds, nfds, timeout_ms);
    if (pret > 0) {
      bool out_ready= (out_idx >= 0 && (pfds[out_idx].revents & (POLLIN | POLLHUP | POLLERR)));
      bool err_ready= (err_idx >= 0 && (pfds[err_idx].revents & (POLLIN | POLLHUP | POLLERR)));
      bool out_in   = (out_idx >= 0 && (pfds[out_idx].revents & POLLIN));
      bool err_in   = (err_idx >= 0 && (pfds[err_idx].revents & POLLIN));
      if (out_in || err_in) {
        update_activity ();
      }
      if (out_ready && err_ready) return 3;
      if (out_ready) return 1;
      if (err_ready) return 2;
    }
    if (pret == 0 && update_status ()) {
      if (stdout_eof && stderr_eof) return -1;
    }
    return 0;
#endif
  }
};

static inline bool
is_goldfish_process (s7_scheme* sc, s7_pointer obj) {
  return s7_is_c_object (obj) && s7_c_object_type (obj) == g_process_type_tag;
}

static inline Subprocess*
get_goldfish_process (s7_scheme* sc, s7_pointer obj) {
  return static_cast<Subprocess*> (s7_c_object_value (obj));
}

static void
process_free_c_value (void* val) {
  if (val != nullptr) {
    auto* proc= static_cast<Subprocess*> (val);
    delete proc;
  }
}

static s7_pointer
process_to_string_glue (s7_scheme* sc, s7_pointer args) {
  s7_pointer         self= s7_car (args);
  auto*              proc= static_cast<Subprocess*> (s7_c_object_value (self));
  std::ostringstream oss;
  if (!proc) {
    oss << "#<process: invalid>";
  }
  else {
    proc->update_status ();
    oss << "#<process pid:" << proc->pid << " status:";
    if (proc->exited) {
      oss << "exited(" << proc->exit_code << ")";
    }
    else {
      oss << "running";
    }
    oss << ">";
  }
  return s7_make_string (sc, oss.str ().c_str ());
}

s7_pointer
f_subprocess_run_values (s7_scheme* sc, s7_pointer args) {
  s7_pointer cmd_arg= s7_car (args);
  args              = s7_cdr (args);

  if (!s7_is_string (cmd_arg) && !s7_is_proper_list (sc, cmd_arg)) {
    return subprocess_type_error (sc, "g_subprocess-run-values: command must be a proper list or a string", cmd_arg);
  }

  const char* cwd= nullptr;
  if (s7_is_pair (args) && s7_is_string (s7_car (args))) {
    cwd = s7_string (s7_car (args));
    args= s7_cdr (args);
  }
  else if (s7_is_pair (args)) {
    args= s7_cdr (args);
  }

  s7_pointer env_alist= nullptr;
  if (s7_is_pair (args) && !s7_is_null (sc, s7_car (args)) && s7_car (args) != s7_f (sc)) {
    s7_pointer env_arg= s7_car (args);
    if (!s7_is_pair (env_arg)) {
      return subprocess_type_error (sc, "g_subprocess-run-values: env must be an alist", env_arg);
    }
    if (!s7_is_proper_list (sc, env_arg)) {
      return subprocess_type_error (sc, "g_subprocess-run-values: env must be a proper list", env_arg);
    }
    s7_pointer it= env_arg;
    while (s7_is_pair (it)) {
      s7_pointer item= s7_car (it);
      if (!s7_is_pair (item)) {
        return subprocess_type_error (sc, "g_subprocess-run-values: env element must be a pair", item);
      }
      s7_pointer key_arg= s7_car (item);
      if (!s7_is_string (key_arg)) {
        return subprocess_type_error (sc, "g_subprocess-run-values: env key must be a string", key_arg);
      }
      s7_pointer val_arg= s7_cdr (item);
      if (!s7_is_string (val_arg)) {
        return subprocess_type_error (sc, "g_subprocess-run-values: env value must be a string", val_arg);
      }
      it= s7_cdr (it);
    }
    if (!s7_is_null (sc, it)) {
      return subprocess_type_error (sc, "g_subprocess-run-values: env must be a proper list", env_arg);
    }
    env_alist= env_arg;
    args     = s7_cdr (args);
  }
  else if (s7_is_pair (args)) {
    args= s7_cdr (args);
  }

  vector<string>      env_storage;
  vector<const char*> envp;
  if (env_alist) {
    s7_pointer it= env_alist;
    while (s7_is_pair (it)) {
      s7_pointer item= s7_car (it);
      env_storage.push_back (string (s7_string (s7_car (item))) + "=" + s7_string (s7_cdr (item)));
      it= s7_cdr (it);
    }
    for (auto& s : env_storage) {
      envp.push_back (s.c_str ());
    }
    envp.push_back (nullptr);
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
  if (s7_is_pair (args) && (s7_is_integer (s7_car (args)) || s7_is_real (s7_car (args)))) {
    timeout_ms= (tb_long_t) (s7_number_to_real (sc, s7_car (args)) * 1000.0);
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
      else if (strcmp (sym, "tee") == 0) {
        stdout_mode= redirect_mode::tee;
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
      else if (strcmp (sym, "tee") == 0) {
        stderr_mode= redirect_mode::tee;
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
    if (s7_is_symbol (stdin_val) &&
        (strcmp (s7_symbol_name (stdin_val), "null") == 0 || strcmp (s7_symbol_name (stdin_val), "discard") == 0)) {
      stdin_null= true;
    }
    else if (s7_is_string (stdin_val)) {
      stdin_path= s7_string (stdin_val);
    }
  }

  tb_process_attr_t attr= {tb_null};
  attr.flags            = TB_PROCESS_FLAG_NO_WINDOW | TB_PROCESS_FLAG_DETACH;

  if (cwd) attr.curdir= cwd;
  if (!envp.empty ()) attr.envp= (tb_char_t const**) envp.data ();

  bool need_stdout_pipe=
      (stdout_mode == redirect_mode::tee || stdout_mode == redirect_mode::capture) ||
      (stderr_to_stdout && stdout_mode != redirect_mode::file && stdout_mode != redirect_mode::discard);

#ifdef TB_CONFIG_OS_WINDOWS
  HANDLE              h_out_read= NULL, h_out_write= NULL;
  HANDLE              h_err_read= NULL, h_err_write= NULL;
  HANDLE              h_in_read= NULL, h_in_write= NULL;
  SECURITY_ATTRIBUTES sa= {sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
#else
  tb_pipe_file_ref_t out_pipe[2]= {tb_null};
#endif

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
#ifdef TB_CONFIG_OS_WINDOWS
    CreatePipe (&h_out_read, &h_out_write, &sa, 0);
    SetHandleInformation (h_out_read, HANDLE_FLAG_INHERIT, 0);
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.out.file= (tb_file_ref_t) h_out_write;
#else
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (out_pipe, mode, 0);
    set_pipe_pair_cloexec (out_pipe);
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.out.pipe= out_pipe[1];
#endif
  }

#ifndef TB_CONFIG_OS_WINDOWS
  tb_pipe_file_ref_t err_pipe[2]= {tb_null};
#endif
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
  else if (stderr_to_stdout && stdout_mode == redirect_mode::file) {
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.err.path= stdout_path;
    attr.errmode = TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_APPEND;
  }
#ifdef TB_CONFIG_OS_WINDOWS
  else if (stderr_to_stdout && h_out_write) {
    HANDLE h_err_dup= NULL;
    if (DuplicateHandle (GetCurrentProcess (), h_out_write, GetCurrentProcess (), &h_err_dup, 0, TRUE,
                         DUPLICATE_SAME_ACCESS)) {
      attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILE;
      attr.err.file= (tb_file_ref_t) h_err_dup;
      h_err_write  = h_err_dup;
    }
  }
  else if (stderr_mode == redirect_mode::tee || stderr_mode == redirect_mode::capture) {
    CreatePipe (&h_err_read, &h_err_write, &sa, 0);
    SetHandleInformation (h_err_read, HANDLE_FLAG_INHERIT, 0);
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.err.file= (tb_file_ref_t) h_err_write;
  }
#else
  else if (stderr_to_stdout && out_pipe[1]) {
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.err.pipe= out_pipe[1];
  }
  else if (stderr_mode == redirect_mode::tee || stderr_mode == redirect_mode::capture) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (err_pipe, mode, 0);
    set_pipe_pair_cloexec (err_pipe);
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.err.pipe= err_pipe[1];
  }
#endif

#ifndef TB_CONFIG_OS_WINDOWS
  tb_pipe_file_ref_t in_pipe[2]= {tb_null};
#endif
  if (stdin_path) {
    attr.intype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.in.path= stdin_path;
    attr.inmode = TB_FILE_MODE_RO;
  }
#ifdef TB_CONFIG_OS_WINDOWS
  else if (stdin_null) {
    CreatePipe (&h_in_read, &h_in_write, &sa, 0);
    SetHandleInformation (h_in_write, HANDLE_FLAG_INHERIT, 0);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.in.file= (tb_file_ref_t) h_in_read;
    CloseHandle (h_in_write);
    h_in_write= NULL;
  }
  else if (input) {
    CreatePipe (&h_in_read, &h_in_write, &sa, 0);
    SetHandleInformation (h_in_write, HANDLE_FLAG_INHERIT, 0);
    attr.intype  = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.in.file = (tb_file_ref_t) h_in_read;
    DWORD written= 0;
    WriteFile (h_in_write, input, (DWORD) input_len, &written, NULL);
    CloseHandle (h_in_write);
    h_in_write= NULL;
  }
#else
  else if (stdin_null) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO | TB_PIPE_MODE_BLOCK, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (in_pipe, mode, 0);
    set_pipe_pair_cloexec (in_pipe);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.in.pipe= in_pipe[0];
    tb_pipe_file_exit (in_pipe[1]);
  }
  else if (input) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO | TB_PIPE_MODE_BLOCK, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (in_pipe, mode, 0);
    set_pipe_pair_cloexec (in_pipe);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.in.pipe= in_pipe[0];
    tb_pipe_file_write (in_pipe[1], (tb_byte_t*) input, input_len);
    tb_pipe_file_exit (in_pipe[1]);
  }
#endif

#ifdef TB_CONFIG_OS_WINDOWS
  HANDLE h_job= create_process_job ();
#endif

  tb_process_ref_t process= tb_null;
  if (s7_is_string (cmd_arg)) {
    const char* cmd_c= s7_string (cmd_arg);
#ifdef TB_CONFIG_OS_WINDOWS
    process= tb_process_init_cmd (cmd_c, &attr);
#elif !defined(__EMSCRIPTEN__)
    wordexp_t p;
    int       ret= wordexp (cmd_c, &p, 0);
    if (ret == 0 && p.we_wordc > 0) {
      process= tb_process_init (p.we_wordv[0], (tb_char_t const**) p.we_wordv, &attr);
      wordfree (&p);
    }
    else {
      process= tb_process_init_cmd (cmd_c, &attr);
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
    if (!argv.empty ()) {
      argv.push_back (nullptr);
      process= tb_process_init (argv[0], argv.data (), &attr);
    }
  }

#ifdef TB_CONFIG_OS_WINDOWS
  if (process && h_job) {
    assign_process_to_job (h_job, process);
  }
#endif

#ifdef TB_CONFIG_OS_WINDOWS
  if (h_out_write) {
    CloseHandle (h_out_write);
    h_out_write= NULL;
  }
  if (h_err_write) {
    CloseHandle (h_err_write);
    h_err_write= NULL;
  }
  if (h_in_read) {
    CloseHandle (h_in_read);
    h_in_read= NULL;
  }
#else
  if (out_pipe[1]) {
    tb_pipe_file_exit (out_pipe[1]);
    out_pipe[1]= tb_null;
  }
  if (err_pipe[1]) {
    tb_pipe_file_exit (err_pipe[1]);
    err_pipe[1]= tb_null;
  }
#endif

  string    stdout_str;
  string    stderr_str;
  tb_long_t status= -1;

  if (process) {
    tb_hong_t start_time= tb_mclock ();
    bool      timed_out = false;
    bool      out_open  = need_stdout_pipe;
    bool      err_open  = (stderr_mode == redirect_mode::tee || stderr_mode == redirect_mode::capture);
#ifndef TB_CONFIG_OS_WINDOWS
    if (!out_pipe[0]) out_open= false;
    if (!err_pipe[0]) err_open= false;
#else
    if (!h_out_read) out_open= false;
    if (!h_err_read) err_open= false;
#endif

    // 流式读取排空循环，防止大输出（如 10MB）管道缓冲区满导致子进程与父进程死锁
    while (out_open || err_open) {
      bool read_any= false;
#ifdef TB_CONFIG_OS_WINDOWS
      if (out_open) {
        DWORD avail= 0;
        if (PeekNamedPipe (h_out_read, NULL, 0, NULL, &avail, NULL)) {
          if (avail > 0) {
            char  buf[65536];
            DWORD to_read= (std::min) ((DWORD) sizeof (buf), avail);
            DWORD n      = 0;
            if (ReadFile (h_out_read, buf, to_read, &n, NULL) && n > 0) {
              stdout_str.append (buf, n);
              if (stdout_mode == redirect_mode::tee) {
                fwrite (buf, 1, n, stdout);
                fflush (stdout);
              }
              read_any= true;
            }
          }
        }
        else {
          out_open= false;
        }
      }
      if (err_open) {
        DWORD avail= 0;
        if (PeekNamedPipe (h_err_read, NULL, 0, NULL, &avail, NULL)) {
          if (avail > 0) {
            char  buf[65536];
            DWORD to_read= (std::min) ((DWORD) sizeof (buf), avail);
            DWORD n      = 0;
            if (ReadFile (h_err_read, buf, to_read, &n, NULL) && n > 0) {
              stderr_str.append (buf, n);
              if (stderr_mode == redirect_mode::tee) {
                fwrite (buf, 1, n, stderr);
                fflush (stderr);
              }
              read_any= true;
            }
          }
        }
        else {
          err_open= false;
        }
      }
#else
      struct pollfd pfds[2];
      int           nfds   = 0;
      int           out_idx= -1;
      int           err_idx= -1;
      if (out_open) {
        out_idx              = nfds++;
        pfds[out_idx].fd     = tb_pipefile2fd (out_pipe[0]);
        pfds[out_idx].events = POLLIN;
        pfds[out_idx].revents= 0;
      }
      if (err_open) {
        err_idx              = nfds++;
        pfds[err_idx].fd     = tb_pipefile2fd (err_pipe[0]);
        pfds[err_idx].events = POLLIN;
        pfds[err_idx].revents= 0;
      }
      int pret= ::poll (pfds, nfds, 50);
      if (pret > 0) {
        if (out_idx >= 0 && (pfds[out_idx].revents & (POLLIN | POLLHUP | POLLERR))) {
          char    buf[65536];
          ssize_t n= read (pfds[out_idx].fd, buf, sizeof (buf));
          if (n > 0) {
            stdout_str.append (buf, n);
            if (stdout_mode == redirect_mode::tee) {
              fwrite (buf, 1, n, stdout);
              fflush (stdout);
            }
            read_any= true;
          }
          else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {
            out_open= false;
          }
        }
        if (err_idx >= 0 && (pfds[err_idx].revents & (POLLIN | POLLHUP | POLLERR))) {
          char    buf[65536];
          ssize_t n= read (pfds[err_idx].fd, buf, sizeof (buf));
          if (n > 0) {
            stderr_str.append (buf, n);
            if (stderr_mode == redirect_mode::tee) {
              fwrite (buf, 1, n, stderr);
              fflush (stderr);
            }
            read_any= true;
          }
          else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {
            err_open= false;
          }
        }
      }
#endif

      if (timeout_ms >= 0 && (tb_mclock () - start_time >= timeout_ms)) {
#ifdef TB_CONFIG_OS_WINDOWS
        if (h_job) {
          TerminateJobObject (h_job, 1);
        }
        tb_process_kill (process);
        tb_int_t p_pid= get_process_pid (process);
        if (p_pid > 0) {
          string cmd= "taskkill /F /T /PID " + std::to_string (p_pid) + " >NUL 2>&1";
          system (cmd.c_str ());
        }
#else
        tb_int_t p_pid= get_process_pid (process);
        if (p_pid > 1) {
          kill (-p_pid, SIGKILL);
          kill (p_pid, SIGKILL);
        }
        else if (p_pid > 0) {
          kill (p_pid, SIGKILL);
        }
        else {
          tb_process_kill (process);
        }
#endif
        tb_process_wait (process, &status, -1);
        status   = -1;
        timed_out= true;
        break;
      }

      tb_long_t wait_ret= tb_process_wait (process, &status, 0);
      if (wait_ret > 0) {
        break;
      }

#ifdef TB_CONFIG_OS_WINDOWS
      if (!read_any) Sleep (5);
#endif
    }

#ifdef TB_CONFIG_OS_WINDOWS
    if (h_out_read) {
      char  buf[65536];
      DWORD n= 0;
      while (ReadFile (h_out_read, buf, sizeof (buf), &n, NULL) && n > 0) {
        stdout_str.append (buf, n);
        if (stdout_mode == redirect_mode::tee) {
          fwrite (buf, 1, n, stdout);
          fflush (stdout);
        }
      }
      CloseHandle (h_out_read);
      h_out_read= NULL;
    }

    if (h_err_read) {
      char  buf[65536];
      DWORD n= 0;
      while (ReadFile (h_err_read, buf, sizeof (buf), &n, NULL) && n > 0) {
        stderr_str.append (buf, n);
        if (stderr_mode == redirect_mode::tee) {
          fwrite (buf, 1, n, stderr);
          fflush (stderr);
        }
      }
      CloseHandle (h_err_read);
      h_err_read= NULL;
    }
#else
    if (out_pipe[0]) {
      char buf[65536];
      int  fd= tb_pipefile2fd (out_pipe[0]);
      fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK);
      ssize_t n;
      while ((n= read (fd, buf, sizeof (buf))) > 0) {
        stdout_str.append (buf, n);
        if (stdout_mode == redirect_mode::tee) {
          fwrite (buf, 1, n, stdout);
          fflush (stdout);
        }
      }
      tb_pipe_file_exit (out_pipe[0]);
      out_pipe[0]= tb_null;
    }

    if (err_pipe[0]) {
      char buf[65536];
      int  fd= tb_pipefile2fd (err_pipe[0]);
      fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK);
      ssize_t n;
      while ((n= read (fd, buf, sizeof (buf))) > 0) {
        stderr_str.append (buf, n);
        if (stderr_mode == redirect_mode::tee) {
          fwrite (buf, 1, n, stderr);
          fflush (stderr);
        }
      }
      tb_pipe_file_exit (err_pipe[0]);
      err_pipe[0]= tb_null;
    }
#endif

    if (!timed_out && status == -1) {
      tb_long_t remaining_ms= -1;
      if (timeout_ms >= 0) {
        tb_hong_t elapsed= tb_mclock () - start_time;
        remaining_ms     = (elapsed >= timeout_ms) ? 0 : (tb_long_t) (timeout_ms - elapsed);
      }
      tb_long_t wait_ret= tb_process_wait (process, &status, remaining_ms);
      if (wait_ret == 0) {
#ifdef TB_CONFIG_OS_WINDOWS
        if (h_job) {
          TerminateJobObject (h_job, 1);
        }
        tb_process_kill (process);
        tb_int_t p_pid= get_process_pid (process);
        if (p_pid > 0) {
          string cmd= "taskkill /F /T /PID " + std::to_string (p_pid) + " >NUL 2>&1";
          system (cmd.c_str ());
        }
#else
        tb_int_t p_pid= get_process_pid (process);
        if (p_pid > 1) {
          kill (-p_pid, SIGKILL);
          kill (p_pid, SIGKILL);
        }
        else if (p_pid > 0) {
          kill (p_pid, SIGKILL);
        }
        else {
          tb_process_kill (process);
        }
#endif
        tb_process_wait (process, &status, -1);
        status= -1;
      }
    }
    tb_process_exit (process);
  }
#ifdef TB_CONFIG_OS_WINDOWS
  if (h_job) {
    CloseHandle (h_job);
    h_job= NULL;
  }
#endif
#ifdef TB_CONFIG_OS_WINDOWS
  if (h_out_read) CloseHandle (h_out_read);
  if (h_err_read) CloseHandle (h_err_read);
#endif

  s7_pointer out_s7 = s7_make_string (sc, stdout_str.c_str ());
  s7_pointer err_s7 = s7_make_string (sc, stderr_str.c_str ());
  s7_pointer code_s7= s7_make_integer (sc, (s7_int) status);
  return s7_values (sc, s7_cons (sc, out_s7, s7_cons (sc, err_s7, s7_cons (sc, code_s7, s7_nil (sc)))));
}

s7_pointer
f_process_p (s7_scheme* sc, s7_pointer args) {
  return s7_make_boolean (sc, is_goldfish_process (sc, s7_car (args)));
}

s7_pointer
f_subprocess_spawn (s7_scheme* sc, s7_pointer args) {
  s7_pointer cmd_arg   = s7_car (args);
  s7_pointer cwd_arg   = s7_cadr (args);
  s7_pointer env_arg   = s7_caddr (args);
  s7_pointer stdin_arg = s7_cadddr (args);
  s7_pointer stdout_arg= s7_car (s7_cddddr (args));
  s7_pointer stderr_arg= s7_cadr (s7_cddddr (args));

  if (!s7_is_string (cmd_arg) && !s7_is_proper_list (sc, cmd_arg)) {
    return subprocess_type_error (sc, "g_subprocess-spawn: command must be a proper list or a string", cmd_arg);
  }

  const char* cwd= nullptr;
  if (s7_is_string (cwd_arg)) {
    cwd= s7_string (cwd_arg);
  }

  s7_pointer env_alist= nullptr;
  if (!s7_is_null (sc, env_arg) && env_arg != s7_f (sc)) {
    if (!s7_is_proper_list (sc, env_arg)) {
      return subprocess_type_error (sc, "g_subprocess-spawn: env must be a proper list", env_arg);
    }
    s7_pointer it= env_arg;
    while (s7_is_pair (it)) {
      s7_pointer item= s7_car (it);
      if (!s7_is_pair (item)) {
        return subprocess_type_error (sc, "g_subprocess-spawn: env element must be a pair", item);
      }
      s7_pointer key_arg= s7_car (item);
      if (!s7_is_string (key_arg)) {
        return subprocess_type_error (sc, "g_subprocess-spawn: env key must be a string", key_arg);
      }
      s7_pointer val_arg= s7_cdr (item);
      if (!s7_is_string (val_arg)) {
        return subprocess_type_error (sc, "g_subprocess-spawn: env value must be a string", val_arg);
      }
      it= s7_cdr (it);
    }
    env_alist= env_arg;
  }

  vector<string>      env_storage;
  vector<const char*> envp;
  if (env_alist) {
    s7_pointer it= env_alist;
    while (s7_is_pair (it)) {
      s7_pointer item= s7_car (it);
      env_storage.push_back (string (s7_string (s7_car (item))) + "=" + s7_string (s7_cdr (item)));
      it= s7_cdr (it);
    }
    for (auto& s : env_storage) {
      envp.push_back (s.c_str ());
    }
    envp.push_back (nullptr);
  }

  tb_process_attr_t attr= {tb_null};
  attr.flags            = TB_PROCESS_FLAG_NO_WINDOW | TB_PROCESS_FLAG_DETACH;
  if (cwd) attr.curdir= cwd;
  if (!envp.empty ()) attr.envp= (tb_char_t const**) envp.data ();

  bool        stdin_pipe= false;
  const char* stdin_path= nullptr;
  bool        stdin_null= false;
  if (s7_is_symbol (stdin_arg)) {
    const char* sym= s7_symbol_name (stdin_arg);
    if (strcmp (sym, "pipe") == 0) stdin_pipe= true;
    else if (strcmp (sym, "null") == 0) stdin_null= true;
  }
  else if (s7_is_string (stdin_arg)) {
    stdin_path= s7_string (stdin_arg);
  }

  bool        stdout_pipe   = false;
  bool        stdout_discard= false;
  const char* stdout_path   = nullptr;
  if (s7_is_symbol (stdout_arg)) {
    const char* sym= s7_symbol_name (stdout_arg);
    if (strcmp (sym, "pipe") == 0 || strcmp (sym, "capture") == 0) stdout_pipe= true;
    else if (strcmp (sym, "discard") == 0) stdout_discard= true;
  }
  else if (s7_is_string (stdout_arg)) {
    stdout_path= s7_string (stdout_arg);
  }

  bool        stderr_pipe     = false;
  bool        stderr_discard  = false;
  bool        stderr_to_stdout= false;
  const char* stderr_path     = nullptr;
  if (s7_is_symbol (stderr_arg)) {
    const char* sym= s7_symbol_name (stderr_arg);
    if (strcmp (sym, "pipe") == 0 || strcmp (sym, "capture") == 0) stderr_pipe= true;
    else if (strcmp (sym, "stdout") == 0) stderr_to_stdout= true;
    else if (strcmp (sym, "discard") == 0) stderr_discard= true;
  }
  else if (s7_is_string (stderr_arg)) {
    stderr_path= s7_string (stderr_arg);
  }

#ifdef TB_CONFIG_OS_WINDOWS
  HANDLE              h_job     = create_process_job ();
  HANDLE              h_out_read= NULL, h_out_write= NULL;
  HANDLE              h_err_read= NULL, h_err_write= NULL;
  HANDLE              h_in_read= NULL, h_in_write= NULL;
  SECURITY_ATTRIBUTES sa= {sizeof (SECURITY_ATTRIBUTES), NULL, TRUE};
#else
  tb_pipe_file_ref_t out_pipe[2]= {tb_null};
  tb_pipe_file_ref_t err_pipe[2]= {tb_null};
  tb_pipe_file_ref_t in_pipe[2] = {tb_null};
#endif

  // Configure stdout
  if (stdout_path) {
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.out.path= stdout_path;
    attr.outmode = TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC;
  }
  else if (stdout_discard) {
    attr.outtype= TB_PROCESS_REDIRECT_TYPE_FILEPATH;
#ifdef TB_CONFIG_OS_WINDOWS
    attr.out.path= "NUL";
#else
    attr.out.path= "/dev/null";
#endif
    attr.outmode= TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC;
  }
  else if (stdout_pipe) {
#ifdef TB_CONFIG_OS_WINDOWS
    CreatePipe (&h_out_read, &h_out_write, &sa, 0);
    SetHandleInformation (h_out_read, HANDLE_FLAG_INHERIT, 0);
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.out.file= (tb_file_ref_t) h_out_write;
#else
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (out_pipe, mode, 0);
    set_pipe_pair_cloexec (out_pipe);
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.out.pipe= out_pipe[1];
#endif
  }

  // Configure stderr
  if (stderr_path) {
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.err.path= stderr_path;
    attr.errmode = TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC;
  }
  else if (stderr_discard) {
    attr.errtype= TB_PROCESS_REDIRECT_TYPE_FILEPATH;
#ifdef TB_CONFIG_OS_WINDOWS
    attr.err.path= "NUL";
#else
    attr.err.path= "/dev/null";
#endif
    attr.errmode= TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC;
  }
  else if (stderr_to_stdout && stdout_path) {
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.err.path= stdout_path;
    attr.errmode = TB_FILE_MODE_RW | TB_FILE_MODE_CREAT | TB_FILE_MODE_APPEND;
  }
#ifdef TB_CONFIG_OS_WINDOWS
  else if (stderr_to_stdout && h_out_write) {
    HANDLE h_err_dup= NULL;
    if (DuplicateHandle (GetCurrentProcess (), h_out_write, GetCurrentProcess (), &h_err_dup, 0, TRUE,
                         DUPLICATE_SAME_ACCESS)) {
      attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILE;
      attr.err.file= (tb_file_ref_t) h_err_dup;
      h_err_write  = h_err_dup;
    }
  }
  else if (stderr_pipe) {
    CreatePipe (&h_err_read, &h_err_write, &sa, 0);
    SetHandleInformation (h_err_read, HANDLE_FLAG_INHERIT, 0);
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.err.file= (tb_file_ref_t) h_err_write;
  }
#else
  else if (stderr_to_stdout && out_pipe[1]) {
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.err.pipe= out_pipe[1];
  }
  else if (stderr_pipe) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (err_pipe, mode, 0);
    set_pipe_pair_cloexec (err_pipe);
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.err.pipe= err_pipe[1];
  }
#endif

  // Configure stdin
  if (stdin_path) {
    attr.intype = TB_PROCESS_REDIRECT_TYPE_FILEPATH;
    attr.in.path= stdin_path;
    attr.inmode = TB_FILE_MODE_RO;
  }
#ifdef TB_CONFIG_OS_WINDOWS
  else if (stdin_null) {
    CreatePipe (&h_in_read, &h_in_write, &sa, 0);
    SetHandleInformation (h_in_write, HANDLE_FLAG_INHERIT, 0);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.in.file= (tb_file_ref_t) h_in_read;
    CloseHandle (h_in_write);
    h_in_write= NULL;
  }
  else if (stdin_pipe) {
    CreatePipe (&h_in_read, &h_in_write, &sa, 0);
    SetHandleInformation (h_in_write, HANDLE_FLAG_INHERIT, 0);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_FILE;
    attr.in.file= (tb_file_ref_t) h_in_read;
  }
#else
  else if (stdin_null) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO | TB_PIPE_MODE_BLOCK, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (in_pipe, mode, 0);
    set_pipe_pair_cloexec (in_pipe);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.in.pipe= in_pipe[0];
    tb_pipe_file_exit (in_pipe[1]);
  }
  else if (stdin_pipe) {
    tb_size_t mode[2]= {TB_PIPE_MODE_RO | TB_PIPE_MODE_BLOCK, TB_PIPE_MODE_WO | TB_PIPE_MODE_BLOCK};
    tb_pipe_file_init_pair (in_pipe, mode, 0);
    set_pipe_pair_cloexec (in_pipe);
    attr.intype = TB_PROCESS_REDIRECT_TYPE_PIPE;
    attr.in.pipe= in_pipe[0];
  }
#endif

  tb_process_ref_t process= tb_null;
  if (s7_is_string (cmd_arg)) {
    const char* cmd_c= s7_string (cmd_arg);
#ifdef TB_CONFIG_OS_WINDOWS
    process= tb_process_init_cmd (cmd_c, &attr);
#elif !defined(__EMSCRIPTEN__)
    wordexp_t p;
    int       ret= wordexp (cmd_c, &p, 0);
    if (ret == 0 && p.we_wordc > 0) {
      process= tb_process_init (p.we_wordv[0], (tb_char_t const**) p.we_wordv, &attr);
      wordfree (&p);
    }
    else {
      process= tb_process_init_cmd (cmd_c, &attr);
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
    if (!argv.empty ()) {
      argv.push_back (nullptr);
      process= tb_process_init (argv[0], argv.data (), &attr);
    }
  }

#ifdef TB_CONFIG_OS_WINDOWS
  if (h_out_write) CloseHandle (h_out_write);
  if (h_err_write) CloseHandle (h_err_write);
  if (h_in_read) CloseHandle (h_in_read);
#else
  if (out_pipe[1]) tb_pipe_file_exit (out_pipe[1]);
  if (err_pipe[1]) tb_pipe_file_exit (err_pipe[1]);
  if (in_pipe[0]) tb_pipe_file_exit (in_pipe[0]);
#endif

  if (!process) {
#ifdef TB_CONFIG_OS_WINDOWS
    if (h_job) CloseHandle (h_job);
    if (h_out_read) CloseHandle (h_out_read);
    if (h_err_read) CloseHandle (h_err_read);
    if (h_in_write) CloseHandle (h_in_write);
#else
    if (out_pipe[0]) tb_pipe_file_exit (out_pipe[0]);
    if (err_pipe[0]) tb_pipe_file_exit (err_pipe[0]);
    if (in_pipe[1]) tb_pipe_file_exit (in_pipe[1]);
#endif
    return subprocess_type_error (sc, "g_subprocess-spawn: failed to start process", cmd_arg);
  }

  auto* sub   = new Subprocess ();
  sub->process= process;
  sub->pid    = get_process_pid (process);
  sub->update_activity ();

#ifdef TB_CONFIG_OS_WINDOWS
  assign_process_to_job (h_job, process);
  sub->h_job     = h_job;
  sub->h_out_read= h_out_read;
  sub->h_err_read= h_err_read;
  sub->h_in_write= h_in_write;
  if (!h_out_read) sub->stdout_eof= true;
  if (!h_err_read) sub->stderr_eof= true;
#else
  sub->out_pipe= out_pipe[0];
  sub->err_pipe= err_pipe[0];
  sub->in_pipe = in_pipe[1];
  if (!out_pipe[0]) sub->stdout_eof= true;
  if (!err_pipe[0]) sub->stderr_eof= true;
#endif

  return s7_make_c_object (sc, g_process_type_tag, sub);
}

s7_pointer
f_subprocess_pid (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-pid: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  return s7_make_integer (sc, proc->pid);
}

s7_pointer
f_subprocess_alive_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-alive?: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  return s7_make_boolean (sc, proc->alive ());
}

s7_pointer
f_subprocess_poll (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-poll: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  if (proc->poll () == 1) {
    return s7_make_integer (sc, proc->exit_code);
  }
  return s7_f (sc);
}

s7_pointer
f_subprocess_wait (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-wait: first argument must be a process", proc_arg);
  }
  tb_long_t timeout_ms= -1;
  if (s7_is_pair (s7_cdr (args))) {
    s7_pointer to_arg= s7_cadr (args);
    if (!s7_is_integer (to_arg)) {
      return subprocess_type_error (sc, "g_subprocess-wait: timeout must be an integer", to_arg);
    }
    timeout_ms= s7_integer (to_arg);
  }

  auto* proc= get_goldfish_process (sc, proc_arg);
  int   r   = proc->wait (timeout_ms);
  if (r == 1) {
    return s7_make_integer (sc, proc->exit_code);
  }
  return s7_f (sc);
}

s7_pointer
f_subprocess_terminate (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-terminate: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  proc->terminate ();
  return s7_unspecified (sc);
}

s7_pointer
f_subprocess_kill (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-kill: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  proc->kill_proc ();
  return s7_unspecified (sc);
}

s7_pointer
f_subprocess_close (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-close: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  proc->close_pipes ();
  return s7_unspecified (sc);
}

s7_pointer
f_subprocess_close_stdin (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-close-stdin: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  proc->close_stdin ();
  return s7_unspecified (sc);
}

s7_pointer
f_subprocess_write (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  s7_pointer data_arg= s7_cadr (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "process-write-stdin: first argument must be a process", proc_arg);
  }
  const char* data_ptr= nullptr;
  size_t      data_len= 0;
  if (s7_is_string (data_arg)) {
    data_ptr= s7_string (data_arg);
    data_len= s7_string_length (data_arg);
  }
  else if (s7_is_byte_vector (data_arg)) {
    data_ptr= (const char*) s7_byte_vector_elements (data_arg);
    data_len= (size_t) s7_vector_length (data_arg);
  }
  else {
    return subprocess_type_error (sc, "process-write-stdin: second argument must be a string or bytevector", data_arg);
  }
  auto*  proc   = get_goldfish_process (sc, proc_arg);
  size_t written= proc->write_stdin (data_ptr, data_len);
  return s7_make_integer (sc, (s7_int) written);
}

s7_pointer
f_subprocess_read (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-read: first argument must be a process", proc_arg);
  }
  size_t max_bytes= 65536;
  bool   is_err   = false;

  args= s7_cdr (args);
  if (s7_is_pair (args) && s7_is_integer (s7_car (args))) {
    max_bytes= (size_t) s7_integer (s7_car (args));
    args     = s7_cdr (args);
  }
  if (s7_is_pair (args) && s7_is_symbol (s7_car (args))) {
    const char* sym= s7_symbol_name (s7_car (args));
    if (strcmp (sym, "stderr") == 0) is_err= true;
  }

  auto* proc          = get_goldfish_process (sc, proc_arg);
  auto [chunk, is_eof]= proc->read_chunk (is_err, max_bytes);
  if (is_eof) {
    return s7_eof_object (sc);
  }
  return s7_make_string_with_length (sc, chunk.data (), chunk.size ());
}

s7_pointer
f_subprocess_read_line (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-read-line: first argument must be a process", proc_arg);
  }
  bool is_err= false;
  args       = s7_cdr (args);
  if (s7_is_pair (args) && s7_is_symbol (s7_car (args))) {
    const char* sym= s7_symbol_name (s7_car (args));
    if (strcmp (sym, "stderr") == 0) is_err= true;
  }

  auto* proc         = get_goldfish_process (sc, proc_arg);
  auto [line, is_eof]= proc->read_line (is_err);
  if (is_eof) {
    return s7_eof_object (sc);
  }
  return s7_make_string_with_length (sc, line.data (), line.size ());
}

s7_pointer
f_subprocess_poll_io (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-poll-io: first argument must be a process", proc_arg);
  }
  int timeout_ms= 50;
  args          = s7_cdr (args);
  if (s7_is_pair (args) && s7_is_integer (s7_car (args))) {
    timeout_ms= (int) s7_integer (s7_car (args));
  }

  auto* proc= get_goldfish_process (sc, proc_arg);
  int   r   = proc->poll_io (timeout_ms);
  if (r == 1) return s7_make_symbol (sc, "stdout");
  if (r == 2) return s7_make_symbol (sc, "stderr");
  if (r == 3) return s7_make_symbol (sc, "both");
  if (r == -1) return s7_make_symbol (sc, "eof");
  return s7_f (sc);
}

s7_pointer
f_subprocess_eof_p (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-eof?: first argument must be a process", proc_arg);
  }
  bool is_err= false;
  args       = s7_cdr (args);
  if (s7_is_pair (args) && s7_is_symbol (s7_car (args))) {
    const char* sym= s7_symbol_name (s7_car (args));
    if (strcmp (sym, "stderr") == 0) is_err= true;
  }

  auto* proc= get_goldfish_process (sc, proc_arg);
  return s7_make_boolean (sc, proc->is_eof (is_err));
}

#ifdef TB_CONFIG_OS_WINDOWS
static bool
windows_kill_tree (int pid, bool is_sigkill, int timeout_ms, Subprocess* proc) {
  if (proc && proc->exited) return true;
  if (!is_sigkill && timeout_ms > 0 && proc) {
    int r= proc->wait (timeout_ms);
    if (r == 1) {
      return true;
    }
  }
  if (proc && proc->h_job) {
    TerminateJobObject (proc->h_job, 1);
  }
  if (proc && proc->process) {
    tb_process_kill (proc->process);
  }
  if (pid > 0) {
    string cmd= "taskkill /F /T /PID " + std::to_string (pid) + " >NUL 2>&1";
    system (cmd.c_str ());
  }
  if (proc) {
    proc->exited= true;
  }
  return true;
}
#else
static bool
posix_kill_tree (int pid, bool is_sigkill, int timeout_ms, Subprocess* proc) {
  if (pid <= 0) return false;
  if (pid == 1) return true;

  int sig= is_sigkill ? SIGKILL : SIGTERM;

  if (pid > 1) {
    kill (-pid, sig);
  }
  kill (pid, sig);

  if (is_sigkill || timeout_ms <= 0) {
    if (!is_sigkill && timeout_ms <= 0) {
      bool group_alive= (pid > 1) && (kill (-pid, 0) == 0 || errno != ESRCH);
      bool main_alive = (kill (pid, 0) == 0 || errno != ESRCH);
      if (group_alive || main_alive) {
        if (pid > 1) kill (-pid, SIGKILL);
        kill (pid, SIGKILL);
      }
    }
    if (proc && !proc->exited) {
      proc->wait (50);
      proc->exited= true;
    }
    return true;
  }

  tb_hong_t start     = tb_mclock ();
  bool      all_exited= false;
  while (tb_mclock () - start < timeout_ms) {
    if (proc) {
      proc->update_status ();
    }
    bool main_alive= false;
    if (proc) {
      main_alive= !proc->exited;
    }
    else {
      int status= 0;
      int r     = waitpid (pid, &status, WNOHANG);
      if (r > 0) {
        main_alive= false;
      }
      else {
        main_alive= (kill (pid, 0) == 0 || errno != ESRCH);
      }
    }

    bool group_alive= false;
    if (pid > 1) {
      group_alive= (kill (-pid, 0) == 0 || errno != ESRCH);
    }

    if (!main_alive && !group_alive) {
      all_exited= true;
      break;
    }
    tb_msleep (20);
  }

  if (!all_exited) {
    if (pid > 1) kill (-pid, SIGKILL);
    kill (pid, SIGKILL);
    tb_msleep (20);
  }

  if (proc && !proc->exited) {
    proc->wait (50);
    proc->exited= true;
  }
  return true;
}
#endif

s7_pointer
f_subprocess_kill_tree (s7_scheme* sc, s7_pointer args) {
  s7_pointer  target= s7_car (args);
  Subprocess* proc  = nullptr;
  int         pid   = -1;

  if (is_goldfish_process (sc, target)) {
    proc= get_goldfish_process (sc, target);
    if (!proc || proc->exited) {
      return s7_t (sc);
    }
    pid= proc->pid;
  }
  else if (s7_is_integer (target)) {
    pid= (int) s7_integer (target);
  }
  else {
    return subprocess_type_error (sc, "g_subprocess-kill-tree: first argument must be a process or integer", target);
  }

  if (pid <= 0) {
    return s7_t (sc);
  }

  bool is_sigkill= true;
  args           = s7_cdr (args);
  if (s7_is_pair (args) && s7_is_symbol (s7_car (args))) {
    const char* sym= s7_symbol_name (s7_car (args));
    if (strcmp (sym, "sigterm") == 0) {
      is_sigkill= false;
    }
  }

  int timeout_ms= 3000;
  if (s7_is_pair (args)) {
    args= s7_cdr (args);
    if (s7_is_pair (args) && s7_is_integer (s7_car (args))) {
      timeout_ms= (int) s7_integer (s7_car (args));
    }
  }

#ifdef TB_CONFIG_OS_WINDOWS
  windows_kill_tree (pid, is_sigkill, timeout_ms, proc);
#else
  posix_kill_tree (pid, is_sigkill, timeout_ms, proc);
#endif

  return s7_t (sc);
}

s7_pointer
f_subprocess_last_activity_time (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-last-activity-time: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  return s7_make_real (sc, proc->get_last_activity_time_sec ());
}

s7_pointer
f_subprocess_idle_seconds (s7_scheme* sc, s7_pointer args) {
  s7_pointer proc_arg= s7_car (args);
  if (!is_goldfish_process (sc, proc_arg)) {
    return subprocess_type_error (sc, "g_subprocess-idle-seconds: argument must be a process", proc_arg);
  }
  auto* proc= get_goldfish_process (sc, proc_arg);
  return s7_make_real (sc, proc->get_idle_seconds ());
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

void
glue_subprocess (s7_scheme* sc) {
  if (g_process_type_tag == 0) {
    g_process_type_tag= s7_make_c_type (sc, "process");
    s7_c_type_set_free (sc, g_process_type_tag, process_free_c_value);
    s7_c_type_set_to_string (sc, g_process_type_tag, process_to_string_glue);
  }

  glue_subprocess_run_values (sc);

  glue_define (sc, "g_process?", "(g_process? obj) => boolean", f_process_p, 1, 0);
  glue_define (sc, "g_subprocess-spawn", "(g_subprocess-spawn cmd cwd env stdin stdout stderr) => <process>",
               f_subprocess_spawn, 6, 0);
  glue_define (sc, "g_subprocess-pid", "(g_subprocess-pid proc) => integer", f_subprocess_pid, 1, 0);
  glue_define (sc, "g_subprocess-alive?", "(g_subprocess-alive? proc) => boolean", f_subprocess_alive_p, 1, 0);
  glue_define (sc, "g_subprocess-poll", "(g_subprocess-poll proc) => integer | #f", f_subprocess_poll, 1, 0);
  glue_define (sc, "g_subprocess-wait", "(g_subprocess-wait proc [timeout-ms]) => integer | #f", f_subprocess_wait, 1,
               1);
  glue_define (sc, "g_subprocess-terminate", "(g_subprocess-terminate proc) => unspecified", f_subprocess_terminate, 1,
               0);
  glue_define (sc, "g_subprocess-kill", "(g_subprocess-kill proc) => unspecified", f_subprocess_kill, 1, 0);
  glue_define (sc, "g_subprocess-kill-tree", "(g_subprocess-kill-tree proc-or-pid [signal] [timeout-ms]) => boolean",
               f_subprocess_kill_tree, 1, 2);
  glue_define (sc, "g_subprocess-close", "(g_subprocess-close proc) => unspecified", f_subprocess_close, 1, 0);
  glue_define (sc, "g_subprocess-close-stdin", "(g_subprocess-close-stdin proc) => unspecified",
               f_subprocess_close_stdin, 1, 0);
  glue_define (sc, "g_subprocess-write", "(g_subprocess-write proc data) => integer", f_subprocess_write, 2, 0);
  glue_define (sc, "g_subprocess-read", "(g_subprocess-read proc [max-bytes] [stream]) => string | eof",
               f_subprocess_read, 1, 2);
  glue_define (sc, "g_subprocess-read-line", "(g_subprocess-read-line proc [stream]) => string | eof",
               f_subprocess_read_line, 1, 1);
  glue_define (sc, "g_subprocess-poll-io", "(g_subprocess-poll-io proc [timeout-ms]) => symbol | #f",
               f_subprocess_poll_io, 1, 1);
  glue_define (sc, "g_subprocess-eof?", "(g_subprocess-eof? proc [stream]) => boolean", f_subprocess_eof_p, 1, 1);
  glue_define (sc, "g_subprocess-last-activity-time", "(g_subprocess-last-activity-time proc) => real",
               f_subprocess_last_activity_time, 1, 0);
  glue_define (sc, "g_subprocess-idle-seconds", "(g_subprocess-idle-seconds proc) => real", f_subprocess_idle_seconds,
               1, 0);
}

} // namespace goldfish
