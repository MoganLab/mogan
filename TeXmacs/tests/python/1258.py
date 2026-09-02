#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                           | Platform      | Status |
| -------------------------------- | ------------- | ------ |
| Darcy Shen <da@liii.pro>         | Linux (X11)   | Passed |
| ShuLi Zheng <2831850183@qq.com>  | macOS (arm64) | Passed |

Automated end-to-end UI test for issue 1258:
Verify that the underscore in 'draft_YYYYMMDDHHMMSS.tmu' is visible in
the Home page (Startup tab) Recent Documents list after saving and previewing.
"""

import os
import sys
import time
import subprocess
import numpy as np
from PIL import ImageGrab, Image

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
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/debug/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/debug/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def focus_mogan_window():
    """Ensure Mogan window is raised and focused across platforms."""
    if sys.platform == "darwin":
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

    try:
        import Xlib.display
        import Xlib.X
        import Xlib.protocol.event

        d = Xlib.display.Display()
        root = d.screen().root

        def find_mogan(win):
            try:
                name = win.get_wm_name()
                if name and "Liii STEM" in name:
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


def check_underscore_visible(screenshot_path):
    """
    Analyzes the screenshot of the Home page to verify that the underscore
    in 'draft_...' recent document filename is visible (not clipped).
    """
    img = Image.open(screenshot_path)
    w, h = img.size

    # Locate white background area of the main page (excluding dark sidebar)
    rgb = np.array(img.convert("RGB"))
    mid_row = rgb[int(h * 0.5), :, :]
    white_indices = np.where((mid_row[:, 0] > 240) & (mid_row[:, 1] > 240) & (mid_row[:, 2] > 240))[0]
    if len(white_indices) == 0:
        print("[1258] ERROR: Could not locate main content area in screenshot.")
        return False

    left_x = white_indices[0] + 20
    right_x = min(w, left_x + 1600)

    # Crop the recent documents region
    recent_crop = img.crop((left_x, int(h * 0.40), right_x, int(h * 0.95)))
    arr = np.array(recent_crop.convert("L"))

    # Binarize: text is dark (< 128), background is light (> 200)
    binary = (arr < 128).astype(np.uint8)

    # Find text lines by horizontal projection
    row_counts = binary.sum(axis=1)
    lines = []
    in_line = False
    start_r = 0
    for r in range(len(row_counts)):
        if row_counts[r] > 5 and not in_line:
            in_line = True
            start_r = r
        elif row_counts[r] <= 5 and in_line:
            in_line = False
            lines.append((start_r, r))
    if in_line:
        lines.append((start_r, len(row_counts)))

    print(f"[1258] Detected {len(lines)} line clusters in recent documents area")

    # Check for the underscore stroke rendered below the baseline of 'draft'
    scale = 2 if w > 2000 else 1
    max_stroke_h = 6 * scale
    max_gap = 6 * scale

    for idx, (r1, r2) in enumerate(lines):
        cluster_h = r2 - r1
        # The underscore stroke has a small height directly under the main text line
        if cluster_h <= max_stroke_h and idx > 0:
            prev_r1, prev_r2 = lines[idx - 1]
            gap = r1 - prev_r2
            if gap <= max_gap:
                line_binary = binary[r1:r2, :]
                cols = np.where(line_binary.sum(axis=0) > 0)[0]
                if len(cols) > 0:
                    c1, c2 = cols[0], cols[-1]
                    print(f"[1258] Found visible underscore stroke: rows [{r1}:{r2}] (h={cluster_h}), cols [{c1}:{c2}] (w={c2 - c1})")
                    return True

    return False


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1258] Using Mogan binary: {bin_path}")

    # On macOS, ensure .app bundle is ad-hoc signed so AMFI allows execution
    if sys.platform == "darwin":
        app_dir = os.path.dirname(os.path.dirname(os.path.dirname(bin_path)))
        if app_dir.endswith(".app"):
            subprocess.run(["codesign", "--force", "--deep", "--sign", "-", app_dir], capture_output=True)

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1258] Launching Mogan...")
    proc = subprocess.Popen([bin_path], env=env)
    keyboard = KeyboardController()
    mouse = MouseController()
    mod_key = Key.cmd if sys.platform == "darwin" else Key.ctrl

    try:
        # 1. Wait for Mogan window to open and raise to front
        time.sleep(3.5)
        focus_mogan_window()
        time.sleep(0.5)

        # 2. Click window to ensure focus
        mouse.position = (500, 500)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        # 3. Create a new document / tab (Cmd+T / Ctrl+T)
        print("[1258] Creating new tab...")
        with keyboard.pressed(mod_key):
            keyboard.press("t")
            keyboard.release("t")
        time.sleep(2.0)

        # 4. Type 'hello' and commit (Enter avoids IME composition on macOS)
        print("[1258] Typing 'hello'...")
        keyboard.type("hello")
        time.sleep(0.3)
        keyboard.press(Key.enter)
        keyboard.release(Key.enter)
        time.sleep(1.0)

        # 5. Preview document (Cmd+P / Ctrl+P) -> triggers auto-save draft and opens PDF tab
        print("[1258] Triggering preview...")
        with keyboard.pressed(mod_key):
            keyboard.press("p")
            keyboard.release("p")
        time.sleep(3.5)

        # 6. Return to Home page: click first tab / press Cmd+1 or Ctrl+1
        print("[1258] Returning to Home page...")
        focus_mogan_window()
        time.sleep(0.2)
        tab_click_pos = (130, 42) if sys.platform == "darwin" else (80, 40)
        mouse.position = tab_click_pos
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.3)
        with keyboard.pressed(mod_key):
            keyboard.press("1")
            keyboard.release("1")
        time.sleep(2.0)

        # 7. Take screenshot
        screenshot_path = "/tmp/1258_screenshot.png"
        print(f"[1258] Taking screenshot -> {screenshot_path}")
        img = ImageGrab.grab()
        img.save(screenshot_path)

        # 8. Check if underscore is visible
        success = check_underscore_visible(screenshot_path)
        if success:
            print("[1258] TEST PASSED: Underscore '_' in recent documents is visible!")
            return 0
        else:
            print("[1258] TEST FAILED: Underscore '_' not detected or clipped.")
            return 1

    finally:
        print("[1258] Terminating Mogan...")
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
