
/******************************************************************************
 * MODULE     : edit_complete.cpp
 * DESCRIPTION: Tab completion
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
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
#include "tree_observer.hpp"
#include "tree_traverse.hpp"

#include <moebius/data/scheme.hpp>

using namespace moebius;
using moebius::data::scm_quote;
using moebius::data::scm_unquote;

/******************************************************************************
 * Finding completions in text
 ******************************************************************************/

static void
find_completions (drd_info drd, tree t, hashset<string>& h, string prefix= "") {
  if (is_atomic (t)) {
    string s= t->label;
    int    i= 0, n= N (s);
    while (i < n) {
      if (is_iso_alpha (s[i])) {
        int start= i;
        while ((i < n) && (is_iso_alpha (s[i])))
          i++;
        string r= s (start, i);
        if (starts (r, prefix) && (r != prefix))
          h->insert (r (N (prefix), N (r)));
      }
      else skip_symbol (s, i);
    }
  }
  else {
    int i, n= N (t);
    for (i= 0; i < n; i++)
      if (drd->is_accessible_child (t, i))
        find_completions (drd, t[i], h, prefix);
  }
}

static array<string>
find_completions (drd_info drd, tree t, string prefix= "",
                  string completion_style= "inline") {
  hashset<string> h;
  if (completion_style == string ("popup")) {
    h->insert (string ("")); // 在补全结果中插入一个空值以保留前缀自身
  }
  find_completions (drd, t, h, prefix);
  return as_completions (h);
}

/******************************************************************************
 * Completion mode
 ******************************************************************************/

bool
edit_interface_rep::complete_try () {
  completion_style= get_preference ("completion style");
  tree st         = subtree (et, path_up (tp));
  if (is_compound (st)) {
    set_input_normal ();
    return false;
  }
  string s           = st->label, ss;
  int    s_N         = N (s);
  int    end         = last_item (tp);
  int    min_compls_N= 1;
  if (completion_style == string ("popup")) {
    min_compls_N= 2; // 至少需要2个补全结果(前缀自身和一个补全结果)
  }
  array<string> a;
  if (inside (LABEL) || inside (REFERENCE) || inside (PAGEREF) ||
      inside (as_tree_label ("eqref")) ||
      inside (as_tree_label ("smart-ref"))) {
    if (end != s_N) {
      set_input_normal ();
      return false;
    }
    ss    = copy (s);
    tree t= get_labels ();
    int  i, n= N (t);
    for (i= 0; i < n; i++)
      if (is_atomic (t[i]) && starts (t[i]->label, s))
        a << string (t[i]->label (s_N, N (t[i]->label)));
    if (!contains (string (""), a)) {
      // 在补全结果中插入一个空值以保留前缀自身
      // 例如在 ref 中输入 "auto" 时，补全结果会包含以 "auto"
      // 自身和为以之前缀的补全 而不仅仅是以 "auto-" 开头的补全
      a= append (string (""), a);
    }
    if (input_mode == INPUT_COMPLETE && is_empty (ss)) {
      // 当处于补全模式且前缀为空时，退出补全模式
      // 此情况发生在补全模式中使用 backspace 删除到空时
      set_input_normal ();
      return false;
    }
  }
  else {
    if ((end == 0) || (!is_iso_alpha (s[end - 1])) ||
        ((end != s_N) && is_iso_alpha (s[end]))) {
      set_input_normal ();
      return false;
    }
    int start= end - 1;
    while ((start > 0) && is_iso_alpha (s[start - 1]))
      start--;
    ss= s (start, end);
    a = find_completions (drd, et, ss, completion_style);
  }
  int a_N= N (a);
  if (a_N < min_compls_N) {
    set_input_normal ();
    return false;
  }
  complete_start (ss, a);
  return true;
}

void
edit_interface_rep::complete_message () {
  int    i, n= N (completions);
  string s  = "";
  string sep= translate (", "); // Might be needed for oriental languages
  for (i= 1; i < min (n, 11); i++) {
    int j= (completion_pos + i) % n;
    if (i != 1) s << sep;
    s << completion_prefix << completions[j];
  }
  if (completion_style == string ("inline")) {
    set_message (concat ("Other completions: ", verbatim (s)), "tab");
  }
}

