;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1139.scm
;; DESCRIPTION : Integration test for path-exists? crash fix
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; 验证编辑器 `path-exists?`（C++ 粘合函数 `tmg_path_existsP`）在收到
;; boolean、string、number 等非法参数时不会 abort，而是抛出可被捕获的
;; Scheme 类型错误。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (stem-path-exists?-raises-error? arg)
  "Call editor path-exists? with ARG and return #t if it raises an error\n   instead of returning normally or crashing."
  (catch #t (lambda () (stem-path-exists? arg) #f) (lambda args #t))
) ;define

(define (test-path-exists?-boolean)
  (display "Testing path-exists? with boolean arguments...\n")
  ;; 修复前：#t/#f 会进入 tmscm_is_path 的 tmscm_car，触发 s7 abort
  ;; 修复后：应抛出 Scheme 类型错误
  (check (stem-path-exists?-raises-error? #t) => #t)
  (check (stem-path-exists?-raises-error? #f) => #t)
  (display "boolean arguments tests passed!\n")
) ;define

(define (test-path-exists?-other-invalid)
  (display "Testing path-exists? with other invalid arguments...\n")
  (check (stem-path-exists?-raises-error? "foo") => #t)
  (check (stem-path-exists?-raises-error? 42) => #t)
  (check (stem-path-exists?-raises-error? 'sym) => #t)
  ;; 列表中只要有一个元素不是整数，就不是合法 path
  (check (stem-path-exists?-raises-error? '(1 #t)) => #t)
  (display "other invalid arguments tests passed!\n")
) ;define

(define (test-path-exists?-valid)
  (display "Testing path-exists? with valid arguments...\n")
  ;; 空路径 (list) 是合法的 path，不应报错；结果应为 boolean
  (check (boolean? (stem-path-exists? (list))) => #t)
  ;; 单元素整数路径也是合法 path
  (check (boolean? (stem-path-exists? '(0))) => #t)
  (display "valid arguments tests passed!\n")
) ;define

(tm-define (test_1139)
  (display "Running test_1139...\n")
  (test-path-exists?-boolean)
  (test-path-exists?-other-invalid)
  (test-path-exists?-valid)
  (check-report)
) ;tm-define
