;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : menu-entry-test.scm
;; DESCRIPTION : Tests for menu entry attribute derivation
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; menu-entry-attributes 从动作源码一次性导出菜单项的全部展示属性
;; （样式位 / 勾选标记 / 省略号标签 / 快捷键 / 气球帮助），
;; 替代原先 make-menu-entry-sub 中分散的 5+ 次 promise-source 调用。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel gui menu-entry-test)
  (:use (kernel gui menu-widget) (kernel gui gui-markup))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;; 带各种属性的测试命令（名字带独立前缀避免与真实命令冲突）

(tm-define (menu-entry-test-plain) (noop))

(tm-define (menu-entry-test-checked) (:check-mark "v" (lambda () #t)) (noop))

(tm-define (menu-entry-test-unchecked) (:check-mark "v" (lambda () #f)) (noop))

(tm-define (menu-entry-test-inapplicable) (:applicable #f) (noop))

(tm-define (menu-entry-test-interactive) (:interactive #t) (noop))

(tm-define (menu-entry-test-hint) "hint text")

(tm-define (menu-entry-test-ballooned) (:balloon menu-entry-test-hint) (noop))

(define (test-plain-entry)
  (check (menu-entry-attributes "Open" '(menu-entry-test-plain) 0 #f #f #f)
    =>
    '(0 "" "Open" "" #f)
  ) ;check
) ;define

(define (test-check-mark)
  (check (menu-entry-attributes "Opt" '(menu-entry-test-checked) 0 #f #f #f)
    =>
    '(0 "v" "Opt" "" #f)
  ) ;check
  (check (menu-entry-attributes "Opt" '(menu-entry-test-unchecked) 0 #f #f #f)
    =>
    '(0 "" "Opt" "" #f)
  ) ;check
  ;; 显式 check 覆盖（check 标签语法）：(check :label "v*" pred-thunk)
  (check (menu-entry-attributes "Opt"
           '(menu-entry-test-plain)
           0
           #f
           (list "v*" (lambda () #t))
           #f
         ) ;menu-entry-attributes
    =>
    '(0 "v*" "Opt" "" #f)
  ) ;check
) ;define

(define (test-inapplicable-greys)
  ;; 不适用条目叠加 widget-style-inert + widget-style-grey
  (check (menu-entry-attributes "Opt" '(menu-entry-test-inapplicable) 0 #f #f #f)
    =>
    (list (+ widget-style-inert widget-style-grey) "" "Opt" "" #f)
  ) ;check
  ;; 原有样式位保留
  (check (menu-entry-attributes "Opt" '(menu-entry-test-inapplicable) 1 #f #f #f)
    =>
    (list (+ 1 widget-style-inert widget-style-grey) "" "Opt" "" #f)
  ) ;check
) ;define

(define (test-interactive-adds-dots)
  (check (menu-entry-attributes "Open" '(menu-entry-test-interactive) 0 #f #f #f)
    =>
    '(0 "" "Open..." "" #f)
  ) ;check
) ;define

(define (test-balloon-help)
  (check (menu-entry-attributes "Opt" '(menu-entry-test-ballooned) 0 #f #f #f)
    =>
    '(0 "" "Opt" "" "hint text")
  ) ;check
  ;; bal? 为真（标签自带显式 balloon）时跳过气球帮助搜索
  (check (menu-entry-attributes "Opt" '(menu-entry-test-ballooned) 0 #f #f #t)
    =>
    '(0 "" "Opt" "" #f)
  ) ;check
) ;define

(define (test-make-menu-items-smoke)
  ;; 组合路径冒烟：真实条目经 make-menu-items 产出 widget 列表
  (check (pair? (make-menu-items (list "Open" (lambda () (noop))) 0 #f)) => #t)
) ;define

(tm-define (regtest-menu-entry)
  (test-plain-entry)
  (test-check-mark)
  (test-inapplicable-greys)
  (test-interactive-adds-dots)
  (test-balloon-help)
  (test-make-menu-items-smoke)
  (check-report)
) ;tm-define
