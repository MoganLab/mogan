/******************************************************************************
 * MODULE     : scheme_der.cpp
 * DESCRIPTION: parse scheme formatted text into Texmacs trees
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "lolly/data/base64.hpp"
#include "moebius/data/scheme.hpp"
#include "moebius/drd/drd_std.hpp"
#include "path.hpp"
#include "tree_helper.hpp"

using moebius::TUPLE;

namespace moebius {
namespace data {

string
scm_unquote (string s) {
  if (is_quoted (s)) {
    int    i, n= N (s);
    string r;
    for (i= 1; i < n - 1; i++)
      if (s[i] == '\\' &&
          (s[i + 1] == '\\' || (s[i + 1] == '\"' && i + 2 != n)))
        r << s[++i];
      else r << s[i];
    return r;
  }
  else return s;
}

/******************************************************************************
 * Handling escape characters
 ******************************************************************************/
void
unslash (string& s, int i, int end_index, string& r, int& r_index) {
  // 循环顶先判界再读字符:原实现末尾的 ch= s[i] 会越界读一字节
  while (i < end_index) {
    char ch= s[i];
    if ((ch == '\\') && ((i + 1) < end_index)) {
      i++;
      ch= s[i];
      switch (ch) {
      case '0':
        r[r_index]= ((char) 0);
        r_index++;
        break;
      case 'n':
        r[r_index]= '\n';
        r_index++;
        break;
      case 't':
        r[r_index]= '\t';
        r_index++;
        break;
      default:
        r[r_index]= ch;
        r_index++;
      }
    }
    else {
      r[r_index]= ch;
      r_index++;
    }
    i++;
  }
}

// Character type flags for fast classification via lookup table
enum {
  CT_SPC  = 1 << 0, // whitespace: ' ', '\t', '\n'
  CT_PAREN= 1 << 1, // parenthesis: '(', ')'
  CT_ESC  = 1 << 2, // backslash: '\\'
  CT_QUOTE= 1 << 3, // double quote: '\"'
  CT_SEMI = 1 << 4  // semicolon: ';'
};

static unsigned char char_type[256];
static bool          char_type_init= false;

static void
init_char_type () {
  char_type[(unsigned char) ' ']|= CT_SPC | CT_PAREN;
  char_type[(unsigned char) '\t']|= CT_SPC | CT_PAREN;
  char_type[(unsigned char) '\n']|= CT_SPC | CT_PAREN;
  char_type[(unsigned char) '(']|= CT_PAREN;
  char_type[(unsigned char) ')']|= CT_PAREN;
  char_type[(unsigned char) '\\']|= CT_ESC;
  char_type[(unsigned char) '\"']|= CT_QUOTE;
  char_type[(unsigned char) ';']|= CT_SEMI;
  char_type_init= true;
}

static void
skip_scheme_blanks (string& s, int& i, const int length) {
  unsigned char* types= char_type;
  while (i < length) {
    unsigned char t= types[(unsigned char) s[i]];
    if (!(t & (CT_SPC | CT_SEMI))) break;
    if (t & CT_SEMI) {
      i++;
      while (i < length && s[i] != '\n')
        i++;
    }
    else i++;
  }
}

static scheme_tree
string_to_scheme_tree (string& s, int& i, const int length) {
  for (; i < length; i++)
    switch (s[i]) {

    case ' ':
    case '\t':
    case '\n':
      break;
    case '(': {
      scheme_tree p (TUPLE);
      i++;
      while (true) {
        skip_scheme_blanks (s, i, length);
        if ((i == length) || (s[i] == ')')) break;
        p << string_to_scheme_tree (s, i, length);
      }
      if (i < length) i++;
      return p;
    }

    case '\'':
      i++;
      return scheme_tree (TUPLE, tree ("\'"),
                          string_to_scheme_tree (s, i, length));

    case '\"': { // "
      i++;
      int            end_index  = i;
      const int      start_index= i;
      char           ch;
      unsigned char* types= char_type;
      // 先判界再读字符:原实现的 ch= s[end_index] 会越界读末尾一字节
      while (end_index < length) {
        ch= s[end_index];
        if (ch == '\"') break;
        if (types[(unsigned char) ch] & CT_ESC) {
          if (end_index < length - 1) end_index++;
        }
        end_index++;
      }
      const int r_size      = 1; // N ("\"");
      int       quoted_index= r_size;
      string    quoted (r_size + end_index - i);
      quoted[0]= '"';
      unslash (s, start_index, end_index, quoted, quoted_index);
      if (i < length) i= end_index + 1;
      else i= end_index;
      quoted->resize (quoted_index + 1);
      quoted[quoted_index]= '"';
      return scheme_tree (quoted);
    }

    case ';':
      while ((i < length) && (s[i] != '\n'))
        i++;
      break;

    default: {
      int            end_index  = i;
      const int      start_index= i;
      char           ch;
      unsigned char* types= char_type;
      // 先判界再读字符,避免词元结尾在缓冲区末尾时越界读
      while (end_index < length) {
        ch= s[end_index];
        if (types[(unsigned char) ch] & (CT_SPC | CT_PAREN)) break;
        if (types[(unsigned char) ch] & CT_ESC) {
          if (end_index < length - 1) end_index++;
        }
        end_index++;
      }
      const int r_size     = 0; // empty string
      int       token_index= r_size;
      string    token (r_size + end_index - i);
      unslash (s, start_index, end_index, token, token_index);
      i= end_index;
      token->resize (token_index);
      return scheme_tree (token);
    }
    }

  return scheme_tree ("");
}

