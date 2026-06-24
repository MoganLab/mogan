
/******************************************************************************
 * MODULE     : md5_test.cpp
 * DESCRIPTION: tests on md5
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "file.hpp"
#include "lolly/hash/md5.hpp"
#include "sys_utils.hpp"

using lolly::hash::md5_hexdigest;

TEST_CASE ("md5_hexdigest") {
  SUBCASE ("normal file") {
    url    license_file= url_pwd () * "LICENSE";
    string expected_md5;
    // 根据平台使用对应的MD5值
    if (os_win ()) {
      expected_md5=
          "3c34afdc3adf82d2448f12715a255122"; // Windows (CRLF) 版本的实际MD5值
    }
    else {
      expected_md5= "d32239bcb673463ab874e80d47fae504"; // Linux (LF) 版本
    }

    string_eq (md5_hexdigest (license_file), expected_md5);
  }
  SUBCASE ("empty file") {
    url temp= url_temp ();
    string_save ("", temp);
    CHECK_EQ (file_size (temp), 0);
    string_eq (md5_hexdigest (temp), "d41d8cd98f00b204e9800998ecf8427e");
  }
  SUBCASE ("invalid file") {
    string_eq (md5_hexdigest (url_system ("https://mogan.app")), "");
    string_eq (md5_hexdigest (url_system ("/path/to/not_exist")), "");
  }
}
