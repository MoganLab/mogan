
/******************************************************************************
 * MODULE     : edit_keyboard.cpp
 * DESCRIPTION: Keyboard handling
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "archiver.hpp"
#include "converter.hpp"
#include "cork.hpp"
#include "edit_interface.hpp"
#include "new_view.hpp"
#include "new_window.hpp"
#include "object_l5.hpp"
#include "preferences.hpp"
#include "tm_buffer.hpp"
#include "tm_window.hpp"

#include <lolly/data/numeral.hpp>

using namespace moebius;

using lolly::data::to_Hex;

#ifdef Q_OS_MAC
extern hashmap<int, string> qtcomposemap;
#endif

/******************************************************************************
 * Basic subroutines for keyboard handling
 ******************************************************************************/
int
edit_interface_rep::get_input_mode () {
  return input_mode;
}

void
edit_interface_rep::set_input_mode (int mode) {
  interrupt_shortcut ();
  // avoids keyboard shortcuts when using the menu between two keystrokes

  if ((mode == INPUT_NORMAL) && (input_mode != INPUT_NORMAL)) {
    selection_cancel ();
    completions= array<string> ();
  }
  input_mode= mode;
}

void
edit_interface_rep::set_input_normal () {
  int prev_input_mode= input_mode;
  set_input_mode (INPUT_NORMAL);

  if (prev_input_mode == INPUT_COMPLETE) {
    hide_completion_popup ();
  }
  prev_math_comb= "";
  hide_math_completion_popup ();
}

bool
edit_interface_rep::in_normal_mode () {
  return input_mode == INPUT_NORMAL;
}

bool
edit_interface_rep::in_search_mode () {
  return input_mode == INPUT_SEARCH;
}

bool
edit_interface_rep::in_replace_mode () {
  return input_mode == INPUT_REPLACE;
}

bool
edit_interface_rep::in_spell_mode () {
  return input_mode == INPUT_SPELL;
}

bool
edit_interface_rep::kbd_get_command (string which, string& help, command& c) {
  return sv->kbd_get_command (which, help, c);
}

/******************************************************************************
 * Main keyboard routines
 ******************************************************************************/

void
edit_interface_rep::interrupt_shortcut () {
  if (sh_mark != 0) mark_end (sh_mark);
  sh_s   = "";
  sh_mark= 0;
}

bool
edit_interface_rep::try_shortcut (string comb) {
  int     status;
  command cmd;
  string  shorth;
  string  help;

  sv->get_keycomb (comb, status, cmd, shorth, help);
  // cout << "Try " << comb << " -> " << shorth << ", " << help
  //<< "; " << sh_mark << ", " << status << "\n";
  if (status != 0) {
    if (status >= 3) {
      interrupt_shortcut ();
      status-= 3;
      if (status == 0) return false;
    }
    else {
      if (sh_mark != 0 && !mark_cancel (sh_mark)) {
        sh_mark= 0;
        return false;
      }
    }
    sh_s   = comb;
    sh_mark= new_marker ();
    mark_start (sh_mark);
    archive_state ();
    string rew_s= sv->kbd_post_rewrite (sh_s);
    tree   rew  = sv->kbd_system_rewrite (rew_s);
    if (N (help) > 0) set_message (help, rew);
    tree rhs= (shorth == rew_s ? tree ("") : sv->kbd_system_rewrite (shorth));
    // cout << "Shortcut: " << sh_s << " -> " << rew << "\n";
    if ((search_forwards (" ", comb) >= 0 && comb != " ") ||
        (search_forwards ("-", comb) >= 0 && comb != "-")) {
      tree t= rhs;
      if (is_compound (t, "render-key", 1)) t= t[0];
      if (is_func (t, WITH)) t= t[N (t) - 1];
      string r= as_string (t);
      if (starts (r, "<") && !starts (r, "<#"))
        if (cork_to_utf8 (r) != r)
          rhs= tree (CONCAT, rhs, " (" * r (1, N (r) - 1) * ")");
      call ("set-temporary-message", tree (CONCAT, "keyboard shortcut: ", rew),
            verbatim (rhs), shorth == "" ? 1 : 3000);
    }
    if ((status & 1) == 1) cmd ();
    else if (N (shorth) > 0) call ("kbd-insert", shorth);
    // cout << "Mark= " << sh_mark << "\n";
    string mode= as_string (call ("get-env", "mode"));
    if (mode == "math" && get_preference ("completion style") == "popup") {
      math_complete_try (comb);
      return true;
    }
    prev_math_comb= "";
    hide_math_completion_popup ();
    return true;
  }
  prev_math_comb= "";
  hide_math_completion_popup ();
  return false;
}

