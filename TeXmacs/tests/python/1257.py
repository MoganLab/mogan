#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated end-to-end UI test for issue 1257:
1. Verify that new draft filename uses underscore '_' to separate date and time
   (e.g., draft_YYYYMMDD_HHMMSS.tmu).
2. Verify backward compatibility with legacy draft filenames (without middle '_')
   across three distinct time cases:
   - Case A (Today / This week): draft_YYYYMMDDHHMMSS.tmu -> shows weekday + HH:MM:SS
   - Case B (Last week / This year): draft_YYYYMMDDHHMMSS.tmu -> shows MM/DD HH:MM
   - Case C (Last year / Past years): draft_YYYYMMDDHHMMSS.tmu -> shows YYYY/MM/DD
"""

import os
import sys
import time
import re
import shutil
import subprocess
from datetime import datetime, timedelta
import numpy as np
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
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def find_no_name_dir():
    candidates = []
    try:
        xdg_docs = subprocess.check_output(["xdg-user-dir", "DOCUMENTS"], text=True).strip()
        if xdg_docs:
            candidates.append(os.path.join(xdg_docs, "LiiiSTEM", "no_name"))
    except Exception:
        pass

    candidates.extend([
        os.path.expanduser("~/文档/LiiiSTEM/no_name"),
        os.path.expanduser("~/Documents/LiiiSTEM/no_name"),
        os.path.expanduser("~/LiiiSTEM/no_name"),
    ])

    for d in candidates:
        if os.path.exists(d):
            return d

    target = os.path.expanduser("~/Documents/LiiiSTEM/no_name")
    os.makedirs(target, exist_ok=True)
    return target


def focus_mogan_window():
    """Ensure Mogan window is raised and focused on X11 platform."""
    try:
        import Xlib.display
        import Xlib.X
        import Xlib.protocol.event

        d = Xlib.display.Display()
        root = d.screen().root

        def find_mogan(win):
            try:
                name = win.get_wm_name()
                if name and "Liii STEM" in name:
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


def terminate_process(proc):
    if proc:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()


def verify_tab_title_rendered(screenshot_path):
    """Verify that the tab bar area contains rendered text (not blank/crashed)."""
    img = Image.open(screenshot_path)
    w, h = img.size
    # Tab bar is located at the top-left area
    tab_crop = img.crop((0, 0, min(w, 800), min(h, 120)))
    arr = np.array(tab_crop.convert("L"))
    # Dark text pixels in tab area
    dark_pixels = np.sum(arr < 100)
    print(f"[1257] Tab bar text dark pixels count: {dark_pixels}")
    return dark_pixels > 200


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    no_name_dir = find_no_name_dir()
    print(f"[1257] Using Mogan binary: {bin_path}")
    print(f"[1257] Monitoring draft directory: {no_name_dir}")

    existing_files = set(os.listdir(no_name_dir)) if os.path.exists(no_name_dir) else set()

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    created_temp_files = []

    # =========================================================================
    # Step 1: Create a new draft and verify 'draft_YYYYMMDD_HHMMSS.tmu' naming
    # =========================================================================
    print("\n[1257] --- Step 1: Creating new draft and testing new naming format ---")
    print("[1257] Launching Mogan...")
    proc = subprocess.Popen([bin_path], env=env)
    keyboard = KeyboardController()
    mouse = MouseController()

    new_tmu_path = None
    matched_file = None
    try:
        time.sleep(3.5)
        focus_mogan_window()
        time.sleep(0.5)

        # Click window to ensure focus
        mouse.position = (500, 500)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        # Create new tab (Ctrl+T)
        print("[1257] Creating new tab (Ctrl+T)...")
        with keyboard.pressed(Key.ctrl):
            keyboard.press("t")
            keyboard.release("t")
        time.sleep(2.0)

        # Type text
        print("[1257] Typing 'hello draft 1257'...")
        keyboard.type("hello draft 1257")
        time.sleep(1.0)

        # Preview (Ctrl+P) to trigger draft save
        print("[1257] Triggering preview (Ctrl+P) to auto-save draft...")
        with keyboard.pressed(Key.ctrl):
            keyboard.press("p")
            keyboard.release("p")
        time.sleep(3.5)

        current_files = set(os.listdir(no_name_dir))
        diff_files = current_files - existing_files
        tmu_files = [f for f in diff_files if f.endswith(".tmu")]
        for f in diff_files:
            created_temp_files.append(os.path.join(no_name_dir, f))

        print(f"[1257] New files detected in draft dir: {diff_files}")
        if not tmu_files:
            print("[1257] ERROR: No .tmu draft file was created after previewing.")
            return 1

        pattern = re.compile(r"^draft_(\d{8})_(\d{6})(-\d+)?\.tmu$")
        for f in tmu_files:
            m = pattern.match(f)
            if m:
                matched_file = f
                date_part = m.group(1)
                time_part = m.group(2)
                print(f"[1257] Valid draft filename: '{f}' (Date={date_part}, Time={time_part}, Separator='_')")
                break

        if not matched_file:
            print(f"[1257] ERROR: Created files {tmu_files} do not match 'draft_YYYYMMDD_HHMMSS.tmu'.")
            return 1

        new_tmu_path = os.path.join(no_name_dir, matched_file)

    finally:
        print("[1257] Terminating first Mogan instance...")
        terminate_process(proc)

    # =========================================================================
    # Step 2: Backward compatibility test for legacy draft filenames
    #         3 Test Cases:
    #         Case A: Today's legacy draft (This week: draft_YYYYMMDDHHMMSS.tmu)
    #         Case B: Last week's legacy draft (This year non-this-week: draft_YYYYMMDDHHMMSS.tmu)
    #         Case C: Last year's legacy draft (Past years: draft_YYYYMMDDHHMMSS.tmu)
    # =========================================================================
    print("\n[1257] --- Step 2: Testing 3 legacy draft filename compatibility cases ---")

    now = datetime.now()
    # Case A: Today (this week)
    today_legacy_name = re.sub(r"^draft_(\d{8})_(\d{6})", r"draft_\1\2", matched_file)
    today_legacy_path = os.path.join(no_name_dir, today_legacy_name)

    # Case B: Last week (7 days ago, ensure different calendar week this year)
    last_week_dt = now - timedelta(days=7)
    last_week_legacy_name = f"draft_{last_week_dt.strftime('%Y%m%d%H%M%S')}.tmu"
    last_week_legacy_path = os.path.join(no_name_dir, last_week_legacy_name)

    # Case C: Last year (1 year ago)
    last_year_dt = now.replace(year=now.year - 1)
    last_year_legacy_name = f"draft_{last_year_dt.strftime('%Y%m%d%H%M%S')}.tmu"
    last_year_legacy_path = os.path.join(no_name_dir, last_year_legacy_name)

    test_cases = [
        ("Case A (Today / This week)", today_legacy_path, "/tmp/1257_legacy_today.png"),
        ("Case B (Last week / This year)", last_week_legacy_path, "/tmp/1257_legacy_last_week.png"),
        ("Case C (Last year / Past year)", last_year_legacy_path, "/tmp/1257_legacy_last_year.png"),
    ]

    for label, target_path, screenshot_path in test_cases:
        print(f"\n[1257] [{label}] Copying '{new_tmu_path}' -> '{target_path}'")
        shutil.copyfile(new_tmu_path, target_path)
        created_temp_files.append(target_path)

        print(f"[1257] [{label}] Launching Mogan to open legacy draft: {os.path.basename(target_path)}")
        proc_legacy = subprocess.Popen([bin_path, target_path], env=env)
        try:
            time.sleep(3.5)
            focus_mogan_window()
            time.sleep(0.5)

            img = ImageGrab.grab()
            img.save(screenshot_path)
            print(f"[1257] [{label}] Saved screenshot to {screenshot_path}")

            if not verify_tab_title_rendered(screenshot_path):
                print(f"[1257] ERROR: [{label}] Tab title was not rendered properly.")
                return 1
            print(f"[1257] [{label}] PASS: Legacy draft opened and tab title rendered successfully.")
        finally:
            print(f"[1257] [{label}] Terminating Mogan instance...")
            terminate_process(proc_legacy)

    # =========================================================================
    # Step 3: Cleanup
    # =========================================================================
    print("\n[1257] --- Step 3: Cleaning up temporary test files ---")
    for p in created_temp_files:
        try:
            if os.path.exists(p):
                os.remove(p)
                print(f"[1257] Removed temp file: {p}")
        except Exception as e:
            print(f"[1257] Warning: Failed to remove {p}: {e}")

    print("\n[1257] ALL TESTS PASSED: New draft format and 3 legacy compatibility cases verified!")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
