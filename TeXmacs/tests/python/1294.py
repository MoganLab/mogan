#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""1294.py: 复现与验证 LaTeX 自递归宏粘贴崩溃的自动化测试脚本。

支持用例:
  - 1294_1.tex: 无参自递归宏 (用例 249)
  - 1294_2.tex: 带参自递归宏 (用例 289)
  - 1294_3.tex: 相互递归宏 (crash_pattern_c)

启动命令: xmake r stem -d
用法:
  python3 TeXmacs/tests/python/1294.py               # 默认依序测试 1294_1.tex, 1294_2.tex, 1294_3.tex
  python3 TeXmacs/tests/python/1294.py [path/to.tex] # 测试指定文件
"""

import os
import sys
import glob
import time
import subprocess
import threading

from Xlib import X, display, Xatom
from Xlib.protocol import event
from pynput.keyboard import Controller as Kbd, Key
from pynput.mouse import Controller as Mouse, Button

# 定位工程根目录
_cur = os.path.dirname(os.path.abspath(__file__))
while _cur and _cur != "/" and not os.path.isdir(os.path.join(_cur, "TeXmacs")):
    _cur = os.path.dirname(_cur)
ROOT_DIR = _cur if _cur else os.path.dirname(os.path.abspath(__file__))
HERE = ROOT_DIR

# 4K (3840x2312) 实测坐标
POS_DOC = (960, 900)
POS_EDIT_MENU = (220, 140)
POS_PASTE_FROM = (227, 1008)
POS_LATEX_ITEM = (618, 1135)

DRAFT_DIRS = [
    os.path.expanduser("~/文档/LiiiSTEM/no_name"),
    os.path.expanduser("~/Documents/LiiiSTEM/no_name"),
]

DEFAULT_CASES = [
    os.path.join(ROOT_DIR, "TeXmacs", "tests", "tex", "1294_1.tex"),
    os.path.join(ROOT_DIR, "TeXmacs", "tests", "tex", "1294_2.tex"),
    os.path.join(ROOT_DIR, "TeXmacs", "tests", "tex", "1294_3.tex"),
]


class X11Clipboard:
    """X11 剪贴板持有者，在后台响应 SelectionRequest。"""

    def __init__(self):
        self.disp = display.Display()
        self.screen = self.disp.screen()
        self.window = self.screen.root.create_window(
            0, 0, 1, 1, 0, self.screen.root_depth
        )
        self.CLIPBOARD = self.disp.intern_atom('CLIPBOARD')
        self.PRIMARY = self.disp.intern_atom('PRIMARY')
        self.TARGETS = self.disp.intern_atom('TARGETS')
        self.UTF8 = self.disp.intern_atom('UTF8_STRING')
        self.TEXT = self.disp.intern_atom('TEXT')
        self.STRING = Xatom.STRING
        self.text = ""
        self.running = True
        self.thread = threading.Thread(target=self._loop, daemon=True)
        self.thread.start()

    def set_text(self, text):
        self.text = text
        self.window.set_selection_owner(self.CLIPBOARD, X.CurrentTime)
        self.window.set_selection_owner(self.PRIMARY, X.CurrentTime)
        self.disp.flush()

    def _loop(self):
        while self.running:
            try:
                if self.disp.pending_events() == 0:
                    time.sleep(0.01)
                    continue
                e = self.disp.next_event()
                if e.type == X.SelectionRequest:
                    req_win = self.disp.create_resource_object('window', e.requestor)
                    prop = e.property
                    if prop == X.NONE:
                        prop = e.target
                    if e.target == self.TARGETS:
                        req_win.change_property(
                            prop, Xatom.ATOM, 32,
                            [self.TARGETS, self.UTF8, self.STRING, self.TEXT]
                        )
                        ev = event.SelectionNotify(
                            time=e.time, requestor=e.requestor,
                            selection=e.selection, target=e.target,
                            property=prop
                        )
                    elif e.target in (self.UTF8, self.STRING, self.TEXT):
                        req_win.change_property(
                            prop, e.target, 8, self.text.encode('utf-8')
                        )
                        ev = event.SelectionNotify(
                            time=e.time, requestor=e.requestor,
                            selection=e.selection, target=e.target,
                            property=prop
                        )
                    else:
                        ev = event.SelectionNotify(
                            time=e.time, requestor=e.requestor,
                            selection=e.selection, target=e.target,
                            property=X.NONE
                        )
                    req_win.send_event(ev)
                    self.disp.flush()
            except Exception:
                pass

    def stop(self):
        self.running = False


def clean_drafts():
    """清理自动保存草稿，防止弹窗阻碍启动。"""
    for d in DRAFT_DIRS:
        if os.path.isdir(d):
            for f in glob.glob(os.path.join(d, "draft_*.tmu")):
                try:
                    os.remove(f)
                except OSError:
                    pass


def activate_window():
    """激活 Mogan STEM 窗口。"""
    js_code = (
        'const wins = workspace.windowList ? workspace.windowList() : workspace.windows();\n'
        'for (let i = 0; i < wins.length; i++) {\n'
        '    const w = wins[i];\n'
        '    const cls = (w.resourceClass || "").toLowerCase();\n'
        '    const name = (w.resourceName || "").toLowerCase();\n'
        '    const title = (w.caption || "");\n'
        '    if (cls.includes("stem") || name.includes("stem") || title.includes("Mogan")) {\n'
        '        workspace.activeWindow = w;\n'
        '        if (w.activate) w.activate();\n'
        '    }\n'
        '}\n'
    )
    tmp_js = "/tmp/activate_moganstem.js"
    with open(tmp_js, "w", encoding="utf-8") as f:
        f.write(js_code)

    try:
        import dbus
        bus = dbus.SessionBus()
        kwin = bus.get_object("org.kde.KWin", "/Scripting")
        iface = dbus.Interface(kwin, "org.kde.kwin.Scripting")
        script_id = iface.loadScript(tmp_js)
        script_obj = bus.get_object("org.kde.KWin", f"/Scripting/Script{script_id}")
        script_iface = dbus.Interface(script_obj, "org.kde.kwin.Script")
        script_iface.run()
        iface.unloadScript(str(script_id))
    except Exception:
        pass

    try:
        d = display.Display()
        root = d.screen().root
        NET_ACTIVE_WINDOW = d.intern_atom('_NET_ACTIVE_WINDOW')
        NET_CLIENT_LIST = d.intern_atom('_NET_CLIENT_LIST')
        prop = root.get_full_property(NET_CLIENT_LIST, Xatom.WINDOW)
        if prop:
            for wid in prop.value:
                win = d.create_resource_object('window', wid)
                c = str(win.get_wm_class() or "").lower()
                if "stem" in c or "mogan" in c:
                    data = [2, X.CurrentTime, 0, 0, 0]
                    ev = event.ClientMessage(window=wid, client_type=NET_ACTIVE_WINDOW, data=(32, data))
                    root.send_event(ev, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
                    d.flush()
                    break
    except Exception:
        pass


def is_window_ready():
    try:
        out = subprocess.check_output(
            ["xprop", "-root", "_NET_CLIENT_LIST"],
            text=True, stderr=subprocess.DEVNULL
        )
        for wid in out.split("#")[-1].replace(",", " ").split():
            c = subprocess.check_output(
                ["xprop", "-id", wid, "WM_CLASS"],
                text=True, stderr=subprocess.DEVNULL
            )
            if "moganstem" in c.lower() or "liiistem" in c.lower():
                return True
    except Exception:
        pass
    return False


class TestRunner:
    def __init__(self):
        self.proc = None
        self.clip = X11Clipboard()
        self.kbd = Kbd()
        self.mouse = Mouse()

    def start_app(self):
        self.stop_app()
        clean_drafts()
        subprocess.run(["pkill", "-9", "-f", "moganstem"], stderr=subprocess.DEVNULL)

        print("[1294] Starting Mogan STEM via: xmake r stem -d")
        self.proc = subprocess.Popen(["xmake", "r", "stem", "-d"], cwd=HERE)

        t0 = time.time()
        while time.time() - t0 < 15:
            if is_window_ready():
                break
            time.sleep(0.3)

        time.sleep(1.0)
        activate_window()
        time.sleep(0.5)

    def is_alive(self):
        if self.proc is None:
            return False
        return self.proc.poll() is None

    def stop_app(self):
        if self.proc is not None:
            if self.proc.poll() is None:
                self.proc.terminate()
                try:
                    self.proc.wait(timeout=2)
                except Exception:
                    self.proc.kill()
            self.proc = None
        subprocess.run(["pkill", "-9", "-f", "moganstem"], stderr=subprocess.DEVNULL)
        clean_drafts()

    def run_case(self, case_name, tex_content):
        """运行单个用例的粘贴测试。"""
        if not self.is_alive():
            self.start_app()

        print(f"\n--- Running: {case_name} ---")
        print(f"Content: {tex_content.strip()}")

        # 新建标签页并聚焦
        activate_window()
        time.sleep(0.3)
        self.kbd.press(Key.ctrl)
        self.kbd.tap('t')
        self.kbd.release(Key.ctrl)
        time.sleep(0.8)

        self.mouse.position = POS_DOC
        time.sleep(0.2)
        self.mouse.click(Button.left)
        time.sleep(0.4)

        # 设置剪贴板并粘贴
        self.clip.set_text(tex_content)
        activate_window()
        time.sleep(0.2)

        print("  -> Clicking: Edit -> Paste from -> LaTeX")
        self.mouse.position = POS_EDIT_MENU
        time.sleep(0.3)
        self.mouse.click(Button.left)
        time.sleep(0.4)

        self.mouse.position = POS_PASTE_FROM
        time.sleep(0.4)

        self.mouse.position = POS_LATEX_ITEM
        time.sleep(0.3)
        self.mouse.click(Button.left)

        # 检查粘贴阶段是否崩溃
        t_wait = 0.0
        crashed = False
        while t_wait < 3.5:
            time.sleep(0.2)
            t_wait += 0.2
            if not self.is_alive():
                crashed = True
                print("  [!] Crashed during paste!")
                break

        # 处理可能的错误弹窗并关闭
        if not crashed:
            time.sleep(0.5)
            print("  -> Closing possible error popup (Enter)...")
            self.kbd.tap(Key.enter)
            time.sleep(0.5)
            if not self.is_alive():
                crashed = True
                print("  [!] Crashed after dismissing popup!")

        # 关闭弹窗后，继续编辑测试（防止损坏状态下继续操作引起崩溃）
        if not crashed:
            print("  -> Testing continue editing after paste...")
            activate_window()
            time.sleep(0.3)
            self.mouse.position = POS_DOC
            time.sleep(0.2)
            self.mouse.click(Button.left)
            time.sleep(0.3)

            # 键入文本
            self.kbd.type("testing edit after paste ")
            time.sleep(0.3)
            # 回车触发分段排版
            self.kbd.tap(Key.enter)
            time.sleep(0.3)
            # 键入更多内容
            self.kbd.type("continue typing 12345")
            time.sleep(0.3)
            # 回车
            self.kbd.tap(Key.enter)
            time.sleep(0.3)
            # 退格删除
            for _ in range(8):
                self.kbd.tap(Key.backspace)
                time.sleep(0.05)
            # 光标移动
            self.kbd.tap(Key.up)
            time.sleep(0.1)
            self.kbd.tap(Key.down)
            time.sleep(0.1)
            self.kbd.type(" finished")
            time.sleep(0.5)

            # 等待排版和可能的后台处理
            t_wait = 0.0
            while t_wait < 2.5:
                time.sleep(0.2)
                t_wait += 0.2
                if not self.is_alive():
                    crashed = True
                    print("  [!] Crashed during continue editing!")
                    break

        status = "CRASH" if crashed else "PASS"
        print(f"  Result for {case_name}: {status}")
        return not crashed

    def close(self):
        self.clip.stop()
        self.stop_app()


def main():
    if len(sys.argv) > 1:
        test_files = sys.argv[1:]
    else:
        test_files = [f for f in DEFAULT_CASES if os.path.exists(f)]
        if not test_files:
            print("[!] Default test cases not found in TeXmacs/tests/tex/!")
            sys.exit(1)

    runner = TestRunner()
    results = {}

    try:
        for tf in test_files:
            name = os.path.basename(tf)
            with open(tf, "r", encoding="utf-8") as f:
                content = f.read().strip()
            passed = runner.run_case(name, content)
            results[name] = "PASS" if passed else "CRASH"

        print("\n" + "=" * 50)
        print("TEST SUMMARY:")
        print("=" * 50)
        all_passed = True
        for name, status in results.items():
            print(f"  {name:20s}: {status}")
            if status != "PASS":
                all_passed = False

        sys.exit(0 if all_passed else 1)

    finally:
        runner.close()


if __name__ == "__main__":
    main()
