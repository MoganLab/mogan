
/******************************************************************************
 * MODULE     : parsehtml.cpp
 * DESCRIPTION: wrapper for HTML parsing to handle extensions such as MathJax
 * COPYRIGHT  : (C) 2019  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "html.hpp"

#ifdef QTTEXMACS
#include <QProgressDialog>
#include <QApplication>
#endif

#include "Xml/xml.hpp"
#include "converter.hpp"
#include "hashset.hpp"
#include "parse_string.hpp"

static int                  mathjax_serial= 1;
static hashmap<int, string> mathjax_strings;
static hashmap<int, tree>   mathjax_trees;

/******************************************************************************
 * MathJax extension
 ******************************************************************************/

bool
contains_mathjax (string s) {
  int pos= search_forwards ("<head>", 0, s);
  if (pos < 0) return false;
  pos= search_forwards ("<script", pos, s);
  if (pos < 0) return false;
  pos= search_forwards ("MathJax.js", pos, s);
  if (pos < 0) return false;
  pos= search_forwards ("</head>", pos, s);
  return pos >= 0;
}

bool
get_mathjax (string s, int& i, string close) {
  while (i < N (s)) {
    string expect= "";
    if (read (s, i, "$$")) {
      if (close == "$$") return true;
      else expect= "$$";
    }
    else if (read (s, i, "$")) {
      if (close == "$") return true;
      else expect= "$";
    }
    else if (read (s, i, "\\(")) expect= "\\)";
    else if (read (s, i, "\\[")) expect= "\\]";
    else if (read (s, i, "\\begin{equation}")) expect= "\\end{equation}";
    else if (read (s, i, "\\begin{equation*}")) expect= "\\end{equation*}";
    else if (read (s, i, "\\begin{eqnarray}")) expect= "\\end{eqnarray}";
    else if (read (s, i, "\\begin{eqnarray*}")) expect= "\\end{eqnarray*}";
    else if (test (s, i, "\\)") || test (s, i, "\\]") ||
             test (s, i, "\\end{equation}") ||
             test (s, i, "\\end{equation*}") ||
             test (s, i, "\\end{eqnarray}") ||
             test (s, i, "\\end{eqnarray*}")) {
      if (!test (s, i, close)) return false;
      i+= N (close);
      return true;
    }
    else if (close == "") return false;
    else i++;
    if (N (expect) != 0) {
      if (!get_mathjax (s, i, expect)) return false;
      if (close == "") return true;
    }
  }
  return false;
}

bool
acceptable_mathjax (string s) {
  if (!starts (s, "$") || starts (s, "$$")) return true;
  if (occurs ("</", s)) return false;
  if (occurs ("<span", s)) return false;
  if (occurs ("<div ", s)) return false;
  return true;
}

string
process_mathjax (string s) {
  int    i= 0;
  string r;
  while (i < N (s)) {
    int pos= i;
    if (s[i] == '\\' || s[i] == '$') {
      if (get_mathjax (s, i, "") && acceptable_mathjax (s (pos, i))) {
        mathjax_strings (mathjax_serial)= s (pos, i);
        r << "<mathjax>" << as_string (mathjax_serial) << "</mathjax>";
        mathjax_serial++;
      }
      else {
        i= pos;
        r << s[i++];
      }
    }
    else r << s[i++];
  }
  return r;
}

tree
retrieve_mathjax (int id) {
  if (!mathjax_strings->contains (id)) return "";
  tree r= mathjax_strings[id];
  mathjax_strings->reset (id);
  return r;
}

/******************************************************************************
 * Script handling
 ******************************************************************************/

bool
contains_script (string s) {
  return search_forwards ("<script", 0, s) >= 0 &&
         search_forwards ("</script>", 0, s) >= 0;
}

string
process_script (string s) {
  int    i= 0, start;
  string r= "";

  int s_N= N (s);
  while (i < s_N) {
    start= search_forwards ("<script", i, s);
    if (start < 0) {
      r << s (i, s_N);
      break;
    }

    // before <script
    r << s (i, start);

    // inside <script ... >
    int tag_end= search_forwards (">", start, s);
    r << s (start, tag_end + 1);

    // between <script> and </script>
    int    close_tag     = search_forwards ("</script>", tag_end + 1, s);
    string script_content= s (tag_end + 1, close_tag);
    script_content       = replace (script_content, "<", "&lt;");
    script_content       = replace (script_content, ">", "&gt;");
    r << script_content;
    r << "</script>";

    // "</script>" length is 9
    i= close_tag + 9;
  }
  return r;
}

/******************************************************************************
 * Interface
 ******************************************************************************/
tree
parse_plain_html (string s) {
  xml_html_parser parser;
  parser.html= true;
  tree t     = parser.parse (s);
  return t;
}

tree
parse_html (string s) {
  if (contains_mathjax (s)) s= process_mathjax (s);
  if (contains_script (s)) s= process_script (s);
  return parse_plain_html (s);
}

#ifdef QTTEXMACS
static QProgressDialog* html_progress_dialog = nullptr;
#endif

void
html_progress_start (int total) {
#ifdef QTTEXMACS
  if (QApplication::instance () && qobject_cast<QApplication*> (QApplication::instance ())) {
    QWidget* main_window = QApplication::activeWindow ();
    html_progress_dialog = new QProgressDialog (
        "Exporting HTML...", "Cancel", 0, total, main_window);
    html_progress_dialog->setWindowModality (Qt::WindowModal);
    html_progress_dialog->setMinimumDuration (0);
    html_progress_dialog->setValue (0);
    html_progress_dialog->show ();
    html_progress_dialog->repaint ();
    QCoreApplication::processEvents ();
  }
#else
  (void) total;
#endif
}

void
html_progress_update (int current) {
#ifdef QTTEXMACS
  if (html_progress_dialog) {
    html_progress_dialog->setValue (current);
    html_progress_dialog->repaint ();
    QCoreApplication::processEvents ();
  }
#else
  (void) current;
#endif
}

void
html_progress_end () {
#ifdef QTTEXMACS
  if (html_progress_dialog) {
    html_progress_dialog->close ();
    delete html_progress_dialog;
    html_progress_dialog = nullptr;
  }
#endif
}