void
edit_interface_rep::math_complete_try (string comb) {
  comb           = replace (comb, "\\", "\\\\");
  bool need_popup= as_bool (call ("math-tabcycle-menu-needed?", comb));
  if (need_popup) {
    string wid_expr= (string) "(make-menu-widget " *
                     "`((tile 99 (link (lambda () " *
                     "(math-tabcycle-symbols ,\"" * comb * "\"))))) 0)";
    widget wid= as_widget (eval (wid_expr));
    cursor cu;
    if ((last_cursor != NULL) && (starts (prev_math_comb, comb))) {
      cu= last_cursor;
    }
    else {
      cu         = eb->find_check_cursor (previous_valid (et, tp));
      last_cursor= cu;
    }
    set_math_completion_popup (wid);
    show_math_completion_popup (cu, magf, get_scroll_x (), get_scroll_y (),
                                get_canvas_x ());
    prev_math_comb= comb;
  }
}

static string
std_accent (string s) {
  string c= "x";
  c[0]    = '\0';
  s       = replace (s, c, "`");
  c[0]    = '\1';
  s       = replace (s, c, "'");
  c[0]    = '\2';
  s       = replace (s, c, "^");
  c[0]    = '\3';
  s       = replace (s, c, "~");
  c[0]    = '\4';
  s       = replace (s, c, "\"");
  return s;
}

static bool          speech_pre_edit;
static time_t        last_uttering;
static array<string> pauses;
static string        current_speech;

static void
handle_speech (string s) {
  if (speech_pre_edit) {
    time_t now= texmacs_time ();
    if (now > last_uttering + 1000) {
      while (N (pauses) != 0 &&
             !starts (current_speech, pauses[N (pauses) - 1]))
        pauses= range (pauses, 0, N (pauses) - 1);
      if (N (pauses) == 0 || N (current_speech) > N (pauses[N (pauses) - 1]))
        pauses << current_speech;
    }
  }
  string       prefix= "";
  list<string> bursts;
  for (int i= 0; i < N (pauses); i++) {
    if (starts (s, pauses[i]) && N (pauses[i]) > N (prefix)) {
      string next= pauses[i];
      int    j   = N (prefix);
      while (j < N (next) && next[j] == ' ')
        j++;
      if (j < N (next)) bursts << next (j, N (next));
      prefix= next;
    }
  }
  if (N (s) > N (prefix)) {
    int j= N (prefix);
    while (j < N (s) && s[j] == ' ')
      j++;
    if (j < N (s)) bursts << s (j, N (s));
  }
  call ("keyboard-speech", bursts);
  last_uttering= texmacs_time ();
}

bool
edit_interface_rep::is_combo_shortcuts (string key) {
  return starts (key, "A-") || starts (key, "S-") || starts (key, "C-") ||
         starts (key, "M-");
}

