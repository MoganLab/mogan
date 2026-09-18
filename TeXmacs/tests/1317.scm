;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1317.scm
;; DESCRIPTION : 集成测试：PDF 标签页加载、关闭及重新加载流程
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1317] 验证 PDF 标签页加载后正常进入 buffer 与 view 列表，关闭后 buffer 与
;;   view 被真正释放，重新加载同一路径的 PDF 时重新创建并激活 buffer 与 view。
;;   测试结束时关闭 PDF 标签页并关闭窗口退出。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1317      # 真实 GUI 模式运行断言链
;;   xmake r 1317                       # headless 冒烟验证
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define step-delay-ms 500)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) 100)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at
          (lambda ()
            (display "[1317-step] ")
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

(define (test-pdf-url)
  (url-append (get-texmacs-path) "tests/PDF/pdf_1_4_sample.pdf")
) ;define

(define (close-pdf-tab pdf-url)
  (let ((views (buffer->views pdf-url)))
    (when (nnull? views)
      (let* ((v (car views)) (win (or (view->window-of-tabpage v) (current-window))))
        (when win
          (safely-kill-tabpage-by-url win v pdf-url)
        ) ;when
      ) ;let*
    ) ;when
  ) ;let
) ;define

(tm-define (test_1317)
  (let ((pdf-url (test-pdf-url)))
    (define (check-pdf-unloaded)
      (check-true (not (buffer-exists? pdf-url)))
      (check (buffer->views pdf-url) => '())
    ) ;define
    (define (check-pdf-loaded)
      (check-true (buffer-exists? pdf-url))
      (check-true (nnull? (buffer->views pdf-url)))
    ) ;define
    (run-chain
      (list (cons "1. baseline: pdf buffer does not exist initially"
              (lambda () (check-pdf-unloaded))
            ) ;cons
        (cons "2. load-pdf-buffer opens PDF tab and creates buffer"
          (lambda () (load-pdf-buffer pdf-url))
        ) ;cons
        (cons "3. verify PDF buffer and view exist" (lambda () (check-pdf-loaded)))
        (cons "4. close PDF tab via close-pdf-tab" (lambda () (close-pdf-tab pdf-url)))
        (cons "5. verify PDF buffer and view are removed after tab close"
          (lambda () (check-pdf-unloaded))
        ) ;cons
        (cons "6. reopen the same PDF file via load-pdf-buffer"
          (lambda () (load-pdf-buffer pdf-url))
        ) ;cons
        (cons "7. verify PDF buffer and view are recreated again"
          (lambda () (check-pdf-loaded))
        ) ;cons
        (cons "8. cleanup: close PDF tab" (lambda () (close-pdf-tab pdf-url)))
        (cons "9. report and quit" (lambda () (check-report) (quit-TeXmacs)))
      ) ;list
    ) ;run-chain
  ) ;let
) ;tm-define
