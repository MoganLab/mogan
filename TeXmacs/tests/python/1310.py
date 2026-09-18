#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated UI test for issue 1310:
1. Launch Mogan STEM.
2. On Home page, open the document '中科大少年班数学分析讲义一.stem' from recent documents.
3. Open menu: 文档 -> 页码... (Document -> Page numbers).
4. Verify the PageNumber dialog opens with existing rules.
5. When rules have no modifications, verify:
   - '应用' (Apply) button is disabled/greyed out.
   - Hovering over '应用' displays the tooltip '未检测到页码规则的变更'.
6. Delete the last rule.
7. Verify rules are now modified:
   - '应用' button becomes enabled (dark accent color).
   - Tooltip '未检测到页码规则的变更' is no longer displayed.
8. Press Esc to dismiss the dialog without altering document.
9. Exit Mogan STEM cleanly.
"""

import os
import sys
import time
import tempfile
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

# Fallback: import pynput
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


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1310] Using binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1310] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, "-d"], env=env, cwd=repo_root)

    try:
        time.sleep(3.5)
        ret = proc.poll()
        if ret is not None:
            print(f"[1310] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[1310] Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(0.5)

        wx, wy, ww, wh = get_mogan_window_rect()
        scale = ww / 1920.0
        print(f"[1310] Window rect: ({wx}, {wy}, {ww}, {wh}), scale: {scale:.2f}")

        mouse = MouseController()
        kb = KeyboardController()

        # Step 2: Open "中科大少年班数学分析讲义一.stem" from recent documents
        doc_x = wx + int(250 * scale)
        doc_y = wy + int(495 * scale)
        print(f"[1310] Step 2: Clicking recent document at ({doc_x}, {doc_y})...")
        mouse.position = (doc_x, doc_y)
        time.sleep(0.3)
        mouse.click(Button.left)

        print("[1310] Waiting for Mathematical Analysis document to load and typeset...")
        time.sleep(10.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[1310] ERROR: Mogan crashed while opening document with code {ret}")
            return 1

        # Step 3: Open "文档" (Document) menu
        menu_doc_x = wx + int(312 * scale)
        menu_doc_y = wy + int(70 * scale)
        print(f"[1310] Step 3: Clicking '文档' (Document) menu at ({menu_doc_x}, {menu_doc_y})...")
        mouse.position = (menu_doc_x, menu_doc_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.8)

        # Step 4: Click "页码..." (Page numbers...)
        menu_pn_x = wx + int(310 * scale)
        menu_pn_y = wy + int(317 * scale)
        print(f"[1310] Step 4: Clicking '页码...' at ({menu_pn_x}, {menu_pn_y})...")
        mouse.position = (menu_pn_x, menu_pn_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1310] ERROR: Mogan crashed after opening PageNumber dialog with code {ret}")
            return 1

        # Step 5: Hover over "应用" (Apply) button
        apply_x = wx + int(1220 * scale)
        apply_y = wy + int(822 * scale)
        print(f"[1310] Step 5: Hovering over '应用' (Apply) button at ({apply_x}, {apply_y})...")
        mouse.position = (apply_x, apply_y)
        time.sleep(1.0)

        img_disabled = ImageGrab.grab()
        disabled_path = os.path.join(tempfile.gettempdir(), "1310_apply_disabled.png")
        img_disabled.save(disabled_path)
        print(f"[1310] Saved disabled state screenshot to {disabled_path}")

        # Verify tooltip appears above the Apply button
        # Tooltip bubble is centered above Apply button with dark background #2d3748
        tt_x1 = wx + int(1075 * scale)
        tt_y1 = wy + int(760 * scale)
        tt_x2 = wx + int(1225 * scale)
        tt_y2 = wy + int(790 * scale)
        tt_arr = np.array(img_disabled.crop((tt_x1, tt_y1, tt_x2, tt_y2)))

        # Dark pixels of the tooltip pill: R<60, G<70, B<90
        tt_dark_pixels = int(np.sum((tt_arr[:, :, 0] < 60) & (tt_arr[:, :, 1] < 70) & (tt_arr[:, :, 2] < 90)))
        print(f"[1310] Tooltip dark pixels when disabled: {tt_dark_pixels}")
        if tt_dark_pixels < 200:
            print(f"[1310] ERROR: Tooltip '未检测到页码规则的变更' not detected! Dark pixels: {tt_dark_pixels}")
            return 1
        print("[1310] SUCCESS: Tooltip '未检测到页码规则的变更' appeared correctly on hover!")

        # Verify Apply button is disabled (greyed out, no solid dark pill)
        btn_box = (apply_x - int(20 * scale), apply_y - int(8 * scale),
                   apply_x + int(20 * scale), apply_y + int(8 * scale))
        btn_arr_dis = np.array(img_disabled.crop(btn_box))
        solid_dark_dis = int(np.sum((btn_arr_dis[:, :, 0] < 80) & (btn_arr_dis[:, :, 1] < 80) & (btn_arr_dis[:, :, 2] < 80)))
        print(f"[1310] Apply button solid dark pixels when disabled: {solid_dark_dis}")
        if solid_dark_dis > 50:
            print("[1310] ERROR: Apply button is not greyed out when rules unchanged!")
            return 1
        print("[1310] SUCCESS: Apply button is disabled and greyed out!")

        # Step 6: Click delete button (✕) of the last rule to modify rules
        del_x = wx + int(1346 * scale)
        del_y = wy + int(650 * scale)
        print(f"[1310] Step 6: Clicking delete button of last rule at ({del_x}, {del_y})...")
        mouse.position = (del_x, del_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.0)

        # Move mouse away then hover back over Apply button
        mouse.position = (wx + int(750 * scale), wy + int(750 * scale))
        time.sleep(0.2)
        mouse.position = (apply_x, apply_y)
        time.sleep(1.0)

        img_enabled = ImageGrab.grab()
        enabled_path = os.path.join(tempfile.gettempdir(), "1310_apply_enabled.png")
        img_enabled.save(enabled_path)
        print(f"[1310] Saved enabled state screenshot to {enabled_path}")

        # Verify Apply button is now enabled (dark accent pill)
        btn_arr_en = np.array(img_enabled.crop(btn_box))
        solid_dark_en = int(np.sum((btn_arr_en[:, :, 0] < 80) & (btn_arr_en[:, :, 1] < 80) & (btn_arr_en[:, :, 2] < 80)))
        print(f"[1310] Apply button solid dark pixels when enabled: {solid_dark_en}")
        if solid_dark_en < 200:
            print(f"[1310] ERROR: Apply button did not become enabled after rule change! Dark pixels: {solid_dark_en}")
            return 1
        print("[1310] SUCCESS: Apply button is enabled with active accent styling!")

        # Verify tooltip is no longer shown
        tt_arr_en = np.array(img_enabled.crop((tt_x1, tt_y1, tt_x2, tt_y2)))
        tt_dark_en = int(np.sum((tt_arr_en[:, :, 0] < 60) & (tt_arr_en[:, :, 1] < 70) & (tt_arr_en[:, :, 2] < 90)))
        print(f"[1310] Tooltip dark pixels when enabled: {tt_dark_en}")
        if tt_dark_en > 100:
            print(f"[1310] ERROR: Tooltip unexpectedly visible when button enabled! Dark pixels: {tt_dark_en}")
            return 1
        print("[1310] SUCCESS: Tooltip disappeared when button is enabled!")

        # Step 7: Close dialog with Esc without saving
        print("[1310] Step 7: Dismissing dialog with Esc...")
        kb.press(Key.esc)
        kb.release(Key.esc)
        time.sleep(0.8)

        # Step 8: Clean exit with Ctrl+Q
        print("[1310] Step 8: Exiting Mogan STEM cleanly...")
        with kb.pressed(Key.ctrl):
            kb.press("q")
            kb.release("q")
        time.sleep(1.5)

        ret = proc.poll()
        if ret is None:
            proc.terminate()
            proc.wait(timeout=3.0)

        print("[1310] ALL CHECKS PASSED SUCCESSFULLY!")
        return 0

    except Exception as e:
        print(f"[1310] Exception during test: {e}")
        try:
            proc.terminate()
            proc.wait(timeout=3.0)
        except Exception:
            pass
        return 1


if __name__ == "__main__":
    sys.exit(run_test())
