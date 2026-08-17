
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
#include "lolly/data/unicode.hpp"

namespace lolly {
namespace data {

/******************************************************************************
 * JSON 字符串转义与解析辅助
 ******************************************************************************/

static bool
json_ws (char c) {
  return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

static int
json_hex (char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// JSON 字符串转义：引号/反斜杠/控制字符转义，其余非 ASCII 保持字面 UTF-8
static string
json_escape (string s) {
  const char hex[]= "0123456789abcdef";
  string     r;
  int        n= N (s);
  for (int i= 0; i < n; i++) {
    char c= s[i];
    switch (c) {
    case '"':
      r << "\\\"";
      break;
    case '\\':
      r << "\\\\";
      break;
    case '\b':
      r << "\\b";
      break;
    case '\f':
      r << "\\f";
      break;
    case '\n':
      r << "\\n";
      break;
    case '\r':
      r << "\\r";
      break;
    case '\t':
      r << "\\t";
      break;
    default:
      if ((unsigned char) c < 0x20) {
        r << "\\u00";
        r << hex[(unsigned char) c >> 4];
        r << hex[(unsigned char) c & 0x0f];
      }
      else r << c;
    }
  }
  return r;
}

/******************************************************************************
 * Json 对象访问
 ******************************************************************************/

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

/******************************************************************************
 * Json 序列化
 ******************************************************************************/

string
json::dump () {
  json_tree t= this->rep->t;
  if (this->is_string ()) {
    return "\"" * json_escape (as_string (t)) * "\"";
  }
  if (this->is_null ()) {
    return "null";
  }
  if (this->is_bool ()) {
    return as_string (t[0]);
  }
  if (this->is_number ()) {
    return as_string (t[0]);
  }
  if (t->op == JSON_ARRAY) {
    string ret= "[";
    int    n  = arity (t);
    for (int i= 0; i < n; i++) {
      if (i > 0) ret << ",";
      ret << json (t[i]).dump ();
    }
    ret << "]";
    return ret;
  }
  if (t->op == JSON_OBJECT) {
    string ret= "{";
    int    n  = arity (t);
    for (int i= 0; i < n; i++) {
      if (i > 0) ret << ",";
      ret << "\"" << json_escape (as_string (t[i][0])) << "\"";
      ret << ":";
      ret << json (t[i][1]).dump ();
    }
    ret << "}";
    return ret;
  }
  return "null";
}

string
as_string (json j) {
  if (j.is_string ()) {
    return to_string (j->t);
  }
  TM_FAILED ("not a string");
}

/******************************************************************************
 * Json 解析
 ******************************************************************************/

namespace {
struct json_parser {
  string s;
  int    i;
  bool   failed;

  json_parser (string s0) : s (s0), i (0), failed (false) {}

  void skip_ws () {
    while (i < N (s) && json_ws (s[i]))
      i++;
  }
  bool match (const char* lit) {
    string l (lit);
    if (i + N (l) <= N (s) && s (i, i + N (l)) == l) {
      i+= N (l);
      return true;
    }
    return false;
  }

  json_tree parse_value () {
    skip_ws ();
    if (i >= N (s)) {
      failed= true;
      return json_tree ();
    }
    char c= s[i];
    if (c == '{') return parse_object ();
    if (c == '[') return parse_array ();
    if (c == '"') return parse_string ();
    if (c == 't' || c == 'f' || c == 'n') return parse_literal ();
    if (c == '-' || (c >= '0' && c <= '9')) return parse_number ();
    failed= true;
    return json_tree ();
  }

  json_tree parse_object () {
    i++; // skip '{'
    json_tree obj (JSON_OBJECT);
    skip_ws ();
    if (i < N (s) && s[i] == '}') {
      i++;
      return obj;
    }
    while (true) {
      skip_ws ();
      if (i >= N (s) || s[i] != '"') {
        failed= true;
        return json_tree ();
      }
      json_tree key= parse_string ();
      if (failed) return json_tree ();
      skip_ws ();
      if (i >= N (s) || s[i] != ':') {
        failed= true;
        return json_tree ();
      }
      i++; // skip ':'
      json_tree val= parse_value ();
      if (failed) return json_tree ();
      obj << json_tree (JSON_PAIR, key, val);
      skip_ws ();
      if (i < N (s) && s[i] == ',') {
        i++;
        continue;
      }
      if (i < N (s) && s[i] == '}') {
        i++;
        return obj;
      }
      failed= true;
      return json_tree ();
    }
  }

  json_tree parse_array () {
    i++; // skip '['
    json_tree arr (JSON_ARRAY);
    skip_ws ();
    if (i < N (s) && s[i] == ']') {
      i++;
      return arr;
    }
    while (true) {
      json_tree val= parse_value ();
      if (failed) return json_tree ();
      arr << val;
      skip_ws ();
      if (i < N (s) && s[i] == ',') {
        i++;
        continue;
      }
      if (i < N (s) && s[i] == ']') {
        i++;
        return arr;
      }
      failed= true;
      return json_tree ();
    }
  }

  json_tree parse_string () {
    i++; // skip '"'
    string r;
    while (i < N (s)) {
      char c= s[i];
      if (c == '"') {
        i++;
        return json_tree (r);
      }
      if (c == '\\') {
        i++;
        if (i >= N (s)) {
          failed= true;
          return json_tree ();
        }
        char e= s[i];
        i++;
        switch (e) {
        case '"':
          r << '"';
          break;
        case '\\':
          r << '\\';
          break;
        case '/':
          r << '/';
          break;
        case 'b':
          r << '\b';
          break;
        case 'f':
          r << '\f';
          break;
        case 'n':
          r << '\n';
          break;
        case 'r':
          r << '\r';
          break;
        case 't':
          r << '\t';
          break;
        case 'u': {
          if (i + 4 > N (s)) {
            failed= true;
            return json_tree ();
          }
          uint32_t code= 0;
          for (int k= 0; k < 4; k++) {
            int h= json_hex (s[i + k]);
            if (h < 0) {
              failed= true;
              return json_tree ();
            }
            code= (code << 4) | (uint32_t) h;
          }
          i+= 4;
          r << encode_as_utf8 (code);
          break;
        }
        default:
          failed= true;
          return json_tree ();
        }
      }
      else {
        r << c;
        i++;
      }
    }
    failed= true;
    return json_tree ();
  }

  json_tree parse_number () {
    int start= i;
    if (i < N (s) && s[i] == '-') i++;
    while (i < N (s) && s[i] >= '0' && s[i] <= '9')
      i++;
    bool is_float= false;
    if (i < N (s) && s[i] == '.') {
      is_float= true;
      i++;
      while (i < N (s) && s[i] >= '0' && s[i] <= '9')
        i++;
    }
    if (i < N (s) && (s[i] == 'e' || s[i] == 'E')) {
      is_float= true;
      i++;
      if (i < N (s) && (s[i] == '+' || s[i] == '-')) i++;
      while (i < N (s) && s[i] >= '0' && s[i] <= '9')
        i++;
    }
    if (start == i) {
      failed= true;
      return json_tree ();
    }
    string num= s (start, i);
    if (is_float) return json_tree (DOUBLE_TYPE, json_tree (num));
    return json_tree (INT64_TYPE, json_tree (num));
  }

  json_tree parse_literal () {
    if (match ("true")) return json_tree (BOOL_TYPE, json_tree ("true"));
    if (match ("false")) return json_tree (BOOL_TYPE, json_tree ("false"));
    if (match ("null")) return json_tree (NULL_TYPE);
    failed= true;
    return json_tree ();
  }
};
} // anonymous namespace

json
json::read (string s) {
  json_parser p (s);
  p.skip_ws ();
  if (p.i >= N (s)) return json ();
  json_tree t= p.parse_value ();
  if (p.failed) return json ();
  p.skip_ws ();
  if (p.i < N (s)) return json (); // 尾随垃圾视为非法
  json j (t);
  if (j.is_object ()) {
    // 解析出的对象 index 哈希表是空的，重建以便 contains/get 按键查询
    json_tree tt= j->t;
    for (int i= 0; i < lolly::data::arity (tt); i++)
      if (lolly::data::is_atomic (tt[i][0])) j->index (tt[i][0]->label)= i;
  }
  return j;
}

} // namespace data
} // namespace lolly
