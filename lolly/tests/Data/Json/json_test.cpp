/** \file json_test.cpp
 *  \copyright GPLv3
 *  \details Unitests for json
 *  \author Darcy Shen
 *  \date   2023
 */

#include "a_lolly_test.hpp"
#include "analyze.hpp"
#include "lolly/data/json.hpp"
#include "string.hpp"
#include "tm_ostream.hpp"

using json= lolly::data::json;

TEST_CASE ("as_string") {
  string_eq (json ().dump (), "{}");
  string_eq (json ("string").dump (), raw_quote ("string"));
  string_eq (json (true).dump (), "true");
  string_eq (json (false).dump (), "false");
  string_eq (json (1).dump (), "1");
  string_eq (json (2.11).dump (), "2.11");
  string_eq (json::json_null ().dump (), "null");
}

TEST_CASE ("access") {
  json j= json ();
  j.set ("name", "Bob");
  string_eq (as_string (j ("name")), "Bob");
  j.set ("name", "John");
  string_eq (as_string (j ("name")), "John");
  j.set ("age", 12);
  string_eq (j.dump (), "{\"name\":\"John\",\"age\":12}");
}

TEST_CASE ("escape") {
  string_eq (json ("a\"b").dump (), "\"a\\\"b\"");
  string_eq (json ("a\\b").dump (), "\"a\\\\b\"");
  string_eq (json ("a\nb").dump (), "\"a\\nb\"");
  string_eq (json ("\x01").dump (), "\"\\u0001\"");
}

TEST_CASE ("read") {
  json j= json::read ("{\"a\":\"1\",\"b\":\"2\"}");
  string_eq (as_string (j ("a")), "1");
  string_eq (as_string (j ("b")), "2");

  // dump 往返
  string_eq (json::read (j.dump ()).dump (), j.dump ());

  // 转义与 \uXXXX 解码
  string_eq (as_string (json::read ("\"a\\\"b\"")), "a\"b");
  string_eq (as_string (json::read ("\"\\u00e9\"")), "\xC3\xA9");

  // 数组 / 数字 / 布尔 / null
  json arr= json::read ("[1,2,3]");
  CHECK (arr.is_array ());
  string_eq (arr.dump (), "[1,2,3]");

  json obj= json::read ("{\"n\":42,\"f\":2.5,\"b\":true,\"z\":null}");
  CHECK (obj ("n").is_number ());
  CHECK (obj ("b").is_bool ());
  CHECK (obj ("z").is_null ());

  // 非法输入降级为空 object
  CHECK (json::read ("{oops}").is_object ());
}
