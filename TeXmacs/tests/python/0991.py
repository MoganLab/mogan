#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) |        |

Automated UI test and screenshot utility for issue 0991:
Verify LLM chat session layout and styling in Mogan STEM.

Full flow: clear style cache -> launch Mogan -> wait until the window
is really up -> raise it above other windows -> screenshot -> close.

Usage:
  python3 0991.py [path/to/doc.tmu] [output_screenshot.png]
  python3 TeXmacs/tests/python/0991.py [path/to/doc.tmu] [output_screenshot.png]

Environment:
  MOGAN_WAIT_TIMEOUT  Seconds to wait for the window to appear
                      (default 240, useful while a build is in progress)
"""

import os
import shutil
import subprocess
import sys
import time

from PIL import ImageGrab

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

WAIT_TIMEOUT = int(os.environ.get("MOGAN_WAIT_TIMEOUT", "240"))

# Desktop shell windows that must never be iconified
X11_SKIP_CLASSES = {"plasmashell", "krunner", "kwin", "ksmserver", "fcitx"}


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


def clear_mogan_cache():
    """Stale style/plugin cache makes layout tests lie; drop it before launching."""
    if IS_WINDOWS or IS_DARWIN:
        return
    cache = os.path.expanduser("~/.cache/MoganLab")
    if not os.path.exists(cache):
        return
    for tool in ("trash", "trash-put"):
        if shutil.which(tool):
            subprocess.run([tool, cache], check=False)
            print(f"[0991] Cleared cache via {tool}: {cache}")
            return
    shutil.rmtree(cache, ignore_errors=True)
    print(f"[0991] Cleared cache via rmtree: {cache}")


# ---------------------------------------------------------------- X11 helpers

def x11_walk(win):
    yield win
    try:
        for child in win.query_tree().children:
            yield from x11_walk(child)
    except Exception:
        pass


def x11_cls_name(w):
    try:
        return w.get_wm_class(), w.get_wm_name()
    except Exception:
        return None, None


def x11_client_of(frame):
    """First descendant (or self) carrying a WM_CLASS."""
    for w in x11_walk(frame):
        cls, _ = x11_cls_name(w)
        if cls:
            return w
    return None


def x11_frames(d):
    """List of (frame, client) for top-level windows in stacking order."""
    root = d.screen().root
    out = []
    for frame in root.query_tree().children:
        client = x11_client_of(frame)
        if client is not None:
            out.append((frame, client))
    return out


def x11_find_mogan(d):
    for w in x11_walk(d.screen().root):
        cls, _ = x11_cls_name(w)
        if cls and any("mogan" in c.lower() for c in cls):
            g = w.get_geometry()
            if g.width > 400 and g.height > 300:
                return w
    return None


def x11_is_viewable(d, frame):
    try:
        import Xlib.X
        return frame.get_attributes().map_state == Xlib.X.IsViewable
    except Exception:
        return False


def x11_wait_for_window(timeout):
    """Poll until the Mogan window exists; works while a build is in progress."""
    import Xlib.display
    d = Xlib.display.Display()
    deadline = time.time() + timeout
    while time.time() < deadline:
        w = x11_find_mogan(d)
        if w is not None:
            return d, w
        time.sleep(1.0)
    raise TimeoutError(f"Mogan window did not appear within {timeout}s")


def x11_raise_mogan(d, mogan):
    """Iconify viewable windows covering the Mogan rect, then activate Mogan.

    KWin denies _NET_ACTIVE_WINDOW from clients without user input, so
    overlapping windows (terminals, browsers) are minimized and restored
    around the screenshot instead.
    """
    import Xlib.X
    import Xlib.protocol.event

    root = d.screen().root
    wm_change = d.intern_atom("WM_CHANGE_STATE")
    net_active = d.intern_atom("_NET_ACTIVE_WINDOW")
    mask = Xlib.X.SubstructureRedirectMask | Xlib.X.SubstructureNotifyMask

    mg = mogan.get_geometry()
    mc = mogan.translate_coords(root, 0, 0)
    mrect = (mc.x, mc.y, mc.x + mg.width, mc.y + mg.height)

    frames = x11_frames(d)
    mogan_frame = next((f for f, c in frames if c.id == mogan.id), mogan)
    mogan_idx = next((i for i, (f, c) in enumerate(frames) if c.id == mogan.id), -1)

    iconified = []
    for i, (frame, client) in enumerate(frames):
        if client.id == mogan.id or i < mogan_idx:
            continue  # only windows stacked above Mogan can cover it
        cls, _ = x11_cls_name(client)
        if not cls or any(c.lower() in X11_SKIP_CLASSES for c in cls):
            continue
        if not x11_is_viewable(d, frame):
            continue
        g = frame.get_geometry()
        if g.width < 200 or g.height < 200:
            continue
        # intersect with Mogan rect?
        if g.x + g.width <= mrect[0] or g.x >= mrect[2] or \
           g.y + g.height <= mrect[1] or g.y >= mrect[3]:
            continue
        root.send_event(Xlib.protocol.event.ClientMessage(
            window=client, client_type=wm_change, data=(32, [3, 0, 0, 0, 0])),
            event_mask=mask)
        iconified.append((client, cls_name_str(cls)))
    d.sync()
    if iconified:
        print(f"[0991] Iconified covering windows: {iconified}")
        time.sleep(1.5)

    root.send_event(Xlib.protocol.event.ClientMessage(
        window=mogan, client_type=net_active,
        data=(32, [2, Xlib.X.CurrentTime, 0, 0, 0])),
        event_mask=mask)
    try:
        mogan.set_input_focus(Xlib.X.RevertToParent, Xlib.X.CurrentTime)
    except Exception:
        pass
    mogan_frame.configure(stack_mode=Xlib.X.Above)
    d.sync()
    time.sleep(2.0)

    return [c for c, _ in iconified]


def x11_restore(d, clients):
    import Xlib.X
    import Xlib.protocol.event
    root = d.screen().root
    wm_change = d.intern_atom("WM_CHANGE_STATE")
    mask = Xlib.X.SubstructureRedirectMask | Xlib.X.SubstructureNotifyMask
    for c in clients:
        try:
            root.send_event(Xlib.protocol.event.ClientMessage(
                window=c, client_type=wm_change, data=(32, [1, 0, 0, 0, 0])),
                event_mask=mask)
        except Exception:
            pass
    d.sync()


def cls_name_str(cls):
    return "/".join(cls) if cls else "?"


# ------------------------------------------------------------ other platforms

def focus_mogan_window():
    """Raise and focus the Mogan window on macOS / Windows."""
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
    """Returns (x, y, w, h) of the Mogan window, or full screen as fallback."""
    if not IS_DARWIN and not IS_WINDOWS:
        try:
            import Xlib.display
            d = Xlib.display.Display()
            w = x11_find_mogan(d)
            if w:
                g = w.get_geometry()
                c = w.translate_coords(d.screen().root, 0, 0)
                return (c.x, c.y, g.width, g.height)
        except Exception:
            pass

    img = ImageGrab.grab()
    return (0, 0, img.size[0], img.size[1])


# -------------------------------------------------------------------- main flow

def run_0991_test(doc_path=None, output_path=None):
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)

    if doc_path is None:
        default_candidates = [
            os.path.join(repo_root, "TeXmacs", "tests", "tmu", "0991.tmu"),
            os.path.join(repo_root, "0991.tmu"),
        ]
        for c in default_candidates:
            if os.path.exists(c):
                doc_path = c
                break
        if doc_path is None:
            doc_path = default_candidates[0]

    if output_path is None:
        output_path = "/tmp/0991.png"

    print(f"[0991] Target document: {doc_path}")
    print(f"[0991] Output screenshot: {output_path}")
    print(f"[0991] Using Mogan binary: {bin_path}")

    print("[0991] Step 0: Clearing Mogan cache...")
    clear_mogan_cache()

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[0991] Step 1: Launching Mogan STEM...")
    proc = subprocess.Popen([bin_path, doc_path], env=env, cwd=repo_root)

    iconified = []
    x11_d = None
    try:
        print(f"[0991] Step 2: Waiting for the Mogan window (up to {WAIT_TIMEOUT}s)...")
        if not IS_DARWIN and not IS_WINDOWS:
            x11_d, _ = x11_wait_for_window(WAIT_TIMEOUT)
        else:
            time.sleep(5.0)

        ret = proc.poll()
        if ret is not None:
            print(f"[0991] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        # let the document finish typesetting before shooting
        time.sleep(5.0)

        print("[0991] Step 3: Raising Mogan above other windows...")
        if x11_d is not None:
            mogan = x11_find_mogan(x11_d)
            if mogan is not None:
                iconified = x11_raise_mogan(x11_d, mogan)
        else:
            focus_mogan_window()
            time.sleep(1.0)

        wx, wy, ww, wh = get_mogan_window_rect()
        print(f"[0991] Mogan window bounds: ({wx}, {wy}, {ww}, {wh})")

        mouse = MouseController()
        kb = KeyboardController()

        click_x = wx + ww // 2
        click_y = wy + wh // 2
        print(f"[0991] Step 4: Clicking document area at ({click_x}, {click_y})...")
        mouse.position = (click_x, click_y)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(1.0)

        print("[0991] Step 5: Capturing screenshot...")
        if ww > 100 and wh > 100:
            screenshot = ImageGrab.grab(bbox=(wx, wy, wx + ww, wy + wh))
        else:
            screenshot = ImageGrab.grab()
        screenshot.save(output_path)
        print(f"[0991] Screenshot successfully saved to: {output_path}")

        print("[0991] Step 6: Closing Mogan (Ctrl+Q)...")
        modifier = Key.cmd if IS_DARWIN else Key.ctrl
        with kb.pressed(modifier):
            kb.press("q")
            kb.release("q")
        time.sleep(1.5)

        ret = proc.poll()
        if ret is None:
            print("[0991] Process did not exit after Ctrl+Q, terminating...")
            proc.terminate()
            try:
                proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                proc.kill()

        print("[0991] Finished successfully.")
        return 0
    finally:
        if x11_d is not None and iconified:
            print("[0991] Restoring iconified windows...")
            x11_restore(x11_d, iconified)
        if proc.poll() is None:
            proc.kill()


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
    sys.exit(run_0991_test(doc, out))
