/******************************************************************************
 * MODULE     : im_react_bridge.cpp
 * DESCRIPTION: WASM-only bridge between the ImGui widget tree and the React
 *              shell (web/). See im_react_bridge.hpp for the full description.
 *              The entire file body is compiled out on non-EMSCRIPTEN builds.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "im_react_bridge.hpp"

#ifdef __EMSCRIPTEN__

#include "array.hpp"
#include "converter.hpp"  // cork_to_utf8
#include "cork.hpp"       // tm_var_encode
#include "dictionary.hpp" // translate, get_input/output_language
#include "im_menu.hpp"    // im_menu_rep, im_activate/deactivate_popup
#include "message.hpp"    // slot, blackbox, SLOT_*
#include "string.hpp"
#include "tm_debug.hpp" // bench_start / bench_end
#include "widget.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <unordered_map>

// Pull ccall()/cwrap() into the Emscripten runtime methods for this TU.
EM_JS_DEPS (mogan_react, "$ccall");

/******************************************************************************
 * JS → C++ entry points (EMSCRIPTEN_KEEPALIVE)
 *
 *   mogan_menu_invoke(id)        — run a registered menu command via the
 *                                  existing deferred queue so we never re-enter
 *                                  the menu tree during traversal.
 *   mogan_menu_close_popup()     — dismiss the active right-click popup.
 *   mogan_set_chrome_metrics(m,f)— record menu/footer pixel heights reported
 *                                  by React so the ImGui canvas is laid out
 *                                  between them.
 ******************************************************************************/

// Command registry rebuilt on every menu serialization. Key = the int id we
// emitted into the JSON; value = the command to run. Commands are reference-
// counted (smart pointer), so holding them here keeps the Scheme lambda alive.
static std::unordered_map<int, command> g_cmd_registry;
static int                              g_next_cmd_id= 1;

// Lazy-submenu registry, same id space and lifecycle as g_cmd_registry (both
// are cleared on every full-menu push, and React always receives a fresh tree
// with freshly-assigned ids). Key = the submenu's id; value = its lazy
// promise, forced only when React actually expands that submenu — this keeps
// menu pushes from eagerly expanding every submenu (the Qt/QTMLazyMenu and
// native ImGui BeginMenu semantics), which is the main update_menus cost.
static std::unordered_map<int, promise<widget>> g_submenu_registry;

// Chrome metrics reported by JS (px). Zero until the React shell measures its
// menu/footer; see im_react_chrome_metrics().
static int g_js_menu_h  = 0;
static int g_js_footer_h= 0;

extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_menu_invoke (int id) {
  auto it= g_cmd_registry.find (id);
  if (it == g_cmd_registry.end ()) return;
  command cmd= it->second;
  // Reuse the existing deferred queue (see im_menu.cpp): the React click lands
  // inside the rAF/interpose window, so enqueue-then-flush-next-frame keeps the
  // no-reentrancy invariant documented at im_menu.cpp:34-39.
  im_queue_menu_command (cmd);
  im_flush_menu_commands ();
  // 兜底：命令已执行，若活动 popup 仍未注销（例如 React 侧某个关闭路径漏调
  // mogan_menu_close_popup），在此清掉，否则编辑器鼠标分发会被
  // im_has_active_popup 永久门控。
  im_deactivate_active_popup ();
}

extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_menu_close_popup () {
  // React dismissed the context menu. Forward to the existing deactivate path
  // so the editor resumes receiving mouse events.
  im_react_close_popup ();
}

extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_set_chrome_metrics (int menu_h, int footer_h) {
  g_js_menu_h  = menu_h;
  g_js_footer_h= footer_h;
}

/******************************************************************************
 * C++ → JS hooks (EM_JS)
 *
 *   im_js_push_menu(json)                       — replace the whole menu tree
 *   im_js_push_footer(left, mid, right, inter)  — footer text update
 *   im_js_open_popup(json, x, y)                — open the context menu
 *   im_js_close_popup()                         — close the context menu
 *
 * Each dispatches to a window.moganOn* registrar installed by
 *web/src/bridge.ts.
 ******************************************************************************/

EM_JS (void, im_js_push_menu, (const char* json), {
  try {
    var tree= JSON.parse (UTF8ToString (json));
    if (typeof window.moganOnMenu === 'function') window.moganOnMenu (tree);
  } catch (e) {
    console.error ('mogan: push_menu failed', e);
  }
});

