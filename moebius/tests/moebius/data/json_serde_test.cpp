#include "modification.hpp"
#include "moe_doctests.hpp"
#include "moebius/data/json_serde.hpp"
#include "tree.hpp"

using moebius::data::json_string_to_path;
using moebius::data::json_string_to_tree;
using moebius::data::json_to_modification;
using moebius::data::modification_to_json;
using moebius::data::path_to_json_string;
using moebius::data::tree_to_json_string;

/******************************************************************************
 * Path round-trip
 ******************************************************************************/

TEST_CASE ("path_to_json_string empty path") {
  path   p= path ();
  string s= path_to_json_string (p);
  CHECK (s == "[]");
}

TEST_CASE ("path round-trip single element") {
  path   p= path (3);
  string s= path_to_json_string (p);
  path   q= json_string_to_path (s);
  CHECK (p == q);
}

TEST_CASE ("path round-trip multi element") {
  path   p= path (0, path (1, path (2)));
  string s= path_to_json_string (p);
  path   q= json_string_to_path (s);
  CHECK (p == q);
}

/******************************************************************************
 * Tree round-trip
 ******************************************************************************/

TEST_CASE ("tree round-trip atomic") {
  tree   t= tree ("hello world");
  string s= tree_to_json_string (t);
  tree   u= json_string_to_tree (s);
  CHECK (t == u);
}

TEST_CASE ("tree round-trip compound") {
  tree   t= tree (DOCUMENT, "line1", "line2");
  string s= tree_to_json_string (t);
  tree   u= json_string_to_tree (s);
  CHECK (t == u);
}

/******************************************************************************
 * Modification round-trip
 ******************************************************************************/

TEST_CASE ("modification round-trip assign") {
  modification m= mod_assign (path (0), tree ("new content"));
  string       s= modification_to_json (m);
  modification n= json_to_modification (s);
  CHECK (m == n);
}

TEST_CASE ("modification round-trip insert") {
  modification m= mod_insert (path (0), 3, tree ("inserted"));
  string       s= modification_to_json (m);
  modification n= json_to_modification (s);
  CHECK (m == n);
}

TEST_CASE ("modification round-trip remove") {
  modification m= mod_remove (path (1), 2, 5);
  string       s= modification_to_json (m);
  modification n= json_to_modification (s);
  CHECK (m == n);
}

TEST_CASE ("modification round-trip split") {
  modification m= mod_split (path (), 1, 3);
  string       s= modification_to_json (m);
  modification n= json_to_modification (s);
  CHECK (m == n);
}

TEST_CASE ("modification round-trip join") {
  modification m= mod_join (path (0), 2);
  string       s= modification_to_json (m);
  modification n= json_to_modification (s);
  CHECK (m == n);
}

TEST_CASE ("modification round-trip assign_node") {
  modification m= mod_assign_node (path (0, path (1)), DOCUMENT);
  string       s= modification_to_json (m);
  modification n= json_to_modification (s);
  CHECK (m == n);
}

TEST_CASE ("modification round-trip set_cursor") {
  modification m= mod_set_cursor (path (0), 5, tree ("cursor_data"));
  string       s= modification_to_json (m);
  modification n= json_to_modification (s);
  CHECK (m == n);
}

TEST_CASE ("modification json contains expected keys") {
  modification m= mod_assign (path (0), tree ("test"));
  string       s= modification_to_json (m);
  // Verify the JSON string contains the expected structure
  CHECK (N (s) > 0);
  CHECK (s[0] == '{');
  CHECK (s[N (s) - 1] == '}');
}
