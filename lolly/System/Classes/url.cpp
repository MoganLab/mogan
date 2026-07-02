
/******************************************************************************
 * MODULE     : url.cpp
 * DESCRIPTION: unified resource location handling
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "url.hpp"
#include "analyze.hpp"
#include "sys_utils.hpp"
#include "tbox/tbox.h"
#include <cstring>
#include <ctype.h>
#include <vector>

#if defined(OS_MINGW) || defined(OS_WIN)
#define WINPATHS
#endif

class string_builder {
private:
  std::vector<char> buffer;

public:
  string_builder () { buffer.reserve (256); }

  void append (const string& s) {
    if (N (s) > 0) {
      // Create a non-const copy to access begin()
      string      temp= s;
      const char* data= temp.begin ();
      int         len = N (s);
      buffer.insert (buffer.end (), data, data + len);
    }
  }

  void append (const char* s) {
    if (s && *s) {
      int len= strlen (s);
      buffer.insert (buffer.end (), s, s + len);
    }
  }

  void append (char c) { buffer.push_back (c); }

  string to_string () {
    if (buffer.empty ()) return string ("");
    string result ((int) buffer.size ());
    if (buffer.size () > 0) {
      std::memcpy (result.begin (), buffer.data (), buffer.size ());
    }
    return result;
  }
};

static url_tree
url_tuple (string label) {
  return url_tree (URL_TUPLE, url_tree (label));
}

static url_tree
url_tuple (string label, string value) {
  return url_tree (URL_TUPLE, url_tree (label), url_tree (value));
}

static url_tree
url_tuple (string label, string value, string content) {
  return url_tree (URL_TUPLE, url_tree (label), url_tree (value),
                   url_tree (content));
}

static url_tree
url_tuple (string label, url_tree t1, url_tree t2) {
  return url_tree (URL_TUPLE, url_tree (label), t1, t2);
}

static inline bool
is_tuple (url_tree t, string s) {
  return (t->op == URL_TUPLE) && (N (t) >= 1) && (t[0] == s);
}
static inline bool
is_tuple (url_tree t, const char* s) {
  return (t->op == URL_TUPLE) && (N (t) >= 1) && (t[0] == s);
}
static inline bool
is_tuple (url_tree t, string s, int n) {
  return (t->op == URL_TUPLE) && (N (t) == (n + 1)) && (t[0] == s);
}
static inline bool
is_tuple (url_tree t, const char* s, int n) {
  return (t->op == URL_TUPLE) && (N (t) == (n + 1)) && (t[0] == s);
}

url::url () : rep (tm_new<url_rep> (url_tuple ("none"))) {}
url::url (const char* name) : rep (tm_new<url_rep> (url_system (name)->t)) {}
url::url (string name) : rep (tm_new<url_rep> (url_system (name)->t)) {}
url::url (string path_name, string name)
    : rep (tm_new<url_rep> (url_system (path_name, name)->t)) {}

string
url::protocol () {
  string url_label= this->label ();
  if (url_label == "root") {
    return rep->t[1]->label;
  }
  else if (url_label == "concat") {
    return as_url (rep->t[1]).protocol ();
  }
  else if (url_label == "or") {
    // For mutiple urls, the protocol must be consistent
    // otherwise, it is in empty protocol
    string p1= as_url (rep->t[1]).protocol ();
    if (p1 == "") return ""; // if empty protocol, return empty protocol fast

    string p2= as_url (rep->t[2]).protocol ();
    if (p1 == p2) return p1;
    else return ""; // if the protocol does not match, return empty protocol
  }
  else {
    return "default";
  }
}

url
url_none () {
  return as_url (url_tuple ("none"));
}

url
url_root (string protocol) {
  return as_url (url_tuple ("root", protocol));
}

url
url_wildcard () { // any url
  return as_url (url_tuple ("wildcard"));
}

url
url_wildcard (string name) { // string with * wildcards
  return as_url (url_tuple ("wildcard", name));
}

/******************************************************************************
 * Generic url constructor
 ******************************************************************************/
