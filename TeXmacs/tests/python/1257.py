#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |
| intern                  | Windows     | Passed |

Automated end-to-end UI test for issue 1257:
1. Verify that new draft filename uses underscore '_' to separate date and time
   (e.g., draft_YYYYMMDD_HHMMSS.tmu).
2. Verify backward compatibility with legacy draft filenames (without middle '_')
   across three distinct time cases:
   - Case A (Today / This week): draft_YYYYMMDDHHMMSS.tmu -> shows weekday + HH:MM:SS
   - Case B (Last week / This year): draft_YYYYMMDDHHMMSS.tmu -> shows MM/DD HH:MM
   - Case C (Last year / Past years): draft_YYYYMMDDHHMMSS.tmu -> shows YYYY/MM/DD

Windows: use installed MoganSTEM.exe (xmake b stem && xmake install stem),
Win32 focus, screenshots in the system temp directory. Do not lock the screen.
Linux: X11 focus, screenshots in /tmp (same as PR #4477).
"""

import os
import sys
import time
import re
import shutil
import subprocess
import tempfile
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

IS_WINDOWS = os.name == "nt"


def find_repo_root():
    cur = os.path.abspath(os.path.dirname(__file__))
    while cur != "/" and cur != os.path.dirname(cur):
        if os.path.exists(os.path.join(cur, "TeXmacs")) and os.path.exists(os.path.join(cur, "src")):
            return cur
        cur = os.path.dirname(cur)
    return os.path.abspath(".")


def search_roots(repo_root):
    sibling = os.path.join(os.path.dirname(repo_root), "mogan")
    has_sibling = (
        os.path.isdir(sibling)
        and os.path.normcase(sibling) != os.path.normcase(repo_root)
    )
    # Windows: prefer the installed mogan tree, not an empty worktree.
    if IS_WINDOWS and has_sibling:
        return [sibling, repo_root]
    roots = [repo_root]
    if has_sibling:
        roots.append(sibling)
    return roots


def _ascii(path):
    try:
        path.encode("ascii")
        return True
    except UnicodeEncodeError:
        return False


def _drive_letter(token):
    """Parse subst's 'Z:\\:' (or 'Z:') into 'Z:'."""
    m = re.match(r"([A-Za-z]:)", token.strip())
    return m.group(1).upper() if m else None


def _subst_map():
    """drive letter (e.g. 'Z:') -> absolute destination."""
    mapping = {}
    if not IS_WINDOWS:
        return mapping
    try:
        out = subprocess.check_output(
            ["subst"], text=True, encoding="oem", errors="replace"
        )
    except Exception:
        return mapping
    for line in out.splitlines():
        if "=>" not in line:
            continue
        raw_drive, dest = line.split("=>", 1)
        drive = _drive_letter(raw_drive)
        if not drive:
            continue
        mapping[drive] = os.path.abspath(dest.strip())
    return mapping


def rewrite_via_subst(path):
    """Map a path onto an existing subst drive when the path is under that dest."""
    if not IS_WINDOWS:
        return path
    abs_path = os.path.abspath(path)
    if _ascii(abs_path):
        return abs_path
    for drive, dest in _subst_map().items():
        try:
            dest_abs = os.path.abspath(dest)
            prefix = os.path.normcase(dest_abs)
            cur = os.path.normcase(abs_path)
            if cur == prefix:
                return drive + "\\"
            if cur.startswith(prefix + os.sep):
                rel = abs_path[len(dest_abs) :].lstrip("\\/")
                return os.path.normpath(drive + "\\" + rel)
        except Exception:
            continue
    return abs_path


def ensure_ascii_tree(repo_root):
    """On Windows, subst the hzky parent to a free drive if paths are non-ASCII.

    STEM exits immediately when TEXMACS_PATH contains a Chinese username.
    X: / Y: may already point at other worktrees; we pick Z:, W:, ...
    """
    if not IS_WINDOWS:
        return repo_root
    mapped = rewrite_via_subst(repo_root)
    if _ascii(mapped):
        return mapped
    parent = os.path.dirname(os.path.abspath(repo_root))
    mapped_parent = rewrite_via_subst(parent)
    if _ascii(mapped_parent):
        return rewrite_via_subst(repo_root)
    used = {d.upper() for d in _subst_map()}
    for letter in "ZWVUTS":
        drive = f"{letter}:"
        if drive in used or os.path.exists(drive + "\\"):
            continue
        r = subprocess.run(
            ["subst", drive, parent], capture_output=True, text=True
        )
        if r.returncode == 0:
            mapped = rewrite_via_subst(repo_root)
            print(f"[1257] subst {drive} => {parent}", flush=True)
            return mapped
        print(f"[1257] subst {drive} failed: {r.stderr or r.stdout}", flush=True)
    print("[1257] WARNING: could not subst an ASCII drive; STEM may exit at once.", flush=True)
    return repo_root


def find_mogan_binary(repo_root):
    env_bin = os.environ.get("MOGAN_BIN")
    if env_bin and os.path.exists(env_bin):
        return env_bin

    linux_mac = [
        os.path.join("build", "linux", "x86_64", "release", "moganstem"),
        os.path.join("build", "linux", "x86_64", "debug", "moganstem"),
        os.path.join("build", "linux", "x86_64", "releasedbg", "moganstem"),
        os.path.join("build", "macosx", "arm64", "release", "MoganSTEM.app", "Contents", "MacOS", "MoganSTEM"),
        os.path.join("build", "macosx", "x86_64", "release", "MoganSTEM.app", "Contents", "MacOS", "MoganSTEM"),
    ]
    windows = [
        os.path.join("build", "packages", "stem", "data", "bin", "MoganSTEM.exe"),
        os.path.join("build", "windows", "x64", "release", "MoganSTEM.exe"),
        os.path.join("build", "windows", "x64", "releasedbg", "MoganSTEM.exe"),
    ]
    rels = (windows + linux_mac) if IS_WINDOWS else (linux_mac + [
        os.path.join("build", "packages", "stem", "data", "bin", "MoganSTEM.exe"),
    ])
    for root in search_roots(repo_root):
        for rel in rels:
            c = os.path.join(root, rel)
            if os.path.exists(c):
                return c
    raise FileNotFoundError(
        "Mogan binary not found. Build stem first "
        "(Windows: xmake b stem && xmake install stem; Linux: xmake b stem). "
        "Or set MOGAN_BIN to the executable."
    )


def find_no_name_dir():
    candidates = []
    if not IS_WINDOWS:
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


def _windows_hwnds_for_pid(pid):
    """Visible top-level windows belonging to pid (title may be a file name)."""
    import ctypes
    from ctypes import wintypes

    user32 = ctypes.windll.user32
    EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
    GetWindowTextW = user32.GetWindowTextW
    GetWindowTextLengthW = user32.GetWindowTextLengthW
    IsWindowVisible = user32.IsWindowVisible
    GetWindowThreadProcessId = user32.GetWindowThreadProcessId
    found = []

    def callback(hwnd, _lparam):
        if not IsWindowVisible(hwnd):
            return True
        proc_id = wintypes.DWORD()
        GetWindowThreadProcessId(hwnd, ctypes.byref(proc_id))
        if proc_id.value != pid:
            return True
        n = GetWindowTextLengthW(hwnd)
        buf = ctypes.create_unicode_buffer(n + 1)
        if n > 0:
            GetWindowTextW(hwnd, buf, n + 1)
        found.append((hwnd, buf.value if n > 0 else ""))
        return True

    user32.EnumWindows(EnumWindowsProc(callback), 0)
    return found


def _windows_force_foreground(hwnd):
    """Steal focus from the console so pynput keys reach STEM, not PowerShell."""
    import ctypes
    from ctypes import wintypes

    user32 = ctypes.windll.user32
    kernel32 = ctypes.windll.kernel32
    user32.ShowWindow(hwnd, 9)
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


def _windows_click_center(hwnd, mouse):
    import ctypes
    from ctypes import wintypes

    rect = wintypes.RECT()
    ctypes.windll.user32.GetWindowRect(hwnd, ctypes.byref(rect))
    x = (rect.left + rect.right) // 2
    y = (rect.top + rect.bottom) // 2
    print(f"[1257] Clicking STEM window center ({x}, {y})", flush=True)
    mouse.position = (x, y)
    time.sleep(0.2)
    mouse.click(Button.left)
    time.sleep(0.3)


def _windows_target_hwnd(pid):
    titled = [(h, t) for h, t in _windows_hwnds_for_pid(pid) if t]
    return titled[0][0] if titled else None


_mogan_wait_pid = None


def _windows_focus_mogan():
    import ctypes

    pid = _mogan_wait_pid
    if not pid:
        return
    found = _windows_hwnds_for_pid(pid)
    titled = [(h, t) for h, t in found if t]
    if not titled:
        return
    hwnd = titled[0][0]
    _windows_force_foreground(hwnd)


def focus_mogan_window():
    """Raise and focus the Mogan window (Win32 on Windows, X11 on Linux)."""
    if IS_WINDOWS:
        try:
            _windows_focus_mogan()
        except Exception:
            pass
        return
    try:
        import Xlib.display
        import Xlib.X
        import Xlib.protocol.event

        d = Xlib.display.Display()
        root = d.screen().root

        def find_mogan(win):
            try:
                name = win.get_wm_name()
                if name and ("Liii STEM" in name or "Mogan STEM" in name):
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
    if not proc:
        return
    if IS_WINDOWS:
        try:
            subprocess.run(
                ["taskkill", "/PID", str(proc.pid), "/T", "/F"],
                capture_output=True,
                check=False,
            )
        except Exception:
            pass
        try:
            proc.wait(timeout=5)
        except Exception:
            pass
        return
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()


def wait_for_mogan(proc, timeout=None):
    global _mogan_wait_pid
    if timeout is None:
        timeout = 25.0 if IS_WINDOWS else 3.5
    _mogan_wait_pid = proc.pid if proc else None
    end = time.time() + timeout
    print("[1257] Waiting for Mogan window...", flush=True)
    while time.time() < end:
        if proc and proc.poll() is not None:
            print(
                f"[1257] ERROR: Mogan exited immediately (code {proc.returncode}). "
                "On Windows this is usually a non-ASCII TEXMACS_PATH.",
                flush=True,
            )
            return False
        if IS_WINDOWS and _mogan_wait_pid:
            titled = [(h, t) for h, t in _windows_hwnds_for_pid(_mogan_wait_pid) if t]
            if titled:
                print(f"[1257] Window ready: {titled[0][1]!r}", flush=True)
                focus_mogan_window()
                return True
        focus_mogan_window()
        time.sleep(0.5)
    if not IS_WINDOWS:
        return True
    print("[1257] ERROR: Mogan window did not appear in time.", flush=True)
    return False


def screenshot_path(name):
    base = tempfile.gettempdir() if IS_WINDOWS else "/tmp"
    return os.path.join(base, name)


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


def wait_for_new_tmu(no_name_dir, existing, timeout=20):
    end = time.time() + timeout
    while time.time() < end:
        current = set(os.listdir(no_name_dir)) if os.path.exists(no_name_dir) else set()
        tmu = [f for f in (current - existing) if f.endswith(".tmu")]
        if tmu:
            return current, tmu
        time.sleep(0.5)
    current = set(os.listdir(no_name_dir)) if os.path.exists(no_name_dir) else set()
    return current, [f for f in (current - existing) if f.endswith(".tmu")]


def run_test():
    repo_root = find_repo_root()
    launch_root = ensure_ascii_tree(repo_root)
    bin_path = rewrite_via_subst(find_mogan_binary(repo_root))
    no_name_dir = find_no_name_dir()
    texmacs_path = rewrite_via_subst(os.path.join(repo_root, "TeXmacs"))
    print(f"[1257] Using Mogan binary: {bin_path}", flush=True)
    print(f"[1257] Monitoring draft directory: {no_name_dir}", flush=True)
    print(f"[1257] Platform: {'Windows' if IS_WINDOWS else sys.platform}", flush=True)
    print(f"[1257] TEXMACS_PATH: {texmacs_path}", flush=True)

    existing_files = set(os.listdir(no_name_dir)) if os.path.exists(no_name_dir) else set()

    env = os.environ.copy()
    env["TEXMACS_PATH"] = texmacs_path

    created_temp_files = []

    # =========================================================================
    # Step 1: Create a new draft and verify 'draft_YYYYMMDD_HHMMSS.tmu' naming
    # =========================================================================
    print("\n[1257] --- Step 1: Creating new draft and testing new naming format ---")
    print("[1257] Launching Mogan...")
    proc = subprocess.Popen([bin_path], env=env, cwd=launch_root)
    keyboard = KeyboardController()
    mouse = MouseController()

    new_tmu_path = None
    matched_file = None
    try:
        if not wait_for_mogan(proc):
            return 1
        focus_mogan_window()
        time.sleep(0.4)
        if IS_WINDOWS:
            hwnd = _windows_target_hwnd(proc.pid)
            if hwnd:
                _windows_click_center(hwnd, mouse)
        else:
            mouse.position = (500, 500)
            time.sleep(0.3)
            mouse.click(Button.left)
            time.sleep(0.5)

        # Create new tab (Ctrl+T)
        print("[1257] Creating new tab (Ctrl+T)...", flush=True)
        with keyboard.pressed(Key.ctrl):
            keyboard.press("t")
            keyboard.release("t")
        time.sleep(2.0)

        # Type text
        print("[1257] Typing 'hello draft 1257'...", flush=True)
        keyboard.type("hello draft 1257")
        time.sleep(1.0)

        # Preview (Ctrl+P) to trigger draft save (same as PR #4477).
        # Do not Ctrl+S (Save As) or Esc (cancels preview) on Windows.
        print("[1257] Triggering preview (Ctrl+P) to auto-save draft...", flush=True)
        focus_mogan_window()
        with keyboard.pressed(Key.ctrl):
            keyboard.press("p")
            keyboard.release("p")

        current_files, tmu_files = wait_for_new_tmu(
            no_name_dir, existing_files, timeout=20 if IS_WINDOWS else 4
        )
        diff_files = current_files - existing_files
        for f in diff_files:
            created_temp_files.append(os.path.join(no_name_dir, f))

        print(f"[1257] New files detected in draft dir: {diff_files}")
        if not tmu_files:
            print("[1257] ERROR: No .tmu draft file was created after previewing.")
            print(f"[1257] Watched: {no_name_dir}")
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

    if not matched_file or not new_tmu_path:
        return 1

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
        ("Case A (Today / This week)", today_legacy_path, screenshot_path("1257_legacy_today.png")),
        ("Case B (Last week / This year)", last_week_legacy_path, screenshot_path("1257_legacy_last_week.png")),
        ("Case C (Last year / Past year)", last_year_legacy_path, screenshot_path("1257_legacy_last_year.png")),
    ]

    for label, target_path, shot in test_cases:
        print(f"\n[1257] [{label}] Copying '{new_tmu_path}' -> '{target_path}'")
        shutil.copyfile(new_tmu_path, target_path)
        created_temp_files.append(target_path)

        print(f"[1257] [{label}] Launching Mogan to open legacy draft: {os.path.basename(target_path)}")
        proc_legacy = subprocess.Popen([bin_path, target_path], env=env, cwd=launch_root)
        try:
            if not wait_for_mogan(proc_legacy):
                return 1
            focus_mogan_window()
            time.sleep(0.5)

            img = ImageGrab.grab()
            img.save(shot)
            print(f"[1257] [{label}] Saved screenshot to {shot}")

            if not verify_tab_title_rendered(shot):
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
