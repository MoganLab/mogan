
/******************************************************************************
 * MODULE     : im_gui.cpp
 * DESCRIPTION: ImGui implementations (stubs) of the top-level widget
 *              factories
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "convert.hpp" // tree_to_generic (verbatim for system clipboard)
#include "font.hpp"
#include "gui.hpp"               // gui_start_loop
#include "im_chooser_widget.hpp" // im_chooser_widget_rep (file chooser)
#include "im_menu.hpp"           // im_menu_rep / im_popup_rep (menus & popups)
#include "im_simple_widget.hpp"  // im_simple_widget_rep (glue placeholder)
#include "im_tm_widget.hpp"      // im_primary_glfw_window (clipboard)
#include "im_widget.hpp"
#include "object.hpp"
#include "promise.hpp"
#include "renderer.hpp"
#include "scheme.hpp"
#include "tm_timer.hpp" // texmacs_time (delayed-command scheduling)
#include "widget.hpp"

#include <GLFW/glfw3.h> // system clipboard (glfwSet/GetClipboardString)

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib> // setenv, free (browser-OS detection)

/******************************************************************************
 * Helpers
 ******************************************************************************/

static widget
im_stub_widget () {
  return widget ((widget_rep*) tm_new<im_widget_rep> ());
}

// Whether gui_open() has initialized the process-global GLFW backend. Guards
// glfwInit() (called once) and the matching glfwTerminate() in gui_close().
static bool s_glfw_initialized= false;

static void
im_glfw_error_callback (int err, const char* description) {
  std::fprintf (stderr, "GLFW Error %d: %s\n", err, description);
}

#ifdef __EMSCRIPTEN__
// 探测浏览器宿主 OS，供 Scheme 层选择匹配的键盘 look-and-feel
// （macos → Cmd 快捷键，windows → Ctrl 快捷键）。Emscripten 没有原生平台
// API，这里通过 JS 读 navigator 判定，结果存入环境变量 MOGAN_BROWSER_OS，
// 由 tm-preferences.scm 的 default-look-and-feel 读取。本函数在 gui_open()
// 中调用，而 gui_open() 在 server/Scheme 初始化之前执行，故键盘模块加载
// 时即可见到该环境变量。
static void
im_detect_browser_os () {
  char* raw= (char*) emscripten_run_script_string (
      "(function(){"
      "var s= (navigator.platform||'') + ' ' + (navigator.userAgent||'');"
      "if(/Mac/i.test(s)) return 'macos';"
      "if(/Win/i.test(s)) return 'windows';"
      "return '';"
      "})()");
  string r= (raw == nullptr) ? string ("") : string (raw);
  free (raw);
  if (N (r) > 0) {
    c_string cs (r);
    setenv ("MOGAN_BROWSER_OS", cs, 1);
  }
}
#endif

/******************************************************************************
 * Top-level window factories (real implementation)
 ******************************************************************************/

// The main TeXmacs widget, owned by im_tm_widget_rep.
widget
texmacs_widget (int mask, command quit) {
  return tm_new<im_tm_widget_rep> (mask, quit);
}

// Promote a widget into a window. Only im_tm_widget_rep is a real window in
// this port; its plain_window_widget() returns itself.
widget
plain_window_widget (widget w, string name, command q) {
  return static_cast<im_widget_rep*> (w.rep)->plain_window_widget (name, q);
}

widget
popup_window_widget (widget w, string s) {
  // 菜单弹出容器：返回自身，使 set_visibility / send_mouse_grab 命中同一 rep。
  im_popup_rep* p= dynamic_cast<im_popup_rep*> (w.rep);
  if (p != nullptr) return widget ((widget_rep*) p);
  return static_cast<im_widget_rep*> (w.rep)->popup_window_widget (s);
}

widget
tooltip_window_widget (widget w, string s) {
  return static_cast<im_widget_rep*> (w.rep)->tooltip_window_widget (s);
}

void
destroy_window_widget (widget w) {
  (void) w;
  // Note the widget's destructor owns the GLFW/ImGui teardown
}

