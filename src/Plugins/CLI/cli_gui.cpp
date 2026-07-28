
/******************************************************************************
 * MODULE     : cli_gui.cpp
 * DESCRIPTION: 无 UI 前端 —— gui.hpp 契约与 widget 工厂的桩实现
 *
 * 形态仿 src/Plugins/ImGui/im_gui.cpp，但去掉全部 GLFW/OpenGL 调用：不创建
 * 窗口、不渲染上屏。文档渲染经 make_rarshal_image / make_raster_image（mupdf）
 * 离屏完成。事件循环 gui_start_loop 只负责把 -x / exec_delayed 入队的命令跑完，
 * 直到 (quit-TeXmacs) → tm_server_rep::quit() → exit(0) 终止进程。
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "cli_widget.hpp"

#include "font.hpp"
#include "gui.hpp"
#include "object.hpp"
#include "promise.hpp"
#include "scheme.hpp"
#include "tm_timer.hpp"
#include "widget.hpp"

/******************************************************************************
 * Helpers
 ******************************************************************************/

static widget
cli_stub_widget () {
  return widget ((widget_rep*) tm_new<cli_widget_rep> ());
}

// texmacs_interpose_handler() 注册的 interpose 回调（驱动 apply_changes /
// exec_pending_commands / windows_refresh）。Qt/ImGui 在各自事件循环里驱动它；
// CLI 在 gui_start_loop 的循环里手动驱动，否则 env_change 不会被清除。
static void (*g_interpose_fn) (void)= nullptr;

// 延迟命令队列（声明在前，gui_start_loop 引用之；定义见文件末尾）。
static array<object> g_delayed_q;
static array<time_t> g_delayed_starts;

void
gui_interpose (void (*fn) (void)) {
  g_interpose_fn= fn;
}

/******************************************************************************
 * 顶层窗口工厂（占位）
 ******************************************************************************/

widget
texmacs_widget (int mask, command quit) {
  (void) mask;
  (void) quit;
  return cli_stub_widget ();
}

widget
plain_window_widget (widget w, string name, command q) {
  (void) w;
  (void) name;
  (void) q;
  return cli_stub_widget ();
}

widget
popup_window_widget (widget w, string s) {
  (void) w;
  (void) s;
  return cli_stub_widget ();
}

widget
tooltip_window_widget (widget w, string s) {
  (void) w;
  (void) s;
  return cli_stub_widget ();
}

void
destroy_window_widget (widget w) {
  (void) w;
}

void
gui_start_loop () {
  // 仅排空入队命令（-x / exec_delayed），直到队列空；(quit-TeXmacs) 在命令执行
  // 中调用 exit(0) 终结进程。
  //
  // 刻意【不】驱动 interpose 回调（apply_changes / animate / windows_refresh）：
  // CLI 无屏；被动 view 的 editor cvw 为空，apply_changes 会经 get_size 对 null
  // widget 取尺寸而段错误。而渲染路径 render_to_images 自带排版（typeset_preamble
  // / typeset_prepare / ::typeset），完全不依赖 apply_changes。
  for (;;) {
    exec_pending_commands ();
    if (N (g_delayed_q) == 0) break;
  }
}

/******************************************************************************
 * 对话框 / 选择器 / 菜单 / 叶子 / 容器工厂（全部占位）
 *   CLI 不交互，这些 widget 永不显示；返回空 widget 或占位即可满足链接。
 ******************************************************************************/

widget
file_chooser_widget (command cmd, string type, string prompt) {
  (void) cmd;
  (void) type;
  (void) prompt;
  return widget ();
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
  return widget ();
}
widget
inputs_list_widget (command call_back, array<string> prompts) {
  (void) call_back;
  (void) prompts;
  return widget ();
}
widget
popup_widget (widget w) {
  (void) w;
  return cli_stub_widget ();
}

widget
horizontal_menu (array<widget> a) {
  (void) a;
  return widget ();
}
widget
vertical_menu (array<widget> a) {
  (void) a;
  return widget ();
}
widget
tile_menu (array<widget> a, int cols) {
  (void) a;
  (void) cols;
  return widget ();
}
widget
minibar_menu (array<widget> a) {
  (void) a;
  return widget ();
}
widget
menu_separator (bool vertical) {
  (void) vertical;
  return widget ();
}
widget
menu_group (string name, int style) {
  (void) name;
  (void) style;
  return widget ();
}
widget
pulldown_button (widget w, promise<widget> pw) {
  (void) w;
  (void) pw;
  return widget ();
}
widget
pullright_button (widget w, promise<widget> pw) {
  (void) w;
  (void) pw;
  return widget ();
}
widget
menu_button (widget w, command cmd, string pre, string ks, int style) {
  (void) w;
  (void) cmd;
  (void) pre;
  (void) ks;
  (void) style;
  return widget ();
}
widget
balloon_widget (widget w, widget help) {
  (void) help;
  return w;
}

