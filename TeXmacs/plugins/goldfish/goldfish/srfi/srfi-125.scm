;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

(define-library (srfi srfi-125)
  (import (srfi srfi-1)
    (srfi srfi-128)
    (liii base)
    (liii error)
  ) ;import
  (export make-hash-table
    hash-table
    hash-table-unfold
    alist->hash-table
    hash-table?
    hash-table-contains?
    hash-table-empty?
    hash-table=?
    hash-table-mutable?
    hash-table-ref
    hash-table-ref/default
    hash-table-set!
    hash-table-delete!
    hash-table-intern!
    hash-table-update!
    hash-table-update!/default
    hash-table-pop!
    hash-table-clear!
    hash-table-size
    hash-table-keys
    hash-table-values
    hash-table-entries
    hash-table-find
    hash-table-count
    hash-table-fold
    hash-table-for-each
    hash-table-map->list
    hash-table->alist
    hash-table-copy
  ) ;export
  (begin

    (define (assert-hash-table-type ht f)
      (when (not (hash-table? ht))
        (error 'type-error
          f
          "this parameter must be typed as hash-table"
        ) ;error
      ) ;when
    ) ;define

    (define s7-hash-table-set!
      hash-table-set!
    ) ;define
    (define s7-make-hash-table
      make-hash-table
    ) ;define
    (define s7-hash-table-entries
      hash-table-entries
    ) ;define

    (define (make-hash-table . args)
      (cond ((null? args) (s7-make-hash-table))
            ((comparator? (car args))
             (let* ((equiv (comparator-equality-predicate (car args)
                           ) ;comparator-equality-predicate
                    ) ;equiv
                    (hash-func (comparator-hash-function (car args))
                    ) ;hash-func
                   ) ;
               (s7-make-hash-table 8
                 (cons equiv hash-func)
                 (cons #t #t)
               ) ;s7-make-hash-table
             ) ;let*
            ) ;
            (else (type-error "make-hash-table"))
      ) ;cond
    ) ;define

    (define alist->hash-table
      (typed-lambda ((lst list?))
        (when (odd? (length lst))
          (value-error "The length of lst must be even!"
          ) ;value-error
        ) ;when
        (let ((ht (make-hash-table)))
          (let loop
            ((rest lst))
            (if (null? rest)
              ht
              (begin
                (hash-table-set! ht
                  (car rest)
                  (cadr rest)
                ) ;hash-table-set!
                (loop (cddr rest))
              ) ;begin
            ) ;if
          ) ;let
        ) ;let
      ) ;typed-lambda
    ) ;define

    (define (hash-table-contains? ht key)
      (not (not (hash-table-ref ht key)))
    ) ;define

    (define (hash-table-empty? ht)
      (zero? (hash-table-size ht))
    ) ;define

    (define (hash-table=? ht1 ht2)
      (equal? ht1 ht2)
    ) ;define

    (define (hash-table-ref/default ht key default)
      (or (hash-table-ref ht key)
        (if (procedure? default)
          (default)
          default
        ) ;if
      ) ;or
    ) ;define

    (define (hash-table-set! ht . rest)
      (assert-hash-table-type ht
        hash-table-set!
      ) ;assert-hash-table-type
      (let ((len (length rest)))
        (when (or (odd? len) (zero? len))
          (error 'wrong-number-of-args
            len
            "but must be even and non-zero"
          ) ;error
        ) ;when
        (s7-hash-table-set! ht
          (car rest)
          (cadr rest)
        ) ;s7-hash-table-set!
        (when (> len 2)
          (apply hash-table-set!
            (cons ht (cddr rest))
          ) ;apply
        ) ;when
      ) ;let
    ) ;define

    (define (hash-table-delete! ht key . keys)
      (assert-hash-table-type ht
        hash-table-delete!
      ) ;assert-hash-table-type
      (let ((all-keys (cons key keys)))
        (length (filter (lambda (x)
                          (if (hash-table-contains? ht x)
                            (begin
                              (s7-hash-table-set! ht x #f)
                              #t
                            ) ;begin
                            #f
                          ) ;if
                        ) ;lambda
                  all-keys
                ) ;filter
        ) ;length
      ) ;let
    ) ;define

    (define (hash-table-update! ht key value)
      (hash-table-set! ht key value)
    ) ;define

    (define (hash-table-update!/default ht
              key
              updater
              default
            ) ;hash-table-update!/default
      (hash-table-set! ht
        key
        (updater (hash-table-ref/default ht key default)
        ) ;updater
      ) ;hash-table-set!
    ) ;define

    (define (hash-table-clear! ht)
      (for-each (lambda (key)
                  (hash-table-set! ht key #f)
                ) ;lambda
        (hash-table-keys ht)
      ) ;for-each
    ) ;define

    (define hash-table-size
      s7-hash-table-entries
    ) ;define

    (define (hash-table-keys ht)
      (map car ht)
    ) ;define

    (define (hash-table-values ht)
      (map cdr ht)
    ) ;define

    (define hash-table-entries
      (typed-lambda ((ht hash-table?))
        (let ((ks (hash-table-keys ht))
              (vs (hash-table-values ht))
             ) ;
          (values ks vs)
        ) ;let
      ) ;typed-lambda
    ) ;define

    (define (hash-table-find proc ht failure)
      (let ((keys (hash-table-keys ht)))
        (let loop
          ((keys keys))
          (if (null? keys)
            (if (procedure? failure)
              (failure)
              failure
            ) ;if
            (let* ((key (car keys))
                   (value (hash-table-ref ht key))
                  ) ;
              (if (proc key value)
                value
                (loop (cdr keys))
              ) ;if
            ) ;let*
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define hash-table-count
      (typed-lambda ((pred? procedure?) (ht hash-table?))
        (count (lambda (x) (pred? (car x) (cdr x)))
          (map values ht)
        ) ;count
      ) ;typed-lambda
    ) ;define

    (define (hash-table-fold proc seed ht)
      (assert-hash-table-type ht
        hash-table-fold
      ) ;assert-hash-table-type
      (let ((result seed))
        (hash-table-for-each (lambda (k v)
                               (set! result (proc k v result))
                             ) ;lambda
          ht
        ) ;hash-table-for-each
        result
      ) ;let
    ) ;define

    (define hash-table-for-each
      (typed-lambda ((proc procedure?) (ht hash-table?))
        (for-each (lambda (x) (proc (car x) (cdr x)))
          ht
        ) ;for-each
      ) ;typed-lambda
    ) ;define

    (define hash-table-map->list
      (typed-lambda ((proc procedure?) (ht hash-table?))
        (map (lambda (x) (proc (car x) (cdr x)))
          ht
        ) ;map
      ) ;typed-lambda
    ) ;define

    (define hash-table->alist
      (typed-lambda ((ht hash-table?))
        (append-map (lambda (x) (list (car x) (cdr x)))
          (map values ht)
        ) ;append-map
      ) ;typed-lambda
    ) ;define

    (define hash-table-copy
      (typed-lambda ((ht hash-table?) . rest)
        (let ((new-ht (make-hash-table))
              (mutable? (if (null? rest) #t (car rest))
              ) ;mutable?
             ) ;
          (hash-table-for-each (lambda (k v)
                                 (hash-table-set! new-ht k v)
                               ) ;lambda
            ht
          ) ;hash-table-for-each
          new-ht
        ) ;let
      ) ;typed-lambda
    ) ;define
  ) ;begin
) ;define-library
