;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2041.scm
;; DESCRIPTION : Test live language switch（切换语言即时生效，无需重启）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; 验证 set-language-and-notify 改为即时生效：set-preference 触发 notify-language
;; 实时 set-output-language，get-output-language 立即跟随，无需重启。
;;
;; 运行：MOGAN_TEST_GUI=1 xmake r 2041
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (texmacs menus preferences-menu))
(import (liii check))

(tm-define (test_2041)
  ;; set-language-and-notify 走 set-preference，notify-language 实时 set-output-language。
  (let ((orig-lang (get-preference "language")))
    (set-language-and-notify "english")
    (check (get-output-language) => "english")
    (set-language-and-notify "chinese")
    (check (get-output-language) => "chinese")
    (set-preference "language" orig-lang)
  ) ;let

  ;; GUI 模式不自动 quit，延迟一拍退出（让 check 输出 flush 后关窗）
  (exec-delayed-at (lambda () (quit-TeXmacs)) (+ (texmacs-time) 500))
) ;tm-define
