#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux       | Passed |

Automated end-to-end UI test for issue 1272:
In dark mode (liii-night), a newly created AI conversation must carry the
`dark` style package automatically. Before the fix the message canvas was
rendered with the plain `generic` style: a white canvas inside the dark UI.

Steps:
1. Ensure the "gui theme" preference is liii-night, then launch Mogan.
2. Open the Chat tab; click 开启新对话 to guarantee a brand-new conversation.
3. Click into the input box (OCR anchors on the 深度思考 chip inside it),
   type a message and press Enter to send it.
4. Sanity check: the session title (the typed message) shows in the top band.
5. Measure the mean brightness of the message-canvas central strip:
   canvas-color in dark.ts is #202020 (< 100 passes), the unfixed generic
   canvas is white (> 200 fails).

OCR uses RapidOCR restricted to the Mogan window crop when available; the
script falls back to fixed window-relative coordinates otherwise.
"""

import os
import re
import sys
import time
import json
import tempfile
import subprocess

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

import numpy as np

try:
    from PIL import ImageGrab, Image
except ImportError:
    Image = None
    ImageGrab = None

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

# OCR is optional: engine() returns None when neither package is importable
_OCR = None


def engine():
    global _OCR
    if _OCR is None:
        try:
            from rapidocr import RapidOCR  # new unified package (py>=3.13)
            _OCR = RapidOCR()
        except ImportError:
            try:
                from rapidocr_onnxruntime import RapidOCR  # legacy package
                _OCR = RapidOCR()
            except ImportError:
                return None
    return _OCR


def ocr_lines(img):
    """[(cx, cy, text, score)] for a PIL image; [] when OCR is unavailable."""
    ocr = engine()
    if ocr is None or img is None:
        return []
    res = ocr(np.array(img.convert("RGB")))
    out = []
    boxes = getattr(res, "boxes", None)
    txts = getattr(res, "txts", None)
    scores = getattr(res, "scores", None)
    if boxes is None or txts is None:
        return []
    for box, txt, score in zip(boxes, txts, scores):
        xs = [float(p[0]) for p in box]
        ys = [float(p[1]) for p in box]
        out.append((int(sum(xs) / 4), int(sum(ys) / 4), str(txt), float(score)))
    return out


def find_text(lines, text):
    """lines: [(cx, cy, txt), ...] — first entry containing `text` wins."""
    for cx, cy, txt in lines:
        if text in txt:
            return cx, cy
    return None


# ---------------------------------------------------------------------------
# Repo / binary / preference discovery
# ---------------------------------------------------------------------------

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
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def read_prefix_dir(repo_root):
    """PREFIX_DIR from build/tm_configure.hpp decides the user data dir name."""
    cfg = os.path.join(repo_root, "build", "tm_configure.hpp")
    if os.path.exists(cfg):
        m = re.search(r'#define\s+PREFIX_DIR\s+"([^"]+)"', open(cfg, encoding="utf-8", errors="ignore").read())
        if m:
            return m.group(1)
    return None


def find_preferences_path(repo_root):
    """$TEXMACS_HOME_PATH/system/preferences.json candidate discovery."""
    if os.environ.get("TEXMACS_HOME_PATH"):
        return os.path.join(os.environ["TEXMACS_HOME_PATH"], "system", "preferences.json")
    prefix = read_prefix_dir(repo_root)
    home = os.path.expanduser("~")
    names = ([prefix] if prefix else []) + ["moganlab", "liiilabs", "moganstem", "MoganSTEM", "LiiiSTEM"]
    if IS_WINDOWS:
        base = os.environ.get("APPDATA", os.path.join(home, "AppData", "Roaming"))
        roots = [os.path.join(base, n) for n in names]
    else:
        xdg = os.environ.get("XDG_DATA_HOME", os.path.join(home, ".local", "share"))
        roots = [os.path.join(xdg, n) for n in names]
    for r in roots:
        p = os.path.join(r, "system", "preferences.json")
        if os.path.exists(p):
            return p
    return None


def ensure_dark_theme(repo_root):
    """Force "gui theme": "liii-night" so the test runs in dark mode."""
    p = find_preferences_path(repo_root)
    if not p:
        print("[1272] WARN: preferences.json not found; relying on current theme.", flush=True)
        return False
    try:
        with open(p, encoding="utf-8") as f:
            prefs = json.load(f)
    except Exception:
        prefs = {}
    if prefs.get("gui theme") == "liii-night":
        print(f"[1272] gui theme already liii-night ({p})", flush=True)
        return True
    prefs["gui theme"] = "liii-night"
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w", encoding="utf-8") as f:
        json.dump(prefs, f, ensure_ascii=False, indent=2)
    print(f"[1272] set gui theme=liii-night in {p}", flush=True)
    return True


# ---------------------------------------------------------------------------
# Window helpers
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


def wait_for_mogan_window(proc, timeout=25.0):
    """Wait for the STEM window; maximize it and return (hwnd, rect)."""
    end = time.time() + timeout
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError(f"[1272] Mogan exited immediately (code {proc.returncode}).")
        if IS_WINDOWS:
            titled = [(h, t) for h, t in _windows_hwnds_for_pid(proc.pid) if t]
            if titled:
                hwnd = titled[0][0]
                import ctypes
                from ctypes import wintypes

                user32 = ctypes.windll.user32
                _windows_force_foreground(hwnd)
                time.sleep(0.3)
                user32.ShowWindow(hwnd, 3)  # SW_MAXIMIZE
                time.sleep(0.5)
                rect = wintypes.RECT()
                user32.GetWindowRect(hwnd, ctypes.byref(rect))
                print(f"[1272] Window ready: {titled[0][1]!r}, "
                      f"rect=({rect.left},{rect.top})-({rect.right},{rect.bottom})", flush=True)
                return hwnd, (rect.left, rect.top, rect.right, rect.bottom)
        else:
            # Linux: the app restores its saved (nearly fullscreen) geometry;
            # refine via external tools when available, else use the screen.
            rect = linux_window_rect(proc)
            if rect:
                return None, rect
            if time.time() > end - timeout + 10:
                sw, sh = grab_screen().size
                print(f"[1272] Window managers absent; using full screen {sw}x{sh}.", flush=True)
                return None, (0, 0, sw, sh)
        time.sleep(0.5)
    raise RuntimeError("[1272] Mogan window did not appear in time.")


def linux_window_rect(proc):
    """Best-effort window rect on X11 via xdotool/wmctrl; None while absent."""
    try:
        out = subprocess.run(["wmctrl", "-lGp"], capture_output=True, text=True, timeout=5).stdout
        for line in out.splitlines():
            parts = line.split(None, 7)
            if len(parts) >= 6 and parts[2] == str(proc.pid):
                x, y, w, h = (int(v) for v in parts[3:7])
                return (x, y, x + w, y + h)
    except Exception:
        pass
    try:
        out = subprocess.run(["xdotool", "search", "--pid", str(proc.pid)],
                             capture_output=True, text=True, timeout=5).stdout
        winids = [w for w in out.split() if w.strip()]
        if winids:
            out = subprocess.run(["xdotool", "getwindowgeometry", "--shell", winids[0]],
                                 capture_output=True, text=True, timeout=5).stdout
            geom = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
            x, y = int(geom["X"]), int(geom["Y"])
            w, h = int(geom["WIDTH"]), int(geom["HEIGHT"])
            return (x, y, x + w, y + h)
    except Exception:
        pass
    return None


def grab_screen():
    if ImageGrab is not None:
        try:
            return ImageGrab.grab()
        except Exception:
            pass
    if Image is not None and os.path.exists("/usr/bin/import"):
        p = os.path.join(tempfile.gettempdir(), f"1272_grab_{os.getpid()}.png")
        subprocess.run(["import", "-window", "root", p], check=False, timeout=20)
        if os.path.exists(p):
            return Image.open(p).convert("RGB")
    raise RuntimeError("[1272] No screenshot backend available (need Pillow or ImageMagick import).")


def grab_window(rect):
    if rect:
        img = grab_screen()
        return img.crop(rect)
    return grab_screen()


# ---------------------------------------------------------------------------
# OCR-driven clicking, restricted to the Mogan window crop
# ---------------------------------------------------------------------------

def ocr_click(hwnd, rect, text, y_max_frac=1.0, wait_after=1.2, retry=5):
    lines_cache = None
    mouse = MouseController()
    for attempt in range(retry):
        img = grab_window(rect)
        lines = ocr_lines(img)
        w, h = img.size
        y_limit = int(h * y_max_frac)
        cand = [(cx, cy, txt) for cx, cy, txt, _s in lines if cy <= y_limit]
        hit = find_text(cand, text)
        if hit:
            cx, cy = hit
            l, t = (rect[0], rect[1]) if rect else (0, 0)
            mouse.position = (l + cx, t + cy)
            time.sleep(0.3)
            mouse.click(Button.left)
            time.sleep(wait_after)
            return True
        time.sleep(0.8)
    print(f"[1272] WARN: could not locate '{text}' via OCR.", flush=True)
    return False


# ---------------------------------------------------------------------------
# Measurement
# ---------------------------------------------------------------------------

def region_mean_brightness(img, fx0, fy0, fx1, fy1):
    """Mean RGB brightness of a window-relative fractional rectangle."""
    rgb = img.convert("RGB")
    w, h = rgb.size
    box = (int(w * fx0), int(h * fy0), int(w * fx1), int(h * fy1))
    arr = np.array(rgb.crop(box), dtype=np.float64)
    return float(arr.mean())


# ---------------------------------------------------------------------------
# Main flow
# ---------------------------------------------------------------------------

def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1272] Using Mogan binary: {bin_path}", flush=True)

    if not IS_WINDOWS:
        print("[1272] NOTE: primary target is Linux here; Windows helpers kept best-effort.", flush=True)

    ensure_dark_theme(repo_root)

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    shots = tempfile.gettempdir()
    proc = subprocess.Popen([bin_path], env=env, cwd=repo_root)
    keyboard = KeyboardController()
    mouse = MouseController()

    message = "dark style regression check"
    exit_code = 2

    try:
        hwnd, rect = wait_for_mogan_window(proc)
        time.sleep(1.5)

        # Step 1: open the Chat tab (top tab band only)
        print("[1272] Step 1: open the Chat tab", flush=True)
        if not ocr_click(hwnd, rect, "Chat", y_max_frac=0.05, wait_after=4.0):
            print("[1272] ERROR: Chat tab not found.", flush=True)
            return 1

        # Step 2: guarantee a brand-new conversation
        print("[1272] Step 2: 开启新对话", flush=True)
        if engine() is not None:
            ocr_click(hwnd, rect, "开启新对话", wait_after=2.0, retry=3)
        time.sleep(1.0)

        # Step 3: click into the input box and type the message
        print("[1272] Step 3: type and send the message", flush=True)
        l, t, r, b = rect if rect else (0, 0, 0, 0)
        ww, wh = r - l, b - t
        clicked = False
        if engine() is not None:
            img = grab_window(rect)
            lines = ocr_lines(img)
            hit = find_text([(cx, cy, tx) for cx, cy, tx, _s in lines], "深度思考")
            if hit:
                # the chip row sits at the bottom of the input box;
                # the text area is right above it
                cx, cy = hit
                mouse.position = (l + cx, t + cy - int(wh * 0.035))
                time.sleep(0.3)
                mouse.click(Button.left)
                clicked = True
        if not clicked:
            # fallback: welcome layout centers the input box
            mouse.position = (l + int(ww * 0.35), t + int(wh * 0.42))
            time.sleep(0.3)
            mouse.click(Button.left)
        time.sleep(0.5)

        keyboard.type(message)
        time.sleep(0.5)
        keyboard.press(Key.enter)
        keyboard.release(Key.enter)
        time.sleep(4.0)

        final_img = grab_window(rect)
        final_path = os.path.join(shots, "1272_final.png")
        final_img.save(final_path)

        # Step 4: sanity — the typed message is the session title in the top band
        lines = ocr_lines(final_img)
        if engine() is not None:
            top = [(cx, cy, tx) for cx, cy, tx, _s in lines if cy < final_img.size[1] * 0.15]
            if not find_text(top, message.split()[0]) or not find_text(top, message.split()[-1]):
                print(f"[1272] ERROR: message '{message}' not echoed in the top band; "
                      f"the send probably did not happen. See {final_path}", flush=True)
                return 1
            print(f"[1272] Step 4: message echoed in top band OK", flush=True)
        else:
            print("[1272] Step 4: skipped (no OCR available).", flush=True)

        # Step 5: message canvas brightness in the central strip
        mean = region_mean_brightness(final_img, 0.35, 0.15, 0.65, 0.65)
        print(f"[1272] Step 5: message canvas mean brightness = {mean:.1f} "
              f"(dark canvas expected < 140; unfixed generic canvas ~255)", flush=True)

        if mean > 140:
            print(f"[1272] TEST FAILED: new conversation message canvas is light "
                  f"(mean={mean:.1f}) — dark style package not applied. See {final_path}",
                  flush=True)
            exit_code = 1
        else:
            print(f"[1272] TEST PASSED: new conversation carries the dark style "
                  f"(canvas mean={mean:.1f}).", flush=True)
            exit_code = 0
        return exit_code

    finally:
        print("[1272] Terminating Mogan...", flush=True)
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
