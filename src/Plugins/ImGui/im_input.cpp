
/******************************************************************************
 * MODULE     : im_input.cpp
 * DESCRIPTION: GLFW -> Mogan input translation for the ImGui port.
 *              See im_input.hpp for more info.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "im_input.hpp"

#include "analyze.hpp" // locase, os_macos
#include "hashmap.hpp"
#include "tm_debug.hpp"

#include <GLFW/glfw3.h>

/******************************************************************************
 * Note:
 * Mogan's mouse/button modifier bitmask (must match edit_interface.hpp):
 *  ShiftMask=256
 *  ControlMask=1024
 *  Mod1Mask(Alt)=2048
 *  Mod2Mask(Meta)=4096
 *  left=1  middle=2  right=4
 *  XButton1=8  XButton2=16
 *******************************************************************************
 * Note:
 *                |                 |
 * Cursor         |  Canvas         |  Scroll
 * y              |  -------->x     |  x<--------|TL
 * |              |  |TL            |            |
 * |              |  |              |            |
 * |------->x     |  |              |            |
 *  TL            |  y              |            y
 *
 ******************************************************************************/

/******************************************************************************
 * Key map (GLFW special keys -> Mogan key names)
 ******************************************************************************/

static hashmap<int, string> im_keymap ("");

static inline void
im_map_key (int code, const string& name) {
  im_keymap (code)= name;
}

static void
im_init_keymap () {
  static bool fInit= false;
  if (fInit) return;
  fInit= true;

  im_map_key (GLFW_KEY_SPACE, "space");
  im_map_key (GLFW_KEY_TAB, "tab");
  im_map_key (GLFW_KEY_ENTER, "return");
  im_map_key (GLFW_KEY_KP_ENTER, "return");
  im_map_key (GLFW_KEY_ESCAPE, "escape");
  im_map_key (GLFW_KEY_BACKSPACE, "backspace");
  im_map_key (GLFW_KEY_INSERT, "insert");
  im_map_key (GLFW_KEY_DELETE, "delete");
  im_map_key (GLFW_KEY_HOME, "home");
  im_map_key (GLFW_KEY_END, "end");
  im_map_key (GLFW_KEY_PAGE_UP, "pageup");
  im_map_key (GLFW_KEY_PAGE_DOWN, "pagedown");
  im_map_key (GLFW_KEY_SCROLL_LOCK, "scrolllock");
  im_map_key (GLFW_KEY_PAUSE, "pause");
  im_map_key (GLFW_KEY_PRINT_SCREEN, "print");
  im_map_key (GLFW_KEY_MENU, "menu");

  im_map_key (GLFW_KEY_LEFT, "left");
  im_map_key (GLFW_KEY_RIGHT, "right");
  im_map_key (GLFW_KEY_UP, "up");
  im_map_key (GLFW_KEY_DOWN, "down");

  im_map_key (GLFW_KEY_F1, "F1");
  im_map_key (GLFW_KEY_F2, "F2");
  im_map_key (GLFW_KEY_F3, "F3");
  im_map_key (GLFW_KEY_F4, "F4");
  im_map_key (GLFW_KEY_F5, "F5");
  im_map_key (GLFW_KEY_F6, "F6");
  im_map_key (GLFW_KEY_F7, "F7");
  im_map_key (GLFW_KEY_F8, "F8");
  im_map_key (GLFW_KEY_F9, "F9");
  im_map_key (GLFW_KEY_F10, "F10");
  im_map_key (GLFW_KEY_F11, "F11");
  im_map_key (GLFW_KEY_F12, "F12");
  im_map_key (GLFW_KEY_F13, "F13");
  im_map_key (GLFW_KEY_F14, "F14");
  im_map_key (GLFW_KEY_F15, "F15");
  im_map_key (GLFW_KEY_F16, "F16");
  im_map_key (GLFW_KEY_F17, "F17");
  im_map_key (GLFW_KEY_F18, "F18");
  im_map_key (GLFW_KEY_F19, "F19");
  im_map_key (GLFW_KEY_F20, "F20");
  im_map_key (GLFW_KEY_F21, "F21");
  im_map_key (GLFW_KEY_F22, "F22");
  im_map_key (GLFW_KEY_F23, "F23");
  im_map_key (GLFW_KEY_F24, "F24");
  im_map_key (GLFW_KEY_F25, "F25");
}

/******************************************************************************
 * Modifier translation
 ******************************************************************************/

