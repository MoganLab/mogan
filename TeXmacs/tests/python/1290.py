#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated UI test and screenshot utility for issue 1290:
Verify modern blockquote style (<quote-env>) with left decorative bar in Mogan STEM.

Usage:
  python3 1290.py [path/to/doc.tmu] [output_screenshot.png]
"""

import os
import sys
import time
import tempfile
import subprocess
from PIL import ImageGrab

# Fallback: import pynput from ~/git/pynput/lib if not installed system-wide
try:
    import pynput
except ImportError:
    pynput_path = os.path.expanduser("~/git/pynput/lib")
    if os.path.exists(pynput_path):
        sys.path.insert(0, pynput_path)
    import pynput

from pynput.keyboard import Key, Controller as KeyboardController
from pynput.mouse import Button, Controller as MouseController

IS_WINDOWS = sys.platform == "win32"
IS_DARWIN = sys.platform == "darwin"


def find_repo_root():
    cur = os.path.abspath(os.path.dirname(__file__))
    while cur != "/" and cur != os.path.dirname(cur):
        if os.path.exists(os.path.join(cur, "TeXmacs")) and os.path.exists(os.path.join(cur, "src")):
            return cur
        cur = os.path.dirname(cur)
    return os.path.abspath(".")


def find_mogan_binary(repo_root):
    candidates = [
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def get_mogan_window_rect():
    """Returns (x, y, w, h) of Mogan window, or default full screen."""
    if not IS_DARWIN and not IS_WINDOWS:
        try:
            import Xlib.display
            d = Xlib.display.Display()
            root = d.screen().root

            def find_win(win):
                try:
                    cls = win.get_wm_class()
                    if cls and "mogan" in cls[0].lower():
                        return win
                    name = win.get_wm_name()
                    if name and ("Mogan" in name or "STEM" in name):
                        return win
                    for child in win.query_tree().children:
                        res = find_win(child)
                        if res:
                            return res
                except Exception:
                    pass
                return None

            w = find_win(root)
            if w:
                geom = w.get_geometry()
                coords = w.translate_coords(root, 0, 0)
                return (coords.x, coords.y, geom.width, geom.height)
        except Exception:
            pass

    img = ImageGrab.grab()
    return (0, 0, img.size[0], img.size[1])


def focus_mogan_window():
    """Ensure Mogan window is raised and focused across platforms."""
    if IS_DARWIN:
        try:
            import AppKit
            for app in AppKit.NSWorkspace.sharedWorkspace().runningApplications():
                if "Mogan" in (app.localizedName() or ""):
                    app.activateWithOptions_(AppKit.NSApplicationActivateIgnoringOtherApps)
                    return
        except Exception:
            pass
        subprocess.run(
            ["osascript", "-e", 'tell application "System Events" to set frontmost of first process whose name contains "Mogan" to true'],
            capture_output=True,
        )
        return

    if IS_WINDOWS:
        try:
            import ctypes
            from ctypes import wintypes
            user32 = ctypes.windll.user32

            def callback(hwnd, _lparam):
                if user32.IsWindowVisible(hwnd):
                    n = user32.GetWindowTextLengthW(hwnd)
                    buf = ctypes.create_unicode_buffer(n + 1)
                    user32.GetWindowTextW(hwnd, buf, n + 1)
                    if "STEM" in buf.value or "Mogan" in buf.value:
                        user32.ShowWindow(hwnd, 9)
                        user32.SetForegroundWindow(hwnd)
                        return False
                return True

            EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
            user32.EnumWindows(EnumWindowsProc(callback), 0)
        except Exception:
            pass
        return

    # Linux (X11)
    try:
        import Xlib.display
        import Xlib.X
        import Xlib.protocol.event

        d = Xlib.display.Display()
        root = d.screen().root

        def find_mogan(win):
            try:
                cls = win.get_wm_class()
                if cls and "mogan" in cls[0].lower():
                    return win
                name = win.get_wm_name()
                if name and ("Mogan" in name or "STEM" in name):
                    return win
                for child in win.query_tree().children:
                    res = find_mogan(child)
                    if res:
                        return res
            except Exception:
                pass
            return None

        w = find_mogan(root)
        if w:
            net_active = d.intern_atom("_NET_ACTIVE_WINDOW")
            cm = Xlib.protocol.event.ClientMessage(
                window=w,
                client_type=net_active,
                data=(32, [2, Xlib.X.CurrentTime, 0, 0, 0]),
            )
            root.send_event(
                cm,
                event_mask=Xlib.X.SubstructureRedirectMask | Xlib.X.SubstructureNotifyMask,
            )
            w.set_input_focus(Xlib.X.RevertToParent, Xlib.X.CurrentTime)
            w.configure(stack_mode=Xlib.X.Above)
            d.sync()
    except Exception:
        pass


def capture_quote_env(doc_path=None, output_path=None):
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)

    if doc_path is None:
        default_candidates = [
            os.path.join(repo_root, "TeXmacs", "tests", "tmu", "1290.tmu"),
            os.path.join(repo_root, "1290.tmu"),
        ]
        for c in default_candidates:
            if os.path.exists(c):
                doc_path = c
                break
        if doc_path is None:
            doc_path = default_candidates[0]

    if output_path is None:
        output_path = os.path.join(tempfile.gettempdir(), "1290.png")

    print(f"[1290] Target document: {doc_path}")
    print(f"[1290] Output screenshot: {output_path}")
    print(f"[1290] Using Mogan binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1290] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, doc_path], env=env, cwd=repo_root)

    try:
        time.sleep(3.5)
        ret = proc.poll()
        if ret is not None:
            print(f"[1290] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[1290] Step 2: Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(0.5)

        wx, wy, ww, wh = get_mogan_window_rect()
        print(f"[1290] Mogan window bounds: ({wx}, {wy}, {ww}, {wh})")

        mouse = MouseController()
        kb = KeyboardController()

        click_x = wx + ww // 2
        click_y = wy + wh // 2
        print(f"[1290] Step 3: Using pynput to click document area at ({click_x}, {click_y})...")
        mouse.position = (click_x, click_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.0)

        print("[1290] Step 4: Capturing screenshot...")
        if ww > 100 and wh > 100:
            bbox = (wx, wy, wx + ww, wy + wh)
            screenshot = ImageGrab.grab(bbox=bbox)
        else:
            screenshot = ImageGrab.grab()

        screenshot.save(output_path)
        print(f"[1290] Screenshot successfully saved to: {output_path}")

        print("[1290] Step 5: Using pynput to close Mogan (Ctrl+Q)...")
        modifier = Key.cmd if IS_DARWIN else Key.ctrl
        with kb.pressed(modifier):
            kb.press("q")
            kb.release("q")
        time.sleep(1.0)

        ret = proc.poll()
        if ret is None:
            proc.terminate()
            try:
                proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                proc.kill()

        print("[1290] Verification completed successfully.")
        return 0
    finally:
        if proc.poll() is None:
            proc.kill()


if __name__ == "__main__":
    doc = None
    out = None
    for arg in sys.argv[1:]:
        if arg.endswith(".tmu") or arg.endswith(".tm"):
            doc = arg
        elif arg.endswith(".png"):
            out = arg
        elif doc is None:
            doc = arg
        else:
            out = arg
    sys.exit(capture_quote_env(doc, out))
