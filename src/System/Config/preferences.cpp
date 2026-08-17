
/******************************************************************************
 * MODULE     : preferences.cpp
 * DESCRIPTION: User preferences for TeXmacs
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "preferences.hpp"
#include "analyze.hpp"
#include "file.hpp"
#include "iterator.hpp"
#include "merge_sort.hpp"
#include "scheme.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tree_helper.hpp"

#include <moebius/data/scheme.hpp>
#include <nlohmann/json.hpp>
#include <string>

using moebius::data::block_to_scheme_tree;
using moebius::data::scm_quote;
using moebius::data::scm_unquote;
using nlohmann::json;

tree texmacs_settings= tuple ();

/******************************************************************************
 * Old style settings files
 ******************************************************************************/

static string
line_read (string s, int& i) {
  int start= i, n= N (s);
  for (start= i; i < n; i++)
    if (s[i] == '\n') break;
  string r= s (start, i);
  if (i < n) i++;
  return r;
}

void
get_old_settings (string s) {
  int i= 0, j;
  while (i < N (s)) {
    string l= line_read (s, i);
    for (j= 0; j < N (l); j++)
      if (l[j] == '=') {
        string left= l (0, j);
        while ((j < N (l)) && ((l[j] == '=') || (l[j] == ' ')))
          j++;
        string right= l (j, N (l));
        set_setting (left, right);
      }
  }
}

/******************************************************************************
 * Subroutines for the TeXmacs settings
 ******************************************************************************/

string
get_setting (string var, string def) {
  int i, n= N (texmacs_settings);
  for (i= 0; i < n; i++)
    if (is_tuple (texmacs_settings[i], var, 1)) {
      return scm_unquote (as_string (texmacs_settings[i][1]));
    }
  return def;
}

void
set_setting (string var, string val) {
  int i, n= N (texmacs_settings);
  for (i= 0; i < n; i++)
    if (is_tuple (texmacs_settings[i], var, 1)) {
      texmacs_settings[i][1]= scm_quote (val);
      return;
    }
  texmacs_settings << tuple (var, scm_quote (val));
}

/******************************************************************************
 * Changing the user preferences
 ******************************************************************************/

bool                    user_prefs_modified= false;
hashmap<string, string> user_prefs ("");
void                    notify_preference (string var);

bool
has_user_preference (string var) {
  return user_prefs->contains (var);
}

void
set_user_preference (string var, string val) {
  if (val == "default") user_prefs->reset (var);
  else user_prefs (var)= val;
  user_prefs_modified= true;
  notify_preference (var);
}

void
set_user_preference_silent (string var, string val) {
  if (val == "default") user_prefs->reset (var);
  else user_prefs (var)= val;
  user_prefs_modified= true;
}

void
reset_user_preference (string var) {
  user_prefs->reset (var);
  user_prefs_modified= true;
  notify_preference (var);
}

string
get_user_preference (string var, string val) {
  if (user_prefs->contains (var)) return user_prefs[var];
  else return val;
}

/******************************************************************************
 * Loading and saving user preferences
 ******************************************************************************/

// lolly string 与 std::string 互转（nlohmann::json 的键/值用 std::string）
static string
lolly_string (const std::string& s) {
  return string (s.c_str ());
}

static std::string
std_string (const string& s) {
  std::string r= std::string (c_string (s));
  return r;
}

// 读取 JSON 首选项文件：仅导入键值均为字符串的原子项，
// 其余（数字/布尔/null）跳过以容错
static void
load_json_preferences (url prefs_file) {
  json j= json::parse (std_string (string_load (prefs_file)), nullptr, false);
  if (j.is_discarded () || !j.is_object ()) return;
  for (json::iterator it= j.begin (); it != j.end (); ++it)
    if (it.value ().is_string ())
      user_prefs (lolly_string (it.key ()))=
          lolly_string (it.value ().get<std::string> ());
}

void
load_user_preferences () {
  url prefs_file= get_tm_preference_path ();
  user_prefs    = hashmap<string, string> ("");
  if (exists (prefs_file)) {
    load_json_preferences (prefs_file);
  }
  user_prefs_modified= false;
}

void
save_user_preferences () {
  if (!user_prefs_modified) return;
  url              prefs_file= get_tm_preference_path ();
  iterator<string> it        = iterate (user_prefs);
  array<string>    a;
  while (it->busy ())
    a << it->next ();
  merge_sort (a);
  json j= json::object ();
  for (int i= 0; i < N (a); i++)
    j[std_string (a[i])]= std_string (user_prefs[a[i]]);
  if (save_string (prefs_file, lolly_string (j.dump ())))
    std_warning << "The user preferences could not be saved\n";
  user_prefs_modified= false;
}

/******************************************************************************
 * User preferences
 ******************************************************************************/

static bool preferences_ok= false;

void
notify_preferences_booted () {
  preferences_ok= true;
}

void
set_preference (string var, string val) {
  if (!preferences_ok) set_user_preference (var, val);
  else (void) call ("set-preference", var, val);
}

void
notify_preference (string var) {
  if (preferences_ok) (void) call ("notify-preference", var);
}

string
get_preference (string var, string def) {
  if (!preferences_ok) return get_user_preference (var, def);
  else {
    string pref= as_string (call ("get-preference", var));
    if (pref == "default") return def;
    else return pref;
  }
}