void
skip_ipv6 (string s, int& i) {
  i++;
  while (i < N (s) &&
         (s[i] == ':' || (s[i] >= '0' && s[i] <= '9') ||
          (s[i] >= 'a' && s[i] <= 'f') || (s[i] >= 'A' && s[i] <= 'F')))
    i++;
  if (i < N (s) && s[i] == ']') i++;
}

static bool
heuristic_is_path (string name, int type) {
  char sep= (type == 0) ? URL_SEPARATOR : ':';
  int  i= 0, n= N (name);
  while (i < n)
    if (name[i] == '[') skip_ipv6 (name, i);
    else if (name[i] == sep) return true;
    else i++;
  return false;
}

static bool
heuristic_is_default (string name, int type) {
#ifdef WINPATHS
  // FIXME: we probably should take into account 'type' too
  if (N (name) < 2) return false;
  if ((name[0] == '\\') && (name[1] == '\\')) return true;
  return isalpha (name[0]) && (name[1] == ':') &&
         ((N (name) == 2) || (name[2] == '\\') || (name[2] == '/'));
#else
  char sep= (type == 0) ? URL_CONCATER : '/';
  return (name != "") && (name[0] == sep);
#endif
}

static bool
heuristic_is_http (string name) {
  return starts (name, "www.");
  // FIXME: we might want to recognize some other ones like google.com too
}

static bool
heuristic_is_ftp (string name) {
  return starts (name, "ftp.");
}

static bool
heuristic_is_mingw_default (string name, int type) {
#ifdef WINPATHS
  return type != URL_SYSTEM && N (name) >= 2 && name[0] == '/' &&
         is_alpha (name[1]) && (N (name) == 2 || name[2] == '/');
#else
  (void) name;
  (void) type;
  return false;
#endif
}

static url
url_get_atom (string s, int type) {
  if (type < URL_STANDARD) {
    if (s == "~") return url_system (get_env ("HOME"));
    if (starts (s, "$")) {
      string val= get_env (s (1, N (s)));
      if (val == "") return url_none ();
      return unblank (url_system (val));
    }
  }
  if (contains (s, '*')) return url_wildcard (s);
#ifdef WINPATHS
  if (N (s) == 2 && ends (s, ":"))
    s->resize (1); // remove the ':' after unit letter
#endif
  return as_url (url_tree (s));
}

url
url_get_name (string s, int type, int i) {
  char      sep   = (type == URL_SYSTEM) ? URL_CONCATER : '/';
  int       n     = N (s);
  list<url> u_list= list<url> ();
  while (i < n) {
    int start= i;
    while ((i < n) && (s[i] != sep) && (s[i] != '/')) {
      if (s[i] == '[') skip_ipv6 (s, i);
      else i++;
    }
    if (i > start) {
      url u = url_get_atom (s (start, i), type);
      u_list= list (u, u_list);
    }
    else if (i == start) {
      i++;
      if (i == n) {
        u_list= list (as_url (url_tree ("")), u_list);
      }
    }
  }

  if (is_nil (u_list)) return as_url (url_tree (""));
  url ret= u_list->item;
  u_list = u_list->next;
  while (!is_nil (u_list)) {
    url u= u_list->item;
    if (is_here (u) || (u->t == "")) {
      // do nothing
    }
    else if (is_atomic (u)) {
      ret= as_url (url_tuple ("concat", u->t, ret->t));
    }
    else {
      ret= u * ret;
    }
    u_list= u_list->next;
  }
  return ret;
}

static url
url_get_path (string s, int type= URL_STANDARD, int i= 0) {
  char sep  = (type == URL_SYSTEM) ? URL_SEPARATOR : ':';
  int  start= i, n= N (s);
  if (i == n) return url_none ();
  while ((i < n) && (s[i] != sep)) {
    if (s[i] == '[') skip_ipv6 (s, i);
    else i++;
  }
  url u= url_general (s (start, i), type);
  if (i == n) return u;
  if (start == i) return url_get_path (s, type, i + 1);
  return u | url_get_path (s, type, i + 1);
}

