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

(define-library (srfi srfi-132)
  (export list-sorted? vector-sorted?
          list-merge  list-sort  list-stable-sort  vector-merge  vector-sort  vector-stable-sort
          list-merge! list-sort! list-stable-sort! vector-merge! vector-sort! vector-stable-sort!
  ) ;export
  (import (liii list)
          (liii error)
          (scheme case-lambda)
  ) ;import
  (begin


    (define (list-sorted? less-p lis)
      (if (null? lis)
        #t
        (do ((first lis (cdr first))
             (second (cdr lis) (cdr second))
             (res #t (not (less-p (car second) (car first)))))
          ((or (null? second) (not res)) res)
        ) ;do
      ) ;if
    ) ;define

    (define vector-sorted?
      (case-lambda
        ((less-p v) (vector-sorted? less-p v 0 (vector-length v)))
        ((less-p v start) (vector-sorted? less-p v start (vector-length v)))
        ((less-p v start end)
         (if (or (< start 0) (> end (vector-length v)) (> start end))
           (raise "Invalid start or end parameters") ; 使用 raise 抛出错误
           (let loop ((i start))
             (if (>= i (- end 1))
               #t
               (if (less-p (vector-ref v (+ i 1)) (vector-ref v i))
                 #f
                 (loop (+ i 1))
               ) ;if
             ) ;if
           ) ;let
         ) ;if
        ) ;
      ) ;case-lambda
    ) ;define

    (define (list-merge less-p lis1 lis2)
      (let loop
        ((res '())
         (lis1 lis1)
         (lis2 lis2)
        ) ;
        (cond
          ((and (null? lis1) (null? lis2)) (reverse res))
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
          (let loop ((left left) (right right) (prev prev))
            (cond
              ((null? left) (set-cdr! prev right))
              ((null? right) (set-cdr! prev left))
              ((less-p (car left) (car right))
               (set-cdr! prev left)
               (loop (cdr left) right left)
              ) ;
              (else
                (set-cdr! prev right)
                (loop left (cdr right) right)
              ) ;else
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
        (cond
          ((= l r) '())
          ((= (+ l 1) r) (list (list-ref lis l)))
          (else
            (let* ((mid (quotient (+ l r) 2))
                   (l-sorted (sort l mid))
                   (r-sorted (sort mid r)))
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
          (let ((pivot (car lis))
                (rest (cdr lis)))
            (let ((smaller (filter (lambda (x) (less-p x pivot)) rest))
                  (larger (filter (lambda (x) (not (less-p x pivot))) rest)))
              (append (list-sort less-p smaller)
                      (list pivot)
                      (list-sort less-p larger)
              ) ;append
            ) ;let
          ) ;let
      ) ;if
    ) ;define

    (define (list-sort! less-p lst)
      ;; 辅助函数：将列表分成小于和大于 pivot 的部分
      (define (partition! lst pivot less-p)
        (let loop ((lst lst) (less '()) (greater '()))
          (cond
            ((null? lst) (values (reverse less) (reverse greater)))  ;; 返回小于和大于部分
            ((less-p (car lst) pivot)
             (loop (cdr lst) (cons (car lst) less) greater)
            ) ;
            (else
              (loop (cdr lst) less (cons (car lst) greater))
            ) ;else
          ) ;cond
        ) ;let
      ) ;define
      ;; 排序函数：原地排序
      (if (or (null? lst) (null? (cdr lst)))  ;; 如果列表为空或只有一个元素，已经排序好
          lst
          (let* ((pivot (car lst)))
            (call-with-values 
              (lambda () (partition! (cdr lst) pivot less-p))  ;; 调用 partition 并返回小于和大于部分
              (lambda (less greater)
                ;; 对小于和大于部分递归排序
                (let ((sorted-less (list-sort! less-p less))
                      (sorted-greater (list-sort! less-p greater)))
                  ;; 如果 sorted-less 是空，直接返回 sorted-greater
                  (if (null? sorted-less)
                      sorted-greater
                      (begin
                        ;; 原地连接两个部分和 pivot
                        (set-cdr! (last-pair sorted-less) (cons pivot sorted-greater))
                        sorted-less  ;; 返回排序后的列表
                      ) ;begin
                  ) ;if
                ) ;let
              ) ;lambda
            ) ;call-with-values
          ) ;let*
      ) ;if
    ) ;define

    (define list-stable-sort!
      (lambda (less-p lis)
        (define (split! lis)
          (let loop ((slow lis) (fast (cdr lis)))
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
        ((less-p v)
         (list->vector (list-stable-sort less-p (vector->list v)))
        ) ;
        ((less-p v start)
         (list->vector (list-stable-sort less-p (subvector->list v start (vector-length v))))
        ) ;
        ((less-p v start end)
         (list->vector (list-stable-sort less-p (subvector->list v start end)))
        ) ;
      ) ;case-lambda
    ) ;define

    (define vector-sort vector-stable-sort)

    (define (vector-sort! . r) (???))

    (define (vector-stable-sort! . r) (???))

    (define (subvector->list v start end)
      (do ((r '() (cons (vector-ref v p) r))
           (p start (+ 1 p)))
        ((>= p end) (reverse r))
      ) ;do
    ) ;define

    (define vector-merge
      (case-lambda
        ((less-p v1 v2)
         (list->vector (list-merge less-p (vector->list v1) (vector->list v2)))
        ) ;
        ((less-p v1 v2 start1)
         (list->vector (list-merge less-p (subvector->list v1 start1 (vector-length v1)) (vector->list v2)))
        ) ;
        ((less-p v1 v2 start1 end1)
         (list->vector (list-merge less-p (subvector->list v1 start1 end1) (vector->list v2)))
        ) ;
        ((less-p v1 v2 start1 end1 start2)
         (list->vector (list-merge less-p (subvector->list v1 start1 end1) (subvector->list v2 start2 (vector-length v2))))
        ) ;
        ((less-p v1 v2 start1 end1 start2 end2)
         (list->vector (list-merge less-p (subvector->list v1 start1 end1) (subvector->list v2 start2 end2)))
        ) ;
      ) ;case-lambda
    ) ;define

    (define (vector-merge! . r) (???))


  ) ;begin
) ;define-library

