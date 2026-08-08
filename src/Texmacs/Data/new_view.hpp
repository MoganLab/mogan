
/******************************************************************************
 * MODULE     : new_view.hpp
 * DESCRIPTION: View management
 * COPYRIGHT  : (C) 1999-2012  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef NEW_VIEW_H
#define NEW_VIEW_H
#include "tree.hpp"
#include "url.hpp"
class editor;

array<url> get_all_views ();
array<url> get_all_views_unsorted (bool current_window_only);
array<url> buffer_to_views (url name);
editor     get_current_editor ();
editor     view_to_editor (url u);
bool       has_current_view ();
void       set_current_view (url u);
url        get_current_view ();
url        get_current_view_safe ();
void       set_current_drd (url u);
url        window_to_view (url win);
url        view_to_buffer (url u);
url        view_to_window (url u);
url        view_to_window_of_tabpage (url u);
int        count_tabpages_in_window (url win_u);
void       move_tabpage (int old_pos, int new_pos);
url        get_new_view (url name);
url        get_recent_view (url name);
url        get_passive_view (url name);
url        get_passive_view_of_tabpage (url name);
void       delete_view (url u);
void       kill_tabpage (url win_u, url u);
void       notify_rename_before (url old_name);
void       notify_rename_after (url new_name);
void       window_set_view (url win, url new_u, bool focus);
bool       view_set_window (url view_u, url win_u, bool focus);
void       view_set_new_window (url view_u);
void       hack_refresh_window_editors (url win1, url win2, bool revert);
bool       switch_to_other_tabpage (url view_u);
void       switch_to_buffer (url name);
void       focus_on_editor (editor ed);
bool       focus_on_buffer (url name, bool isfocus);
bool       var_focus_on_buffer (url name);
void       make_cursor_visible (url u);
url        get_most_recent_view ();
void       invalidate_most_recent_view ();
bool       is_chat_tab_buffer (url name);
bool       is_startup_tab_buffer (url name);
bool       is_tmfs_view_type (string s, string type);
bool       is_tmfs_view_type (url s, string type);

// Low level types and routines
class tm_view_rep;
typedef tm_view_rep* tm_view;
tm_view              concrete_view (url name);
url                  abstract_view (tm_view vw);
void                 attach_view (url win_u, url u);
void                 detach_view (url u);
url                  get_recent_view (url name, bool s, bool o, bool a, bool p);

#endif // defined NEW_VIEW_H