url
url_path (string s, int type) {
  url u= url_get_path (s, type);
  return u;
}

static url
url_mingw_default (string name, int type) {
  string s= name (0, 2) * ":" * name (2, N (name));
  return url_root ("default") * url_get_name (s, type);
}

static url
url_default (string name, int type= URL_SYSTEM) {
  url u= url_get_name (name, type);
#ifdef WINPATHS
  // FIXME: this hack seems a bit too simple
  if (is_concat (u) && (u[2]->t == "")) u= u[1];
  // cout << name << " -> " << url_root ("default") * u << "\n";
  return url_root ("default") * u;
#else
  if (u->t == "") return url_root ("default");
  return url_root ("default") * u;
#endif
}

url
url_general (string name, int type= URL_SYSTEM) {
  if (contains (name, "://")) {
    array<string> tokens  = tokenize (name, "://");
    string        protocol= tokens[0];
    string        path    = tokens[1];
    if (N (tokens) == 2 && is_alphanum (protocol) && N (protocol) >= 2) {
      if (protocol == "file") return file_url (path);
      if (protocol == "http") return http_url (path);
      if (protocol == "https") return https_url (path);
      if (protocol == "ftp") return ftp_url (path);
      return url_root (tokens[0]) * url_get_name (tokens[1], type);
    }
  }
  if (starts (name, "local:")) {
    return file_url (name (6, N (name)));
  }
  if (starts (name, "//")) {
    return blank_url (name (2, N (name)));
  }
  if (heuristic_is_path (name, type)) {
    return url_path (name, type);
  }
  if (heuristic_is_default (name, type)) return url_default (name, type);
  if (os_mingw () && heuristic_is_mingw_default (name, type))
    return url_mingw_default (name, type);
  if (type != URL_CLEAN_UNIX) {
    if (heuristic_is_http (name)) return http_url (name);
    if (heuristic_is_ftp (name)) return ftp_url (name);
  }
  return url_get_name (name, type);
}

url
url_system (string name) {
  return url_general (name, URL_SYSTEM);
}

url
url_system (string dir, string name) {
  return url_system (dir) * url_system (name);
}

url
url_standard (string name) {
  return url_general (name, URL_STANDARD);
}

url
url_standard (string dir, string name) {
  return url_standard (dir) * url_standard (name);
}

url
url_pwd () {
  char path[TB_PATH_MAXN];
  if (tb_directory_current (path, TB_PATH_MAXN)) {
    return url_system (path);
  }
  else {
    TM_FAILED ("FAILED to get pwd");
    return url_none ();
  }
}

/******************************************************************************
 * Rooted url constructors
 ******************************************************************************/
url
url_ramdisc (string contents) {
  return as_url (url_tuple ("root", "ramdisc", contents));
}

/******************************************************************************
 * Operations on urls
 ******************************************************************************/

static bool
is_special_root (url u) {
#ifdef WINPATHS
  return is_root (u);
#else
  return is_root_web (u);
#endif
}

static bool
is_semi_root (url u) {
  // url u such that u/.. == u (website or windows drive name)
  return is_concat (u) && is_special_root (u[1]) && is_atomic (u[2]);
}

url
head (url u) {
  return u * url_parent ();
}

url
tail (url u) {
  if (is_concat (u)) {
    if (is_root_web (u[1]) && is_atomic (u[2])) return url_here ();
    return tail (u[2]);
  }
  if (is_or (u)) return tail (u[1]) | tail (u[2]);
  if (is_root (u)) return url_here ();
  return u;
}

