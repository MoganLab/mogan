#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Automated end-to-end UI test for issue 1243:
Capture screenshots at distinct stages of AI chat sidebar workflow for agent/human verification:
1. Stage 1: Document selection is automatically prefilled into chat input (Initial state).
2. Stage 2: Non-empty chat input is preserved when reopening sidebar.
3. Stage 3: Chat input remains empty when opened with no document selection.
4. Stage 4: Chat input is prefilled with new document selection after 1 round of AI dialogue.
5. Stage 5: Chat input is prefilled with new document selection after 2 rounds of AI dialogue.
"""

import os
import sys
import time
import tempfile
import subprocess

# Ensure UTF-8 and unbuffered stdout
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(line_buffering=True, encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(line_buffering=True, encoding="utf-8")

# Set DPI awareness on Windows as early as possible so all coordinates and grabs align
if sys.platform == "win32":
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(2)  # PROCESS_PER_MONITOR_DPI_AWARE
    except Exception:
        try:
            import ctypes
            ctypes.windll.user32.SetProcessDPIAware()
        except Exception:
            pass

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


class ClipboardManager:
    """Cross-platform clipboard helper."""

    def __init__(self):
        if sys.platform != "darwin":
            try:
                import tkinter as tk
                self.root = tk.Tk()
                self.root.withdraw()
            except Exception:
                self.root = None
        else:
            self.root = None

    def get(self):
        if sys.platform == "darwin":
            try:
                return subprocess.check_output(["pbpaste"], timeout=1, text=True)
            except Exception:
                return ""
        if self.root:
            try:
                self.root.update()
                return self.root.clipboard_get()
            except Exception:
                pass
        if sys.platform == "win32":
            try:
                import ctypes
                import ctypes.wintypes
                user32 = ctypes.windll.user32
                kernel32 = ctypes.windll.kernel32
                if user32.OpenClipboard(0):
                    CF_UNICODETEXT = 13
                    h_mem = user32.GetClipboardData(CF_UNICODETEXT)
                    text = ""
                    if h_mem:
                        ptr = kernel32.GlobalLock(h_mem)
                        text = ctypes.c_wchar_p(ptr).value or ""
                        kernel32.GlobalUnlock(h_mem)
                    user32.CloseClipboard()
                    return text
            except Exception:
                pass
        return ""

    def set(self, text):
        if sys.platform == "darwin":
            try:
                p = subprocess.Popen(["pbcopy"], stdin=subprocess.PIPE)
                p.communicate(text.encode("utf-8"))
            except Exception:
                pass
            return
        if self.root:
            try:
                self.root.clipboard_clear()
                self.root.clipboard_append(text)
                self.root.update()
            except Exception:
                pass

    def close(self):
        if self.root:
            try:
                self.root.destroy()
            except Exception:
                pass


def ensure_english_ime(hwnd=None):
    """
    On Windows, ensure Chinese IME is turned off / switched to US English layout
    so that simulated keyboard strokes and hotkeys are not intercepted.
    """
    if sys.platform != "win32":
        return
    try:
        import ctypes
        import ctypes.wintypes
        user32 = ctypes.windll.user32
        imm32 = ctypes.windll.imm32
        kernel32 = ctypes.windll.kernel32

        # 1. Activate US English (00000409) in the current test runner thread
        hkl_us = user32.LoadKeyboardLayoutW("00000409", 1)
        user32.ActivateKeyboardLayout(ctypes.c_void_p(hkl_us), 0)

        # 2. Check and turn off Caps Lock if it's currently on
        VK_CAPITAL = 0x14
        if user32.GetKeyState(VK_CAPITAL) & 1:
            user32.keybd_event(VK_CAPITAL, 0x45, 1, 0)
            user32.keybd_event(VK_CAPITAL, 0x45, 3, 0)

        # 3. For target Mogan window / thread, switch layout & close IME
        if hwnd:
            tid_target = user32.GetWindowThreadProcessId(hwnd, None)
            current_tid = kernel32.GetCurrentThreadId()
            if tid_target and tid_target != current_tid:
                user32.AttachThreadInput(current_tid, tid_target, True)
                user32.ActivateKeyboardLayout(ctypes.c_void_p(hkl_us), 0)
                user32.AttachThreadInput(current_tid, tid_target, False)

            # Request input language change
            WM_INPUTLANGCHANGEREQUEST = 0x0050
            user32.PostMessageW(hwnd, WM_INPUTLANGCHANGEREQUEST, 1, hkl_us)

            # Set conversion mode to alphanumeric & turn off IME via Imm context
            himc = imm32.ImmGetContext(hwnd)
            if himc:
                imm32.ImmSetOpenStatus(himc, 0)
                imm32.ImmSetConversionStatus(himc, 0, 0)
                imm32.ImmReleaseContext(hwnd, himc)

            # Send IME control commands to default IME window
            ime_hwnd = imm32.ImmGetDefaultIMEWnd(hwnd)
            if ime_hwnd:
                WM_IME_CONTROL = 0x0283
                IMC_SETOPENSTATUS = 0x0006
                IMC_SETCONVERSIONMODE = 0x0002
                user32.SendMessageW(ime_hwnd, WM_IME_CONTROL, IMC_SETOPENSTATUS, 0)
                user32.SendMessageW(ime_hwnd, WM_IME_CONTROL, IMC_SETCONVERSIONMODE, 0)
    except Exception as e:
        print(f"[1243] Note: ensure_english_ime warning: {e}")


def find_repo_root():
    cur = os.path.abspath(os.path.dirname(__file__))
    while cur != "/" and cur != os.path.dirname(cur):
        if os.path.exists(os.path.join(cur, "TeXmacs")) and os.path.exists(os.path.join(cur, "src")):
            return cur
        cur = os.path.dirname(cur)
    return os.path.abspath(".")


def find_mogan_binary(repo_root):
    candidates = [
        os.path.join(repo_root, "build", "packages", "stem", "data", "bin", "MoganSTEM.exe"),
        os.path.join(repo_root, "build", "windows", "x64", "releasedbg", "MoganSTEM.exe"),
        os.path.join(repo_root, "build", "windows", "x64", "release", "MoganSTEM.exe"),
        os.path.join(repo_root, "build", "windows", "x64", "debug", "MoganSTEM.exe"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/debug/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/debug/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem && xmake i stem).")


def kill_existing_mogan():
    """Ensure no stale Mogan processes interfere with the test."""
    try:
        if sys.platform == "win32":
            subprocess.run(["taskkill", "/F", "/IM", "MoganSTEM.exe"], capture_output=True)
        elif sys.platform == "darwin":
            subprocess.run(["pkill", "-9", "-f", "MoganSTEM"], capture_output=True)
        else:
            subprocess.run(["pkill", "-9", "-f", "moganstem"], capture_output=True)
    except Exception:
        pass


def find_mogan_hwnd(proc_pid=None):
    """Find Mogan top-level HWND on Windows."""
    if sys.platform != "win32":
        return None
    try:
        import ctypes
        import ctypes.wintypes
        user32 = ctypes.windll.user32

        matched_hwnds = []

        def enum_cb(hwnd, lparam):
            if not user32.IsWindowVisible(hwnd):
                return True
            length = user32.GetWindowTextLengthW(hwnd)
            if length > 0:
                buff = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(hwnd, buff, length + 1)
                title = buff.value
                lpdw_process_id = ctypes.wintypes.DWORD()
                user32.GetWindowThreadProcessId(hwnd, ctypes.byref(lpdw_process_id))
                if proc_pid and lpdw_process_id.value == proc_pid:
                    matched_hwnds.append((hwnd, title, True))
                elif any(k in title for k in ["Liii STEM", "Mogan", "TeXmacs"]):
                    matched_hwnds.append((hwnd, title, False))
            return True

        WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.wintypes.HWND, ctypes.wintypes.LPARAM)
        user32.EnumWindows(WNDENUMPROC(enum_cb), 0)

        for hwnd, title, is_pid in matched_hwnds:
            if is_pid:
                return hwnd
        if matched_hwnds:
            return matched_hwnds[0][0]
    except Exception:
        pass
    return None


def focus_mogan_window(proc=None):
    """Ensure Mogan window is raised and focused across platforms."""
    if sys.platform == "darwin":
        try:
            import AppKit
            for app in AppKit.NSWorkspace.sharedWorkspace().runningApplications():
                if "Mogan" in (app.localizedName() or ""):
                    app.activateWithOptions_(AppKit.NSApplicationActivateIgnoringOtherApps)
                    return None
        except Exception:
            pass
        subprocess.run(
            ["osascript", "-e", 'tell application "System Events" to set frontmost of first process whose name contains "Mogan" to true'],
            capture_output=True,
        )
        return None

    if sys.platform == "win32":
        try:
            import ctypes
            import ctypes.wintypes
            user32 = ctypes.windll.user32
            kernel32 = ctypes.windll.kernel32

            pid = proc.pid if proc else None
            hwnd = find_mogan_hwnd(pid)
            if hwnd:
                user32.ShowWindow(hwnd, 9)  # SW_RESTORE
                fg_hwnd = user32.GetForegroundWindow()
                fg_tid = user32.GetWindowThreadProcessId(fg_hwnd, None)
                cur_tid = kernel32.GetCurrentThreadId()
                if fg_tid and fg_tid != cur_tid:
                    user32.AttachThreadInput(cur_tid, fg_tid, True)
                    user32.SetForegroundWindow(hwnd)
                    user32.BringWindowToTop(hwnd)
                    user32.SetFocus(hwnd)
                    user32.AttachThreadInput(cur_tid, fg_tid, False)
                else:
                    user32.SetForegroundWindow(hwnd)
                    user32.BringWindowToTop(hwnd)
                    user32.SetFocus(hwnd)
                ensure_english_ime(hwnd)
                return hwnd
        except Exception as e:
            print(f"[1243] Note: focus_mogan_window warning: {e}")
        return None

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
    return None


def find_plus_icon_in_image(im):
    """Detect the '+' new-tab icon (a symmetric cross) in a title bar crop."""
    gray = im.convert("L")
    w, h = gray.size
    best_score = 0
    best_pos = None
    for y in range(15, h - 15):
        for x in range(30, min(w - 30, 1500)):
            if gray.getpixel((x, y)) < 100:
                arm_u = sum(1 for dy in range(1, 8) if gray.getpixel((x, y - dy)) < 130)
                arm_d = sum(1 for dy in range(1, 8) if gray.getpixel((x, y + dy)) < 130)
                arm_l = sum(1 for dx in range(1, 8) if gray.getpixel((x - dx, y)) < 130)
                arm_r = sum(1 for dx in range(1, 8) if gray.getpixel((x + dx, y)) < 130)
                corners_light = sum(
                    1
                    for dx, dy in [(-5, -5), (5, -5), (-5, 5), (5, 5)]
                    if gray.getpixel((x + dx, y + dy)) > 170
                )
                if arm_u >= 3 and arm_d >= 3 and arm_l >= 3 and arm_r >= 3 and corners_light >= 3:
                    score = arm_u + arm_d + arm_l + arm_r
                    if score > best_score:
                        best_score = score
                        best_pos = (x, y)
    return best_pos


def get_plus_button_coords(hwnd):
    """Find absolute screen coordinates of the '+' new-tab button."""
    if sys.platform == "win32" and hwnd:
        try:
            import ctypes
            import ctypes.wintypes
            user32 = ctypes.windll.user32
            rect = ctypes.wintypes.RECT()
            if user32.GetWindowRect(hwnd, ctypes.byref(rect)):
                bar_h = 90
                w = max(100, rect.right - rect.left)
                im = ImageGrab.grab(
                    bbox=(rect.left, rect.top, min(rect.right, rect.left + min(1600, w)), rect.top + bar_h)
                )
                rel_pos = find_plus_icon_in_image(im)
                if rel_pos:
                    return (rect.left + rel_pos[0], rect.top + rel_pos[1])
        except Exception as e:
            print(f"[1243] Note: get_plus_button_coords exception: {e}")
    return None


def wait_and_get_plus_button_coords(hwnd, timeout=10.0):
    """Poll and wait until the '+' new-tab button is confirmed in the title bar."""
    start_time = time.time()
    while time.time() - start_time < timeout:
        coords = get_plus_button_coords(hwnd)
        if coords:
            return coords
        time.sleep(0.5)
    return None


def get_window_document_coords(hwnd):
    """Compute document clicking coordinates inside the window."""
    if sys.platform == "win32" and hwnd:
        try:
            import ctypes
            import ctypes.wintypes
            user32 = ctypes.windll.user32
            rect = ctypes.wintypes.RECT()
            if user32.GetWindowRect(hwnd, ctypes.byref(rect)):
                # Pick a point squarely inside the document editor area (1/3 from left, 1/3 from top)
                w = max(100, rect.right - rect.left)
                h = max(100, rect.bottom - rect.top)
                x = rect.left + w // 3
                y = rect.top + h // 3
                return (x, y)
        except Exception:
            pass
    return (500, 400)


def insert_doc_text(keyboard, clipboard, text, mod_key):
    """Insert text into the active document or input widget reliably."""
    clipboard.set(text)
    time.sleep(0.15)
    with keyboard.pressed(mod_key):
        keyboard.press("v")
        keyboard.release("v")
    time.sleep(0.4)


def get_screenshot_dir():
    env_dir = os.environ.get("MOGAN_SCREENSHOT_DIR")
    if env_dir:
        os.makedirs(env_dir, exist_ok=True)
        return env_dir

    if sys.platform == "win32":
        candidates = [
            "C:\\tmp",
            os.path.join(tempfile.gettempdir(), "mogan_1243"),
        ]
        for c in candidates:
            try:
                os.makedirs(c, exist_ok=True)
                return c
            except Exception:
                continue
        return tempfile.gettempdir()
    else:
        os.makedirs("/tmp", exist_ok=True)
        return "/tmp"


def terminate_process(proc):
    if proc:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()


def capture_stage_screenshot(filename, description, stage_list, out_dir=None):
    """Capture full screen for agent visual verification and record stage info."""
    if not out_dir:
        out_dir = get_screenshot_dir()
    path = os.path.join(out_dir, filename)
    ImageGrab.grab().save(path)
    stage_list.append((len(stage_list) + 1, description, path))
    print(f"[1243] Screenshot captured: {path} ({description})")
    return path


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    print(f"[1243] Using Mogan binary: {bin_path}")

    # On macOS, ensure .app bundle is ad-hoc signed so AMFI allows execution
    if sys.platform == "darwin":
        app_dir = os.path.dirname(os.path.dirname(os.path.dirname(bin_path)))
        if app_dir.endswith(".app"):
            subprocess.run(["codesign", "--force", "--deep", "--sign", "-", app_dir], capture_output=True)

    out_dir = get_screenshot_dir()
    print(f"[1243] Screenshot output directory: {out_dir}")

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    clipboard = ClipboardManager()
    keyboard = KeyboardController()
    mouse = MouseController()
    mod_key = Key.cmd if sys.platform == "darwin" else Key.ctrl
    captured_screenshots = []

    kill_existing_mogan()
    time.sleep(0.5)

    print("[1243] Launching Mogan...")
    proc = subprocess.Popen([bin_path], env=env)

    hwnd = None
    try:
        # Wait for Mogan window to appear and focus it without clicking inside
        for _ in range(20):
            time.sleep(0.5)
            hwnd = focus_mogan_window(proc)
            if hwnd or sys.platform != "win32":
                break
        time.sleep(1.0)
        ensure_english_ime(hwnd)

        doc_pos = get_window_document_coords(hwnd)
        print(f"[1243] Document click coordinates: {doc_pos}")

        # =========================================================================
        # Setup: Create a new document tab by waiting for and clicking '+'
        # =========================================================================
        print("\n[1243] --- Setup: Waiting for '+' button to appear in tab bar ---")
        ensure_english_ime(hwnd)
        plus_pos = wait_and_get_plus_button_coords(hwnd, timeout=10.0)
        if plus_pos:
            print(f"[1243] Confirmed '+' button at {plus_pos}, clicking...")
            mouse.position = plus_pos
            time.sleep(0.3)
            mouse.click(Button.left)
        else:
            print("[1243] Fallback: Creating tab via shortcut...")
            with keyboard.pressed(mod_key):
                keyboard.press("t")
                keyboard.release("t")
        time.sleep(2.0)

        # =========================================================================
        # Stage 1: Initial state - Selection prefilled on opening sidebar
        # =========================================================================
        print("\n[1243] --- Stage 1: Initial state prefill ---")
        stage1_text = "Stage1SelectionInitial"
        print(f"[1243] Inserting document text: '{stage1_text}'...")
        ensure_english_ime(hwnd)
        insert_doc_text(keyboard, clipboard, stage1_text, mod_key)
        time.sleep(0.5)

        print("[1243] Selecting document text...")
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.8)

        print("[1243] Opening AI chat sidebar (mod+J)...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(3.0)

        capture_stage_screenshot(
            "1243_stage1_initial_prefill.png",
            "初始状态：文档选区自动预填入聊天输入区",
            captured_screenshots,
            out_dir,
        )

        # Copy input to check clipboard
        ensure_english_ime(hwnd)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.4)
        with keyboard.pressed(mod_key):
            keyboard.press("c")
            keyboard.release("c")
        time.sleep(0.8)
        print(f"[1243] Stage 1 Clipboard text: '{clipboard.get().strip()}'")

        # =========================================================================
        # Stage 2: Non-empty chat input preservation
        # =========================================================================
        print("\n[1243] --- Stage 2: Non-empty chat input protection ---")
        keyboard.press(Key.right)
        keyboard.release(Key.right)
        time.sleep(0.3)
        ensure_english_ime(hwnd)
        insert_doc_text(keyboard, clipboard, "_PreservedDraft", mod_key)
        time.sleep(0.5)

        print("[1243] Closing sidebar (mod+J)...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(2.0)

        # Modify document selection
        mouse.position = doc_pos
        mouse.click(Button.left)
        time.sleep(0.5)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.3)
        ensure_english_ime(hwnd)
        insert_doc_text(keyboard, clipboard, "OtherDocumentSelection", mod_key)
        time.sleep(0.5)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.8)

        # Reopen sidebar
        print("[1243] Reopening AI chat sidebar (mod+J)...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(3.0)

        capture_stage_screenshot(
            "1243_stage2_nonempty_preserved.png",
            "非空保护：输入区已有草稿时重新打开侧边栏不被覆盖",
            captured_screenshots,
            out_dir,
        )

        ensure_english_ime(hwnd)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.4)
        with keyboard.pressed(mod_key):
            keyboard.press("c")
            keyboard.release("c")
        time.sleep(0.8)
        print(f"[1243] Stage 2 Clipboard text: '{clipboard.get().strip()}'")

        # =========================================================================
        # Stage 3: No document selection leaves chat input empty
        # =========================================================================
        print("\n[1243] --- Stage 3: No selection empty input ---")
        # Clear chat input
        ensure_english_ime(hwnd)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.3)
        keyboard.press(Key.backspace)
        keyboard.release(Key.backspace)
        time.sleep(0.5)

        # Close sidebar
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(2.0)

        # Deselect in document
        mouse.position = doc_pos
        mouse.click(Button.left)
        time.sleep(0.3)
        keyboard.press(Key.right)
        keyboard.release(Key.right)
        time.sleep(0.5)

        # Reopen sidebar
        print("[1243] Opening sidebar with no selection...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(3.0)

        capture_stage_screenshot(
            "1243_stage3_empty_selection.png",
            "无选区：文档无选区时打开侧边栏输入区保持为空",
            captured_screenshots,
            out_dir,
        )

        # =========================================================================
        # Stage 4: Dialogue round 1 sent -> Prefill round 2 selection
        # =========================================================================
        print("\n[1243] --- Stage 4: Prefill after Round 1 AI dialogue ---")
        # In chat input, insert Round 1 question and press Enter to send
        ensure_english_ime(hwnd)
        insert_doc_text(keyboard, clipboard, "What is Mogan STEM?", mod_key)
        time.sleep(0.5)
        print("[1243] Sending Round 1 message via Enter...")
        keyboard.press(Key.enter)
        keyboard.release(Key.enter)
        time.sleep(2.5)

        # Close sidebar
        print("[1243] Closing sidebar after Round 1...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(2.0)

        # In document, select Round 2 text
        mouse.position = doc_pos
        mouse.click(Button.left)
        time.sleep(0.5)
        round2_doc_text = "Stage4SelectionRound2"
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.3)
        ensure_english_ime(hwnd)
        insert_doc_text(keyboard, clipboard, round2_doc_text, mod_key)
        time.sleep(0.5)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.8)

        # Reopen sidebar: should show 1 round history AND prefill round 2 text in input
        print("[1243] Reopening sidebar (1 round history + new selection prefill)...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(3.0)

        capture_stage_screenshot(
            "1243_stage4_after_round1_prefill.png",
            "1 轮对话后预填：侧边栏已有 1 轮历史对话，新选区成功预填",
            captured_screenshots,
            out_dir,
        )

        ensure_english_ime(hwnd)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.4)
        with keyboard.pressed(mod_key):
            keyboard.press("c")
            keyboard.release("c")
        time.sleep(0.8)
        print(f"[1243] Stage 4 Clipboard text: '{clipboard.get().strip()}'")

        # =========================================================================
        # Stage 5: Dialogue round 2 sent -> Prefill round 3 selection
        # =========================================================================
        print("\n[1243] --- Stage 5: Prefill after Round 2 AI dialogue ---")
        # Send Round 2 message via Enter
        print("[1243] Sending Round 2 message via Enter...")
        keyboard.press(Key.enter)
        keyboard.release(Key.enter)
        time.sleep(2.5)

        # Close sidebar
        print("[1243] Closing sidebar after Round 2...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(2.0)

        # In document, select Round 3 text
        mouse.position = doc_pos
        mouse.click(Button.left)
        time.sleep(0.5)
        round3_doc_text = "Stage5SelectionRound3"
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.3)
        ensure_english_ime(hwnd)
        insert_doc_text(keyboard, clipboard, round3_doc_text, mod_key)
        time.sleep(0.5)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.8)

        # Reopen sidebar: should show 2 rounds history AND prefill round 3 text in input
        print("[1243] Reopening sidebar (2 rounds history + new selection prefill)...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(3.0)

        capture_stage_screenshot(
            "1243_stage5_after_round2_prefill.png",
            "2 轮对话后预填：侧边栏已有 2 轮历史对话，新选区成功预填",
            captured_screenshots,
            out_dir,
        )

        ensure_english_ime(hwnd)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.4)
        with keyboard.pressed(mod_key):
            keyboard.press("c")
            keyboard.release("c")
        time.sleep(0.8)
        print(f"[1243] Stage 5 Clipboard text: '{clipboard.get().strip()}'")

    finally:
        print("\n[1243] Terminating Mogan...")
        terminate_process(proc)
        clipboard.close()

    print("\n" + "=" * 80)
    print("[1243] 测试流程执行完毕，已生成以下阶段截图：")
    for stage_num, desc, path in captured_screenshots:
        print(f"  - Stage {stage_num}: {desc}")
        print(f"    截图路径: {path}")
    print("=" * 80)
    print("[1243] 【重要提示】自动化脚本不直接断言最终结论，必须由 Agent 阅读以上截图判定是否通过测试！")
    print("=" * 80 + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
