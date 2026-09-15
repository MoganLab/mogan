;; SRFI-1 list-processing library                      -*- Scheme -*-
;; Reference implementation
;;
;; SPDX-License-Identifier: MIT
;;
;; Copyright (c) 1998, 1999 by Olin Shivers. You may do as you please with
;; this code as long as you do not remove this copyright notice or
;; hold me liable for its use. Please send bug reports to shivers@ai.mit.edu.
;; -Olin
;;
;; Copyright (c) 2024 The Goldfish Scheme Authors
;; Follow the same License as the original one

(define-library (srfi srfi-1)
  (import (liii error) (liii base))
  (export circular-list iota list-copy xcons cons*)
  (export circular-list? null-list? proper-list? dotted-list?)
  (export first second third fourth fifth sixth seventh eighth ninth tenth)
  (export take drop take-right drop-right fold fold-right split-at reduce
    reduce-right append-map filter partition remove find delete
    delete-duplicates zip count
  ) ;export
  (export assoc assq assv alist-cons take-while drop-while list-index any every
    last-pair last
  ) ;export
  (begin

    (define (xcons a b)
      (cons b a)
    ) ;define

    (define (cons* a . b)
      (if (null? b) a (cons a (apply cons* b)))
    ) ;define

    (define (proper-list? x)
      (let loop
        ((x x) (lag x))
        (if (pair? x)
          (let ((x (cdr x)))
            (if (pair? x)
              (let ((x (cdr x)) (lag (cdr lag)))
                (and (not (eq? x lag)) (loop x lag))
              ) ;let
              (null? x)
            ) ;if
          ) ;let
          (null? x)
        ) ;if
      ) ;let
    ) ;define

    (define (dotted-list? x)
      (let loop
        ((x x) (lag x))
        (if (pair? x)
          (let ((x (cdr x)))
            (if (pair? x)
              (let ((x (cdr x)) (lag (cdr lag)))
                (and (not (eq? x lag)) (loop x lag))
              ) ;let
              (not (null? x))
            ) ;if
          ) ;let
          (not (null? x))
        ) ;if
      ) ;let
    ) ;define

    (define (null-list? l)
      (cond ((pair? l) #f)
            ((null? l) #t)
            (else (type-error "null-list?: argument out of domain" l))
      ) ;cond
    ) ;define

    (define (%at-least-n-elements? x n)
      (let loop
        ((x x) (n n))
        (cond ((= n 0) #t)
              ((pair? x) (loop (cdr x) (- n 1)))
              (else #f)
        ) ;cond
      ) ;let
    ) ;define

    (define (first x)
      (if (pair? x) (car x) (type-error "first: argument must be a pair" x))
    ) ;define

    (define (second x)
      (if (%at-least-n-elements? x 2)
        (cadr x)
        (type-error "second: argument must have at least 2 elements" x)
      ) ;if
    ) ;define

    (define (third x)
      (if (%at-least-n-elements? x 3)
        (caddr x)
        (type-error "third: argument must have at least 3 elements" x)
      ) ;if
    ) ;define

    (define (fourth x)
      (if (%at-least-n-elements? x 4)
        (list-ref x 3)
        (type-error "fourth: argument must have at least 4 elements" x)
      ) ;if
    ) ;define

    (define (fifth x)
      (if (%at-least-n-elements? x 5)
        (list-ref x 4)
        (type-error "fifth: argument must have at least 5 elements" x)
      ) ;if
    ) ;define

    (define (sixth x)
      (if (%at-least-n-elements? x 6)
        (list-ref x 5)
        (type-error "sixth: argument must have at least 6 elements" x)
      ) ;if
    ) ;define

    (define (seventh x)
      (if (%at-least-n-elements? x 7)
        (list-ref x 6)
        (type-error "seventh: argument must have at least 7 elements" x)
      ) ;if
    ) ;define

    (define (eighth x)
      (if (%at-least-n-elements? x 8)
        (list-ref x 7)
        (type-error "eighth: argument must have at least 8 elements" x)
      ) ;if
    ) ;define

    (define (ninth x)
      (if (%at-least-n-elements? x 9)
        (list-ref x 8)
        (type-error "ninth: argument must have at least 9 elements" x)
      ) ;if
    ) ;define

    (define (tenth x)
      (if (%at-least-n-elements? x 10)
        (list-ref x 9)
        (type-error "tenth: argument must have at least 10 elements" x)
      ) ;if
    ) ;define

    (define take g_take)

    (define (drop lst k)
      (unless (or (pair? lst) (null? lst))
        (type-error "drop: first argument must be a pair or null" lst)
      ) ;unless
      (unless (integer? k)
        (type-error "drop: second argument must be an integer" k)
      ) ;unless
      (list-tail lst k)
    ) ;define

    (define take-right g_take_right)

    (define drop-right g_drop_right)

    (define (split-at lst i)
      (when (< i 0)
        (value-error "require a index greater than 0, but got ~A -- split-at" i)
      ) ;when
      (let ((result (cons #f '())))
        (do ((j i (- j 1)) (rest lst (cdr rest)) (node result (cdr node)))
          ((zero? j) (values (cdr result) rest))
          (when (not (pair? rest))
            (value-error "lst length cannot be greater than i, where lst is ~A, but i is ~A-- split-at"
              lst
              i
            ) ;value-error
          ) ;when
          (set-cdr! node (cons (car rest) '()))
        ) ;do
      ) ;let
    ) ;define

    (define (last-pair l)
      (unless (pair? l)
        (type-error "last-pair: argument must be a pair" l)
      ) ;unless
      (if (pair? (cdr l)) (last-pair (cdr l)) l)
    ) ;define

    (define (last l)
      (unless (pair? l)
        (type-error "last: argument must be a pair" l)
      ) ;unless
      (car (last-pair l))
    ) ;define

    (define count g_count)

    (define (zip . lists)
      (for-each (lambda (l) (unless (list? l) (type-error "zip: argument must be a list" l)))
        lists
      ) ;for-each
      (apply map list lists)
    ) ;define

    (define (fold f initial . lists)
      (unless (procedure? f)
        (type-error "fold: expected procedure, got ~S" f)
      ) ;unless
      (cond ((null? lists) initial)
            ((and (pair? lists) (null? (cdr lists)) (list? (car lists)))
             (g_fold f initial (car lists))
            ) ;
            (else
              (let loop
                ((acc initial) (lsts lists))
                (if (any null? lsts)
                  acc
                  (let* ((cars (map car lsts)) (cdrs (map cdr lsts)))
                    (loop (apply f (append cars (list acc))) cdrs)
                  ) ;let*
                ) ;if
              ) ;let
            ) ;else
      ) ;cond
    ) ;define

    (define (fold-right f initial . lists)
      (unless (procedure? f)
        (type-error "fold-right: expected procedure, got ~S" f)
      ) ;unless
      (cond ((null? lists) initial)
            ((and (pair? lists) (null? (cdr lists)) (list? (car lists)))
             (g_fold_right f initial (car lists))
            ) ;
            (else
              (let loop
                ((lsts lists))
                (if (any null? lsts)
                  initial
                  (let* ((cars (map car lsts)) (cdrs (map cdr lsts)))
                    (apply f (append cars (list (loop cdrs))))
                  ) ;let*
                ) ;if
              ) ;let
            ) ;else
      ) ;cond
    ) ;define

    (define (reduce f initial l)
      (unless (procedure? f)
        (type-error "reduce: first argument must be a procedure" f)
      ) ;unless
      (unless (or (pair? l) (null? l))
        (type-error "reduce: third argument must be a list" l)
      ) ;unless
      (if (null? l) initial (fold f (car l) (cdr l)))
    ) ;define

    (define (reduce-right f initial l)
      (unless (procedure? f)
        (type-error "reduce-right: first argument must be a procedure" f)
      ) ;unless
      (unless (or (pair? l) (null? l))
        (type-error "reduce-right: third argument must be a list" l)
      ) ;unless
      (if (null? l)
        initial
        (let recur
          ((head (car l)) (l (cdr l)))
          (if (pair? l) (f head (recur (car l) (cdr l))) head)
        ) ;let
      ) ;if
    ) ;define

    (define (append-map proc . lists)
      (unless (procedure? proc)
        (type-error "append-map: expected procedure, got ~S" proc)
      ) ;unless
      (for-each (lambda (lst)
                  (unless (list? lst)
                    (type-error "append-map: expected list, got ~S" lst)
                  ) ;unless
                ) ;lambda
        lists
      ) ;for-each
      (apply append (apply map proc lists))
    ) ;define

    (define filter g_filter)

    (define (partition pred l)
      (unless (procedure? pred)
        (type-error "partition: first argument must be a procedure" pred)
      ) ;unless
      (let loop
        ((lst l) (satisfies '()) (dissatisfies '()))
        (cond ((null? lst) (cons satisfies dissatisfies))
              ((not (pair? lst))
               (type-error "partition: second argument must be a proper list" l)
              ) ;
              ((pred (car lst)) (loop (cdr lst) (cons (car lst) satisfies) dissatisfies))
              (else (loop (cdr lst) satisfies (cons (car lst) dissatisfies)))
        ) ;cond
      ) ;let
    ) ;define

    (define (remove pred l)
      (unless (procedure? pred)
        (type-error "remove: first argument must be a procedure" pred)
      ) ;unless
      (filter (lambda (x) (not (pred x))) l)
    ) ;define

    (define find g_find)

    (define (take-while pred lst)
      (unless (procedure? pred)
        (type-error "take-while: first argument must be a procedure" pred)
      ) ;unless
      (if (null? lst)
        '()
        (if (pair? lst)
          (if (pred (car lst)) (cons (car lst) (take-while pred (cdr lst))) '())
          (type-error "take-while: second argument must be a list" lst)
        ) ;if
      ) ;if
    ) ;define

    (define (drop-while pred l)
      (unless (procedure? pred)
        (type-error "drop-while: first argument must be a procedure" pred)
      ) ;unless
      (if (null? l)
        '()
        (if (pair? l)
          (if (pred (car l)) (drop-while pred (cdr l)) l)
          (type-error "drop-while: second argument must be a list" l)
        ) ;if
      ) ;if
    ) ;define

    (define list-index g_list_index)

    (define any g_any)

    (define every g_every)

    (define (%extract-maybe-equal caller maybe-equal)
      (let ((my-equal (if (null-list? maybe-equal) equal? (car maybe-equal))))
        (if (procedure? my-equal)
          my-equal
          (type-error (string-append (symbol->string caller) ": comparator must be a procedure")
            my-equal
          ) ;type-error
        ) ;if
      ) ;let
    ) ;define

    (define (delete x l . maybe-equal)
      (let ((my-equal (%extract-maybe-equal 'delete maybe-equal)))
        (filter (lambda (y) (not (my-equal x y))) l)
      ) ;let
    ) ;define

    (define %hash-table-supported-eq-funcs
      (list eq? eqv? equal? equivalent? = string=? char=?)
    ) ;define

    (define (%can-use-hash-table? eq-func)
      (memq eq-func %hash-table-supported-eq-funcs)
    ) ;define

    (define (%delete-duplicates-hash lis eq-func)
      (let ((seen (s7-make-hash-table 8 eq-func)) (result '()))
        (for-each
          (lambda (x)
            (unless (hash-table-ref seen x)
              (s7-hash-table-set! seen x #t)
              (set! result (cons x result))
            ) ;unless
          ) ;lambda
          lis
        ) ;for-each
        (reverse result)
      ) ;let
    ) ;define

    (define (%delete-duplicates-scan lis my-equal)
      (let loop
        ((remaining lis) (seen '()) (result '()))
        (cond ((null? remaining) (reverse result))
              ((member (car remaining) seen my-equal) (loop (cdr remaining) seen result))
              (else (loop (cdr remaining) (cons (car remaining) seen) (cons (car remaining) result))
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    ;; right-duplicate deletion
    ;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;; delete-duplicates delete-duplicates!
    ;;
    ;; Hybrid strategy: Use hash table O(n) for supported functions and
    ;; optimized scan O(n²) for other functions
    ;; 
    ;; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    (define (delete-duplicates lis . maybe-equal)
      (unless (list? lis)
        (type-error "delete-duplicates: first argument must be a list" lis)
      ) ;unless
      (let ((my-equal (%extract-maybe-equal 'delete-duplicates maybe-equal)))
        (cond ((null? lis) lis)
              ((%can-use-hash-table? my-equal) (%delete-duplicates-hash lis my-equal))
              (else (%delete-duplicates-scan lis my-equal))
        ) ;cond
      ) ;let
    ) ;define

    (define (alist-cons key value alist)
      (cons (cons key value) alist)
    ) ;define

    (define (circular-list val1 . vals)
      (let ((ans (cons val1 vals)))
        (set-cdr! (last-pair ans) ans)
        ans
      ) ;let
    ) ;define

    (define (circular-list? x)
      (let loop
        ((x x) (lag x))
        (and (pair? x)
          (let ((x (cdr x)))
            (and (pair? x)
              (let ((x (cdr x)) (lag (cdr lag)))
                (or (eq? x lag) (loop x lag))
              ) ;let
            ) ;and
          ) ;let
        ) ;and
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
