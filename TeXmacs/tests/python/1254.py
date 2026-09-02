#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status |
| ----------------------- | ----------- | ------ |
| Darcy Shen <da@liii.pro>| Linux (X11) | Passed |

Automated end-to-end UI test for issue 1254:
Verify that inserting headings (chapter / section / subsection / subsubsection / paragraph / subparagraph)
via Alt+0..5 shortcuts while cursor is inside an existing heading automatically exits
the current heading structure, inserts a newline, and creates the new heading without
breaking, splitting, or improperly nesting inside the existing heading.
"""

import os
import sys
import time
import re
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


def extract_headings(tmu_content):
    """Extract all headings in the body tag."""
    body_match = re.search(r"<\\body>(.*?)</body>", tmu_content, re.DOTALL)
    if not body_match:
        return []
    body_content = body_match.group(1)
    pattern = re.compile(r"<(chapter|section|subsection|subsubsection|paragraph|subparagraph)\|([^>]+)>")
    return pattern.findall(body_content)


def run_test():
    repo_root = find_repo_root()
    bin_path = find_mogan_binary(repo_root)
    no_name_dir = find_no_name_dir()
    print(f"[1254] Using Mogan binary: {bin_path}")
    print(f"[1254] Monitoring draft directory: {no_name_dir}")

    existing_files = set(os.listdir(no_name_dir)) if os.path.exists(no_name_dir) else set()

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")

    created_temp_files = []

    print("\n[1254] --- Step 1: Launching Mogan and opening a new document ---")
    proc = subprocess.Popen([bin_path], env=env)
    keyboard = KeyboardController()
    mouse = MouseController()

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
        print("[1254] Creating new tab (Ctrl+T)...")
        with keyboard.pressed(Key.ctrl):
            keyboard.press("t")
            keyboard.release("t")
        time.sleep(2.0)

        # =========================================================================
        # Step 2: Insert headings using Alt+0..5 while cursor is inside headings
        # =========================================================================
        print("\n[1254] --- Step 2: Inserting headings while cursor is inside heading structures ---")

        # 1. Insert Chapter 1 (Alt+0)
        print("[1254] [1/9] Inserting Chapter 1 (Alt+0 -> 'Chapter 1')...")
        with keyboard.pressed(Key.alt):
            keyboard.press("0")
            keyboard.release("0")
        time.sleep(0.4)
        keyboard.type("Chapter 1")
        time.sleep(0.4)

        # 2. While cursor is inside Chapter 1, press Alt+1 to insert Section 1
        print("[1254] [2/9] Cursor is inside Chapter 1 -> Pressing Alt+1 to insert Section 1...")
        with keyboard.pressed(Key.alt):
            keyboard.press("1")
            keyboard.release("1")
        time.sleep(0.4)
        keyboard.type("Section 1")
        time.sleep(0.4)

        # 3. While cursor is inside Section 1, press Alt+1 to insert Section 2
        print("[1254] [3/9] Cursor is inside Section 1 -> Pressing Alt+1 to insert Section 2...")
        with keyboard.pressed(Key.alt):
            keyboard.press("1")
            keyboard.release("1")
        time.sleep(0.4)
        keyboard.type("Section 2")
        time.sleep(0.4)

        # 4. While cursor is inside Section 2, press Alt+2 to insert Subsection 2.1
        print("[1254] [4/9] Cursor is inside Section 2 -> Pressing Alt+2 to insert Subsection 2.1...")
        with keyboard.pressed(Key.alt):
            keyboard.press("2")
            keyboard.release("2")
        time.sleep(0.4)
        keyboard.type("Subsection 2.1")
        time.sleep(0.4)

        # 5. While cursor is inside Subsection 2.1, press Alt+2 to insert Subsection 2.2
        print("[1254] [5/9] Cursor is inside Subsection 2.1 -> Pressing Alt+2 to insert Subsection 2.2...")
        with keyboard.pressed(Key.alt):
            keyboard.press("2")
            keyboard.release("2")
        time.sleep(0.4)
        keyboard.type("Subsection 2.2")
        time.sleep(0.4)

        # 6. While cursor is inside Subsection 2.2, press Alt+3 to insert Subsubsection 2.2.1
        print("[1254] [6/9] Cursor is inside Subsection 2.2 -> Pressing Alt+3 to insert Subsubsection 2.2.1...")
        with keyboard.pressed(Key.alt):
            keyboard.press("3")
            keyboard.release("3")
        time.sleep(0.4)
        keyboard.type("Subsubsection 2.2.1")
        time.sleep(0.4)

        # 7. While cursor is inside Subsubsection 2.2.1, press Alt+4 to insert Paragraph 1
        print("[1254] [7/9] Cursor is inside Subsubsection 2.2.1 -> Pressing Alt+4 to insert Paragraph 1...")
        with keyboard.pressed(Key.alt):
            keyboard.press("4")
            keyboard.release("4")
        time.sleep(0.4)
        keyboard.type("Paragraph 1")
        time.sleep(0.4)

        # 8. While cursor is inside Paragraph 1, press Alt+5 to insert Subparagraph 1.1
        print("[1254] [8/9] Cursor is inside Paragraph 1 -> Pressing Alt+5 to insert Subparagraph 1.1...")
        with keyboard.pressed(Key.alt):
            keyboard.press("5")
            keyboard.release("5")
        time.sleep(0.4)
        keyboard.type("Subparagraph 1.1")
        time.sleep(0.4)

        # 9. While cursor is inside Subparagraph 1.1, press Alt+1 to insert Section 3
        print("[1254] [9/9] Cursor is inside Subparagraph 1.1 -> Pressing Alt+1 to insert Section 3...")
        with keyboard.pressed(Key.alt):
            keyboard.press("1")
            keyboard.release("1")
        time.sleep(0.4)
        keyboard.type("Section 3")
        time.sleep(0.4)

        # Take screenshot for diagnosis
        screenshot_path = "/tmp/1254_screenshot.png"
        img = ImageGrab.grab()
        img.save(screenshot_path)
        print(f"[1254] Saved screenshot to {screenshot_path}")

        # Trigger preview (Ctrl+P) to auto-save the draft
        print("[1254] Triggering preview (Ctrl+P) to auto-save draft...")
        with keyboard.pressed(Key.ctrl):
            keyboard.press("p")
            keyboard.release("p")
        time.sleep(3.5)

        # Find newly created draft file
        current_files = set(os.listdir(no_name_dir)) if os.path.exists(no_name_dir) else set()
        diff_files = current_files - existing_files
        tmu_files = [f for f in diff_files if f.endswith(".tmu")]
        for f in diff_files:
            created_temp_files.append(os.path.join(no_name_dir, f))

        if not tmu_files:
            print("[1254] ERROR: No .tmu draft file was created after previewing.")
            return 1

        target_file = os.path.join(no_name_dir, tmu_files[-1])
        print(f"[1254] Inspecting draft file: {target_file}")
        with open(target_file, "r", encoding="utf-8") as f:
            tmu_content = f.read()

        # =========================================================================
        # Step 3: Validate AST and heading structure
        # =========================================================================
        print("\n[1254] --- Step 3: Validating heading structure in saved document ---")
        headings = extract_headings(tmu_content)
        print(f"[1254] Extracted headings: {headings}")

        expected_headings = [
            ("chapter", "Chapter 1"),
            ("section", "Section 1"),
            ("section", "Section 2"),
            ("subsection", "Subsection 2.1"),
            ("subsection", "Subsection 2.2"),
            ("subsubsection", "Subsubsection 2.2.1"),
            ("paragraph", "Paragraph 1"),
            ("subparagraph", "Subparagraph 1.1"),
            ("section", "Section 3"),
        ]

        if headings != expected_headings:
            print(f"[1254] ERROR: Headings do not match expected structure!")
            print(f"  Expected: {expected_headings}")
            print(f"  Actual:   {headings}")
            return 1

        print("[1254] PASS: All headings were correctly inserted without nesting or structure corruption.")

    finally:
        print("\n[1254] Terminating Mogan instance...")
        terminate_process(proc)

        # =========================================================================
        # Step 4: Cleanup temporary draft files
        # =========================================================================
        print("[1254] Cleaning up temporary test files...")
        for p in created_temp_files:
            try:
                if os.path.exists(p):
                    os.remove(p)
                    print(f"[1254] Removed temp file: {p}")
            except Exception as e:
                print(f"[1254] Warning: Failed to remove {p}: {e}")

    print("\n[1254] ALL TESTS PASSED: Smart heading insertion verified!")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
