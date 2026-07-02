
/******************************************************************************
 * MODULE     : subprocess.cpp
 * DESCRIPTION: subprocess related routines
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <errno.h>

#if defined(OS_MACOS) || defined(OS_LINUX)
#include <wordexp.h>
#endif

#include "lolly/system/subprocess.hpp"
#include "tbox/tbox.h"

namespace lolly {
namespace system {

#ifdef OS_WASM
int
call (string cmd) {
  return -1;
}
#else
int
call (string cmd) {
  tb_process_attr_t attr= {tb_null};
  attr.flags            = TB_PROCESS_FLAG_NO_WINDOW;
  c_string cmd_c (cmd);

#if defined(OS_MINGW) || defined(OS_WIN)
  return (int) tb_process_run_cmd (cmd_c, &attr);
#else
  wordexp_t p;
  int       ret= wordexp (cmd_c, &p, 0);
  if (ret != 0) {
    return ret;
  }
  if (p.we_wordc == 0) {
    wordfree (&p);
    return EINVAL;
  }
  ret= (int) tb_process_run (p.we_wordv[0], (tb_char_t const**) p.we_wordv,
                             &attr);
  wordfree (&p);
  return ret;
#endif
}
#endif

int
check_output (string s, string& result, bool stderr_only, int64_t timeout) {
  tb_long_t status= -1;
  // init pipe files
  tb_pipe_file_ref_t file[2]= {0};
  if (!tb_pipe_file_init_pair (file, tb_null, 0)) {
    return status;
  }

  // init process
  c_string          cmd_ (s);
  tb_process_attr_t attr= {0};
  attr.flags            = TB_PROCESS_FLAG_NO_WINDOW;
  if (stderr_only) {
    attr.err.pipe= file[1];
    attr.errtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
  }
  else {
    attr.out.pipe= file[1];
    attr.outtype = TB_PROCESS_REDIRECT_TYPE_PIPE;
  }
  tb_process_ref_t process= tb_process_init_cmd (cmd_, &attr);
  if (process) {
    // read pipe data
    tb_size_t read= 0;
    // TODO: should be a config here
    tb_byte_t data[8192]= {0};
    tb_size_t size      = sizeof (data);
    tb_bool_t wait      = tb_false;
    while (read < size) {
      tb_long_t real= tb_pipe_file_read (file[0], data + read, size - read);
      if (real > 0) {
        read+= real;
        wait= tb_false;
      }
      else if (!real && !wait) {
        tb_long_t ok   = 0;
        int       retry= 25;
        if (read > 0) {
          retry= 2;
        }
        while (retry > 0 && (ok == 0)) {
          ok   = tb_pipe_file_wait (file[0], TB_PIPE_EVENT_READ, timeout);
          retry= retry - 1;
        }
        tb_check_break (ok > 0);
        wait= tb_true;
      }
      else break;
    }

    result= as_string ((tb_char_t*) data);

    // wait process
    tb_process_wait (process, &status, timeout);

    // exit process
    tb_process_exit (process);
  }

  // exit pipe files
  tb_pipe_file_exit (file[0]);
  tb_pipe_file_exit (file[1]);
  return status;
}

} // namespace system
} // namespace lolly