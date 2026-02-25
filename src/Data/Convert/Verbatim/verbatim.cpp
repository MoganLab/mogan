
/******************************************************************************
 * MODULE     : verbatim.cpp
 * DESCRIPTION: routines for converting between TeXmacs trees and text
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "convert.hpp"
#include "converter.hpp"
#include "cork.hpp"
#include "locale.hpp"
#include "tree_helper.hpp"
#include "wencoding.hpp"
#include <moebius/drd/drd_std.hpp>
#include <moebius/vars.hpp>

using namespace moebius;
using moebius::drd::drd_env_read;
using moebius::drd::std_drd;
using moebius::drd::the_drd;

/******************************************************************************
 * TeXmacs to verbatim
 ******************************************************************************/

static void   print_verbatim (string& buf, tree t, bool wrap);
static string as_verbatim (tree t, bool wrap);

static void
print_verbatim_arg (string& buf, tree t, bool wrap) {
  string s= as_verbatim (t, wrap);
  if (tm_string_length (s) <= 1 || is_numeric (s) || is_iso_alpha (s))
    print_verbatim (buf, t, wrap);
  else {
    buf << "(";
    print_verbatim (buf, t, wrap);
    buf << ")";
  }
}

static int
get_width (string s) {
  if (occurs ("\n", s)) return 0;
  else return tm_string_length (s);
}

static void
print_verbatim_table (string& buf, tree t, bool wrap) {
  if (N (buf) > 0 && buf[N (buf) - 1] != '\n') buf << "\n";
  int  i, nr= N (t), j, nc= 0;
  tree tab (TUPLE, nr);
  for (i= 0; i < nr; i++) {
    tree row= t[i];
    if (is_func (row, CWITH)) row= row[N (row) - 1];
    if (!is_func (row, ROW)) tab[i]= tree (TUPLE);
    else {
      nc    = max (nc, N (row));
      tab[i]= tree (TUPLE, N (row));
      for (j= 0; j < N (row); j++)
        tab[i][j]= as_verbatim (row[j], wrap);
    }
  }
  array<int> w (nc);
  for (j= 0; j < nc; j++)
    w[j]= 0;
  for (i= 0; i < nr; i++) {
    for (j= 0; j < N (tab[i]); j++)
      w[j]= max (w[j], get_width (tab[i][j]->label));
  }
  for (i= 0; i < nr; i++) {
    for (j= 0; j < N (tab[i]); j++) {
      int spc= w[j] - get_width (tab[i][j]->label) + 1;
      buf << tab[i][j]->label;
      if (j != N (tab[i]) - 1)
        for (int k= 0; k < spc; k++)
          buf << " ";
    }
    buf << "\n";
  }
}

static void
print_verbatim (string& buf, tree t, bool wrap) {
  if (is_atomic (t)) buf << t->label;
  else switch (L (t)) {
    case SURROUND:
      print_verbatim (buf, t[0], wrap);
      print_verbatim (buf, t[2], wrap);
      print_verbatim (buf, t[1], wrap);
      break;
    case HSPACE:
    case SPACE:
      if (is_atomic (t[0]) && starts (t[0]->label, "-")) break;
    case HTAB:
      if (N (buf) > 0 && buf[N (buf) - 1] != '\n') buf << " ";
      break;
    case AROUND:
    case VAR_AROUND:
      print_verbatim (buf, t[0], wrap);
      print_verbatim (buf, t[1], wrap);
      print_verbatim (buf, t[2], wrap);
      break;
    case BIG_AROUND:
      print_verbatim (buf, t[0], wrap);
      print_verbatim (buf, t[1], wrap);
      break;
    case LEFT:
    case MID:
    case RIGHT:
    case BIG:
    case LPRIME:
    case RPRIME:
      print_verbatim (buf, t[0], wrap);
      break;
    case RSUB:
      buf << "_";
      print_verbatim_arg (buf, t[0], wrap);
      break;
    case RSUP:
      buf << "^";
      print_verbatim_arg (buf, t[0], wrap);
      break;
    case FRAC:
      print_verbatim_arg (buf, t[0], wrap);
      buf << "/";
      print_verbatim_arg (buf, t[1], wrap);
      break;
    case SQRT:
      if (N (t) == 1) {
        buf << "sqrt(";
        print_verbatim (buf, t[0], wrap);
        buf << ")";
      }
      else {
        print_verbatim_arg (buf, t[0], wrap);
        print_verbatim_arg (buf, tree (RSUP, tree (FRAC, "1", t[1])), wrap);
      }
      break;
    case WIDE:
      print_verbatim_arg (buf, t[0], wrap);
      print_verbatim (buf, t[1], wrap);
      break;
    case SYNTAX:
      print_verbatim_arg (buf, t[0], wrap);
      break;
    case TABLE:
      print_verbatim_table (buf, t, wrap);
      break;
    default:
      if (is_compound (t, "TeXmacs", 0)) print_verbatim (buf, "TeXmacs", wrap);
      else {
        int i, n= N (t);
        for (i= 0; i < n; i++)
          if (the_drd->is_accessible_child (t, i)) {
            if (is_document (t) && (i > 0)) {
              if (wrap && N (buf) > 0 && buf[N (buf) - 1] != '\n') buf << "\n";
              buf << "\n";
            }
            tree w= std_drd->get_env_child (t, i, tree (ATTR));
            if (drd_env_read (w, MODE, "text") == "prog" ||
                drd_env_read (w, FONT_FAMILY, "rm") == "tt")
              print_verbatim (buf, t[i], false);
            else print_verbatim (buf, t[i], wrap);
          }
      }
      break;
    }
}