void
gui_start_loop () {
  // The first window created via ensure_window() -> ... -> texmacs_widget()
  im_run_main_loop ();
}

/******************************************************************************
 * Dialog / chooser factories (stubs)
 ******************************************************************************/

widget
file_chooser_widget (command cmd, string type, string prompt) {
  return widget (
      (widget_rep*) tm_new<im_chooser_widget_rep> (cmd, type, prompt));
}

widget
printer_widget (command cmd, url ps_pdf_file) {
  (void) cmd;
  (void) ps_pdf_file;
  return widget ();
}

widget
color_picker_widget (command cmd, bool bg, array<tree> proposals) {
  (void) cmd;
  (void) bg;
  (void) proposals;
  return im_stub_widget ();
}

widget
inputs_list_widget (command call_back, array<string> prompts) {
  (void) call_back;
  (void) prompts;
  return im_stub_widget ();
}

widget
popup_widget (widget w) {
  // 包一层弹出容器：持有 vertical_menu 根，处理弹出协议并在帧循环里渲染。
  return widget ((widget_rep*) tm_new<im_popup_rep> (w));
}

/******************************************************************************
 * Menu factories
 *
 * 参考 qt_widget.cpp 的 qt_ui_element_rep::create + add_children：每个工厂构造
 * 一个 im_menu_rep 节点（按 kind），把参数存入字段，容器类用 add_children 挂
 * 子节点。渲染由 im_menu.cpp 的 render_node 每帧递归完成。
 ******************************************************************************/

// 从标签 widget 取 cork 原文。balloon_widget 已在工厂层解包，故这里直接读
// im_menu_rep::label（text_widget 节点）。
static string
im_extract_label (widget w) {
  im_menu_rep* m= dynamic_cast<im_menu_rep*> (w.rep);
  return (m == nullptr) ? string ("") : m->label;
}

widget
horizontal_menu (array<widget> a) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_container);
  m->add_children (a);
  return widget ((widget_rep*) m);
}
widget
vertical_menu (array<widget> a) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_container);
  m->add_children (a);
  return widget ((widget_rep*) m);
}
widget
tile_menu (array<widget> a, int cols) {
  (void) cols; // ImGui 暂以普通容器渲染（不分行）
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_container);
  m->add_children (a);
  return widget ((widget_rep*) m);
}
widget
minibar_menu (array<widget> a) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_container);
  m->add_children (a);
  return widget ((widget_rep*) m);
}
widget
menu_separator (bool vertical) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_separator);
  m->vertical   = vertical;
  return widget ((widget_rep*) m);
}
widget
menu_group (string name, int style) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_group);
  m->label      = name;
  m->style      = style;
  return widget ((widget_rep*) m);
}
widget
pulldown_button (widget w, promise<widget> pw) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_submenu);
  m->label      = im_extract_label (w);
  m->sub        = pw;
  return widget ((widget_rep*) m);
}
widget
pullright_button (widget w, promise<widget> pw) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_submenu);
  m->label      = im_extract_label (w);
  m->sub        = pw;
  return widget ((widget_rep*) m);
}
widget
menu_button (widget w, command cmd, string pre, string ks, int style) {
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_button);
  m->label      = im_extract_label (w);
  m->cmd        = cmd;
  m->pre        = pre;
  m->ks         = ks;
  m->style      = style;
  return widget ((widget_rep*) m);
}
widget
balloon_widget (widget w, widget help) {
  // ImGui 端暂不显示悬浮帮助；解包返回内层 widget（通常为 text_widget），
  // 使父级 menu_button / pulldown_button 能取到标签。
  (void) help;
  return w;
}

/******************************************************************************
 * Leaf widget factories (stubs)
 ******************************************************************************/

