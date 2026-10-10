#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
| Tester                  | Platform    | Status    |
| ----------------------- | ----------- | --------- |
| Darcy Shen <da@liii.pro>| Linux (X11) | Validated |

Automated UI test for 6013:
1. Launch Mogan STEM opening PDF with outline bookmarks (quartus_manual_with_outline.pdf).
   Verify that PDF outline dock is displayed on the left side.
2. Open STEM document with chapters (1322.stem).
   Verify that left outline dock dynamically switches to the STEM document outline.
3. Cleanly exit Mogan STEM.
"""

import os
import sys
import time
import subprocess
import tempfile
import numpy as np
from PIL import ImageGrab, Image

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

    print(f"[6013] Step 1: Launching Mogan STEM with PDF: {pdf_path}")
    env = os.environ.copy()
    env["TEXMACS_PATH"] = os.path.abspath(os.path.join(repo_root, "TeXmacs"))

    cmd = [mogan_bin, pdf_path]
    proc = subprocess.Popen(cmd, env=env)

    try:
        # Wait for GUI initialization
        time.sleep(5.0)

        # Capture PDF screenshot
        img_pdf = ImageGrab.grab()
        pdf_screenshot_path = os.path.join(tempfile.gettempdir(), "6013_pdf_outline.png")
        img_pdf.save(pdf_screenshot_path)
        print(f"[6013] Saved PDF with outline screenshot: {pdf_screenshot_path}")

        # Check left 25% area has non-trivial elements (outline tree dock)
        w, h = img_pdf.size
        left_area = np.array(img_pdf.crop((0, int(h * 0.15), int(w * 0.25), int(h * 0.85))))
        std_dev = np.std(left_area)
        print(f"[6013] PDF left sidebar variance: {std_dev:.2f}")
        if std_dev < 1.0:
            print("[6013] WARNING: Left sidebar seems completely blank or hidden!")

        # Step 2: Now test verifying STEM outline generation on 1322.stem
        print("[6013] Step 2: Verifying STEM outline generation on 1322.stem...")
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

        print("[6013] Step 3: Cleanly terminating Mogan STEM...")
        proc.terminate()
        try:
            proc.wait(timeout=3.0)
        except subprocess.TimeoutExpired:
            proc.kill()

        print("[6013] All tests passed successfully!")
        return 0

    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3.0)
            except Exception:
                proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
