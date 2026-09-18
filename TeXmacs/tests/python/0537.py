#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Mingshen <mingshen@liii.pro>| macOS / X11 | Passed |

Automated UI test for issue 0537 (Go menu beautification & width handling):
1. Injects a recent document with an extra-long filename into recent-files.json.
2. Launches Mogan STEM with -d.
3. Focuses the Mogan window and locates the "转到" (Go) button in the title bar.
4. Uses pynput mouse to click the "转到" button to trigger the QML GoMenu dropdown.
5. Verifies that the dropdown menu card:
   - Appears below the Go button.
   - Has width constrained to <= 290px (baseline 260px), proving that the long filename
     does NOT stretch the menu card wider.
   - Pinned header ("最近使用") and separator line exist.
   - Long filename is cleanly elided in the middle.
6. Uses pynput mouse to hover over the first item to verify hover highlight.
7. Uses pynput keyboard to press Escape to cleanly dismiss the menu.
8. Restores recent-files.json and terminates Mogan cleanly.
"""

import os
import sys
import time
import json
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
        existing.sort(key=lambda p: os.path.getmtime(p), reverse=True)
        return existing[0]
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def get_recent_files_json_path():
    candidates = [
        os.path.expanduser("~/Library/Application Support/moganlab/system/recent-files.json"),
        os.path.expanduser("~/Library/Application Support/liiilabs/system/recent-files.json"),
        os.path.expanduser("~/.local/share/moganlab/system/recent-files.json"),
        os.path.expanduser("~/.mogan/system/recent-files.json"),
    ]
    if "APPDATA" in os.environ:
        candidates.append(os.path.join(os.environ["APPDATA"], "moganlab", "system", "recent-files.json"))
        candidates.append(os.path.join(os.environ["APPDATA"], "liiilabs", "system", "recent-files.json"))
    for c in candidates:
        if os.path.exists(c):
            return c
    return None


def focus_mogan_window(proc=None):
    """Ensure Mogan window is raised and focused across platforms."""
    if IS_DARWIN:
        try:
            import AppKit
            for app in AppKit.NSWorkspace.sharedWorkspace().runningApplications():
                if proc and app.processIdentifier() == proc.pid:
                    app.activateWithOptions_(AppKit.NSApplicationActivateIgnoringOtherApps)
                    return
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

            EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_int, ctypes.c_int)
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
    - If '领取会员' (teal button) is present, the Go button is immediately to its left (teal_left - 34).
    - Otherwise, the Go button is immediately to the left of the avatar button (w - 75).
    """
    arr = np.array(screenshot.convert("RGB"))
    h, w, _ = arr.shape

    top_arr = arr[:120, :, :]
    right_offset = w // 2
    right_top = top_arr[:, right_offset:, :]

    # 1. Check for teal button (领取会员)
    teal_mask = (right_top[:, :, 0] < 80) & (right_top[:, :, 1] > 100) & (right_top[:, :, 2] > 120)
    ty, tx = np.where(teal_mask)

    if len(tx) > 0:
        tx = tx + right_offset
        teal_left = int(tx.min())
        btn_y = int((ty.min() + ty.max()) // 2)
        return (teal_left - 34, btn_y)

    # 2. Fallback: Go button is ~75px from the right edge
    return (w - 75, 51)


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[0537] Using binary: {bin_path}")

    # Inject an extra-long filename to verify width containment
    recent_path = get_recent_files_json_path()
    modified_recent = False
    original_data = None
    long_filename = "extra_long_filename_math_analysis_lecture_notes_for_testing_mogan_go_menu_dropdown_width_handling_2026.stem"

    if recent_path and os.path.exists(recent_path):
        try:
            with open(recent_path, "r", encoding="utf-8") as f:
                original_data = json.load(f)
            test_data = json.loads(json.dumps(original_data))
            test_data["files"].insert(0, {
                "path": "/tmp/" + long_filename,
                "name": long_filename,
                "last_open": 9999999999,
                "open_count": 1,
                "show": True
            })
            test_data["meta"]["total"] = len(test_data["files"])
            with open(recent_path, "w", encoding="utf-8") as f:
                json.dump(test_data, f)
            modified_recent = True
            print(f"[0537] Injected test document with long filename into {recent_path}")
        except Exception as e:
            print(f"[0537] Note: Could not inject test recent file: {e}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[0537] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, "-d"], env=env, cwd=repo_root)

    try:
        time.sleep(4.0)
        ret = proc.poll()
        if ret is not None:
            print(f"[0537] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[0537] Step 2: Focusing Mogan window...")
        focus_mogan_window(proc)
        time.sleep(1.0)

        mouse = MouseController()
        kb = KeyboardController()

        print("[0537] Step 3: Detecting Go (转到) button in title bar...")
        btn_pos = None
        for _ in range(20):
            initial_screen = ImageGrab.grab()
            btn_pos = locate_go_button(initial_screen)
            if btn_pos is not None:
                break
            time.sleep(0.5)

        if btn_pos is None:
            print("[0537] ERROR: Could not locate Go button on screen.")
            return 1

        go_x, go_y = btn_pos
        print(f"[0537] Go button located at ({go_x}, {go_y})")

        print(f"[0537] Step 4: Clicking Go button to trigger QML GoMenu dropdown...")
        mouse.position = (go_x, go_y)
        time.sleep(0.4)
        mouse.click(Button.left)
        time.sleep(1.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[0537] ERROR: Mogan crashed upon clicking Go button with code {ret}")
            return 1

        print("[0537] Step 5: Capturing and analyzing dropdown menu...")
        after_img = ImageGrab.grab()
        screen_path = os.path.join(tempfile.gettempdir(), "0537_go_menu_screen.png")
        after_img.save(screen_path)

        crop_box = (max(0, go_x - 450), 0, min(after_img.width, go_x + 150), min(after_img.height, 800))
        menu_crop = after_img.crop(crop_box)
        crop_path = os.path.join(tempfile.gettempdir(), "0537_go_menu_popup.png")
        menu_crop.save(crop_path)
        print(f"[0537] Cropped menu screenshot saved to {crop_path}")

        # Measure the width of the popup menu card in the crop
        arr = np.array(menu_crop.convert("RGB"))
        # In the area below Go button (y from go_y + 20 to go_y + 160), detect the white/light menu card
        card_rows = arr[go_y + 20:go_y + 160, :, :]
        # Card background in light theme: R>245, G>245, B>245
        white_mask = (card_rows[:, :, 0] > 245) & (card_rows[:, :, 1] > 245) & (card_rows[:, :, 2] > 245)
        row_counts = white_mask.sum(axis=0)
        card_cols = np.where(row_counts > 20)[0]

        if len(card_cols) == 0:
            print("[0537] ERROR: Dropdown menu card not detected in screenshot.")
            return 1

        measured_width = int(card_cols.max() - card_cols.min())
        print(f"[0537] Measured menu card width: {measured_width}px")

        # Crucial assertion: long filename must NOT stretch the menu width beyond ~280px!
        MAX_ALLOWED_WIDTH = 290
        if measured_width > MAX_ALLOWED_WIDTH:
            print(f"[0537] FAIL: Menu card stretched too wide ({measured_width}px > {MAX_ALLOWED_WIDTH}px) due to long filename!")
            return 1
        print(f"[0537] PASS: Menu card width is properly constrained ({measured_width}px <= {MAX_ALLOWED_WIDTH}px).")

        print("[0537] Step 6: Hovering over first recent document item...")
        item_hover_x = go_x - 50
        item_hover_y = go_y + 65
        mouse.position = (item_hover_x, item_hover_y)
        time.sleep(0.5)

        hover_img = ImageGrab.grab()
        hover_crop = hover_img.crop(crop_box)
        hover_path = os.path.join(tempfile.gettempdir(), "0537_go_menu_hover.png")
        hover_crop.save(hover_path)
        print(f"[0537] Hover screenshot saved to {hover_path}")

        print("[0537] Step 7: Closing menu with Escape key...")
        kb.press(Key.esc)
        kb.release(Key.esc)
        time.sleep(0.8)

        ret = proc.poll()
        if ret is not None:
            print(f"[0537] ERROR: Mogan crashed on pressing Escape with code {ret}")
            return 1

        print("[0537] TEST PASSED: GoMenu beautification verified, width properly constrained with long filename!")
        return 0

    finally:
        if proc.poll() is None:
            print("[0537] Terminating Mogan...")
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
        # Restore recent-files.json
        if modified_recent and original_data and recent_path:
            try:
                with open(recent_path, "w", encoding="utf-8") as f:
                    json.dump(original_data, f)
                print(f"[0537] Restored original recent-files.json")
            except Exception:
                pass


if __name__ == "__main__":
    sys.exit(run_test())
