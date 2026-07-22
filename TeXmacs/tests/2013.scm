;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2013.scm
;; DESCRIPTION : Integration tests for telemetry path helpers (liii-path refactor)
;; COPYRIGHT   : (C) 2026  Yuki Lu
;;
;; 验证 telemetry 路径函数改用 (liii path) 后拼接结果的结构正确性。
;; 这些函数依赖 get-texmacs-home-path（运行时），但拼接出的相对结构
;; （/system/telemetry、/main、main-telemetry.json、.json.tmp）是确定性的。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT NO WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check)
  (only (liii path) path-exists? path-with-suffix path->string)
) ;import

(use-modules (plugin telemetry-utils))

(check-set-mode! 'report-failed)

(define (string-ends-with? s suffix)
  (let ((slen (string-length s)) (suf (string-length suffix)))
    (and (>= slen suf) (string=? (substring s (- slen suf) slen) suffix))
  ) ;let
) ;define

(define (test-telemetry-dir)
  (display "Testing telemetry-dir...\n")
  (let ((dir (telemetry-dir)))
    ;; 必须以 /system/telemetry 结尾
    (check (string-ends-with? dir "/system/telemetry") => #t)
    ;; 必须是绝对路径（以 home-path 开头）
    (check (string-ends-with? dir
             (string-append (telemetry-home-path) "/system/telemetry")
           ) ;string-ends-with?
      =>
      #t
    ) ;check
    ;; 目录必须真实存在（telemetry-dir 会 ensure-dir）
    (check (path-exists? dir) => #t)
  ) ;let
  (display "telemetry-dir tests passed!\n")
) ;define

(define (test-telemetry-main-dir)
  (display "Testing telemetry-main-dir...\n")
  (let ((dir (telemetry-main-dir)))
    (check (string-ends-with? dir "/system/telemetry/main") => #t)
    (check (path-exists? dir) => #t)
  ) ;let
  (display "telemetry-main-dir tests passed!\n")
) ;define

(define (test-telemetry-meta-path)
  (display "Testing telemetry-meta-path...\n")
  (let ((path (telemetry-meta-path)))
    ;; 文件名必须是 main-telemetry.json，位于 main 目录下
    (check (string-ends-with? path "/system/telemetry/main/main-telemetry.json")
      =>
      #t
    ) ;check
  ) ;let
  (display "telemetry-meta-path tests passed!\n")
) ;define

(define (test-telemetry-full-path)
  (display "Testing telemetry-full-path...\n")
  (let ((p (telemetry-full-path "detail-telemetry-20260625-1.jsonl")))
    ;; full-path = main-dir + "/" + filename
    (check (string-ends-with? p "/system/telemetry/main/detail-telemetry-20260625-1.jsonl")
      =>
      #t
    ) ;check
  ) ;let
  (display "telemetry-full-path tests passed!\n")
) ;define

(define (test-telemetry-tmp-path)
  (display "Testing telemetry write-meta tmp path derivation...\n")
  ;; 验证 .tmp 临时文件派生逻辑：
  ;; main-telemetry.json + ".json.tmp" suffix 替换 → main-telemetry.json.tmp
  ;; 这是 telemetry-write-meta 内部用的临时文件命名约定。
  (let* ((meta (telemetry-meta-path))
         (tmp (path->string (path-with-suffix meta ".json.tmp")))
        ) ;
    (check (string-ends-with? tmp "main-telemetry.json.tmp") => #t)
    ;; tmp 必须以 meta 为前缀（meta + ".tmp"）
    (check (string=? tmp (string-append meta ".tmp")) => #t)
  ) ;let*
  (display "telemetry tmp-path tests passed!\n")
) ;define

(tm-define (test_2013)
  (display "Running test_2013...\n")
  (test-telemetry-dir)
  (test-telemetry-main-dir)
  (test-telemetry-meta-path)
  (test-telemetry-full-path)
  (test-telemetry-tmp-path)
  (check-report)
) ;tm-define
