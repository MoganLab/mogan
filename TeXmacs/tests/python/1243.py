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
import subprocess
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
                return ""
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
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/debug/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/debug/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def focus_mogan_window():
    """Ensure Mogan window is raised and focused across platforms."""
    if sys.platform == "darwin":
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


def capture_stage_screenshot(filename, description, stage_list):
    """Capture full screen for agent visual verification and record stage info."""
    path = os.path.join("/tmp", filename)
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

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    clipboard = ClipboardManager()
    keyboard = KeyboardController()
    mouse = MouseController()
    mod_key = Key.cmd if sys.platform == "darwin" else Key.ctrl
    captured_screenshots = []

    print("[1243] Launching Mogan...")
    proc = subprocess.Popen([bin_path], env=env)

    try:
        time.sleep(3.5)
        focus_mogan_window()
        time.sleep(0.5)

        # Click inside the window to guarantee focus
        mouse.position = (500, 500)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(0.5)

        # =========================================================================
        # Setup: Create a new document tab
        # =========================================================================
        print("\n[1243] --- Setup: Creating new tab ---")
        with keyboard.pressed(mod_key):
            keyboard.press("t")
            keyboard.release("t")
        time.sleep(2.0)

        # =========================================================================
        # Stage 1: Initial state - Selection prefilled on opening sidebar
        # =========================================================================
        print("\n[1243] --- Stage 1: Initial state prefill ---")
        stage1_text = "Stage1SelectionInitial"
        print(f"[1243] Typing document text: '{stage1_text}'...")
        keyboard.type(stage1_text)
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
        )

        # Copy input to check clipboard
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
        keyboard.type("_PreservedDraft")
        time.sleep(0.5)

        print("[1243] Closing sidebar (mod+J)...")
        with keyboard.pressed(mod_key):
            keyboard.press("j")
            keyboard.release("j")
        time.sleep(2.0)

        # Modify document selection
        mouse.position = (350, 350)
        mouse.click(Button.left)
        time.sleep(0.5)
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.3)
        keyboard.type("OtherDocumentSelection")
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
        )

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
        mouse.position = (350, 350)
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
        )

        # =========================================================================
        # Stage 4: Dialogue round 1 sent -> Prefill round 2 selection
        # =========================================================================
        print("\n[1243] --- Stage 4: Prefill after Round 1 AI dialogue ---")
        # In chat input, type Round 1 question and press Enter to send
        keyboard.type("What is Mogan STEM?")
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
        mouse.position = (350, 350)
        mouse.click(Button.left)
        time.sleep(0.5)
        round2_doc_text = "Stage4SelectionRound2"
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.3)
        keyboard.type(round2_doc_text)
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
        )

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
        mouse.position = (350, 350)
        mouse.click(Button.left)
        time.sleep(0.5)
        round3_doc_text = "Stage5SelectionRound3"
        with keyboard.pressed(mod_key):
            keyboard.press("a")
            keyboard.release("a")
        time.sleep(0.3)
        keyboard.type(round3_doc_text)
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
        )

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