widget
text_widget (string s, int style, color col, bool tsp) {
  (void) col;
  (void) tsp;
  // 作为菜单标签来源：存 cork 原文，渲染时由 display_label 翻译为 utf8。
  im_menu_rep* m= tm_new<im_menu_rep> (im_menu_rep::k_text);
  m->label      = s;
  m->style      = style;
  return widget ((widget_rep*) m);
}
widget
xpm_widget (url file_name) {
  (void) file_name;
  return im_stub_widget ();
}
widget
input_text_widget (command call_back, string type, array<string> def, int style,
                   string width) {
  (void) call_back;
  (void) type;
  (void) def;
  (void) style;
  (void) width;
  return im_stub_widget ();
}
widget
numeric_input_widget (command call_back, string width, string unit, int min_val,
                      int max_val, int step, int def) {
  (void) call_back;
  (void) width;
  (void) unit;
  (void) min_val;
  (void) max_val;
  (void) step;
  (void) def;
  return im_stub_widget ();
}
widget
enum_widget (command cb, array<string> vals, string val, int st, string w) {
  (void) cb;
  (void) vals;
  (void) val;
  (void) st;
  (void) w;
  return im_stub_widget ();
}
widget
choice_widget (command cb, array<string> vals, string val) {
  (void) cb;
  (void) vals;
  (void) val;
  return im_stub_widget ();
}
widget
choice_widget (command cb, array<string> vals, array<string> mc) {
  (void) cb;
  (void) vals;
  (void) mc;
  return im_stub_widget ();
}
widget
choice_widget (command cb, array<string> vals, string val, string filt) {
  (void) cb;
  (void) vals;
  (void) val;
  (void) filt;
  return im_stub_widget ();
}
widget
tree_view_widget (command cmd, tree data, tree data_roles) {
  (void) cmd;
  (void) data;
  (void) data_roles;
  return im_stub_widget ();
}
widget
tab_page_widget (url u, widget title, widget close_btn, bool is_active) {
  (void) u;
  (void) title;
  (void) close_btn;
  (void) is_active;
  return im_stub_widget ();
}

/******************************************************************************
 * Container / layout factories (stubs)
 ******************************************************************************/

widget
empty_widget () {
  return widget ();
}
widget
glue_widget (bool hx, bool vx, SI w, SI h) {
  (void) hx;
  (void) vx;
  (void) w;
  (void) h;
  return im_stub_widget ();
}
widget
glue_widget (tree col, bool hx, bool vx, SI w, SI h) {
  (void) col;
  (void) hx;
  (void) vx;
  (void) w;
  (void) h;
  return im_stub_widget ();
}
widget
horizontal_list (array<widget> a) {
  (void) a;
  return im_stub_widget ();
}
widget
vertical_list (array<widget> a) {
  (void) a;
  return im_stub_widget ();
}
widget
division_widget (string name, widget w) {
  (void) name;
  (void) w;
  return im_stub_widget ();
}
widget
aligned_widget (array<widget> lhs, array<widget> rhs, SI hsep, SI vsep, SI lpad,
                SI rpad) {
  (void) lhs;
  (void) rhs;
  (void) hsep;
  (void) vsep;
  (void) lpad;
  (void) rpad;
  return im_stub_widget ();
}
widget
tabs_widget (array<widget> tabs, array<widget> bodies) {
  (void) tabs;
  (void) bodies;
  return im_stub_widget ();
}
widget
icon_tabs_widget (array<url> us, array<widget> ss, array<widget> bs) {
  (void) us;
  (void) ss;
  (void) bs;
  return im_stub_widget ();
}
widget
wrapped_widget (widget w, command quit) {
  (void) w;
  (void) quit;
  return im_stub_widget ();
}
widget
user_canvas_widget (widget wid, int style) {
  (void) wid;
  (void) style;
  return im_stub_widget ();
}
widget
resize_widget (widget w, int style, string w1, string h1, string w2, string h2,
               string w3, string h3, string hpos, string vpos) {
  (void) w;
  (void) style;
  (void) w1;
  (void) h1;
  (void) w2;
  (void) h2;
  (void) w3;
  (void) h3;
  (void) hpos;
  (void) vpos;
  return im_stub_widget ();
}
widget
hsplit_widget (widget l, widget r) {
  (void) l;
  (void) r;
  return im_stub_widget ();
}
widget
vsplit_widget (widget t, widget b) {
  (void) t;
  (void) b;
  return im_stub_widget ();
}
widget
extend_widget (widget w, array<widget> a) {
  (void) a;
  return w;
}
widget
toggle_widget (command cmd, bool on, int style) {
  (void) cmd;
  (void) on;
  (void) style;
  return im_stub_widget ();
}
widget
wait_widget (SI width, SI height, string message) {
  (void) width;
  (void) height;
  (void) message;
  return widget ();
}
widget
ink_widget (command cb) {
  (void) cb;
  return widget ();
}
widget
refresh_widget (string tmwid, string kind) {
  (void) tmwid;
  (void) kind;
  return im_stub_widget ();
}
widget
refreshable_widget (object prom, string kind) {
  (void) prom;
  (void) kind;
  return im_stub_widget ();
}

