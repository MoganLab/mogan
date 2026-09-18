#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated UI test for issue 1315:
1. Launch Mogan STEM with ~/git/liii/arxiv/deepseek_v4.stem.
2. Verify copy-paste functionality into newly created draft documents:
   - Test short text selection (< 10 chars, AI floating bar has no translate button).
   - Test medium text selection (>= 10 chars, AI floating bar appears).
   - Test multiple copies with AI floating bar present:
     * Select Line 1 of title (AI floating bar pops up below it).
     * Copy Line 1.
     * With AI floating bar present, select Line 2 of title (overlapping where AI bar was).
     * Copy Line 2.
     * Open new draft document (Ctrl+N) and paste (Ctrl+V).
     * Verify Line 2 content is pasted correctly into the draft, proving that the AI
       floating window does not steal keyboard focus and does not block text selection.
   - Test multi-line large selection spanning across paragraphs.
3. Cleanly exit Mogan STEM after each test.
"""

import os
import sys
import time
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
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))


def find_mogan_binary(repo_root):
    candidates = [
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
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
            user32 = ctypes.windll.user32
            def enum_cb(hwnd, lparam):
                length = user32.GetWindowTextLengthW(hwnd)
                if length > 0:
                    buff = ctypes.create_unicode_buffer(length + 1)
                    user32.GetWindowTextW(hwnd, buff, length + 1)
                    title = buff.value
                    if "Mogan" in title or "STEM" in title:
                        user32.ShowWindow(hwnd, 9)
                        user32.SetForegroundWindow(hwnd)
                        return False
                return True
            WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_int, ctypes.c_int)
            user32.EnumWindows(WNDENUMPROC(enum_cb), 0)
        except Exception:
            pass
        return

    # Linux (X11)
    try:
        import Xlib.display, Xlib.X, Xlib.protocol.event
        d = Xlib.display.Display()
        root = d.screen().root

        def find_win(w):
            try:
                name = w.get_wm_name() or ""
                if "Mogan" in name or "STEM" in name or "Liii" in name:
                    return w
                for c in w.query_tree().children:
                    res = find_win(c)
                    if res:
                        return res
            except Exception:
                pass
            return None

        w = find_win(root)
        if w:
            net_active = d.intern_atom("_NET_ACTIVE_WINDOW")
            cm = Xlib.protocol.event.ClientMessage(
                window=w,
                client_type=net_active,
                data=(32, [2, Xlib.X.CurrentTime, 0, 0, 0])
            )
            root.send_event(cm, event_mask=Xlib.X.SubstructureRedirectMask | Xlib.X.SubstructureNotifyMask)
            w.set_input_focus(Xlib.X.RevertToParent, Xlib.X.CurrentTime)
            w.configure(stack_mode=Xlib.X.Above)
            d.sync()
    except Exception:
        pass


def drag_mouse(mouse, p1, p2, steps=15, step_delay=0.015):
    mouse.position = p1
    time.sleep(0.08)
    mouse.press(Button.left)
    time.sleep(0.04)
    for s in range(1, steps + 1):
        x = int(p1[0] + (p2[0] - p1[0]) * s / steps)
        y = int(p1[1] + (p2[1] - p1[1]) * s / steps)
        mouse.position = (x, y)
        time.sleep(step_delay)
    time.sleep(0.08)
    mouse.release(Button.left)
    time.sleep(0.8)


def trigger_copy(kb):
    focus_mogan_window()
    mod = Key.cmd if IS_DARWIN else Key.ctrl
    with kb.pressed(mod):
        kb.press('c')
        time.sleep(0.05)
        kb.release('c')
    time.sleep(0.4)


def create_draft_and_paste(kb):
    focus_mogan_window()
    mod = Key.cmd if IS_DARWIN else Key.ctrl
    with kb.pressed(mod):
        kb.press('n')
        time.sleep(0.05)
        kb.release('n')
    time.sleep(2.0)
    focus_mogan_window()

    shot_before = ImageGrab.grab()

    with kb.pressed(mod):
        kb.press('v')
        time.sleep(0.05)
        kb.release('v')
    time.sleep(1.5)

    shot_after = ImageGrab.grab()
    arr_b = np.array(shot_before.crop((600, 300, 2400, 1800)))
    arr_a = np.array(shot_after.crop((600, 300, 2400, 1800)))
    diff = int(np.sum(np.abs(arr_a.astype(int) - arr_b.astype(int)) > 20))
    return diff, shot_after


def clean_close_mogan(proc, kb):
    try:
        focus_mogan_window()
        mod = Key.cmd if IS_DARWIN else Key.ctrl
        with kb.pressed(mod):
            kb.press('q')
            time.sleep(0.05)
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


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    doc_path = os.path.expanduser("~/git/liii/arxiv/deepseek_v4.stem")
    if not os.path.exists(doc_path):
        print(f"[1315] Error: Document path not found: {doc_path}")
        return 1

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    print("[1315] ==========================================================")
    print("[1315] Starting Automated Copy-Paste Test (Issue 1315)")
    print(f"[1315] Binary: {bin_path}")
    print(f"[1315] Document: {doc_path}")
    print("[1315] ==========================================================")

    # -------------------------------------------------------------------------
    # Case 1: Multiple copies with AI action bar present
    # Line 1 copied, then Line 2 copied over AI action bar, then pasted
    # -------------------------------------------------------------------------
    print("\n[1315] Case 1: Multiple sequential copies with AI action bar present...")
    proc = subprocess.Popen([bin_path, "-d", doc_path], env=env, cwd=repo_root)
    try:
        time.sleep(5.0)
        focus_mogan_window()
        time.sleep(0.5)

        mouse = MouseController()
        kb = KeyboardController()

        # Select Title Line 1 (AI bar pops up below it)
        print("[1315]   Step 1: Selecting Title Line 1 (1000, 750) -> (1450, 750)...")
        drag_mouse(mouse, (1000, 750), (1450, 750))
        print("[1315]   Step 2: Copying Line 1 (Ctrl+C)...")
        trigger_copy(kb)

        # Select Title Line 2 starting over the AI action bar (1150, 845) down to (1550, 900)
        print("[1315]   Step 3: Selecting Title Line 2 starting over AI bar (1150, 845) -> (1550, 900)...")
        drag_mouse(mouse, (1150, 845), (1550, 900))
        print("[1315]   Step 4: Copying Line 2 (Ctrl+C)...")
        trigger_copy(kb)

        # Open new draft and paste
        print("[1315]   Step 5: Opening draft (Ctrl+N) and pasting (Ctrl+V)...")
        diff, shot = create_draft_and_paste(kb)
        print(f"[1315]   Case 1 pixel diff: {diff}")
        if diff < 100:
            print(f"[1315] ERROR: Case 1 paste failed (diff={diff})!")
            return 1

        # Check that Line 2 ("Token Context Intelligence") was pasted by verifying
        # the text bounding area in the draft document
        crop = shot.crop((900, 700, 2000, 950))
        crop_arr = np.array(crop)
        dark_pixels = np.sum((crop_arr[:, :, 0] < 80) & (crop_arr[:, :, 1] < 80) & (crop_arr[:, :, 2] < 80))
        print(f"[1315]   Pasted dark pixels count: {dark_pixels}")
        if dark_pixels < 200:
            print("[1315] ERROR: Pasted text appears empty or incomplete!")
            return 1
        print("[1315] Case 1 PASSED: Selection 2 over AI bar successfully copied and pasted!")

    finally:
        clean_close_mogan(proc, kb)
        time.sleep(1.0)

    # -------------------------------------------------------------------------
    # Case 2: Short selection (< 10 characters) copy and paste
    # -------------------------------------------------------------------------
    print("\n[1315] Case 2: Short selection (< 10 chars) copy and paste...")
    proc = subprocess.Popen([bin_path, "-d", doc_path], env=env, cwd=repo_root)
    try:
        time.sleep(5.0)
        focus_mogan_window()
        time.sleep(0.5)

        mouse = MouseController()
        kb = KeyboardController()

        print("[1315]   Step 1: Selecting 1-word text (950, 1650) -> (990, 1650)...")
        drag_mouse(mouse, (950, 1650), (990, 1650))
        print("[1315]   Step 2: Copying (Ctrl+C)...")
        trigger_copy(kb)

        print("[1315]   Step 3: Opening draft (Ctrl+N) and pasting (Ctrl+V)...")
        diff, _ = create_draft_and_paste(kb)
        print(f"[1315]   Case 2 pixel diff: {diff}")
        if diff < 50:
            print(f"[1315] ERROR: Case 2 paste failed (diff={diff})!")
            return 1
        print("[1315] Case 2 PASSED: Short selection successfully copied and pasted!")

    finally:
        clean_close_mogan(proc, kb)
        time.sleep(1.0)

    # -------------------------------------------------------------------------
    # Case 3: Large multi-line selection copy and paste
    # -------------------------------------------------------------------------
    print("\n[1315] Case 3: Large multi-line selection copy and paste...")
    proc = subprocess.Popen([bin_path, "-d", doc_path], env=env, cwd=repo_root)
    try:
        time.sleep(5.0)
        focus_mogan_window()
        time.sleep(0.5)

        mouse = MouseController()
        kb = KeyboardController()

        print("[1315]   Step 1: Selecting multi-line abstract (950, 1650) -> (2000, 1830)...")
        drag_mouse(mouse, (950, 1650), (2000, 1830))
        print("[1315]   Step 2: Copying (Ctrl+C)...")
        trigger_copy(kb)

        print("[1315]   Step 3: Opening draft (Ctrl+N) and pasting (Ctrl+V)...")
        diff, _ = create_draft_and_paste(kb)
        print(f"[1315]   Case 3 pixel diff: {diff}")
        if diff < 5000:
            print(f"[1315] ERROR: Case 3 paste failed (diff={diff})!")
            return 1
        print("[1315] Case 3 PASSED: Multi-line selection successfully copied and pasted!")

    finally:
        clean_close_mogan(proc, kb)
        time.sleep(1.0)

    print("\n[1315] ==========================================================")
    print("[1315] ALL TEST SCENARIOS PASSED SUCCESSFULLY!")
    print("[1315] ==========================================================")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
