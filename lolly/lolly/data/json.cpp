
/******************************************************************************
 * MODULE     : json.cpp
 * DESCRIPTION: Json Data Type
 * COPYRIGHT  : (C) 2023  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "json.hpp"
#include "analyze.hpp"

namespace lolly {
namespace data {

bool
json::contains (string key) {
  if (!is_object ()) return false;
  return rep->index->contains (key);
}

json
json::get (string key) {
  if (is_object ()) {
    if (rep->index->contains (key)) {
      int       i   = rep->index (key);
      json_tree pair= rep->t[i];
      return json (pair[1]);
    }
  }
  return json_null ();
}

json
json::operator() (string key) {
  return get (key);
}

void
json::set (string key, json value) {
  if (!is_object ()) return;
  if (contains (key)) {
    int       i   = rep->index (key);
    json_tree pair= rep->t[i];
    pair[1]       = value->t;
  }
  else {
    rep->index (key)= arity (rep->t);
    rep->t << json_tree (JSON_PAIR, key, value->t);
  }
}

// TODO: Use json escape and unescape
string
json::dump () {
  json_tree t= this->rep->t;
  if (this->is_string ()) {
    return raw_quote (as_string (t));
  }
  if (this->is_null ()) {
    return raw_quote ("null");
  }
  if (this->is_bool ()) {
    return raw_quote (as_string (t[0]));
  }
  if (this->is_number ()) {
    return as_string (t[0]);
  }
  if (t->op == JSON_ARRAY) {
    string ret= "[";
    for (int i= 0; i < arity (t); i++) {
      ret= ret * json (t[0]).dump ();
    }
    ret= ret * "]";
    return ret;
  }
  if (t->op == JSON_OBJECT) {
    string ret= "{";
    int    n  = arity (t);
    if (n > 0) {
      ret= ret * raw_quote (as_string (t[0][0]));
      ret= ret * ":";
      ret= ret * json (t[0][1]).dump ();
    }
    for (int i= 1; i < n; i++) {
      ret= ret * ",";
      ret= ret * raw_quote (as_string (t[i][0]));
      ret= ret * ":";
      ret= ret * json (t[i][1]).dump ();
    }
    ret= ret * "}";
    return ret;
  }
}

string
as_string (json j) {
  if (j.is_string ()) {
    return to_string (j->t);
  }
  TM_FAILED ("not a string");
}

} // namespace data
} // namespace lolly