string
suffix (url u, bool use_locase) {
  u= tail (u);
  if (!is_atomic (u)) return "";
  string s= as_string (u);
  int    i, n= N (s);
  for (i= n - 1; i >= 0; i--)
    if (s[i] == '.') break;
  if ((i > 0) && (i < n - 1)) {
    string r= s (i + 1, n);
    while ((N (r) > 0) && (r[N (r) - 1] == '~' || r[N (r) - 1] == '#'))
      r= r (0, N (r) - 1);
    int found= index_of (r, '?');
    if (found != -1) r= r (0, found);
    if (use_locase) return locase_all (r);
    else return r;
  }
  return "";
}

string
suffix (url u) {
  return suffix (u, true);
}

string
basename (url u, string suf) {
  string s    = as_string (tail (u));
  int    found= index_of (s, '?');
  if (found != -1) s= s (0, found);
  return remove_suffix (s, suf);
}

string
basename (url u) {
  string s= suffix (u, false);
  if (!is_empty (s)) s= "." * s;
  return basename (u, s);
}

url
glue (url u, string s) {
  if (is_atomic (u)) return as_url (url_tree (u->t->label * s));
  if (is_concat (u)) return u[1] * glue (u[2], s);
  if (is_or (u)) return glue (u[1], s) | glue (u[2], s);
  if (is_none (u)) return u;
  TM_FAILED ("can't glue string to url");
  return u;
}

url
unglue (url u, int nr) {
  if (is_atomic (u))
    return as_url (url_tree (u->t->label (0, max (N (u->t->label) - nr, 0))));
  if (is_concat (u)) return u[1] * unglue (u[2], nr);
  if (is_or (u)) return unglue (u[1], nr) | unglue (u[2], nr);
  if (is_none (u)) return u;
  TM_FAILED ("can't unglue from url");
  return u;
}

url
unblank (url u) {
  if (is_concat (u) && (u[2]->t == "")) return u[1];
  if (is_concat (u)) return u[1] * unblank (u[2]);
  if (is_or (u)) return unblank (u[1]) | unblank (u[2]);
  return u;
}

url
relative (url base, url u) {
  return head (base) * u;
}

url
delta_sub (url base, url u) {
  if (is_atomic (base)) return u;
  if (is_concat (base) && is_concat (u) && (base[1] == u[1])) {
    if (is_special_root (base[1]) && is_concat (base[2]) && is_concat (u[2]) &&
        base[2][1] != u[2][1])
      return url_none ();
    return delta_sub (base[2], u[2]);
  }
  if (is_concat (base) && !is_semi_root (base))
    return url_parent () * delta_sub (head (base), u);
  return url_none ();
}

url
delta (url base, url u) {
  if (is_or (u)) return delta (base, u[1]) | delta (base, u[2]);
  url res= delta_sub (base, u);
  if (is_none (res)) return u;
  return res;
}

string
get_root (url u) {
  return u.protocol ();
}

url
unroot (url u) {
  if (is_concat (u)) return unroot (u[1]) * u[2];
  if (is_or (u)) return unroot (u[1]) | unroot (u[2]);
  if (is_root (u)) return url_here ();
  return u;
}

url
reroot (url u, string protocol) {
  if (is_concat (u)) return reroot (u[1], protocol) * u[2];
  if (is_or (u)) return reroot (u[1], protocol) | reroot (u[2], protocol);
  if (is_root (u)) return url_root (protocol);
  return u;
}

static url
expand (url u1, url u2) {
  if (is_or (u1)) return expand (u1[1], u2) | expand (u1[2], u2);
  if (is_or (u2)) return expand (u1, u2[1]) | expand (u1, u2[2]);
  if (is_ancestor (u2)) {
    if (is_concat (u1)) return u1 | expand (u1[1], u2);
    if (is_special_root (u1)) return u2;
    return u1 | u2;
  }
  if (is_concat (u2) && is_ancestor (u2[1]))
    return expand (expand (u1, u2[1]), u2[2]);
  return u1 * u2;
}

