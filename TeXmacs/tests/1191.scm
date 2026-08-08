;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1191.scm
;; DESCRIPTION : GUI 集成测试：tab 栏与 tm-menu 解耦后行为不变
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1191] tab 栏不再经 tm-menu（menu-expand / menu_cache），改由 scheme
;;   直接用 widget 原语构建、C++ 经 tab_pages() 专用通道下发。本测试钉死
;;   解耦后的外部行为契约（任一条回退都会红）：
;;     1. (texmacs-tab-pages) 直接返回非空 widget（旧的是 tm-menu，只能被
;;        menu-expand 消费，不能直接当 widget 用）。
;;     2. (update-menus 'tab-pages) 走新通道重建不报错。
;;     3. 签名（tabpage-menu-signature）语义不变：切 tab 签名不变（Qt 端
;;        只做 active 高亮、不重建），增/删 tab 签名变。
;;     4. 关闭路径 safely-kill-tabpage-by-url（tab 关闭按钮的命令）可用。
;;
;;   注意动作与断言分步：new-document / kill 的 tab 增删是异步落地的，
;;   同步步里立即断言 tabpage-list 会拿到旧值，故每步只做一类事。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1191      # 真实 GUI，跑断言链
;;
;; 注意：断言在异步链里，必须 MOGAN_TEST_GUI=1 才执行——headless 模式
;; （xmake r 1191）启动即 (quit-TeXmacs)，异步链来不及调度，断言不跑
;; （仅冒烟进程不崩）。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
;; 重新 load 被测模块，保证本测试拿到的是工作区版本而非启动期缓存。
(load "./TeXmacs/progs/texmacs/menus/tabpage-menu.scm")

(check-set-mode! 'report-failed)

;; 步骤间隔：给 Qt 事件循环 + tab 异步增删 + 菜单重建足够时间。

(define step-delay-ms 4000)

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。
;; 每个元素是 (label . action-thunk)。

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1191-step] ")
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

;; 关指定 buffer 的 tab：走 tab 关闭按钮同款命令。buffer 已去脏，不弹确认框。

(define (find-view-of buf)
  (let loop ((l (tabpage-list #t)))
    (cond ((null? l) #f)
          ((== (view->buffer (car l)) buf) (car l))
          (else (loop (cdr l))))
  ) ;let
) ;define

(define (kill-tab-of buf)
  (let ((v (find-view-of buf)))
    (when v
      (safely-kill-tabpage-by-url (view->window-of-tabpage v) v buf)
    ) ;when
  ) ;let
) ;define

(tm-define (test_1191)
  (let ((n0 0) (s0 "") (s1 "") (s2 "") (buf1 #f) (buf2 #f))
    (run-chain (list
      (cons "baseline"
        (lambda ()
          (set! n0 (length (tabpage-list #t)))
          (set! s0 (tabpage-menu-signature))
          (check-true (>= n0 1))
        ) ;lambda
      ) ;cons
      (cons "texmacs-tab-pages builds a widget directly (no tm-menu)"
        (lambda ()
          ;; 旧实现是 tm-menu，直接调用返回的不是 widget；新实现必须给出
          ;; 非空 widget 对象（widget-hmenu 包一串 widget-tab-page）。
          (check-true (not (null? (texmacs-tab-pages))))
        ) ;lambda
      ) ;cons
      (cons "update-menus 'tab-pages rebuilds via new channel"
        (lambda () (update-menus 'tab-pages))
      ) ;cons
      (cons "new-document #1"
        (lambda () (new-document))
      ) ;cons
      (cons "check tab #1 + new-document #2"
        (lambda ()
          (set! buf1 (current-buffer))
          (buffer-pretend-saved buf1)
          (set! s1 (tabpage-menu-signature))
          (check (length (tabpage-list #t)) => (+ n0 1))
          (check-true (not (== s1 s0)))
          (new-document)
        ) ;lambda
      ) ;cons
      (cons "check tab #2"
        (lambda ()
          (set! buf2 (current-buffer))
          (buffer-pretend-saved buf2)
          (set! s2 (tabpage-menu-signature))
          (check (length (tabpage-list #t)) => (+ n0 2))
          (check-true (not (== s2 s1)))
        ) ;lambda
      ) ;cons
      (cons "switch to view 1"
        (lambda () (switch-to-view-index 1))
      ) ;cons
      (cons "signature unchanged by switch + switch to view 2"
        (lambda ()
          (check (tabpage-menu-signature) => s2)
          (switch-to-view-index 2)
        ) ;lambda
      ) ;cons
      (cons "signature still unchanged; rebuild works after switches"
        (lambda ()
          (check (tabpage-menu-signature) => s2)
          (update-menus 'tab-pages)
          (check-true (not (null? (texmacs-tab-pages))))
        ) ;lambda
      ) ;cons
      (cons "close tab of buf2 via close-button command"
        (lambda () (kill-tab-of buf2))
      ) ;cons
      (cons "check count -1 and signature changed; close tab of buf1"
        (lambda ()
          (check (length (tabpage-list #t)) => (+ n0 1))
          (check-true (not (== (tabpage-menu-signature) s2)))
          (kill-tab-of buf1)
        ) ;lambda
      ) ;cons
      (cons "check back to baseline count"
        (lambda ()
          (check (length (tabpage-list #t)) => n0)
        ) ;lambda
      ) ;cons
      (cons "report + quit"
        (lambda ()
          (check-report)
          (quit-TeXmacs)
        ) ;lambda
      ) ;cons
    )) ;run-chain
  ) ;let
) ;tm-define
