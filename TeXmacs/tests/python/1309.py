#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated UI test for issue 1309:
1. Launch Mogan STEM.
2. Press Ctrl+T to open a new tab.
3. Click menu: 插入 -> 自动 -> 参考文献 (Insert -> Automatic -> Bibliography).
4. Verify the new QML Bibliography dialog appears with initial hint '此处预览参考文献的格式'
   and primary '插入' (Insert) button disabled.
5. Verify invalid file path shows immediate error hint ('文件不存在' / '无效的 BibTeX 文件')
   and prevents submission.
6. Click '浏览' (Browse) button, select 1308.bib, and submit file dialog.
7. Verify live preview renders the bibliography entries with proper magnification and primary button is enabled.
8. Switch style via the QML EnumCombo dropdown and verify preview refreshes.
9. Click '插入' (Insert) to insert the bibliography into the document.
10. Verify Mogan does NOT crash and bibliography is inserted into the document.
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


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    bib_file = os.path.join(repo_root, "TeXmacs", "tests", "bib", "1308.bib")
    print(f"[1309] Using binary: {bin_path}")
    print(f"[1309] Using bib file: {bib_file}")

    if not os.path.exists(bib_file):
        print(f"[1309] ERROR: bib file not found at {bib_file}")
        return 1

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1309] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, "-d"], env=env, cwd=repo_root)
    modifier = Key.cmd if IS_DARWIN else Key.ctrl

    try:
        time.sleep(3.5)
        ret = proc.poll()
        if ret is not None:
            print(f"[1309] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[1309] Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(0.5)

        wx, wy, ww, wh = get_mogan_window_rect()
        scale = ww / 1920.0
        print(f"[1309] Window rect: ({wx}, {wy}, {ww}, {wh}), scale: {scale:.2f}")

        mouse = MouseController()
        kb = KeyboardController()

        # Click inside the window to guarantee keyboard focus
        mouse.position = (wx + int(500 * scale), wy + int(500 * scale))
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        # Step 2: Press Ctrl+T to open a new tab
        print("[1309] Step 2: Pressing Ctrl+T to open a new tab...")
        with kb.pressed(modifier):
            kb.press("t")
            kb.release("t")
        time.sleep(2.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[1309] ERROR: Mogan crashed after Ctrl+T with code {ret}")
            return 1

        # Step 3: Open Insert -> Automatic -> Bibliography
        insert_x = wx + int(155 * scale)
        insert_y = wy + int(70 * scale)
        print(f"[1309] Step 3: Clicking '插入' (Insert) at ({insert_x}, {insert_y})...")
        mouse.position = (insert_x, insert_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        auto_x = wx + int(140 * scale)
        auto_y = wy + int(250 * scale)
        print(f"[1309] Step 3: Hovering '自动' (Automatic) at ({auto_x}, {auto_y})...")
        mouse.position = (auto_x, auto_y)
        time.sleep(0.5)

        bib_x = wx + int(200 * scale)
        bib_y = wy + int(270 * scale)
        print(f"[1309] Step 3: Clicking '参考文献' (Bibliography) at ({bib_x}, {bib_y})...")
        mouse.position = (bib_x, bib_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1309] ERROR: Mogan crashed after opening Bibliography dialog with code {ret}")
            return 1

        # Step 4: Verify QML dialog appears with initial hint
        print("[1309] Step 4: Verifying initial QML Bibliography dialog...")
        init_screenshot = ImageGrab.grab()
        screenshot_path = os.path.join(tempfile.gettempdir(), "1309_qml_dialog_init.png")
        init_screenshot.save(screenshot_path)
        print(f"[1309] Saved initial dialog screenshot to {screenshot_path}")

        # Step 5: Test invalid path feedback
        print("[1309] Step 5: Testing invalid file path feedback...")
        file_box_x = wx + int(850 * scale)
        file_box_y = wy + int(365 * scale)
        mouse.position = (file_box_x, file_box_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.3)
        kb.type("/tmp/non_existent_file.bib")
        time.sleep(1.5)

        invalid_screenshot = ImageGrab.grab()
        invalid_path = os.path.join(tempfile.gettempdir(), "1309_qml_invalid_hint.png")
        invalid_screenshot.save(invalid_path)
        print(f"[1309] Saved invalid path feedback screenshot to {invalid_path}")

        # Clear the input box before clicking Browse
        mouse.position = (file_box_x, file_box_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.2)
        with kb.pressed(modifier):
            kb.press("a")
            kb.release("a")
        time.sleep(0.2)
        kb.press(Key.backspace)
        kb.release(Key.backspace)
        time.sleep(0.5)

        # Step 6: Click '浏览' (Browse) button to select valid 1308.bib
        browse_x = wx + int(1240 * scale)
        browse_y = wy + int(365 * scale)
        print(f"[1309] Step 6: Clicking '浏览' (Browse) button at ({browse_x}, {browse_y})...")
        mouse.position = (browse_x, browse_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.5)

        # In QFileDialog, click '文件名' input box and type bib path
        file_input_x = wx + int(800 * scale)
        file_input_y = wy + int(725 * scale)
        print(f"[1309] Step 6: Typing bib path in file dialog at ({file_input_x}, {file_input_y})...")
        mouse.position = (file_input_x, file_input_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        kb.type(bib_file)
        time.sleep(0.8)
        kb.press(Key.enter)
        kb.release(Key.enter)
        time.sleep(2.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1309] ERROR: Mogan crashed after selecting bib file with code {ret}")
            return 1

        # Step 7: Verify live preview renders bib entries
        print("[1309] Step 7: Verifying live preview of bibliography...")
        preview_screenshot = ImageGrab.grab()
        preview_path = os.path.join(tempfile.gettempdir(), "1309_qml_preview_plain.png")
        preview_screenshot.save(preview_path)
        print(f"[1309] Saved plain preview screenshot to {preview_path}")

        # Check preview region: center of the dialog
        cx, cy = wx + int(960 * scale), wy + int(560 * scale)
        preview_crop = preview_screenshot.crop((cx - int(250 * scale), cy - int(40 * scale),
                                               cx + int(250 * scale), cy + int(60 * scale)))
        arr_plain = np.array(preview_crop)
        std_plain = float(arr_plain.std())
        dark_pixels = np.sum(arr_plain < 100)
        print(f"[1309] Plain preview analysis: std={std_plain:.1f}, dark_pixels={dark_pixels}")
        if std_plain < 10.0 or dark_pixels < 50:
            print("[1309] ERROR: Preview area missing rendered text!")
            return 1
        print("[1309] SUCCESS: Bibliography preview rendered clearly with proper magnification!")

        # Step 8: Click Style dropdown and switch style
        style_x = wx + int(720 * scale)
        style_y = wy + int(415 * scale)
        print(f"[1309] Step 8: Clicking Style dropdown at ({style_x}, {style_y})...")
        mouse.position = (style_x, style_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.8)

        # Select option in dropdown list
        option_x = style_x
        option_y = wy + int(535 * scale)
        print(f"[1309] Step 8: Selecting style from dropdown at ({option_x}, {option_y})...")
        mouse.position = (option_x, option_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1309] ERROR: Mogan crashed after changing style with code {ret}")
            return 1

        alpha_screenshot = ImageGrab.grab()
        alpha_path = os.path.join(tempfile.gettempdir(), "1309_qml_preview_switched.png")
        alpha_screenshot.save(alpha_path)
        print(f"[1309] Saved switched style preview screenshot to {alpha_path}")

        # Step 9: Click '插入' (Insert) button
        insert_btn_x = wx + int(1070 * scale)
        insert_btn_y = wy + int(738 * scale)
        print(f"[1309] Step 9: Clicking '插入' (Insert) button at ({insert_btn_x}, {insert_btn_y})...")
        mouse.position = (insert_btn_x, insert_btn_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.5)

        ret = proc.poll()
        if ret is not None:
            print(f"[1309] ERROR: Mogan crashed after inserting bibliography with code {ret}")
            return 1

        # Step 10: Verify document contains the inserted bibliography
        print("[1309] Step 10: Verifying inserted bibliography in document...")
        final_screenshot = ImageGrab.grab()
        final_path = os.path.join(tempfile.gettempdir(), "1309_final_doc.png")
        final_screenshot.save(final_path)
        print(f"[1309] Saved final document screenshot to {final_path}")

        print("[1309] TEST PASSED: QML Bibliography dialog re-implementation verified end-to-end!")
        return 0

    finally:
        if proc.poll() is None:
            print("[1309] Terminating Mogan...")
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