string
im_from_modifiers (int mods) {
  // GLFW 报告的是物理按键：GLFW_MOD_CONTROL 在所有平台上都表示 Ctrl 键，
  // 而 GLFW_MOD_SUPER 在 macOS 上表示 Cmd 键，在其他平台上表示 Super（Win）键。
  // Qt 会在 macOS 上交换 Control 和 Meta
  //   macOS:  Ctrl -> "C-", Cmd -> "M-"
  //   other:  Ctrl -> "C-", Win -> "M-"
  string r= "";
  if (mods & GLFW_MOD_SHIFT) r= "S-" * r;
  if (mods & GLFW_MOD_ALT) r= "A-" * r;
  if (mods & GLFW_MOD_CONTROL) r= "C-" * r;
  if (mods & GLFW_MOD_SUPER) r= "M-" * r;
  return r;
}

/******************************************************************************
 * Key event translation
 ******************************************************************************/

// 对于主要含义是可打印字符、并通过 char 回调传递的 GLFW 按键，返回 true。
// 普通可打印字符输入由该回调处理（从而获得真实的 Unicode 码点，包括
// Shift/Caps Lock 修饰后的字符）；这里仅处理特殊按键以及 Ctrl/Super 快捷键。
static inline bool
im_is_printable_key (int key) {
  // GLFW 以 ASCII 值枚举的 A–Z, 0–9, etc.
  return (key >= 32 && key <= 93) || key == GLFW_KEY_GRAVE_ACCENT;
}

// US 布局下可打印键 shift 后的字形（数字行与常见符号）；字母键由调用方处理
// （GLFW 已给出大写 ASCII）。返回 0 表示无标准 shift 字形。仅用于 Alt+Shift
// 快捷键还原字形：macOS 浏览器会把 Alt+Shift+7 合字为 ‡，char 回调拿不到
// shift 后字形，只能据物理键码 + US 布局还原。
static char
im_shift_glyph (int key) {
  switch (key) {
  case '1':
    return '!';
  case '2':
    return '@';
  case '3':
    return '#';
  case '4':
    return '$';
  case '5':
    return '%';
  case '6':
    return '^';
  case '7':
    return '&';
  case '8':
    return '*';
  case '9':
    return '(';
  case '0':
    return ')';
  case '-':
    return '_';
  case '=':
    return '+';
  case '[':
    return '{';
  case ']':
    return '}';
  case ';':
    return ':';
  case '\'':
    return '"';
  case '`':
    return '~';
  case '\\':
    return '|';
  case ',':
    return '<';
  case '.':
    return '>';
  case '/':
    return '?';
  default:
    return 0;
  }
}

string
im_from_key_event (int key, int scancode, int action, int mods) {
  (void) scancode;
  // Ignore pure modifier-key presses
  if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT ||
      key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL ||
      key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT ||
      key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER ||
      key == GLFW_KEY_CAPS_LOCK || key == GLFW_KEY_NUM_LOCK ||
      key == GLFW_KEY_SCROLL_LOCK)
    return "";

  if (action == GLFW_RELEASE) {
    return "";
  }

  string mods_text= im_from_modifiers (mods);
  im_init_keymap ();

  // Ctrl/Super + letter -> shortcut string. Match Qt's from_key_press_event:
  // with Shift held, drop "S-" and use the uppercase letter so Cmd+Shift+Z ->
  // "M-Z" (bound to redo), Cmd+Shift+S -> "M-S" (save-as); without Shift, use
  // the lowercase letter (Cmd+Z -> "M-z", undo).
  const bool shortcut= (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)) != 0;
  if (shortcut && key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
    char c= (char) key;
    if (mods & GLFW_MOD_SHIFT)
      return im_from_modifiers (mods & ~GLFW_MOD_SHIFT) * string (c);
    return mods_text * string (locase (c));
  }

  // Special keys (arrows, F1..F25, return, ...) honour the modifier prefix.
  if (im_keymap->contains (key)) {
    string name= im_keymap[key];
    // For named keys, drop "S-" (shift) only for dead-key style mappings; here
    // we keep all modifiers, matching Qt's from_key_press_event Case 1.
    return mods_text * name;
  }

  // 普通可打印按键且未按下 Ctrl/Super, 交由 char 回调以 Unicode 文本形式处理。
  if (im_is_printable_key (key) && (mods_text == "" || mods_text == "S-"))
    return "";

  // Ctrl/Cmd + 可打印非字母键：发出快捷键串，对齐 Qt 的 from_key_press_event
  // （字母键已在上面单独处理）。GLFW 的 key 码即未 shift 的 ASCII，shift 后的字
  // 形（如 +）由 char 回调负责，故对 "X-S-" 丢弃 S-、按未 shift 码点拼接。
  // 修复 Cmd+= / Cmd+- (zoom) 等修饰键 + 非字母键被静默丢弃的问题。仅限
  // Ctrl/Cmd：Alt 走 char 回调做 Option 合字输入，不在此处理。
  if (shortcut && im_is_printable_key (key))
    return ((mods & GLFW_MOD_SHIFT) ? im_from_modifiers (mods & ~GLFW_MOD_SHIFT)
                                    : mods_text) *
           string (locase ((char) key));

  // Alt(Option) + Shift + 可打印键：发出 "A-" + shift 字形，对齐 Qt 的 "A-S-"
  // 分支。macos look-and-feel 下 "text &"->"A-&"、"text $"->"A-$"，但 macOS
  // 浏览 器会把 Alt+Shift+7 合字为 ‡（char 回调拿不到 '&'），而 GLFW key
  // 码又是未 shift 的 '7'，故按 US 布局 shift 表还原字形（im_shift_glyph）。仅
  // Alt+Shift： plain Alt（无 Shift）仍走 char 回调做 Option 合字（Alt+e
  // 死键、Alt+符号合 字），不在此处理，避免破坏重音/合字输入。
  if ((mods & GLFW_MOD_ALT) && (mods & GLFW_MOD_SHIFT) &&
      im_is_printable_key (key)) {
    char c= (char) key;
    char g= im_shift_glyph (c);
    return "A-" * string (g != 0 ? g : c);
  }

  if (DEBUG_KEYBOARD)
    debug_keyboard << "im_from_key_event: unmapped key " << key << LF;
  return "";
}

