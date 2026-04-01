;
; Copyright (C) 2020 Adam Nelson
;
; Permission is hereby granted, free of charge, to any person obtaining a
; copy of this software and associated documentation files (the
; "Software"), to deal in the Software without restriction, including
; without limitation the rights to use, copy, modify, merge, publish,
; distribute, sublicense, and/or sell copies of the Software, and to
; permit persons to whom the Software is furnished to do so, subject to
; the following conditions:
;
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
; CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
; TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
; SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
;
; Copyright (C) 2026 The Goldfish Scheme Authors
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

(define-library (srfi srfi-214)
  (export
    ;; Constructors
    make-flexvector flexvector
    ;; Predicates
    flexvector? flexvector-empty?
    ;; Accessors
    flexvector-ref flexvector-front flexvector-back
    flexvector-length
    ;; Mutators
    flexvector-set! flexvector-add! flexvector-add-back!
    flexvector-add-front! flexvector-remove! flexvector-remove-back!
    flexvector-remove-front! flexvector-remove-range!
    flexvector-clear! flexvector-fill! flexvector-swap! flexvector-reverse!
    ;; Conversion
    flexvector->vector vector->flexvector
    flexvector->list list->flexvector reverse-flexvector->list reverse-list->flexvector
    flexvector->string string->flexvector
    ;; Copying
    flexvector-copy flexvector-copy!
    flexvector-reverse-copy flexvector-reverse-copy!
    ;; Iteration
    flexvector-for-each flexvector-for-each/index
    flexvector-map flexvector-map! flexvector-map/index flexvector-map/index!
    flexvector-fold flexvector-fold-right
    flexvector-filter flexvector-filter! flexvector-filter/index flexvector-filter/index!
    flexvector-append-map flexvector-append-map/index
    flexvector-count flexvector-cumulate
    ;; Searching
    flexvector-index flexvector-index-right flexvector-skip flexvector-skip-right
    flexvector-any flexvector-every flexvector-binary-search
    ;; Partitioning
    flexvector-partition
    ;; Concatenation
    flexvector-append flexvector-concatenate flexvector-append-subvectors
    flexvector-append!
    ;; Comparison
    flexvector=?
    ;; Unfolding
    flexvector-unfold flexvector-unfold-right
    ;; Generators
    flexvector->generator generator->flexvector
  ) ;export

  (import (scheme base)
          (scheme case-lambda)
          (srfi srfi-1)
  ) ;import

  (begin

;;;; Utility

(define (assume condition . args)
  (if (not condition)
      (apply error args)
  ) ;if
) ;define

;;;; Record type and internal utilities

(define-record-type <flexvector>
  (%make-flexvector fv-vector fv-length)
  flexvector?
  (fv-vector vec set-vec!)
  (fv-length flexvector-length set-flexvector-length!)
) ;define-record-type

(define (cap fv)
  (vector-length (vec fv))
) ;define

(define (grow! fv)
  (define old-vec (vec fv))
  (define new-vec (make-vector (quotient (* (vector-length old-vec) 3) 2)))
  (vector-copy! new-vec 0 old-vec)
  (set-vec! fv new-vec)
  new-vec
) ;define

;;;; Constructors

(define make-flexvector
  (case-lambda
    ((size)
     (assume (>= size 0))
     (%make-flexvector (make-vector (max size 4)) size)
    ) ;
    ((size fill)
     (assume (>= size 0))
     (%make-flexvector (make-vector (max size 4) fill) size)
    ) ;
  ) ;case-lambda
) ;define

(define (flexvector . xs)
  (if (null? xs)
    (%make-flexvector (make-vector 4) 0)
    (list->flexvector xs)
  ) ;if
) ;define

;;;; Predicates

(define (flexvector-empty? fv)
  (assume (flexvector? fv))
  (zero? (flexvector-length fv))
) ;define

;;;; Accessors

(define (flexvector-ref fv index)
  (assume (flexvector? fv))
  (assume (integer? index))
  (assume (< -1 index (flexvector-length fv)))
  (vector-ref (vec fv) index)
) ;define

(define (flexvector-front fv)
  (assume (flexvector? fv))
  (assume (not (flexvector-empty? fv)))
  (flexvector-ref fv 0)
) ;define

(define (flexvector-back fv)
  (assume (flexvector? fv))
  (assume (not (flexvector-empty? fv)))
  (flexvector-ref fv (- (flexvector-length fv) 1))
) ;define

;;;; Mutators

(define (flexvector-set! fv index x)
  (assume (flexvector? fv))
  (assume (integer? index))
  (assume (< -1 index (flexvector-length fv)))
  (let ((last-value (vector-ref (vec fv) index)))
    (vector-set! (vec fv) index x)
    last-value
  ) ;let
) ;define

(define flexvector-add!
  (case-lambda
    ((fv i x)
     (assume (flexvector? fv))
     (assume (integer? i))
     (let* ((len (flexvector-length fv))
            (v (if (< len (cap fv)) (vec fv) (grow! fv))))
       (when (or (< i 0) (> i len))
         (error 'value-error "flexvector-add!: index out of bounds"))
       (vector-copy! v (+ i 1) v i len)
       (vector-set! v i x)
       (set-flexvector-length! fv (+ len 1))
       fv
     ) ;let*
    ) ;
    ((fv i . xs)
     (flexvector-add-all! fv i xs)
    ) ;
  ) ;case-lambda
) ;define

(define flexvector-add-back!
  (case-lambda
    ((fv x)
     (assume (flexvector? fv))
     (let* ((len (flexvector-length fv))
            (v (if (< len (cap fv)) (vec fv) (grow! fv))))
       (vector-set! v len x)
       (set-flexvector-length! fv (+ len 1))
       fv
     ) ;let*
    ) ;
    ((fv x . xs)
     (flexvector-add-back! fv x)
     (apply flexvector-add-back! fv xs)
    ) ;
  ) ;case-lambda
) ;define

(define (flexvector-add-all! fv i xs)
  (assume (flexvector? fv))
  (assume (integer? i))
  (assume (list? xs))
  (let* ((len (flexvector-length fv))
         (xv (list->vector xs))
         (xvlen (vector-length xv))
         (v (let lp ((v (vec fv)))
              (if (< (+ len xvlen) (vector-length v)) v (lp (grow! fv)))))
         ) ;v
    (when (or (< i 0) (> i len))
      (error 'value-error "flexvector-add!: index out of bounds"))
    (vector-copy! v (+ i xvlen) v i len)
    (vector-copy! v i xv 0 xvlen)
    (set-flexvector-length! fv (+ len xvlen))
    fv
  ) ;let*
) ;define

(define flexvector-add-front!
  (case-lambda
    ((fv x) (flexvector-add! fv 0 x))
    ((fv . xs) (apply flexvector-add! fv 0 xs))
  ) ;case-lambda
) ;define

(define (flexvector-remove! fv i)
  (assume (flexvector? fv))
  (assume (integer? i))
  (assume (<= 0 i (- (flexvector-length fv) 1)))
  (let ((removed (flexvector-ref fv i)))
    (flexvector-remove-range! fv i (+ i 1))
    removed
  ) ;let
) ;define

(define (flexvector-remove-range! fv start end)
  (assume (flexvector? fv))
  (let ((len (flexvector-length fv)))
    (when (< start 0) (set! start 0))
    (when (>= end len) (set! end len))
    (assume (<= start end))
    (vector-copy! (vec fv) start (vec fv) end)
    (let ((new-len (- len (- end start))))
      (vector-fill! (vec fv) #f new-len len)
      (set-flexvector-length! fv new-len)
    ) ;let
  ) ;let
  fv
) ;define

(define (flexvector-remove-front! fv)
  (assume (flexvector? fv))
  (assume (not (flexvector-empty? fv)))
  (flexvector-remove! fv 0)
) ;define

(define (flexvector-remove-back! fv)
  (assume (flexvector? fv))
  (assume (not (flexvector-empty? fv)))
  (flexvector-remove! fv (- (flexvector-length fv) 1))
) ;define

(define (flexvector-clear! fv)
  (assume (flexvector? fv))
  (set-vec! fv (make-vector 4))
  (set-flexvector-length! fv 0)
  fv
) ;define

(define flexvector-fill!
  (case-lambda
    ((fv fill)
     (flexvector-fill! fv fill 0 (flexvector-length fv))
    ) ;
    ((fv fill start)
     (flexvector-fill! fv fill start (flexvector-length fv))
    ) ;
    ((fv fill start end)
     (let ((actual-end (min end (flexvector-length fv))))
       (do ((i (max 0 start) (+ i 1)))
           ((>= i actual-end))
         (flexvector-set! fv i fill)
       ) ;do
     ) ;let
    ) ;
  ) ;case-lambda
) ;define

(define (flexvector-swap! fv i j)
  (assume (flexvector? fv))
  (assume (integer? i))
  (assume (integer? j))
  (let ((tmp (flexvector-ref fv i)))
    (flexvector-set! fv i (flexvector-ref fv j))
    (flexvector-set! fv j tmp)
  ) ;let
) ;define

(define (flexvector-reverse! fv . o)
  (assume (flexvector? fv))
  (let lp ((left (if (pair? o) (car o) 0))
           (right (- (if (and (pair? o) (pair? (cdr o)))
                         (cadr o)
                         (flexvector-length fv))
                     1))
           ) ;right
    (cond
      ((>= left right) (if #f #f))
      (else
        (flexvector-swap! fv left right)
        (lp (+ left 1) (- right 1))
      ) ;else
    ) ;cond
  ) ;let
) ;define

;;;; Conversion

(define vector->flexvector
  (case-lambda
    ((vec)
     (assume (vector? vec))
     (vector->flexvector vec 0 (vector-length vec))
    ) ;
    ((vec start)
     (assume (vector? vec))
     (vector->flexvector vec start (vector-length vec))
    ) ;
    ((vec start end)
     (assume (vector? vec))
     (assume (<= 0 start end (vector-length vec)))
     (let ((len (- end start)))
       (cond
         ((< len 4)
           (let ((new-vec (make-vector 4)))
             (vector-copy! new-vec 0 vec start end)
             (%make-flexvector new-vec len)
           ) ;let
         ) ;
         (else
           (%make-flexvector (vector-copy vec start end) len)
         ) ;else
       ) ;cond
     ) ;let
    ) ;
  ) ;case-lambda
) ;define

(define flexvector->vector
  (case-lambda
    ((fv)
     (assume (flexvector? fv))
     (flexvector->vector fv 0 (flexvector-length fv))
    ) ;
    ((fv start)
     (assume (flexvector? fv))
     (flexvector->vector fv start (flexvector-length fv))
    ) ;
    ((fv start end)
     (assume (flexvector? fv))
     (assume (<= 0 start end (flexvector-length fv)))
     (vector-copy (vec fv) start end)
    ) ;
  ) ;case-lambda
) ;define

(define (list->flexvector xs)
  (let* ((vec (list->vector xs))
         (len (vector-length vec)))
    (cond
      ((< len 4)
        (let ((new-vec (make-vector 4)))
          (vector-copy! new-vec 0 vec)
          (%make-flexvector new-vec len)
        ) ;let
      ) ;
      (else
        (%make-flexvector vec len)
      ) ;else
    ) ;cond
  ) ;let*
) ;define

(define flexvector->list
  (case-lambda
    ((fv)
     (flexvector->list fv 0 (flexvector-length fv))
    ) ;
    ((fv start)
     (flexvector->list fv start (flexvector-length fv))
    ) ;
    ((fv start end)
     (if (< end start)
       (error "invalid start/end specification")
       (let lp ((acc '()) (idx (- end 1)))
         (if (< idx start)
           acc
           (lp (cons (vector-ref (vec fv) idx) acc)
               (- idx 1)
           ) ;lp
         ) ;if
       ) ;let
     ) ;if
    ) ;
  ) ;case-lambda
) ;define

(define (reverse-flexvector->list fv . o)
  (assume (flexvector? fv))
  (flexvector->list (apply flexvector-reverse-copy fv o))
) ;define

(define (reverse-list->flexvector ls)
  (assume (list? ls))
  (let ((fv (list->flexvector ls)))
    (flexvector-reverse! fv)
    fv
  ) ;let
) ;define

(define (string->flexvector s . o)
  (assume (string? s))
  (vector->flexvector (apply string->vector s o))
) ;define

(define (flexvector->string fv . o)
  (assume (flexvector? fv))
  (vector->string (apply flexvector->vector fv o))
) ;define

;;;; Copying

(define flexvector-copy
  (case-lambda
    ((fv)
     (assume (flexvector? fv))
     (%make-flexvector (vector-copy (vec fv))
                       (flexvector-length fv)
     ) ;%make-flexvector
    ) ;
    ((fv start)
     (assume (flexvector? fv))
     (flexvector-copy fv start (flexvector-length fv))
    ) ;
    ((fv start end)
     (assume (flexvector? fv))
     (assume (<= 0 start end (flexvector-length fv)))
     (vector->flexvector (vector-copy (vec fv) start end))
    ) ;
  ) ;case-lambda
) ;define

(define flexvector-copy!
  (case-lambda
    ((to at from)
     (assume (flexvector? from))
     (flexvector-copy! to at from 0 (flexvector-length from))
    ) ;
    ((to at from start)
     (assume (flexvector? from))
     (flexvector-copy! to at from start (flexvector-length from))
    ) ;
    ((to at from start end)
     (assume (flexvector? to))
     (assume (<= 0 at (flexvector-length to)))
     (assume (<= 0 start end (flexvector-length from)))
     (let* ((vf (vec from))
            (lt (+ (flexvector-length to) (- end start)))
            (vt (let lp ((v (vec to)))
                  (if (< lt (vector-length v)) v (lp (grow! to)))))
            ) ;vt
       (vector-copy! vt at vf start end)
       (set-flexvector-length! to
         (max (flexvector-length to) (+ at (- end start)))
       ) ;set-flexvector-length!
     ) ;let*
    ) ;
  ) ;case-lambda
) ;define

(define flexvector-reverse-copy
  (case-lambda
    ((fv)
     (define fv2 (flexvector-copy fv))
     (flexvector-reverse! fv2)
     fv2
    ) ;
    (args
     (define fv (apply flexvector-copy args))
     (flexvector-reverse! fv)
     fv
    ) ;args
  ) ;case-lambda
) ;define

(define flexvector-reverse-copy!
  (case-lambda
    ((to at from)
     (assume (flexvector? from))
     (flexvector-reverse-copy! to at from 0 (flexvector-length from))
    ) ;
    ((to at from start)
     (assume (flexvector? from))
     (flexvector-reverse-copy! to at from start (flexvector-length from))
    ) ;
    ((to at from start end)
     (flexvector-copy! to at from start end)
     (flexvector-reverse! to at (+ at (- end start)))
    ) ;
  ) ;case-lambda
) ;define

;;;; Iteration

(define flexvector-for-each/index
  (case-lambda
    ((proc fv)
     (assume (procedure? proc))
     (assume (flexvector? fv))
     (let ((len (flexvector-length fv)))
       (do ((i 0 (+ i 1))) ((= i len))
         (proc i (flexvector-ref fv i))
       ) ;do
     ) ;let
    ) ;
    ((proc . fvs)
     (assume (procedure? proc))
     (let ((len (apply min (map flexvector-length fvs))))
       (do ((i 0 (+ i 1))) ((= i len))
         (apply proc i (map (lambda (fv) (flexvector-ref fv i)) fvs))
       ) ;do
     ) ;let
    ) ;
  ) ;case-lambda
) ;define

(define flexvector-for-each
  (case-lambda
    ((proc fv)
     (assume (procedure? proc))
     (flexvector-for-each/index (lambda (i x) (proc x)) fv)
    ) ;
    ((proc . fvs)
     (assume (procedure? proc))
     (apply flexvector-for-each/index (lambda (i . xs) (apply proc xs)) fvs)
    ) ;
  ) ;case-lambda
) ;define

(define flexvector-map/index!
  (case-lambda
    ((proc fv)
     (assume (procedure? proc))
     (assume (flexvector? fv))
     (flexvector-for-each/index
       (lambda (i x) (flexvector-set! fv i (proc i x)))
       fv
     ) ;flexvector-for-each/index
     fv
    ) ;
    ((proc fv . fvs)
     (assume (procedure? proc))
     (assume (flexvector? fv))
     (apply flexvector-for-each/index
       (lambda (i . xs) (flexvector-set! fv i (apply proc i xs)))
       fv
       fvs
     ) ;apply
     fv
    ) ;
  ) ;case-lambda
) ;define

(define flexvector-map!
  (case-lambda
    ((proc fv)
     (assume (procedure? proc))
     (flexvector-map/index! (lambda (i x) (proc x)) fv)
    ) ;
    ((proc . fvs)
     (assume (procedure? proc))
     (apply flexvector-map/index! (lambda (i . xs) (apply proc xs)) fvs)
    ) ;
  ) ;case-lambda
) ;define

(define (flexvector-map/index proc fv . fvs)
  (assume (flexvector? fv))
  (apply flexvector-map/index! proc (flexvector-copy fv) fvs)
) ;define

(define (flexvector-map proc fv . fvs)
  (assume (flexvector? fv))
  (apply flexvector-map! proc (flexvector-copy fv) fvs)
) ;define

(define (flexvector-append-map/index proc fv . fvs)
  (define out (flexvector))
  (flexvector-for-each
    (lambda (x) (flexvector-append! out x))
    (apply flexvector-map/index proc fv fvs)
  ) ;flexvector-for-each
  out
) ;define

(define (flexvector-append-map proc fv . fvs)
  (define out (flexvector))
  (flexvector-for-each
    (lambda (x) (flexvector-append! out x))
    (apply flexvector-map proc fv fvs)
  ) ;flexvector-for-each
  out
) ;define

(define (flexvector-fold kons knil fv1 . o)
  (assume (procedure? kons))
  (assume (flexvector? fv1))
  (let ((len (flexvector-length fv1)))
    (if (null? o)
        (let lp ((i 0) (acc knil))
          (if (>= i len) acc (lp (+ i 1) (kons acc (flexvector-ref fv1 i))))
        ) ;let
        (let lp ((i 0) (acc knil))
          (if (>= i len)
              acc
              (lp (+ i 1)
                  (apply kons acc (flexvector-ref fv1 i)
                         (map (lambda (fv) (flexvector-ref fv i)) o)
                  ) ;apply
              ) ;lp
          ) ;if
        ) ;let
    ) ;if
  ) ;let
) ;define

(define (flexvector-fold-right kons knil fv1 . o)
  (assume (procedure? kons))
  (assume (flexvector? fv1))
  (let ((len (flexvector-length fv1)))
    (if (null? o)
        (let lp ((i (- len 1)) (acc knil))
          (if (negative? i) acc (lp (- i 1) (kons acc (flexvector-ref fv1 i))))
        ) ;let
        (let lp ((i (- len 1)) (acc knil))
          (if (negative? i)
              acc
              (lp (- i 1)
                  (apply kons acc (flexvector-ref fv1 i)
                         (map (lambda (fv) (flexvector-ref fv i)) o)
                  ) ;apply
              ) ;lp
          ) ;if
        ) ;let
    ) ;if
  ) ;let
) ;define

(define flexvector-filter/index!
  (case-lambda
    ((pred? fv)
     (assume (flexvector? fv))
     (let ((v (vec fv)) (len (flexvector-length fv)))
       (let lp ((i 0) (j 0))
         (cond
           ((>= i len)
             (set-flexvector-length! fv j)
             fv
           ) ;
           ((pred? i (vector-ref v i))
             (unless (= i j) (vector-set! v j (vector-ref v i)))
             (lp (+ i 1) (+ j 1))
           ) ;
           (else
             (lp (+ i 1) j)
           ) ;else
         ) ;cond
       ) ;let
     ) ;let
    ) ;
    ((pred? fv . fvs)
     (assume (flexvector? fv))
     (let ((v (vec fv)) (len (flexvector-length fv)))
       (let lp ((i 0) (j 0))
         (cond
           ((>= i len)
             (set-flexvector-length! fv j)
             fv
           ) ;
           ((apply pred?
                   i
                   (vector-ref v i)
                   (map (lambda (fv) (flexvector-ref fv i)) fvs))
             (unless (= i j) (vector-set! v j (vector-ref v i)))
             (lp (+ i 1) (+ j 1))
           ) ;
           (else
             (lp (+ i 1) j)
           ) ;else
         ) ;cond
       ) ;let
     ) ;let
    ) ;
  ) ;case-lambda
) ;define

(define flexvector-filter!
  (case-lambda
    ((pred? fv)
     (assume (procedure? pred?))
     (assume (flexvector? fv))
     (flexvector-filter/index! (lambda (i x) (pred? x)) fv)
    ) ;
    ((pred? . fvs)
     (assume (procedure? pred?))
     (apply flexvector-filter/index! (lambda (i . xs) (apply pred? xs)) fvs)
    ) ;
  ) ;case-lambda
) ;define

(define (flexvector-filter/index proc fv . fvs)
  (assume (flexvector? fv))
  (apply flexvector-filter/index! proc (flexvector-copy fv) fvs)
) ;define

(define (flexvector-filter proc fv . fvs)
  (assume (flexvector? fv))
  (apply flexvector-filter! proc (flexvector-copy fv) fvs)
) ;define

(define (flexvector-count pred? fv1 . o)
  (assume (procedure? pred?))
  (assume (flexvector? fv1))
  (apply flexvector-fold
         (lambda (count . x) (+ count (if (apply pred? x) 1 0)))
         0
         fv1 o
  ) ;apply
) ;define

(define (flexvector-cumulate f knil fv)
  (assume (procedure? f))
  (assume (flexvector? fv))
  (let* ((len (flexvector-length fv))
         (res (make-vector len)))
    (let lp ((i 0) (acc knil))
      (if (>= i len)
          (vector->flexvector res)
          (let ((acc (f acc (flexvector-ref fv i))))
            (vector-set! res i acc)
            (lp (+ i 1) acc)
          ) ;let
      ) ;if
    ) ;let
  ) ;let*
) ;define

;;;; Searching

(define (flexvector-index pred? fv1 . o)
  (assume (procedure? pred?))
  (assume (flexvector? fv1))
  (let ((len (flexvector-length fv1)))
    (let lp ((i 0))
      (and (< i len)
           (if (apply pred?
                      (flexvector-ref fv1 i)
                      (map (lambda (fv) (flexvector-ref fv i)) o))
               i
               (lp (+ i 1))
           ) ;if
      ) ;and
    ) ;let
  ) ;let
) ;define

(define (flexvector-index-right pred? fv1 . o)
  (assume (procedure? pred?))
  (assume (flexvector? fv1))
  (let ((len (flexvector-length fv1)))
    (let lp ((i (- len 1)))
      (and (>= i 0)
           (if (apply pred?
                      (flexvector-ref fv1 i)
                      (map (lambda (fv) (flexvector-ref fv i)) o))
               i
               (lp (- i 1))
           ) ;if
      ) ;and
    ) ;let
  ) ;let
) ;define

(define (complement f)
  (lambda args (not (apply f args)))
) ;define

(define (flexvector-skip pred? fv1 . o)
  (assume (procedure? pred?))
  (assume (flexvector? fv1))
  (apply flexvector-index (complement pred?) fv1 o)
) ;define

(define (flexvector-skip-right pred? fv1 . o)
  (assume (procedure? pred?))
  (assume (flexvector? fv1))
  (apply flexvector-index-right (complement pred?) fv1 o)
) ;define

(define flexvector-binary-search
  (case-lambda
    ((fv value cmp)
     (flexvector-binary-search fv value cmp 0 (flexvector-length fv))
    ) ;
    ((fv value cmp start)
     (flexvector-binary-search fv value cmp start (flexvector-length fv))
    ) ;
    ((fv value cmp start end)
     (assume (flexvector? fv))
     (assume (procedure? cmp))
     (assume (integer? start))
     (assume (integer? end))
     (assume (<= start end))
     (let lp ((lo (max start 0))
              (hi (- (min end (flexvector-length fv)) 1)))
       (and (<= lo hi)
            (let* ((mid (quotient (+ lo hi) 2))
                   (x (flexvector-ref fv mid))
                   (y (cmp value x)))
              (cond
                ((< y 0) (lp lo (- mid 1)))
                ((> y 0) (lp (+ mid 1) hi))
                (else mid)
              ) ;cond
            ) ;let*
       ) ;and
     ) ;let
    ) ;
  ) ;case-lambda
) ;define

(define (flexvector-any pred? fv . o)
  (assume (procedure? pred?))
  (assume (flexvector? fv))
  (let ((len (apply min (flexvector-length fv) (map flexvector-length o))))
    (let lp ((i 0))
      (and (< i len)
           (or (apply pred?
                      (flexvector-ref fv i)
                      (map (lambda (v) (flexvector-ref v i)) o))
               (lp (+ i 1))
           ) ;or
      ) ;and
    ) ;let
  ) ;let
) ;define

(define (flexvector-every pred? fv . o)
  (assume (procedure? pred?))
  (assume (flexvector? fv))
  (let ((len (apply min (flexvector-length fv) (map flexvector-length o))))
    (or (zero? len)
      (let lp ((i 0))
        (let ((x (apply pred?
                        (flexvector-ref fv i)
                        (map (lambda (v) (flexvector-ref v i)) o))))
          (if (= i (- len 1))
              x
              (and x (lp (+ i 1)))
          ) ;if
        ) ;let
      ) ;let
    ) ;or
  ) ;let
) ;define

;;;; Partitioning

(define (flexvector-partition pred? fv)
  (assume (procedure? pred?))
  (assume (flexvector? fv))
  (let ((left (flexvector)) (right (flexvector)))
    (flexvector-for-each
      (lambda (x) (flexvector-add-back! (if (pred? x) left right) x))
      fv
    ) ;flexvector-for-each
    (values left right)
  ) ;let
) ;define

;;;; Concatenation

(define (flexvector-append fv . fvs)
  (assume (flexvector? fv))
  (apply flexvector-append! (flexvector-copy fv) fvs)
) ;define

(define (flexvector-concatenate ls)
  (apply flexvector-append ls)
) ;define

(define (flexvector-append-subvectors . o)
  (let lp ((ls o) (vecs '()))
    (if (null? ls)
        (flexvector-concatenate (reverse vecs))
        (lp (cdr (cddr ls))
            (cons (flexvector-copy (car ls) (cadr ls) (car (cddr ls))) vecs)
        ) ;lp
    ) ;if
  ) ;let
) ;define

(define (flexvector-append! fv . fvs)
  (assume (flexvector? fv))
  (assume (every flexvector? fvs))
  (for-each
    (lambda (fv2) (flexvector-copy! fv (flexvector-length fv) fv2))
    fvs
  ) ;for-each
  fv
) ;define

;;;; Comparison

(define (flexvector=? eq . o)
  (cond
    ((null? o) #t)
    ((null? (cdr o)) #t)
    (else
      (and (let* ((fv1 (car o))
                  (fv2 (cadr o))
                  (len (flexvector-length fv1)))
             (and (= len (flexvector-length fv2))
                  (let lp ((i 0))
                    (or (>= i len)
                        (and (eq (flexvector-ref fv1 i) (flexvector-ref fv2 i))
                             (lp (+ i 1)))
                        ) ;and
                    ) ;or
                  ) ;let
             ) ;and
           (apply flexvector=? eq (cdr o))
      ) ;and
    ) ;else
  ) ;cond
) ;define

;;;; Unfolding

(define flexvector-unfold
  (case-lambda
    ((p f g seed)
     (define fv (flexvector))
     (assume (procedure? p))
     (assume (procedure? f))
     (assume (procedure? g))
     (do ((seed seed (g seed))) ((p seed) fv)
       (flexvector-add-back! fv (f seed))
     ) ;do
    ) ;
    ((p f g . seeds)
     (define fv (flexvector))
     (assume (procedure? p))
     (assume (procedure? f))
     (assume (procedure? g))
     (do ((seeds seeds (let-values ((seeds (apply g seeds))) seeds)))
         ((apply p seeds) fv)
       (flexvector-add-back! fv (apply f seeds))
     ) ;do
    ) ;
  ) ;case-lambda
) ;define

(define (flexvector-unfold-right . args)
  (define fv (apply flexvector-unfold args))
  (flexvector-reverse! fv)
  fv
) ;define

;;;; Generators

(define (generator->flexvector g)
  (assume (procedure? g))
  (flexvector-unfold eof-object? (lambda (x) x) (lambda (_) (g)) (g))
) ;define

(define (flexvector->generator fv)
  (assume (flexvector? fv))
  (let ((i 0))
    (lambda ()
      (if (< i (flexvector-length fv))
        (let ((element (flexvector-ref fv i)))
          (set! i (+ i 1))
          element
        ) ;let
        (eof-object)
      ) ;if
    ) ;lambda
  ) ;let
) ;define

) ;define-library
) ;define-library
