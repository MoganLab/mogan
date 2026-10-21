;;
;; Copyright (C) 2024 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;

(define-library (liii diff matcher)
  (export get-matching-blocks get-opcodes get-grouped-opcodes)
  (import (scheme base) (liii base) (liii hash-table) (liii sort))
  (begin

    (define (build-b2j b)
      (let ((ht (make-hash-table)) (lb (vector-length b)))
        (let loop
          ((j (- lb 1)))
          (if (>= j 0)
            (let* ((elem (vector-ref b j)) (cur (hash-table-ref/default ht elem '())))
              (hash-table-set! ht elem (cons j cur))
              (loop (- j 1))
            ) ;let*
            ht
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (update-best k i j best-box)
      (when (> k (vector-ref best-box 2))
        (vector-set! best-box 0 (+ (- i k) 1))
        (vector-set! best-box 1 (+ (- j k) 1))
        (vector-set! best-box 2 k)
      ) ;when
    ) ;define

    ;; 每轮 i 重建 j2len 哈希表（对齐 difflib 的 newj2len 语义），
    ;; 用 O(1) 查表替代 alist 线性 assoc
    (define (scan-indices inds blo bhi i j2len best-box)
      (let ((new-j2len (make-hash-table)))
        (let loop
          ((rem inds))
          (cond
           ((or (null? rem) (>= (car rem) bhi)) new-j2len)
           ((< (car rem) blo) (loop (cdr rem)))
           (else
             (let* ((j (car rem)) (k (+ (hash-table-ref/default j2len (- j 1) 0) 1)))
               (update-best k i j best-box)
               (hash-table-set! new-j2len j k)
               (loop (cdr rem))
             ) ;let*
           ) ;else
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (find-longest-match a b b2j alo ahi blo bhi)
      (let ((best-box (vector alo blo 0)))
        (let loop
          ((i alo) (j2len (make-hash-table)))
          (if (< i ahi)
            (let* ((elem (vector-ref a i))
                   (inds (hash-table-ref/default b2j elem '()))
                   (new-j2len (scan-indices inds blo bhi i j2len best-box))
                  ) ;
              (loop (+ i 1) new-j2len)
            ) ;let*
            (vector->list best-box)
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (block<? x y)
      (let ((xi (list-ref x 0)) (yi (list-ref y 0)))
        (if (= xi yi) (< (list-ref x 1) (list-ref y 1)) (< xi yi))
      ) ;let
    ) ;define

    (define (collapse-matching-blocks blocks)
      (if (null? blocks)
        '()
        (let loop
          ((rem (cdr blocks)) (cur (car blocks)) (acc '()))
          (let ((i1 (list-ref cur 0)) (j1 (list-ref cur 1)) (k1 (list-ref cur 2)))
            (if (null? rem)
              (reverse (cons cur acc))
              (let* ((nxt (car rem))
                     (i2 (list-ref nxt 0))
                     (j2 (list-ref nxt 1))
                     (k2 (list-ref nxt 2))
                    ) ;
                (if (and (= (+ i1 k1) i2) (= (+ j1 k1) j2))
                  (loop (cdr rem) (list i1 j1 (+ k1 k2)) acc)
                  (loop (cdr rem) nxt (cons cur acc))
                ) ;if
              ) ;let*
            ) ;if
          ) ;let
        ) ;let
      ) ;if
    ) ;define

    (define (get-matching-blocks a b)
      (let* ((la (vector-length a))
             (lb (vector-length b))
             (b2j (build-b2j b))
             (queue (list (list 0 la 0 lb)))
             (matching-blocks '())
            ) ;
        (let loop
          ()
          (unless (null? queue)
            (let* ((entry (car queue))
                   (alo (list-ref entry 0))
                   (ahi (list-ref entry 1))
                   (blo (list-ref entry 2))
                   (bhi (list-ref entry 3))
                  ) ;
              (set! queue (cdr queue))
              (let* ((match (find-longest-match a b b2j alo ahi blo bhi))
                     (i (list-ref match 0))
                     (j (list-ref match 1))
                     (k (list-ref match 2))
                    ) ;
                (when (> k 0)
                  (set! matching-blocks (cons match matching-blocks))
                  (when (and (< alo i) (< blo j))
                    (set! queue (cons (list alo i blo j) queue))
                  ) ;when
                  (when (and (< (+ i k) ahi) (< (+ j k) bhi))
                    (set! queue (cons (list (+ i k) ahi (+ j k) bhi) queue))
                  ) ;when
                ) ;when
              ) ;let*
              (loop)
            ) ;let*
          ) ;unless
        ) ;let
        (let* ((sorted (list-sort block<? matching-blocks))
               (collapsed (collapse-matching-blocks sorted))
              ) ;
          (append collapsed (list (list la lb 0)))
        ) ;let*
      ) ;let*
    ) ;define

    (define (get-opcodes matching-blocks)
      (let loop
        ((blocks matching-blocks) (i 0) (j 0) (acc '()))
        (if (null? blocks)
          (reverse acc)
          (let* ((block (car blocks))
                 (ai (list-ref block 0))
                 (bj (list-ref block 1))
                 (size (list-ref block 2))
                 (acc1
                   (cond ((and (< i ai) (< j bj)) (cons (list 'replace i ai j bj) acc))
                         ((< i ai) (cons (list 'delete i ai j bj) acc))
                         ((< j bj) (cons (list 'insert i ai j bj) acc))
                         (else acc)
                   ) ;cond
                 ) ;acc1
                 (next-i (+ ai size))
                 (next-j (+ bj size))
                 (acc2 (if (> size 0) (cons (list 'equal ai next-i bj next-j) acc1) acc1))
                ) ;
            (loop (cdr blocks) next-i next-j acc2)
          ) ;let*
        ) ;if
      ) ;let
    ) ;define

    (define (fixup-leading-code code n)
      (if (eq? (car code) 'equal)
        (let ((i1 (list-ref code 1))
              (i2 (list-ref code 2))
              (j1 (list-ref code 3))
              (j2 (list-ref code 4))
             ) ;
          (list 'equal (max i1 (- i2 n)) i2 (max j1 (- j2 n)) j2)
        ) ;let
        code
      ) ;if
    ) ;define

    (define (fixup-trailing-code code n)
      (if (eq? (car code) 'equal)
        (let ((i1 (list-ref code 1))
              (i2 (list-ref code 2))
              (j1 (list-ref code 3))
              (j2 (list-ref code 4))
             ) ;
          (list 'equal i1 (min i2 (+ i1 n)) j1 (min j2 (+ j1 n)))
        ) ;let
        code
      ) ;if
    ) ;define

    (define (prepare-fixed-codes codes n)
      (let* ((codes (if (null? codes) (list (list 'equal 0 1 0 1)) codes))
             (first-fixed (fixup-leading-code (car codes) n))
             (rest-c (cdr codes))
            ) ;
        (if (null? rest-c)
          (list (fixup-trailing-code first-fixed n))
          (let* ((rev (reverse rest-c)) (last-fixed (fixup-trailing-code (car rev) n)))
            (cons first-fixed (append (reverse (cdr rev)) (list last-fixed)))
          ) ;let*
        ) ;if
      ) ;let*
    ) ;define

    (define (split-equal-group code n group groups)
      (let* ((tag (list-ref code 0))
             (i1 (list-ref code 1))
             (i2 (list-ref code 2))
             (j1 (list-ref code 3))
             (j2 (list-ref code 4))
             (sub (list tag i1 (min i2 (+ i1 n)) j1 (min j2 (+ j1 n))))
             (new-grp (reverse (cons sub group)))
             (tail (list tag (max i1 (- i2 n)) i2 (max j1 (- j2 n)) j2))
            ) ;
        (values (list tail) (cons new-grp groups))
      ) ;let*
    ) ;define

    (define (finalize-groups group groups)
      (if
        (and (not (null? group))
          (not
            (and (= (length group) 1) (eq? (car (car group)) 'equal))
          ) ;not
        ) ;and
        (reverse (cons (reverse group) groups))
        (reverse groups)
      ) ;if
    ) ;define

    (define (get-grouped-opcodes codes n)
      (let* ((fixed-codes (prepare-fixed-codes codes n)) (nn (+ n n)))
        (let loop
          ((rem fixed-codes) (group '()) (groups '()))
          (if (null? rem)
            (finalize-groups group groups)
            (let* ((code (car rem))
                   (tag (list-ref code 0))
                   (i1 (list-ref code 1))
                   (i2 (list-ref code 2))
                  ) ;
              (if (and (eq? tag 'equal) (> (- i2 i1) nn))
                (call-with-values (lambda () (split-equal-group code n group groups))
                  (lambda (new-cur new-grps) (loop (cdr rem) new-cur new-grps))
                ) ;call-with-values
                (loop (cdr rem) (cons code group) groups)
              ) ;if
            ) ;let*
          ) ;if
        ) ;let
      ) ;let*
    ) ;define

  ) ;begin
) ;define-library
