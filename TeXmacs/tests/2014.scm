;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2014.scm
;; DESCRIPTION : GUI 复现：切换/新建/关闭/拖拽 tab 时观察 menu 缓存命中与重建
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   排查"切 tab / 增删 / 排序 tab 触发 SLOT_TAB_PAGES 整条重建"。配合
;;   get_menu_widget 里 which==4 的 [tabpage] menu cache HIT/MISS 日志与
;;   QTMTabPageContainer 的 [tabpage] rebuild/active 计数日志，在真实 GUI 里
;;   驱动四种动作，看每次是 HIT（不重建）还是 MISS（重建）。
;;
;;   覆盖动作：
;;     1. 切换 active（switch-to-view-index）—— 应只 active、零 rebuild。
;;     2. 点 + 新建（new-document）—— 增 tab，签名变 → 1 次 rebuild。
;;     3. 关闭一个 tab（safely-kill-tabpage）—— 减 tab，签名变 → 1 次 rebuild。
;;     4. 拖拽排序（move-buffer-to-index）—— 集合不变、仅顺序变，签名按
;;        tabpage-list 顺序拼接 → 顺序变也算签名变 → 1 次 rebuild（但指针复用）。
;;
;;   夹具从 $TEXMACS_PATH/tests/tmu 复制到 /tmp，避免 save/编辑污染检入副本。
;;   用 exec-delayed-at 串异步链，不阻塞 Qt 事件循环；链尾自己 quit。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2014
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 2014))

(import (only (srfi srfi-1) list-index))

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

;; 步骤间隔：给 Qt 事件循环 + typeset 足够时间，避免异步事件叠加。

(define step-delay-ms 4000)

;; 打印当前 tab 数与 buffer 列表，便于把 [tabpage] 日志和动作对齐。

(define (log-state label)
  (let ((n (length (tabpage-list #t))))
    (display "[2014-step] ")
    (display label)
    (display "  tabs=")
    (display n)
    (newline)
  ) ;let
) ;define

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。
;; 每个元素是 (label . action-thunk)。

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
                           (log-state (string-append "after " label))
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
    ;; 动作链：
    ;;  - 载入 a、b（建 tab）
    ;;  - 1. 来回切 5 轮（应零 rebuild，全部 menu cache HIT）
    ;;  - 2. 新建 1 个 tab（+，应 1 次 MISS + 1 次 rebuild，added 仅 +1）
    ;;  - 3. 关闭当前 tab（应 1 次 MISS + 1 次 rebuild，removed 仅 +1）
    ;;  - 4. 排序：把当前 buffer 移到首位（应 1 次 MISS + 1 次 rebuild，added/removed 均为 0）
    ;;  - 结束退出
    ;;
    ;; 关闭/排序的可预测性要点：
    ;;  - new-document 建的是脏 buffer，safely-kill-tabpage 会命中 buffer-modified?
    ;;    分支弹确认框、卡住异步链。故新建后立即 buffer-pretend-saved 去脏。
    ;;  - 排序用 move-buffer-to-index 移到 index 0（首位），顺序必变。
    (let ((steps (append (list (cons "load a" (lambda () (load-buffer path-a)))
                           (cons "load b" (lambda () (load-buffer path-b)))
                         ) ;list
                   ;; 1) 切换 active：5 轮来回切 view 1 / view 2
                   (let loop
                     ((i 0) (acc '()))
                     (if (>= i 5)
                       (reverse acc)
                       (loop (+ i 1)
                         (append (list (cons (string-append "round " (number->string i) ": -> a")
                                         (lambda () (switch-to-view-index 1))
                                       ) ;cons
                                   (cons (string-append "round " (number->string i) ": -> b")
                                     (lambda () (switch-to-view-index 2))
                                   ) ;cons
                                 ) ;list
                           acc
                         ) ;append
                       ) ;loop
                     ) ;if
                   ) ;let
                   ;; 2) 新建 tab（+）并去脏，便于后续安全关闭
                   (list (cons "new-document (+)" (lambda () (new-document))))
                   (list (cons "buffer-pretend-saved"
                           (lambda () (buffer-pretend-saved (current-buffer)))
                         ) ;cons
                   ) ;list
                   ;; 3) 关闭当前 tab（新建的那个，已去脏，不弹框）
                   (list (cons "safely-kill-tabpage" (lambda () (safely-kill-tabpage))))
                   ;; 4) 排序：当前 buffer 移到首位（index 0）
                   (list (cons "move-buffer-to-index (reorder to 0)"
                           (lambda ()
                             (let ((buf (current-buffer)))
                               (when buf
                                 (move-buffer-to-index buf 0)
                               ) ;when
                             ) ;let
                           ) ;lambda
                         ) ;cons
                   ) ;list
                   ;; 结束
                   (list (cons "done; quitting" (lambda () (quit-TeXmacs))))
                 ) ;append
          ) ;steps
         ) ;
      (display "[2014-step] starting delayed chain\n")
      (run-chain steps)
    ) ;let
  ) ;let*
) ;tm-define