void
edit_interface_rep::key_press (string gkey) {
  set_user_active (true);
  string zero= "a";
  zero[0]    = '\0';
  string key = replace (gkey, "<#0>", zero);

  if (starts (key, "pre-edit:") && speech_pre_edit &&
      ends (key, ":" * current_speech))
    return;

  if (pre_edit_mark != 0) {
    ASSERT (sh_mark == 0, "invalid shortcut during pre-edit");
    mark_cancel (pre_edit_mark);
    pre_edit_s   = "";
    pre_edit_mark= 0;
  }
  if (starts (key, "speech:")) {
    if (pre_edit_s != "") return;
    interrupt_shortcut ();
    archive_state ();
    handle_speech (key (7, N (key)));
    call ("speech-pause");
    speech_pre_edit= false;
    pauses         = array<string> ();
    current_speech = "";
    interrupt_shortcut ();
    return;
  }
  if (starts (key, "pre-edit:")) {
    string s= key (9, N (key));
    int    i, n= N (s), pos= N (s);
    for (i= 0; i < n; i++)
      if (s[i] == ':' && is_int (s (0, i))) {
        int k= as_int (s (0, i));
        s    = s (i + 1, n);
        pos  = 0;
        for (int j= 0; j < k && pos < N (s); j++)
          tm_char_forwards (s, pos);
        break;
      }
    if (as_bool (call ("disable-pre-edit?", std_accent (s)))) {
      pre_edit_skip= false;
      if (s == "") return;
      pre_edit_skip= true;
      key          = std_accent (s);
    }
    else if (pre_edit_skip) {
      if (s == "") pre_edit_skip= false;
      return;
    }
    else {
      if (s == "") return;
      interrupt_shortcut ();
      pre_edit_s   = s;
      pre_edit_mark= new_marker ();
      mark_start (pre_edit_mark);
      archive_state ();
      insert_tree (compound ("pre-edit", s), path (0, pos));
      return;
    }
  }
  else if (pre_edit_skip) {
    string u= cork_to_utf8 (key);
    string r= as_string (call ("downgrade-pre-edit", u));
    if (r == "") return;
    else key= r;
  }

  // 在辅助窗口中的编辑区域按下 Escape 键时，关闭辅助窗口
  // parent_window 非空标志了此窗口为辅助窗口
  url parent_window= concrete_window ()->parent;
  if (parent_window != url_none () && key == "escape") {
    exec_delayed (scheme_cmd (
        "(when (defined? 'close-auxiliary-widget) (close-auxiliary-widget))"));
    focus_on_buffer (window_to_buffer (parent_window), true);
  }

  string new_sh= N (sh_s) == 0 ? key : sh_s * " " * key;
  if (try_shortcut (new_sh)) return;
  if (new_sh != key) {
    interrupt_shortcut ();
    if (try_shortcut (key)) return;
  }

  string rew= sv->kbd_post_rewrite (key);
  if (N (rew) == 1) {
    int i ((unsigned char) rew[0]);
    if ((i >= 1 && i <= 127) || (i >= 128 && i <= 255) || (i == 25))
      if (!inside_active_graphics ()) {
        archive_state ();
        call ("kbd-insert", rew);
        source_complete_try ();
      }
    interrupt_shortcut ();
  }
  else if (contains_unicode_char (rew)) {
    archive_state ();
    call ("kbd-insert", key);
    interrupt_shortcut ();
  }
#ifdef Q_OS_MAC
  else if (N (key) == 3 && starts (key, "A-") &&
           qtcomposemap->contains ((int) (unsigned char) key[2])) {
    string compose_key= qtcomposemap[(int) (unsigned char) key[2]];
    key_press (compose_key);
  }
#endif
  else if (!occurs (" ", key) && N (key) > 1 && key[1] != '-' &&
           cork_to_utf8 ("<" * key * ">") != ("<" * key * ">") &&
           !inside_active_graphics ()) {
    archive_state ();
    call ("kbd-insert", "<" * key * ">");
    interrupt_shortcut ();
  }
  else if (N (key) > 1 && !is_combo_shortcuts (key) && key != "escape") {
    archive_state ();
    call ("insert", key);
    interrupt_shortcut ();
  }
  else {
    if (DEBUG_KEYBOARD) {
      debug_keyboard
          << "unrecognized key " << key << ". "
          << "Undefined shortcut or key missing in the encoding files.\n";
    }
  }
}

void
edit_interface_rep::emulate_keyboard (string keys, string action) {
  string s= keys;
  while (s != "") {
    int i;
    for (i= 1; i < N (s); i++)
      if (s[i] == ' ') break;
    call ("keyboard-press", object (s (0, i)), object ((double) 0));
    if (i < N (s)) i++;
    s= s (i, N (s));
  }
  if (N (action) != 0)
    set_message (concat ("You can also obtain ", action, " by typing ", keys),
                 action);
}

/******************************************************************************
 * Retrieving keyboard shortcuts
 ******************************************************************************/

tree
edit_interface_rep::kbd (string s) {
  return sv->kbd_system_rewrite (s);
}

tree
edit_interface_rep::kbd_shortcut (string cmd) {
  string s= as_string (eval ("(kbd-find-inv-binding '" * cmd * ")"));
  return kbd (s);
}

/******************************************************************************
 * Event handlers
 ******************************************************************************/

