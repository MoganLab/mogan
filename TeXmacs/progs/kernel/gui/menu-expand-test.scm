;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : menu-expand-test.scm
;; DESCRIPTION : Contract tests for pure menu expansion logic
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 锁定 menu-widget.scm / gui-markup.scm 中纯数据变换函数的当前行为，
;; 作为后续性能优化的回归网。只覆盖无 GUI 依赖的纯逻辑。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel gui menu-expand-test)
  (:use (kernel gui menu-widget) (kernel gui gui-markup))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; gui-normalize
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-gui-normalize)
  (check (gui-normalize '()) => '())
  (check (gui-normalize '(a b c)) => '(a b c))
  ;; (list ...) 形式的子表达式被拍平进结果
  (check (gui-normalize '((list 1 2) 3)) => '(1 2 3))
  (check (gui-normalize '((list (list 1) 2) (list 3))) => '(1 2 3))
  (check (gui-normalize '(a (list) b)) => '(a b))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; cache-menu?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-cache-menu?)
  (check (cache-menu? 'input) => #f)
  (check (cache-menu? 'foo) => #t)
  (check (cache-menu? "s") => #t)
  (check (cache-menu? '(a b)) => #t)
  ;; 树中任何位置出现 input 符号都不可缓存
  (check (cache-menu? '(a input)) => #f)
  (check (cache-menu? '(a (b (input)))) => #f)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; menu-expand
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-menu-expand-atoms)
  (check (menu-expand "str") => "str")
  (check (menu-expand '---) => '---)
  ;; 字符串打头的普通条目：展开后只保留标签（动作过程被剥离）
  (check (menu-expand (list "New" (lambda () 1))) => '("New"))
) ;define

(define (test-menu-expand-layout)
  (check (menu-expand '(horizontal (glue #t #f 5 0) ---))
    =>
    '(horizontal (glue #t #f 5 0) ---)
  ) ;check
  (check (menu-expand '(vertical (text "a") (text "b")))
    =>
    '(vertical (text "a") (text "b"))
  ) ;check
) ;define

(define (test-menu-expand-if)
  ;; 条件为真：展开为条目列表；为假：空列表
  (check (menu-expand (list 'if (lambda () #t) '(text "a"))) => '((text "a")))
  (check (menu-expand (list 'if (lambda () #f) '(text "a"))) => '())
) ;define

(define (test-menu-expand-when)
  (check (menu-expand (list 'when (lambda () #t) '(text "a")))
    =>
    '(when #t (text "a"))
  ) ;check
  ;; 条件为假时条目中的过程仍被源码化（保证可缓存的纯数据结构）
  (check (menu-expand (list 'when (lambda () #f) (list 'text "a" (lambda () 1))))
    =>
    '(when #f (text "a" (lambda () 1)))
  ) ;check
) ;define

(define (test-menu-expand-for)
  ;; (for gen-func vals-promise)：gen-func 返回条目列表，对 vals 逐个展开
  (check (menu-expand (list 'for (lambda (x) (list (list 'text x))) (lambda () '("a"
                                                                                 "b")))
         ) ;menu-expand
    =>
    '((text "a") (text "b"))
  ) ;check
) ;define

(define (test-menu-expand-mini)
  (check (menu-expand (list 'mini (lambda () #t) '(text "a")))
    =>
    '(mini #t (text "a"))
  ) ;check
) ;define

(define (test-menu-expand-procedures)
  ;; replace-procedures 的间接契约：结构中的过程被源码化
  (check (menu-expand (list 'text "a" (lambda () 1)))
    =>
    '(text "a" (lambda () 1))
  ) ;check
  ;; must-eval-list 成员（toggle）：on-thunk 被求值
  (check (menu-expand (list 'toggle (lambda (a) a) (lambda () #t)))
    =>
    '(toggle (lambda (a) a) #t)
  ) ;check
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Regtest entry
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-menu-expand)
  (test-gui-normalize)
  (test-cache-menu?)
  (test-menu-expand-atoms)
  (test-menu-expand-layout)
  (test-menu-expand-if)
  (test-menu-expand-when)
  (test-menu-expand-for)
  (test-menu-expand-mini)
  (test-menu-expand-procedures)
  (check-report)
) ;tm-define
