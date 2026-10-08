#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Automated test suite for Mogan STEM clipboard copy/paste reliability.

Scenarios tested:
  1. Same document: copy plain text, paste.
  2. Same document: copy structure (e.g., section header), paste.
  3. Same document: copy mixed (structure + text / formatted), paste.
  4. Copy plain text, new draft document (Ctrl+T/new-document), paste.
  5. Copy structure, new draft document, paste.
  6. Copy mixed, new draft document, paste.

Variables controlled:
  - Fresh MoganSTEM process per single test run.
  - Test single copy (1x Ctrl+C) and multiple copies (2x, 3x Ctrl+C).
  - Source document: D:/repositories/liii/arxiv/deepseek_v4.stem
"""

import os
import sys
import time
import json
import subprocess
import ctypes
from ctypes import wintypes

# Ensure UTF-8 output
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
        os.path.join(repo_root, "build/packages/stem/data/bin/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/releasedbg/MoganSTEM.exe"),
        os.path.join(repo_root, "build/windows/x64/release/MoganSTEM.exe"),
        os.path.join(repo_root, "build/linux/x86_64/releasedbg/moganstem"),
        os.path.join(repo_root, "build/linux/x86_64/release/moganstem"),
        os.path.join(repo_root, "build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"),
    ]
    for c in candidates:
        if os.path.exists(c):
            return os.path.abspath(c)
    raise FileNotFoundError("MoganSTEM binary not found. Please build stem first (xmake b stem).")


def kill_existing_mogan():
    if IS_WINDOWS:
        subprocess.run(["taskkill", "/F", "/IM", "MoganSTEM.exe"], capture_output=True, check=False)
        time.sleep(0.3)


def read_windows_clipboard_safe():
    """Safely inspect Windows clipboard formats and text without crashing on access violations."""
    if not IS_WINDOWS:
        return {}
    user32 = ctypes.windll.user32
    kernel32 = ctypes.windll.kernel32

    if not user32.OpenClipboard(0):
        return {"error": "cannot_open_clipboard"}

    formats = []
    has_tm_clipboard = False
    has_tm_pid = False

    try:
        fmt = 0
        while True:
            fmt = user32.EnumClipboardFormats(fmt)
            if fmt == 0:
                break
            name_buf = ctypes.create_unicode_buffer(256)
            user32.GetClipboardFormatNameW(fmt, name_buf, 256)
            name = name_buf.value
            formats.append((fmt, name))
            if name == "application/x-texmacs-clipboard":
                has_tm_clipboard = True
            elif name == "application/x-texmacs-pid":
                has_tm_pid = True
    finally:
        user32.CloseClipboard()

    return {
        "formats_count": len(formats),
        "has_tm_clipboard": has_tm_clipboard,
        "has_tm_pid": has_tm_pid,
    }


def generate_test_scm(case_id, repeat_copies, result_path):
    escaped_result_path = result_path.replace("\\", "/")

    scm = f"""
(use-modules (utils library tree))

(define (record-result res)
  (string-save (object->string res) (unix->url "{escaped_result_path}"))
  (quit-TeXmacs))