scheme_tree
string_to_scheme_tree (string s) {
  if (!char_type_init) init_char_type ();
  // 含 CR 时才做整串替换拷贝
  bool has_cr= false;
  for (int k= 0; k < N (s); k++)
    if (s[k] == '\015') {
      has_cr= true;
      break;
    }
  if (has_cr) s= replace (s, "\015", "");
  int       i     = 0;
  const int length= N (s);
  return string_to_scheme_tree (s, i, length);
}

scheme_tree
block_to_scheme_tree (string s) {
  if (!char_type_init) init_char_type ();
  scheme_tree p (TUPLE);
  int         i     = 0;
  const int   length= N (s);
  skip_scheme_blanks (s, i, length);
  while (i < length && s[i] == ')')
    i++;
  while (i < length) {
    p << string_to_scheme_tree (s, i, length);
    skip_scheme_blanks (s, i, length);
    while (i < length && s[i] == ')')
      i++;
  }
  return p;
}

tree
scheme_tree_to_tree (scheme_tree t, hashmap<string, int> codes, bool flag) {
  if (is_atomic (t)) return scm_unquote (t->label);
  else if ((N (t) == 0) || is_compound (t[0])) {
    return compound (
        "errput", concat ("The tree was ", as_string (L (t)), ": ", tree (t)));
  }
  else {
    int        i, n= N (t);
    tree_label code= (tree_label) codes[t[0]->label];
    if (flag) code= make_tree_label (t[0]->label);
    if (code == UNKNOWN) {
      tree u (EXPAND, n);
      u[0]= copy (t[0]);
      for (i= 1; i < n; i++)
        u[i]= scheme_tree_to_tree (t[i], codes, flag);
      return u;
    }
    else if (code == RAW_DATA) {

      tree   u (RAW_DATA, 1);
      string base64_data= scm_unquote (t[1]->label);
      string binary_data= lolly::data::decode_base64 (base64_data);
      u[0]->label       = binary_data;
      return u;
    }
    else {
      tree u (code, n - 1);
      for (i= 1; i < n; i++)
        u[i - 1]= scheme_tree_to_tree (t[i], codes, flag);
      return u;
    }
  }
}

tree
scheme_tree_to_tree (scheme_tree t) {
  return scheme_tree_to_tree (t, drd::STD_CODE, true);
}

tree
scheme_tree_to_tree (scheme_tree t, string version) {
  version= scm_unquote (version);
  tree doc, error (moebius::ERROR, "bad format or data");
  // if (version_inf (version, "1.0.2.4"))
  //   doc= scheme_tree_to_tree (t, get_codes (version), false);
  // else doc= scheme_tree_to_tree (t);
  doc= scheme_tree_to_tree (t);
  if (!is_document (doc)) return error;
  // return upgrade (doc, version);
  return doc;
}

tree
scheme_to_tree (string s) {
  return scheme_tree_to_tree (string_to_scheme_tree (s));
}

tree
scheme_document_to_tree (string s) {
  tree error (moebius::ERROR, "bad format or data");
  // 允许文件以 ; 行注释（如版权头）开头，先跳过再找 document 前缀
  int       start= 0;
  const int s_N  = N (s);
  while (start < s_N) {
    if (s[start] == ';') {
      while (start < s_N && s[start] != '\n')
        start++;
    }
    else if (is_space (s[start])) start++;
    else break;
  }
  if (start > 0) s= s (start, s_N);
  if (starts (s, "(document (apply \"TeXmacs\" ") ||
      starts (s, "(document (expand \"TeXmacs\" ") ||
      starts (s, "(document (TeXmacs ")) {
    int i, begin= 27;
    if (starts (s, "(document (expand \"TeXmacs\" ")) begin= 28;
    if (starts (s, "(document (TeXmacs ")) begin= 19;
    for (i= begin; i < N (s); i++)
      if (s[i] == ')') break;
    string version= s (begin, i);
    tree   t      = string_to_scheme_tree (s);
    return scheme_tree_to_tree (t, version);
  }
  return error;
}

} // namespace data
} // namespace moebius
