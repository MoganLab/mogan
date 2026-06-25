
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

using lolly::hash::md5_hexdigest;

TEST_CASE ("md5_hexdigest") {
  SUBCASE ("normal file") {
    // 使用固定内容的临时文件，使期望哈希在所有平台上一致且确定，
    // 避免依赖真实 LICENSE 文件的行尾（CRLF/LF 随平台/autocrlf 变化）。
    url    file= url_temp ();
    string_save ("hello world", file);
    string expected_md5= "5eb63bbbe01eeed093cb22bb8f5acdc3";
    string_eq (md5_hexdigest (file), expected_md5);
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
