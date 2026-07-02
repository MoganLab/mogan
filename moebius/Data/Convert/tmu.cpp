
/******************************************************************************
 * MODULE     : tmu.cpp
 * DESCRIPTION: conversion between TeXmacs trees and the TMU file format
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2024  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "tmu.hpp"

#include "analyze.hpp"
#include "path.hpp"
#include "tree_helper.hpp"

#include <lolly/data/numeral.hpp>
#include <lolly/data/unicode.hpp>
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

// moebius 编译单元看不到主工程的 tm_configure.hpp,这里给版本宏占位定义。
// 版本号锁死到主工程当前值(TEXMACS_VERSION "2.1.4" / XMACS_VERSION
// "2026.1.2"),主工程升级时需同步。后续可单独 PR 把版本号参数化。
#ifndef XMACS_VERSION
#define XMACS_VERSION "2026.3.0"
#endif
#ifndef TEXMACS_VERSION
#define TEXMACS_VERSION "2.1.4"
#endif

using lolly::data::binary_to_hexadecimal;
using lolly::data::decode_from_utf8;
using lolly::data::from_hex;
using lolly::data::to_Hex;
using moebius::drd::STD_CODE;
using moebius::drd::std_contains;

using namespace moebius;

/******************************************************************************
 * Version-dependent feature codes (used by tmu_reader)
 ******************************************************************************/

static void
rename_feature (hashmap<string, int>& H, string old_name, string new_name) {
  H (old_name)= H[new_name];
  H->reset (new_name);
}

static void
new_feature (hashmap<string, int>& H, string new_name) {
  H->reset (new_name);
}

static hashmap<string, int>
get_codes (string version) {
  hashmap<string, int> H (UNKNOWN);
  H->join (STD_CODE);

  if (version_inf ("1.0.7.6", version)) return H;

  rename_feature (H, "group", "rigid");
  rename_feature (H, "postscript", "image");

  if (version_inf ("1.0.6.9", version)) return H;

  rename_feature (H, "frozen", "freeze");

  if (version_inf ("1.0.6.2", version)) return H;

  new_feature (H, "expand-as");
  new_feature (H, "locus");
  new_feature (H, "id");
  new_feature (H, "hard-id");
  new_feature (H, "link");
  new_feature (H, "url");
  new_feature (H, "script");

  if (version_inf ("1.0.4.1", version)) return H;

  new_feature (H, "copy");
  new_feature (H, "cm-length");
  new_feature (H, "mm-length");
  new_feature (H, "in-length");
  new_feature (H, "pt-length");
  new_feature (H, "bp-length");
  new_feature (H, "dd-length");
  new_feature (H, "pc-length");
  new_feature (H, "cc-length");
  new_feature (H, "fs-length");
  new_feature (H, "fbs-length");
  new_feature (H, "em-length");
  new_feature (H, "ln-length");
  new_feature (H, "sep-length");
  new_feature (H, "yfrac-length");
  new_feature (H, "ex-length");
  new_feature (H, "fn-length");
  new_feature (H, "fns-length");
  new_feature (H, "bls-length");
  new_feature (H, "spc-length");
  new_feature (H, "xspc-length");
  new_feature (H, "par-length");
  new_feature (H, "pag-length");
  new_feature (H, "tmpt-length");
  new_feature (H, "px-length");
  new_feature (H, "tmlen");

  if (version_inf ("1.0.3.12", version)) return H;

  new_feature (H, "unquote*");

  if (version_inf ("1.0.3.4", version)) return H;

  new_feature (H, "for-each");
  new_feature (H, "quasi");
  rename_feature (H, "hold", "quasiquote");
  rename_feature (H, "release", "unquote");

  if (version_inf ("1.0.3.3", version)) return H;

  new_feature (H, "quote-value");
  new_feature (H, "quote-arg");
  new_feature (H, "mark");
  new_feature (H, "use-package");
  new_feature (H, "style-only");
  new_feature (H, "style-only*");
  new_feature (H, "rewrite-inactive");
  new_feature (H, "inline-tag");
  new_feature (H, "open-tag");
  new_feature (H, "middle-tag");
  new_feature (H, "close-tag");

  if (version_inf ("1.0.2.8", version)) return H;

  rename_feature (H, "raw_data", "raw-data");
  rename_feature (H, "sub_table", "subtable");
  rename_feature (H, "drd_props", "drd-props");
  rename_feature (H, "get_label", "get-label");
  rename_feature (H, "get_arity", "get-arity");
  rename_feature (H, "map_args", "map-args");
  rename_feature (H, "eval_args", "eval-args");
  rename_feature (H, "find_file", "find-file");
  rename_feature (H, "is_tuple", "is-tuple");
  rename_feature (H, "look_up", "look-up");
  rename_feature (H, "var_if", "if*");
  rename_feature (H, "var_inactive", "inactive*");
  rename_feature (H, "var_active", "active*");
  rename_feature (H, "text_at", "text-at");
  rename_feature (H, "var_spline", "spline*");
  rename_feature (H, "old_matrix", "old-matrix");
  rename_feature (H, "old_table", "old-table");
  rename_feature (H, "old_mosaic", "old-mosaic");
  rename_feature (H, "old_mosaic_item", "old-mosaic-item");
  rename_feature (H, "var_expand", "expand*");
  rename_feature (H, "hide_expand", "hide-expand");

  rename_feature (H, "with_limits", "with-limits");
  rename_feature (H, "line_break", "line-break");
  rename_feature (H, "new_line", "new-line");
  rename_feature (H, "line_separator", "line-sep");
  rename_feature (H, "next_line", "next-line");
  rename_feature (H, "no_line_break", "no-break");
  rename_feature (H, "no_first_indentation", "no-indent");
  rename_feature (H, "enable_first_indentation", "yes-indent");
  rename_feature (H, "no_indentation_after", "no-indent*");
  rename_feature (H, "enable_indentation_after", "yes-indent*");
  rename_feature (H, "page_break_before", "page-break*");
  rename_feature (H, "page_break", "page-break");
  rename_feature (H, "no_page_break_before", "no-page-break*");
  rename_feature (H, "no_page_break_after", "no-page-break");
  rename_feature (H, "new_page_before", "new-page*");
  rename_feature (H, "new_page", "new-page");
  rename_feature (H, "new_double_page_before", "new-dpage*");
  rename_feature (H, "new_double_page", "new-dpage");

  if (version_inf ("1.0.2.5", version)) return H;

  new_feature (H, "compound");
  new_feature (H, "xmacro");
  new_feature (H, "get_label");
  new_feature (H, "get_arity");
  new_feature (H, "map_args");
  new_feature (H, "eval_args");
  new_feature (H, "drd_props");

  if (version_inf ("1.0.2.0", version)) return H;

  new_feature (H, "with_limits");
  new_feature (H, "line_break");
  new_feature (H, "new_line");
  new_feature (H, "line_separator");
  new_feature (H, "next_line");
  new_feature (H, "no_line_break");
  new_feature (H, "no_first_indentation");
  new_feature (H, "enable_first_indentation");
  new_feature (H, "no_indentation_after");
  new_feature (H, "enable_indentation_after");
  new_feature (H, "page_break_before");
  new_feature (H, "page_break");
  new_feature (H, "no_page_break_before");
  new_feature (H, "no_page_break_after");
  new_feature (H, "new_page_before");
  new_feature (H, "new_page");
  new_feature (H, "new_double_page_before");
  new_feature (H, "new_double_page");

  if (version_inf ("1.0.1.25", version)) return H;

  new_feature (H, "active");
  new_feature (H, "var_inactive");
  new_feature (H, "var_active");
  new_feature (H, "attr");

  if (version_inf ("1.0.0.20", version)) return H;

  new_feature (H, "text_at");

  if (version_inf ("1.0.0.19", version)) return H;

  new_feature (H, "find_file");

  if (version_inf ("1.0.0.14", version)) return H;

  rename_feature (H, "paragraph", "para");

  if (version_inf ("1.0.0.5", version)) return H;

  new_feature (H, "var_if");
  new_feature (H, "hide_expand");

  if (version_inf ("1.0.0.2", version)) return H;

  new_feature (H, "superpose");
  new_feature (H, "spline");
  new_feature (H, "var_spline");

  return H;
}

