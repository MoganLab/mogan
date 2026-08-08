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

;; exists?/forall?：谓词短路遍历（g_any/g_every 的 C 实现）。

(define (test-exists-basic)
  (check (exists? even? '(1 3 4 5)) => #t)
  (check (exists? even? '(1 3 5)) => #f)
  (check (exists? even? '()) => #f)
  (check (exists? (lambda (x) (and (> x 2) "hit")) '(1 2 3)) => #t)
) ;define

(define (test-exists-errors)
  (check-catch 'wrong-type-arg (exists? (lambda (x) #f) '(1 2 . 3)))
) ;define

(define (test-forall-basic)
  (check (forall? even? '(2 4 6)) => #t)
  (check (forall? even? '(2 3 6)) => #f)
  (check (forall? even? '()) => #t)
) ;define

;; list-find：返回首个满足谓词的元素（g_find 的 C 实现）。

(define (test-list-find-basic)
  (check (list-find '(3 1 4 1 5) even?) => 4)
  (check (list-find '(1 3 5) even?) => #f)
  (check (list-find '() even?) => #f)
) ;define

(define (test-list-find-errors)
  (check-catch 'wrong-type-arg (list-find '(1 2 . 3) (lambda (x) #f)))
) ;define

;; list-fold/list-fold-right：单列表路径走 g_fold/g_fold_right 的 C 实现。

(define (test-list-fold-basic)
  (check (list-fold + 0 '(1 2 3 4)) => 10)
  (check (list-fold cons '() '(1 2 3 4)) => '(4 3 2 1))
  (check (list-fold + 0 '()) => 0)
  (check (list-fold + 0 '(1 2 3) '(4 5 6)) => 21)
) ;define

(define (test-list-fold-errors)
  (check-catch 'wrong-type-arg (list-fold + 0 '(1 2 . 3)))
) ;define

(define (test-list-fold-right-basic)
  (check (list-fold-right + 0 '(1 2 3 4)) => 10)
  (check (list-fold-right cons '() '(1 2 3 4)) => '(1 2 3 4))
  (check (list-fold-right + 0 '()) => 0)
  (check (list-fold-right + 0 '(1 2 3) '(4 5 6)) => 21)
) ;define

(define (test-list-fold-right-errors)
  (check-catch 'wrong-type-arg (list-fold-right + 0 '(1 2 . 3)))
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
  (test-exists-basic)
  (test-exists-errors)
  (test-forall-basic)
  (test-list-find-basic)
  (test-list-find-errors)
  (test-list-fold-basic)
  (test-list-fold-errors)
  (test-list-fold-right-basic)
  (test-list-fold-right-errors)
  (check-report)
) ;tm-define
