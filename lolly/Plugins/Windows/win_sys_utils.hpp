
/******************************************************************************
 * MODULE     : mingw_sys_utils.hpp
 * DESCRIPTION: external command handling
 * COPYRIGHT  : (C) 2015 Denis RAUX
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef WIN_SYS_UTILS_H
#define WIN_SYS_UTILS_H

#include "array.hpp"
#include "string.hpp"

SN win_get_process_id ();

#ifdef OS_MINGW
int win_system (array<::string> arg, array<int> fd_in, array<::string> str_in,
                array<int> fd_out, array<::string*> str_out);
#endif

#endif // defined MINGW_SYS_UTILS_H
