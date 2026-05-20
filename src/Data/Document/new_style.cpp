
/******************************************************************************
 * MODULE     : new_style.cpp
 * DESCRIPTION: Style and DRD computation and caching
 * COPYRIGHT  : (C) 1999-2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "new_style.hpp"
#include "../../Typeset/env.hpp"
#include "convert.hpp"
#include "data_cache.hpp"
#include "file.hpp"
#include "merge_sort.hpp"
#include "moebius/data/scheme.hpp"
#include "tm_file.hpp"
#include "tmfs_url.hpp"
#include "web_files.hpp"

#include <lolly/hash/md5.hpp>
#include <lolly/hash/sha.hpp>

using namespace moebius;
using moebius::data::scheme_to_tree;
using moebius::data::tree_to_scheme;
using moebius::drd::init_std_drd;
using moebius::drd::std_drd;
using moebius::drd::the_drd;

/******************************************************************************
 * Global data
 ******************************************************************************/

struct style_data_rep {
  hashmap<tree, hashmap<string, tree>> style_cache;
  hashmap<tree, tree>                  style_drd;
  hashmap<string, bool>                style_busy;
  hashmap<string, tree>                style_void;
  drd_info                             drd_void;
  hashmap<tree, hashmap<string, tree>> style_cached;
  hashmap<tree, drd_info>              drd_cached;

  style_data_rep ()
      : style_cache (hashmap<string, tree> (UNINIT)),
        style_drd (tree (COLLECTION)), style_busy (false), style_void (UNINIT),
        drd_void ("void"), style_cached (style_void), drd_cached (drd_void) {}
};

static style_data_rep* sd= NULL;

static void
init_style_data () {
  if (sd == NULL) sd= tm_new<style_data_rep> ();
}

extern hashmap<string, tree> style_tree_cache;
hashmap<string, bool>        hidden_packages (false);
static hashmap<string, bool> hidden_package_set (false);
static bool                  hidden_package_set_initialized= false;

static url
resolve_local_style (string style_name) {
  string pack= style_name * ".ts";
  url    ret = resolve (url ("$TEXMACS_STYLE_PATH") * pack);
  if (DEBUG_IO) {
    debug_io << "Resolved local style: " << style_name << " -> " << ret << LF;
  }
  return ret;
}

static url
resolve_relative_style (string style_name, url master) {
  string pack= style_name * ".ts";
  url    ret = resolve (expand (head (master) * url_ancestor () * pack));
  if (DEBUG_IO) {
    debug_io << "Resolved relative style: (" << style_name << ", " << master
             << ")" << LF << "-> " << ret << LF;
  }
  return ret;
}

url
resolve_style (string style_name, url master) {
  bool remote= is_rooted_web (master);

  // TODO: the relative style resolving should only resolve styles in the same
  // Git repo
  if (remote) {
    url relative_style= resolve_relative_style (style_name, master);
    if (!is_none (relative_style)) {
      return relative_style;
    }
    return resolve_local_style (style_name);
  }
  else {
    url local_style= resolve_local_style (style_name);
    if (!is_none (local_style)) {
      return local_style;
    }
    return resolve_relative_style (style_name, master);
  }
}

/******************************************************************************
 * Modify style so as to search in all ancestor directories
 ******************************************************************************/

tree
preprocess_style (tree styles, url name) {
  if (is_rooted_tmfs (name)) return styles;
  if (is_atomic (styles)) styles= tree (TUPLE, styles);
  if (!is_tuple (styles)) return styles;

  int  n= N (styles);
  tree r (TUPLE, n);
  for (int i= 0; i < n; i++) {
    r[i]= styles[i];
    if (!is_atomic (styles[i])) continue;
    string style_name    = styles[i]->label;
    url    resolved_style= resolve_style (style_name, name);
    if (!is_none (resolved_style)) {
      r[i]= as_string (resolved_style);
    }
  }

  return r;
}

/******************************************************************************
 * Caching style files on disk
 ******************************************************************************/

static string
cache_file_name_sub (tree t) {
  if (is_atomic (t)) {
    string s= t->label;
    if (ends (s, ".ts")) {
      url style= url_system (s);
      if (is_rooted_web (style)) {
        url local_style= get_from_web (s);
        return lolly::hash::sha256_hexdigest (local_style);
      }
      if (is_local_and_single (style)) {
        return lolly::hash::sha256_hexdigest (style);
      }
    }
    s= replace (s, "/", "%");
    s= replace (s, "\\", "%");
    s= replace (s, ":", "_");
    return s;
  }
  else {
    string        ret;
    array<string> file_name_arr;
    int           i, n= N (t);
    for (i= 0; i < n; i++) {
      file_name_arr << cache_file_name_sub (t[i]);
    }
    merge_sort (file_name_arr);
    for (i= 0; i < n; i++) {
      ret << file_name_arr[i] << "__";
    }
    return ret;
  }
}

static string
cache_file_name (tree t) {
  url tmp= url_temp ("txt");
  save_string (tmp, cache_file_name_sub (t));
  return lolly::hash::md5_hexdigest (tmp) * ".scm";
}

void ensure_hidden_package_set ();

