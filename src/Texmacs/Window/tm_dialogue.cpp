
/******************************************************************************
 * MODULE     : tm_dialogue.cpp
 * DESCRIPTION: Dialogues
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "convert.hpp"
#include "dictionary.hpp"
#include "file.hpp"
#include "message.hpp"
#include "preferences.hpp"
#include "sys_utils.hpp"
#include "tm_file.hpp"
#include "tm_frame.hpp"
#include "tm_window.hpp"
#include "tmfs_url.hpp"

#include <moebius/data/scheme.hpp>

using moebius::data::scheme_tree_to_tree;
using moebius::data::scm_unquote;

/******************************************************************************
 * Dialogues
 ******************************************************************************/

class dialogue_command_rep : public command_rep {
  server_rep* sv;
  object      fun;
  scheme_tree p;
  int         nr_args;

public:
  dialogue_command_rep (server_rep* sv2, object fun2, int nr_args2)
      : sv (sv2), fun (fun2), nr_args (nr_args2) {}
  dialogue_command_rep (server_rep* sv2, object fun2, scheme_tree p2)
      : sv (sv2), fun (fun2), p (p2), nr_args (N (p2)) {}
  void        apply ();
  tm_ostream& print (tm_ostream& out) { return out << "<command dialogue>"; }
};

static string get_type (scheme_tree p, int i);

void
dialogue_command_rep::apply () {
  int    i;
  object cmd  = null_object ();
  object learn= null_object ();
  for (i= nr_args - 1; i >= 0; i--) {
    string s_arg;
    sv->dialogue_inquire (i, s_arg);
    if (s_arg == "#f") {
      exec_delayed (scheme_cmd ("(dialogue-end)"));
      return;
    }
    object arg= string_to_object (s_arg);
    cmd       = cons (arg, cmd);
    if (!is_empty (p) && get_type (p, i) == "password")
      learn= cons (cons (object (as_string (i)), object ("")), learn);
    else learn= cons (cons (object (as_string (i)), arg), learn);
    // call ("learn-interactive-arg", fun, object (i), arg);
  }
  call ("learn-interactive", fun, learn);
  cmd= cons (fun, cmd);
  exec_delayed (scheme_cmd ("(dialogue-end)"));
  exec_delayed (scheme_cmd (cmd));
}

command
dialogue_command (server_rep* sv, object fun, scheme_tree p) {
  return tm_new<dialogue_command_rep> (sv, fun, p);
}

command
dialogue_command (server_rep* sv, object fun, int n) {
  return tm_new<dialogue_command_rep> (sv, fun, n);
}

void
tm_frame_rep::dialogue_start (string name, widget wid) {
  if (is_nil (dialogue_win)) {
    string lan= get_output_language ();
    if (lan == "russian") lan= "english";
    name        = translate (name, "english", lan);
    dialogue_wid= wid;
    dialogue_win= plain_window_widget (dialogue_wid, name);

    widget win= concrete_window ()->win;
    SI     ox, oy, dx, dy, ex= 0, ey= 0;
    get_position (win, ox, oy);
    get_size (win, dx, dy);
    get_size (dialogue_win, ex, ey);
    ox+= (dx - ex) >> 1;
    oy-= (dy - ey) >> 1;
    set_position (dialogue_win, ox, oy);
    set_visibility (dialogue_win, true);
  }
}

void
tm_frame_rep::dialogue_inquire (int i, string& arg) {
  if (i == 0) arg= get_string_input (dialogue_wid);
  else {
    widget field_i= get_form_field (dialogue_wid, i);
    arg           = get_string_input (field_i);
  }
}

void
tm_frame_rep::dialogue_end () {
  if (!is_nil (dialogue_win)) {
    set_visibility (dialogue_win, false);
    destroy_window_widget (dialogue_win);
    dialogue_win= widget ();
    dialogue_wid= widget ();
  }
}

/*
static int
gcd (int i, int j) {
  if (i<j)  return gcd (j, i);
  if (j==0) return i;
  return gcd (j, i%j);
}
*/

