;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : list-test.scm
;; DESCRIPTION : 纯逻辑单元测试：kernel 列表工具函数（cDr 等）。
;;               无 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r list-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/kernel/library/list.scm")

;; cDr：移除列表最后一个元素。

(define (test-cdr-basic)
  (check (cDr '(1 2 3)) => '(1 2))
  (check (cDr '(a b)) => '(a))
  (check (cDr '(a)) => '())
) ;define

;; 元素本身可以是任意类型（嵌套列表、字符串、布尔），cDr 只看顶层结构。

(define (test-cdr-mixed-elements)
  (check (cDr '((a b) (c d))) => '((a b)))
  (check (cDr '("x" 1 (a) #t)) => '("x" 1 (a)))
) ;define

;; 纯函数：不修改入参，且返回新列表（后续 set-car! 不影响原列表）。

(define (test-cdr-pure)
  (let* ((l (list 1 2 3)) (r (cDr l)))
    (check l => '(1 2 3))
    (set-car! r 99)
    (check l => '(1 2 3))
  ) ;let*
) ;define

;; 空列表无最后一个元素可移除，抛 out-of-range（list-drop-right 语义）。

(define (test-cdr-empty-errors)
  (check-catch 'out-of-range (cDr '()))
) ;define

;; cDDr/cDDDr：移除列表末尾 2/3 个元素。

(define (test-cddr-basic)
  (check (cDDr '(1 2 3 4)) => '(1 2))
  (check (cDDr '(1 2)) => '())
  (check (cDDDr '(1 2 3 4)) => '(1))
  (check (cDDDr '(1 2 3)) => '())
) ;define

;; 纯函数：返回新列表，set-car! 结果不影响原列表。

(define (test-cddr-pure)
  (let* ((l (list 1 2 3 4)) (r (cDDr l)))
    (set-car! r 99)
    (check l => '(1 2 3 4))
  ) ;let*
) ;define

;; 长度不足 k 时抛 out-of-range。

(define (test-cddr-errors)
  (check-catch 'out-of-range (cDDr '(1)))
  (check-catch 'out-of-range (cDDDr '(1 2)))
) ;define

;; cADr：取倒数第二个元素。

(define (test-cadr-star-basic)
  (check (cADr '(1 2 3)) => 2)
  (check (cADr '(1 2)) => 1)
  (check (cADr '("a" "b" "c")) => "b")
) ;define

;; 列表不足两个元素时抛 out-of-range。

(define (test-cadr-star-errors)
  (check-catch 'out-of-range (cADr '(1)))
  (check-catch 'out-of-range (cADr '()))
) ;define

;; list-head：取列表前 k 个元素（g_take 的 C 实现）。

(define (test-list-head-basic)
  (check (list-head '(1 2 3) 2) => '(1 2))
  (check (list-head '(1 2 3) 3) => '(1 2 3))
  (check (list-head '(1 2 3) 0) => '())
  (check (list-head '((a b) (c d) e) 2) => '((a b) (c d)))
) ;define

;; 纯函数：返回新列表，set-car! 结果不影响原列表。

(define (test-list-head-pure)
  (let* ((l (list 1 2 3)) (r (list-head l 2)))
    (set-car! r 99)
    (check l => '(1 2 3))
  ) ;let*
) ;define

;; 边界：k 为负、非整数、超出长度、对非列表取非零个，均抛 wrong-type-arg。

(define (test-list-head-errors)
  (check-catch 'wrong-type-arg (list-head '(1 2 3) -1))
  (check-catch 'wrong-type-arg (list-head '(1 2 3) 4))
  (check-catch 'wrong-type-arg (list-head '(1 2 3) 1.5))
  (check-catch 'wrong-type-arg (list-head 'a 1))
) ;define

;; list-drop-right：移除列表末尾 k 个元素（g_drop_right 的 C 实现）。

(define (test-list-drop-right-basic)
  (check (list-drop-right '(1 2 3 4) 1) => '(1 2 3))
  (check (list-drop-right '(1 2 3 4) 2) => '(1 2))
  (check (list-drop-right '(1 2) 2) => '())
  (check (list-drop-right '(1 2 3) 0) => '(1 2 3))
  (check (list-drop-right '() 0) => '())
  (check (list-drop-right '((a b) (c d) e) 1) => '((a b) (c d)))
) ;define

;; 纯函数：返回新列表（k=0 时也是拷贝），set-car! 结果不影响原列表。

(define (test-list-drop-right-pure)
  (let* ((l (list 1 2 3)) (r (list-drop-right l 1)))
    (set-car! r 99)
    (check l => '(1 2 3))
  ) ;let*
  (let* ((l (list 1 2 3)) (r (list-drop-right l 0)))
    (set-car! r 99)
    (check l => '(1 2 3))
  ) ;let*
) ;define

;; 边界：k 为负、超出长度抛 out-of-range；k 非整数、lst 非列表抛 wrong-type-arg。

(define (test-list-drop-right-errors)
  (check-catch 'out-of-range (list-drop-right '(1 2 3) -1))
  (check-catch 'out-of-range (list-drop-right '(1 2 3) 4))
  (check-catch 'out-of-range (list-drop-right '() 1))
  (check-catch 'wrong-type-arg (list-drop-right '(1 2 3) 1.5))
  (check-catch 'wrong-type-arg (list-drop-right 'a 1))
) ;define

(tm-define (regtest-list)
  (test-cdr-basic)
  (test-cdr-mixed-elements)
  (test-cdr-pure)
  (test-cdr-empty-errors)
  (test-cddr-basic)
  (test-cddr-pure)
  (test-cddr-errors)
  (test-cadr-star-basic)
  (test-cadr-star-errors)
  (test-list-head-basic)
  (test-list-head-pure)
  (test-list-head-errors)
  (test-list-drop-right-basic)
  (test-list-drop-right-pure)
  (test-list-drop-right-errors)
  (check-report)
) ;tm-define
