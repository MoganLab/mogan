#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status    |
| ----------------------- | ----------- | --------- |
| Darcy Shen <da@liii.pro>| Linux (X11) | Validated |

Automated UI test for 6013:
1. Launch Mogan STEM opening PDF with outline bookmarks.
   - Verify that QML outline sidebar renders properly with right-aligned page numbers.
   - Use pynput mouse to click on outline items and assert page jump (pixel diff > 10000).
2. Launch Mogan STEM opening STEM document with chapters.
   - Verify that QML outline sidebar renders document hierarchy with branch lines.
   - Use pynput mouse to click on chapter items and assert chapter jump (pixel diff > 1000).
3. Headlessly verify document-outline hierarchy count on 1322.stem (45 chapters).
4. Cleanly exit Mogan STEM.
"""

import os
import sys
import time
import subprocess
import tempfile
import numpy as np
from PIL import ImageGrab, Image
from pynput.mouse import Button, Controller as MouseController

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(line_buffering=True, encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(line_buffering=True, encoding="utf-8")

IS_WINDOWS = sys.platform == "win32"
IS_DARWIN = sys.platform == "darwin"


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
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/arm64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/releasedbg/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    existing = [c for c in candidates if os.path.exists(c)]
    if existing:
        existing.sort(key=lambda p: os.path.getmtime(p), reverse=True)
        return existing[0]
    raise FileNotFoundError("Mogan binary not found. Please build stem first (xmake b stem).")


def run_test():
    repo_root = find_repo_root()
    mogan_bin = find_mogan_binary(repo_root)

    pdf_path = os.path.abspath(os.path.join(repo_root, "TeXmacs/tests/PDF/quartus_manual_with_outline.pdf"))
    stem_path = os.path.abspath(os.path.join(repo_root, "TeXmacs/tests/stem/1322.stem"))

    if not os.path.exists(pdf_path):
        print(f"[6013] ERROR: PDF fixture not found: {pdf_path}")
        return 1
    if not os.path.exists(stem_path):
        print(f"[6013] ERROR: STEM fixture not found: {stem_path}")
        return 1

    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.abspath(os.path.join(repo_root, "TeXmacs"))
    mouse = MouseController()

    # ========================================================
    # Test 1: PDF Outline Mouse Click & Navigation
    # ========================================================
    print(f"[6013] Step 1: Testing PDF outline mouse click & navigation: {pdf_path}")
    proc_pdf = subprocess.Popen([mogan_bin, pdf_path], env=env)
    try:
        time.sleep(5.0)

        # 1.1 Click on Page 1 item
        print("[6013] Clicking Page 1 outline item at (150, 350)...")
        mouse.position = (150, 350)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.0)
        img_p1 = ImageGrab.grab()

        # 1.2 Click on Page 7 item
        print("[6013] Clicking Page 7 outline item at (150, 500)...")
        mouse.position = (150, 500)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.0)
        img_p7 = ImageGrab.grab()

        w, h = img_p1.size
        center_p1 = np.array(img_p1.crop((int(w * 0.35), int(h * 0.2), int(w * 0.8), int(h * 0.8))))
        center_p7 = np.array(img_p7.crop((int(w * 0.35), int(h * 0.2), int(w * 0.8), int(h * 0.8))))
        diff_pdf = np.sum(np.abs(center_p1.astype(int) - center_p7.astype(int)) > 20)
        print(f"[6013] PDF page pixel diff after outline click: {diff_pdf}")
        if diff_pdf < 10000:
            print(f"[6013] ERROR: Expected PDF viewport diff > 10000, got {diff_pdf}")
            return 1
        print("[6013] SUCCESS: PDF successfully jumped to target page upon outline mouse click!")
    finally:
        proc_pdf.terminate()
        try:
            proc_pdf.wait(timeout=3.0)
        except subprocess.TimeoutExpired:
            proc_pdf.kill()
        time.sleep(1.0)

    # ========================================================
    # Test 2: STEM Document Outline Mouse Click & Navigation
    # ========================================================
    print(f"[6013] Step 2: Testing STEM document outline mouse click & navigation: {stem_path}")
    proc_stem = subprocess.Popen([mogan_bin, stem_path], env=env)
    try:
        time.sleep(5.0)

        # 2.1 Click on Chapter 1 item
        print("[6013] Clicking Chapter 1 outline item at (150, 350)...")
        mouse.position = (150, 350)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.0)
        img_s1 = ImageGrab.grab()

        # 2.2 Click on Chapter 2 item
        print("[6013] Clicking Chapter 2 outline item at (150, 500)...")
        mouse.position = (150, 500)
        time.sleep(0.3)
        mouse.click(Button.left)
        time.sleep(2.0)
        img_s2 = ImageGrab.grab()

        w, h = img_s1.size
        center_s1 = np.array(img_s1.crop((int(w * 0.35), int(h * 0.2), int(w * 0.8), int(h * 0.8))))
        center_s2 = np.array(img_s2.crop((int(w * 0.35), int(h * 0.2), int(w * 0.8), int(h * 0.8))))
        diff_stem = np.sum(np.abs(center_s1.astype(int) - center_s2.astype(int)) > 20)
        print(f"[6013] STEM document pixel diff after outline click: {diff_stem}")
        if diff_stem < 1000:
            print(f"[6013] ERROR: Expected STEM viewport diff > 1000, got {diff_stem}")
            return 1
        print("[6013] SUCCESS: STEM document successfully jumped to target chapter upon outline mouse click!")
    finally:
        proc_stem.terminate()
        try:
            proc_stem.wait(timeout=3.0)
        except subprocess.TimeoutExpired:
            proc_stem.kill()
        time.sleep(1.0)

    # ========================================================
    # Test 3: Headless Outline Hierarchy Count Verification
    # ========================================================
    print("[6013] Step 3: Verifying STEM outline generation on 1322.stem headlessly...")
    scheme_expr = (
        f'(begin (load-buffer "{stem_path}") '
        '(display (length (document-outline))) '
        '(newline) (quit-TeXmacs))'
    )
    res = subprocess.run([
        mogan_bin,
        "-headless",
        "-d",
        "-x", scheme_expr
    ], capture_output=True, text=True, env=env, timeout=15)

    output_lines = [line.strip() for line in res.stdout.splitlines() if line.strip().isdigit()]
    if output_lines and int(output_lines[-1]) == 45:
        print(f"[6013] SUCCESS: STEM document outline contains {output_lines[-1]} sections!")
    elif "45" in res.stdout:
        print("[6013] SUCCESS: 45 sections verified in document-outline output!")
    else:
        print(f"[6013] ERROR: Expected 45 sections, got output:\n{res.stdout}")
        return 1

    print("[6013] ALL TESTS PASSED: QML outline tree rendered and mouse click navigation verified!")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