/******************************************************************************
 * Conversion of TMU strings of the present format to TeXmacs trees
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
          r << ((char) from_hex (buf (pos, pos + 2)));
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
 * Conversion of TeXmacs trees to TMU strings
 ******************************************************************************/

const string TMU_VERSION= "1.1.0";

struct tmu_writer {
  string buf; // the resulting string
  string spc; // "" or " "
  string tmp; // not yet flushed characters

  int  tab;      // number of tabs after CR
  bool spc_flag; // true if last printed character was a space or CR
  bool ret_flag; // true if last printed character was a CR

  tmu_writer ()
      : buf (""), spc (""), tmp (""), tab (0), spc_flag (true),
        ret_flag (true) {}

  void cr ();
  void flush ();
  void write_space ();
  void write_return ();
  void write (string s, bool flag= true, bool encode_space= false);
  void br (int indent= 0);
  void tag (string before, string s, string after);
  void apply (string func, array<tree> args);
  void write (tree t);
};

void
tmu_writer::cr () {
  int i, n= N (buf);
  for (i= n - 1; i >= 0; i--)
    if ((buf[i] != ' ') || ((i > 0) && (buf[i - 1] == '\\'))) break;
  if (i < n - 1) {
    buf= buf (0, i + 1);
    n  = n - N (buf);
    for (i= 0; i < n; i++)
      buf << "\\ ";
  }
  buf << '\n';
  for (i= 0; i < min (tab, 20); i++)
    buf << ' ';
}

void
tmu_writer::flush () {
  int i, m= N (spc), n= N (tmp);
  if ((m + n) == 0) return;
  buf << spc << tmp;
  spc= "";
  tmp= "";
}

void
tmu_writer::write_space () {
  if (spc_flag) tmp << "\\ ";
  else {
    flush ();
    spc= " ";
  }
  spc_flag= true;
  ret_flag= false;
}