EM_JS (void, im_js_push_submenu, (int id, const char* json), {
  try {
    var children= JSON.parse (UTF8ToString (json));
    if (typeof window.moganOnSubmenu === 'function')
      window.moganOnSubmenu (id, children);
  } catch (e) {
    console.error ('mogan: push_submenu failed', e);
  }
});

EM_JS (void, im_js_push_footer,
       (const char* left, const char* mid, const char* right, int interactive),
       {
         try {
           var s= {
             left : UTF8ToString (left),
             middle : UTF8ToString (mid),
             right : UTF8ToString (right),
             interactive : interactive ? true : false
           };
           if (typeof window.moganOnFooter === 'function')
             window.moganOnFooter (s);
         } catch (e) {
           console.error ('mogan: push_footer failed', e);
         }
       });

EM_JS (void, im_js_open_popup, (const char* json, double x, double y), {
  try {
    var tree= JSON.parse (UTF8ToString (json));
    if (typeof window.moganOnOpenPopup === 'function')
      window.moganOnOpenPopup (tree, x, y);
  } catch (e) {
    console.error ('mogan: open_popup failed', e);
  }
});

EM_JS (void, im_js_close_popup, (), {
  try {
    if (typeof window.moganOnClosePopup === 'function')
      window.moganOnClosePopup ();
  } catch (e) { /* ignore */
  }
});

/******************************************************************************
 * JSON serialization
 ******************************************************************************/

// Escape a string for inclusion in a JSON string literal. The labels here are
// already UTF-8 (cork_to_utf8 + tm_var_encode via display_label), so we only
// need to escape the JSON-special characters.
static string
json_escape (const string& s) {
  string r;
  r << '"';
  for (int i= 0; i < N (s); ++i) {
    char c= s[i];
    switch (c) {
    case '"':
      r << "\\\"";
      break;
    case '\\':
      r << "\\\\";
      break;
    case '\b':
      r << "\\b";
      break;
    case '\f':
      r << "\\f";
      break;
    case '\n':
      r << "\\n";
      break;
    case '\r':
      r << "\\r";
      break;
    case '\t':
      r << "\\t";
      break;
    default:
      if ((unsigned char) c < 0x20) {
        // control char as \u00XX
        r << "\\u00";
        const char* hex= "0123456789abcdef";
        r << hex[(c >> 4) & 0xf];
        r << hex[c & 0xf];
      }
      else r << c;
    }
  }
  r << '"';
  return r;
}

static string
json_bool (bool b) {
  return b ? string ("true") : string ("false");
}

// Forward recursion: serialize one node, appending to `out`.
static void
serialize_node (widget w, string& out) {
  im_menu_rep* m= dynamic_cast<im_menu_rep*> (w.rep);
  if (m == nullptr) {
    out << "null";
    return;
  }
  switch (m->kind) {
  case im_menu_rep::k_tile: {
    array<widget>& kids= m->menu_children ();
    out << "{\"kind\":\"tile\",\"cols\":";
    out << as_string (m->cols);
    out << ",\"children\":[";
    for (int i= 0; i < N (kids); ++i) {
      if (i > 0) out << ',';
      serialize_node (kids[i], out);
    }
    out << "]}";
  } break;
  case im_menu_rep::k_container: {
    array<widget>& kids= m->menu_children ();
    out << "{\"kind\":\"container\",\"children\":[";
    for (int i= 0; i < N (kids); ++i) {
      if (i > 0) out << ',';
      serialize_node (kids[i], out);
    }
    out << "]}";
  } break;
  case im_menu_rep::k_submenu: {
    out << "{\"kind\":\"submenu\",\"label\":";
    out << json_escape (m->display_label ());
    // Lazy expansion: do NOT force m->sub here. Register the promise under a
    // fresh id and emit only the label; React requests the children via
    // mogan_menu_expand(id) when the user actually opens this submenu. This
    // mirrors Qt QTMLazyMenu / native ImGui BeginMenu (expand-on-open) and is
    // the key to keeping menu pushes cheap.
    if (!is_nil (m->sub)) {
      int id                = g_next_cmd_id++;
      g_submenu_registry[id]= m->sub;
      out << ",\"id\":";
      out << as_string (id);
    }
    out << "}";
  } break;
  case im_menu_rep::k_button: {
    int  id     = g_next_cmd_id++;
    bool checked= m->pre != "" || (m->style & WIDGET_STYLE_PRESSED);
    bool enabled= (m->style & WIDGET_STYLE_INERT) == 0;
    if (!is_nil (m->cmd)) g_cmd_registry[id]= m->cmd;
    out << "{\"kind\":\"button\",\"id\":";
    out << as_string (id);
    out << ",\"label\":";
    out << json_escape (m->display_label ());
    if (N (m->ks) > 0) {
      out << ",\"shortcut\":";
      out << json_escape (m->ks);
    }
    out << ",\"checked\":";
    out << json_bool (checked);
    out << ",\"enabled\":";
    out << json_bool (enabled);
    out << "}";
  } break;
  case im_menu_rep::k_separator: {
    out << "{\"kind\":\"separator\"}";
  } break;
  case im_menu_rep::k_group: {
    out << "{\"kind\":\"group\",\"label\":";
    out << json_escape (m->display_label ());
    out << "}";
  } break;
  case im_menu_rep::k_text: {
    out << "{\"kind\":\"text\",\"label\":";
    out << json_escape (m->display_label ());
    out << "}";
  } break;
  }
}

