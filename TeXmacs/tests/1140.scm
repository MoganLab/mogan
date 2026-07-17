;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1140.scm
;; DESCRIPTION : Integration test for path-exists? wrong argument fix
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; 验证 `graphics-utils.scm` 中不会因为把 `#f` 传入 `path->tree` 而间接导致
;; 编辑器 `path-exists?` 收到 boolean 参数。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (raises-error? thunk)
  "Return #t if THUNK raises an error, #f otherwise."
  (catch #t (lambda () (thunk) #f) (lambda args #t))
) ;define

(define (test-tm-upwards-path-invalid)
  (display "Testing tm-upwards-path with invalid path...\n")
  ;; 修复前：#f 经 path->tree 传给 path-exists?，触发类型错误
  ;; 修复后：应直接返回 #f，不调用 path-exists?
  (check (raises-error? (lambda () (tm-upwards-path #f '(with) '()))) => #f)
  (check (tm-upwards-path #f '(with) '()) => #f)
  (display "tm-upwards-path invalid path tests passed!\n")
) ;define

(define (test-get-upwards-tree-property-invalid)
  (display "Testing get-upwards-tree-property with invalid path...\n")
  (check (raises-error? (lambda () (get-upwards-tree-property #f "x"))) => #f)
  (check (eq? (get-upwards-tree-property #f "x") nothing) => #t)
  (display "get-upwards-tree-property invalid path tests passed!\n")
) ;define

(define (test-graphics-eval-magnify-no-graphics)
  (display "Testing graphics-eval-magnify without graphics...\n")
  ;; headless 下没有 graphics 上下文，graphics-graphics-path 返回 #f
  (check (raises-error? (lambda () (graphics-eval-magnify))) => #f)
  (check (graphics-eval-magnify) => "default")
  (display "graphics-eval-magnify without graphics tests passed!\n")
) ;define

(tm-define (test_1140)
  (display "Running test_1140...\n")
  (test-tm-upwards-path-invalid)
  (test-get-upwards-tree-property-invalid)
  (test-graphics-eval-magnify-no-graphics)
  (check-report)
) ;tm-define