void
edit_interface_rep::complete_start (string prefix, array<string> compls) {
  // check consistency
  tree st= subtree (et, path_up (tp));
  if (is_compound (st)) return;
  string s  = st->label;
  int    end= last_item (tp);
  if ((end < N (prefix)) || (s (end - N (prefix), end) != prefix)) return;

  // perform first completion and switch to completion mode if necessary
  else {
    completion_prefix= prefix;
    completions      = close_completions (compls);
    completion_pos   = 0;

    int           completions_N= N (completions);
    array<string> full_completions;
    for (int i= 0; i < completions_N; ++i) {
      string c= completions[i];
      full_completions << (completion_prefix * c);
    }
    path tp1     = tp;
    int  prefix_N= N (completion_prefix);
    for (int i= 0; (i < prefix_N && last_item (tp1) > 0); ++i) {
      tp1= previous_valid (et, tp1); // 向前移动到前缀的起始位置
    }
    cursor cu= eb->find_check_cursor (tp1);
    if (completion_style == string ("popup")) {
#ifdef QTTEXMACS
      show_completion_popup ("text", tp, full_completions, cu, magf,
                             get_scroll_x (), get_scroll_y (), get_canvas_x (),
                             get_canvas_y ());
#endif
    }

    insert_tree (completions[0]);

    complete_message ();
    set_input_mode (INPUT_COMPLETE);
  }
}

bool
edit_interface_rep::complete_keypress (string key) {
  completion_style= get_preference ("completion style");
  if (completion_style == string ("popup")) {
    return complete_popup (key);
  }
  else {
    return complete_inline (key);
  }
}

bool
edit_interface_rep::complete_popup (string key) {
  // Popup 补全的键盘响应流程
  int    completions_N    = N (completions);
  string old_completion   = completions[completion_pos];
  string full_completion  = completion_prefix * old_completion;
  int    full_completion_N= N (full_completion);
  if (key == "space") key= " ";

  if (key == "enter" || key == "return") {
    set_input_normal ();
    return true;
  }
  else if (key == "escape" || key == " ") {
    set_input_normal ();
    return false;
  }
  else if ((key != "tab") && (key != "S-tab") && (key != "up") &&
           (key != "down")) {
    int     status;
    command _;  // unused
    string  __; // unused
    sv->get_keycomb (key, status, _, __, __);
    if (key != "backspace" && (status & 1) == 1) {
      set_input_normal ();
    }
    return false;
  }
  tree   st= subtree (et, path_up (tp));
  string s = st->label;
  if (is_compound (st)) {
    set_input_normal ();
    return false;
  }
  int end= last_item (tp);

  if (key == "tab" || key == "down") {
    completion_pos++;
#ifdef QTTEXMACS
    completion_popup_next (true);
#endif
  }
  else if (key == "S-tab" || key == "up") {
    completion_pos--;
#ifdef QTTEXMACS
    completion_popup_next (false);
#endif
  }
  if (completion_pos < 0) completion_pos= completions_N - 1;
  if (completion_pos >= completions_N) completion_pos= 0;
  complete_message ();
  return true;
}

bool
edit_interface_rep::complete_inline (string key) {
  // Inline 补全的键盘响应流程
  set_message ("", "");
  if (key == "space") key= " ";
  if ((key != "tab") && (key != "S-tab")) {
    set_input_normal ();
    return false;
  }
  tree st= subtree (et, path_up (tp));
  if (is_compound (st)) {
    set_input_normal ();
    return false;
  }
  string s            = st->label;
  int    end          = last_item (tp);
  string old_s        = completions[completion_pos];
  string test         = completion_prefix * old_s;
  int    test_N       = N (test);
  int    completions_N= N (completions);
  if ((end < test_N) || (s (end - test_N, end) != test)) {
    set_input_normal ();
    return false;
  }

  if (key == "tab") completion_pos++;
  else completion_pos--;
  if (completion_pos < 0) completion_pos= completions_N - 1;
  if (completion_pos >= completions_N) completion_pos= 0;
  string new_s  = completions[completion_pos];
  int    old_s_N= N (old_s);
  remove (path_up (tp) * (end - old_s_N), old_s_N);
  insert (path_up (tp) * (end - old_s_N), new_s);
  complete_message ();
  return true;
}

void
edit_interface_rep::complete_variant (string old_completion,
                                      string new_completion) {
  // QTMCompletionPopup::onCurrentItemChanged 会调用此函数当补全项改变时进行编辑
  if (completion_style != string ("popup")) {
    return; // 仅在 popup 补全模式下有效，注意此处使用了缓存的 completion_style
  }
  if (get_input_mode () != INPUT_COMPLETE) {
    return; // 仅在补全模式下有效
  }
  int completion_prefix_N= N (completion_prefix);
  old_completion= old_completion (completion_prefix_N, N (old_completion));
  new_completion= new_completion (completion_prefix_N, N (new_completion));
  int end       = last_item (tp);
  int old_completion_N= N (old_completion);
  remove (path_up (tp) * (end - old_completion_N), old_completion_N);
  insert (path_up (tp) * (end - old_completion_N), new_completion);
  apply_changes ();

#ifdef QTTEXMACS
  update_completion_popup_position (et, eb, tp, magf, get_scroll_x (),
                                    get_scroll_y (), get_canvas_x (),
                                    get_canvas_y (), completion_pos);
#endif
}

