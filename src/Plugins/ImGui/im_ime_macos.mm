
/******************************************************************************
 * MODULE     : im_ime_macos.mm
 * DESCRIPTION: macOS IME bridge for the ImGui (GLFW) frontend (impl).
 *              Swizzles GLFWContentView's NSTextInputClient to capture IME
 *              pre-edit and commit, forwarding them through the queue the WASM
 *              bridge uses. Details inline; see im_ime_macos.hpp for the API.
 * AUTHOR     : JimZhouZZY
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "im_ime_macos.hpp"

#ifdef OS_MACOS

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h> // glfwGetCocoaView

#import <AppKit/AppKit.h>
#include <objc/runtime.h>

// Debug-only IME trace (release builds compile this out). In a debug build the
// NSLog lines surface in the terminal / Console.app, confirming whether the
// swizzle installed and what marked/insert text the IME actually delivers.
#ifdef LIII_DEBUG
#define MOGAN_IME_LOG(...) NSLog (__VA_ARGS__)
#else
#define MOGAN_IME_LOG(...)                                                     \
  do {                                                                         \
  } while (0)
#endif

// Original GLFW implementations, saved before swizzling so the swizzled
// versions can call through.
static IMP g_orig_setMarkedText= nil;
static IMP g_orig_unmarkText   = nil;
static IMP g_orig_insertText   = nil;

// Whether an IME composition is in progress (true between a non-empty
// setMarkedText and the next insertText/unmarkText/empty-setMarkedText).
// Drives the key-callback suppression; cleared reliably on commit, unlike
// GLFW's own hasMarkedText which can stay true and trap keys.
static bool g_ime_composing= false;

// setMarkedText/insertText deliver either NSString or NSAttributedString; pull
// a plain UTF-8 C string from either (NSAttributedString is not an NSString and
// does not respond to UTF8String).
static const char*
mogan_utf8 (id string) {
  if (string == nil) return "";
  if ([string isKindOfClass:[NSString class]])
    return [(NSString*) string UTF8String];
  if ([string isKindOfClass:[NSAttributedString class]])
    return [[(NSAttributedString*) string string] UTF8String];
  return "";
}

/******************************************************************************
 * Swizzled NSTextInputClient implementations
 ******************************************************************************/

// -[GLFWContentView setMarkedText:selectedRange:replacementRange:]
static void
mogan_setMarkedText (id self, SEL _cmd, id string, NSRange selectedRange,
                     NSRange replacementRange) {
  typedef void (*Orig) (id, SEL, id, NSRange, NSRange);
  ((Orig) g_orig_setMarkedText) (self, _cmd, string, selectedRange,
                                 replacementRange);
  // Track composition state before extracting text, and forward the marked
  // text (pre-edit); an empty string clears it.
  g_ime_composing= (string != nil && [string length] > 0);
  MOGAN_IME_LOG (@"im_ime setMarkedText: '%@' composing=%d", string,
                 g_ime_composing);
  im_macos_enqueue_preedit (mogan_utf8 (string));
}

// -[GLFWContentView unmarkText]
static void
mogan_unmarkText (id self, SEL _cmd) {
  typedef void (*Orig) (id, SEL);
  ((Orig) g_orig_unmarkText) (self, _cmd);
  MOGAN_IME_LOG (@"im_ime unmarkText composing=%d", g_ime_composing);
  if (g_ime_composing) {
    g_ime_composing= false;
    im_macos_enqueue_preedit (""); // clear pre-edit
  }
}

// -[GLFWContentView insertText:replacementRange:]
static void
mogan_insertText (id self, SEL _cmd, id string, NSRange replacementRange) {
  if (g_ime_composing) {
    // IME commit: clear pre-edit then commit, both queued so im_main_loop
    // drains them in order. Calling the original would fire the char callback
    // now and insert the commit before the (queued) pre-edit clear.
    MOGAN_IME_LOG (@"im_ime insertText(commit): '%@'", string);
    g_ime_composing= false;
    im_macos_enqueue_preedit ("");
    im_macos_enqueue_commit (mogan_utf8 (string));
    return; // skip original -> no GLFW char callback for this commit
  }
  // Regular keystroke: let GLFW fire the char callback as usual.
  typedef void (*Orig) (id, SEL, id, NSRange);
  ((Orig) g_orig_insertText) (self, _cmd, string, replacementRange);
}

/******************************************************************************
 * Public interface
 ******************************************************************************/

void
im_macos_install_ime (GLFWwindow* window) {
  static bool installed= false;
  if (installed) return;
  NSView* view= glfwGetCocoaView (window);
  if (view == nil) return; // view not ready yet; retry on the next call
  installed= true;
  Class cls= object_getClass (view);
  // nm libglfw3.a confirms these three methods are defined on GLFWContentView
  // itself, so method_setImplementation suffices (class_getInstanceMethod
  // returns the class's own method, not an inherited one).
  Method m;
  m= class_getInstanceMethod (cls, @selector (setMarkedText:
                                              selectedRange:replacementRange:));
  if (m != nil)
    g_orig_setMarkedText=
        method_setImplementation (m, (IMP) mogan_setMarkedText);
  m= class_getInstanceMethod (cls, @selector (unmarkText));
  if (m != nil)
    g_orig_unmarkText= method_setImplementation (m, (IMP) mogan_unmarkText);
  m= class_getInstanceMethod (cls, @selector (insertText:replacementRange:));
  if (m != nil)
    g_orig_insertText= method_setImplementation (m, (IMP) mogan_insertText);
  MOGAN_IME_LOG (@"im_ime install: cls=%@ setMarked=%d unmark=%d insert=%d",
                 cls, g_orig_setMarkedText != nil, g_orig_unmarkText != nil,
                 g_orig_insertText != nil);
}

bool
im_macos_ime_composing () {
  // Use our own flag (cleared on commit) rather than GLFW's hasMarkedText,
  // which can stay true after a commit and trap subsequent keys.
  return g_ime_composing;
}

#endif // ifdef OS_MACOS
