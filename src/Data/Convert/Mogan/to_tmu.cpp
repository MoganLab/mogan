
/******************************************************************************
 * MODULE     : to_tmu.cpp
 * DESCRIPTION: conversion of TeXmacs trees to the TMU file format
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2024  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "convert.hpp"
#include "tree_helper.hpp"

#include <lolly/data/numeral.hpp>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using lolly::data::binary_to_hexadecimal;
using moebius::drd::std_contains;

const string TMU_VERSION= "1.1.0";

/******************************************************************************
 * Conversion of TeXmacs trees to the present TeXmacs string format
 ******************************************************************************/

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

/******************************************************************************
 * Conversion of TeXmacs trees to TeXmacs strings
 ******************************************************************************/

string
mogan_tree_serialize (tree t) {
  tmu_writer tmw;
  tmw.write (t);
  tmw.flush ();
  tmw.buf << "\n";
  return tmw.buf;
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

  return mogan_tree_serialize (t);
}