void
gui_open (int& argc, char** argv) {
  (void) argc;
  (void) argv;
  // The guard makes repeated calls harmless
  if (!s_glfw_initialized) {
    glfwSetErrorCallback (&im_glfw_error_callback);
    if (glfwInit ()) s_glfw_initialized= true;
  }
#ifdef __EMSCRIPTEN__
  // 在 Scheme/键盘模块初始化之前完成宿主 OS 探测，使 default-look-and-feel
  // 能据 MOGAN_BROWSER_OS 选出匹配的 look-and-feel。
  im_detect_browser_os ();
#endif
}

// texmacs_interpose_handler() in tm_server.cpp 注册的 interpose 回调
// 负责执行 perform_select、exec_pending_commands、各编辑器的
// apply_changes()、animate() 以及 windows_refresh()。Qt 在其事件循环中
// 驱动该回调; ImGui 后端则必须在自己的帧循环（im_interpose）中驱动它，
// 否则 env_change 永远不会被清除，handle_repaint() 会因
// "Invalid situation" 而提前返回。将其保存在这里，让 ImGui 驱动这一回调。
static void (*g_interpose_fn) (void)= nullptr;

void
gui_interpose (void (*fn) (void)) {
  g_interpose_fn= fn;
}

void
im_interpose () {
  if (g_interpose_fn != nullptr) g_interpose_fn ();
}

void
gui_close () {
  if (s_glfw_initialized) {
    glfwTerminate ();
    s_glfw_initialized= false;
  }
}

void
gui_root_extents (SI& width, SI& height) {
  // get the screen size: the GLFW counterpart of Qt's
  // QGuiApplication::primaryScreen()->size()
#ifdef __EMSCRIPTEN__
  // Browsers do not expose a meaningful primary monitor to GLFW.
  // Use the current canvas CSS size as the root extent instead.
  double css_w, css_h;
  emscripten_get_element_css_size ("#main-canvas", &css_w, &css_h);
  width = (SI) css_w * PIXEL;
  height= (SI) css_h * PIXEL;
  return;
#else
  int w= 1920, h= 1080;
  if (s_glfw_initialized) {
    GLFWmonitor*       monitor= glfwGetPrimaryMonitor ();
    const GLFWvidmode* mode   = monitor ? glfwGetVideoMode (monitor) : nullptr;
    if (mode) {
      w= mode->width;
      h= mode->height;
    }
  }
  width = w * PIXEL;
  height= h * PIXEL;
#endif
}

void
gui_maximal_extents (SI& width, SI& height) {
  gui_root_extents (width, height);
}

void
gui_refresh () {}

string
gui_version () {
  return "headless";
}

void
set_default_font (string name) {
  (void) name;
}

font
get_default_font (bool tt, bool mini, bool bold) {
  (void) tt;
  (void) mini;
  (void) bold;
  // stub
  return tex_font ("modern", 10, 300, 0);
}

void
load_system_font (string family, int size, int dpi, font_metric& fnm,
                  font_glyphs& fng) {
  (void) family;
  (void) size;
  (void) dpi;
  (void) fnm;
  (void) fng;
  // System fonts are optional; the viewer relies on TeX/Freetype fonts.
}

