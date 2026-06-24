/** \file http_test.cpp
 *  \copyright GPLv3
 *  \details Unitests for http.
 *  \author Darcy Shen
 *  \date   2023
 */

#include "a_lolly_test.hpp"
#include "file.hpp"
#include "hashmap.hpp"
#include "lolly/io/http.hpp"

using namespace lolly::io;

TEST_CASE ("http::head") {
#ifndef OS_WASM
  http_tree r     = http_head ("https://httpbin.org/get");
  long status_code= open_box<long> (http_response_ref (r, STATUS_CODE)->data);
  CHECK_EQ (status_code, 200);
  auto hmap=
      open_box<hashmap<string, string>> (http_response_ref (r, HEADER)->data);
  string content_type= hmap ("content-type");
  string_eq (content_type, "application/json");
#endif
}

TEST_CASE ("http::get") {
#ifndef OS_WASM
  http_tree r     = http_get ("https://httpbin.org/get");
  long status_code= open_box<long> (http_response_ref (r, STATUS_CODE)->data);
  CHECK_EQ (status_code, 200);
  auto hmap=
      open_box<hashmap<string, string>> (http_response_ref (r, HEADER)->data);
  string content_type= hmap ("content-type");
  string_eq (content_type, "application/json");
#endif
}

TEST_CASE ("http::download") {
#ifndef OS_WASM
  url       from= url ("http://mirrors.ustc.edu.cn/gnu/GNUinfo/README");
  url       to  = url_temp_dir () * url ("README");
  http_tree r   = download (from, to);
  CHECK (file_size (to) > 0);
#endif
}
