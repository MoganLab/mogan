#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Windows     | Passed |

Automated end-to-end UI test for issue 1260:
After opening Help -> Welcome, then Help -> User manual -> Getting started,
extra gaps show up both above the document canvas (between the focus toolbar
and the canvas) and below it (between the canvas and the status bar).

Steps:
1. Launch Mogan, wait for the window, maximize it and force foreground.
2. Ctrl+T to create a new tab; capture a reference screenshot and measure
   the document page span (first/last pure-white rows in the central strip).
3. Open Help -> Welcome, then Help -> User manual -> Getting started.
4. Measure the page span again:
   top gap = final_top - ref_top (toolbar <-> canvas),
   bottom gap = ref_bottom - final_bottom (canvas <-> status bar).
5. FAIL when either exceeds a few pixels (theme-independent, scale-aware).

Menu clicks rely on OCR (RapidOCR) restricted to the Mogan window crop, so
text from other windows (terminals, editors) can never be clicked.
"""

import os
import sys
import time
import tempfile
import subprocess
import numpy as np

# Ensure UTF-8 and unbuffered stdout
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(line_buffering=True, encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(line_buffering=True, encoding="utf-8")

# DPI awareness as early as possible so coordinates and grabs align
if sys.platform == "win32":
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(2)  # PER_MONITOR_DPI_AWARE
    except Exception:
        try:
            import ctypes
            ctypes.windll.user32.SetProcessDPIAware()
        except Exception:
            pass

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

IS_WINDOWS = sys.platform == "win32"


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
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


# ---------------------------------------------------------------------------
# Windows window helpers (hwnd lookup, maximize, foreground, geometry)
# ---------------------------------------------------------------------------

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
    """Steal focus from the console so pynput keys reach STEM."""
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
    # Alt tap lets a background process call SetForegroundWindow.
    user32.keybd_event(0x12, 0, 0, 0)
    user32.SetForegroundWindow(hwnd)
    user32.keybd_event(0x12, 0, 2, 0)


def wait_for_mogan_window(proc, timeout=25.0):
    """Wait for the STEM window; maximize it and return (hwnd, rect)."""
    end = time.time() + timeout
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError(
                f"[1260] Mogan exited immediately (code {proc.returncode})."
            )
        if IS_WINDOWS:
            titled = [(h, t) for h, t in _windows_hwnds_for_pid(proc.pid) if t]
            if titled:
                hwnd = titled[0][0]
                import ctypes
                from ctypes import wintypes

                user32 = ctypes.windll.user32
                _windows_force_foreground(hwnd)
                time.sleep(0.3)
                # Maximize after foreground: _windows_force_foreground restores,
                # which would undo a maximize done earlier.
                user32.ShowWindow(hwnd, 3)  # SW_MAXIMIZE
                time.sleep(0.5)
                rect = wintypes.RECT()
                user32.GetWindowRect(hwnd, ctypes.byref(rect))
                print(f"[1260] Window ready: {titled[0][1]!r}, "
                      f"rect=({rect.left},{rect.top})-({rect.right},{rect.bottom}) (maximized)",
                      flush=True)
                return hwnd, (rect.left, rect.top, rect.right, rect.bottom)
        time.sleep(0.5)
    raise RuntimeError("[1260] Mogan window did not appear in time.")


def window_rect(hwnd):
    import ctypes
    from ctypes import wintypes

    rect = wintypes.RECT()
    ctypes.windll.user32.GetWindowRect(hwnd, ctypes.byref(rect))
    return (rect.left, rect.top, rect.right, rect.bottom)


def grab_window(hwnd):
    rect = window_rect(hwnd) if IS_WINDOWS else None
    img = ImageGrab.grab(bbox=rect) if rect else ImageGrab.grab()
    return img


# ---------------------------------------------------------------------------
# OCR-driven clicking, restricted to the Mogan window crop
# ---------------------------------------------------------------------------

def ocr_find(ocr, img, text, y_max_frac=1.0):
    """Best match of `text` inside a PIL image; returns (cx, cy) image coords."""
    result, _ = ocr(np.array(img))
    if not result:
        return None
    w, h = img.size
    y_limit = int(h * y_max_frac)
    matches = []
    for box, detected, score in result:
        if text not in detected:
            continue
        xs = [p[0] for p in box]
        ys = [p[1] for p in box]
        cy = int(sum(ys) / 4)
        if cy > y_limit:
            continue
        matches.append((int(sum(xs) / 4), cy, detected, float(score)))
    if not matches:
        return None
    # Menu entries sit above canvas text for the same keyword.
    matches.sort(key=lambda m: m[1])
    cx, cy, det, score = matches[0]
    print(f"[1260]   found '{text}' as '{det}' at ({cx}, {cy}) score={score:.2f}", flush=True)
    return cx, cy


def ocr_click(ocr, hwnd, text, y_max_frac=1.0, hover=False, wait_after=1.2, retry=5):
    mouse = MouseController()
    for attempt in range(retry):
        img = grab_window(hwnd)
        match = ocr_find(ocr, img, text, y_max_frac)
        if match:
            cx, cy = match
            l, t, r, b = window_rect(hwnd) if IS_WINDOWS else (0, 0, 0, 0)
            sx, sy = l + cx, t + cy
            mouse.position = (sx, sy)
            time.sleep(0.3)
            if hover:
                time.sleep(0.8)
            else:
                mouse.click(Button.left)
            time.sleep(wait_after)
            return True
        time.sleep(0.5)
    print(f"[1260] ERROR: could not locate '{text}' via OCR.", flush=True)
    return False


# ---------------------------------------------------------------------------
# Gap measurement
# ---------------------------------------------------------------------------

def page_span_y(img):
    """(top, bottom) of the white document page in the central strip.

    Toolbar/status-bar background is light gray (~243) and the stray gap band
    is darker gray (~160); the page is pure white (>= 250). Returns (-1, -1)
    when no white page row is found.
    """
    rgb = img.convert("RGB")
    w, h = rgb.size
    arr = np.array(rgb)
    strip = arr[:, int(w * 0.35): int(w * 0.65)]
    start = int(h * 0.06)
    rows = [y for y in range(start, h)
            if (strip[y] >= 250).all(axis=1).mean() > 0.9]
    if not rows:
        return -1, -1
    return rows[0], rows[-1]


def dpi_scale():
    if not IS_WINDOWS:
        return 1.0
    import ctypes
    try:
        return ctypes.windll.user32.GetDpiForSystem() / 96.0
    except Exception:
        return 1.0


# ---------------------------------------------------------------------------
# Main flow
# ---------------------------------------------------------------------------

def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1260] Using Mogan binary: {bin_path}", flush=True)

    if not IS_WINDOWS:
        print("[1260] NOTE: this repro targets Windows; continuing best-effort.", flush=True)

    try:
        from rapidocr_onnxruntime import RapidOCR
    except ImportError:
        print("[1260] ERROR: RapidOCR is required: pip install rapidocr_onnxruntime", flush=True)
        return 2

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    shots = tempfile.gettempdir()
    proc = subprocess.Popen([bin_path], env=env, cwd=repo_root)
    keyboard = KeyboardController()

    try:
        hwnd, _ = wait_for_mogan_window(proc)
        time.sleep(1.5)

        # Step 1: new tab as the reference state
        print("[1260] Step 1: Ctrl+T new tab", flush=True)
        with keyboard.pressed(Key.ctrl):
            keyboard.press("t")
            keyboard.release("t")
        time.sleep(2.5)

        ref_img = grab_window(hwnd)
        ref_path = os.path.join(shots, "1260_ref_new_tab.png")
        ref_img.save(ref_path)
        ref_top, ref_bottom = page_span_y(ref_img)
        print(f"[1260] Reference page span y = ({ref_top}, {ref_bottom}) ({ref_path})", flush=True)

        ocr = RapidOCR()
        menu_band = 0.12  # menu bar occupies the top band of the window

        # Step 2: Help -> Welcome
        print("[1260] Step 2: Help -> Welcome", flush=True)
        if not ocr_click(ocr, hwnd, "帮助", y_max_frac=menu_band):
            return 1
        if not ocr_click(ocr, hwnd, "欢迎"):
            return 1
        time.sleep(2.5)
        grab_window(hwnd).save(os.path.join(shots, "1260_welcome.png"))

        # Step 3: Help -> User manual -> Getting started
        print("[1260] Step 3: Help -> User manual -> Getting started", flush=True)
        if not ocr_click(ocr, hwnd, "帮助", y_max_frac=menu_band):
            return 1
        if not ocr_click(ocr, hwnd, "用户手册", hover=True):
            return 1
        time.sleep(1.0)
        grab_window(hwnd).save(os.path.join(shots, "1260_manual_submenu.png"))
        if not ocr_click(ocr, hwnd, "开始使用"):
            return 1
        time.sleep(3.0)

        # Step 4: measure the gaps above and below the canvas
        final_img = grab_window(hwnd)
        final_path = os.path.join(shots, "1260_final.png")
        final_img.save(final_path)
        final_top, final_bottom = page_span_y(final_img)
        print(f"[1260] Final page span y = ({final_top}, {final_bottom}) ({final_path})", flush=True)

        if ref_top < 0 or final_top < 0:
            print("[1260] ERROR: could not locate the document page in screenshots.", flush=True)
            return 1

        top_gap = final_top - ref_top
        bottom_gap = ref_bottom - final_bottom
        threshold = int(max(6, 4 * dpi_scale()))
        print(f"[1260] Top gap = {top_gap}px, bottom gap = {bottom_gap}px "
              f"(threshold {threshold}px, scale {dpi_scale():.1f})", flush=True)

        # Zoomed crops of both transitions for human review
        w = final_img.size[0]

        def save_zoom(y_center, name):
            y0 = max(0, y_center - 160)
            crop = final_img.crop((int(w * 0.2), y0, int(w * 0.8), y_center + 120))
            crop = crop.resize((crop.width * 2, crop.height * 2), Image.Resampling.NEAREST)
            crop.save(os.path.join(shots, name))

        save_zoom(final_top, "1260_gap_top_zoom.png")
        save_zoom(final_bottom, "1260_gap_bottom_zoom.png")

        if top_gap > threshold or bottom_gap > threshold:
            print(f"[1260] TEST FAILED: extra gap toolbar<->canvas {top_gap}px, "
                  f"canvas<->status-bar {bottom_gap}px. See "
                  f"{os.path.join(shots, '1260_gap_top_zoom.png')}", flush=True)
            return 1
        print("[1260] TEST PASSED: no extra gap around the canvas.", flush=True)
        return 0

    finally:
        print("[1260] Terminating Mogan...", flush=True)
        if IS_WINDOWS:
            subprocess.run(["taskkill", "/PID", str(proc.pid), "/T", "/F"],
                           capture_output=True)
        else:
            proc.terminate()
        try:
            proc.wait(timeout=5)
        except Exception:
            pass


if __name__ == "__main__":
    sys.exit(run_test())
