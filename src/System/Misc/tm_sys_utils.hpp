
/******************************************************************************
 * MODULE     : tm_sys_utils.hpp
 * DESCRIPTION: system utilities for TeXmacs
 * COPYRIGHT  : (C) 1999-2016  Joris van der Hoeven, Denis Raux
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TM_SYS_UTILS_H
#define TM_SYS_UTILS_H

#include "array.hpp"
#include "string.hpp"
#include "url.hpp"

extern int script_status; // 0: never accept, 1: prompt, 2: always accept

string get_current_cpu_arch ();
string get_pretty_os_name ();

string eval_system (string s);
string var_eval_system (string s);
string check_stdout (string s);
string check_stderr (string s);

url  get_texmacs_path ();
url  get_texmacs_home_path ();
void init_texmacs_home_path ();
url  get_tm_cache_path ();
url  get_tm_preference_path ();

string get_printing_default ();
bool   has_printing_cmd (void);
string get_printing_cmd (void);
void   set_printing_cmd (string cmd);

void open_url (url u);

void set_wait_handler (void (*) (string, string, int));
void system_wait (string message, string argument= "", int level= 0);

bool is_community_stem ();

string stem_user_agent ();
string stem_device_id ();

#endif
