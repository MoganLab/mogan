; ;; SPDX-License-Identifier: MIT
; ;;
; ;; Copyright (C) John Cowan (2015). All Rights Reserved.
; ;; 
; ;; Permission is hereby granted, free of charge, to any person
; ;; obtaining a copy of this software and associated documentation
; ;; files (the "Software"), to deal in the Software without
; ;; restriction, including without limitation the rights to use,
; ;; copy, modify, merge, publish, distribute, sublicense, and/or
; ;; sell copies of the Software, and to permit persons to whom the
; ;; Software is furnished to do so, subject to the following
; ;; conditions:
; ;; 
; ;; The above copyright notice and this permission notice shall be
; ;; included in all copies or substantial portions of the Software.
; ;; 
; ;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
; ;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
; ;; OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
; ;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
; ;; HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
; ;; WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; ;; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
; ;; OTHER DEALINGS IN THE SOFTWARE. 

; ;;; Main part of the SRFI 114 reference implementation

; ;; "There are two ways of constructing a software design: One way is to
; ;; make it so simple that there are obviously no deficiencies, and the
; ;; other way is to make it so complicated that there are no *obvious*
; ;; deficiencies." --Tony Hoare

(define-library (srfi srfi-128)
  (import (scheme base) (liii error))
  (export comparator? comparator-ordered? comparator-hashable? make-comparator make-pair-comparator
          make-list-comparator make-vector-comparator make-eq-comparator make-eqv-comparator
          make-equal-comparator boolean-hash char-hash char-ci-hash string-hash string-ci-hash
          symbol-hash number-hash make-default-comparator default-hash
          comparator-type-test-predicate comparator-equality-predicate comparator-ordering-predicate
          comparator-hash-function comparator-test-type comparator-check-type comparator-hash =? <?
          >? <=? >=?
  ) ;export
  (begin

    (define-record-type comparator
      (make-raw-comparator type-test equality ordering hash ordering? hash?) comparator?
      (type-test comparator-type-test-predicate) (equality comparator-equality-predicate)
      (ordering comparator-ordering-predicate) (hash comparator-hash-function)
      (ordering? comparator-ordered?) (hash? comparator-hashable?)
    ) ;define-record-type

    ; ; Invoke the test type
    (define (comparator-test-type comparator obj)
      ((comparator-type-test-predicate comparator) obj)
    ) ;define

    (define (comparator-check-type comparator obj)
      (if (comparator-test-type comparator obj)
          #t
          (type-error "comparator type check failed" comparator obj)
      ) ;if
    ) ;define

    (define (comparator-hash comparator obj)
      ((comparator-hash-function comparator) obj)
    ) ;define

    (define (binary=? comparator a b)
      ((comparator-equality-predicate comparator) a b)
    ) ;define

    (define (binary<? comparator a b)
      ((comparator-ordering-predicate comparator) a b)
    ) ;define

    (define (binary>? comparator a b)
      (binary<? comparator b a)
    ) ;define

    (define (binary<=? comparator a b)
      (not (binary>? comparator a b))
    ) ;define

    (define (binary>=? comparator a b)
      (not (binary<? comparator a b))
    ) ;define

    (define (%salt%)
      16064047
    ) ;define

    (define (hash-bound)
      33554432
    ) ;define

    (define (make-hasher)
      (let ((result (%salt%)))
        (case-lambda (() result)
                     ((n) (set! result (+ (modulo (* result 33) (hash-bound)) n)) result)
        ) ;case-lambda
      ) ;let
    ) ;define

    (define (make-comparator type-test equality ordering hash)
      (make-raw-comparator (if (eq? type-test #t)
            (lambda (x)
              #t
            ) ;lambda
            type-test)
        (if (eq? equality #t)
            (lambda (x y)
              (eqv? (ordering x y) 0)
            ) ;lambda
            equality
        ) ;if
        (if ordering
            ordering
            (lambda (x y)
              (error "ordering not supported")
            ) ;lambda
        ) ;if
        (if hash
            hash
            (lambda (x y)
              (error "hashing not supported"))) (if ordering #t #f)
        (if hash #t #f)
      ) ;make-raw-comparator
    ) ;define

    (define (make-eq-comparator)
      (make-comparator #t eq? #f default-hash)
    ) ;define

    (define (make-eqv-comparator)
      (make-comparator #t eqv? #f default-hash)
    ) ;define

    (define (make-equal-comparator)
      (make-comparator #t equal? #f default-hash)
    ) ;define

    (define (make-pair-type-test car-comparator cdr-comparator)
      (lambda (obj)
        (and (pair? obj)
             (comparator-test-type car-comparator (car obj))
             (comparator-test-type cdr-comparator (cdr obj))
        ) ;and
      ) ;lambda
    ) ;define

    (define (make-pair=? car-comparator cdr-comparator)
      (lambda (a b)
        (and ((comparator-equality-predicate car-comparator) (car a) (car b))
             ((comparator-equality-predicate cdr-comparator) (cdr a) (cdr b))
        ) ;and
      ) ;lambda
    ) ;define

    (define (make-pair<? car-comparator cdr-comparator)
      (lambda (a b)
        (if (=? car-comparator (car a) (car b))
            (<? cdr-comparator (cdr a) (cdr b))
            (<? car-comparator (car a) (car b))
        ) ;if
      ) ;lambda
    ) ;define

    (define (make-pair-hash car-comparator cdr-comparator)
      (lambda (obj)
        (let ((acc (make-hasher)))
          (acc (comparator-hash car-comparator (car obj)))
          (acc (comparator-hash cdr-comparator (cdr obj)))
          (acc)
        ) ;let
      ) ;lambda
    ) ;define

    (define (make-pair-comparator car-comparator cdr-comparator)
      (make-comparator (make-pair-type-test car-comparator cdr-comparator)
                       (make-pair=? car-comparator cdr-comparator)
                       (make-pair<? car-comparator cdr-comparator)
                       (make-pair-hash car-comparator cdr-comparator)
      ) ;make-comparator
    ) ;define

    ; ; Cheap test for listness
    (define (norp? obj)
      (or (null? obj) (pair? obj))
    ) ;define

    (define (make-list-comparator element-comparator type-test empty? head tail)
      (make-comparator (make-list-type-test element-comparator type-test empty? head tail)
                       (make-list=? element-comparator type-test empty? head tail)
                       (make-list<? element-comparator type-test empty? head tail)
                       (make-list-hash element-comparator type-test empty? head tail)
      ) ;make-comparator
    ) ;define

    (define (make-list-type-test element-comparator type-test empty? head tail)
      (lambda (obj)
        (and (type-test obj)
             (let ((elem-type-test (comparator-type-test-predicate element-comparator)))
               (let loop ((obj obj))
                 (cond ((empty? obj) #t)
                       ((not (elem-type-test (head obj))) #f)
                       (else (loop (tail obj)))
                 ) ;cond
               ) ;let
             ) ;let
        ) ;and
      ) ;lambda
    ) ;define

    (define (make-list=? element-comparator type-test empty? head tail)
      (lambda (a b)
        (let ((elem=? (comparator-equality-predicate element-comparator)))
          (let loop ((a a)
                     (b b))
            (cond ((and (empty? a) (empty? b) #t))
                  ((empty? a) #f)
                  ((empty? b) #f)
                  ((elem=? (head a) (head b)) (loop (tail a) (tail b)))
                  (else #f)
            ) ;cond
          ) ;let
        ) ;let
      ) ;lambda
    ) ;define

    (define (make-list<? element-comparator type-test empty? head tail)
      (lambda (a b)
        (let ((elem=? (comparator-equality-predicate element-comparator))
              (elem<? (comparator-ordering-predicate element-comparator)))
          (let loop ((a a)
                     (b b))
            (cond ((and (empty? a) (empty? b) #f))
                  ((empty? a) #t)
                  ((empty? b) #f)
                  ((elem=? (head a) (head b)) (loop (tail a) (tail b)))
                  ((elem<? (head a) (head b)) #t)
                  (else #f)
            ) ;cond
          ) ;let
        ) ;let
      ) ;lambda
    ) ;define

    (define (make-list-hash element-comparator type-test empty? head tail)
      (lambda (obj)
        (let ((elem-hash (comparator-hash-function element-comparator))
              (acc (make-hasher)))
          (let loop ((obj obj))
            (cond ((empty? obj) (acc))
                  (else
                   (acc (elem-hash (head obj)))
                   (loop (tail obj))
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;lambda
    ) ;define

    (define (make-vector-comparator element-comparator type-test length ref)
      (make-comparator (make-vector-type-test element-comparator type-test length ref)
                       (make-vector=? element-comparator type-test length ref)
                       (make-vector<? element-comparator type-test length ref)
                       (make-vector-hash element-comparator type-test length ref)
      ) ;make-comparator
    ) ;define

    (define (make-vector-type-test element-comparator type-test length ref)
      (lambda (obj)
        (and (type-test obj)
             (let ((elem-type-test (comparator-type-test-predicate element-comparator))
                   (len (length obj)))
               (let loop ((n 0))
                 (cond ((= n len) #t)
                       ((not (elem-type-test (ref obj n))) #f)
                       (else (loop (+ n 1)))
                 ) ;cond
               ) ;let
             ) ;let
        ) ;and
      ) ;lambda
    ) ;define

    (define (make-vector=? element-comparator type-test length ref)
      (lambda (a b)
        (and (= (length a) (length b))
             (let ((elem=? (comparator-equality-predicate element-comparator))
                   (len (length b)))
               (let loop ((n 0))
                 (cond ((= n len) #t)
                       ((elem=? (ref a n) (ref b n)) (loop (+ n 1)))
                       (else #f)
                 ) ;cond
               ) ;let
             ) ;let
        ) ;and
      ) ;lambda
    ) ;define

    (define (make-vector<? element-comparator type-test length ref)
      (lambda (a b)
        (cond ((< (length a) (length b)) #t)
              ((> (length a) (length b)) #f)
              (else
               (let ((elem=? (comparator-equality-predicate element-comparator))
                     (elem<? (comparator-ordering-predicate element-comparator))
                     (len (length a)))
                 (let loop ((n 0))
                   (cond ((= n len) #f)
                         ((elem=? (ref a n) (ref b n)) (loop (+ n 1)))
                         ((elem<? (ref a n) (ref b n)) #t)
                         (else #f)
                   ) ;cond
                 ) ;let
               ) ;let
              ) ;else
        ) ;cond
      ) ;lambda
    ) ;define

    (define (make-vector-hash element-comparator type-test length ref)
      (lambda (obj)
        (let ((elem-hash (comparator-hash-function element-comparator))
              (acc (make-hasher))
              (len (length obj)))
          (let loop ((n 0))
            (cond ((= n len) (acc))
                  (else
                   (acc (elem-hash (ref obj n)))
                   (loop (+ n 1))
                  ) ;else
            ) ;cond
          ) ;let
        ) ;let
      ) ;lambda
    ) ;define

    (define (object-type obj)
      (cond ((null? obj) 0)
            ((pair? obj) 1)
            ((boolean? obj) 2)
            ((char? obj) 3)
            ((string? obj) 4)
            ((symbol? obj) 5)
            ((number? obj) 6)
            ((vector? obj) 7)
            ((bytevector? obj) 8)
            (else 65535)
      ) ;cond
    ) ;define

    (define (boolean<? a b)
      ; ; #f < #t but not otherwise
      (and (not a) b)
    ) ;define

    (define (complex<? a b)
      (if (= (real-part a) (real-part b))
          (< (imag-part a) (imag-part b))
          (< (real-part a) (real-part b))
      ) ;if
    ) ;define

    (define (symbol<? a b)
      (string<? (symbol->string a) (symbol->string b))
    ) ;define

    (define boolean-hash hash-code)
    (define char-hash hash-code)
    (define char-ci-hash hash-code)
    (define string-hash hash-code)
    (define string-ci-hash hash-code)
    (define symbol-hash hash-code)
    (define number-hash hash-code)
    (define default-hash hash-code)

    (define (dispatch-ordering type a b)
      (case type
        ((0) 0)
        ((1)
         ((make-pair<? (make-default-comparator) (make-default-comparator)) a b)
        ) ;
        ((2) (boolean<? a b))
        ((3) (char<? a b))
        ((4) (string<? a b))
        ((5) (symbol<? a b))
        ((6) (complex<? a b))
        ((7)
         ((make-vector<? (make-default-comparator) vector? vector-length vector-ref) a b)
        ) ;
        ((8)
         ((make-vector<? (make-comparator exact-integer? = < default-hash) bytevector? bytevector-length bytevector-u8-ref) a
           b
         ) ;
        ) ;
        (else (binary<? (registered-comparator type) a b))
      ) ;case
    ) ;define

    (define (default-ordering a b)
      (let ((a-type (object-type a))
            (b-type (object-type b)))
        (cond ((< a-type b-type) #t)
              ((> a-type b-type) #f)
              (else (dispatch-ordering a-type a b))
        ) ;cond
      ) ;let
    ) ;define

    (define (dispatch-equality type a b)
      (case type
        ((0) #t)
        ((1)
         ((make-pair=? (make-default-comparator) (make-default-comparator)) a b)
        ) ;
        ((2) (boolean=? a b))
        ((3) (char=? a b))
        ((4) (string=? a b))
        ((5) (symbol=? a b))
        ((6) (= a b))
        ((7)
         ((make-vector=? (make-default-comparator) vector? vector-length vector-ref) a b)
        ) ;
        ((8)
         ((make-vector=? (make-comparator exact-integer? = < default-hash) bytevector? bytevector-length bytevector-u8-ref) a
           b
         ) ;
        ) ;
        (else (binary=? (registered-comparator type) a b))
      ) ;case
    ) ;define

    (define (default-equality a b)
      (let ((a-type (object-type a))
            (b-type (object-type b)))
        (if (= a-type b-type)
            (dispatch-equality a-type a b)
            #f
        ) ;if
      ) ;let
    ) ;define

    (define (make-default-comparator)
      (make-comparator (lambda (obj) #t) default-equality default-ordering default-hash)
    ) ;define

    (define (=? comparator a b . objs)
      (let loop ((a a)
                 (b b)
                 (objs objs))
        (and (binary=? comparator a b)
             (if (null? objs)
                 #t
                 (loop b (car objs) (cdr objs))
             ) ;if
        ) ;and
      ) ;let
    ) ;define

    (define (<? comparator a b . objs)
      (let loop ((a a)
                 (b b)
                 (objs objs))
        (and (binary<? comparator a b)
             (if (null? objs)
                 #t
                 (loop b (car objs) (cdr objs))
             ) ;if
        ) ;and
      ) ;let
    ) ;define

    (define (>? comparator a b . objs)
      (let loop ((a a)
                 (b b)
                 (objs objs))
        (and (binary>? comparator a b)
             (if (null? objs)
                 #t
                 (loop b (car objs) (cdr objs))
             ) ;if
        ) ;and
      ) ;let
    ) ;define

    (define (<=? comparator a b . objs)
      (let loop ((a a)
                 (b b)
                 (objs objs))
        (and (binary<=? comparator a b)
             (if (null? objs)
                 #t
                 (loop b (car objs) (cdr objs))
             ) ;if
        ) ;and
      ) ;let
    ) ;define

    (define (>=? comparator a b . objs)
      (let loop ((a a)
                 (b b)
                 (objs objs))
        (and (binary>=? comparator a b)
             (if (null? objs)
                 #t
                 (loop b (car objs) (cdr objs))
             ) ;if
        ) ;and
      ) ;let
    ) ;define
  ) ;begin
) ;define-library

