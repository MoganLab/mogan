;
; Copyright (C) 2024 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (liii list)
  (export
    ; S7 built-in 和 R7RS 6.4 函数从 (scheme base) 继承
    ; SRFI 1: Constructors
    circular-list iota xcons cons*
    ; SRFI 1: Predicates
    null-list? circular-list? proper-list? dotted-list?
    ; SRFI 1: Selectors
    first second third fourth fifth sixth seventh eighth ninth tenth
    take drop take-right drop-right split-at
    last-pair last
    ; SRFI 1: Miscellaneous: length, append, concatenate, reverse, zip & count
    zip count
    ; SRFI 1: fold, unfold & map
    fold fold-right reduce reduce-right
    filter partition remove append-map
    ; SRFI 1: Searching
    find any every list-index
    take-while drop-while
    ; SRFI 1: Deleting
    delete
    ; SRFI 1: Association List (assoc assq assv 从 scheme base 继承)
    alist-cons
    ; Liii List extensions
    flat-map
    list-null? list-not-null? not-null-list?
    length=? length>? length>=? flatten
    ; Scala-style take/drop with boundary tolerance
    list-take list-drop list-take-right list-drop-right
  ) ;export
  (import (scheme base)
          (srfi srfi-1)
          (srfi srfi-13)
          (liii error)
  ) ;import
  (begin

    (define (length=? x scheme-list)
      (when (not (integer? x))
        (type-error "length=?: first parameter x must be an integer")
      ) ;when
      (when (< x 0)
        (value-error "length=?: expected non-negative integer x but received ~d" x)
      ) ;when
      (cond ((and (= x 0) (null? scheme-list)) #t)
            ((or (= x 0) (null? scheme-list)) #f)
            (else (length=? (- x 1) (cdr scheme-list)))
      ) ;cond
    ) ;define

    (define (length>? lst len)
      (let loop ((lst lst)
                 (cnt 0))
        (cond ((null? lst) (< len cnt))
              ((pair? lst) (loop (cdr lst) (+ cnt 1)))
              (else (< len cnt))
        ) ;cond
      ) ;let
    ) ;define

    (define (length>=? lst len)
      (let loop ((lst lst)
                 (cnt 0))
        (cond ((null? lst) (<= len cnt))
              ((pair? lst) (loop (cdr lst) (+ cnt 1)))
              (else (<= len cnt))
        ) ;cond
      ) ;let
    ) ;define

    (define flat-map append-map)

    (define (list-take lst n)
      (unless (list? lst)
        (type-error "list-take: first argument must be a list" lst)
      ) ;unless
      (unless (integer? n)
        (type-error "list-take: second argument must be an integer" n)
      ) ;unless
      (cond ((< n 0) '())
            ((>= n (length lst)) lst)
            (else (take lst n))
      ) ;cond
    ) ;define

    (define (list-drop lst n)
      (unless (list? lst)
        (type-error "list-drop: first argument must be a list" lst)
      ) ;unless
      (unless (integer? n)
        (type-error "list-drop: second argument must be an integer" n)
      ) ;unless
      (cond ((< n 0) lst)
            ((>= n (length lst)) '())
            (else (drop lst n))
      ) ;cond
    ) ;define

    (define (list-take-right lst n)
      (unless (list? lst)
        (type-error "list-take-right: first argument must be a list" lst)
      ) ;unless
      (unless (integer? n)
        (type-error "list-take-right: second argument must be an integer" n)
      ) ;unless
      (cond ((< n 0) '())
            ((>= n (length lst)) lst)
            (else (take-right lst n))
      ) ;cond
    ) ;define

    (define (list-drop-right lst n)
      (unless (list? lst)
        (type-error "list-drop-right: first argument must be a list" lst)
      ) ;unless
      (unless (integer? n)
        (type-error "list-drop-right: second argument must be an integer" n)
      ) ;unless
      (cond ((< n 0) lst)
            ((>= n (length lst)) '())
            (else (drop-right lst n))
      ) ;cond
    ) ;define

    (define (not-null-list? l)
      (cond
        ((pair? l)
         (or (null? (cdr l)) (pair? (cdr l)))
        ) ;
        ((null? l) #f)
        (else
         (error 'type-error "type mismatch")
        ) ;else
      ) ;cond
    ) ;define

    (define (list-null? l)
      (and (not (pair? l)) (null? l))
    ) ;define

    (define (list-not-null? l)
      (and (pair? l)
           (or (null? (cdr l)) (pair? (cdr l)))
      ) ;and
    ) ;define

    (define* (flatten lst (depth 1))
      (define (flatten-depth-iter rest depth res-node)
        (if (null? rest)
            res-node
            (let ((first (car rest))
                  (tail  (cdr rest)))
              (cond
                ((and (null? first) (not (= 0 depth)))
                 (flatten-depth-iter tail depth res-node)
                ) ;
                ((or (= depth 0) (not (pair? first)))
                 (set-cdr! res-node (cons first '()))
                 (flatten-depth-iter tail depth (cdr res-node))
                ) ;
                (else
                 (flatten-depth-iter
                  tail
                  depth
                  (flatten-depth-iter
                   first
                   (- depth 1)
                   res-node
                  ) ;flatten-depth-iter
                 ) ;flatten-depth-iter
                ) ;else
              ) ;cond
            ) ;let
        ) ;if
      ) ;define
      (define (flatten-depth lst depth)
        (let ((res (cons #f '())))
          (flatten-depth-iter lst depth res)
          (cdr res)
        ) ;let
      ) ;define

      (define (flatten-deepest-iter rest res-node)
        (if (null? rest)
          res-node
          (let ((first (car rest))
                (tail  (cdr rest)))
            (cond
              ((pair? first)
               (flatten-deepest-iter
                tail
                (flatten-deepest-iter
                 first
                 res-node)
                ) ;flatten-deepest-iter
               ) ;flatten-deepest-iter
              ((null? first)
               (flatten-deepest-iter tail res-node)
              ) ;
              (else
               (set-cdr! res-node (cons first '()))
               (flatten-deepest-iter tail (cdr res-node))
              ) ;else
            ) ;cond
          ) ;let
        ) ;if
      ) ;define
      (define (flatten-deepest lst)
        (let ((res (cons #f '())))
          (flatten-deepest-iter lst res)
          (cdr res)
        ) ;let
      ) ;define

      (cond
        ((eq? depth 'deepest)
         (flatten-deepest lst)
        ) ;
        ((integer? depth)
         (flatten-depth lst depth)
        ) ;
        (else
         (type-error
          (string-append
            "flatten: the second argument depth should be symbol "
            "`deepest' or a integer, which will be uesd as depth,"
            " but got a ~A") depth)))
    ) ;define*

  ) ;begin
) ;define-library