string
im_menu_to_json (widget root) {
  // Wrap as a top-level container array so React always gets a list of nodes.
  string       out= "[";
  im_menu_rep* m  = dynamic_cast<im_menu_rep*> (root.rep);
  if (m != nullptr) {
    array<widget>& kids= m->menu_children ();
    for (int i= 0; i < N (kids); ++i) {
      if (i > 0) out << ',';
      serialize_node (kids[i], out);
    }
  }
  out << "]";
  return out;
}

extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_menu_expand (int id) {
  // React opened a lazy submenu: force its promise and push the freshly
  // expanded children back. No-op if the id is stale (menu rebuilt since).
  // Defined after both im_js_push_submenu (EM_JS) and im_menu_to_json so the
  // EM_JS symbol resolves at link time without a forward declaration.
  auto it= g_submenu_registry.find (id);
  if (it == g_submenu_registry.end ()) return;
  promise<widget> sub= it->second;
  if (is_nil (sub)) return;
  widget   w   = sub ();
  string   json= im_menu_to_json (w);
  c_string cs (json);
  im_js_push_submenu (id, (const char*) cs);
}

/******************************************************************************
 * Public push hooks
 ******************************************************************************/

void
im_react_push_menu (widget root) {
  // Each rebuild invalidates prior ids; clear the registry so stale ids from a
  // previous menu tree can never be invoked. (React always gets a fresh tree.)
  g_cmd_registry.clear ();
  g_submenu_registry.clear ();
  g_next_cmd_id= 1;
  bench_start ("menu_serialize_push");
  string   json= im_menu_to_json (root);
  c_string cs (json);
  im_js_push_menu ((const char*) cs);
  bench_end ("menu_serialize_push");
}

void
im_react_push_footer (string left, string middle, string right,
                      bool interactive) {
  c_string cl (left), cm (middle), cr (right);
  im_js_push_footer ((const char*) cl, (const char*) cm, (const char*) cr,
                     interactive ? 1 : 0);
}

void
im_react_open_popup (widget menu, float x, float y) {
  // Context menus don't share the main-menu registry: build a fresh id space
  // scoped to this popup. Clearing here is safe because the main menu is only
  // rebuilt via im_react_push_menu (which re-clears), and the popup is modal —
  // no main-menu click can race while it's open.
  string   json= im_menu_to_json (menu);
  c_string cs (json);
  im_js_open_popup ((const char*) cs, (double) x, (double) y);
}

void
im_react_close_popup () {
  // 先清掉 C++ 侧活动 popup，使编辑器恢复鼠标分发（native 由 BeginPopup
  // 返回 false 隐式完成；WASM 不走那条路径，故显式清）。
  im_deactivate_active_popup ();
  im_js_close_popup ();
}

void
im_react_chrome_metrics (int& menu_h, int& footer_h) {
  // Fallback to the ImGui frame height (~22px) before JS reports anything, so
  // the document canvas is laid out correctly on the very first frames.
  menu_h  = g_js_menu_h > 0 ? g_js_menu_h : 22;
  footer_h= g_js_footer_h > 0 ? g_js_footer_h : 22;
}

#endif // __EMSCRIPTEN__
