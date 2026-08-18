;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : menu-define-test.scm
;; DESCRIPTION : Contract tests for gui-make macro expansion
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 锁定 menu-define.scm 中 gui-make 的展开结果，作为后续优化的回归网。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel gui menu-define-test) (:use (kernel gui menu-define)))

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; gui-make：叶子与符号
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-gui-make-leaves)
  (check (gui-make "File") => "File")
  (check (gui-make '---) => '$---)
  (check (gui-make '/) => '$/)
  ;; === 展开为竖向 glue
  (check (gui-make '===) => '($glue #f #f 0 5))
  ;; // 展开为横向 glue
  (check (gui-make '//) => '($glue #f #f 5 0))
  ;; >> 为可伸展 glue
  (check (gui-make '>>) => '($glue #t #f 5 0))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; gui-make：简单条目
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-gui-make-items)
  (check (gui-make '(text "hello")) => '($menu-text "hello"))
  (check (gui-make '(icon "x")) => '($icon "x"))
  (check (gui-make '(glue #f #f 0 5)) => '($glue #f #f 0 5))
  (check (gui-make '(group "g")) => '($menu-group "g"))
  (check (gui-make '(toggle (noop) #t)) => '($toggle (noop) #t))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; gui-make：递归结构
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-gui-make-composite)
  (check (gui-make '(-> "File" (text "x"))) => '($-> "File" ($menu-text "x")))
  (check (gui-make '(hlist (text "a") /)) => '($hlist ($menu-text "a") $/))
  (check (gui-make '(vlist (text "a") (text "b")))
    =>
    '($vlist ($menu-text "a") ($menu-text "b"))
  ) ;check
  (check (gui-make '(when #t (text "a"))) => '($assuming #t ($menu-text "a")))
  (check (gui-make '(if #t (text "a"))) => '($delayed-when #t ($menu-text "a")))
  ;; 裸 ("label" cmd) 形式的条目
  (check (gui-make '("Open" (noop))) => '($> "Open" (noop)))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Regtest entry
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-menu-define)
  (test-gui-make-leaves)
  (test-gui-make-items)
  (test-gui-make-composite)
  (check-report)
) ;tm-define
