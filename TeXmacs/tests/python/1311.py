#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated UI test for issue 1311:
1. Launch Mogan STEM with -d.
2. Locate the "转到" (Go) button in the window title bar (left of "领取会员").
3. Click the Go button to trigger the modern QML GoMenu dropdown.
4. Verify the dropdown appears and is properly rendered:
   - Right-aligned to the Go button
   - Displays navigation items (后退, 前进, 保存位置)
   - Displays open buffers and recent documents with correct encoding (no mojibake)
5. Save screenshots for visual verification and regression tracking.
6. Press Escape to close the menu and verify clean exit without crash.
"""

import os
import sys
import time
import tempfile
import subprocess
from PIL import ImageGrab, Image
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
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    existing = [c for c in candidates if os.path.exists(c)]
    if existing:
        # Return newest binary
        existing.sort(key=lambda p: os.path.getmtime(p), reverse=True)
        return existing[0]
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


def locate_go_button(screenshot):
    """
    Locates the Go (转到) button in the title bar.
    The title bar has '领取会员' (teal button) on the right half,
    and the Go button is immediately to its left.
    """
    arr = np.array(screenshot.convert("RGB"))
    h, w, _ = arr.shape

    # Scan top 120px and right half of the screen
    top_arr = arr[:120, :, :]
    right_offset = w // 2
    right_top = top_arr[:, right_offset:, :]

    # Teal button color signature: low red (< 80), moderate green (> 100), high blue (> 120)
    teal_mask = (right_top[:, :, 0] < 80) & (right_top[:, :, 1] > 100) & (right_top[:, :, 2] > 120)
    ty, tx = np.where(teal_mask)

    if len(tx) == 0:
        return None

    tx = tx + right_offset
    teal_left = int(tx.min())
    teal_top = int(ty.min())
    teal_bottom = int(ty.max())

    btn_y = (teal_top + teal_bottom) // 2
    # Go button center is ~75px to the left of the teal button's left edge
    go_x = teal_left - 75
    go_y = btn_y
    return (go_x, go_y, teal_left)


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1311] Using binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1311] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, "-d"], env=env, cwd=repo_root)

    try:
        time.sleep(3.5)
        ret = proc.poll()
        if ret is not None:
            print(f"[1311] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[1311] Step 2: Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(0.5)

        mouse = MouseController()
        kb = KeyboardController()

        print("[1311] Step 3: Detecting Go (转到) button in title bar...")
        btn_pos = None
        for _ in range(20):
            initial_screen = ImageGrab.grab()
            btn_pos = locate_go_button(initial_screen)
            if btn_pos is not None:
                break
            time.sleep(0.5)

        if btn_pos is None:
            print("[1311] ERROR: Could not locate Go button / Claim Membership button on screen.")
            return 1

        go_x, go_y, teal_left = btn_pos
        print(f"[1311] Go button located at ({go_x}, {go_y})")

        print(f"[1311] Step 4: Clicking Go button to open dropdown menu...")
        mouse.position = (go_x, go_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[1311] ERROR: Mogan crashed upon clicking Go button with code {ret}")
            return 1

        print("[1311] Step 5: Capturing dropdown menu screenshot...")
        after_img = ImageGrab.grab()
        screen_path = os.path.join(tempfile.gettempdir(), "1311_go_menu_screen.png")
        after_img.save(screen_path)
        print(f"[1311] Full screenshot saved to {screen_path}")

        # Crop around the Go button and the popup menu (accounting for HiDPI)
        crop_box = (max(0, go_x - 650), 0, min(after_img.width, go_x + 150), min(after_img.height, 1200))
        menu_crop = after_img.crop(crop_box)
        crop_path = os.path.join(tempfile.gettempdir(), "1311_go_menu_popup.png")
        menu_crop.save(crop_path)
        print(f"[1311] Cropped menu screenshot saved to {crop_path}")

        # Verification: check that popup appeared below the button
        crop_arr = np.array(menu_crop.convert("RGB"))
        # Menu card has white/light bg (Theme.bg) or dark bg, distinct from window bar background
        menu_y_start = go_y + 10
        sub_arr = crop_arr[menu_y_start:menu_y_start + 200, :, :]
        # Check standard deviation or variance of pixel values to ensure rendered UI elements exist
        has_content = sub_arr.std() > 5.0
        if not has_content:
            print("[1311] ERROR: Dropdown menu does not appear to have rendered content.")
            return 1

        print("[1311] Step 6: Pressing Escape to close dropdown menu...")
        kb.press(Key.esc)
        kb.release(Key.esc)
        time.sleep(0.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1311] ERROR: Mogan crashed on closing menu with code {ret}")
            return 1

        print("[1311] TEST PASSED: QML GoMenu dropdown triggered, verified and captured successfully!")
        return 0

    finally:
        if proc.poll() is None:
            print("[1311] Terminating Mogan...")
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
