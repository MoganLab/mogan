/******************************************************************************
 * MODULE     : json_serde.cpp
 * DESCRIPTION: JSON serialization/deserialization for modification and patch,
 *              enabling network transport for collaborative editing
 * COPYRIGHT  : (C) 2026  cc-fuyu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "moebius/data/json_serde.hpp"
#include "moebius/data/scheme.hpp"
#include "path.hpp"
#include "tree.hpp"

namespace moebius {
namespace data {

/******************************************************************************
 * Path serialization
 ******************************************************************************/

string
path_to_json_string (path p) {
  string r;
  r << "[";
  bool first= true;
  path cur  = p;
  while (!is_nil (cur)) {
    if (!first) r << ",";
    r << as_string (cur->item);
    first= false;
    cur  = cur->next;
  }
  r << "]";
  return r;
}

path
json_string_to_path (string s) {
  int n= N (s);
  if (n < 2 || s[0] != '[' || s[n - 1] != ']') return path ();
  string inner= s (1, n - 1);
  if (N (inner) == 0) return path ();
  path  head;
  path* tail= &head;
  int   i   = 0;
  int   in  = N (inner);
  while (i < in) {
    // skip whitespace
    while (i < in && inner[i] == ' ')
      i++;
    // parse integer
    int start= i;
    if (i < in && inner[i] == '-') i++;
    while (i < in && inner[i] >= '0' && inner[i] <= '9')
      i++;
    string num_str= inner (start, i);
    int    num    = as_int (num_str);
    *tail         = path (num);
    tail          = &((*tail)->next);
    // skip comma
    while (i < in && (inner[i] == ',' || inner[i] == ' '))
      i++;
  }
  return head;
}

/******************************************************************************
 * Tree serialization (using scheme format as transport)
 ******************************************************************************/

string
tree_to_json_string (tree t) {
  return tree_to_scheme (t);
}

tree
json_string_to_tree (string s) {
  return scheme_to_tree (s);
}

/******************************************************************************
 * JSON string escaping helpers
 ******************************************************************************/

static string
json_escape (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++) {
    char c= s[i];
    if (c == '"') r << "\\\"";
    else if (c == '\\') r << "\\\\";
    else if (c == '\n') r << "\\n";
    else if (c == '\r') r << "\\r";
    else if (c == '\t') r << "\\t";
    else r << c;
  }
  return r;
}

static string
json_unescape (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++) {
    if (s[i] == '\\' && i + 1 < n) {
      i++;
      if (s[i] == '"') r << '"';
      else if (s[i] == '\\') r << '\\';
      else if (s[i] == 'n') r << '\n';
      else if (s[i] == 'r') r << '\r';
      else if (s[i] == 't') r << '\t';
      else {
        r << '\\';
        r << s[i];
      }
    }
    else r << s[i];
  }
  return r;
}

/******************************************************************************
 * Modification to JSON
 ******************************************************************************/

string
modification_to_json (modification mod) {
  string type_str= get_type (mod);
  string path_str= path_to_json_string (mod->p);
  string tree_str= tree_to_json_string (mod->t);

  string r;
  r << "{\"type\":\"" << json_escape (type_str) << "\"";
  r << ",\"path\":" << path_str;
  r << ",\"tree\":\"" << json_escape (tree_str) << "\"";
  r << "}";
  return r;
}

/******************************************************************************
 * JSON to Modification - minimal parser
 ******************************************************************************/

// Extract value for a given key from a simple flat JSON object
static string
json_extract_value (string json, string key) {
  string search;
  search << "\"" << key << "\":\"";
  int pos= -1;
  int n  = N (json);
  int sn = N (search);
  for (int i= 0; i + sn <= n; i++) {
    bool match= true;
    for (int j= 0; j < sn; j++) {
      if (json[i + j] != search[j]) {
        match= false;
        break;
      }
    }
    if (match) {
      pos= i + sn;
      break;
    }
  }
  if (pos < 0) return "";

  // Find closing quote (handling escapes)
  string val;
  for (int i= pos; i < n; i++) {
    if (json[i] == '\\' && i + 1 < n) {
      val << json[i];
      val << json[i + 1];
      i++;
    }
    else if (json[i] == '"') break;
    else val << json[i];
  }
  return json_unescape (val);
}

// Extract a JSON array value (e.g. [0,1,3]) for a given key
static string
json_extract_array (string json, string key) {
  string search;
  search << "\"" << key << "\":[";
  int pos= -1;
  int n  = N (json);
  int sn = N (search);
  for (int i= 0; i + sn <= n; i++) {
    bool match= true;
    for (int j= 0; j < sn; j++) {
      if (json[i + j] != search[j]) {
        match= false;
        break;
      }
    }
    if (match) {
      pos= i + sn - 1; // point to '['
      break;
    }
  }
  if (pos < 0) return "[]";
  // Find matching ']'
  for (int i= pos + 1; i < n; i++) {
    if (json[i] == ']') {
      return json (pos, i + 1);
    }
  }
  return "[]";
}

modification
json_to_modification (string s) {
  string type_str= json_extract_value (s, "type");
  string path_str= json_extract_array (s, "path");
  string tree_str= json_extract_value (s, "tree");
  path   p       = json_string_to_path (path_str);
  tree   t       = json_string_to_tree (tree_str);
  return make_modification (type_str, p, t);
}

} // namespace data
} // namespace moebius