url
expand (url u) {
  if (is_or (u)) return expand (u[1]) | expand (u[2]);
  if (is_concat (u)) return expand (expand (u[1]), expand (u[2]));
  return u;
}

static bool
operator<= (url u1, url u2) {
  if (is_atomic (u1) && is_atomic (u2)) return u1->t->label <= u2->t->label;
  if (is_atomic (u1)) return true;
  if (is_atomic (u2)) return false;
  if (is_concat (u1) && is_concat (u2)) {
    if (u1[1] == u2[1]) return u1[2] <= u2[2];
    else return u1[1] <= u2[1];
  }
  if (is_concat (u1)) return true;
  if (is_concat (u2)) return false;
  return true; // does not matter for sorting
}

static url
sort_sub (url add, url to) {
  if (is_or (to)) {
    if (add <= to[1]) return add | to;
    return to[1] | sort_sub (add, to[2]);
  }
  if (add <= to) return add | to;
  else return to | add;
}

url
sort (url u) {
  if (is_or (u)) return sort_sub (u[1], sort (u[2]));
  else return u;
}

static url
factor_sorted (url u) {
  if (!is_or (u)) return u;
  url v= factor_sorted (u[2]);
  if (is_concat (u[1])) {
    if (is_concat (v) && (u[1][1] == v[1])) return u[1][1] * (u[1][2] | v[2]);
    if (is_or (v) && is_concat (v[1]) && (u[1][1] == v[1][1]))
      return (u[1][1] * (u[1][2] | v[1][2])) | v[2];
  }
  return u[1] | v;
}

static url
factor_sub (url u) {
  if (is_concat (u)) return u[1] * factor (u[2]);
  if (is_or (u)) return factor_sub (u[1]) | factor_sub (u[2]);
  return u;
}

url
factor (url u) {
  return factor_sub (factor_sorted (sort (u)));
}

bool
descends (url u, url base) {
  if (u == base) return true;
  if (is_concat (u) && is_atomic (base)) return u[1] == base;
  if (is_concat (u) && is_concat (base)) {
    url iter_u= u, iter_base= base;
    while (iter_u[1] == iter_base[1]) {
      iter_u   = iter_u[2];
      iter_base= iter_base[2];
      if (is_concat (iter_u) && is_concat (iter_base)) {
        continue;
      }
      else {
        return descends (iter_u, iter_base);
      }
    }
    return false;
  }
  if (is_or (u)) return descends (u[1], base) && descends (u[2], base);
  if (is_or (base)) return descends (u, base[1]) || descends (u, base[2]);
  return false;
}

string
drive_letter (url u) {
#ifdef WINPATHS
  string url_str= as_string (u, URL_SYSTEM);

  // Handle drive letters (e.g., "C")
  if (N (url_str) >= 2 && isalpha (url_str[0]) && url_str[1] == ':') {
    return string (1, toupper (url_str[0]));
  }

  // Handle UNC paths (e.g., "\\host\share")
  if (starts (url_str, "\\\\")) {
    int host_end= search_forwards ("\\", 2, url_str);
    if (host_end != -1) {
      int share_end= search_forwards ("\\", host_end + 1, url_str);
      if (share_end != -1) {
        return url_str (0, share_end);
      }
      else {
        return url_str; // Just the UNC host\share, no path
      }
    }
  }

  return "";
#else
  (void) u;
  return ""; // Non-Windows platforms don't have drive letters
#endif
}