static string
as_verbatim (tree t, bool wrap) {
  if (!is_snippet (t)) {
    tree                  init= extract (t, "initial");
    hashmap<string, tree> h   = tree_hashmap (UNINIT, init);
    if (h[MODE] == "prog" || h[FONT_FAMILY] == "tt") wrap= false;
    t= extract (t, "body");
  }
  string buf;
  print_verbatim (buf, t, wrap);
  if (wrap) {
    int i= 0, n= N (buf);
    while (i < n) {
      int pos= i;
      while (i < n && buf[i] != '\n')
        i++;
      array<string> a    = tm_tokenize (buf (pos, i));
      int           start= 0;
      // cout << "a= " << a << "\n";
      // cout << "l= " << N(a) << "\n";
      while (N (a) - start > 78) {
        // cout << "  start= " << start << "\n";
        int mid= start + 78;
        while (mid > start && a[mid] != " ")
          mid--;
        if (mid <= start) break;
        if (mid < N (a)) {
          // cout << "  mid= " << mid << "\n";
          pos+= N (tm_recompose (range (a, start, mid)));
          ASSERT (buf[pos] == ' ', "error in space synchronization");
          buf[pos]= '\n';
          start   = mid + 1;
          pos++;
        }
        else break;
      }
      if (i < n) i++;
    }
  }
  return buf;
}

string
unix_to_dos (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if (s[i] == '\n') r << "\r\n";
    else r << s[i];
  return r;
}

string
var_cork_to_utf8 (string s) {
  string r;
  for (int i= 0; i < N (s);) {
    int start= i;
    while (i < N (s) && s[i] != '\n')
      i++;
    r << cork_to_utf8 (s (start, i));
    if (i < N (s)) {
      r << '\n';
      i++;
    }
  }
  return r;
}

string
var_cork_to_sourcecode (string s) {
  string r;
  for (int i= 0; i < N (s);) {
    int start= i;
    while (i < N (s) && s[i] != '\n')
      i++;
    r << cork_to_sourcecode (s (start, i));
    if (i < N (s)) {
      r << '\n';
      i++;
    }
  }
  return r;
}

string
tree_to_verbatim (tree t, bool wrap, string enc) {
  if (enc == "default") enc= "auto";
  string buf= as_verbatim (t, wrap);
  if (enc == "auto") enc= get_locale_charset ();
  if (enc == "iso-8859-1" || enc == "ISO-8859-1") buf= tm_decode (buf);
  else if (enc == "SourceCode") buf= var_cork_to_sourcecode (buf);
  else if (enc != "cork" && enc != "Cork") buf= var_cork_to_utf8 (buf);
#ifdef OS_WIN
  return unix_to_dos (buf);
#else
  return buf;
#endif
}

/******************************************************************************
 * Verbatim to TeXmacs
 ******************************************************************************/

static string
un_special (string s) {
  int    i, j;
  string r;
  for (i= 0, j= 0; i < N (s); i++, j++)
    if (s[i] == '\t') {
      do {
        r << " ";
        j++;
      } while ((j & 7) != 0);
      j--;
    }
    else if ((s[i] == '\b') && (N (r) > 0) && (r[N (r) - 1] != '\n'))
      r->resize (N (r) - 1);
    else r << s[i];
  return r;
}

