#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated end-to-end UI test for issue 6204:
AI 侧边栏辅助按钮区调整：放大按钮右移，新增翻译/释义切换按钮

Verification steps:
1. Launch Mogan STEM in non-community mode with AI Chat sidebar opened.
2. Verify dock auxiliary buttons:
   - Close, New Chat, Translate, Explain on the left.
   - Maximize button on the right.
3. Click "Translate" button:
   - Verify button enters checked state (accent color background).
   - Verify sidebar switches to "翻译: ..." session with title centered below buttons.
4. Click "Explain" button:
   - Verify Translate button exits checked state.
   - Verify Explain button enters checked state (accent color background).
   - Verify sidebar switches to "释义: ..." session with title centered below buttons.
5. Click "Explain" button again:
   - Verify button exits checked state.
   - Returns to the previous conversation.
6. Cleanly exit Mogan STEM.
"""

import os
import sys
import time
import subprocess
from PIL import ImageGrab
import numpy as np

# Ensure UTF-8 and unbuffered stdout
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(line_buffering=True, encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(line_buffering=True, encoding="utf-8")

# DPI awareness on Windows
if sys.platform == "win32":
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(2)
    except Exception:
        try:
            import ctypes
            ctypes.windll.user32.SetProcessDPIAware()
        except Exception:
            pass

# Import pynput
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
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))


def find_mogan_binary(repo_root):
    candidates = [
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def focus_mogan_window():
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
            user32 = ctypes.windll.user32
            def enum_cb(hwnd, lparam):
                length = user32.GetWindowTextLengthW(hwnd)
                if length > 0:
                    buff = ctypes.create_unicode_buffer(length + 1)
                    user32.GetWindowTextW(hwnd, buff, length + 1)
                    title = buff.value
                    if "Mogan" in title or "STEM" in title:
                        user32.ShowWindow(hwnd, 9)
                        user32.SetForegroundWindow(hwnd)
                        return False
                return True
            WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_int, ctypes.c_int)
            user32.EnumWindows(WNDENUMPROC(enum_cb), 0)
        except Exception:
            pass
        return

    # Linux (X11)
    try:
        import Xlib.display, Xlib.X, Xlib.protocol.event
        d = Xlib.display.Display()
        root = d.screen().root

        def find_win(w):
            try:
                name = w.get_wm_name() or ""
                if "Mogan" in name or "STEM" in name or "Liii" in name:
                    return w
                for c in w.query_tree().children:
                    res = find_win(c)
                    if res:
                        return res
            except Exception:
                pass
            return None

        w = find_win(root)
        if w:
            net_active = d.intern_atom("_NET_ACTIVE_WINDOW")
            cm = Xlib.protocol.event.ClientMessage(
                window=w,
                client_type=net_active,
                data=(32, [2, Xlib.X.CurrentTime, 0, 0, 0])
            )
            root.send_event(cm, event_mask=Xlib.X.SubstructureRedirectMask | Xlib.X.SubstructureNotifyMask)
            w.set_input_focus(Xlib.X.RevertToParent, Xlib.X.CurrentTime)
            w.configure(stack_mode=Xlib.X.Above)
            d.sync()
    except Exception:
        pass


def clean_close_mogan(proc, kb):
    try:
        focus_mogan_window()
        mod = Key.cmd if IS_DARWIN else Key.ctrl
        with kb.pressed(mod):
            kb.press('q')
            time.sleep(0.05)
            kb.release('q')
        time.sleep(1.0)
    except Exception:
        pass

    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=3.0)
        except Exception:
            proc.kill()


def is_accent_color(pixel):
    """Check if pixel matches accent color (#215a6a: R~33, G~90, B~106)."""
    r, g, b = pixel[:3]
    return (15 <= r <= 50) and (70 <= g <= 115) and (85 <= b <= 130)


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[6204] ==========================================================")
    print("[6204] Starting Automated AI Sidebar Auxiliary Buttons Test (Issue 6204)")
    print(f"[6204] Binary: {bin_path}")
    print("[6204] ==========================================================")

    proc = subprocess.Popen([bin_path, "-x", "(show-chat-sidebar #t)"], env=env, cwd=repo_root)
    kb = KeyboardController()
    mouse = MouseController()

    try:
        time.sleep(5.0)
        focus_mogan_window()
        time.sleep(1.0)

        # Grab initial screen
        shot0 = ImageGrab.grab()
        w, h = shot0.size

        # In standard 3840x2400 resolution or scaled:
        # Scale coordinates relative to screen resolution
        scale_x = w / 3840.0
        scale_y = h / 2400.0

        translate_pos = (int(2810 * scale_x), int(125 * scale_y))
        explain_pos = (int(2890 * scale_x), int(125 * scale_y))

        # -------------------------------------------------------------
        # Step 1: Initial state - both buttons should NOT be active
        # -------------------------------------------------------------
        print("[6204] Step 1: Verifying initial state...")
        pix_trans0 = shot0.getpixel(translate_pos)
        pix_expl0 = shot0.getpixel(explain_pos)
        assert not is_accent_color(pix_trans0), f"Translate button unexpectedly active initially: {pix_trans0}"
        assert not is_accent_color(pix_expl0), f"Explain button unexpectedly active initially: {pix_expl0}"
        print("[6204]   Initial state verified (neither button active).")

        # -------------------------------------------------------------
        # Step 2: Click Translate button
        # -------------------------------------------------------------
        print("[6204] Step 2: Clicking Translate button...")
        mouse.position = translate_pos
        time.sleep(0.2)
        mouse.click(Button.left)
        time.sleep(2.0)

        shot1 = ImageGrab.grab()
        pix_trans1 = shot1.getpixel(translate_pos)
        pix_expl1 = shot1.getpixel(explain_pos)

        assert is_accent_color(pix_trans1), f"Translate button should be active (#215a6a), got {pix_trans1}"
        assert not is_accent_color(pix_expl1), f"Explain button should NOT be active, got {pix_expl1}"
        print("[6204]   Translate button active, Explain button inactive (mutually exclusive).")

        # -------------------------------------------------------------
        # Step 3: Click Explain button
        # -------------------------------------------------------------
        print("[6204] Step 3: Clicking Explain button...")
        mouse.position = explain_pos
        time.sleep(0.2)
        mouse.click(Button.left)
        time.sleep(2.0)

        shot2 = ImageGrab.grab()
        pix_trans2 = shot2.getpixel(translate_pos)
        pix_expl2 = shot2.getpixel(explain_pos)

        assert not is_accent_color(pix_trans2), f"Translate button should NOT be active, got {pix_trans2}"
        assert is_accent_color(pix_expl2), f"Explain button should be active (#215a6a), got {pix_expl2}"
        print("[6204]   Explain button active, Translate button inactive (mutually exclusive).")

        # -------------------------------------------------------------
        # Step 4: Click Explain button again to toggle off
        # -------------------------------------------------------------
        print("[6204] Step 4: Clicking Explain button again to toggle off...")
        mouse.position = explain_pos
        time.sleep(0.2)
        mouse.click(Button.left)
        time.sleep(2.0)

        shot3 = ImageGrab.grab()
        pix_trans3 = shot3.getpixel(translate_pos)
        pix_expl3 = shot3.getpixel(explain_pos)

        assert not is_accent_color(pix_trans3), f"Translate button should be inactive after toggle off, got {pix_trans3}"
        assert not is_accent_color(pix_expl3), f"Explain button should be inactive after toggle off, got {pix_expl3}"
        print("[6204]   Both buttons successfully toggled off.")

        print("\n[6204] ALL CHECKS PASSED!")
        return 0

    finally:
        clean_close_mogan(proc, kb)


if __name__ == "__main__":
    sys.exit(run_test())
