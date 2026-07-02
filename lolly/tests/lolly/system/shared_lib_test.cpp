
/******************************************************************************
 * MODULE     : shared_lib_test.cpp
 * DESCRIPTION: tests on dynamic library loading
 * COPYRIGHT  : (C) 2024  jingkaimori
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "lolly/system/shared_lib.hpp"
#include "sys_utils.hpp"

#ifndef OS_WASM
using lolly::system::load_shared_library;
using lolly::system::shared_lib;
#endif

TEST_CASE ("load_shared_library") {
#ifndef OS_WASM
  url lib_path;
  if (os_win ()) {
    lib_path= url_pwd () * "example_dynamic_library.dll";
  }
  else if (os_macos ()) {
    lib_path= url_pwd () * "libexample_dynamic_library.dylib";
  }
  else {
    lib_path= url_pwd () * "libexample_dynamic_library.so";
  }
  shared_lib lib= load_shared_library ("example_dynamic_library", lib_path);
  double (*func) (int)= lib->get_function<double, int> ("square_div_2");
  double res          = func (5);
  CHECK_EQ (res, 12.5);
#endif
}