static string
encode (string s, string enc) {
  if (enc == "auto") return western_to_cork (s);
  else if (enc == "utf-8") return utf8_to_cork (s);
  else if (enc == "iso-8859-1") return tm_encode (s);
  else if (enc == "SourceCode") return sourcecode_to_cork (s);
  else return tm_encode (s);
}

tree
verbatim_to_tree (string s, string enc) {
  s= encode (s, enc);
  int i, j;
  int s_N= N (s);
  for (i= 0; i < s_N; i++)
    if (s[i] == '\n') {
      tree t (DOCUMENT);
      for (i= 0, j= 0; i < s_N; i++)
        if (s[i] == '\n') {
          t << un_special (s (j, i));
          j= i + 1;
        }
      t << un_special (s (j, i));
      return t;
    }
  return un_special (s);
}

string
dos_to_unix (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if (s[i] == '\r' && i + 1 < n && s[i + 1] == '\n') {
      r << "\n";
      i++;
    }
    else r << s[i];
  return r;
}

string
mac_to_unix (string s) {
  int    i, n= N (s);
  string r;
  for (i= 0; i < n; i++)
    if (s[i] == '\r') r << "\n";
    else r << s[i];
  return r;
}

tree
verbatim_to_tree (string s, bool wrap, string enc) {
  if (enc == "default") enc= "auto";
  s= mac_to_unix (dos_to_unix (s));
  if (wrap) {
    string r;
    int    i, n= N (s);
    for (i= 0; i < n;) {
      if (s[i] == '\n' || s[i] == ' ' || s[i] == '\t') {
        int lf= 0;
        while (i < n && (s[i] == '\n' || s[i] == ' ' || s[i] == '\t')) {
          if (s[i] == '\n') lf++;
          i++;
        }
        if (lf <= 1) r << " ";
        else r << "\n";
      }
      else r << s[i++];
    }
    s= r;
  }
  return verbatim_to_tree (s, enc);
}

tree
verbatim_document_to_tree (string s, bool wrap, string enc) {
  if (enc == "default") enc= "auto";
  tree t   = verbatim_to_tree (s, wrap, enc);
  tree init= tree (COLLECTION, tree (ASSOCIATE, LANGUAGE, "verbatim"),
                   tree (ASSOCIATE, FONT_FAMILY, "tt"),
                   tree (ASSOCIATE, PAR_FIRST, "0cm"));
  return tree (DOCUMENT, compound ("body", t), compound ("initial", init));
}

bool
is_verbatim (tree t) {
  return is_compound (t, "code") || is_compound (t, "verbatim") ||
         is_compound (t, "verbatim-code") || is_compound (t, "scm") ||
         is_compound (t, "cpp") || is_compound (t, "mmx") ||
         is_compound (t, "r") || is_compound (t, "fortran") ||
         is_compound (t, "python") || is_compound (t, "scilab") ||
         is_compound (t, "shell") || is_compound (t, "bash") ||
         is_compound (t, "csv") || is_compound (t, "gnuplot") ||
         is_compound (t, "goldfish") || is_compound (t, "goldfish-lang") ||
         is_compound (t, "java") || is_compound (t, "javascript") ||
         is_compound (t, "json") || is_compound (t, "julia") ||
         is_compound (t, "lua") || is_compound (t, "matlab") ||
         is_compound (t, "moonbit") || is_compound (t, "r7rs") ||
         is_compound (t, "scala") || is_compound (t, "sql") ||
         is_compound (t, "scm-code") || is_compound (t, "cpp-code") ||
         is_compound (t, "mmx-code") || is_compound (t, "r-code") ||
         is_compound (t, "fortran-code") || is_compound (t, "python-code") ||
         is_compound (t, "scilab-code") || is_compound (t, "shell-code") ||
         is_compound (t, "bash-code") || is_compound (t, "csv-code") ||
         is_compound (t, "gnuplot-code") || is_compound (t, "goldfish-code") ||
         is_compound (t, "java-code") || is_compound (t, "javascript-code") ||
         is_compound (t, "json-code") || is_compound (t, "julia-code") ||
         is_compound (t, "lua-code") || is_compound (t, "matlab-code") ||
         is_compound (t, "moonbit-code") || is_compound (t, "r7rs-code") ||
         is_compound (t, "scala-code") || is_compound (t, "sql-code") ||
         is_compound (t, "latex_preview") || is_compound (t, "picture-mixed");
}
