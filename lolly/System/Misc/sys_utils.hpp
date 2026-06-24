
/******************************************************************************
 * MODULE     : sys_utils.hpp
 * DESCRIPTION: system utilities
 * COPYRIGHT  : (C) 1999-2016  Joris van der Hoeven, Denis Raux
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef SYS_UTILS_H
#define SYS_UTILS_H

#include "array.hpp"
#include "string.hpp"

#if defined(OS_MINGW) || defined(_WIN32) || defined(_WIN64)
#define URL_CONCATER '\\'
#define URL_SEPARATOR ';'
#else
#define URL_CONCATER '/'
#define URL_SEPARATOR ':'
#endif

string get_env (string var);
void   set_env (string var, string with);

string get_user_login ();
string get_user_name ();

bool os_win ();
bool os_mingw ();
bool os_macos ();
bool os_wasm ();

SN get_process_id ();

array<string> evaluate_system (array<string> arg, array<int> fd_in,
                               array<string> in, array<int> fd_out);
namespace lolly {
void   init_tbox ();
string get_stacktrace (unsigned int max_frames= 127);
} // namespace lolly

#endif // defined SYS_UTILS_H