void
tmu_writer::write_return () {
  if (ret_flag) {
    buf << "\\;\n";
    cr ();
  }
  else {
    if ((spc == " ") && (tmp == "")) {
      spc= "";
      tmp= "\\ ";
    }
    flush ();
    buf << "\n";
    cr ();
  }
  spc_flag= true;
  ret_flag= true;
}

void
tmu_writer::write (string s, bool flag, bool encode_space) {
  if (flag) {
    int i, n= N (s);
    for (i= 0; i < n; i++) {
      char c= s[i];
      if ((c == ' ') && (!encode_space)) write_space ();
      else {
        if (c == ' ') tmp << "\\ ";
        else if (c == '\\') tmp << "\\\\";
        else if (c == '<') tmp << "\\<";
        else if (c == '|') tmp << "\\|";
        else if (c == '>') tmp << "\\>";
        else tmp << c;
        spc_flag= false;
        ret_flag= false;
      }
    }
  }
  else {
    tmp << s;
    if (N (s) != 0) {
      spc_flag= false;
      ret_flag= false;
    }
  }
}

void
tmu_writer::br (int indent) {
  flush ();
  tab+= indent;
  int i;
  int buf_N= N (buf);
  for (i= buf_N - 1; i >= 0; i--) {
    if (buf[i] == '\n') return;
    if (buf[i] != ' ') {
      cr ();
      spc_flag= true;
      ret_flag= false;
      return;
    }
  }
}

void
tmu_writer::tag (string before, string s, string after) {
  write (before, false);
  write (s);
  write (after, false);
}

void
tmu_writer::apply (string func, array<tree> args) {
  int i, last, n= N (args);
  for (i= n - 1; i >= 0; i--)
    if (is_document (args[i]) || is_func (args[i], COLLECTION)) break;
  last= i;

  if (last >= 0) {
    for (i= 0; i <= n; i++) {
      bool flag=
          (i < n) && (is_document (args[i]) || is_func (args[i], COLLECTION));
      if (i == 0) {
        write ("<\\", false);
        write (func, true, true);
      }
      else if (i == last + 1) {
        write ("</", false);
        write (func, true, true);
      }
      else if (is_document (args[i - 1]) || is_func (args[i - 1], COLLECTION)) {
        write ("<|", false);
        write (func, true, true);
      }
      if (i == n) {
        write (">", false);
        break;
      }

      if (flag) {
        write (">", false);
        br (2);
        write (args[i]);
        br (-2);
      }
      else {
        write ("|", false);
        write (args[i]);
      }
    }
  }
  else {
    write ("<", false);
    write (func, true, true);
    for (i= 0; i < n; i++) {
      write ("|", false);
      write (args[i]);
    }
    write (">", false);
  }
}

void
tmu_writer::write (tree t) {
  if (is_atomic (t)) {
    write (t->label);
    return;
  }

  int i, n= N (t);
  switch (L (t)) {
  case RAW_DATA: {
    write ("<#", false);
    string s= as_string (t[0]);
    write (binary_to_hexadecimal (s), false);
    write (">", false);
    break;
  }
  case DOCUMENT:
    spc_flag= true;
    ret_flag= true;
    for (i= 0; i < n; i++) {
      write (t[i]);
      if (i < (n - 1)) write_return ();
      else if (ret_flag) write ("\\;", false);
    }
    break;
  case CONCAT:
    for (i= 0; i < n; i++)
      write (t[i]);
    break;
  case EXPAND:
    if ((n >= 1) && is_atomic (t[0])) {
      string s= t[0]->label;
      if (std_contains (s))
        ;
      else if ((N (s) > 0) && (!is_iso_alpha (s)))
        ;
      else {
        apply (s, A (t (1, n)));
        break;
      }
    }
    apply (as_string (EXPAND), A (t));
    break;
  case COLLECTION:
    tag ("<\\", as_string (COLLECTION), ">");
    if (n == 0) br ();
    else {
      br (2);
      for (i= 0; i < n; i++) {
        write (t[i]);
        if (i < (n - 1)) br ();
      }
      br (-2);
    }
    tag ("</", as_string (COLLECTION), ">");
    break;
  default:
    apply (as_string (L (t)), A (t));
    break;
  }
}

string
tree_to_tmu (tree t) {
  if (!is_snippet (t)) {
    int  t_N= N (t);
    tree r (t, t_N);
    for (int i= 0; i < t_N; i++) {
      if (is_compound (t[i], "style", 1)) {
        tree style= t[i][0];
        if (is_func (style, TUPLE, 1)) style= style[0];
        r[i]   = copy (t[i]);
        r[i][0]= style;
      }
      else if (is_compound (t[i], "TeXmacs")) {
        r[i]= compound ("TMU", tuple (TMU_VERSION, string (XMACS_VERSION)));
      }
      else r[i]= t[i];
    }
    t= r;
  }

  tmu_writer tmw;
  tmw.write (t);
  tmw.flush ();
  tmw.buf << "\n"; // append an extra newline at the end of TMU file
  return tmw.buf;
}

/******************************************************************************
 * Conversion of TMU strings to TeXmacs trees (document-level)
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
