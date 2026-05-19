
/******************************************************************************
 * MODULE     : from_tmu.cpp
 * DESCRIPTION: Convertion from the TMU format
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2024  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "convert.hpp"
#include "path.hpp"
#include "preferences.hpp"
#include "tree_helper.hpp"

#include <lolly/data/numeral.hpp>
#include <lolly/data/unicode.hpp>
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using lolly::data::decode_from_utf8;
using lolly::data::from_hex;
using lolly::data::to_Hex;
using moebius::drd::STD_CODE;

using namespace moebius;

static inline int
from_hex_char (char c) {
  if (is_digit (c)) return (int) (c - '0');
  if ((c >= 'A') && (c <= 'F')) return (int) (c + 10 - 'A');
  if ((c >= 'a') && (c <= 'f')) return (int) (c + 10 - 'a');
  return 0;
}

/******************************************************************************
 * Conversion of TeXmacs strings of the present format to TeXmacs trees
 ******************************************************************************/

struct tmu_reader {
  string               version; // document was composed using this version
  hashmap<string, int> codes;   // codes for to present version
  string               buf;     // the string being read from
  int                  pos;     // the current position of the reader
  string               last;    // last read string

  tmu_reader (string buf2)
      : version (XMACS_VERSION), codes (STD_CODE), buf (buf2), pos (0),
        last ("") {}
  tmu_reader (string buf2, string version2)
      : version (version2), codes (get_codes (version)), buf (buf2), pos (0),
        last ("") {}

  int    skip_blank ();
  string decode (string s);
  string read_char ();
  string read_next ();
  string read_function_name ();
  tree   read_apply (string s, bool skip_flag);
  tree   read (bool skip_flag);
};

int
tmu_reader::skip_blank () {
  int n= 0, buf_N= N (buf);
  for (; pos < buf_N; pos++) {
    if (buf[pos] == ' ') continue;
    if (buf[pos] == '\t') continue;
    if (buf[pos] == '\r') continue;
    if (buf[pos] == '\n') {
      n++;
      continue;
    }
    break;
  }
  return n;
}

string
tmu_reader::decode (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if (((i + 1) < n) && (s[i] == '\\')) {
      i++;
      if (s[i] == ';')
        ;
      else if (s[i] == '\\') r << '\\';
      else r << s[i];
    }
    else r << s[i];
  return r;
}

string
tmu_reader::read_char () {
  int buf_N= N (buf);
  while (((pos + 1) < buf_N) && (buf[pos] == '\\') && (buf[pos + 1] == '\n')) {
    pos+= 2;
    skip_spaces (buf, pos);
  }
  if (pos >= buf_N) return "";

  int start_pos= pos;
  decode_from_utf8 (buf, pos);
  return buf (start_pos, pos);
}

string
tmu_reader::read_next () {
  int    buf_N  = N (buf);
  int    old_pos= pos;
  string c      = read_char ();
  if (c == "") return c;
  switch (c[0]) {
  case '\t':
  case '\n':
  case '\r':
  case ' ':
    pos--;
    if (skip_blank () <= 1) return " ";
    else return "\n";
  case '<': {
    old_pos= pos;
    c      = read_char ();
    if (c == "") return "";
    if (c == "#") return "<#";
    if ((c == "\\") || (c == "|") || (c == "/")) return "<" * c;
    if (is_iso_alpha (c[0]) || (c == ">")) {
      pos= old_pos;
      return "<";
    }
    pos= old_pos;
    return "<";
    /*
    string d= read_char ();
    if ((d == "\\") || (d == "|") || (d == "/")) return "<" * c * d;
    pos= old_pos;
    return "<" * c;
    */
  }
  case '|':
  case '>':
    return c;
  }

  string r;
  pos= old_pos;
  while (true) {
    // fast path: avoid creating temporary strings for ordinary characters
    while (pos < buf_N) {
      char ch= buf[pos];
      if (ch == '\t' || ch == '\r' || ch == '\n' || ch == ' ' ||
          ch == '<' || ch == '|' || ch == '>' || ch == '\\')
        break;
      if ((buf[pos] & 0x80) == 0) {
        r << buf[pos++];
      }
      else {
        int start_pos= pos;
        decode_from_utf8 (buf, pos);
        for (int i= start_pos; i < pos; i++)
          r << buf[i];
      }
    }
    if (pos >= buf_N) return r;

    old_pos= pos;
    c      = read_char ();
    if (c == "") return r;
    else if (c == "\\") {
      if ((pos < buf_N) && (buf[pos] == '\\')) {
        r << c << "\\";
        pos++;
      }
      else r << c << read_char ();
    }
    else if (c == "\t") break;
    else if (c == "\r") break;
    else if (c == "\n") break;
    else if (c == " ") break;
    else if (c == "<") break;
    else if (c == "|") break;
    else if (c == ">") break;
    else r << c;
  }
  pos= old_pos;
  return r;
}

string
tmu_reader::read_function_name () {
  string name= decode (read_next ());
  // cout << "==> " << name << "\n";
  while (true) {
    last= read_next ();
    // cout << "~~> " << last << "\n";
    if ((last == "") || (last == "|") || (last == ">")) break;
  }
  return name;
}

static void
get_collection (tree& u, tree t) {
  if (is_func (t, COLLECTION) || is_func (t, DOCUMENT) || is_func (t, CONCAT)) {
    for (const auto t_i : t) {
      get_collection (u, t_i);
    }
  }
  else if (is_compound (t)) u << t;
}