void
tm_frame_rep::choose_file (object fun, string title, string type, string prompt,
                           url name) {
  // 测试钩子：MOGAN_TEST_CHOOSE_FILE 非空时直接回调该路径。
  string preset= get_env ("MOGAN_TEST_CHOOSE_FILE");
  if (!is_empty (preset)) {
    array<object> args;
    args << object (url_system (preset));
    call (fun, args);
    return;
  }

  command cb = dialogue_command (get_server (), fun, 1);
  widget  wid= file_chooser_widget (cb, type, prompt);
  if (!is_scratch (name)) {
    set_directory (wid, as_string (head (name)));
    if ((type != "image") && (type != "")) {
      url    u      = tail (name);
      string old_suf= suffix (u);
      string new_suf= format_to_suffix (type);
      if ((suffix_to_format (suffix (u)) != type) && (old_suf != "") &&
          (new_suf != "")) {
        u= unglue (u, N (old_suf) + 1);
        u= glue (u, "." * new_suf);
      }
      set_file (wid, as_string (u));
    }
  }
  else {
    // The env HOME is set for Windows in research.cpp
    set_directory (wid, as_system_string (url_system ("$HOME")));
  }
#ifdef __EMSCRIPTEN__
  // WASM: 自身处理保存（同步保存+下载）/打开（JS 文件选择器），不需要
  // dialogue 窗口。但仍需设置 dialogue_wid，供 dialogue_command::apply 经
  // dialogue_inquire 读取选择结果。直接触发 perform_dialog，跳过窗口创建。
  dialogue_wid= wid;
  send_keyboard_focus (wid);
  return;
#endif
  dialogue_start (title, wid);
  if (type == "directory") send_keyboard_focus (get_directory (dialogue_wid));
  else send_keyboard_focus (get_file (dialogue_wid));
}

/******************************************************************************
 * Interactive commands
 ******************************************************************************/

static string
get_prompt (scheme_tree p, int i) {
  if (is_atomic (p[i]) && is_quoted (p[i]->label))
    return translate (scm_unquote (p[i]->label));
  else if (is_tuple (p[i]) && N (p[i]) > 0) {
    if (is_atomic (p[i][0]) && is_quoted (p[i][0]->label))
      return translate (scm_unquote (p[i][0]->label));
    return translate (scheme_tree_to_tree (p[i][0]));
  }
  return translate ("Input:");
}

static string
get_type (scheme_tree p, int i) {
  if (is_tuple (p[i]) && N (p[i]) > 1 && is_atomic (p[i][1]) &&
      is_quoted (p[i][1]->label))
    return scm_unquote (p[i][1]->label);
  return "string";
}

static array<string>
get_proposals (scheme_tree p, int i) {
  array<string> a;
  if (is_tuple (p[i]) && N (p[i]) >= 2) {
    int j, n= N (p[i]);
    for (j= 2; j < n; j++)
      if (is_atomic (p[i][j]) && is_quoted (p[i][j]->label))
        a << scm_unquote (p[i][j]->label);
  }
  return a;
}

class interactive_command_rep : public command_rep {
  server_rep*   sv;  // the underlying server
  tm_window     win; // the underlying TeXmacs window
  object        fun; // the function which is applied to the arguments
  scheme_tree   p;   // the interactive arguments
  int           i;   // counter where we are
  array<string> s;   // feedback from interaction with user

public:
  interactive_command_rep (server_rep* sv2, tm_window win2, object fun2,
                           scheme_tree p2)
      : sv (sv2), win (win2), fun (fun2), p (p2), i (0), s (N (p)) {}
  void        apply ();
  tm_ostream& print (tm_ostream& out) {
    return out << "<command interactive " << p << ">";
  }
};

void
interactive_command_rep::apply () {
  if ((i > 0) && (s[i - 1] == "#f")) return;
  if (i == N (p)) {
    object        learn= null_object ();
    array<object> params (N (p));
    for (i= N (p) - 1; i >= 0; i--) {
      params[i]= string_to_object (s[i]);
      if (get_type (p, i) == "password")
        learn= cons (cons (object (as_string (i)), object ("")), learn);
      else learn= cons (cons (object (as_string (i)), params[i]), learn);
    }
    call ("learn-interactive", fun, learn);
    string ret= object_to_string (call (fun, params));
    if (ret != "" && ret != "<unspecified>" && ret != "#<unspecified>")
      sv->set_message (verbatim (ret), "interactive command");
  }
  else {
    s[i]                   = string ("");
    string        prompt   = get_prompt (p, i);
    string        type     = get_type (p, i);
    array<string> proposals= get_proposals (p, i);
    win->interactive (prompt, type, proposals, s[i], this);
    i++;
  }
}

/******************************************************************************
 * WASM：绕过 inputs_list_widget/dialogue_start 桩，改由 React 模态对话框收集中
 ******************************************************************************/
#ifdef __EMSCRIPTEN__
// Transport（定义于 im_react_bridge.cpp）：把字段推给 React shell。
void im_react_push_dialog (string title, string prompts, string defaults,
                           string types);

// 一次最多一个活动交互对话框：暂存 scheme fun 与参数规格，等 React
// submit/cancel。
//
// `object` 必须用函数局部 static，不能是文件作用域 static 实例：object() 的
// 默认 ctor 会构造 tmscm_object_rep，触达 s7 运行时（tmscm_cons→GC）；而 WASM
// 文件作用域 static 在 __wasm_call_ctors 阶段构造，早于 s7 初始化（tm_s7 为
// null），会在启动期崩于 gc_mark（memory access out of bounds）。函数局部
// static 懒构造——首次调用是 im_wasm_start_dialog，仅由 interactive() 在运行期
// （s7 就绪后）触达。与项目既有 idiom 一致（new_style.cpp 的
// `static object cache;`）。scheme_tree/bool 的 ctor 是 s7-free，仍用普通 static。
static object&
wasm_dlg_fun_slot () {
  static object slot;
  return slot;
}
static scheme_tree g_wasm_dlg_p;
static bool        g_wasm_dlg_pending= false;