void
style_invalidate_cache () {
  style_tree_cache              = hashmap<string, tree> ();
  hidden_packages               = hashmap<string, bool> (false);
  hidden_package_set            = hashmap<string, bool> (false);
  hidden_package_set_initialized= false;
  if (sd != NULL) {
    tm_delete<style_data_rep> (sd);
    sd= NULL;
  }
  init_style_data ();
  remove (get_tm_cache_path () * url_wildcard ("__*"));
}

void
style_set_cache (tree style, hashmap<string, tree> H, tree t) {
  init_style_data ();
  // cout << "set cache " << style << LF;
  sd->style_cache (copy (style))= H;
  sd->style_drd (copy (style))  = t;
  url name= get_tm_cache_path () * url (cache_file_name (style));
  if (!exists (name)) {
    save_string (name, tree_to_scheme (tuple (as_tree (H), t)));
    // cout << "saved " << name << LF;
  }
}

void
style_get_cache (tree style, hashmap<string, tree>& H, tree& t, bool& f) {
  init_style_data ();
  // cout << "get cache " << style << LF;
  if ((style == "") || (style == tree (TUPLE))) {
    f= false;
    return;
  }
  f= sd->style_cache->contains (style);
  if (f) {
    H= sd->style_cache[style];
    t= sd->style_drd[style];
  }
  else {
    string s;
    url    name= get_tm_cache_path () * url (cache_file_name (style));
    if (exists (name) && (!load_string (name, s, false))) {
      // cout << "loaded " << name << LF;
      tree p                        = scheme_to_tree (s);
      H                             = tree_hashmap (UNINIT, p[0]);
      t                             = p[1];
      sd->style_cache (copy (style))= H;
      sd->style_drd (copy (style))  = t;
      f                             = true;
    }
  }
}

/******************************************************************************
 * Get environment and drd of style files
 ******************************************************************************/

bool
compute_env_and_drd (tree style) {
  init_style_data ();
  ASSERT (is_tuple (style), "style tuple expected");
  bool busy= false;
  for (int i= 0; i < N (style); i++)
    busy= busy || sd->style_busy->contains (as_string (style[i]));
  hashmap<string, bool> old_busy= copy (sd->style_busy);
  for (int i= 0; i < N (style); i++)
    sd->style_busy (as_string (style[i]))= true;

  // cout << "Get environment of " << style << INDENT << LF;
  hashmap<string, tree> H;
  drd_info              drd ("none", std_drd);
  url                   none= url ("$PWD/none");
  hashmap<string, tree> lref;
  hashmap<string, tree> gref;
  hashmap<string, tree> laux;
  hashmap<string, tree> gaux;
  hashmap<string, tree> latt;
  hashmap<string, tree> gatt;
  edit_env              env (drd, none, lref, gref, laux, gaux, latt, gatt);
  if (!busy) {
    tree t;
    bool ok;
    style_get_cache (style, H, t, ok);
    if (ok) {
      env->patch_env (H);
      ok= drd->set_locals (t);
      drd->set_environment (H);
    }
    if (!ok) {
      env->exec (tree (USE_PACKAGE, A (style)));
      env->read_env (H);
      drd->heuristic_init (H);
    }
    sd->style_cached (style)= H;
    sd->drd_cached (style)  = drd;
  }
  // cout << UNINDENT << "Got environment of " << style << LF;

  sd->style_busy= old_busy;
  return !busy;
}

hashmap<string, tree>
get_style_env (tree style) {
  // cout << "get_style_env " << style << "\n";
  init_style_data ();
  if (sd->style_cached->contains (style)) return sd->style_cached[style];
  else if (compute_env_and_drd (style)) return sd->style_cached[style];
  else {
    // cout << "Busy style: " << style << "\n";
    return hashmap<string, tree> ();
  }
}

drd_info
get_style_drd (tree style) {
  // cout << "get_style_drd " << style << "\n";
  init_style_data ();
  init_std_drd ();
  if (sd->drd_cached->contains (style)) return sd->drd_cached[style];
  else if (compute_env_and_drd (style)) return sd->drd_cached[style];
  else {
    // cout << "Busy drd: " << style << "\n";
    return std_drd;
  }
}

tree
get_document_preamble (tree t) {
  init_style_data ();
  if (is_atomic (t)) return "";
  else if (is_compound (t, "hide-preamble", 1)) return t[0];
  else if (is_compound (t, "show-preamble", 1)) return t[0];
  else {
    int i, n= N (t);
    for (i= 0; i < n; i++) {
      tree p= get_document_preamble (t[i]);
      if (p != "") return p;
    }
    return "";
  }
}