// Clipboard (copy/cut/paste). The editor reaches the *system* clipboard via
// the "primary" key (C-c/C-v/C-x → kbd-copy/cut/paste → clipboard-* "primary"),
// so "primary" is bridged to the OS/browser clipboard while the structured
// texmacs tree+text is kept internally. On paste we return the structured tree
// while we still own the clipboard (preserves math/formatting on intra-app
// paste), otherwise the foreign plain text.
//
// GLFW's emscripten port does NOT implement the clipboard —
// glfwSet/GetClipboard are no-ops there — so on WASM we use the browser
// clipboard directly:
//   copy  → navigator.clipboard.writeText (called from set_selection);
//   paste → the browser "paste" event hands us the text synchronously; we
//           buffer it and re-drive the paste from the main loop
//           (im_clipboard_consume_paste), because get_selection runs during
//           the keydown, before the paste event fires. GLFW's own Ctrl-V is
//           blocked in im_install_ime_listeners so it can't paste stale data
//           first.
// On desktop GLFW the real glfwSet/GetClipboardString are used instead.
static hashmap<string, tree>   im_sel_t= hashmap<string, tree> (tree (TUPLE));
static hashmap<string, string> im_sel_s= hashmap<string, string> ("");
static string                  im_last_clip_text= "";
#ifdef __EMSCRIPTEN__
static string g_clipboard_foreign    = "";    // text captured by a paste event
static bool g_clipboard_paste_pending= false; // a paste event armed a re-drive

EM_JS (void, im_clipboard_write, (const char* utf8), {
  var s= UTF8ToString (utf8);
  try {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      var p= navigator.clipboard.writeText (s);
      if (p && p.catch) p.catch (function (){});
    }
  } catch (e) {
  }
});

// Browser paste event → here. Buffer the foreign text and arm a deferred paste
// (drained by im_main_loop via im_clipboard_consume_paste).
extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_clipboard_deliver_paste (const char* utf8) {
  g_clipboard_foreign      = (utf8 == nullptr) ? string ("") : string (utf8);
  g_clipboard_paste_pending= true;
}

// im_main_loop: consume one armed paste; true ⇒ re-drive the paste now.
bool
im_clipboard_consume_paste () {
  if (!g_clipboard_paste_pending) return false;
  g_clipboard_paste_pending= false;
  return true;
}
#endif // __EMSCRIPTEN__

// Verbatim (plain-text) rendering of a selection envelope
// ("texmacs" content mode lan) — what we mirror to the system clipboard so
// external applications receive readable text.
static string
im_selection_verbatim (tree t) {
  tree content= (is_tuple (t, "texmacs") && N (t) >= 2) ? t[1] : t;
  return tree_to_generic (content, "verbatim-snippet");
}

// Current text on the system clipboard (for ownership / foreign detection).
static string
im_system_clip_text () {
#ifdef __EMSCRIPTEN__
  return g_clipboard_foreign; // refreshed by paste events
#else
  GLFWwindow* w= im_primary_glfw_window ();
  if (w == nullptr) return "";
  const char* p= glfwGetClipboardString (w);
  return (p == nullptr) ? string ("") : string (p);
#endif
}

bool
set_selection (string cb, tree t, string s, string sv, string sh,
               string format) {
  (void) sv;
  (void) sh;
  (void) format;
  im_sel_t (cb)= copy (t);
  im_sel_s (cb)= copy (s);
  if (cb == "primary") {
    // GLFW/browser expose only plain text; mirror the verbatim rendering so
    // external apps receive readable text (intra-app paste still uses the
    // structured tree stored above).
    string plain     = im_selection_verbatim (t);
    im_last_clip_text= plain;
#ifdef __EMSCRIPTEN__
    g_clipboard_foreign= ""; // we own the clipboard again
    if (N (plain) > 0) im_clipboard_write (as_charp (plain));
#else
    GLFWwindow* w= im_primary_glfw_window ();
    if (w != nullptr && N (plain) > 0) {
      c_string cs (plain);
      glfwSetClipboardString (w, cs);
    }
#endif
  }
  return true;
}

