;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : smart-heading-test.scm
;; DESCRIPTION : 智能插入标题（smart-insert-heading）路径比较的纯逻辑测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/progs/text/text-edit.scm")

;; path-before? 是纯字典序，长度不等时前缀更小（path-less? 在此情形走
;; start/end 特例返回 #f，本测试钉死这个差异）

(define (test-path-before)
  (check (path-before? '(0) '(1)) => #t)
  (check (path-before? '(1) '(0)) => #f)
  (check (path-before? '(0) '(0)) => #f)
  ;; 前缀视为更小：标题块路径是块内光标路径的前缀，归入上方
  (check (path-before? '(0 1) '(0 1 0)) => #t)
  ;; 关键回归场景：section/section* 被 concat 包裹时，光标路径比标题路径深
  (check (path-before? '(0 1) '(0 2 1)) => #t)
  (check (path-before? '(0 2 1) '(0 1)) => #f)
) ;define

(define (test-nearest-above)
  (let ((hs '((0 0) (0 1))))
    ;; 光标在 section* 之后的正文里：最近上方是 (0 1)
    (check (nearest-above-path '(0 2 1) hs) => '(0 1))
    ;; 光标在两个标题之间：最近上方是 (0 0)
    (check (nearest-above-path '(0 1 0 0) '((0 0) (0 2))) => '(0 0))
    ;; 光标在标题块内部：标题自身算最近上方
    (check (nearest-above-path '(0 1 0 2) hs) => '(0 1))
    ;; 上方没有标题
    (check (nearest-above-path '(0 0 0) '((0 1))) => #f)
    (check (nearest-above-path '(0 0) '()) => #f)
  ) ;let
) ;define

(tm-define (regtest-smart-heading)
  (test-path-before)
  (test-nearest-above)
  (check-report)
) ;tm-define
