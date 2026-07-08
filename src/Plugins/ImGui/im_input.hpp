
/******************************************************************************
 * MODULE     : im_input.hpp
 * DESCRIPTION: GLFW -> Mogan input translation for the ImGui port.
 *              Mirrors the Qt helpers in qt_utilities.cpp / QTMWidget.cpp:
 *                - from_modifiers        -> im_from_modifiers
 *                - initkeymap + key map  -> im_init_keymap + im_from_key_event
 *                - mouse_state           -> im_mouse_state
 *                - mouse_decode          -> im_mouse_decode
 *              plus im_from_char() for Unicode text input (counterpart to
 *              of Qt's inputMethodEvent commit string).
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef IM_INPUT_HPP
#define IM_INPUT_HPP

#include "string.hpp"
#include "sys_utils.hpp" // time_t

// GLFW modifier bits to Mogan modifier prefix ("S-", "C-", ...).
string im_from_modifiers (int mods);

// GLFW key event to Mogan key event string
string im_from_key_event (int key, int scancode, int action, int mods);

// GLFW Unicode codepoint to Mogan key string
string im_from_char (unsigned int codepoint);

// GLFW mouse event to Mogan mouse-state bitmask
int im_mouse_state (int button, int mods, bool flag);

// GLFW mouse button to Mogan mouse button
string im_mouse_decode (int mstate);

#endif // defined IM_INPUT_HPP