(define (run-same-doc target-node)
  (let* ((bt (buffer-tree)))
    (tree-select bt target-node)
    (let* ((orig-sel (selection-tree))
           (orig-sel-str (object->string (tree->stree orig-sel))))
      ;; Copy repeat_copies times
      (let loop ((k 0))
        (when (< k {repeat_copies})
          (kbd-copy)
          (loop (+ k 1))))

      ;; Cancel selection and move cursor past target node
      (selection-cancel)
      (tree-go-to bt 12)

      ;; Read tree before paste
      (let* ((bt-before (buffer-tree))
             (before-str (object->string (tree->stree bt-before)))
             (cb-after-copy (clipboard-get "primary"))
             (cb-type (if (and (tree? cb-after-copy) (> (tree-arity cb-after-copy) 0))
                          (tree-label (tree-ref cb-after-copy 0))
                          'invalid)))

        ;; Paste
        (kbd-paste)

        ;; Read tree after paste
        (let* ((bt-after (buffer-tree))
               (after-str (object->string (tree->stree bt-after)))
               ;; Paste succeeded if tree changed and increased in size
               (pasted-ok? (and (not (equal? before-str after-str))
                                (> (string-length after-str) (string-length before-str)))))
          (record-result
            (list (cons "case" {case_id})
                  (cons "repeat" {repeat_copies})
                  (cons "success" pasted-ok?)
                  (cons "cb_type" (object->string cb-type))
                  (cons "orig_sel_preview" (substring orig-sel-str 0 (min 150 (string-length orig-sel-str))))
                  (cons "cb_preview" (substring (object->string (tree->stree cb-after-copy)) 0 (min 150 (string-length (object->string (tree->stree cb-after-copy)))))))))))))

(define (run-new-doc-step1 target-node)
  (let* ((bt (buffer-tree)))
    (tree-select bt target-node)
    (let* ((orig-sel (selection-tree))
           (orig-sel-str (object->string (tree->stree orig-sel))))
      ;; Copy repeat_copies times
      (let loop ((k 0))
        (when (< k {repeat_copies})
          (kbd-copy)
          (loop (+ k 1))))

      (let* ((cb-after-copy (clipboard-get "primary"))
             (cb-type (if (and (tree? cb-after-copy) (> (tree-arity cb-after-copy) 0))
                          (tree-label (tree-ref cb-after-copy 0))
                          'invalid)))
        ;; Create new draft document
        (new-document)
        ;; Schedule step 2 after new draft document tab is created and focused
        (exec-delayed-at
          (lambda ()
            (catch #t
              (lambda ()
                (let* ((new-buf (current-buffer))
                       (new-bt-before (buffer-tree))
                       (before-str (object->string (tree->stree new-bt-before))))
                  (kbd-paste)
                  (let* ((new-bt-after (buffer-tree))
                         (after-str (object->string (tree->stree new-bt-after)))
                         ;; Paste is successful if new document is no longer empty / changed
                         (pasted-ok? (and (not (equal? before-str after-str))
                                          (> (string-length after-str) (string-length before-str)))))
                    (record-result
                      (list (cons "case" {case_id})
                            (cons "repeat" {repeat_copies})
                            (cons "success" pasted-ok?)
                            (cons "cb_type" (object->string cb-type))
                            (cons "new_buf" (object->string new-buf))
                            (cons "orig_sel_preview" (substring orig-sel-str 0 (min 150 (string-length orig-sel-str))))
                            (cons "before_preview" (substring before-str 0 (min 100 (string-length before-str))))
                            (cons "after_preview" (substring after-str 0 (min 150 (string-length after-str))))
                            (cons "cb_preview" (substring (object->string (tree->stree cb-after-copy)) 0 (min 150 (string-length (object->string (tree->stree cb-after-copy)))))))))))
              (lambda (k . a)
                (record-result (list (cons "case" {case_id}) (cons "error" (list k a)))))))
          (+ (texmacs-time) 1200))))))

(tm-define (run-clipboard-case)
  (exec-delayed-at
    (lambda ()
      (catch #t
        (lambda ()
          (cond
            ;; Case 1: Same doc pure text (node 10)
            ((= {case_id} 1) (run-same-doc 10))
            ;; Case 2: Same doc structure (section node 19)
            ((= {case_id} 2) (run-same-doc 19))
            ;; Case 3: Same doc mixed (with node 17)
            ((= {case_id} 3) (run-same-doc 17))
            ;; Case 4: Pure text -> new draft doc
            ((= {case_id} 4) (run-new-doc-step1 10))
            ;; Case 5: Structure -> new draft doc
            ((= {case_id} 5) (run-new-doc-step1 19))
            ;; Case 6: Mixed -> new draft doc
            ((= {case_id} 6) (run-new-doc-step1 17))
            (else (record-result (list (cons "error" "unknown_case"))))))
        (lambda (k . a)
          (record-result (list (cons "case" {case_id}) (cons "outer_error" (list k a)))))))
    (+ (texmacs-time) 1500)))
"""
    return scm


def run_single_test(repo_root, mogan_bin, source_doc, case_id, repeat_copies, timeout=25):
    """
    Executes a single test run in a fresh MoganSTEM instance.
    Controls variable by strictly terminating the process after the test.
    """
    kill_existing_mogan()

    result_file = os.path.join(repo_root, f"test_result_c{case_id}_r{repeat_copies}.txt")
    scm_file = os.path.join(repo_root, f"test_runner_c{case_id}_r{repeat_copies}.scm")

    if os.path.exists(result_file):
        try:
            os.remove(result_file)
        except:
            pass

    scm_content = generate_test_scm(case_id, repeat_copies, result_file)
    with open(scm_file, "w", encoding="utf-8") as f:
        f.write(scm_content)

    cmd = [mogan_bin, "-b", scm_file, "-x", "(run-clipboard-case)", source_doc]
    start_time = time.time()
    proc = subprocess.Popen(cmd)

    result_data = None
    try:
        proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        print(f"[TIMEOUT] Case {case_id} (copies={repeat_copies}) timed out after {timeout}s", flush=True)
        proc.kill()
    finally:
        kill_existing_mogan()

    duration = time.time() - start_time

    # Inspect OS clipboard safely
    cb_os = read_windows_clipboard_safe()

    # Parse result file
    if os.path.exists(result_file):
        try:
            with open(result_file, "r", encoding="utf-8") as f:
                content = f.read().strip()
                result_data = {"raw": content, "duration": round(duration, 2)}
        except Exception as e:
            result_data = {"error": str(e), "duration": round(duration, 2)}
        finally:
            try:
                os.remove(result_file)
            except:
                pass
    else:
        result_data = {"error": "no_result_file", "duration": round(duration, 2)}

    result_data["os_clipboard"] = cb_os

    try:
        if os.path.exists(scm_file):
            os.remove(scm_file)
    except:
        pass

    return result_data


def check_success(raw_str):
    """Accurately check if Scheme association list contains ('success' . #t)."""
    return '("success" . #t)' in raw_str or '(("success" . #t)' in raw_str or '("success" #t)' in raw_str


def main():
    repo_root = find_repo_root()
    mogan_bin = find_mogan_binary(repo_root)
    source_doc = "D:/repositories/liii/arxiv/deepseek_v4.stem"
    if not os.path.exists(source_doc):
        print(f"Error: Source document not found at {source_doc}")
        sys.exit(1)

    print("=================================================================")
    print("Mogan STEM Clipboard Reliability Automated Test")
    print(f"Binary: {mogan_bin}")
    print(f"Source: {source_doc}")
    print("=================================================================\n")

    cases_desc = {
        1: "Same doc: copy pure text -> paste",
        2: "Same doc: copy structure -> paste",
        3: "Same doc: copy mixed -> paste",
        4: "Copy pure text -> new draft doc -> paste",
        5: "Copy structure -> new draft doc -> paste",
        6: "Copy mixed -> new draft doc -> paste",
    }

    all_results = []
    # Test each scenario with single copy (1) and repeated copy (3)
    # Perform multiple test runs, completely closing MoganSTEM between runs
    test_plan = [
        # (case_id, repeat_copies)
        (1, 1), (1, 3),
        (2, 1), (2, 3),
        (3, 1), (3, 3),
        (4, 1), (4, 3),
        (5, 1), (5, 3),
        (6, 1), (6, 3),
    ]

    for round_num in range(1, 3):
        print(f"\n=================== RUN ITERATION {round_num} ===================")
        for case_id, repeat_copies in test_plan:
            desc = cases_desc[case_id]
            print(f"[Testing Case {case_id}] {desc} (copy x{repeat_copies}) ... ", end="", flush=True)
            res = run_single_test(repo_root, mogan_bin, source_doc, case_id, repeat_copies)
            raw = res.get("raw", "")
            success = check_success(raw)

            if success:
                print(f"PASS ({res['duration']}s)")
            else:
                print(f"FAIL ({res['duration']}s)")
                print(f"   -> Result output: {raw}")
                print(f"   -> OS Clipboard : {res.get('os_clipboard')}")

            all_results.append({
                "round": round_num,
                "case_id": case_id,
                "description": desc,
                "repeat_copies": repeat_copies,
                "success": success,
                "duration": res["duration"],
                "raw": raw,
                "os_clipboard": res.get("os_clipboard"),
            })
            time.sleep(1.0)

    print("\n=================================================================")
    print("FINAL SUMMARY REPORT:")
    print("=================================================================")
    passed = [r for r in all_results if r["success"]]
    failed = [r for r in all_results if not r["success"]]
    print(f"Total Runs : {len(all_results)}")
    print(f"Passed     : {len(passed)}")
    print(f"Failed     : {len(failed)}")

    if failed:
        print("\nFailed Cases Details:")
        for f in failed:
            print(f" - Iteration {f['round']}, Case {f['case_id']} ({f['description']}), copy x{f['repeat_copies']}:")
            print(f"   Details: {f['raw']}")
    print("=================================================================")


if __name__ == "__main__":
    main()
