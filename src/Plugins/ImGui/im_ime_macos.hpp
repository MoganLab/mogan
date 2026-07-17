
/******************************************************************************
 * MODULE     : im_ime_macos.hpp
 * DESCRIPTION: macOS IME bridge for the ImGui (GLFW) frontend.
 *              Swizzles GLFWContentView's NSTextInputClient to capture
 *              pre-edit (marked text) and forward it to the editor, reusing
 *              the WASM bridge's "pre-edit:" queue. Non-macOS gets no-op
 *              stubs so callers need no #ifdef.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_IME_MACOS_HPP
#define IM_IME_MACOS_HPP

struct GLFWwindow;

#ifdef OS_MACOS

// Swizzle GLFWContentView's NSTextInputClient to capture IME pre-edit. Call
// once after the GLFW window is created. Idempotent.
void im_macos_install_ime (GLFWwindow* window);

// True while an IME composition is in progress. The key callback uses this to
// suppress literal key dispatch during composition, so the spacebar that
// commits a candidate does not insert a space before the commit. Backed by the
// swizzle flag (cleared on commit), not GLFW's hasMarkedText.
bool im_macos_ime_composing ();

// Enqueue a pre-edit event ("pre-edit:<utf8>", or "pre-edit:" to clear) into
// the shared drain queue. Defined in im_tm_widget.cpp; no-op in math mode.
void im_macos_enqueue_preedit (const char* utf8);

// Enqueue a plain UTF-8 commit string into the drain queue. The macOS bridge
// routes IME commits through the queue (not the char callback) so they drain
// in order with the pre-edit clear, matching the WASM bridge.
void im_macos_enqueue_commit (const char* utf8);

#else
// No-op stubs on non-macOS so call sites compile without #ifdef.
inline void
im_macos_install_ime (GLFWwindow*) {}
inline bool
im_macos_ime_composing () {
  return false;
}
#endif

#endif // ifdef OS_MACOS