tree
tmu_reader::read_apply (string name, bool skip_flag) {
  // cout << "Read apply " << name << INDENT << LF;
  tree t (make_tree_label (name));
  if (codes->contains (name)) {
    // cout << "  " << name << " -> " << as_string ((tree_label) codes [name])
    // << "\n";
    t= tree ((tree_label) codes[name]);
  }

  bool closed= !skip_flag;
  int  buf_N = N (buf);
  while (pos < buf_N) {
    // cout << "last= " << last << LF;
    bool sub_flag= (skip_flag) && ((last == "") || (last[N (last) - 1] != '|'));
    if (sub_flag) (void) skip_blank ();
    t << read (sub_flag);
    if ((last == "/>") || (last == "/|")) closed= true;
    if (closed && ((last == ">") || (last == "/>"))) break;
  }
  // cout << "last= " << last << UNINDENT << LF;
  // cout << "Done" << LF;

  if (is_func (t, COLLECTION)) {
    tree u (COLLECTION);
    get_collection (u, t);
    return u;
  }
  return t;
}

static void
flush (tree& D, tree& C, string& S, bool& spc_flag, bool& ret_flag) {
  if (spc_flag) S << " ";
  if (S != "") {
    if ((N (C) == 0) || (!is_atomic (C[N (C) - 1]))) C << S;
    else C[N (C) - 1]->label << S;
    S       = "";
    spc_flag= false;
  }

  if (ret_flag) {
    if (N (C) == 0) D << "";
    else if (N (C) == 1) D << C[0];
    else D << C;
    C       = tree (CONCAT);
    ret_flag= false;
  }
}

tree
tmu_reader::read (bool skip_flag) {
  int    buf_N= N (buf);
  tree   D (DOCUMENT);
  tree   C (CONCAT);
  string S ("");
  bool   spc_flag= false;
  bool   ret_flag= false;

  while (true) {
    last= read_next ();
    // cout << "--> " << last << "\n";
    if (last == "") break;
    if (last == "|") break;
    if (last == ">") break;

    if (last[0] == '<') {
      char tail_char_of_last= last[N (last) - 1];
      if (tail_char_of_last == '\\') {
        flush (D, C, S, spc_flag, ret_flag);
        string name= read_function_name ();
        if (last == ">") last= "\\>";
        else last= "\\|";
        C << read_apply (name, true);
      }
      else if (tail_char_of_last == '|') {
        (void) read_function_name ();
        if (last == ">") last= "|>";
        else last= "||";
        break;
      }
      else if (tail_char_of_last == '/') {
        (void) read_function_name ();
        if (last == ">") last= "/>";
        else last= "/|";
        break;
      }
      else if (tail_char_of_last == '#') {
        string r;
        while ((buf[pos] != '>') && (pos + 2 < buf_N)) {
          r << ((char) ((from_hex_char (buf[pos]) << 4) +
                        from_hex_char (buf[pos + 1])));
          pos+= 2;
        }
        if (buf[pos] == '>') pos++;
        flush (D, C, S, spc_flag, ret_flag);
        C << tree (RAW_DATA, r);
        last= read_next ();
        break;
      }
      else {
        flush (D, C, S, spc_flag, ret_flag);
        string name= decode (read_next ());
        string sep = ">";
        if (name == ">") name= "";
        else sep= read_next ();
        // cout << "==> " << name << "\n";
        // cout << "~~> " << sep << "\n";
        if (sep == "|") {
          last= "|";
          C << read_apply (name, false);
        }
        else {
          tree t (make_tree_label (name));
          if (codes->contains (name)) {
            // cout << name << " -> " << as_string ((tree_label) codes [name])
            // << "\n";
            t= tree ((tree_label) codes[name]);
          }
          C << t;
        }
      }
    }
    else if (last == " ") spc_flag= true;
    else if (last == "\n") ret_flag= true;
    else {
      flush (D, C, S, spc_flag, ret_flag);
      // cout << "<<< " << last << "\n";
      // cout << ">>> " << decode (last) << "\n";
      S << decode (last);
      if ((S == "") && (N (C) == 0)) C << "";
    }
  }

  if (skip_flag) spc_flag= ret_flag= false;
  flush (D, C, S, spc_flag, ret_flag);
  if (N (C) == 1) D << C[0];
  else if (N (C) > 1) D << C;
  // cout << "*** " << D << "\n";
  if (N (D) == 0) return "";
  if (N (D) == 1) {
    if (!skip_flag) return D[0];
    if (is_func (D[0], COLLECTION)) return D[0];
  }
  return D;
}

tree
tmu_to_tree (string s) {
  tmu_reader tmr (s);
  return tmr.read (true);
}

tree
tmu_to_tree (string s, string version) {
  tmu_reader tmr (s, version);
  return tmr.read (true);
}

/******************************************************************************
 * Conversion of TeXmacs strings to TeXmacs trees
 ******************************************************************************/
tree
tmu_document_to_tree (string s) {
  tree error (ERROR, "bad format or data");

  if (starts (s, "<TMU|<tuple|")) {
    int           i            = index_of (s, '>');
    string        version_tuple= s (N ("<TMU|<tuple|"), i);
    array<string> version_arr  = tokenize (version_tuple, "|");
    string        tmu_version  = version_arr[0];
    string        xmacs_version= version_arr[1];
    tree          doc          = tmu_to_tree (s, xmacs_version);

    if (is_compound (doc, "TeXmacs", 1) || is_expand (doc, "TeXmacs", 1) ||
        is_apply (doc, "TeXmacs", 1))
      doc= tree (DOCUMENT, doc);

    if (!is_document (doc)) return error;

    if (N (doc) == 0 || !is_compound (doc[0], "TeXmacs", 1)) {
      tree d (DOCUMENT);
      d << compound ("TeXmacs", string (TEXMACS_VERSION));
      d << A (doc);
      doc= d;
    }

    return doc;
  }
  return error;
}
