
/******************************************************************************
 * MODULE     : sha_test.cpp
 * DESCRIPTION: tests on sha
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "a_lolly_test.hpp"
#include "file.hpp"
#include "lolly/hash/sha.hpp"

using lolly::hash::sha224_hexdigest;
using lolly::hash::sha256_hexdigest;

TEST_CASE ("sha224_hexdigest") {
  SUBCASE ("normal file") {
    // 使用固定内容的临时文件，使期望哈希在所有平台上一致且确定，
    // 避免依赖真实 LICENSE 文件的行尾（CRLF/LF 随平台/autocrlf 变化）。
    url file= url_temp ();
    string_save ("hello world", file);
    string expected_sha224=
        "2f05477fc24bb4faefd86517156dafdecec45b8ad3cf2522a563582b";
    string_eq (sha224_hexdigest (file), expected_sha224);
  }
  SUBCASE ("empty file") {
    url temp= url_temp ();
    string_save ("", temp);
    CHECK_EQ (file_size (temp), 0);
    string_eq (sha224_hexdigest (temp),
               "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f");
  }
  SUBCASE ("invalid file") {
    string_eq (sha224_hexdigest (url_system ("https://mogan.app")), "");
    string_eq (sha224_hexdigest (url_system ("/path/to/not_exist")), "");
  }
}

TEST_CASE ("sha256_hexdigest") {
  SUBCASE ("normal file") {
    // 使用固定内容的临时文件，使期望哈希在所有平台上一致且确定，
    // 避免依赖真实 LICENSE 文件的行尾（CRLF/LF 随平台/autocrlf 变化）。
    url file= url_temp ();
    string_save ("hello world", file);
    string expected_sha256=
        "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9";
    string_eq (sha256_hexdigest (file), expected_sha256);
  }
  SUBCASE ("empty file") {
    url temp= url_temp ();
    string_save ("", temp);
    CHECK_EQ (file_size (temp), 0);
    string_eq (
        sha256_hexdigest (temp),
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  }
  SUBCASE ("invalid file") {
    string_eq (sha256_hexdigest (url_system ("https://mogan.app")), "");
    string_eq (sha256_hexdigest (url_system ("/path/to/not_exist")), "");
  }
}
