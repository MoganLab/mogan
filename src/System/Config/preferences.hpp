
/******************************************************************************
 * MODULE     : preferences.hpp
 * DESCRIPTION: User preferences for TeXmacs
 * COPYRIGHT  : (C) 2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef PREFERENCES_HPP
#define PREFERENCES_HPP

#include "string.hpp"

string get_setting (string var, string def= "");
void   set_setting (string var, string val);
void   get_old_settings (string s);

bool   has_user_preference (string var);
string get_user_preference (string var, string def= "");
void   set_user_preference (string var, string val);
void   set_user_preference_silent (string var, string val);
void   reset_user_preference (string var);
void   load_user_preferences ();
void   save_user_preferences ();

void   notify_preferences_booted ();
void   set_preference (string var, string val);
void   notify_preference (string var);
string get_preference (string var, string def= "default");

#endif