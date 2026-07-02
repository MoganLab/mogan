
/******************************************************************************
 * MODULE     : url.hpp
 * DESCRIPTION: unified resource location handling
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef LOLLY_URL_H
#define LOLLY_URL_H

#define URL_SYSTEM 0
#define URL_UNIX 1
#define URL_STANDARD 2
#define URL_CLEAN_UNIX 3

#define URL_TUPLE 247

#include "lolly/data/lolly_tree.hpp"
#include "string.hpp"

using lolly::data::lolly_tree;
typedef lolly_tree<int> url_tree;

/******************************************************************************
 * The url data type
 ******************************************************************************/

struct url_rep : concrete_struct {
  url_tree t;
  inline url_rep (url_tree t2) : t (t2) {}
};

class url {
  CONCRETE (url);

private:
  url (url_tree t) : rep (tm_new<url_rep> (t)) {}

public:
  url ();
  url (const char* name);
  url (string name);
  url (string dir, string name);
  inline bool   operator== (url u) { return rep->t == u->t; }
  inline bool   operator!= (url u) { return rep->t != u->t; }
  inline url    operator[] (int i) { return url (rep->t[i]); }
  inline string label () {
    if (is_atomic (rep->t)) return string ("");
    else return to_string (rep->t[0]);
  };
  string     protocol ();
  friend url as_url (url_tree t);
};
CONCRETE_CODE (url);

inline url
as_url (url_tree t) {
  return url (t);
}

/******************************************************************************
 * url subclasses by protocol
 ******************************************************************************/
class file_url : public url {
public:
  file_url (const char* name);
  file_url (string name);
  string concretize ();
};

class ftp_url : public url {
public:
  ftp_url (const char* name);
  ftp_url (string name);
};

class blank_url : public url {
public:
  blank_url (const char* name);
  blank_url (string name);
};

class http_url : public url {
public:
  http_url (const char* name);
  http_url (string name);
};

class https_url : public url {
public:
  https_url (const char* name);
  https_url (string name);
};

bool is_root (url u, string s);
bool is_root_web (url u);
bool is_root_blank (url u);

bool is_rooted (url u, string protocol);
bool is_rooted_web (url u);
bool is_rooted_blank (url u);

/******************************************************************************
 * url routines by label
 * + "" (empty string): ., .., ..., /, /tmp
 * + none: invalid url
 * + root: the url http://gnu.org yields (concat (root "http") "gnu.org");
 * + concat: a/b/c is represented as (concat "a" (concat "b" "c"));
 * + or: the path a:b/c is represented as (or "a" (concat "b" "c"));
 * + wildcard: (wildcard) corresponds to any url
 *   (wildcard "*.tm") is
 *   - to all strings which end with .tm and (wildcard "*.tm" "file")
 *   - to all TeXmacs files (i.e. discarding directories ending with .tm).
 ******************************************************************************/
url url_none ();

inline url
url_here () {
  return as_url (url_tree ("."));
}

inline url
url_parent () {
  return as_url (url_tree (".."));
}

inline url
url_ancestor () {
  return as_url (url_tree ("..."));
}

url url_root (string protocol);

url operator* (url u1, url u2); // concatenation of url with rootless url
url operator* (url u1, const char* name);
url operator* (url u1, string name);
url url_concat (url u1, url u2);
inline url
url_parent (url u) {
  return u * url_parent ();
}

url operator| (url u1, url u2); // disjunction of urls like in file paths
url url_or (url u1, url u2);

url url_wildcard ();
url url_wildcard (string name);

inline bool
is_here (url u) {
  return u->t == ".";
};
inline bool
is_parent (url u) {
  return u->t == "..";
};
inline bool
is_ancestor (url u) {
  return u->t == "...";
};
inline bool
is_none (url u) {
  return u.label () == "none";
};
inline bool
is_root (url u) {
  return u.label () == "root";
};
inline bool
is_concat (url u) {
  return u.label () == "concat";
};
inline bool
is_or (url u) {
  return u.label () == "or";
};
inline bool
is_wildcard (url u) {
  return u.label () == "wildcard";
};

/******************************************************************************
 * url routines by type
 ******************************************************************************/
url url_path (string s, int type= URL_SYSTEM);
url url_general (string name, int type);
url url_unix (string name);
url url_unix (string dir, string name);
url url_system (string name);
url url_system (string dir, string name);
url url_standard (string name);
url url_standard (string dir, string name);

url url_pwd ();

url url_ramdisc (string contents); // ramdisc with contents contents

/******************************************************************************
 * operations on urls
 ******************************************************************************/
url    head (url u);                    // keep only the directory of the file
url    tail (url u);                    // keep only the file name without path
string suffix (url u);                  // get suffix of file in lower case
string suffix (url u, bool use_locase); // get suffix of file
string basename (url u, string suf); // get basename of file with given suffix
string basename (url u);             // get basename of file
url    glue (url u, string s);       // glue suffix to url tail
url    unglue (url u, int nr);       // remove nr chars from suffix
url    unblank (url u);              // a/b/ -> a/b
url    relative (url base, url u);   // a/b, c -> a/c
url    delta (url base, url u);      // relative (a, delta (a, b)) == b
string get_root (url u);             // get root
url    unroot (url u);               // remove root
url    reroot (url u, string s);     // reroot using new protocol
url    expand (url u);               // rewrite a/{b:c} -> a/b:a/c
url    sort (url u);                 // order items in ors
url    factor (url u);               // inverse of expand; also sorts
bool   descends (url u, url base);   // does u descend from base?
string
drive_letter (url u); // get windows uppercase drive letter (windows only)

/******************************************************************************
 * predicates
 ******************************************************************************/
bool is_atomic (url u);
bool is_wildcard (url u, int n);
bool is_pseudo_atomic (url u);

bool is_rooted (url u);
bool is_name (url u);
bool is_rooted_name (url u);
bool is_ramdisc (url u);

void skip_ipv6 (string s, int& i);

/******************************************************************************
 * conversions
 ******************************************************************************/
string      as_string (url u, int type= URL_SYSTEM);
tm_ostream& operator<< (tm_ostream& out, url u);

inline string
as_system_string (url u) {
  return as_string (u, URL_SYSTEM);
}
inline string
as_unix_string (url u) {
  return as_string (u, URL_UNIX);
}
inline string
as_standard_string (url u) {
  return as_string (u, URL_STANDARD);
}

/******************************************************************************
 * utilities
 ******************************************************************************/
url url_get_name (string s, int type= URL_STANDARD, int i= 0);

#endif