/******************************************************************************
 * Tab completion inside sessions
 ******************************************************************************/

static string cursor_symbol ("[tmcursor]");

static tree
put_cursor (tree t, path p) {
  if (is_atomic (t)) {
    string s= t->label;
    return s (0, p->item) * cursor_symbol * s (p->item, N (s));
  }
  else {
    if (p == path (0)) return tree (CONCAT, cursor_symbol, t);
    else if (p == path (1)) return tree (CONCAT, t, cursor_symbol);
    else {
      int  i, n= N (t);
      tree u (t, n);
      for (i= 0; i < n; i++)
        if (i == p->item) u[i]= put_cursor (t[i], p->next);
        else u[i]= t[i];
      return u;
    }
  }
}

string
edit_interface_rep::session_complete_command (tree tt) {
  path p = reverse (obtain_ip (tt));
  tree st= subtree (et, p);
  if ((N (tp) <= N (p)) || (tp[N (p)] != 1)) return "";
  tree t= put_cursor (st[1], tail (tp, N (p) + 1));
  // cout << t << LF;

  (void) eval ("(use-modules (utils plugins plugin-cmd))");
  string lan= get_env_string (PROG_LANGUAGE);
  string ses= get_env_string (PROG_SESSION);
  tree   u  = tree_herk_to_utf8 (t);
  string s  = as_string (call ("utf8raw-serialize", lan, tree_to_stree (u)));
  s         = s (0, N (s) - 1);

  int pos= search_forwards (cursor_symbol, s);
  if (pos == -1) return "";
  s= s (0, pos) * s (pos + N (cursor_symbol), N (s));
  // cout << s << ", " << pos << LF;
  return "(complete " * scm_quote (s) * " " * as_string (pos) * ")";
}

void
edit_interface_rep::custom_complete (tree r) {
  completion_style= get_preference ("completion style");
  if (!is_tuple (r)) return;
  int             i, r_N= N (r);
  int             min_compls_N= 1;
  string          prefix;
  bool            prefix_initialized= false;
  hashset<string> compls;
  if (completion_style == string ("popup")) {
    min_compls_N= 2;       // 至少需要2个补全结果(前缀自身和一个补全结果)
    compls << string (""); // 在补全结果中插入一个空值以保留前缀自身
  }
  for (i= 0; i < r_N; i++) {
    if (is_atomic (r[i])) {
      string l= r[i]->label;
      if (is_quoted (l)) l= scm_unquote (l);
      if (!prefix_initialized) {
        // 这里使用补全的第一个结果（自身）定义了 prefix
        // 第一个结果可能为空，所以不能使用 prefix = "" 来判断
        // 需要一个另外的辅助变量
        prefix            = l;
        prefix_initialized= true;
        continue;
      }
      compls << l;
    }
  }
  // cout << prefix << ", " << compls << LF;
  if ((prefix == "") || (N (compls) < min_compls_N)) {
    set_input_normal ();
    return;
  }
  complete_start (prefix, as_completions (compls));
}

/******************************************************************************
 * Computations with completions
 ******************************************************************************/

array<string>
as_completions (hashset<string> h) {
  tree          t= as_tree (h);
  int           i, n= N (t);
  array<string> a (n);
  for (i= 0; i < n; i++)
    a[i]= t[i]->label;
  merge_sort (a);
  return a;
}

/*
static void
close_completions (hashset<string>& h) {
  array<string> a= as_completions (h);
  int i, j, n= N(a);
  for (i=1; i<n; i++) {
    for (j=0; j < min (N(a[i-1]), N(a[i])); j++)
      if (a[i-1][j] != a[i][j]) break;
    if (j < min (N(a[i-1]), N(a[i])))
      h->insert (a[i](0,j));
  }
}

array<string>
close_completions (array<string> a) {
  int i, n= N(a);
  hashset<string> h;
  for (i=0; i<n; i++) h->insert (a[i]);
  close_completions (h);
  return as_completions (h);
}
*/

array<string>
close_completions (array<string> a) {
  if (N (a) == 0) return a;
  merge_sort (a);
  int i, j, n= N (a), l= N (a[0]);
  for (i= 1; i < n; i++) {
    for (j= 0; j < l && j < N (a[i]); j++)
      if (a[i - 1][j] != a[i][j]) break;
    l= j;
  }
  array<string> r;
  r << a[0](0, l);
  for (i= 0; i < n; i++)
    if (a[i] != r[N (r) - 1]) r << a[i];
  return r;
}

array<string>
strip_completions (array<string> a, string prefix) {
  int           i, n= N (a);
  array<string> b;
  for (i= 0; i < n; i++)
    if (starts (a[i], prefix)) b << a[i](N (prefix), N (a[i]));
  return b;
}
