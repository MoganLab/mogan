;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6001.scm
;; DESCRIPTION : 集成测试：PDF 导出时使用通用等待弹窗（wait-dialog）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   测试 PDF 导出链路接入通用等待弹窗（wait-dialog）：
;;   验证 wrapped-print-to-file 能正常唤起等待弹窗、导出 PDF 文件，
;;   并在完成后正常关闭等待弹窗，PDF 成功生成在目标路径。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 6001                       # Headless 冒烟验证
;;   MOGAN_TEST_GUI=1 xmake r 6001      # GUI 真实断言
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/dynamic/fold-edit.scm")

(check-set-mode! 'report-failed)

(define in-path "/tmp/6001_in.tm")

(define out-path "/tmp/6001_out.pdf")

(define cancel-out-path "/tmp/6001_cancel_out.pdf")

(define step-delay-ms 1000)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at
          (lambda ()
            (display "[6001-step] ")
            (display label)
            (newline)
            (act)
            (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
          ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_6001)
  (run-chain
    (list
      (cons "prepare doc"
        (lambda ()
          (when (url-exists? (system->url out-path))
            (system-remove (system->url out-path))
          ) ;when
          (when (url-exists? (system->url cancel-out-path))
            (system-remove (system->url cancel-out-path))
          ) ;when
          (string-save "<\\document>\n  Test PDF export with modern wait dialog.\n</document>\n"
            in-path
          ) ;string-save
          (load-buffer in-path)
        ) ;lambda
      ) ;cons
      (cons "wrapped-print-to-file" (lambda () (wrapped-print-to-file out-path)))
      (cons "check pdf exists"
        (lambda () (check (url-exists? (system->url out-path)) => #t))
      ) ;cons
      (cons "test-cancel"
        (lambda () (wrapped-print-to-file cancel-out-path) (wait-dialog-cancelled))
      ) ;cons
      (cons "check cancelled pdf does not exist"
        (lambda () (check (url-exists? (system->url cancel-out-path)) => #f))
      ) ;cons
      (cons "report + quit" (lambda () (check-report) (quit-TeXmacs)))
    ) ;list
  ) ;run-chain
) ;tm-define