url
operator* (url u1, url u2) {
  if (is_root (u2) || (is_concat (u2) && is_root (u2[1]))) {
    if (is_concat (u1) && is_root_web (u1[1])) {
      if (is_root (u2, "default") ||
          (is_concat (u2) && is_root (u2[1], "default"))) {
        url v= u1[2];
        while (is_concat (v))
          v= v[1];
        if (is_root (u2)) return u1[1] * v;
        return u1[1] * v * u2[2];
      }
      if (is_root (u2, "blank") || (is_concat (u2) && is_root (u2[1], "blank")))
        return reroot (u2, u1[1][1]->t->label);
    }
    if (is_root (u1) && is_rooted (u1, "file")) {
      return u1 * u2[2];
    }
    return u2;
  }
  if (is_here (u1) || (u1->t == "")) return u2;
  if (is_here (u2)) return u1;
  if (is_none (u1)) return url_none ();
  if (is_none (u2)) return url_none ();
  if (u2 == url_parent ()) {
    if (is_root (u1)) return u1;
    if (is_pseudo_atomic (u1) && (!is_parent (u1))) return url_here ();
    if (is_semi_root (u1)) return u1;
  }
  if (is_concat (u2) && (u2[1] == url_parent ())) {
    if (is_root (u1)) return u1 * u2[2];
    if (is_pseudo_atomic (u1) && (!is_parent (u1))) return u2[2];
    if (is_semi_root (u1)) return u1 * u2[2];
  }
  if (is_concat (u1)) return u1[1] * (u1[2] * u2);
  return as_url (url_tuple ("concat", u1->t, u2->t));
}

url
operator* (url u1, const char* name) {
  return u1 * url (name);
}

url
operator* (url u1, string name) {
  return u1 * url (name);
}

url
url_concat (url u1, url u2) {
  return u1 * u2;
}

url
operator| (url u1, url u2) {
  if (is_none (u1)) return u2;
  if (is_none (u2)) return u1;
  if (is_or (u1)) return u1[1] | (u1[2] | u2);
  if (u1 == u2) return u2;
  if (is_or (u2) && (u1 == u2[1])) return u2;
  return as_url (url_tuple ("or", u1->t, u2->t));
}

url
url_or (url u1, url u2) {
  return u1 | u2;
}

bool
is_atomic (url u) {
  return is_atomic (u->t);
}
bool
is_root (url u, string s) {
  return is_root (u) && (u[1]->t->label == s);
}
bool
is_root_web (url u) {
  return is_root (u, "http") || is_root (u, "https") || is_root (u, "ftp") ||
         is_root (u, "blank");
}
bool
is_wildcard (url u, int n) {
  return is_tuple (u->t, "wildcard", n);
}
bool
is_pseudo_atomic (url u) {
  return is_atomic (u->t) || is_tuple (u->t, "wildcard", 1);
}

bool
is_rooted (url u) {
  return is_root (u) || (is_concat (u) && is_rooted (u[1])) ||
         (is_or (u) && is_rooted (u[1]) && is_rooted (u[2]));
}

bool
is_rooted (url u, string protocol) {
  return is_root (u, protocol) ||
         (is_concat (u) && is_rooted (u[1], protocol)) ||
         (is_or (u) && is_rooted (u[1], protocol) &&
          is_rooted (u[2], protocol));
}

bool
is_rooted_web (url u) {
  return is_root_web (u) || (is_concat (u) && is_rooted_web (u[1])) ||
         (is_or (u) && is_rooted_web (u[1]) && is_rooted_web (u[2]));
}

bool
is_name (url u) {
  if (is_atomic (u)) return true;
  if (!is_concat (u)) return false;
  return is_name (u[1]) && is_name (u[2]);
}

bool
is_rooted_name (url u) {
  return is_concat (u) && is_root (u[1]) && is_name (u[2]);
}

bool
is_name_in_path (url u) {
  if (is_name (u)) return true;
  return is_concat (u) && is_root (u[1], "default") && is_name (u[2]);
}

bool
is_ramdisc (url u) {
  return is_concat (u) && is_root (u[1], "ramdisc");
}

