#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform      | Status |
| ----------------------- | ------------- | ------ |
| MingShen <ms@liii.pro>  | macOS (arm64) | Passed |

Automated UI test for issue 0538:
Verify that the AI sidebar state is closed upon closing a document:
1. Scenario 1 (Single document):
   - Launch Mogan STEM.
   - Create draft document 1.
   - Open AI sidebar (Cmd+J / Ctrl+J).
   - Close draft document 1 (Cmd+W / Ctrl+W).
   - Create draft document 2.
   - Verify sidebar is closed in document 2.
2. Scenario 2 (Multiple documents):
   - Create draft document A and draft document B.
   - Open AI sidebar in draft document B.
   - Close draft document B (Cmd+W / Ctrl+W) to switch to draft document A.
   - Verify sidebar is closed in draft document A.
3. Cleanly exit Mogan STEM.
"""

import os
import sys
import time
import subprocess
import numpy as np
from PIL import ImageGrab, Image

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
    return os.path.abspath(".")


def find_mogan_binary(repo_root):
    candidates = [
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def focus_mogan_window():
    """Ensure Mogan window is raised and focused."""
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
        import Xlib.display
        import Xlib.X
        import Xlib.protocol.event

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


def crop_sidebar_area(img):
    """Crop the right 30% area of the window where the sidebar dock appears."""
    w, h = img.size
    return img.crop((int(w * 0.70), 200, w - 20, h - 200))


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[0538] Using Mogan binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")
    # Bypass unsaved draft dialog in tests
    env["MOGAN_TEST_CONFIRM_CLOSE"] = "Don't save"

    print("[0538] Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path], env=env, cwd=repo_root)
    time.sleep(3.5)

    focus_mogan_window()
    time.sleep(0.5)

    kb = KeyboardController()
    mouse = MouseController()
    mod = Key.cmd if IS_DARWIN else Key.ctrl

    def send_shortcut(key):
        with kb.pressed(mod):
            kb.press(key)
            time.sleep(0.08)
            kb.release(key)
        time.sleep(1.8)

    try:
        # Dismiss any initial modal popup
        kb.press(Key.esc)
        kb.release(Key.esc)
        time.sleep(0.5)

        # Focus window
        mouse.position = (500, 500)
        mouse.click(Button.left)
        time.sleep(0.5)

        # -----------------------------------------------------------------
        # Scenario 1: Single document close & reopen
        # -----------------------------------------------------------------
        print("[0538] Scenario 1: Single document close & reopen...")
        print("[0538] Step 1.1: Creating Draft 1...")
        send_shortcut('t')

        img_draft1_no_sb = ImageGrab.grab()
        crop_no_sb = crop_sidebar_area(img_draft1_no_sb)

        print("[0538] Step 1.2: Opening AI sidebar with shortcut...")
        send_shortcut('j')

        img_draft1_with_sb = ImageGrab.grab()
        crop_with_sb = crop_sidebar_area(img_draft1_with_sb)

        diff_sb_open = np.abs(np.array(crop_with_sb).astype(int) - np.array(crop_no_sb).astype(int)).mean()
        print(f"[0538] Pixel diff between NO sidebar and WITH sidebar: {diff_sb_open:.2f}")
        if diff_sb_open < 10.0:
            print("[0538] WARNING: Sidebar does not appear to be open, diff too small.")

        print("[0538] Step 1.3: Closing Draft 1...")
        send_shortcut('w')

        print("[0538] Step 1.4: Creating Draft 2 from home page...")
        send_shortcut('t')

        img_draft2 = ImageGrab.grab()
        crop_draft2 = crop_sidebar_area(img_draft2)

        diff_s1_closed = np.abs(np.array(crop_draft2).astype(int) - np.array(crop_no_sb).astype(int)).mean()
        diff_s1_open = np.abs(np.array(crop_draft2).astype(int) - np.array(crop_with_sb).astype(int)).mean()
        print(f"[0538] Scenario 1: diff(Draft 2, NO sidebar) = {diff_s1_closed:.2f}")
        print(f"[0538] Scenario 1: diff(Draft 2, WITH sidebar) = {diff_s1_open:.2f}")

        if diff_s1_closed > 5.0 or diff_s1_open < 5.0:
            print("[0538] TEST FAILED: Draft 2 still shows opened sidebar in Scenario 1!")
            return 1
        print("[0538] Scenario 1 PASSED: Draft 2 correctly shows sidebar closed.")

        # Clean up Draft 2
        send_shortcut('w')

        # -----------------------------------------------------------------
        # Scenario 2: Multiple documents tab switch on close
        # -----------------------------------------------------------------
        print("[0538] Scenario 2: Multiple documents tab close & switch...")
        print("[0538] Step 2.1: Creating Draft A...")
        send_shortcut('t')

        print("[0538] Step 2.2: Creating Draft B...")
        send_shortcut('t')

        print("[0538] Step 2.3: Opening AI sidebar in Draft B...")
        send_shortcut('j')

        print("[0538] Step 2.4: Closing Draft B (automatically switching to Draft A)...")
        send_shortcut('w')

        img_draftA = ImageGrab.grab()
        crop_draftA = crop_sidebar_area(img_draftA)

        diff_s2_closed = np.abs(np.array(crop_draftA).astype(int) - np.array(crop_no_sb).astype(int)).mean()
        diff_s2_open = np.abs(np.array(crop_draftA).astype(int) - np.array(crop_with_sb).astype(int)).mean()
        print(f"[0538] Scenario 2: diff(Draft A, NO sidebar) = {diff_s2_closed:.2f}")
        print(f"[0538] Scenario 2: diff(Draft A, WITH sidebar) = {diff_s2_open:.2f}")

        if diff_s2_closed > 5.0 or diff_s2_open < 5.0:
            print("[0538] TEST FAILED: Draft A still shows opened sidebar in Scenario 2!")
            return 1
        print("[0538] Scenario 2 PASSED: Draft A correctly shows sidebar closed.")

        # Clean up Draft A
        send_shortcut('w')

        print("[0538] ALL TESTS PASSED!")
        return 0

    finally:
        try:
            focus_mogan_window()
            with kb.pressed(mod):
                kb.press('q')
                time.sleep(0.08)
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


if __name__ == "__main__":
    sys.exit(run_test())
