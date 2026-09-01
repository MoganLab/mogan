
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0949.scm
;; DESCRIPTION : 回归测试：win/gnome/kde 外观下复制/剪切/粘贴菜单快捷键显示。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [0949] 修复 Windows（含 gnome/kde 外观）下「编辑」菜单把复制/剪切/粘贴
;;   显示成 Ctrl+Ins/Shift+Del/Shift+Ins 的问题。根因：init-windows/gnome/kde
;;   注册的传统备用键（C-insert/S-insert/S-delete，kde/gnome 另有 F16/F18/F20）
;;   晚于 generic-kbd 的 std 主键，而菜单快捷键反查 kbd-find-inv-binding 按
;;   后注册先出取键，备用键顶掉了 C-c/C-x/C-v 的显示。修复：三个 LAF 插件在
;;   备用键之后重绑 std 主键。本测试钉死（任一条回退都会红）：
;;     1. std 外观（windows/gnome/kde）下，反查 (kbd-copy)/(kbd-cut)/(kbd-paste)
;;        得 "C-c"/"C-x"/"C-v"（Qt 菜单据此显示 Ctrl+C/Ctrl+X/Ctrl+V）。
;;     2. 传统备用键仍有效：C-insert→kbd-copy、S-insert→kbd-paste、
;;        S-delete→kbd-cut（按键行为不回归）。
;;     3. 任意外观下，三个命令的反查结果都不得落回备用键（防其它外观回归）。
;;
;; USAGE
;;   xmake b stem && xmake install stem
;;   MOGAN_TEST_GUI=1 xmake r 0949
;;
;; 注意：断言在异步链里，必须 MOGAN_TEST_GUI=1 才执行——headless 模式（xmake r
;; 0949）启动即 (quit-TeXmacs)，异步链来不及调度，断言不跑（仅冒烟进程不崩）。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory of this file.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)

;; 异步步长：只查键盘表（无排版/动画等待），500ms 足够 Qt 事件循环调度。

(define step-delay-ms 500)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[0949-step] ")
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

;; std 外观 = 使用 Ctrl 系 std 主键的三种 look and feel（emacs 用 A-/M- 系、
;; macos 用 M- 系，均无备用键顶替问题，只跑第 3 组断言）。

(define (std-laf?)
  (or (like-windows?) (like-gnome?) (like-kde?))
) ;define

;; 传统备用键集合：历史上会顶掉 std 主键显示的键（含 kde/gnome 的 F16/F18/F20）。

(define legacy-alt-keys '("C-insert" "S-insert" "S-delete" "F16" "F18" "F20"))

(tm-define (test_0949)
  (run-chain (list
               ;; 先强制排空插件与键盘的惰性加载，断言面对确定性的最终表状态。
               (cons "force lazy plugins/keyboard, assert inv bindings + alt keys"
                 (lambda ()
                   (lazy-plugin-force)
                   (lazy-keyboard-force #t)
                   (kbd-flush-pending)
                   ;; 1 std 外观：反查必须得 std 主键（Qt 菜单显示 Ctrl+C/Ctrl+X/Ctrl+V）。
                   (when (std-laf?)
                     (check (kbd-find-inv-binding '(kbd-copy)) => "C-c")
                     (check (kbd-find-inv-binding '(kbd-cut)) => "C-x")
                     (check (kbd-find-inv-binding '(kbd-paste)) => "C-v")
                   ) ;when
                   ;; 2 传统备用键仍然有效（get-kbd-bindings 首项为 (条件 命令 帮助) 三元组）。
                   (when (std-laf?)
                     (check-true (equal? (cadr (car (get-kbd-bindings "C-insert"))) '(kbd-copy)))
                     (check-true (equal? (cadr (car (get-kbd-bindings "S-insert"))) '(kbd-paste)))
                     (check-true (equal? (cadr (car (get-kbd-bindings "S-delete"))) '(kbd-cut)))
                   ) ;when
                   ;; 3 任意外观：反查结果不得落回备用键。
                   (check-true (not (member (kbd-find-inv-binding '(kbd-copy)) legacy-alt-keys)))
                   (check-true (not (member (kbd-find-inv-binding '(kbd-cut)) legacy-alt-keys)))
                   (check-true (not (member (kbd-find-inv-binding '(kbd-paste)) legacy-alt-keys)))
                 ) ;lambda
               ) ;cons
               (cons "report + quit" (lambda () (check-report) (quit-TeXmacs)))
             ) ;list
  ) ;run-chain
) ;tm-define
