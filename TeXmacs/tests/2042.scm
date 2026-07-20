;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2042.scm
;; DESCRIPTION : Test live magic-paste-shortcut switch（切换魔法粘贴快捷键即时生效）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; 验证 magic-paste-shortcut 改为即时生效：set-preference 触发
;; notify-magic-paste-shortcut 重绑 std v/V，kbd-magic-paste / kbd-paste 的
;; 反查绑定立即互换，无需重启。
;;
;; 用 kbd-get-rev 直接查反查表（kbd-find-rev-binding 包装对 std v/V 返回不稳定）。
;;
;; 运行：MOGAN_TEST_GUI=1 xmake r 2042
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic generic-kbd))
(import (liii check))

(tm-define (test_2042)
  ;; --- magic-paste-shortcut 即时重绑 ---
  (let ((orig (get-preference "magic-paste-shortcut")))
    (define (magic-rev)
      (kbd-get-rev "(kbd-magic-paste)")
    ) ;define
    (define (paste-rev)
      (kbd-get-rev "(kbd-paste)")
    ) ;define

    ;; 默认 ctrl+shift+v：magic-paste 与 paste 的反查绑定不同（分别在 std v / std V）
    (set-preference "magic-paste-shortcut" "ctrl+shift+v")
    (let ((m1 (magic-rev)) (p1 (paste-rev)))
      ;; 切到 ctrl+v 后两者应互换 → magic-rev 变成原 paste-rev，反之亦然
      (set-preference "magic-paste-shortcut" "ctrl+v")
      (let ((m2 (magic-rev)) (p2 (paste-rev)))
        (check (equal? m2 p1) => #t)
        (check (equal? p2 m1) => #t)
      ) ;let
    ) ;let

    (set-preference "magic-paste-shortcut" orig)
  ) ;let

  ;; GUI 模式不自动 quit，延迟一拍退出（让 check 输出 flush 后关窗）
  (exec-delayed-at (lambda () (quit-TeXmacs)) (+ (texmacs-time) 500))
) ;tm-define
