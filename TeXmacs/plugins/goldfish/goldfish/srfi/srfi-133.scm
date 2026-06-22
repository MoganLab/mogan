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

(define-library (srfi srfi-133)
  (import (liii base))
  (export vector-empty?
    vector-unfold
    vector-unfold-right
    vector-unfold!
    vector-unfold-right!
    vector-fold
    vector-fold-right
    vector-count
    vector-any
    vector-every
    vector-copy
    vector-copy!
    vector-index
    vector-index-right
    vector-skip
    vector-skip-right
    vector-binary-search
    vector-concatenate
    vector-partition
    vector-append-subvectors
    vector-swap!
    vector-reverse!
    vector-reverse-copy
    vector-reverse-copy!
    vector-map!
    vector-cumulate
    reverse-vector->list
    reverse-list->vector
    vector=
  ) ;export
  (begin

    (define (vector-empty? v)
      (when (not (vector? v))
        (error 'type-error "v is not a vector")
      ) ;when
      (zero? (vector-length v))
    ) ;define

    (define (vector-unfold f length . seeds)
      (let ((vec (make-vector length)))
        (let loop
          ((k 0) (seeds seeds))
          (if (= k length)
            vec
            (let-values (((elem . new-seeds) (apply f k seeds)))
              (vector-set! vec k elem)
              (loop (+ k 1) new-seeds)
            ) ;let-values
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (vector-unfold-right f length . seeds)
      (let ((vec (make-vector length)))
        (let loop
          ((k (- length 1)) (seeds seeds))
          (if (< k 0)
            vec
            (let-values (((elem . new-seeds) (apply f k seeds)))
              (vector-set! vec k elem)
              (loop (- k 1) new-seeds)
            ) ;let-values
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (vector-unfold! f vec start end . seeds)
      (let ((count (- end start)))
        (let loop
          ((k 0) (seeds seeds))
          (if (= k count)
            (if #f #f)
            (let-values (((elem . new-seeds) (apply f k seeds)))
              (vector-set! vec (+ start k) elem)
              (loop (+ k 1) new-seeds)
            ) ;let-values
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (vector-unfold-right! f vec start end . seeds)
      (let ((count (- end start)))
        (let loop
          ((k 0) (seeds seeds))
          (if (= k count)
            (if #f #f)
            (let-values (((elem . new-seeds) (apply f k seeds)))
              (vector-set! vec (- end 1 k) elem)
              (loop (+ k 1) new-seeds)
            ) ;let-values
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (vector= elt=? . rest)
      (define compare2vecs
        (typed-lambda ((cmp procedure?) (vec1 vector?) (vec2 vector?))
          (let* ((len1 (vector-length vec1)) (len2 (vector-length vec2)))
            (if (not (= len1 len2))
              #f
              (let loop
                ((ilhs 0) (irhs 0) (len len1))
                (if (= ilhs len)
                  #t
                  (if (not (cmp (vec1 ilhs) (vec2 irhs))) #f (loop (+ 1 ilhs) (+ 1 irhs) len))
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
        (let loop
          ((vec1 (car rest)) (vec2 (car (cdr rest))) (vrest (cdr (cdr rest))))
          (let ((rst (compare2vecs elt=? vec1 vec2)))
            (when (not (boolean? rst))
              (error 'type-error "elt=> should return bool")
            ) ;when
            (if (compare2vecs elt=? vec1 vec2)
              (if (null? vrest) #t (loop vec2 (car vrest) (cdr vrest)))
              #f
            ) ;if
          ) ;let
        ) ;let
      ) ;if
    ) ;define
    (define (vector-fold f initial vec)
      (let loop
        ((i 0) (acc initial))
        (if (< i (vector-length vec)) (loop (+ i 1) (f (vector-ref vec i) acc)) acc)
      ) ;let
    ) ;define

    (define (vector-fold-right f initial vec)
      (let loop
        ((i (- (vector-length vec) 1)) (acc initial))
        (if (>= i 0) (loop (- i 1) (f (vector-ref vec i) acc)) acc)
      ) ;let
    ) ;define

    (define (vector-count pred v)
      (let loop
        ((i 0) (count 0))
        (cond ((= i (vector-length v)) count)
              ((pred (vector-ref v i)) (loop (+ i 1) (+ count 1)))
              (else (loop (+ i 1) count))
        ) ;cond
      ) ;let
    ) ;define

    (define vector-cumulate
      (typed-lambda ((fn procedure?) knil (vec vector?))
        (let* ((len (vector-length vec)) (v-rst (make-vector len)))
          (let loop
            ((i 0) (lhs knil))
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
    (define (vector-any pred v)
      (let loop
        ((i 0))
        (cond ((= i (vector-length v)) #f)
              ((pred (vector-ref v i)) #t)
              (else (loop (+ i 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define (vector-every pred v)
      (let loop
        ((i 0))
        (cond ((= i (vector-length v)) #t)
              ((not (pred (vector-ref v i))) #f)
              (else (loop (+ i 1)))
        ) ;cond
      ) ;let
    ) ;define

    (define vector-index
      (typed-lambda ((pred procedure?) (v vector?))
        (let loop
          ((i 0))
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
          (let loop
            ((i (- len 1)))
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

    (define (vector-concatenate ls)
      (unless (list? ls)
        (error 'type-error "vector-concatenate: argument is not a list")
      ) ;unless
      (for-each (lambda (v)
                  (unless (vector? v)
                    (error 'type-error "vector-concatenate: list element is not a vector")
                  ) ;unless
                ) ;lambda
        ls
      ) ;for-each
      (apply vector-append ls)
    ) ;define

    (define* (vector-binary-search vec value cmp (start 0) (end (vector-length vec)))
      (let lp
        ((lo start) (hi (- end 1)))
        (and (<= lo hi)
          (let* ((mid (quotient (+ lo hi) 2)) (x (vector-ref vec mid)) (y (cmp value x)))
            (cond ((< y 0) (lp lo (- mid 1)))
                  ((> y 0) (lp (+ mid 1) hi))
                  (else mid)
            ) ;cond
          ) ;let*
        ) ;and
      ) ;let
    ) ;define*

    (define (vector-partition pred v)
      (let* ((len (vector-length v)) (cnt (vector-count pred v)) (ret (make-vector len)))
        (let loop
          ((i 0) (yes 0) (no cnt))
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

    (define (vector-append-subvectors . o)
      (unless (zero? (modulo (length o) 3))
        (error 'wrong-number-of-args
          "vector-append-subvectors: arguments must be in (vec start end) triples"
        ) ;error
      ) ;unless
      (let lp
        ((ls o) (vecs '()))
        (if (null? ls)
          (apply vector-append (reverse vecs))
          (let ((vec (car ls)) (start (cadr ls)) (end (car (cddr ls))))
            (unless (vector? vec)
              (error 'type-error "vector-append-subvectors: vec must be a vector" vec)
            ) ;unless
            (unless (integer? start)
              (error 'type-error "vector-append-subvectors: start must be integer" start)
            ) ;unless
            (unless (integer? end)
              (error 'type-error "vector-append-subvectors: end must be integer" end)
            ) ;unless
            (let ((len (vector-length vec)))
              (when (< start 0)
                (error 'value-error "vector-append-subvectors: start must be nonnegative" start)
              ) ;when
              (when (< end 0)
                (error 'value-error "vector-append-subvectors: end must be nonnegative" end)
              ) ;when
              (when (> start end)
                (error 'value-error "vector-append-subvectors: start > end" start end)
              ) ;when
              (when (> end len)
                (error 'value-error "vector-append-subvectors: end out of range" end len)
              ) ;when
            ) ;let
            (lp (cdr (cddr ls)) (cons (vector-copy vec start end) vecs))
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (vector-swap! vec i j)
      (let ((elem-i (vector-ref vec i)) (elem-j (vector-ref vec j)))
        (vector-set! vec i elem-j)
        (vector-set! vec j elem-i)
      ) ;let
    ) ;define

    (define (vector-reverse! vec . args)
      (let* ((args-length (length args))
             (start (if (null? args) 0 (car args)))
             (end (if (<= args-length 1) (vector-length vec) (cadr args)))
            ) ;

        (unless (and (< args-length 3) (>= args-length 0))
          (error 'wrong-number-of-args "#<vector-reverse!>: too many args" args-length)
        ) ;unless
        (unless (and (integer? start) (integer? end))
          (error 'type-error
            "#<vector-reverse!>: *start* and *end* must be an integer"
            start
            end
          ) ;error
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
            start
            end
          ) ;error
        ) ;when
        (let loop
          ((i start) (j (- end 1)))
          (when (< i j)
            (vector-swap! vec i j)
            (loop (+ i 1) (- j 1))
          ) ;when
        ) ;let
      ) ;let*
    ) ;define

    (define* (vector-reverse-copy vec (start 0) (end (vector-length vec)))
      (let ((v (vector-copy vec start end)))
        (vector-reverse! v)
        v
      ) ;let
    ) ;define*

    (define* (vector-reverse-copy! to at from (start 0) (end (vector-length from)))
      (let* ((temp (vector-copy from start end)) (len (vector-length temp)))
        (let loop
          ((i 0))
          (when (< i len)
            (vector-set! to (+ at (- len i 1)) (vector-ref temp i))
            (loop (+ i 1))
          ) ;when
        ) ;let
      ) ;let*
    ) ;define*

    (define (vector-map! proc vec . rest)
      (let ((len (if (null? rest)
                   (vector-length vec)
                   (apply min (vector-length vec) (map vector-length rest))
                 ) ;if
            ) ;len
           ) ;
        (let loop
          ((i 0))
          (if (= i len)
            vec
            (begin
              (vector-set! vec
                i
                (apply proc (vector-ref vec i) (map (lambda (v) (vector-ref v i)) rest))
              ) ;vector-set!
              (loop (+ i 1))
            ) ;begin
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define* (reverse-vector->list vec (start 0) (end (vector-length vec)))
      (let loop
        ((i start) (acc '()))
        (if (= i end) acc (loop (+ i 1) (cons (vector-ref vec i) acc)))
      ) ;let
    ) ;define*

    (define reverse-list->vector
      (typed-lambda ((lst proper-list?))
        (let* ((len (length lst)) (v-rst (make-vector len)))
          (let loop
            ((l lst) (i (- len 1)))
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
