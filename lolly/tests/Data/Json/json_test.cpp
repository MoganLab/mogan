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
  string_eq (json (true).dump (), raw_quote ("true"));
  string_eq (json (false).dump (), raw_quote ("false"));
  string_eq (json (1).dump (), "1");
  string_eq (json (2.11).dump (), "2.11");
  string_eq (json::json_null ().dump (), raw_quote ("null"));
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
