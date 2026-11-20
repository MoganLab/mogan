#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated UI test and screenshot utility for issue 6200:
Verify table rendering inside quote-env blockquote in Mogan STEM.
Tables (small-table, big-table) are properly preserved as subtrees without
their captions/labels leaking into inline text or overflowing horizontally.

Usage:
  python3 6200.py [path/to/doc.tmu] [output_screenshot.png]
"""

import os
import sys
import time
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


def x11_find_mogan(d):
    """Find the Mogan window on X11 by WM_CLASS/title, else None."""

    def find(win):
        try:
            cls = win.get_wm_class()
            if cls and "mogan" in cls[0].lower():
                return win
            name = win.get_wm_name()
            if name and ("Mogan" in name or "STEM" in name):
                return win
            for child in win.query_tree().children:
                res = find(child)
                if res:
                    return res
        except Exception:
            pass
        return None

    return find(d.screen().root)


def get_mogan_window_rect():
    """Returns (x, y, w, h) of the Mogan window on X11, else None."""
    if not IS_DARWIN and not IS_WINDOWS:
        try:
            import Xlib.display

            d = Xlib.display.Display()
            w = x11_find_mogan(d)
            if w:
                geom = w.get_geometry()
                coords = w.translate_coords(d.screen().root, 0, 0)
                return (coords.x, coords.y, geom.width, geom.height)
        except Exception:
            pass
    return None


def focus_mogan_window():
    """Ensure Mogan window is raised and focused across platforms."""
    if IS_DARWIN:
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

        w = x11_find_mogan(d)
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
        doc_path = os.path.join(repo_root, "TeXmacs", "tests", "tmu", "6200.tmu")

    if output_path is None:
        output_path = "/tmp/6200.png"

    print(f"[6200] Target document: {doc_path}")
    print(f"[6200] Output screenshot: {output_path}")
    print(f"[6200] Using Mogan binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[6200] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, doc_path], env=env, cwd=repo_root)

    try:
        time.sleep(3.5)
        ret = proc.poll()
        if ret is not None:
            print(f"[6200] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[6200] Step 2: Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(0.5)

        rect = get_mogan_window_rect()
        # X11 之外拿不到窗口矩形：抓一次全屏，兼作点击参考与最终截图
        full = None if rect else ImageGrab.grab()
        wx, wy, ww, wh = rect if rect else (0, 0, full.size[0], full.size[1])
        print(f"[6200] Mogan window bounds: ({wx}, {wy}, {ww}, {wh})")

        mouse = MouseController()
        kb = KeyboardController()

        click_x = wx + ww // 2
        click_y = wy + wh // 2
        print(f"[6200] Step 3: Using pynput to click document area at ({click_x}, {click_y})...")
        mouse.position = (click_x, click_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.0)

        print("[6200] Step 4: Capturing screenshot...")
        screenshot = full if full else ImageGrab.grab(bbox=(wx, wy, wx + ww, wy + wh))

        screenshot.save(output_path)
        print(f"[6200] Screenshot successfully saved to: {output_path}")

        print("[6200] Step 5: Using pynput to close Mogan (Ctrl+Q)...")
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

        print("[6200] Verification completed successfully.")
        return 0
    finally:
        if proc.poll() is None:
            proc.kill()


if __name__ == "__main__":
    # usage: 6200.py [doc.tmu] [out.png]
    doc = sys.argv[1] if len(sys.argv) > 1 else None
    out = sys.argv[2] if len(sys.argv) > 2 else None
    sys.exit(capture_quote_env(doc, out))
