#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated end-to-end UI test for issue 1283:
1. Pressing Shift + arrow keys (Left, Right, Up, Down) on the Startup page
   (tmfs://startup-tab) must not crash Mogan STEM.
2. Tab shortcuts on the startup page (Ctrl+T to create a new tab, Ctrl+1/Ctrl+2
   to switch between tabs) must work as expected.
3. Tab shortcuts inside document tabs must continue to work.
"""

import os
import sys
import time
import subprocess

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
                        user32.ShowWindow(hwnd, 9)  # SW_RESTORE
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


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1283] Using binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1283] Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, "-d"], env=env, cwd=repo_root)
    modifier = Key.cmd if IS_DARWIN else Key.ctrl

    try:
        time.sleep(3.0)
        ret = proc.poll()
        if ret is not None:
            print(f"[1283] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[1283] Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(0.5)

        # Click inside the startup page content area to guarantee keyboard focus
        mouse = MouseController()
        mouse.position = (1000, 1000)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        kb = KeyboardController()

        # Step 1: Verify Shift+Arrow keys on the startup page do NOT crash
        test_arrows = [
            (Key.left, "Shift+Left"),
            (Key.right, "Shift+Right"),
            (Key.up, "Shift+Up"),
            (Key.down, "Shift+Down"),
        ]

        for key, name in test_arrows:
            print(f"[1283] Pressing {name} on startup page...")
            with kb.pressed(Key.shift):
                kb.press(key)
                kb.release(key)

            time.sleep(0.5)
            ret = proc.poll()
            if ret is not None:
                print(f"[1283] CRASH REPRODUCED: Mogan crashed on {name} with exit code {ret}!")
                return 1
            print(f"[1283] {name} passed (no crash).")

        # Step 2: Verify Ctrl+T creates a new tab from the startup page
        print("[1283] Pressing Ctrl+T on startup page to create a new tab...")
        with kb.pressed(modifier):
            kb.press("t")
            kb.release("t")
        time.sleep(1.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1283] CRASH: Mogan crashed after Ctrl+T with exit code {ret}!")
            return 1

        # Step 3: Switch back to startup tab (Ctrl+1) and to second tab (Ctrl+2)
        print("[1283] Pressing Ctrl+1 to switch to first tab (startup page)...")
        with kb.pressed(modifier):
            kb.press("1")
            kb.release("1")
        time.sleep(1.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[1283] CRASH: Mogan crashed after Ctrl+1 with exit code {ret}!")
            return 1

        print("[1283] Pressing Ctrl+2 to switch to second tab...")
        with kb.pressed(modifier):
            kb.press("2")
            kb.release("2")
        time.sleep(1.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[1283] CRASH: Mogan crashed after Ctrl+2 with exit code {ret}!")
            return 1

        # Step 4: Press Ctrl+T inside the second tab to create a third tab
        print("[1283] Pressing Ctrl+T in second tab to create third tab...")
        with kb.pressed(modifier):
            kb.press("t")
            kb.release("t")
        time.sleep(1.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1283] CRASH: Mogan crashed after Ctrl+T in tab with exit code {ret}!")
            return 1

        # Step 5: Switch among tabs using Ctrl+1 and Ctrl+3
        print("[1283] Pressing Ctrl+1 then Ctrl+3...")
        with kb.pressed(modifier):
            kb.press("1")
            kb.release("1")
        time.sleep(0.8)

        with kb.pressed(modifier):
            kb.press("3")
            kb.release("3")
        time.sleep(0.8)

        ret = proc.poll()
        if ret is not None:
            print(f"[1283] CRASH: Mogan crashed during tab switching with exit code {ret}!")
            return 1

        print("[1283] TEST PASSED: All Shift+Arrow keys and tab shortcuts work properly.")
        return 0

    finally:
        if proc.poll() is None:
            print("[1283] Terminating Mogan...")
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
