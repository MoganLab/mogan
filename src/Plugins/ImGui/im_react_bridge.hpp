/******************************************************************************
 * MODULE     : im_react_bridge.hpp
 * DESCRIPTION: WASM-only bridge between the ImGui widget tree and the React
 *              shell (web/). The native ImGui/GLFW desktop app does not use
 *              any of this; the whole API is compiled out unless __EMSCRIPTEN__
 *              is defined.
 *
 *              Responsibilities:
 *                - Serialize the im_menu_rep tree to JSON for the React menu /
 *                  context menu, assigning each leaf command a stable int id.
 *                - Dispatch menu item clicks (by id) through the existing
 *                  deferred command queue (im_queue_menu_command) so the
 *                  no-reentrancy invariant is preserved.
 *                - Push footer state and chrome pixel metrics between JS and
 *                  C++ via EM_JS / EMSCRIPTEN_KEEPALIVE hooks.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_REACT_BRIDGE_HPP
#define IM_REACT_BRIDGE_HPP

#include "command.hpp"
#include "widget.hpp"

#ifdef __EMSCRIPTEN__

/// Serialize a menu widget tree to a JSON array string. Every leaf button is
/// assigned a fresh integer id and registered in the internal command table so
/// mogan_menu_invoke(id) can later dispatch it. The JSON shape mirrors the
/// MenuNode type in web/src/types.ts.
string im_menu_to_json (widget root);

/// Push the serialized menu tree to the React shell (replaces the whole menu).
void im_react_push_menu (widget root);

/// Push the three-column footer text + interactive flag to the React shell.
void im_react_push_footer (string left, string middle, string right,
                           bool interactive);

/// Open the React context menu at the given screen position (px).
void im_react_open_popup (widget menu, float x, float y);

/// Close the React context menu (and the C++ active popup).
void im_react_close_popup ();

/// Return the chrome pixel heights most recently reported by JS (menu, footer).
/// Before JS reports anything, both default to the native ImGui frame height
/// (~22px) so the document canvas is laid out correctly on the first frames.
void im_react_chrome_metrics (int& menu_h, int& footer_h);

#else

// Non-WASM: provide no-op stubs so callers can compile unconditionally.
inline string
im_menu_to_json (widget) {
  return "[]";
}
inline void
im_react_push_menu (widget) {}
inline void
im_react_push_footer (string, string, string, bool) {}
inline void
im_react_open_popup (widget, float, float) {}
inline void
im_react_close_popup () {}
inline void
im_react_chrome_metrics (int&, int&) {}

#endif // __EMSCRIPTEN__

#endif // IM_REACT_BRIDGE_HPP
