;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1311.scm
;; DESCRIPTION : 集成测试：通过 Go (转到) 菜单打开文档，验证新文档标签页为选中状态
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 timeout 15s xmake r 1311
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1311)
  (:use (kernel gui menu-widget)
    (texmacs menus file-menu)
    (texmacs menus tabpage-menu)
  ) ;:use
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

(define step-delay-ms 1000)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at
          (lambda ()
            (display* "[1311-step] " label "\n")
            (act)
            (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
          ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(define (check-current-document expected-url)
  (let ((cur-buf (current-buffer)) (cur-vw (current-view)))
    (check (url-tail cur-buf) => (url-tail expected-url))
    (check-true (nnull? cur-vw))
  ) ;let
) ;define

(tm-define (test_1311)
  (let* ((fixture-path (string-append (getenv "TEXMACS_PATH") "/tests/tmu/0991.tmu"))
         (fixture-url (string->url fixture-path))
         (pdf-fixture-path (string-append (getenv "TEXMACS_PATH") "/tests/PDF/38_2.pdf"))
         (startup-url (string->url "tmfs://startup-tab"))
         (steps
           (list
             ;; 1. 从启动页通过 go-menu-load-buffer 打开一个新文档
             (cons "step 1: open document via go-menu-load-buffer"
               (lambda () (go-menu-load-buffer fixture-path))
             ) ;cons
             ;; 2. 验证新文档加载，且当前活动 view 是该文档
             (cons "step 2: verify buffer and active view"
               (lambda () (check-current-document fixture-url))
             ) ;cons
             ;; 3. 切回启动页标签
             (cons "step 3: switch back to startup-tab"
               (lambda () (switch-to-buffer* startup-url))
             ) ;cons
             ;; 4. 再次通过 go-menu-load-buffer 切换到已打开的文档
             (cons "step 4: switch to existing document via go-menu-load-buffer"
               (lambda () (go-menu-load-buffer fixture-path))
             ) ;cons
             ;; 5. 验证成功切回该文档并保持选中
             (cons "step 5: verify switched back to document"
               (lambda () (check-current-document fixture-url))
             ) ;cons
             ;; 6. 通过 go-menu-load-buffer 打开 PDF 文档
             (cons "step 6: open pdf via go-menu-load-buffer"
               (lambda () (go-menu-load-buffer pdf-fixture-path))
             ) ;cons
             ;; 7. 验证 PDF 文档成功加载
             (cons "step 7: verify pdf buffer and active view"
               (lambda () (check-current-document (string->url pdf-fixture-path)))
             ) ;cons
             (cons "step 8: report and quit" (lambda () (check-report) (quit-TeXmacs)))
           ) ;list
         ) ;steps
        ) ;
    (display "[1311] starting test chain\n")
    (run-chain steps)
  ) ;let*
) ;tm-define