drd_info
get_document_drd (tree doc) {
  init_style_data ();
  tree style= extract (doc, "style");
  if (extract (doc, "TeXmacs") == "") {
    if (the_drd->get_syntax (make_tree_label ("theorem")) != tree (UNINIT))
      return the_drd;
    style= tree (TUPLE, "generic");
  }
  // cout << "style= " << style << "\n";
  drd_info drd= get_style_drd (style);
  tree     p  = get_document_preamble (doc);
  if (p != "") {
    drd                       = drd_info ("preamble", drd);
    url                   none= url ("$PWD/none");
    hashmap<string, tree> lref;
    hashmap<string, tree> gref;
    hashmap<string, tree> laux;
    hashmap<string, tree> gaux;
    hashmap<string, tree> latt;
    hashmap<string, tree> gatt;
    edit_env              env (drd, none, lref, gref, laux, gaux, latt, gatt);
    hashmap<string, tree> H;
    env->exec (tree (USE_PACKAGE, A (style)));
    env->exec (p);
    env->read_env (H);
    drd->heuristic_init (H);
  }
  return drd;
}

/******************************************************************************
 * The style and package menus
 ******************************************************************************/

static bool
ignore_dir (string dir) {
  return (dir == "Beamer") || (dir == "Compute") || (dir == "Customize") ||
         (dir == "Environment") || (dir == "Gui") || (dir == "Header") ||
         (dir == "Latex") || (dir == "Miscellaneous") || (dir == "Obsolete") ||
         (dir == "Poster") || (dir == "Section") || (dir == "Session") ||
         (dir == "Standard") || (dir == "Test") || (dir == "Themes");
}

static void
collect_hidden_packages (url u, bool hidden, hashmap<string, bool>& pkgs) {
  if (is_or (u)) {
    collect_hidden_packages (u[1], hidden, pkgs);
    collect_hidden_packages (u[2], hidden, pkgs);
    return;
  }
  if (is_concat (u)) {
    string dir= upcase_first (as_string (u[1]));
    if (dir == "CVS" || dir == ".svn") return;
    collect_hidden_packages (u[2], hidden || ignore_dir (dir), pkgs);
    return;
  }
  if (hidden && is_atomic (u)) {
    string l= as_string (u);
    if (ends (l, ".ts")) l= l (0, N (l) - 3);
    else if (ends (l, ".hook")) l= l (0, N (l) - 5);
    else return;
    pkgs (l)= true;
  }
}

static url get_package_root ();

void
ensure_hidden_package_set () {
  if (!hidden_package_set_initialized) {
    bench_start ("hidden_package_init");
    collect_hidden_packages (get_package_root (), false, hidden_package_set);
    hidden_package_set_initialized= true;
    bench_end ("hidden_package_init");
  }
}

bool
hidden_package (string name) {
  if (name == "std-latex") return false;
  ensure_hidden_package_set ();
  return hidden_package_set->contains (name);
}

static string
compute_style_menu (url u, int kind) {
  if (is_or (u)) {
    string sep= "\n";
    if (is_atomic (u[1]) &&
        ((is_concat (u[2]) && (u[2][1] != "CVS") && (u[2][1] != ".svn")) ||
         (is_or (u[2]) && is_concat (u[2][1]))))
      sep= "\n---\n";
    return compute_style_menu (u[1], kind) * sep *
           compute_style_menu (u[2], kind);
  }
  if (is_concat (u)) {
    string dir= as_string (u[1]);
    if (dir == "texmacs") {
      dir= "TeXmacs";
    }
    else {
      dir= upcase_first (dir);
    }
    string sub= compute_style_menu (u[2], kind);
    if (ignore_dir (dir) || dir == "CVS" || dir == ".svn") return "";
    return "(-> \"" * dir * "\" " * sub * ")";
  }
  if (is_atomic (u)) {
    string l= as_string (u);
    if (ends (l, ".ts")) l= l (0, N (l) - 3);
    else if (ends (l, ".hook")) l= l (0, N (l) - 5);
    else return "";
    string cmd ("set-main-style");
    if (kind == 1) cmd= "add-style-package";
    if (kind == 2) cmd= "remove-style-package";
    if (kind == 3) cmd= "toggle-style-package";
    return "((verbatim \"" * l * "\") (" * cmd * " \"" * l * "\"))";
  }
  return "";
}

static url
get_package_root () {
  static url pck_u= descendance ("$TEXMACS_PACKAGE_ROOT");
  return pck_u;
}

object
get_style_menu () {
  static object cache;
  if (cache != null_object ()) return cache;
  url    sty_u= descendance ("$TEXMACS_STYLE_ROOT");
  string sty  = compute_style_menu (sty_u, 0);
  cache       = eval ("(menu-dynamic " * sty * ")");
  return cache;
}

object
get_add_package_menu () {
  static object cache;
  if (cache != null_object ()) return cache;
  url    pck_u= get_package_root ();
  string pck  = compute_style_menu (pck_u, 1);
  cache       = eval ("(menu-dynamic " * pck * ")");
  return cache;
}

object
get_remove_package_menu () {
  static object cache;
  if (cache != null_object ()) return cache;
  url    pck_u= get_package_root ();
  string pck  = compute_style_menu (pck_u, 2);
  cache       = eval ("(menu-dynamic " * pck * ")");
  return cache;
}

object
get_toggle_package_menu () {
  static object cache;
  if (cache != null_object ()) return cache;
  url    pck_u= get_package_root ();
  string pck  = compute_style_menu (pck_u, 3);
  cache       = eval ("(menu-dynamic " * pck * ")");
  return cache;
}
