;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1198.scm
;; DESCRIPTION : 回归测试：GUI 下导出 PDF 抛 "no window attached to view"
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   在真实 GUI 进程中复现 File -> Export -> Pdf 的完整链路
;;   （wrapped-print-to-file：复制当前 buffer 到临时 buffer、切过去排版打印、
;;   切回、关闭临时 buffer），断言 PDF 真正落盘。
;;   修复前：链路中某处取到未挂窗口的 current view，concrete_window () 断言
;;   "no window attached to view" 被抛出，导出中断、PDF 不生成。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1198      # 真实 GUI，跑断言链
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

;; wrapped-print-to-file 里调 screens-buffer?（定义在 dynamic/fold-edit.scm，
;; 生产环境经菜单惰性加载）；测试在启动早期就跑，需显式加载避免 unbound。
(load "./TeXmacs/progs/dynamic/fold-edit.scm")

(check-set-mode! 'report-failed)

(define in-path "/tmp/1198_in.tm")
(define out-path "/tmp/1198_out.pdf")

(define step-delay-ms 1000)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1198-step] ")
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

(tm-define (test_1198)
  (run-chain (list
    (cons "prepare doc" (lambda ()
      (string-save "<\\document>\n  Hello PDF export test.\n</document>\n" in-path)
      (load-buffer in-path)
    ))
    (cons "wrapped-print-to-file" (lambda ()
      (wrapped-print-to-file out-path)
    ))
    (cons "check pdf exists" (lambda ()
      (check (url-exists? (system->url out-path)) => #t)
    ))
    (cons "report + quit" (lambda ()
      (check-report)
      (quit-TeXmacs)
    ))
  ))
) ;tm-define
