/******************************************************************************
 * MODULE     : scheme_ser.cpp
 * DESCRIPTION: serialize tree as scheme formatted text
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "lolly/data/base64.hpp"
#include "moebius/data/scheme.hpp"
#include "tree_helper.hpp"

namespace moebius {
namespace data {

string
scm_quote (string s) {
  // R5RS compliant external string representation.
  // 普通字符成段一次追加,转义字符单独处理
  int    i, n= N (s), run= 0;
  string r;
  r << '"';
  for (i= 0; i < n; i++)
    if (s[i] == '\"' || s[i] == '\\') {
      if (i > run) r << s (run, i);
      r << '\\' << s[i];
      run= i + 1;
    }
  if (n > run) r << s (run, n);
  r << '"';
  return r;
}

/******************************************************************************
 * Handling escape characters
 ******************************************************************************/

string
slash (string s) {
  // 普通字符成段一次追加,特殊字符单独转义
  int    i, n= N (s), run= 0;
  string r;
  for (i= 0; i < n; i++)
    switch (s[i]) {
    case '(':
    case ')':
    case ' ':
    case '\'':
      if (i > run) r << s (run, i);
      if ((n < 2) || (s[0] != '\042') || (s[n - 1] != '\042')) r << "\\";
      r << s[i];
      run= i + 1;
      break;
    case '\\':
      if (i > run) r << s (run, i);
      r << '\\' << s[i];
      run= i + 1;
      break;
    case '\042':
      if (((i == 0) && (s[n - 1] == '\042')) ||
          ((i == (n - 1)) && (s[0] == '\042'))) {
        ; // 首尾引号原样保留,并入当前 run
      }
      else {
        if (i > run) r << s (run, i);
        r << "\\" << s[i];
        run= i + 1;
      }
      break;
    case ((char) 0):
      if (i > run) r << s (run, i);
      r << "\\0";
      run= i + 1;
      break;
    case '\t':
      if (i > run) r << s (run, i);
      r << "\\t";
      run= i + 1;
      break;
    case '\n':
      if (i > run) r << s (run, i);
      r << "\\n";
      run= i + 1;
      break;
    default:
      break;
    }
  if (n > run) r << s (run, n);
  return r;
}

/******************************************************************************
 * Converting scheme trees to strings
 ******************************************************************************/

static void
scheme_tree_to_string (string& out, scheme_tree p) {
  if (!is_tuple (p)) {
    string s= p->label;
    if (is_quoted (s)) out << scm_quote (raw_unquote (s));
    else out << slash (s);
  }
  else {
    if (is_tuple (p, "\'", 1)) {
      out << "\'";
      scheme_tree_to_string (out, p[1]);
    }
    else {
      int i, n= N (p);
      out << "(";
      for (i= 0; i < n; i++) {
        if (i > 0) out << " ";
        scheme_tree_to_string (out, p[i]);
      }
      out << ")";
    }
  }
}

string
scheme_tree_to_string (scheme_tree p) {
  string out;
  scheme_tree_to_string (out, p);
  return out;
}

string
scheme_tree_to_block (scheme_tree p) {
  string out;
  int    i, n= N (p);
  for (i= 0; i < n; i++)
    out << scheme_tree_to_string (p[i]) << "\n";
  return out;
}

scheme_tree
tree_to_scheme_tree (tree t) {
  if (is_atomic (t)) return scm_quote (t->label);
  else if (is_func (t, EXPAND) && is_atomic (t[0])) {
    int  i, n= N (t);
    tree u (TUPLE, n);
    u[0]= copy (t[0]);
    for (i= 1; i < n; i++)
      u[i]= tree_to_scheme_tree (t[i]);
    return u;
  }
  else if (is_func (t, RAW_DATA)) {
    // cout << "DEBUG: RAW_DATA tree content:" << "\n";
    // print_tree(t, 0);

    tree   u (TUPLE, 2);
    string s= as_string (L (t));
    u[0]    = copy (s);
    u[1]    = lolly::data::encode_base64 (t[0]->label);

    u[1]= tree_to_scheme_tree (u[1]);
    // cout << "DEBUG: u: " << "\n";
    // print_tree(u, 0);
    return u;
  }
  else {
    int    i, n= N (t);
    tree   u (TUPLE, n + 1);
    string s= as_string (L (t));
    if (N (s) > 0 && is_digit (s[0]))
      if (is_int (s)) s= "'" * s;
    u[0]= copy (s);
    for (i= 0; i < n; i++)
      u[i + 1]= tree_to_scheme_tree (t[i]);
    return u;
  }
}

string
tree_to_scheme (tree t) {
  return scheme_tree_to_string (tree_to_scheme_tree (t));
}

} // namespace data
} // namespace moebius
