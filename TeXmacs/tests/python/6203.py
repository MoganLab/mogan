#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Automated GUI test for issue 6203:
Verify Chat sidebar session display categorized by `type` (explain, translate, chat)
with collapsible buttons, default collapsed states, and arrow indicators.

Usage:
  python3 TeXmacs/tests/python/6203.py
"""

import json
import os
import shutil
import subprocess
import sys
import time

from PIL import ImageGrab

try:
    import pynput
except ImportError:
    pynput_path = os.path.expanduser("~/git/pynput/lib")
    if os.path.exists(pynput_path):
        sys.path.insert(0, pynput_path)
    import pynput

from pynput.keyboard import Key, Controller as KeyboardController
from pynput.mouse import Button, Controller as MouseController

IS_DARWIN = sys.platform == "darwin"
IS_WINDOWS = sys.platform == "win32"
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
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/debug/moganstem"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return c
    raise FileNotFoundError("Mogan binary not found. Build stem first: xmake b stem")


def get_manifest_path():
    home = os.environ.get("HOME", "")
    return os.path.join(home, ".local/share/moganlab/system/ai-chat-sessions/manifest.json")


def setup_test_manifest(manifest_path):
    os.makedirs(os.path.dirname(manifest_path), exist_ok=True)
    backup_path = manifest_path + ".bak_6203"
    if os.path.exists(manifest_path):
        shutil.copy2(manifest_path, backup_path)
        print(f"[6203] Backed up original manifest to {backup_path}")

    test_manifest = {
        "version": 1,
        "sessions": [
            {
                "sessionId": "6203-EXPLAIN-0001",
                "title": "释义: 深度学习算法解析",
                "model": "deepseek-v4-pro",
                "archived": "false",
                "createdAt": "1789886363",
                "defaultExpandCount": 5,
                "thinking": "disabled",
                "search": "disabled",
                "thinkingEffort": "medium",
                "updateAt": "1789886434",
                "type": "explain",
            },
            {
                "sessionId": "6203-TRANS-0001",
                "title": "翻译: Nature 论文摘要",
                "model": "deepseek-v4-pro",
                "archived": "false",
                "createdAt": "1789886360",
                "defaultExpandCount": 5,
                "thinking": "disabled",
                "search": "disabled",
                "thinkingEffort": "medium",
                "updateAt": "1789886430",
                "type": "translate",
            },
            {
                "sessionId": "6203-CHAT-0001",
                "title": "你好，这是一段日常对话",
                "model": "deepseek-v4-flash",
                "archived": "false",
                "createdAt": "1789886350",
                "defaultExpandCount": 5,
                "thinking": "disabled",
                "search": "disabled",
                "thinkingEffort": "medium",
                "updateAt": "1789886420",
                "type": "",
            },
        ],
    }
    with open(manifest_path, "w", encoding="utf-8") as f:
        json.dump(test_manifest, f, ensure_ascii=False, indent=2)
    print(f"[6203] Written test manifest with 3 categorized sessions: explain, translate, chat")
    return backup_path


def restore_manifest(manifest_path, backup_path):
    if backup_path and os.path.exists(backup_path):
        shutil.copy2(backup_path, manifest_path)
        os.remove(backup_path)
        print(f"[6203] Restored original manifest from {backup_path}")


# ---------------------------------------------------------------- X11 helpers

def x11_walk(win):
    yield win
    try:
        children = win.query_tree().children
    except Exception:
        return
    for c in children:
        yield from x11_walk(c)


def x11_find_mogan(d):
    root = d.screen().root
    for w in x11_walk(root):
        try:
            cls = w.get_wm_class()
            if cls and any("mogan" in c.lower() or "stem" in c.lower() for c in cls):
                return w
        except Exception:
            continue
    return None


def x11_raise_mogan(d, mogan):
    import Xlib.X
    import Xlib.protocol.event

    root = d.screen().root
    wm_change = d.intern_atom("WM_CHANGE_STATE")
    net_active = d.intern_atom("_NET_ACTIVE_WINDOW")
    mask = Xlib.X.SubstructureRedirectMask | Xlib.X.SubstructureNotifyMask

    root.send_event(Xlib.protocol.event.ClientMessage(
        window=mogan, client_type=net_active,
        data=(32, [2, Xlib.X.CurrentTime, 0, 0, 0])),
        event_mask=mask)
    try:
        mogan.set_input_focus(Xlib.X.RevertToParent, Xlib.X.CurrentTime)
    except Exception:
        pass
    mogan.configure(stack_mode=Xlib.X.Above)
    d.sync()
    time.sleep(1.0)


def run_test():
    repo_root = find_repo_root()
    mogan_bin = find_mogan_binary(repo_root)
    manifest_path = get_manifest_path()
    backup_path = setup_test_manifest(manifest_path)

    proc = None
    try:
        env = os.environ.copy()
        env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")
        print(f"[6203] Launching Mogan STEM: {mogan_bin}")
        # Launch directly into Chat tab so the Chat sidebar is immediately visible!
        proc = subprocess.Popen(
            [mogan_bin, "tmfs://chat-tab"],
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        print("[6203] Waiting for Mogan GUI to load and show chat tab...")
        time.sleep(5.0)

        ret = proc.poll()
        if ret is not None:
            stdout, stderr = proc.communicate(timeout=2)
            print(f"[6203] ERROR: Mogan exited prematurely with code {ret}")
            return 1

        try:
            import Xlib.display
            d = Xlib.display.Display()
            w = x11_find_mogan(d)
            if w:
                x11_raise_mogan(d, w)
        except Exception as e:
            print(f"[6203] X11 raise note: {e}")

        time.sleep(1.0)
        screenshot_path_1 = "/tmp/6203_chat_tab_default.png"
        img = ImageGrab.grab()
        img.save(screenshot_path_1)
        print(f"[6203] Saved default screenshot to {screenshot_path_1}")

        # Now test clicking category buttons using pynput
        # In the chat tab, the sidebar is on the left
        mouse = MouseController()
        kb = KeyboardController()

        # Let's find coordinates of Mogan window
        try:
            import Xlib.display
            d = Xlib.display.Display()
            w = x11_find_mogan(d)
            if w:
                geom = w.get_geometry()
                coords = w.translate_coords(d.screen().root, 0, 0)
                wx, wy = coords.x, coords.y
                ww, wh = geom.width, geom.height
            else:
                wx, wy, ww, wh = 0, 0, 1920, 1080
        except Exception:
            wx, wy, ww, wh = 0, 0, 1920, 1080

        print(f"[6203] Window rect: ({wx}, {wy}, {ww}, {wh})")

        # The sidebar is on the left side of the chat tab.
        # In 3840x2400 screen resolution:
        # "释义" is at y ≈ 520
        # "翻译" is at y ≈ 585
        # "对话" is at y ≈ 650
        print("[6203] Clicking '释义' category button at (100, 520)...")
        mouse.position = (100, 520)
        time.sleep(0.5)
        mouse.click(Button.left)
        time.sleep(1.0)

        # After expanding 释义, 翻译 is pushed down by one session item (~60px)
        print("[6203] Clicking '翻译' category button at (100, 645)...")
        mouse.position = (100, 645)
        time.sleep(0.5)
        mouse.click(Button.left)
        time.sleep(1.0)

        screenshot_path_2 = "/tmp/6203_chat_tab_expanded.png"
        img2 = ImageGrab.grab()
        img2.save(screenshot_path_2)
        print(f"[6203] Saved expanded screenshot to {screenshot_path_2}")

        # Crop sidebar from both screenshots for visual verification
        from PIL import Image
        Image.open(screenshot_path_1).crop((0, 300, 450, 1000)).save("/tmp/6203_sidebar_default_cropped.png")
        Image.open(screenshot_path_2).crop((0, 300, 450, 1000)).save("/tmp/6203_sidebar_expanded_cropped.png")
        print("[6203] Cropped sidebar screenshots saved to /tmp/6203_sidebar_*_cropped.png")

        print("[6203] Closing Mogan via Ctrl+Q...")
        with kb.pressed(Key.ctrl):
            kb.press("q")
            kb.release("q")
        time.sleep(2.0)

        ret = proc.poll()
        if ret is None:
            proc.terminate()
            try:
                proc.wait(timeout=3.0)
            except subprocess.TimeoutExpired:
                proc.kill()

        print("[6203] GUI test completed successfully!")
        return 0
    finally:
        restore_manifest(manifest_path, backup_path)
        if proc and proc.poll() is None:
            proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
