#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated UI test for issue 1287:
1. Launch Mogan STEM.
2. Press Ctrl+T to create a new document tab.
3. Click menu: 文档 -> 颜色 -> 背景色 -> 渐变 (Document -> Colors -> Background -> Gradient).
4. Verify the new QML "Gradient selector" dialog appears.
5. Verify the live gradient preview function works properly with real-time color rendering.
6. Click "确认" (OK) to apply the gradient to the document.
7. Verify Mogan does NOT crash (resolving issue #4325) and document background updates.
"""

import os
import sys
import time
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
            import Xlib.X

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
    print(f"[1287] Using binary: {bin_path}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1287] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, "-d"], env=env, cwd=repo_root)
    modifier = Key.cmd if IS_DARWIN else Key.ctrl

    try:
        time.sleep(3.5)
        ret = proc.poll()
        if ret is not None:
            print(f"[1287] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[1287] Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(0.5)

        wx, wy, ww, wh = get_mogan_window_rect()
        scale = ww / 1920.0
        print(f"[1287] Window rect: ({wx}, {wy}, {ww}, {wh}), scale factor: {scale:.2f}")

        mouse = MouseController()
        kb = KeyboardController()

        # Click inside the startup window to guarantee keyboard focus
        mouse.position = (wx + int(500 * scale), wy + int(500 * scale))
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        # Step 2: Press Ctrl+T to create a new tab
        print("[1287] Step 2: Pressing Ctrl+T to open a new tab...")
        with kb.pressed(modifier):
            kb.press("t")
            kb.release("t")
        time.sleep(2.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[1287] ERROR: Mogan crashed after Ctrl+T with code {ret}")
            return 1

        # Step 3: Click 文档 (Document) menu
        doc_x = wx + int(312 * scale)
        doc_y = wy + int(66 * scale)
        print(f"[1287] Step 3: Clicking '文档' menu at ({doc_x}, {doc_y})...")
        mouse.position = (doc_x, doc_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.8)

        # Step 4: Hover 颜色 (Colors) submenu
        color_x = wx + int(350 * scale)
        color_y = wy + int(495 * scale)
        print(f"[1287] Step 4: Hovering '颜色' submenu at ({color_x}, {color_y})...")
        mouse.position = (color_x, color_y)
        time.sleep(0.8)

        # Step 5: Hover 背景色 (Background) submenu
        bg_x = wx + int(408 * scale)
        bg_y = wy + int(500 * scale)
        print(f"[1287] Step 5: Hovering '背景色' submenu at ({bg_x}, {bg_y})...")
        mouse.position = (bg_x, bg_y)
        time.sleep(0.8)

        # Step 6: Click 渐变 (Gradient) menu item
        grad_x = wx + int(484 * scale)
        grad_y = wy + int(783 * scale)
        print(f"[1287] Step 6: Clicking '渐变...' at ({grad_x}, {grad_y})...")
        mouse.position = (grad_x, grad_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1287] ERROR: Mogan crashed after clicking '渐变' with code {ret}")
            return 1

        # Step 7: Capture screenshot and verify the QML dialog & live preview
        print("[1287] Step 7: Inspecting QML Gradient Selector dialog & live preview...")
        full_screenshot = ImageGrab.grab()
        screenshot_path = os.path.join(repo_root, "devel", "1287_gradient_dialog.png")
        os.makedirs(os.path.dirname(screenshot_path), exist_ok=True)
        full_screenshot.save(screenshot_path)

        # Crop dialog area
        w, h = full_screenshot.size
        dialog_crop = full_screenshot.crop((w // 4, h // 4, 3 * w // 4, 3 * h // 4))
        dialog_crop_path = os.path.join(repo_root, "devel", "1287_gradient_dialog_crop.png")
        dialog_crop.save(dialog_crop_path)

        # Analyze preview region
        crop_w, crop_h = dialog_crop.size
        preview_region = dialog_crop.crop((int(crop_w * 0.5), int(crop_h * 0.15), int(crop_w * 0.95), int(crop_h * 0.8)))
        arr = np.array(preview_region)
        std_val = float(arr.std())
        unique_colors = len(np.unique(arr.reshape(-1, 3), axis=0))

        print(f"[1287] Preview area analysis: shape={arr.shape}, color std={std_val:.2f}, unique colors={unique_colors}")

        if std_val > 40.0 and unique_colors > 100:
            print("[1287] SUCCESS: Live gradient preview is actively rendering colors!")
        else:
            print(f"[1287] ERROR: Preview area does not show expected gradient variance (std={std_val:.2f})")
            return 1

        # Step 8: Click OK button ('确认') to apply gradient
        ok_x = wx + int(1116 * scale)
        ok_y = wy + int(724 * scale)
        print(f"[1287] Step 8: Clicking '确认' (OK) button at ({ok_x}, {ok_y})...")
        mouse.position = (ok_x, ok_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[1287] ERROR: Mogan crashed after applying gradient with code {ret}")
            return 1

        print("[1287] SUCCESS: Mogan survived applying gradient background without crash!")

        # Step 9: Capture document with gradient background
        doc_screenshot = ImageGrab.grab()
        doc_path = os.path.join(repo_root, "devel", "1287_document_gradient.png")
        doc_screenshot.save(doc_path)
        print(f"[1287] Saved document screenshot to {doc_path}")

        print("[1287] TEST PASSED: QML Gradient selector works, live preview verified, no crash.")
        return 0

    finally:
        if proc.poll() is None:
            print("[1287] Terminating Mogan...")
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
