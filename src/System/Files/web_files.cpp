
/******************************************************************************
 * MODULE     : web_files.cpp
 * DESCRIPTION: file handling via the web
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "web_files.hpp"
#include "analyze.hpp"
#include "file.hpp"
#include "hashmap.hpp"
#include "sys_utils.hpp"
#include "tm_debug.hpp"
#include "tm_file.hpp"
#include "tree_helper.hpp"

#include "cork.hpp"
#include "scheme.hpp"
#include "tmfs_url.hpp"

#include <lolly/io/http.hpp>

#ifdef QTTEXMACS
#include <QApplication>
#include <QEventLoop>
#endif

using lolly::io::http_head;
using lolly::io::http_headers;
using lolly::io::http_label;
using lolly::io::http_tree;

#define MAX_CACHED 25

static int                 web_nr= 0;
static array<tree>         web_cache (MAX_CACHED);
static hashmap<tree, tree> web_cache_resolve ("");

/******************************************************************************
 * Caching
 ******************************************************************************/

static url
get_cache (url name) {
  tree key= as_tree (name);
  if (web_cache_resolve->contains (key)) {
    int  i, j;
    tree tmp= web_cache_resolve[key];
    for (i= 0; i < MAX_CACHED; i++)
      if (web_cache[i] == key) {
        // cout << name << " in cache as " << tmp << " at " << i << "\n";
        for (j= i; ((j + 1) % MAX_CACHED) != web_nr; j= (j + 1) % MAX_CACHED)
          web_cache[j]= web_cache[(j + 1) % MAX_CACHED];
        web_cache[j]= key;
        break;
      }
    return as_url (as_url_tree (tmp)); // url_system (tmp);
  }
  return url_none ();
}

static url
set_cache (url name, url tmp) {
  tree name_tree= as_tree (name);
  web_cache_resolve->reset (web_cache[web_nr]);
  web_cache[web_nr]            = name_tree;
  web_cache_resolve (name_tree)= as_tree (tmp);
  web_nr                       = (web_nr + 1) % MAX_CACHED;
  return tmp;
}

void
web_cache_invalidate (url name) {
  tree name_tree= as_tree (name);
  for (int i= 0; i < MAX_CACHED; i++)
    if (web_cache[i] == name_tree) {
      web_cache[i]= tree ("");
      web_cache_resolve->reset (name_tree);
    }
}

/******************************************************************************
 * Web files
 ******************************************************************************/

static string
web_encode (string s) {
  return tm_decode (s);
}

static string
fetch_tool () {
  static bool   done= false;
  static string tool= "";
  if (done) return tool;
  if (tool == "" && exists_in_path ("wget")) tool= "wget";
  if (tool == "" && exists_in_path ("curl")) tool= "curl";
  done= true;
  return tool;
}

url
get_from_web (url name) {
  if (!is_rooted_web (name)) return url_none ();

#ifdef QTTEXMACS
  // HACK:
  // Process pending events to ensure the wait dialog is rendered
  // before the synchronous download blocks the main thread
  qApp->processEvents (QEventLoop::ExcludeUserInputEvents);
#endif

  http_tree head_res   = http_head (name);
  long      status_code= open_box<long> (
      http_response_ref (head_res, http_label::STATUS_CODE)->data);
  http_headers head_headers= open_box<hashmap<string, string>> (
      http_response_ref (head_res, http_label::HEADER)->data);
  string content_type= head_headers ("content-type");

  if (status_code != 200 && status_code != 403) {
    if (DEBUG_IO) {
      debug_io << "HEAD status code " << status_code << " " << name << LF;
    }
    return url_none ();
  }

  url res= get_cache (name);
  if (!is_none (res)) return res;

  string suf= suffix (name);
  if (is_empty (suf)) {
    if (starts (content_type, "image/")) {
      suf= replace (content_type, "image/", "");
    }
  }

  url          tmp      = url_temp (suf);
  http_headers headers  = http_headers ();
  headers ("User-Agent")= string ("Mogan/") * XMACS_VERSION * " (" *
                          get_pretty_os_name () * "; " *
                          get_current_cpu_arch () * ")";
  lolly::io::download (name, tmp, headers);
  if (DEBUG_IO) {
    debug_io << "Download from " << name << "=> " << tmp << LF;
  }

  if (!exists (tmp)) {
    return url_none ();
  }
  else {
    set_cache (name, tmp);
    return tmp;
  }
}

/******************************************************************************
 * Files from a hyperlink file system
 ******************************************************************************/

url
get_from_server (url u) {
  if (!is_rooted_tmfs (u)) return url_none ();
  url res= get_cache (u);
  if (!is_none (res)) return res;

  string name= as_string (u);
  if (ends (name, "~") || ends (name, "#")) {
    if (!is_rooted_tmfs (name)) return url_none ();
    if (!as_bool (call ("tmfs-can-autosave?", unglue (u, 1))))
      return url_none ();
  }
  string r= as_string (call ("tmfs-load", object (name)));
  if (r == "") return url_none ();
  url tmp= url_temp (suffix (name));
  (void) save_string (tmp, r, true);

  // return set_cache (u, tmp);
  return tmp;
  // FIXME: certain files could be cached, but others not
  // for instance, files which are loaded in a delayed fashion
  // would always be cached as empty files, which is erroneous.
}

bool
save_to_server (url u, string s) {
  if (!is_rooted_tmfs (u)) return true;
  string name= as_string (u);
  (void) call ("tmfs-save", object (name), object (s));
  return false;
}

/******************************************************************************
 * Ramdisc
 ******************************************************************************/

url
get_from_ramdisc (url u) {
  if (!is_ramdisc (u)) return url_none ();
  url res= get_cache (u);
  if (!is_none (res)) return (res);
  url tmp= url_temp (suffix (u));
  save_string (tmp, u[1][2]->t->label);
  return set_cache (u, tmp);
}
