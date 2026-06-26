;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2014.scm
;; DESCRIPTION : GUI 复现：切换 tab 时观察 menu 缓存是否命中
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   排查"切 tab 仍触发 SLOT_TAB_PAGES 整条重建"。配合 get_menu_widget 里
;;   which==4 的 [tabpage] menu cache HIT/MISS 日志，在真实 GUI 里切换两个
;;   文档，看每次切换是 HIT（不重建）还是 MISS（重建）。
;;
;;   夹具从 $TEXMACS_PATH/tests/tmu 复制到 /tmp，避免 save/编辑污染检入副本。
;;   带 stem-doc-id 以排除 auto-backup 标脏干扰（见 1106）。
;;
;;   用 exec-delayed-at 串异步链，不阻塞 Qt 事件循环；链尾自己 quit。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2014
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 2014))

(define (tmp-name name)
  (string->url (string-append "/tmp/" name))
) ;define

(define (refresh-fixture name)
  (let ((src (string->url (string-append "$TEXMACS_PATH/tests/tmu/" name))))
    (when (url-exists? src)
      (system-copy src (tmp-name name))
    ) ;when
  ) ;let
) ;define

(define step-delay-ms 4000)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2014-step] ")
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

(tm-define (test_2014)
  (refresh-fixture "2014_a.tmu")
  (refresh-fixture "2014_b.tmu")
  (let* ((path-a (tmp-name "2014_a.tmu")) (path-b (tmp-name "2014_b.tmu")))
    (let ((steps (list (cons "load a" (lambda () (load-buffer path-a)))
                   (cons "load b" (lambda () (load-buffer path-b)))
                 ) ;list
          ) ;steps
         ) ;
      ;; 在两个 tab 间来回切 5 轮。
      (let loop
        ((i 0) (acc steps))
        (if (>= i 5)
          (set! steps
            (append acc (list (cons "done; quitting" (lambda () (quit-TeXmacs)))))
          ) ;set!
          (loop (+ i 1)
            (append acc
              (list (cons (string-append "round " (number->string i) ": -> a")
                      (lambda () (switch-to-view-index 1))
                    ) ;cons
                (cons (string-append "round " (number->string i) ": -> b")
                  (lambda () (switch-to-view-index 2))
                ) ;cons
              ) ;list
            ) ;append
          ) ;loop
        ) ;if
      ) ;let
      (display "[2014-step] starting delayed chain\n")
      (run-chain steps)
    ) ;let
  ) ;let*
) ;tm-define
