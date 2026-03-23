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

(define-library (srfi srfi-133)
  (import (liii base))
  (export vector-empty? vector-fold vector-fold-right vector-count vector-any vector-every
          vector-copy vector-copy! vector-index vector-index-right vector-skip vector-skip-right
          vector-partition vector-swap! vector-reverse! vector-cumulate reverse-list->vector vector=
  ) ;export
  (begin

    (define (vector-empty? v)
      (when (not (vector? v))
        (error 'type-error "v is not a vector")
      ) ;when
      (zero? (vector-length v))
    ) ;define

    (define (vector= elt=? . rest)
      (define compare2vecs
        (typed-lambda ((cmp procedure?) (vec1 vector?) (vec2 vector?))
          (let* ((len1 (vector-length vec1))
                 (len2 (vector-length vec2)))
            (if (not (= len1 len2))
                #f
                (let loop ((ilhs 0)
                           (irhs 0)
                           (len len1))
                  (if (= ilhs len)
                      #t
                      (if (not (cmp (vec1 ilhs) (vec2 irhs)))
                          #f
                          (loop (+ 1 ilhs) (+ 1 irhs) len)
                      ) ;if
                  ) ;if
                ) ;let
            ) ;if
          ) ;let*
        ) ;typed-lambda
      ) ;define
      (when (not (procedure? elt=?))
        (error 'type-error "elt=? should be a procedure")
      ) ;when
      (if (or (null? rest) (= 1 (length rest)))
          #t
          (let loop ((vec1 (car rest))
                     (vec2 (car (cdr rest)))
                     (vrest (cdr (cdr rest))))
            (let ((rst (compare2vecs elt=? vec1 vec2)))
              (when (not (boolean? rst))
                (error 'type-error "elt=> should return bool")
              ) ;when
              (if (compare2vecs elt=? vec1 vec2)
                  (if (null? vrest)
                      #t
                      (loop vec2 (car vrest) (cdr vrest))
                  ) ;if
                  #f
              ) ;if
            ) ;let
          ) ;let
      ) ;if
    ) ;define
    (define (vector-fold f initial vec)
      (let loop ((i 0)
                 (acc initial))
        (if (< i (vector-length vec))
            (loop (+ i 1) (f (vector-ref vec i) acc))
            acc
        ) ;if
      ) ;let
    ) ;define

    (define (vector-fold-right f initial vec)
      (let loop ((i (- (vector-length vec) 1))
                 (acc initial))
        (if (>= i 0)
            (loop (- i 1) (f (vector-ref vec i) acc))
            acc
        ) ;if
      ) ;let
    ) ;define

    ; TODO optional parameters
    (define (vector-count pred v)
      (let loop ((i 0)
                 (count 0))
        (cond ((= i (vector-length v)) count)
              ((pred (vector-ref v i)) (loop (+ i 1) (+ count 1)))
              (else (loop (+ i 1) count))
        ) ;cond
      ) ;let
    ) ;define

    ; Return a new vector v-rst with same length of input vector vec.
    ; Every element of the result is the result the i-th iteration of fn cumu_i vec_i.
    ; Where fn should be a procedure with 2 args.
    ; The type of knil and vector could be different.
    ; In the i-th iteration, cumu_i = fn cumu_(i-1) vec_i, with cumu_0 = fn knil vec_0.

    (define vector-cumulate
      (typed-lambda ((fn procedure?) knil (vec vector?))
        (let* ((len (vector-length vec))
               (v-rst (make-vector len)))
          (let loop ((i 0)
                     (lhs knil))
            (if (= i len)
                v-rst
                (let ((cumu-i (fn lhs (vec i))))
                  (begin
                    (vector-set! v-rst i cumu-i)
                    (loop (+ 1 i) cumu-i)
                  ) ;begin
                ) ;let
            ) ;if
          ) ;let
        ) ;let*
      ) ;typed-lambda
    ) ;define
    ; TODO optional parameters
    (define (vector-any pred v)
      (let loop ((i 0))
        (cond ((= i (vector-length v)) #f)
              ((pred (vector-ref v i)) #t)
              (else (loop (+ i 1)))
        ) ;cond
      ) ;let
    ) ;define

    ; TODO optional parameters
    (define (vector-every pred v)
      (let loop ((i 0))
        (cond ((= i (vector-length v)) #t)
              ((not (pred (vector-ref v i))) #f)
              (else (loop (+ i 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define vector-index
      (typed-lambda ((pred procedure?) (v vector?))
        (let loop ((i 0))
          (cond ((= i (vector-length v)) #f)
                ((pred (vector-ref v i)) i)
                (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;typed-lambda
    ) ;define

    (define vector-index-right
      (typed-lambda ((pred procedure?) (v vector?))
        (let ((len (vector-length v)))
          (let loop ((i (- len 1)))
            (cond ((< i 0) #f)
                  ((pred (vector-ref v i)) i)
                  (else (loop (- i 1)))
            ) ;cond
          ) ;let
        ) ;let
      ) ;typed-lambda
    ) ;define

    (define (vector-skip pred v)
      (vector-index (lambda (x) (not (pred x))) v)
    ) ;define

    (define (vector-skip-right pred v)
      (vector-index-right (lambda (x) (not (pred x))) v)
    ) ;define

    (define (vector-partition pred v)
      (let* ((len (vector-length v))
             (cnt (vector-count pred v))
             (ret (make-vector len)))
        (let loop ((i 0)
                   (yes 0)
                   (no cnt))
          (if (= i len)
              (values ret cnt)
              (let ((elem (vector-ref v i)))
                (if (pred elem)
                    (begin
                      (vector-set! ret yes elem)
                      (loop (+ i 1) (+ yes 1) no)
                    ) ;begin
                    (begin
                      (vector-set! ret no elem)
                      (loop (+ i 1) yes (+ no 1))
                    ) ;begin
                ) ;if
              ) ;let
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

    (define (vector-swap! vec i j)
      (let ((elem-i (vector-ref vec i))
            (elem-j (vector-ref vec j)))
        (vector-set! vec i elem-j)
        (vector-set! vec j elem-i)
      ) ;let
    ) ;define

    (define (vector-reverse! vec . args)
      (let* ((args-length (length args))
             (start (if (null? args) 0 (car args)))
             (end (if (<= args-length 1)
                      (vector-length vec)
                      (cadr args)))
             ) ;

        (unless (and (< args-length 3)
                     (>= args-length 0))
          (error 'wrong-number-of-args "#<vector-reverse!>: too many args" args-length)
        ) ;unless
        (unless (and (integer? start) (integer? end))
          (error 'type-error "#<vector-reverse!>: *start* and *end* must be an integer" start end)
        ) ;unless
        (when (< start 0)
          (error 'out-of-range "#<vector-reverse!>: *start* cannot be negative" start)
        ) ;when
        (when (> end (vector-length vec))
          (error 'out-of-range "#<vector-reverse!>: *end* exceeds vector length" end)
        ) ;when
        (when (> start end)
          (error 'out-of-range
                 "#<vector-reverse!>: *start* must be less than or equal to *end*"
                 start end
          ) ;error
        ) ;when
        (let loop ((i start)
                   (j (- end 1)))
          (when (< i j)
            (vector-swap! vec i j)
            (loop (+ i 1) (- j 1))
          ) ;when
        ) ;let
      ) ;let*
    ) ;define

    ; Input a proper-list, return a vector with inversed order elements.
    (define reverse-list->vector
      (typed-lambda ((lst proper-list?))
        (let* ((len (length lst))
               (v-rst (make-vector len)))
          (let loop ((l lst)
                     (i (- len 1)))
            (if (null? l)
                v-rst
                (begin
                  (vector-set! v-rst i (car l))
                  (loop (cdr l) (- i 1))
                ) ;begin
            ) ;if
          ) ;let
        ) ;let*
      ) ;typed-lambda
    ) ;define
  ) ;begin
) ;define-library