static void
as_string_impl (url u, int type, string_builder& sb) {
  if (is_none (u)) {
    sb.append ("{}");
    return;
  }

  if (is_atomic (u)) {
    sb.append (u->t->label);
    return;
  }

  if (is_concat (u)) {
    int stype= type;
    if (is_root (u[1]) &&
        !(is_root (u[1], "default") || is_root (u[1], "file"))) {
      stype= URL_STANDARD;
    }

    string sep= (stype == URL_SYSTEM ? string (URL_CONCATER) : string ("/"));

    if (!is_root (u[1], "default")) {
      if ((!is_name (u[1])) && (!is_root (u[1]))) {
        sb.append ("{");
        as_string_impl (u[1], type, sb);
        sb.append ("}");
      }
      else {
        as_string_impl (u[1], type, sb);
      }
    }

    if (os_win () && ((is_root (u[1], "default") && type == URL_SYSTEM) ||
                      is_root (u[1], "file"))) {

      string_builder root_sb, remain_sb;
      if (is_concat (u[2])) {
        as_string_impl (u[2][1], type, root_sb);
        as_string_impl (u[2][2], type, remain_sb);
      }
      else {
        as_string_impl (u[2], type, root_sb);
      }

      string root  = root_sb.to_string ();
      string remain= remain_sb.to_string ();

      if (is_root (u[1], "default")) {
        if (N (root) == 1) {
          sb.append (root);
          sb.append (":");
          sb.append (sep);
          sb.append (remain);
        }
        else {
          sb.append ("\\\\");
          sb.append (root);
          sb.append ("\\");
          sb.append (remain);
        }
      }
      else {
        if (N (root) == 1) {
          sb.append ("/");
          sb.append (root);
          sb.append (":");
          sb.append (sep);
          sb.append (remain);
        }
        else {
          sb.append (root);
          sb.append ("/");
          sb.append (remain);
        }
      }
      return;
    }

    sb.append (sep);

    // Handle right side (u[2])
    if ((!is_concat (u[2])) && (!is_atomic (u[2])) &&
        (!is_wildcard (u[2], 1))) {
      sb.append ("{");
      as_string_impl (u[2], stype, sb);
      sb.append ("}");
    }
    else {
      as_string_impl (u[2], stype, sb);
    }
    return;
  }

  if (is_or (u)) {
    // Handle left side
    if (!is_name_in_path (u[1])) {
      sb.append ("{");
      as_string_impl (u[1], type, sb);
      sb.append ("}");
    }
    else {
      as_string_impl (u[1], type, sb);
    }

    if (os_win ()) {
      if (type == URL_STANDARD) {
        sb.append (":");
      }
      else {
        sb.append (string (URL_SEPARATOR));
      }
    }
    else {
      sb.append (string (URL_SEPARATOR));
    }

    // Handle right side
    if ((!is_or (u[2])) && (!is_name_in_path (u[2]))) {
      sb.append ("{");
      as_string_impl (u[2], type, sb);
      sb.append ("}");
    }
    else {
      as_string_impl (u[2], type, sb);
    }
    return;
  }

  // Handle special cases
  if (os_win ()) {
    if (is_root (u, "default")) {
      int stype= type;
      if (is_root (u[1]) && (!is_root (u[1], "default"))) {
        stype= URL_STANDARD;
      }
      if (stype != URL_SYSTEM) {
        sb.append ("/");
      }
      return;
    }
  }
  else {
    if (is_root (u, "default")) {
      sb.append ("/");
      return;
    }
  }
  if (is_root (u, "blank")) {
    sb.append ("/");
    return;
  }
  if (is_root (u, "file")) {
    sb.append (u[1]->t->label);
    sb.append ("://");
    return;
  }
  if (is_root (u)) {
    sb.append (u[1]->t->label);
    sb.append (":/");
    return;
  }
  if (is_wildcard (u, 0)) {
    sb.append ("**");
    return;
  }
  if (is_wildcard (u, 1)) {
    sb.append (u->t[1]->label);
    return;
  }
  TM_FAILED ("bad url");
}

string
as_string (url u, int type) {
  string_builder sb;
  as_string_impl (u, type, sb);
  return sb.to_string ();
}

tm_ostream&
operator<< (tm_ostream& out, url u) {
  return out << as_string (u, URL_SYSTEM);
}
