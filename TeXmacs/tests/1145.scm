;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1145.scm
;; DESCRIPTION : GUI 复现：连续输入与进出数学模式时观察 mode 工具栏重建
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   配合 C++ 侧 [1145] 诊断日志（get_menu_widget 的 EQUAL/CACHE_HIT/MISS
;;   计数、replaceButtons 的 SAME/REPLACE 计数与耗时），在真实 GUI 里驱动：
;;     A. 纯文本连续输入 —— 展开结果应不变，期望全 EQUAL、零 REPLACE。
;;     B. 反复进出数学模式 —— 上下文切换，展开结果变化，观察每次切换是否
;;        全量重建所有按钮（REPLACE n=...）。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1145
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1145))

;; 步骤间隔：给 typeset + idle update_menus 足够时间（idle >= 1/60s 才触发）

(define step-delay-ms 1500)

(define (log-step label)
  (display "[1145-step] ")
  (display label)
  (newline)
) ;define

;; 每步间隔 step-delay-ms：lambda 返回剩余毫秒表示继续等待，返回 #t 表示完成。

(define (run-chain steps on-done)
  (if (null? steps)
    (on-done)
    (exec-delayed-pause (let ((start (texmacs-time)))
                          (lambda ()
                            (let ((left (- (+ start step-delay-ms) (texmacs-time))))
                              (if (> left 0)
                                left
                                (begin
                                  (log-step (caar steps))
                                  ((cdar steps))
                                  ;; 同步触发 C++ update_menus，不依赖 idle/焦点
                                  (update-menus)
                                  (run-chain (cdr steps) on-done)
                                  #t
                                ) ;begin
                              ) ;if
                            ) ;let
                          ) ;lambda
                        ) ;let
    ) ;exec-delayed-pause
  ) ;if
) ;define

(tm-define (test_1145)
  (new-document)
  (let* ((phase-a
           ;; A: 纯文本输入 5 步，每步插 6 个字符
           (let loop
             ((i 0) (acc '()))
             (if (>= i 5)
               (reverse acc)
               (loop (+ i 1)
                 (cons (cons (string-append "A: type text " (number->string i))
                         (lambda () (insert "abcdef"))
                       ) ;cons
                   acc
                 ) ;cons
               ) ;loop
             ) ;if
           ) ;let
         ) ;phase-a
         (phase-b
           ;; B: 5 轮 进出数学模式
           (let loop
             ((i 0) (acc '()))
             (if (>= i 5)
               acc
               (loop (+ i 1)
                 (append acc
                   (list (cons (string-append "B" (number->string i) ": insert math")
                           (lambda () (insert '(math "x")))
                         ) ;cons
                     (cons (string-append "B" (number->string i) ": enter math")
                       (lambda () (go-left))
                     ) ;cons
                     (cons (string-append "B" (number->string i) ": type in math")
                       (lambda () (insert "y"))
                     ) ;cons
                     (cons (string-append "B" (number->string i) ": exit math")
                       (lambda () (go-right))
                     ) ;cons
                   ) ;list
                 ) ;append
               ) ;loop
             ) ;if
           ) ;let
         ) ;phase-b
        ) ;
    (run-chain (append phase-a phase-b)
      (lambda () (display "[1145-step] done, quit") (newline) (quit-TeXmacs))
    ) ;run-chain
  ) ;let*
) ;tm-define
