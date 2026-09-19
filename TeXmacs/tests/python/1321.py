#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform        | Status |
| ----------------------- | --------------- | ------ |
| Darcy Shen <da@liii.pro>| Windows / Linux | Passed |

Automated end-to-end UI test for issue 1321:
When exporting PDF from a document using dark mode (has-style-package? "dark"),
the Export as PDF dialog must offer an option:
"在导出的PDF中，仍旧采用深色模式" (Keep dark mode in exported PDF).

Steps:
1. Ensure the "gui theme" preference is "liii-night" so the document is loaded in dark mode.
2. Launch Mogan STEM with TeXmacs/tests/tmu/1321.tmu.
3. Automatically raise and focus the Mogan window to the foreground.
4. Open "文件" (File) menu -> "导出为PDF..." (Export as PDF...).
5. Verify the Export as PDF dialog appears and contains the option:
   "在导出的PDF中，仍旧采用深色模式" (or "深色模式").
6. Click the toggle to test user interaction.
7. Dismiss the dialog with "取消" (Cancel) or Esc.
8. Exit Mogan STEM cleanly and restore preferences.
"""

import os
import sys
import time
import json
import subprocess
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

try:
    from PIL import ImageGrab, Image
except ImportError:
    Image = None
    ImageGrab = None

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

_OCR = None


def engine():
    global _OCR
    if _OCR is None:
        try:
            from rapidocr_onnxruntime import RapidOCR
            _OCR = RapidOCR()
        except ImportError:
            try:
                from rapidocr import RapidOCR
                _OCR = RapidOCR()
            except ImportError:
                return None
    return _OCR


def ocr_lines(img):
    """Returns [(cx, cy, txt), ...] for a PIL Image; [] if OCR unavailable."""
    ocr = engine()
    if ocr is None or img is None:
        return []
    res = ocr(np.array(img.convert("RGB")))
    if isinstance(res, tuple):
        res = res[0]
    if not res:
        return []
    out = []
    for item in res:
        if isinstance(item, (list, tuple)) and len(item) >= 2:
            box, txt = item[0], item[1]
            xs = [float(p[0]) for p in box]
            ys = [float(p[1]) for p in box]
            cx = int(sum(xs) / len(xs))
            cy = int(sum(ys) / len(ys))
            out.append((cx, cy, str(txt)))
    return out


def find_text(lines, text):
    """Returns (cx, cy) of the first line containing `text`."""
    for cx, cy, txt in lines:
        if text in txt:
            return cx, cy
    return None


def find_repo_root():
    cur = os.path.abspath(os.path.dirname(__file__))
    while cur != "/" and cur != os.path.dirname(cur):
        if os.path.exists(os.path.join(cur, "TeXmacs")) and os.path.exists(os.path.join(cur, "src")):
            return cur
        cur = os.path.dirname(cur)
    return os.path.abspath(".")


def find_mogan_binary(repo_root):
    candidates = [
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def find_preferences_path(repo_root):
    home = os.path.expanduser("~")
    names = ["moganlab", "MoganSTEM", "LiiiSTEM"]
    if IS_DARWIN:
        roots = [os.path.join(home, "Library", "Application Support", n) for n in names]
    elif IS_WINDOWS:
        base = os.environ.get("APPDATA", os.path.join(home, "AppData", "Roaming"))
        roots = [os.path.join(base, n) for n in names]
    else:
        xdg = os.environ.get("XDG_DATA_HOME", os.path.join(home, ".local", "share"))
        roots = [os.path.join(xdg, n) for n in names]
    for r in roots:
        p = os.path.join(r, "system", "preferences.json")
        if os.path.exists(p):
            return p
    default_root = roots[0]
    return os.path.join(default_root, "system", "preferences.json")


def ensure_dark_theme(repo_root):
    p = find_preferences_path(repo_root)
    if not p:
        return None
    try:
        with open(p, encoding="utf-8") as f:
            prefs = json.load(f)
    except Exception:
        prefs = {}
    orig_theme = prefs.get("gui theme", "default")
    if orig_theme != "liii-night":
        prefs["gui theme"] = "liii-night"
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "w", encoding="utf-8") as f:
            json.dump(prefs, f, ensure_ascii=False, indent=2)
        print(f"[1321] Set gui theme=liii-night in {p} (was {orig_theme})", flush=True)
    else:
        print(f"[1321] gui theme already liii-night in {p}", flush=True)
    return orig_theme


def restore_theme(repo_root, orig_theme):
    if orig_theme is None:
        return
    p = find_preferences_path(repo_root)
    if not p or not os.path.exists(p):
        return
    try:
        with open(p, encoding="utf-8") as f:
            prefs = json.load(f)
    except Exception:
        prefs = {}
    if prefs.get("gui theme") != orig_theme:
        prefs["gui theme"] = orig_theme
        with open(p, "w", encoding="utf-8") as f:
            json.dump(prefs, f, ensure_ascii=False, indent=2)
        print(f"[1321] Restored gui theme={orig_theme} in {p}", flush=True)


def _windows_hwnds_for_pid(pid):
    import ctypes
    from ctypes import wintypes

    user32 = ctypes.windll.user32
    EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
    found = []

    def callback(hwnd, _lparam):
        if not user32.IsWindowVisible(hwnd):
            return True
        proc_id = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(proc_id))
        if proc_id.value != pid:
            return True
        n = user32.GetWindowTextLengthW(hwnd)
        buf = ctypes.create_unicode_buffer(n + 1)
        if n > 0:
            user32.GetWindowTextW(hwnd, buf, n + 1)
        found.append((hwnd, buf.value if n > 0 else ""))
        return True

    user32.EnumWindows(EnumWindowsProc(callback), 0)
    return found


def _windows_force_foreground(hwnd):
    import ctypes
    from ctypes import wintypes

    user32 = ctypes.windll.user32
    kernel32 = ctypes.windll.kernel32
    user32.ShowWindow(hwnd, 9)  # SW_RESTORE
    fg = user32.GetForegroundWindow()
    pid = wintypes.DWORD()
    tid_fg = user32.GetWindowThreadProcessId(fg, ctypes.byref(pid))
    tid_self = kernel32.GetCurrentThreadId()
    user32.AttachThreadInput(tid_self, tid_fg, True)
    user32.BringWindowToTop(hwnd)
    user32.SetForegroundWindow(hwnd)
    user32.AttachThreadInput(tid_self, tid_fg, False)
    user32.keybd_event(0x12, 0, 0, 0)
    user32.SetForegroundWindow(hwnd)
    user32.keybd_event(0x12, 0, 2, 0)


def wait_for_and_focus_window(proc, timeout=25.0):
    """Wait for the STEM window, bring to foreground, and return (x1, y1, x2, y2)."""
    end = time.time() + timeout
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError(f"[1321] Mogan exited prematurely (code {proc.returncode}).")
        if IS_WINDOWS:
            titled = [(h, t) for h, t in _windows_hwnds_for_pid(proc.pid) if t]
            if titled:
                hwnd = titled[0][0]
                import ctypes
                from ctypes import wintypes

                user32 = ctypes.windll.user32
                _windows_force_foreground(hwnd)
                time.sleep(0.3)
                rect = wintypes.RECT()
                user32.GetWindowRect(hwnd, ctypes.byref(rect))
                print(f"[1321] Window ready: {titled[0][1]!r}, "
                      f"rect=({rect.left},{rect.top})-({rect.right},{rect.bottom})", flush=True)
                return (rect.left, rect.top, rect.right, rect.bottom)
        elif IS_DARWIN:
            try:
                import AppKit
                for app in AppKit.NSWorkspace.sharedWorkspace().runningApplications():
                    if "Mogan" in (app.localizedName() or ""):
                        app.activateWithOptions_(AppKit.NSApplicationActivateIgnoringOtherApps)
                        break
            except Exception:
                pass
            img = ImageGrab.grab()
            return (0, 0, img.size[0], img.size[1])
        else:
            # Linux
            try:
                out = subprocess.run(["wmctrl", "-lGp"], capture_output=True, text=True, timeout=5).stdout
                for line in out.splitlines():
                    parts = line.split(None, 7)
                    if len(parts) >= 6 and parts[2] == str(proc.pid):
                        x, y, w, h = (int(v) for v in parts[3:7])
                        subprocess.run(["wmctrl", "-ia", parts[0]], timeout=2)
                        return (x, y, x + w, y + h)
            except Exception:
                pass
            img = ImageGrab.grab()
            return (0, 0, img.size[0], img.size[1])
        time.sleep(0.5)
    img = ImageGrab.grab()
    return (0, 0, img.size[0], img.size[1])


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    tmu_path = os.path.join(repo_root, "TeXmacs", "tests", "tmu", "1321.tmu")
    print(f"[1321] Using binary: {bin_path}")
    print(f"[1321] Using document: {tmu_path}")

    orig_theme = ensure_dark_theme(repo_root)

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1321] Step 1: Launching Mogan STEM with dark mode document 1321.tmu...")
    proc = subprocess.Popen([bin_path, "-d", tmu_path], env=env, cwd=repo_root)

    kb = KeyboardController()
    mouse = MouseController()
    mod_key = Key.cmd if IS_DARWIN else Key.ctrl

    try:
        time.sleep(3.0)
        ret = proc.poll()
        if ret is not None:
            print(f"[1321] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        print("[1321] Waiting for window and bringing to foreground...")
        win_rect = wait_for_and_focus_window(proc)
        wx1, wy1, wx2, wy2 = win_rect
        ww = wx2 - wx1
        wh = wy2 - wy1
        scale = ww / 1920.0
        print(f"[1321] Focused Mogan window: ({wx1}, {wy1}, {wx2}, {wy2})")

        # Step 2: Open "文件" (File) menu
        print("[1321] Step 2: Locating '文件' (File) menu...")
        file_pos = None
        for attempt in range(25):
            img = ImageGrab.grab(bbox=win_rect)
            lines = ocr_lines(img)
            # Find '文件' in top bar (small y)
            for cx, cy, txt in lines:
                if ("文件" in txt or "File" in txt) and cy < 150:
                    file_pos = (wx1 + cx, wy1 + cy)
                    break
            if file_pos:
                print(f"[1321] Found '文件' via OCR at {file_pos} (attempt {attempt + 1})")
                break
            time.sleep(0.5)

        if not file_pos:
            file_pos = (wx1 + int(45 * scale), wy1 + int(45 * scale))
            print(f"[1321] OCR did not find '文件', using fallback pos {file_pos}")

        mouse.position = file_pos
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.0)

        # Step 3: Click "导出为PDF..." (Export as PDF...)
        print("[1321] Step 3: Locating '导出为PDF...' in menu...")
        export_pos = None
        for attempt in range(10):
            img = ImageGrab.grab(bbox=win_rect)
            lines = ocr_lines(img)
            for cx, cy, txt in lines:
                if "导出为PDF" in txt or "Export as PDF" in txt:
                    export_pos = (wx1 + cx, wy1 + cy)
                    break
            if export_pos:
                print(f"[1321] Found '导出为PDF' via OCR at {export_pos} (attempt {attempt + 1})")
                break
            time.sleep(0.5)

        if not export_pos:
            export_pos = (wx1 + int(50 * scale), wy1 + int(322 * scale))
            print(f"[1321] OCR did not find '导出为PDF', using fallback pos {export_pos}")

        mouse.position = export_pos
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.0)

        # Step 4: Verify the Export as PDF dialog has the dark mode option
        print("[1321] Step 4: Checking Export as PDF dialog options...")
        dark_opt_pos = None
        dialog_lines = []
        for attempt in range(10):
            img = ImageGrab.grab(bbox=win_rect)
            dialog_lines = ocr_lines(img)
            for cx, cy, txt in dialog_lines:
                if "深色模式" in txt or "dark mode" in txt or "Keep dark" in txt:
                    dark_opt_pos = (wx1 + cx, wy1 + cy)
                    break
            if dark_opt_pos:
                break
            time.sleep(0.5)

        print(f"[1321] OCR detected {len(dialog_lines)} text entries inside Mogan window.")
        for cx, cy, txt in dialog_lines:
            if any(k in txt for k in ["PDF", "导出", "模式", "附件"]):
                print(f"  dialog entry: ({wx1 + cx}, {wy1 + cy}): {txt}")

        if not dark_opt_pos:
            print("[1321] ERROR: '在导出的PDF中，仍旧采用深色模式' option NOT found in Export as PDF dialog!")
            return 2

        print(f"[1321] SUCCESS: Found dark mode option at {dark_opt_pos}!")

        # Step 5: Toggle the dark mode switch
        print(f"[1321] Step 5: Clicking dark mode toggle at {dark_opt_pos}...")
        mouse.position = dark_opt_pos
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        # Step 6: Dismiss the dialog via '取消' (Cancel)
        print("[1321] Step 6: Dismissing dialog with '取消' (Cancel)...")
        cancel_pos = None
        for cx, cy, txt in dialog_lines:
            if "取消" in txt or "Cancel" in txt:
                cancel_pos = (wx1 + cx, wy1 + cy)
                break

        if cancel_pos:
            print(f"[1321] Clicking '取消' at {cancel_pos}...")
            mouse.position = cancel_pos
            time.sleep(0.3)
            mouse.click(Button.left)
            time.sleep(0.8)
        else:
            print("[1321] Pressing Esc to dismiss dialog...")
            kb.press(Key.esc)
            kb.release(Key.esc)
            time.sleep(0.8)

        # Step 7: Close document and window cleanly
        print("[1321] Step 7: Closing document and exiting Mogan...")
        if IS_WINDOWS:
            _windows_force_foreground(proc.pid)
        with kb.pressed(mod_key):
            kb.press('q')
            kb.release('q')
        time.sleep(1.5)

        print("[1321] TEST PASSED!")
        return 0

    finally:
        if proc.poll() is None:
            try:
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
        restore_theme(repo_root, orig_theme)


if __name__ == "__main__":
    sys.exit(run_test())