static string
join_newline (array<string> a) {
  string r;
  for (int i= 0; i < N (a); ++i) {
    if (i > 0) r << '\n';
    r << a[i];
  }
  return r;
}

// 由 tm_frame_rep::interactive 在 WASM 下调用：跳过桩化的
// inputs_list_widget/dialogue_start，改为推送字段给 React 模态。
void
im_wasm_start_dialog (object fun, scheme_tree p) {
  int           n= N (p);
  array<string> prompts (n), defaults (n), types (n);
  for (int i= 0; i < n; ++i) {
    prompts[i]             = get_prompt (p, i);
    types[i]               = get_type (p, i);
    array<string> proposals= get_proposals (p, i);
    defaults[i]            = (N (proposals) > 0) ? proposals[0] : string ("");
  }
  string title= translate ("Enter data");
  if (n > 0 && ends (prompts[0], "?")) title= translate ("Question");
  wasm_dlg_fun_slot ()= fun;
  g_wasm_dlg_p        = p;
  g_wasm_dlg_pending  = true;
  im_react_push_dialog (title, join_newline (prompts), join_newline (defaults),
                        join_newline (types));
}

// React 回传值（或取消）。镜像 dialogue_command_rep::apply（行 51-73）。
void
im_wasm_dialog_deliver (array<string> values, bool cancelled) {
  if (!g_wasm_dlg_pending) return;
  object&     fun_slot= wasm_dlg_fun_slot ();
  object      fun     = fun_slot;
  scheme_tree p       = g_wasm_dlg_p;
  g_wasm_dlg_pending  = false;
  fun_slot            = object ();
  g_wasm_dlg_p        = scheme_tree ();
  if (cancelled) return;
  int    nr   = N (p);
  object cmd  = null_object ();
  object learn= null_object ();
  for (int i= nr - 1; i >= 0; --i) {
    string s_arg= (i < N (values)) ? values[i] : string ("");
    object arg  = string_to_object (s_arg);
    cmd         = cons (arg, cmd);
    if (get_type (p, i) == "password")
      learn= cons (cons (object (as_string (i)), object ("")), learn);
    else learn= cons (cons (object (as_string (i)), arg), learn);
  }
  call ("learn-interactive", fun, learn);
  cmd= cons (fun, cmd);
  exec_delayed (scheme_cmd (cmd));
}
#endif // __EMSCRIPTEN__

void
tm_frame_rep::interactive (object fun, scheme_tree p) {
  ASSERT (is_tuple (p), "tuple expected");
#ifdef __EMSCRIPTEN__
  // WASM：N(p)>0 的交互式命令统一走 React 模态（绕过桩化的对话框控件）。
  if (N (p) > 0) {
    im_wasm_start_dialog (fun, p);
    return;
  }
#endif
  if (N (p) == 0) {
    string ret= object_to_string (call (fun));
    if (ret != "" && ret != "<unspecified>" && ret != "#<unspecified>")
      set_message (verbatim (ret), "interactive command");
  }
  else if (get_preference ("interactive questions") == "popup" || N (p) > 1 ||
           (is_aux_buffer (get_current_buffer_safe ()) &&
            !is_rooted_tmfs (get_current_buffer_safe (), "part"))) {
    int           i, n= N (p);
    array<string> prompts (n);
    for (i= 0; i < n; i++)
      prompts[i]= get_prompt (p, i);
    command cb = dialogue_command (get_server (), fun, p);
    widget  wid= inputs_list_widget (cb, prompts);
    for (i= 0; i < n; i++) {
      widget input_wid= get_form_field (wid, i);
      set_input_type (input_wid, get_type (p, i));
      array<string> proposals= get_proposals (p, i);
      int           j, k= N (proposals);
      if (k > 0) set_string_input (input_wid, proposals[0]);
      for (j= 0; j < k; j++)
        add_input_proposal (input_wid, proposals[j]);
    }
    string title= translate ("Enter data");
    if (ends (prompts[0], "?")) title= translate ("Question");
    dialogue_start (title, wid);
    send_keyboard_focus (get_form_field (dialogue_wid, 0));
  }
  else {
    if (concrete_window ()->get_interactive_mode ()) beep ();
    else {
      command interactive_cmd=
          tm_new<interactive_command_rep> (this, concrete_window (), fun, p);
      interactive_cmd ();
    }
  }
}
