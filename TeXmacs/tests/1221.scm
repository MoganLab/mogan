;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1221.scm
;; DESCRIPTION : GUI 集成测试：PDF 阅读位置记忆的 C++ 链路回归。
;;               覆盖两个曾导致「重新打开不跳页」的缺陷：
;;               1. loadFromFile 末尾 updatePageNavigation 发 pageChanged(1)
;;                  把已存页码覆盖成第 1 页（恢复页码须在加载前查询）；
;;               2. 程序退出时 ~qt_tm_widget_rep 不执行，退出前无保存时机
;;                  （改为 pageChanged 即存 preference）。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1221
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/texmacs/texmacs/tm-files.scm")

(define test-pdf
  (url->system "$TEXMACS_PATH/tests/PDF/quartus_manual_with_outline.pdf"))

(define other-buf
  (url->system "$TEXMACS_PATH/tests/64_1.tm"))

(define step-delay-ms 500)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1221-step] ")
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

(tm-define (test_1221)
  (run-chain (list
    ;; 预置阅读记录：第 5 页
    (cons "seed last page 5"
      (lambda ()
        (pdf-last-page-set test-pdf 5)
        (check (pdf-last-page-get test-pdf) => 5)))
    ;; 打开 PDF。注意：加载触发的 pageChanged(1) 与恢复跳页的回写
    ;; （singleShot(0)，滚动 valueChanged → pageChanged）是异步的，
    ;; 检查须等一拍再做
    (cons "open pdf"
      (lambda ()
        (load-buffer test-pdf)))
    ;; 恢复完成后记录须仍是 5（不得停留在加载时的第 1 页）
    (cons "restore keeps record"
      (lambda ()
        (check (pdf-last-page-get test-pdf) => 5)))
    ;; 切走再切回：记录仍不得被第 1 页覆盖
    (cons "switch away"
      (lambda ()
        (load-buffer other-buf)
        (check (pdf-last-page-get test-pdf) => 5)))
    (cons "switch back keeps record"
      (lambda ()
        (load-buffer test-pdf)
        (check (pdf-last-page-get test-pdf) => 5)))
    (cons "report + quit"
      (lambda ()
        (check-report)
        (quit-TeXmacs)))
  ))
) ;tm-define
