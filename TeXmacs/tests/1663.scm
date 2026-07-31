;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1663.scm
;; DESCRIPTION : 复现：少量上下标导致 script() 被调用上万次
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   新建文档并插入单行数学公式 a^2+b^2+c^3（仅 3 个上标），排版稳定后
;;   bench-print-all 观察 font_script_calculation 的 invocations。
;;   期望：调用次数与上标数量同量级（几十次）；历史上高达上万次。
;;
;;   用 exec-delayed-at 串异步链，不阻塞 Qt 事件循环；链尾自己 quit。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1663    # 真实 GUI，排版真正执行
;;   xmake r 1663                     # headless，仅冒烟进程不崩
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1663))

(define step-delay-ms 3000)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1663-step] ")
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

(tm-define (test_1663)
  (let ((steps (list (cons "new-document" (lambda () (new-document)))
                 (cons "insert math a^2+b^2+c^3"
                   (lambda ()
                     (insert '(math (concat "a" (rsup "2") "+b" (rsup "2")
                                            "+c" (rsup "3"))))
                   ) ;lambda
                 ) ;cons
                 (cons "idle (wait for typesetting)" (lambda () (noop)))
                 (cons "bench-print-all" (lambda () (bench-print-all)))
                 (cons "done; quitting" (lambda () (quit-TeXmacs)))
               ) ;list
        ) ;steps
       ) ;
    (display "[1663-step] starting delayed chain\n")
    (run-chain steps)
  ) ;let
) ;tm-define
