
/******************************************************************************
 * MODULE     : edit_source.cpp
 * DESCRIPTION: source edit routines
 * COPYRIGHT  : (C) 2025 JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "connect.hpp"
#include "dictionary.hpp"
#include "edit_interface.hpp"
#include "hashset.hpp"
#include "iterator.hpp"
#include "merge_sort.hpp"
#include "observers.hpp"
#include "preferences.hpp"
#include "tm_buffer.hpp"
#include "tree_observer.hpp"

#include <moebius/data/scheme.hpp>

using namespace moebius;
using moebius::data::scm_quote;
using moebius::data::scm_unquote;

void
edit_interface_rep::source_complete_try () {
  bool is_source     = (get_env_string ("mode") == "src");
  bool is_source_mode= (get_env_string ("preamble") == "true");

  // Disable autocomplete options in the macro editor
  string buf_name= as_string (buf->buf->name);
  if ((starts (buf_name, "tmfs://aux/edit-") &&
       buf_name != "tmfs://aux/edit-shortcuts" &&
       !starts (buf_name, "tmfs://aux/edit-comment")) ||
      buf_name == "tmfs://aux/macro-editor") {
    return;
  }

  if (is_source && (!is_source_mode)) {
    completion_style= get_preference ("completion style");
    if (completion_style != "popup") {
      return;
    }
    tree st          = subtree (et, path_up (tp));
    completion_prefix= st->label;
    if (completion_prefix != "") {
      completion_pos     = 0;
      completions        = array<string> ();
      array<object> tmp  = as_array_object (eval (
          "(fuzzy-match-macro-prefix " * scm_quote (completion_prefix) * ")"));
      int           tmp_N= N (tmp);

      for (int i= 0; i < tmp_N; i++) {
        completions << scm_unquote (object_to_string (tmp[i]));
      }
      path tp1     = tp;
      int  prefix_N= N (completion_prefix);
      for (int i= 0; (i < prefix_N && last_item (tp1) > 0); ++i) {
        tp1= previous_valid (et, tp1); // 向前移动到前缀的起始位置
      }
      cursor cu= eb->find_check_cursor (tp1);
      if (completion_style == string ("popup")) {
        show_completion_popup ("source", tp, completions, cu, magf,
                               get_scroll_x (), get_scroll_y (),
                               get_canvas_x (), get_canvas_y ());
      }
    }
    else {
      hide_completion_popup ();
    }
  }
  else if (!(get_input_mode () == INPUT_COMPLETE)) {
    hide_completion_popup ();
  }
}

bool
edit_interface_rep::source_complete_keypress (string key) {
  int completions_N= N (completions);
  completion_style = get_preference ("completion style");
  if (!completion_popup_visible ()) {
    return false;
  }

  // 排除特例
  if (key == "tab" && completions_N == 1) {
    // 仅有一个补全项时，先补全后隐藏补全框，然后返回 false 正常处理按键
    source_complete_variant (completions[0]);
    hide_completion_popup ();
    return false;
  }
  if (!(key == "tab" || key == "S-tab" || key == "down" || key == "up" ||
        key == "escape" || key == "enter" || key == "return" || key == "left" ||
        key == "right" || key == "space")) {
    int     status;
    command _;  // unused
    string  __; // unused
    sv->get_keycomb (key, status, _, __, __);
    if ((status & 1) == 1) {
      // 遇到绑定功能的按键时，隐藏补全框
      hide_completion_popup ();
    }
    return false; // 仅处理特定的键
  }

  // 按键处理逻辑
  if (key == "enter" || key == "return") {
    tree st= subtree (et, path_up (tp));
    if (completions_N > completion_pos &&
        (st->label != completions[completion_pos])) {
      // 当前词未被补全时，按回车键补全
      source_complete_variant (completions[completion_pos]);
    }
    else {
      hide_completion_popup ();
      return false; // 当前词已被补全，按回车键正常处理
    }
    hide_completion_popup ();
  }
  else if (key == "left" || key == "right" || key == "space") {
    hide_completion_popup ();
    return false;
  }
  else if (key == "down" || key == "tab") {
    completion_pos++;
    completion_popup_next (true);
  }
  else if (key == "up" || key == "S-tab") {
    completion_pos--;
    completion_popup_next (false);
  }
  else if (key == "escape") {
    hide_completion_popup ();
  }
  else if (key == "tab" || key == "down") {
    completion_pos++;
    completion_popup_next (true);
  }
  else if (key == "S-tab" || key == "up") {
    completion_pos--;
    completion_popup_next (false);
  }
  if (completion_pos < 0) completion_pos= completions_N - 1;
  if (completion_pos >= completions_N) completion_pos= 0;
  return true;
}

void
edit_interface_rep::source_complete_variant (string new_completion) {
  tree   st              = subtree (et, path_up (tp));
  string old_completion  = st->label;
  int    old_completion_N= N (old_completion);
  int    end             = last_item (tp);
  go_to (path_up (tp) * old_completion_N);
  remove (path_up (tp) * 0, old_completion_N);
  insert (path_up (tp) * 0, new_completion);
  apply_changes ();

  update_completion_popup_position (et, eb, tp, magf, get_scroll_x (),
                                    get_scroll_y (), get_canvas_x (),
                                    get_canvas_y (), completion_pos);
}
