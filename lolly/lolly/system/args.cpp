
/******************************************************************************
 * MODULE     : args.cpp
 * DESCRIPTION: lolly::system::args like nowide::args
 * COPYRIGHT  : (C) 2024  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "args.hpp"
#include "lolly/data/unicode.hpp"
#include "string.hpp"

#if defined(OS_WIN) || defined(OS_MINGW)
#include <windows.h>
#endif
#include <stdio.h>
#if defined(OS_WIN) || defined(OS_MINGW)
#include <shellapi.h>
#endif

#if defined(OS_MINGW) || defined(OS_WIN)
using lolly::data::wchar_to_utf8;
#endif

namespace lolly {
namespace system {
#if defined(OS_WIN) || defined(OS_MINGW)
void
args::fix_args (int& argc, char**& argv) {
  int       wargc;
  wchar_t** wargv= CommandLineToArgvW (GetCommandLineW (), &wargc);
  if (!wargv) {
    argc              = 0;
    static char* dummy= 0;
    argv              = &dummy;
    return;
  }
  try {
    args_.resize (wargc + 1, 0);
    arg_values_.resize (wargc);
    for (int i= 0; i < wargc; i++) {
      arg_values_[i]= wchar_to_utf8 (wargv[i]);
      args_[i]      = as_charp (arg_values_[i]);
    }
    argc= wargc;
    argv= &args_[0];
  } catch (...) {
    LocalFree (wargv);
    throw;
  }
  LocalFree (wargv);
}
#endif

} // namespace system
} // namespace lolly
