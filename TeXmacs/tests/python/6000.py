#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Automated test for issue 6000:
PDF exported with background color / dark mode should not have white lines
on the right edge (or other edges) due to rounding errors during rasterization.
"""

import os
import sys
import subprocess
from PIL import Image


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
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
        os.path.join(repo_root, "build/macosx/x86_64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None


def verify_pdf(pdf_path):
    if not os.path.isfile(pdf_path):
        print(f"FAIL: {pdf_path} not found")
        return False

    prefix = "/tmp/test_6000_verify"
    test_dpis = [96, 120, 150, 200, 300]
    for dpi in test_dpis:
        subprocess.run(
            ["pdftoppm", "-r", str(dpi), "-png", pdf_path, prefix],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        png_path = f"{prefix}-1.png"
        if not os.path.isfile(png_path):
            print(f"FAIL: could not render page at dpi {dpi}")
            return False

        im = Image.open(png_path)
        w, h = im.size
        # Check rightmost column
        right_col = set([im.getpixel((w - 1, y)) for y in range(h)])
        if (255, 255, 255) in right_col:
            print(f"FAIL at dpi={dpi}: white line detected in rightmost column: {right_col}")
            return False

        # Check top row
        top_row = set([im.getpixel((x, 0)) for x in range(w)])
        if (255, 255, 255) in top_row:
            print(f"FAIL at dpi={dpi}: white line detected in top row: {top_row}")
            return False

    return True


def main():
    if len(sys.argv) > 1:
        pdf_path = sys.argv[1]
    else:
        repo_root = find_repo_root()
        mogan = find_mogan_binary(repo_root)
        if not mogan:
            print("FAIL: Mogan binary not found")
            sys.exit(1)

        tmu_path = os.path.join(repo_root, "TeXmacs/tests/tmu/6000.tmu")
        pdf_path = "/tmp/test_6000_python.pdf"
        if os.path.exists(pdf_path):
            os.remove(pdf_path)

        env = dict(os.environ)
        env["TEXMACS_PATH"] = os.path.join(repo_root, "TeXmacs")
        cmd = [
            mogan,
            "-headless",
            "-d",
            "-x",
            f'(begin (load-buffer "{tmu_path}" :strict) (export-buffer-to-pdf "{pdf_path}" #f #t) (quit-TeXmacs))',
        ]
        subprocess.run(cmd, env=env, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    if verify_pdf(pdf_path):
        with open("/tmp/test_6000_ok", "w") as f:
            f.write("OK\n")
        print("OK")
        sys.exit(0)
    else:
        print("FAIL")
        sys.exit(1)


if __name__ == "__main__":
    main()