widget
text_widget (string s, int style, color col, bool tsp) {
  (void) s;
  (void) style;
  (void) col;
  (void) tsp;
  return widget ();
}
widget
xpm_widget (url file_name) {
  (void) file_name;
  return widget ();
}
widget
input_text_widget (command call_back, string type, array<string> def, int style,
                   string width) {
  (void) call_back;
  (void) type;
  (void) def;
  (void) style;
  (void) width;
  return widget ();
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
  return widget ();
}
widget
enum_widget (command cb, array<string> vals, string val, int st, string w) {
  (void) cb;
  (void) vals;
  (void) val;
  (void) st;
  (void) w;
  return widget ();
}
widget
choice_widget (command cb, array<string> vals, string val) {
  (void) cb;
  (void) vals;
  (void) val;
  return widget ();
}
widget
choice_widget (command cb, array<string> vals, array<string> mc) {
  (void) cb;
  (void) vals;
  (void) mc;
  return widget ();
}
widget
choice_widget (command cb, array<string> vals, string val, string filt) {
  (void) cb;
  (void) vals;
  (void) val;
  (void) filt;
  return widget ();
}
widget
tree_view_widget (command cmd, tree data, tree data_roles) {
  (void) cmd;
  (void) data;
  (void) data_roles;
  return widget ();
}
widget
tab_page_widget (url u, widget title, widget close_btn, bool is_active) {
  (void) u;
  (void) title;
  (void) close_btn;
  (void) is_active;
  return widget ();
}

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
  return widget ();
}
widget
glue_widget (tree col, bool hx, bool vx, SI w, SI h) {
  (void) col;
  (void) hx;
  (void) vx;
  (void) w;
  (void) h;
  return widget ();
}
widget
horizontal_list (array<widget> a) {
  (void) a;
  return widget ();
}
widget
vertical_list (array<widget> a) {
  (void) a;
  return widget ();
}
widget
division_widget (string name, widget w) {
  (void) name;
  (void) w;
  return widget ();
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
  return widget ();
}
widget
tabs_widget (array<widget> tabs, array<widget> bodies) {
  (void) tabs;
  (void) bodies;
  return widget ();
}
widget
icon_tabs_widget (array<url> us, array<widget> ss, array<widget> bs) {
  (void) us;
  (void) ss;
  (void) bs;
  return widget ();
}
widget
wrapped_widget (widget w, command quit) {
  (void) w;
  (void) quit;
  return widget ();
}
widget
user_canvas_widget (widget wid, int style) {
  (void) wid;
  (void) style;
  return widget ();
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
  return widget ();
}
widget
hsplit_widget (widget l, widget r) {
  (void) l;
  (void) r;
  return widget ();
}
widget
vsplit_widget (widget t, widget b) {
  (void) t;
  (void) b;
  return widget ();
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
  return widget ();
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
  return widget ();
}
widget
refreshable_widget (object prom, string kind) {
  (void) prom;
  (void) kind;
  return widget ();
}

/******************************************************************************
 * gui.hpp 契约
 ******************************************************************************/

void
gui_open (int& argc, char** argv) {
  (void) argc;
  (void) argv;
}
void
gui_close () {}
void
gui_refresh () {}

void
gui_root_extents (SI& width, SI& height) {
  // 无显示器：返回固定值（仿 im_gui 的非 GLFW 回退）。
  width = 1920 * PIXEL;
  height= 1080 * PIXEL;
}
void
gui_maximal_extents (SI& width, SI& height) {
  gui_root_extents (width, height);
}

string
gui_version () {
  return "cli";
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
}

// 剪贴板：仅内存 store（无系统剪贴板）。
static hashmap<string, tree>   cli_sel_t= hashmap<string, tree> (tree (TUPLE));
static hashmap<string, string> cli_sel_s= hashmap<string, string> ("");

bool
set_selection (string cb, tree t, string s, string sv, string sh,
               string format) {
  (void) sv;
  (void) sh;
  (void) format;
  cli_sel_t (cb)= copy (t);
  cli_sel_s (cb)= copy (s);
  return true;
}
bool
get_selection (string cb, tree& t, string& s, string format) {
  (void) format;
  if (cli_sel_t->contains (cb)) {
    t= copy (cli_sel_t[cb]);
    s= copy (cli_sel_s[cb]);
    return true;
  }
  return false;
}
void
clear_selection (string cb) {
  cli_sel_t->reset (cb);
  cli_sel_s->reset (cb);
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

// get_nr_windows (new_window.cpp) 读取的窗口计数。CLI 不开窗，恒为 0。
int nr_windows= 0;

/******************************************************************************
 * Window backend 残留
 ******************************************************************************/

int
get_identifier (window w) {
  (void) w;
  return 0;
}
window
get_window (int id) {
  (void) id;
  return (window) nullptr;
}

/******************************************************************************
 * 延迟命令队列（仿 im_gui.cpp / Qt command_queue）
 ******************************************************************************/

void
exec_delayed (object cmd) {
  g_delayed_q << cmd;
  g_delayed_starts << (texmacs_time () - 1000000000); // 立即可执行
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
