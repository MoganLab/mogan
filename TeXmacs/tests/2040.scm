;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2040.scm
;; DESCRIPTION : Test live-switch preferences (无需重启的即时生效)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; 验证两个原本「需重启」的首选项改为即时生效：
;; 1. language：set-language-and-notify → notify-language → set-output-language，
;;    get-output-language 实时跟随（无需重启）。
;; 2. magic-paste-shortcut：notify 重绑 std v/V，kbd-get-rev 反查绑定互换。
;;    用 kbd-get-rev 直接查反查表（kbd-find-rev-binding 包装对 std v/V 返回不稳定）。
;;
;; 运行：MOGAN_TEST_GUI=1 xmake r 2040
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic generic-kbd) (texmacs menus preferences-menu))
(import (liii check))

(tm-define (test_2040)
  ;; --- language 即时切换 ---
  ;; set-language-and-notify 走 set-preference，notify-language 实时 set-output-language。
  (let ((orig-lang (get-preference "language")))
    (set-language-and-notify "english")
    (check (get-output-language) => "english")
    (set-language-and-notify "chinese")
    (check (get-output-language) => "chinese")
    (set-preference "language" orig-lang)
  ) ;let

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