bool
get_selection (string cb, tree& t, string& s, string format) {
  (void) format;
  if (cb == "primary") {
    string cur= im_system_clip_text ();
    // Do we still own the clipboard? On desktop we compare against the live
    // system text; on WASM the text is only knowable from paste events, so an
    // empty (not-yet-captured) buffer means "assume ours".
    bool own;
#ifdef __EMSCRIPTEN__
    own= is_empty (cur) || cur == im_last_clip_text;
#else
    own= !is_empty (im_last_clip_text) && cur == im_last_clip_text;
#endif
    if (own && im_sel_t->contains (cb)) {
      t= copy (im_sel_t[cb]);
      s= copy (im_sel_s[cb]);
      return true;
    }
    // Foreign content → paste as plain text.
    t= tuple ("extern", copy (cur));
    s= copy (cur);
    return !is_empty (cur);
  }
  // "mouse"/"clipboard"/...: no system API (WASM) or internal-only; serve the
  // internal store.
  if (im_sel_t->contains (cb)) {
    t= copy (im_sel_t[cb]);
    s= copy (im_sel_s[cb]);
    return true;
  }
  return false;
}

void
clear_selection (string cb) {
  im_sel_t->reset (cb);
  im_sel_s->reset (cb);
  if (cb == "primary") {
    im_last_clip_text= "";
#ifdef __EMSCRIPTEN__
    g_clipboard_foreign= "";
#endif
  }
}

void
beep () {}

void
needs_update () {}

bool
check_event (int type) {
  (void) type;
  return false;
}

void
image_gc (string name) {
  (void) name;
}

void
show_help_balloon (widget balloon, SI x, SI y) {
  (void) balloon;
  (void) x;
  (void) y;
}

void
show_wait_indicator (widget base, string message, string argument) {
  (void) base;
  (void) message;
  (void) argument;
}

void
external_event (string type, time_t t) {
  (void) type;
  (void) t;
}

// Number of open windows (read by get_nr_windows in new_window.cpp).
int nr_windows= 0;

/******************************************************************************
 * Window backend (window.hpp)
 ******************************************************************************/

int
get_identifier (window w) {
  (void) w;
  return 1; // TODO: implement ImGUi identifier
}

window
get_window (int id) {
  (void) id;
  return (window) nullptr;
}

/******************************************************************************
 * Delayed commands
 ******************************************************************************/

// Delayed commands — mirrors Qt's command_queue (qt_gui.cpp). A command may
// return an integer pause (ms); it is then re-queued and re-run after that
// delay. This drives the IME pre-edit debounce (delayed-keyboard-press in
// kbd-handlers.scm): without re-queueing, the pre-edit lambda returns its
// remaining wait time and is dropped, so pre-edit text never renders.
static array<object> g_delayed_q;
static array<time_t> g_delayed_starts;

void
exec_delayed (object cmd) {
  g_delayed_q << cmd;
  // far in the past → eligible to run on the next exec_pending_commands
  g_delayed_starts << (texmacs_time () - 1000000000);
}

void
exec_delayed_pause (object cmd) {
  g_delayed_q << cmd;
  g_delayed_starts << texmacs_time ();
}

void
clear_pending_commands () {
  g_delayed_q     = array<object> (0);
  g_delayed_starts= array<time_t> (0);
}

void
exec_pending_commands () {
  array<object> q = g_delayed_q;
  array<time_t> s = g_delayed_starts;
  g_delayed_q     = array<object> (0);
  g_delayed_starts= array<time_t> (0);
  for (int i= 0; i < N (q); i++) {
    time_t now= texmacs_time ();
    if (now - s[i] >= 0) {
      object obj= call (q[i]);
      if (is_int (obj) && now - s[i] < 1000000000) {
        time_t pause= as_int (obj);
        g_delayed_q << q[i];
        g_delayed_starts << (now + pause);
      }
    }
    else {
      g_delayed_q << q[i];
      g_delayed_starts << s[i];
    }
  }
}
