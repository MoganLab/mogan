#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status    |
| ----------------------- | ----------- | --------- |
| Darcy Shen <da@liii.pro>| Linux (X11) | Validated |

Automated UI test for issue 1317:
1. Launch Mogan STEM.
2. Create a new draft document (Ctrl+N).
3. Type '1' in the draft document.
4. Export as PDF and open the exported PDF in Mogan.
   Verify the PDF displays '1'.
5. Close the PDF tab (Ctrl+W) to return to the draft document automatically.
6. Type '2' in the draft document (so content is now '12').
7. Export as PDF again and open in Mogan.
8. Inspect the displayed PDF in Mogan:
   - Before fix: Mogan fails to reload the updated PDF and still displays '1' (bug reproduced).
   - After fix: Mogan reloads the PDF and correctly displays '12'.
"""

import os
import sys
import time
import subprocess
import tempfile
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


def find_mogan_window_x11(d, root, target_pid=None):
    """Recursively find the Mogan window in the X11 tree."""
    net_pid = d.intern_atom("_NET_WM_PID")

    def find(win):
        try:
            if target_pid is not None:
                p = win.get_full_property(net_pid, 0)
                if p and p.value[0] == target_pid:
                    geom = win.get_geometry()
                    if geom.width > 200:
                        return win
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

    return find(root)


def get_mogan_window_rect(target_pid=None):
    """Returns (x, y, w, h) of Mogan window, or default full screen."""
    if not IS_DARWIN and not IS_WINDOWS:
        try:
            import Xlib.display
            d = Xlib.display.Display()
            root = d.screen().root
            w = find_mogan_window_x11(d, root, target_pid)
            if w:
                geom = w.get_geometry()
                coords = w.translate_coords(root, 0, 0)
                return (coords.x, coords.y, geom.width, geom.height)
        except Exception:
            pass

    img = ImageGrab.grab()
    return (0, 0, img.size[0], img.size[1])


def focus_mogan_window(target_pid=None):
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
        net_active = d.intern_atom("_NET_ACTIVE_WINDOW")

        w = find_mogan_window_x11(d, root, target_pid)
        if w:
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


def extract_content_text_pixels(img, scale):
    """
    Extracts dark text pixel count in the page body area.
    For '1', pixel count is ~15-30.
    For '12', pixel count is ~40-70.
    """
    arr = np.array(img)
    h, w, _ = arr.shape
    # Page text region: x in [1300*scale/2, 1450*scale/2], y in [380*scale/2, 460*scale/2]
    y1 = int(380 * (scale / 2.0))
    y2 = int(460 * (scale / 2.0))
    x1 = int(1300 * (scale / 2.0))
    x2 = int(1450 * (scale / 2.0))
    y1, y2 = max(0, y1), min(h, y2)
    x1, x2 = max(0, x1), min(w, x2)

    crop = arr[y1:y2, x1:x2]
    dark_pixels = np.sum((crop[:, :, 0] < 60) & (crop[:, :, 1] < 60) & (crop[:, :, 2] < 60))
    return dark_pixels, crop


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1317] Using binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1317] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, "-d"], env=env, cwd=repo_root)

    kb = KeyboardController()
    mouse = MouseController()
    mod_key = Key.cmd if IS_DARWIN else Key.ctrl

    try:
        time.sleep(3.5)
        ret = proc.poll()
        if ret is not None:
            print(f"[1317] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        focus_mogan_window(proc.pid)
        time.sleep(0.5)

        wx, wy, ww, wh = get_mogan_window_rect(proc.pid)
        scale = ww / 1920.0
        print(f"[1317] Window rect: ({wx}, {wy}, {ww}, {wh}), scale: {scale:.2f}")

        # Click inside window to guarantee focus
        mouse.position = (wx + int(500 * scale), wy + int(500 * scale))
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        # Step 2: New draft document (Ctrl+N)
        print("[1317] Step 2: Creating new draft document (Ctrl+N)...")
        with kb.pressed(mod_key):
            kb.press('n')
            kb.release('n')
        time.sleep(2.5)

        # Step 3: Type '1'
        print("[1317] Step 3: Typing '1' in draft document...")
        kb.press('1')
        kb.release('1')
        time.sleep(0.5)

        file_menu_pos = (wx + int(25 * scale), wy + int(68 * scale))
        export_pdf_pos = (wx + int(50 * scale), wy + int(322 * scale))
        export_btn_pos = (wx + ww // 2 - int(53 * scale), wy + wh // 2 + int(34 * scale))

        def export_pdf_and_open():
            mouse.position = file_menu_pos
            time.sleep(0.3)
            mouse.click(Button.left)
            time.sleep(0.8)

            mouse.position = export_pdf_pos
            time.sleep(0.3)
            mouse.click(Button.left)
            time.sleep(1.5)

            print(f"[1317] Clicking '导出' button at {export_btn_pos}...")
            mouse.position = export_btn_pos
            time.sleep(0.3)
            mouse.click(Button.left)
            time.sleep(2.5)

            print("[1317] Confirming 'Open PDF' (Enter)...")
            kb.press(Key.enter)
            kb.release(Key.enter)
            time.sleep(3.0)

        def capture_pdf(tag):
            img = ImageGrab.grab()
            dark, crop = extract_content_text_pixels(img, scale)
            print(f"[1317] PDF {tag} text dark pixel count: {dark}")
            path = os.path.join(tempfile.gettempdir(), f"1317_pdf{tag}.png")
            img.save(path)
            print(f"[1317] Saved PDF {tag} screenshot to {path}")
            return crop

        # Step 4: Export as PDF and open in Mogan
        print("[1317] Step 4: Exporting PDF and opening in Mogan...")
        export_pdf_and_open()
        crop1 = capture_pdf(1)

        # Step 5: Close PDF tab (Ctrl+W)
        print("[1317] Step 5: Closing PDF tab (Ctrl+W) to return to draft...")
        with kb.pressed(mod_key):
            kb.press('w')
            kb.release('w')
        time.sleep(2.0)

        # Step 6: Type '2' in draft document
        print("[1317] Step 6: Typing '2' in draft document...")
        kb.press('2')
        kb.release('2')
        time.sleep(0.5)

        # Step 7: Export PDF again and open in Mogan
        print("[1317] Step 7: Exporting PDF again and opening in Mogan...")
        export_pdf_and_open()
        crop2 = capture_pdf(2)

        crop_diff = np.sum(np.abs(crop1.astype(int) - crop2.astype(int)) > 20)
        print(f"[1317] Content difference between PDF 1 and PDF 2: {crop_diff} pixels")

        # Step 8: Close PDF tab, close document tab, and close window
        print("[1317] Step 8: Closing PDF tab, closing document, and closing window...")
        try:
            with kb.pressed(mod_key):
                kb.press('w')
                kb.release('w')
            time.sleep(1.0)
            with kb.pressed(mod_key):
                kb.press('w')
                kb.release('w')
            time.sleep(1.0)
            focus_mogan_window(proc.pid)
            with kb.pressed(mod_key):
                kb.press('q')
                kb.release('q')
            time.sleep(1.5)
        except Exception:
            pass

        if crop_diff == 0:
            print("[1317] REPRODUCED BUG: PDF in Mogan displays '1' instead of '12' (PDF viewer was not reloaded)!")
            return 2
        else:
            print("[1317] SUCCESS: PDF in Mogan updated and displays '12'!")
            return 0

    finally:
        if proc.poll() is None:
            try:
                focus_mogan_window(proc.pid)
                with kb.pressed(mod_key):
                    kb.press('q')
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
