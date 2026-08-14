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

(define-library (srfi srfi-132)
  (export list-sorted? vector-sorted? list-merge list-sort list-stable-sort
    vector-merge vector-sort vector-stable-sort list-merge! list-sort!
    list-stable-sort! vector-merge! vector-sort! vector-stable-sort!
  ) ;export
  (import (liii list) (liii error) (scheme case-lambda))
  (begin

    ;; list-sorted? 复用 liii_sort.cpp 的 C++ 实现 g_list-sorted?
    (define (list-sorted? less-p lis)
      (g_list-sorted? less-p lis)
    ) ;define

    (define vector-sorted?
      (case-lambda
       ((less-p v) (vector-sorted? less-p v 0 (vector-length v)))
       ((less-p v start) (vector-sorted? less-p v start (vector-length v)))
       ((less-p v start end)
        (if (or (< start 0) (> end (vector-length v)) (> start end))
          (raise "Invalid start or end parameters")
          (let loop
            ((i start))
            (if (>= i (- end 1))
              #t
              (if (less-p (vector-ref v (+ i 1)) (vector-ref v i)) #f (loop (+ i 1)))
            ) ;if
          ) ;let
        ) ;if
       ) ;
      ) ;case-lambda
    ) ;define

    (define (list-merge less-p lis1 lis2)
      (let loop
        ((res '()) (lis1 lis1) (lis2 lis2))
        (cond ((and (null? lis1) (null? lis2)) (reverse res))
              ((null? lis1) (loop (cons (car lis2) res) lis1 (cdr lis2)))
              ((null? lis2) (loop (cons (car lis1) res) lis2 (cdr lis1)))
              ((less-p (car lis2) (car lis1)) (loop (cons (car lis2) res) lis1 (cdr lis2)))
              (else (loop (cons (car lis1) res) (cdr lis1) lis2))
        ) ;cond
      ) ;let
    ) ;define

    (define list-merge!
      (lambda (less-p lis1 lis2)
        (define (merge! left right prev)
          (let loop
            ((left left) (right right) (prev prev))
            (cond ((null? left) (set-cdr! prev right))
                  ((null? right) (set-cdr! prev left))
                  ((less-p (car left) (car right))
                   (set-cdr! prev left)
                   (loop (cdr left) right left)
                  ) ;
                  (else (set-cdr! prev right) (loop left (cdr right) right))
            ) ;cond
          ) ;let
        ) ;define
        (let ((dummy (cons '() '())))
          (merge! lis1 lis2 dummy)
          (cdr dummy)
        ) ;let
      ) ;lambda
    ) ;define

    (define (list-stable-sort less-p lis)
      (define (sort l r)
        (cond ((= l r) '())
              ((= (+ l 1) r) (list (list-ref lis l)))
              (else (let* ((mid (quotient (+ l r) 2)) (l-sorted (sort l mid)) (r-sorted (sort mid r)))
                      (list-merge less-p l-sorted r-sorted)
                    ) ;let*
              ) ;else
        ) ;cond
      ) ;define
      (sort 0 (length lis))
    ) ;define

    (define (list-sort less-p lis)
      (if (or (null? lis) (null? (cdr lis)))
        lis
        (let ((pivot (car lis)) (rest (cdr lis)))
          (let ((smaller (filter (lambda (x) (less-p x pivot)) rest))
                (larger (filter (lambda (x) (not (less-p x pivot))) rest))
               ) ;
            (append (list-sort less-p smaller) (list pivot) (list-sort less-p larger))
          ) ;let
        ) ;let
      ) ;if
    ) ;define

    ;; list-sort! 复用 S7 内置的 sort!：基于 C qsort 的原地排序，
    ;; 只重写各 pair 的 car，列表骨架不变，返回值与输入 eq?。
    ;; 注意参数顺序相反：list-sort! 是 (list-sort! less-p lis)，
    ;; 内置 sort! 是 (sort! seq less?)。
    (define (list-sort! less-p lis)
      (sort! lis less-p)
    ) ;define

    (define list-stable-sort!
      (lambda (less-p lis)
        (define (split! lis)
          (let loop
            ((slow lis) (fast (cdr lis)))
            (if (or (null? fast) (null? (cdr fast)))
              (let ((mid (cdr slow)))
                (set-cdr! slow '())
                (values lis mid)
              ) ;let
              (loop (cdr slow) (cddr fast))
            ) ;if
          ) ;let
        ) ;define
        (if (or (null? lis) (null? (cdr lis)))
          lis
          (let-values (((left right) (split! lis)))
            (list-merge! less-p
              (list-stable-sort! less-p left)
              (list-stable-sort! less-p right)
            ) ;list-merge!
          ) ;let-values
        ) ;if
      ) ;lambda
    ) ;define

    (define vector-stable-sort
      (case-lambda
       ((less-p v) (list->vector (list-stable-sort less-p (vector->list v))))
       ((less-p v start)
        (list->vector (list-stable-sort less-p (subvector->list v start (vector-length v)))
        ) ;list->vector
       ) ;
       ((less-p v start end)
        (list->vector (list-stable-sort less-p (subvector->list v start end)))
       ) ;
      ) ;case-lambda
    ) ;define

    (define vector-sort vector-stable-sort)

    ;; vector-sort! 复用 S7 内置的 sort!：基于 C qsort 的原地排序，
    ;; 直接重写向量元素，返回值与输入 eq?。
    ;; 注意参数顺序相反：vector-sort! 是 (vector-sort! less-p v)，
    ;; 内置 sort! 是 (sort! seq less?)。
    (define vector-sort!
      (case-lambda
       ((less-p v)
        (if (vector? v)
          (sort! v less-p)
          (type-error "vector-sort!: expected a vector" v)
        ) ;if
       ) ;
       ((less-p v start)
        (if (vector? v)
          (vector-sort! less-p v start (vector-length v))
          (type-error "vector-sort!: expected a vector" v)
        ) ;if
       ) ;
       ((less-p v start end)
        (cond ((not (vector? v)) (type-error "vector-sort!: expected a vector" v))
              ((or (< start 0) (> end (vector-length v)) (> start end))
               (value-error "Invalid start or end parameters")
              ) ;
              (else
                ;; S7 subvector 与原向量共享存储，
                ;; 对子区间 sort! 即原地排序原向量的对应区间
                (sort! (subvector v start end) less-p)
                v
              ) ;else
        ) ;cond
       ) ;
      ) ;case-lambda
    ) ;define

    (define (vector-stable-sort! . r)
      (???)
    ) ;define

    (define (subvector->list v start end)
      (do ((r '() (cons (vector-ref v p) r)) (p start (+ 1 p)))
        ((>= p end) (reverse r))
      ) ;do
    ) ;define

    (define vector-merge
      (case-lambda
       ((less-p v1 v2)
        (list->vector (list-merge less-p (vector->list v1) (vector->list v2)))
       ) ;
       ((less-p v1 v2 start1)
        (list->vector (list-merge less-p
                        (subvector->list v1 start1 (vector-length v1))
                        (vector->list v2)
                      ) ;list-merge
        ) ;list->vector
       ) ;
       ((less-p v1 v2 start1 end1)
        (list->vector (list-merge less-p (subvector->list v1 start1 end1) (vector->list v2))
        ) ;list->vector
       ) ;
       ((less-p v1 v2 start1 end1 start2)
        (list->vector (list-merge less-p
                        (subvector->list v1 start1 end1)
                        (subvector->list v2 start2 (vector-length v2))
                      ) ;list-merge
        ) ;list->vector
       ) ;
       ((less-p v1 v2 start1 end1 start2 end2)
        (list->vector (list-merge less-p
                        (subvector->list v1 start1 end1)
                        (subvector->list v2 start2 end2)
                      ) ;list-merge
        ) ;list->vector
       ) ;
      ) ;case-lambda
    ) ;define

    (define (vector-merge! . r)
      (???)
    ) ;define

  ) ;begin
) ;define-library
