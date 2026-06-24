
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
#include "sys_utils.hpp"

using lolly::hash::sha224_hexdigest;
using lolly::hash::sha256_hexdigest;

TEST_CASE ("sha224_hexdigest") {
  SUBCASE ("normal file") {
    string expected_sha224;
    if (os_win ()) {
      expected_sha224= "d62ef1c7dea18146aba76ae54edc492502102a4a1c525d38ecf3612"
                       "9"; // Windows (CRLF) 版本的实际SHA224值
    }
    else {
      expected_sha224= "2f051189f6ddb2d2fd11faeb8cf3e3b38b4216f2798ce1d92421e12"
                       "9"; // Linux (LF)
                            // 版本的SHA224值，需要在Linux环境下实际计算
    }
    string_eq (sha224_hexdigest (url_pwd () * "LICENSE"), expected_sha224);
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
    string expected_sha256;
    if (os_win ()) {
      expected_sha256= "0b383d5a63da644f628d99c33976ea6487ed89aaa59f0b3257992de"
                       "ac1171e6b"; // Windows (CRLF) 版本的实际SHA256值
    }
    else {
      expected_sha256= "8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c54"
                       "5eb65b903"; // Linux (LF)
                                    // 版本的SHA256值，需要在Linux环境下实际计算
    }
    string_eq (sha256_hexdigest (url_pwd () * "LICENSE"), expected_sha256);
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