void
edit_interface_rep::handle_keypress (string key_u8, time_t t) {
  if (is_nil (buf)) return;

  string key        = utf8_to_cork (key_u8);
  bool   need_unwrap= true;
  int    key_N      = tm_string_length (key);
  for (int i= 0; i < key_N; i++) {
    string key_i= tm_forward_access (key, i);
    if (!(starts (key_i, "<") && ends (key_i, ">") && !starts (key_i, "<#"))) {
      need_unwrap= false;
      break;
    }
  }
  if (need_unwrap) {
    key= key (1, N (key) - 1);
  }

  if (t > last_event) last_event= t;
  bool started= false;
  try {
    if (DEBUG_KEYBOARD) {
      // for (int i=0; i<N(key); i++)
      //   cout << ((int) (unsigned char) key[i]) << " ";
      // cout << "\n";
      debug_keyboard << "Pressed " << key_u8 << "|" << key << " at " << t << LF;
      debug_keyboard << "  Codes";
      for (int i= 0; i < N (key); i++)
        debug_keyboard << " " << (unsigned int) (unsigned char) key[i];
      debug_keyboard << LF;
    }
    // time_t t1= texmacs_time ();
    if (is_nil (eb)) apply_changes ();
    start_editing ();
    started= true;

    string zero= "a";
    zero[0]    = '\0';
    string gkey= replace (key, zero, "<#0>");
    if (gkey == "<#3000>") gkey= "space";
    else if (gkey == "gtr") gkey= ">";
    else if (gkey == "less") gkey= "<";

    if (key_u8 == "`") {
      call ("keyboard-press", object (key_u8), object ((double) t));
    }
    else if (key_u8 == "‘") {
      call ("insert", "`");
    }
    else if (key_u8 == "’") {
      call ("insert", "<#2019>");
    }
    else if (key_u8 == "M-`") {
      call ("keyboard-press", object (key_u8), object ((double) t));
    }
    else if (key_u8 == "“”") {
      call ("insert", "<#201C><#201D>");
    }
    else if (key_u8 == "‘’") {
      call ("insert", "<#2018><#2019>");
    }
    else if (key_u8 == "——") {
      call ("insert", "<#2014><#2014>");
    }
    else if (starts (gkey, "pre-edit:")) {
      if (get_input_mode () == INPUT_COMPLETE) {
        // pre-edit之前确保不在补全模式下，以防意外触发补全
        set_input_normal ();
      }
      call ("delayed-keyboard-press", object (gkey), object ((double) t));
    }
    else {
      call ("keyboard-press", object (gkey), object ((double) t));
    }

    update_focus_loci ();
    if (!is_nil (focus_ids) && got_focus)
      call ("link-follow-ids", object (focus_ids), object ("focus"));
    notify_change (THE_DECORATIONS);
    // 键盘事件后更新文本工具栏显示状态
    update_text_toolbar ();
    end_editing ();
    // time_t t2= texmacs_time ();
    // if (t2 - t1 >= 10) cout << "handle_keypress took " << t2-t1 << "ms\n";
  } catch (string msg) {
    if (started) {
      cancel_editing ();
      interrupt_shortcut ();
    }
  }
  handle_exceptions ();
}

void drag_left_reset ();
void drag_right_reset ();

void
edit_interface_rep::handle_keyboard_focus (bool has_focus, time_t t) {
  if (is_nil (buf)) return;
  if (DEBUG_KEYBOARD) {
    if (has_focus) debug_keyboard << "Got focus at " << t << "\n";
    else debug_keyboard << "Lost focus at " << t << "\n";
  }
  if (got_focus != has_focus) {
    drag_left_reset ();
    drag_right_reset ();
  }
  got_focus= has_focus;
  if (got_focus) {
    cursor_blink_visible= true;
    cursor_blink_active = (cursor_blink_period > 0);
    if (cursor_blink_active)
      cursor_blink_next= texmacs_time () + cursor_blink_period;
  }
  else {
    cursor_blink_active = false;
    cursor_blink_visible= false;
    cursor_blink_next   = 0;
  }
  (void) t;
  notify_change (THE_FREEZE);
  notify_change (THE_FOCUS);
  if (got_focus) {
    focus_on_this_editor ();
    notify_change (THE_DECORATIONS);
  }
  call ("keyboard-focus", object (has_focus), object ((double) t));
}
