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

(define-library (liii vector)
  (import (scheme base)
          (srfi srfi-133)
          (srfi srfi-13)
  ) ;import
  (export
    ; R7RS 6.8 Vector 函数从 (scheme base) 继承
    ; SRFI-133 扩展函数
    vector-empty?
    vector-fold vector-fold-right
    vector-count
    vector-any vector-every
    vector-index vector-index-right vector-skip vector-skip-right vector-partition
    vector-swap! vector-reverse! vector-cumulate reverse-list->vector
    vector= vector-contains?
    ; Liii Extras
    vector-filter vector-contains?
    ; Scala-style take/drop with boundary tolerance
    vector-take vector-drop vector-take-right vector-drop-right
    ; int-vector 函数 (s7 内置)
    int-vector int-vector?
    make-int-vector
    int-vector-ref int-vector-set!
    ; complex-vector 函数 (s7 内置)
    complex-vector complex-vector?
    make-complex-vector
    complex-vector-ref complex-vector-set!
    ; float-vector 函数 (s7 内置)
    float-vector float-vector?
    make-float-vector
    float-vector-ref float-vector-set!
  ) ;export
  (begin

    (define (vector-filter pred vec)
      (let*
        ((result-list
           (vector-fold (lambda (elem acc)
                          (if (pred elem)
                              (cons elem acc)
                              acc)
                          ) ;if
                        '()
                        vec)
           ) ;vector-fold
         (result-length (length result-list))
         (result-vec (make-vector result-length))
        ) ;
        (let loop ((i (- result-length 1)) (lst result-list))
          (if (null? lst)
              result-vec
              (begin
                (vector-set! result-vec i (car lst))
                (loop (- i 1) (cdr lst))
              ) ;begin
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (vector-contains? vec elem . args)
      (let ((cmp (if (null? args) equal? (car args))))
        (not (not (vector-index (lambda (x) (cmp x elem)) vec)))
      ) ;let
    ) ;define

    (define (vector-take vec n)
      (unless (vector? vec)
        (type-error "vector-take: first argument must be a vector" vec)
      ) ;unless
      (unless (integer? n)
        (type-error "vector-take: second argument must be an integer" n)
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) (vector))
              ((>= n len) vec)
              (else (vector-copy vec 0 n))
        ) ;cond
      ) ;let
    ) ;define

    (define (vector-drop vec n)
      (unless (vector? vec)
        (type-error "vector-drop: first argument must be a vector" vec)
      ) ;unless
      (unless (integer? n)
        (type-error "vector-drop: second argument must be an integer" n)
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) vec)
              ((>= n len) (vector))
              (else (vector-copy vec n))
        ) ;cond
      ) ;let
    ) ;define

    (define (vector-take-right vec n)
      (unless (vector? vec)
        (type-error "vector-take-right: first argument must be a vector" vec)
      ) ;unless
      (unless (integer? n)
        (type-error "vector-take-right: second argument must be an integer" n)
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) (vector))
              ((>= n len) vec)
              (else (vector-copy vec (- len n)))
        ) ;cond
      ) ;let
    ) ;define

    (define (vector-drop-right vec n)
      (unless (vector? vec)
        (type-error "vector-drop-right: first argument must be a vector" vec)
      ) ;unless
      (unless (integer? n)
        (type-error "vector-drop-right: second argument must be an integer" n)
      ) ;unless
      (let ((len (vector-length vec)))
        (cond ((< n 0) vec)
              ((>= n len) (vector))
              (else (vector-copy vec 0 (- len n)))
        ) ;cond
      ) ;let
    ) ;define

  ) ;begin
) ;define-library

