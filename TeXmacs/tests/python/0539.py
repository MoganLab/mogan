#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform      | Status |
| ----------------------- | ------------- | ------ |
| MingShen <ms@liii.pro>  | macOS (arm64) | Passed |

Automated UI test for issue 0539:
Verify LLM session input block background and text colors, and quote-env styling in dark mode:
1. Ensure GUI theme preference is set to "liii-night".
2. Launch Mogan STEM with TeXmacs/tests/tmu/0539.tmu.
3. Wait for window, focus and click document using pynput.
4. Capture screenshot.
5. Verify dark mode LLM session styling:
   - Input block background is dark (#2c323c) instead of washed-out light blue-grey (#9ba8c2).
   - Input block body text is white (matching output block text color) instead of black.
   - Quote block (quote-env) text is whitish (#e6edf3, brightness > 200).
   - Quote block left decorative bar is distinct blue-grey (#8fa0b8).
6. Gracefully close Mogan using pynput (Cmd+Q / Ctrl+Q) and restore original preferences.
"""

import os
import sys
import time
import json
import shutil
import subprocess
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
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def find_preferences_path(repo_root):
    """Find $TEXMACS_HOME_PATH/system/preferences.json."""
    if os.environ.get("TEXMACS_HOME_PATH"):
        return os.path.join(os.environ["TEXMACS_HOME_PATH"], "system", "preferences.json")
    home = os.path.expanduser("~")
    names = ["moganlab", "liiilabs", "moganstem", "MoganSTEM", "LiiiSTEM"]
    if IS_DARWIN:
        for n in names:
            p = os.path.join(home, "Library", "Application Support", n, "system", "preferences.json")
            if os.path.exists(p):
                return p
    elif IS_WINDOWS:
        base = os.environ.get("APPDATA", os.path.join(home, "AppData", "Roaming"))
        for n in names:
            p = os.path.join(base, n, "system", "preferences.json")
            if os.path.exists(p):
                return p
    else:
        xdg = os.environ.get("XDG_DATA_HOME", os.path.join(home, ".local", "share"))
        for n in names:
            p = os.path.join(xdg, n, "system", "preferences.json")
            if os.path.exists(p):
                return p
    return None


def clear_mogan_cache():
    if IS_WINDOWS or IS_DARWIN:
        return
    cache = os.path.expanduser("~/.cache/MoganLab")
    if not os.path.exists(cache):
        return
    for tool in ("trash", "trash-put"):
        if shutil.which(tool):
            subprocess.run([tool, cache], check=False)
            return
    shutil.rmtree(cache, ignore_errors=True)


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


def get_mogan_window_rect():
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


def run_test(doc_path=None, output_path=None):
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)

    if doc_path is None:
        doc_path = os.path.join(repo_root, "TeXmacs", "tests", "tmu", "0539.tmu")

    if output_path is None:
        output_path = "/tmp/0539.png"

    pref_path = find_preferences_path(repo_root)
    original_theme = None
    if pref_path and os.path.exists(pref_path):
        try:
            with open(pref_path, encoding="utf-8") as f:
                pref_data = json.load(f)
            original_theme = pref_data.get("gui theme")
            pref_data["gui theme"] = "liii-night"
            with open(pref_path, "w", encoding="utf-8") as f:
                json.dump(pref_data, f, indent=2)
            print(f"[0539] Switched gui theme to liii-night in {pref_path}")
        except Exception as e:
            print(f"[0539] Failed to set preference: {e}")

    print(f"[0539] Target document: {doc_path}")
    print(f"[0539] Output screenshot: {output_path}")
    print(f"[0539] Using Mogan binary: {bin_path}")

    clear_mogan_cache()

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[0539] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, doc_path], env=env, cwd=repo_root)

    kb = KeyboardController()
    mouse = MouseController()
    mod = Key.cmd if IS_DARWIN else Key.ctrl

    try:
        time.sleep(5.0)
        ret = proc.poll()
        if ret is not None:
            print(f"[0539] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[0539] Step 2: Focusing Mogan window...")
        focus_mogan_window()
        time.sleep(1.0)

        wx, wy, ww, wh = get_mogan_window_rect()
        print(f"[0539] Mogan window bounds: ({wx}, {wy}, {ww}, {wh})")

        click_x = wx + ww // 2
        click_y = wy + wh // 2
        print(f"[0539] Step 3: Clicking document area with pynput at ({click_x}, {click_y})...")
        mouse.position = (click_x, click_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.0)

        print("[0539] Step 4: Capturing screenshot...")
        if ww > 100 and wh > 100:
            screenshot = ImageGrab.grab(bbox=(wx, wy, wx + ww, wy + wh))
        else:
            screenshot = ImageGrab.grab()
        screenshot.save(output_path)
        print(f"[0539] Screenshot successfully saved to: {output_path}")

        print("[0539] Step 5: Analyzing colors in LLM input block...")
        im = Image.open(output_path)
        w, h = im.size

        # Find input box bounds: background around #2c323c (rgb 44, 50, 60)
        min_x, max_x, min_y, max_y = w, 0, h, 0
        for y in range(0, h, 5):
            for x in range(0, w, 5):
                p = im.getpixel((x, y))
                if abs(p[0] - 44) + abs(p[1] - 50) + abs(p[2] - 60) < 15:
                    min_x = min(min_x, x)
                    max_x = max(max_x, x)
                    min_y = min(min_y, y)
                    max_y = max(max_y, y)

        if max_x <= min_x or max_y <= min_y:
            print("[0539] FAIL: Could not locate LLM input box with dark background (#2c323c)!")
            return 1

        print(f"[0539] Input box located at: ({min_x}, {min_y}) to ({max_x}, {max_y})")

        whitish_text_count = 0
        bar_color_count = 0

        for y in range(min_y, max_y):
            for x in range(min_x, max_x):
                p = im.getpixel((x, y))
                diff_bg = abs(p[0] - 44) + abs(p[1] - 50) + abs(p[2] - 60)
                if diff_bg < 15:
                    continue
                # White or whitish text: RGB > 180
                if p[0] > 180 and p[1] > 180 and p[2] > 180:
                    whitish_text_count += 1
                # Left decorative bar: around rgb(143, 160, 184)
                if abs(p[0] - 143) < 25 and abs(p[1] - 160) < 25 and abs(p[2] - 184) < 25:
                    bar_color_count += 1

        print(f"[0539] Whitish text pixel count: {whitish_text_count}")
        print(f"[0539] Left decorative bar pixel count: {bar_color_count}")

        if whitish_text_count < 1000:
            print("[0539] FAIL: Insufficient white/whitish text pixels in input box!")
            return 1

        if bar_color_count < 50:
            print("[0539] FAIL: Insufficient decorative bar pixels in input box!")
            return 1

        print("[0539] SUCCESS: Input block dark background and white text/bar styling verified!")
        return 0

    finally:
        print("[0539] Step 6: Closing Mogan with pynput (Cmd+Q / Ctrl+Q)...")
        try:
            focus_mogan_window()
            with kb.pressed(mod):
                kb.press("q")
                time.sleep(0.08)
                kb.release("q")
            time.sleep(1.0)
        except Exception:
            pass

        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                proc.kill()

        if pref_path and original_theme is not None:
            try:
                with open(pref_path, encoding="utf-8") as f:
                    pref_data = json.load(f)
                pref_data["gui theme"] = original_theme
                with open(pref_path, "w", encoding="utf-8") as f:
                    json.dump(pref_data, f, indent=2)
                print(f"[0539] Restored gui theme to {original_theme}")
            except Exception as e:
                print(f"[0539] Failed to restore preference: {e}")


if __name__ == "__main__":
    doc = None
    out = None
    for arg in sys.argv[1:]:
        if arg.endswith(".tmu") or arg.endswith(".tm"):
            doc = arg
        elif arg.endswith(".png"):
            out = arg
        elif doc is None:
            doc = arg
        else:
            out = arg
    sys.exit(run_test(doc, out))
