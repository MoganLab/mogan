;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1151.scm
;; DESCRIPTION : GUI 复现：启动后焦点工具栏（focus toolbar）未正确显示
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   启动默认落在启动页；经 new-document 打开真实文档后，观察焦点工具栏
;;   是否随 chrome 重建正确出现。配合 update_menus / SLOT_FOCUS_ICONS 的
;;   临时日志确认 menu_icons(2) 是否送达、按钮是否被替换。
;;
;;   用 exec-delayed-at 串异步链，不阻塞 Qt 事件循环；链尾自己 quit。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1151
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1151))

(define step-delay-ms 4000)

(define (log-state label)
  (display "[1151-step] ")
  (display label)
  (display "  buffer=")
  (display (current-buffer))
  (newline)
) ;define

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1151-step] ")
                           (display label)
                           (newline)
                           (act)
                           (log-state (string-append "after " label))
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_1151)
  (let ((steps (list (cons "new-document" (lambda () (new-document)))
                 (cons "insert text" (lambda () (insert "hello")))
                 (cons "idle (observe focus toolbar)" (lambda () (noop)))
                 (cons "done; quitting" (lambda () (quit-TeXmacs)))
               ) ;list
        ) ;steps
       ) ;
    (display "[1151-step] starting delayed chain\n")
    (run-chain steps)
  ) ;let
) ;tm-define