string
im_from_char (unsigned int codepoint) {
  // Encode the codepoint as UTF-8 into Mogan's byte-string.
  string r= "";
  if (codepoint < 0x80) {
    r << (char) codepoint;
  }
  else if (codepoint < 0x800) {
    r << (char) (0xC0 | (codepoint >> 6));
    r << (char) (0x80 | (codepoint & 0x3F));
  }
  else if (codepoint < 0x10000) {
    r << (char) (0xE0 | (codepoint >> 12));
    r << (char) (0x80 | ((codepoint >> 6) & 0x3F));
    r << (char) (0x80 | (codepoint & 0x3F));
  }
  else {
    r << (char) (0xF0 | (codepoint >> 18));
    r << (char) (0x80 | ((codepoint >> 12) & 0x3F));
    r << (char) (0x80 | ((codepoint >> 6) & 0x3F));
    r << (char) (0x80 | (codepoint & 0x3F));
  }
  return r;
}

/******************************************************************************
 * Mouse state translation
 ******************************************************************************/

int
im_mouse_state (int button, int mods, bool flag) {
  (void) flag;
  // Note: GLFW delivers only the changed button
  int i= 0;
  switch (button) {
  case GLFW_MOUSE_BUTTON_LEFT:
    i+= 1;
    break;
  case GLFW_MOUSE_BUTTON_MIDDLE:
    i+= 2;
    break;
  case GLFW_MOUSE_BUTTON_RIGHT:
    i+= 4;
    break;
  case GLFW_MOUSE_BUTTON_4:
    i+= 8;
    break;
  case GLFW_MOUSE_BUTTON_5:
    i+= 16;
    break;
  default:
    break;
  }
  if (mods & GLFW_MOD_SHIFT) i+= 256;
#ifdef OS_MACOS
  // 与 Qt 在 macOS 上的映射保持一致
  if (mods & GLFW_MOD_CONTROL) i= 1024 + 4;
  if (mods & GLFW_MOD_ALT) i= 2048 + 2;
  if (mods & GLFW_MOD_SUPER) i+= 4096;
#else
  if (mods & GLFW_MOD_CONTROL) i+= 1024;
  if (mods & GLFW_MOD_ALT) i+= 2048;
  if (mods & GLFW_MOD_SUPER) i+= 4096;
#endif
  return i;
}

string
im_mouse_decode (int mstate) {
  // 最后检查左键：在 macOS 上，Ctrl/Option 会分别模拟右键/中键，但物理左键
  // 的状态位也可能同时被按下，优先采用模拟按键（与 Qt 的行为一致）。
  if (mstate & 2) return "middle";
  if (mstate & 4) return "right";
  if (mstate & 1) return "left";
  if (mstate & 8) return "up";
  if (mstate & 16) return "down";
  return "unknown";
}
