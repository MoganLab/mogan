;;
;; Copyright (C) 2021 Wolfgang Corcoran-Mathe
;;
;; Permission is hereby granted, free of charge, to any person obtaining a
;; copy of this software and associated documentation files (the
;; "Software"), to deal in the Software without restriction, including
;; without limitation the rights to use, copy, modify, merge, publish,
;; distribute, sublicense, and/or sell copies of the Software, and to
;; permit persons to whom the Software is furnished to do so, subject to
;; the following conditions:
;;
;; The above copyright notice and this permission notice shall be included
;; in all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;; OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;; CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;; TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;; SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

(define-library (srfi srfi-224)
  (export
    ;; Constructors
    fxmapping
    alist->fxmapping
    alist->fxmapping/combinator
    fxmapping-unfold
    fxmapping-accumulate
    ;; Predicates
    fxmapping?
    fxmapping-contains?
    fxmapping-empty?
    fxmapping-disjoint?
    ;; Accessors
    fxmapping-ref
    fxmapping-min
    fxmapping-max
    ;; Updaters
    fxmapping-adjoin
    fxmapping-adjoin/combinator
    fxmapping-set
    fxmapping-adjust
    fxmapping-delete
    fxmapping-delete-all
    fxmapping-update
    fxmapping-alter
    fxmapping-delete-min
    fxmapping-update-min
    fxmapping-pop-min
    fxmapping-delete-max
    fxmapping-update-max
    fxmapping-pop-max
    ;; The whole fxmapping
    fxmapping-size
    fxmapping-find
    fxmapping-count
    fxmapping-any?
    fxmapping-every?
    ;; Mapping and folding
    fxmapping-map
    fxmapping-for-each
    fxmapping-fold
    fxmapping-fold-right
    fxmapping-map->list
    fxmapping-filter
    fxmapping-remove
    fxmapping-partition
    ;; Conversion
    fxmapping->alist
    fxmapping->decreasing-alist
    fxmapping-keys
    fxmapping-values
    ;; Comparison
    fxmapping=?
    fxmapping<?
    fxmapping>?
    fxmapping<=?
    fxmapping>=?
    ;; Set theory operations
    fxmapping-union
    fxmapping-intersection
    fxmapping-difference
    fxmapping-xor
    fxmapping-union/combinator
    fxmapping-intersection/combinator
    ;; Subsets
    fxsubmapping=
    fxmapping-open-interval
    fxmapping-closed-interval
    fxmapping-open-closed-interval
    fxmapping-closed-open-interval
    fxsubmapping<
    fxsubmapping<=
    fxsubmapping>
    fxsubmapping>=
    fxmapping-split
  ) ;export

  (import (scheme base)
    (scheme case-lambda)
    (srfi srfi-1)
    (rename (liii bitwise)
      (ash arithmetic-shift)
    ) ;rename
  ) ;import

  (begin

    ;; ;; Utility

    (define (assume condition . args)
      (if (not condition) (apply error args))
    ) ;define

    (define (plist-fold proc nil ps)
      (let loop
        ((b nil) (ps ps))
        (cond ((null? ps) b)
              ((null? (cdr ps))
               (error "plist-fold: invalid plist")
              ) ;
              (else (loop (proc (car ps) (cadr ps) b)
                      (cddr ps)
                    ) ;loop
              ) ;else
        ) ;cond
      ) ;let
    ) ;define

    (define (first-arg _k x _y)
      x
    ) ;define
    (define (second-arg _k _x y)
      y
    ) ;define

    ;; ;; Trie implementation (Patricia trie for integer keys)

    (define the-empty-trie #f)
    (define (trie-empty? t)
      (not t)
    ) ;define

    (define-record-type <leaf>
      (leaf key value)
      leaf?
      (key leaf-key)
      (value leaf-value)
    ) ;define-record-type

    (define-record-type <branch>
      (raw-branch prefix
        branching-bit
        left
        right
      ) ;raw-branch
      branch?
      (prefix branch-prefix)
      (branching-bit branch-branching-bit)
      (left branch-left)
      (right branch-right)
    ) ;define-record-type

    (define (valid-integer? x)
      (integer? x)
    ) ;define

    (define fx-least -9223372036854775808)

    (define (mask k m)
      (if (= m fx-least)
        0
        (logand k (logxor (lognot (- m 1)) m))
      ) ;if
    ) ;define

    (define (match-prefix? k p m)
      (= (mask k m) p)
    ) ;define

    (define (lowest-set-bit b)
      (logand b (- b))
    ) ;define

    (define (highest-bit-mask k guess-m)
      (let lp
        ((x (logand k (lognot (- guess-m 1)))))
        (let ((m (lowest-set-bit x)))
          (if (= x m) m (lp (- x m)))
        ) ;let
      ) ;let
    ) ;define

    (define (branching-bit p1 m1 p2 m2)
      (if (negative? (logxor p1 p2))
        fx-least
        (highest-bit-mask (logxor p1 p2)
          (max 1 (* 2 (max m1 m2)))
        ) ;highest-bit-mask
      ) ;if
    ) ;define

    (define (zero-bit? k m)
      (zero? (logand k m))
    ) ;define

    (define (branch prefix mask trie1 trie2)
      (cond ((not trie1) trie2)
            ((not trie2) trie1)
            (else (raw-branch prefix mask trie1 trie2)
            ) ;else
      ) ;cond
    ) ;define

    (define (trie-join prefix1
              mask1
              trie1
              prefix2
              mask2
              trie2
            ) ;trie-join
      (let ((m (branching-bit prefix1
                 mask1
                 prefix2
                 mask2
               ) ;branching-bit
            ) ;m
           ) ;
        (if (zero-bit? prefix1 m)
          (branch (mask prefix1 m) m trie1 trie2)
          (branch (mask prefix1 m) m trie2 trie1)
        ) ;if
      ) ;let
    ) ;define

    (define (trie-insert/combine trie
              key
              value
              combine
            ) ;trie-insert/combine
      (letrec ((new-leaf (leaf key value))
               (insert (lambda (t)
                         (cond ((not t) new-leaf)
                               ((leaf? t)
                                (let ((k (leaf-key t)) (v (leaf-value t)))
                                  (if (= key k)
                                    (leaf k (combine key value v))
                                    (trie-join key 0 new-leaf k 0 t)
                                  ) ;if
                                ) ;let
                               ) ;
                               (else (let ((p (branch-prefix t))
                                           (m (branch-branching-bit t))
                                           (l (branch-left t))
                                           (r (branch-right t))
                                          ) ;
                                       (if (match-prefix? key p m)
                                         (if (zero-bit? key m)
                                           (branch p m (insert l) r)
                                           (branch p m l (insert r))
                                         ) ;if
                                         (trie-join key 0 new-leaf p m t)
                                       ) ;if
                                     ) ;let
                               ) ;else
                         ) ;cond
                       ) ;lambda
               ) ;insert
              ) ;
        (assume (valid-integer? key)
          "invalid key"
        ) ;assume
        (insert trie)
      ) ;letrec
    ) ;define

    (define (trie-insert trie key value)
      (trie-insert/combine trie
        key
        value
        (lambda (_k new _old) new)
      ) ;trie-insert/combine
    ) ;define

    (define (trie-adjoin trie key value)
      (trie-insert/combine trie
        key
        value
        (lambda (_k _new old) old)
      ) ;trie-insert/combine
    ) ;define

    (define (trie-adjust trie key proc)
      (letrec ((update (lambda (t)
                         (cond ((not t) t)
                               ((leaf? t)
                                (let ((k (leaf-key t)) (v (leaf-value t)))
                                  (if (= key k) (leaf k (proc v)) t)
                                ) ;let
                               ) ;
                               (else (let ((p (branch-prefix t))
                                           (m (branch-branching-bit t))
                                           (l (branch-left t))
                                           (r (branch-right t))
                                          ) ;
                                       (if (match-prefix? key p m)
                                         (if (zero-bit? key m)
                                           (branch p m (update l) r)
                                           (branch p m l (update r))
                                         ) ;if
                                         t
                                       ) ;if
                                     ) ;let
                               ) ;else
                         ) ;cond
                       ) ;lambda
               ) ;update
              ) ;
        (update trie)
      ) ;letrec
    ) ;define

    (define (trie-delete trie key)
      (letrec ((update (lambda (t)
                         (cond ((not t) #f)
                               ((leaf? t)
                                (if (= key (leaf-key t)) #f t)
                               ) ;
                               (else (let ((p (branch-prefix t))
                                           (m (branch-branching-bit t))
                                           (l (branch-left t))
                                           (r (branch-right t))
                                          ) ;
                                       (if (match-prefix? key p m)
                                         (if (zero-bit? key m)
                                           (branch p m (update l) r)
                                           (branch p m l (update r))
                                         ) ;if
                                         t
                                       ) ;if
                                     ) ;let
                               ) ;else
                         ) ;cond
                       ) ;lambda
               ) ;update
              ) ;
        (update trie)
      ) ;letrec
    ) ;define

    (define (trie-assoc trie key failure success)
      (letrec ((search (lambda (t)
                         (cond ((not t) (failure))
                               ((leaf? t)
                                (let ((k (leaf-key t)) (v (leaf-value t)))
                                  (if (= k key) (success v) (failure))
                                ) ;let
                               ) ;
                               (else (let ((p (branch-prefix t))
                                           (m (branch-branching-bit t))
                                           (l (branch-left t))
                                           (r (branch-right t))
                                          ) ;
                                       (if (match-prefix? key p m)
                                         (if (zero-bit? key m)
                                           (search l)
                                           (search r)
                                         ) ;if
                                         (failure)
                                       ) ;if
                                     ) ;let
                               ) ;else
                         ) ;cond
                       ) ;lambda
               ) ;search
              ) ;
        (search trie)
      ) ;letrec
    ) ;define

    (define (trie-assoc/default trie key default)
      (trie-assoc trie
        key
        (lambda () default)
        values
      ) ;trie-assoc
    ) ;define

    (define (trie-contains? trie key)
      (trie-assoc trie
        key
        (lambda () #f)
        (lambda (_) #t)
      ) ;trie-assoc
    ) ;define

    (define (trie-min trie)
      (letrec ((search (lambda (t)
                         (and t
                           (if (leaf? t)
                             (values (leaf-key t) (leaf-value t))
                             (search (branch-left t))
                           ) ;if
                         ) ;and
                       ) ;lambda
               ) ;search
              ) ;
        (if (branch? trie)
          (if (negative? (branch-branching-bit trie))
            (search (branch-right trie))
            (search (branch-left trie))
          ) ;if
          (search trie)
        ) ;if
      ) ;letrec
    ) ;define

    (define (trie-max trie)
      (letrec ((search (lambda (t)
                         (and t
                           (if (leaf? t)
                             (values (leaf-key t) (leaf-value t))
                             (search (branch-right t))
                           ) ;if
                         ) ;and
                       ) ;lambda
               ) ;search
              ) ;
        (if (branch? trie)
          (if (negative? (branch-branching-bit trie))
            (search (branch-left trie))
            (search (branch-right trie))
          ) ;if
          (search trie)
        ) ;if
      ) ;letrec
    ) ;define

    (define (trie-fold-left proc nil trie)
      (if (not trie)
        nil
        (let lp
          ((t trie) (b nil) (kont values))
          (if (leaf? t)
            (kont (proc (leaf-key t) (leaf-value t) b)
            ) ;kont
            (lp (branch-left t)
              b
              (lambda (c)
                (lp (branch-right t) c kont)
              ) ;lambda
            ) ;lp
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    (define (trie-fold-right proc nil trie)
      (if (not trie)
        nil
        (let lp
          ((t trie) (b nil) (kont values))
          (if (leaf? t)
            (kont (proc (leaf-key t) (leaf-value t) b)
            ) ;kont
            (lp (branch-right t)
              b
              (lambda (c) (lp (branch-left t) c kont))
            ) ;lp
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    (define (trie-map proc trie)
      (letrec ((tmap (lambda (t)
                       (cond ((not t) #f)
                             ((leaf? t)
                              (leaf (leaf-key t)
                                (proc (leaf-key t) (leaf-value t))
                              ) ;leaf
                             ) ;
                             (else (let ((p (branch-prefix t))
                                         (m (branch-branching-bit t))
                                         (l (branch-left t))
                                         (r (branch-right t))
                                        ) ;
                                     (branch p m (tmap l) (tmap r))
                                   ) ;let
                             ) ;else
                       ) ;cond
                     ) ;lambda
               ) ;tmap
              ) ;
        (tmap trie)
      ) ;letrec
    ) ;define

    (define (trie-filter pred trie)
      (letrec ((filter (lambda (t)
                         (cond ((not t) #f)
                               ((leaf? t)
                                (if (pred (leaf-key t) (leaf-value t))
                                  t
                                  #f
                                ) ;if
                               ) ;
                               (else (let ((p (branch-prefix t))
                                           (m (branch-branching-bit t))
                                           (l (branch-left t))
                                           (r (branch-right t))
                                          ) ;
                                       (branch p m (filter l) (filter r))
                                     ) ;let
                               ) ;else
                         ) ;cond
                       ) ;lambda
               ) ;filter
              ) ;
        (filter trie)
      ) ;letrec
    ) ;define

    (define (trie-partition pred trie)
      (letrec ((part (lambda (t)
                       (cond ((not t) (values #f #f))
                             ((leaf? t)
                              (if (pred (leaf-key t) (leaf-value t))
                                (values t #f)
                                (values #f t)
                              ) ;if
                             ) ;
                             (else (let ((p (branch-prefix t))
                                         (m (branch-branching-bit t))
                                         (l (branch-left t))
                                         (r (branch-right t))
                                        ) ;
                                     (let-values (((il ol) (part l)) ((ir or) (part r)))
                                       (values (branch p m il ir)
                                         (branch p m ol or)
                                       ) ;values
                                     ) ;let-values
                                   ) ;let
                             ) ;else
                       ) ;cond
                     ) ;lambda
               ) ;part
              ) ;
        (part trie)
      ) ;letrec
    ) ;define

    (define (trie-size trie)
      (if (not trie)
        0
        (let lp
          ((n 0) (t trie) (kont values))
          (cond ((leaf? t) (kont (+ n 1)))
                (else (lp n
                        (branch-left t)
                        (lambda (m)
                          (lp m (branch-right t) kont)
                        ) ;lambda
                      ) ;lp
                ) ;else
          ) ;cond
        ) ;let
      ) ;if
    ) ;define

    (define (trie-find pred trie failure success)
      (letrec ((search (lambda (t kont)
                         (cond ((not t) (kont))
                               ((leaf? t)
                                (if (pred (leaf-key t) (leaf-value t))
                                  (success (leaf-key t) (leaf-value t))
                                  (kont)
                                ) ;if
                               ) ;
                               (else (search (branch-left t)
                                       (lambda ()
                                         (search (branch-right t) kont)
                                       ) ;lambda
                                     ) ;search
                               ) ;else
                         ) ;cond
                       ) ;lambda
               ) ;search
              ) ;
        (search trie failure)
      ) ;letrec
    ) ;define

    (define (trie-disjoint? trie1 trie2)
      (letrec ((disjoint? (lambda (s t)
                            (or (not s)
                              (not t)
                              (cond ((and (leaf? s) (leaf? t))
                                     (not (= (leaf-key s) (leaf-key t)))
                                    ) ;
                                    ((leaf? s)
                                     (let ((k (leaf-key s)))
                                       (not (trie-contains? t k))
                                     ) ;let
                                    ) ;
                                    ((leaf? t)
                                     (let ((k (leaf-key t)))
                                       (not (trie-contains? s k))
                                     ) ;let
                                    ) ;
                                    (else (let ((p (branch-prefix s))
                                                (m (branch-branching-bit s))
                                                (sl (branch-left s))
                                                (sr (branch-right s))
                                                (q (branch-prefix t))
                                                (n (branch-branching-bit t))
                                                (tl (branch-left t))
                                                (tr (branch-right t))
                                               ) ;
                                            (cond ((and (= m n) (= p q))
                                                   (and (disjoint? sl tl)
                                                     (disjoint? sr tr)
                                                   ) ;and
                                                  ) ;
                                                  ((and (> m n) (match-prefix? q p m))
                                                   (if (zero-bit? q m)
                                                     (disjoint? sl t)
                                                     (disjoint? sr t)
                                                   ) ;if
                                                  ) ;
                                                  ((and (> n m) (match-prefix? p q n))
                                                   (if (zero-bit? p n)
                                                     (disjoint? s tl)
                                                     (disjoint? s tr)
                                                   ) ;if
                                                  ) ;
                                                  (else #t)
                                            ) ;cond
                                          ) ;let
                                    ) ;else
                              ) ;cond
                            ) ;or
                          ) ;lambda
               ) ;disjoint?
              ) ;
        (disjoint? trie1 trie2)
      ) ;letrec
    ) ;define

    (define (trie=? comp trie1 trie2)
      (cond ((and (not trie1) (not trie2)) #t)
            ((and (leaf? trie1) (leaf? trie2))
             (and (= (leaf-key trie1) (leaf-key trie2))
               (comp (leaf-value trie1)
                 (leaf-value trie2)
               ) ;comp
             ) ;and
            ) ;
            ((and (branch? trie1) (branch? trie2))
             (let ((p (branch-prefix trie1))
                   (m (branch-branching-bit trie1))
                   (l1 (branch-left trie1))
                   (r1 (branch-right trie1))
                   (q (branch-prefix trie2))
                   (n (branch-branching-bit trie2))
                   (l2 (branch-left trie2))
                   (r2 (branch-right trie2))
                  ) ;
               (and (= m n)
                 (= p q)
                 (trie=? comp l1 l2)
                 (trie=? comp r1 r2)
               ) ;and
             ) ;let
            ) ;
            (else #f)
      ) ;cond
    ) ;define

    (define (trie-proper-subset? comp trie1 trie2)
      (eqv? (trie-subset-compare comp trie1 trie2)
        'less
      ) ;eqv?
    ) ;define

    (define (trie-subset-compare comp trie1 trie2)
      (letrec ((compare (lambda (s t)
                          (cond ((not s) 'less)
                                ((not t) 'greater)
                                ((and (leaf? s) (leaf? t))
                                 (if (= (leaf-key s) (leaf-key t))
                                   (if (comp (leaf-value s) (leaf-value t))
                                     'equal
                                     'greater
                                   ) ;if
                                   'greater
                                 ) ;if
                                ) ;
                                ((leaf? s) 'less)
                                ((leaf? t) 'greater)
                                (else (compare-branches s t))
                          ) ;cond
                        ) ;lambda
               ) ;compare
               (compare-branches (lambda (s t)
                                   (let ((p (branch-prefix s))
                                         (m (branch-branching-bit s))
                                         (sl (branch-left s))
                                         (sr (branch-right s))
                                         (q (branch-prefix t))
                                         (n (branch-branching-bit t))
                                         (tl (branch-left t))
                                         (tr (branch-right t))
                                        ) ;
                                     (cond ((> m n) 'greater)
                                           ((> n m)
                                            (if (match-prefix? p q n)
                                              (let ((comp (if (zero-bit? p n)
                                                            (compare s tl)
                                                            (compare s tr)
                                                          ) ;if
                                                    ) ;comp
                                                   ) ;
                                                (if (eqv? comp 'greater) 'greater 'less)
                                              ) ;let
                                              'greater
                                            ) ;if
                                           ) ;
                                           ((= p q)
                                            (let ((cl (compare sl tl))
                                                  (cr (compare sr tr))
                                                 ) ;
                                              (cond ((or (eqv? cl 'greater)
                                                       (eqv? cr 'greater)
                                                     ) ;or
                                                     'greater
                                                    ) ;
                                                    ((and (eqv? cl 'equal) (eqv? cr 'equal))
                                                     'equal
                                                    ) ;
                                                    (else 'less)
                                              ) ;cond
                                            ) ;let
                                           ) ;
                                           (else 'greater)
                                     ) ;cond
                                   ) ;let
                                 ) ;lambda
               ) ;compare-branches
              ) ;
        (compare trie1 trie2)
      ) ;letrec
    ) ;define

    (define (trie-merge combine trie1 trie2)
      (letrec ((merge (lambda (s t)
                        (cond ((not s) t)
                              ((not t) s)
                              ((leaf? s)
                               (trie-insert/combine t
                                 (leaf-key s)
                                 (leaf-value s)
                                 (lambda (k new old) (combine k old new))
                               ) ;trie-insert/combine
                              ) ;
                              ((leaf? t)
                               (trie-insert/combine s
                                 (leaf-key t)
                                 (leaf-value t)
                                 combine
                               ) ;trie-insert/combine
                              ) ;
                              ((and (branch? s) (branch? t))
                               (merge-branches s t)
                              ) ;
                        ) ;cond
                      ) ;lambda
               ) ;merge
               (merge-branches (lambda (s t)
                                 (let ((p (branch-prefix s))
                                       (m (branch-branching-bit s))
                                       (sl (branch-left s))
                                       (sr (branch-right s))
                                       (q (branch-prefix t))
                                       (n (branch-branching-bit t))
                                       (tl (branch-left t))
                                       (tr (branch-right t))
                                      ) ;
                                   (cond ((and (= m n) (= p q))
                                          (branch p m (merge sl tl) (merge sr tr))
                                         ) ;
                                         ((and (> m n) (match-prefix? q p m))
                                          (if (zero-bit? q m)
                                            (branch p m (merge sl t) sr)
                                            (branch p m sl (merge sr t))
                                          ) ;if
                                         ) ;
                                         ((and (> n m) (match-prefix? p q n))
                                          (if (zero-bit? p n)
                                            (branch q n (merge s tl) tr)
                                            (branch q n tl (merge s tr))
                                          ) ;if
                                         ) ;
                                         (else (trie-join p m s q n t))
                                   ) ;cond
                                 ) ;let
                               ) ;lambda
               ) ;merge-branches
              ) ;
        (merge trie1 trie2)
      ) ;letrec
    ) ;define

    (define (trie-union trie1 trie2)
      (trie-merge (lambda (_k _x y) y)
        trie1
        trie2
      ) ;trie-merge
    ) ;define

    (define (trie-intersection combine trie1 trie2)
      (letrec ((intersect (lambda (s t)
                            (cond ((or (not s) (not t)) #f)
                                  ((leaf? s)
                                   (let ((k (leaf-key s)) (v (leaf-value s)))
                                     (trie-assoc t
                                       k
                                       (lambda () #f)
                                       (lambda (v2) (leaf k (combine k v v2)))
                                     ) ;trie-assoc
                                   ) ;let
                                  ) ;
                                  ((leaf? t)
                                   (let ((k (leaf-key t)) (v (leaf-value t)))
                                     (trie-assoc s
                                       k
                                       (lambda () #f)
                                       (lambda (v2) (leaf k (combine k v2 v)))
                                     ) ;trie-assoc
                                   ) ;let
                                  ) ;
                                  (else (intersect-branches s t))
                            ) ;cond
                          ) ;lambda
               ) ;intersect
               (intersect-branches (lambda (s t)
                                     (let ((p (branch-prefix s))
                                           (m (branch-branching-bit s))
                                           (sl (branch-left s))
                                           (sr (branch-right s))
                                           (q (branch-prefix t))
                                           (n (branch-branching-bit t))
                                           (tl (branch-left t))
                                           (tr (branch-right t))
                                          ) ;
                                       (cond ((> m n)
                                              (and (match-prefix? q p m)
                                                (if (zero-bit? q m)
                                                  (intersect sl t)
                                                  (intersect sr t)
                                                ) ;if
                                              ) ;and
                                             ) ;
                                             ((> n m)
                                              (and (match-prefix? p q n)
                                                (if (zero-bit? p n)
                                                  (intersect s tl)
                                                  (intersect s tr)
                                                ) ;if
                                              ) ;and
                                             ) ;
                                             ((= p q)
                                              (branch p
                                                m
                                                (intersect sl tl)
                                                (intersect sr tr)
                                              ) ;branch
                                             ) ;
                                             (else #f)
                                       ) ;cond
                                     ) ;let
                                   ) ;lambda
               ) ;intersect-branches
              ) ;
        (intersect trie1 trie2)
      ) ;letrec
    ) ;define

    (define (trie-difference trie1 trie2)
      (letrec ((difference (lambda (s t)
                             (cond ((not s) #f)
                                   ((not t) s)
                                   ((leaf? s)
                                    (let ((k (leaf-key s)))
                                      (if (trie-contains? t k) #f s)
                                    ) ;let
                                   ) ;
                                   ((leaf? t) (trie-delete s (leaf-key t)))
                                   (else (branch-difference s t))
                             ) ;cond
                           ) ;lambda
               ) ;difference
               (branch-difference (lambda (s t)
                                    (let ((p (branch-prefix s))
                                          (m (branch-branching-bit s))
                                          (sl (branch-left s))
                                          (sr (branch-right s))
                                          (q (branch-prefix t))
                                          (n (branch-branching-bit t))
                                          (tl (branch-left t))
                                          (tr (branch-right t))
                                         ) ;
                                      (cond ((and (= m n) (= p q))
                                             (branch p
                                               m
                                               (difference sl tl)
                                               (difference sr tr)
                                             ) ;branch
                                            ) ;
                                            ((and (> m n) (match-prefix? q p m))
                                             (if (zero-bit? q m)
                                               (branch p m (difference sl t) sr)
                                               (branch p m sl (difference sr t))
                                             ) ;if
                                            ) ;
                                            ((and (> n m) (match-prefix? p q n))
                                             (if (zero-bit? p n)
                                               (difference s tl)
                                               (difference s tr)
                                             ) ;if
                                            ) ;
                                            (else s)
                                      ) ;cond
                                    ) ;let
                                  ) ;lambda
               ) ;branch-difference
              ) ;
        (difference trie1 trie2)
      ) ;letrec
    ) ;define

    (define (trie-xor trie1 trie2)
      (letrec ((xor (lambda (s t)
                      (cond ((not s) t)
                            ((not t) s)
                            ((and (leaf? s) (leaf? t))
                             (let ((ks (leaf-key s)) (kt (leaf-key t)))
                               (if (= ks kt)
                                 #f
                                 (trie-join ks 0 s kt 0 t)
                               ) ;if
                             ) ;let
                            ) ;
                            ((leaf? s)
                             (let ((k (leaf-key s)) (v (leaf-value s)))
                               (if (trie-contains? t k)
                                 (trie-delete t k)
                                 (trie-insert t k v)
                               ) ;if
                             ) ;let
                            ) ;
                            ((leaf? t)
                             (let ((k (leaf-key t)) (v (leaf-value t)))
                               (if (trie-contains? s k)
                                 (trie-delete s k)
                                 (trie-insert s k v)
                               ) ;if
                             ) ;let
                            ) ;
                            (else (xor-branches s t))
                      ) ;cond
                    ) ;lambda
               ) ;xor
               (xor-branches (lambda (s t)
                               (let ((p (branch-prefix s))
                                     (m (branch-branching-bit s))
                                     (sl (branch-left s))
                                     (sr (branch-right s))
                                     (q (branch-prefix t))
                                     (n (branch-branching-bit t))
                                     (tl (branch-left t))
                                     (tr (branch-right t))
                                    ) ;
                                 (cond ((and (= m n) (= p q))
                                        (branch p m (xor sl tl) (xor sr tr))
                                       ) ;
                                       ((and (> m n) (match-prefix? q p m))
                                        (if (zero-bit? q m)
                                          (branch p m (xor sl t) sr)
                                          (branch p m sl (xor sr t))
                                        ) ;if
                                       ) ;
                                       ((and (> n m) (match-prefix? p q n))
                                        (if (zero-bit? p n)
                                          (branch q n (xor s tl) tr)
                                          (branch q n tl (xor s tr))
                                        ) ;if
                                       ) ;
                                       (else (trie-join p m s q n t))
                                 ) ;cond
                               ) ;let
                             ) ;lambda
               ) ;xor-branches
              ) ;
        (xor trie1 trie2)
      ) ;letrec
    ) ;define

    ;; ;; Fxmapping type

    (define-record-type <fxmapping>
      (raw-fxmapping trie)
      fxmapping?
      (trie fxmapping-trie)
    ) ;define-record-type

    ;; ;; Constructors

    (define (fxmapping . args)
      (raw-fxmapping (plist-fold (lambda (k v trie)
                                   (trie-adjoin trie k v)
                                 ) ;lambda
                       the-empty-trie
                       args
                     ) ;plist-fold
      ) ;raw-fxmapping
    ) ;define

    (define (pair-or-null? x)
      (or (pair? x) (null? x))
    ) ;define

    (define (alist->fxmapping/combinator comb as)
      (assume (procedure? comb))
      (assume (pair-or-null? as))
      (raw-fxmapping (fold (lambda (p trie)
                             (assume (pair? p)
                               "alist->fxmapping/combinator: not a pair"
                             ) ;assume
                             (trie-insert/combine trie
                               (car p)
                               (cdr p)
                               comb
                             ) ;trie-insert/combine
                           ) ;lambda
                       the-empty-trie
                       as
                     ) ;fold
      ) ;raw-fxmapping
    ) ;define

    (define (alist->fxmapping as)
      (alist->fxmapping/combinator second-arg
        as
      ) ;alist->fxmapping/combinator
    ) ;define

    (define fxmapping-unfold
      (case-lambda
       ((stop? mapper successor seed)
        (assume (procedure? stop?))
        (assume (procedure? mapper))
        (assume (procedure? successor))
        (let lp
          ((trie the-empty-trie) (seed seed))
          (if (stop? seed)
            (raw-fxmapping trie)
            (let-values (((k v) (mapper seed)))
              (assume (valid-integer? k))
              (lp (trie-adjoin trie k v)
                (successor seed)
              ) ;lp
            ) ;let-values
          ) ;if
        ) ;let
       ) ;
       ((stop? mapper successor . seeds)
        (assume (procedure? stop?))
        (assume (procedure? mapper))
        (assume (procedure? successor))
        (assume (pair? seeds))
        (let lp
          ((trie the-empty-trie) (seeds seeds))
          (if (apply stop? seeds)
            (raw-fxmapping trie)
            (let-values (((k v) (apply mapper seeds))
                         (seeds* (apply successor seeds))
                        ) ;
              (assume (valid-integer? k))
              (lp (trie-adjoin trie k v) seeds*)
            ) ;let-values
          ) ;if
        ) ;let
       ) ;
      ) ;case-lambda
    ) ;define

    ;; ;; Predicates

    (define (fxmapping-contains? fxmap n)
      (assume (fxmapping? fxmap))
      (assume (valid-integer? n))
      (trie-contains? (fxmapping-trie fxmap)
        n
      ) ;trie-contains?
    ) ;define

    (define (fxmapping-empty? fxmap)
      (assume (fxmapping? fxmap))
      (not (fxmapping-trie fxmap))
    ) ;define

    (define (fxmapping-disjoint? fxmap1 fxmap2)
      (assume (fxmapping? fxmap1))
      (assume (fxmapping? fxmap2))
      (trie-disjoint? (fxmapping-trie fxmap1)
        (fxmapping-trie fxmap2)
      ) ;trie-disjoint?
    ) ;define

    ;; ;; Accessors

    (define fxmapping-ref
      (case-lambda
       ((fxmap key)
        (fxmapping-ref fxmap
          key
          (lambda ()
            (error "fxmapping-ref: key not found"
              key
              fxmap
            ) ;error
          ) ;lambda
          values
        ) ;fxmapping-ref
       ) ;
       ((fxmap key failure)
        (fxmapping-ref fxmap key failure values)
       ) ;
       ((fxmap key failure success)
        (assume (fxmapping? fxmap))
        (assume (valid-integer? key))
        (assume (procedure? failure))
        (assume (procedure? success))
        (trie-assoc (fxmapping-trie fxmap)
          key
          failure
          success
        ) ;trie-assoc
       ) ;
      ) ;case-lambda
    ) ;define

    (define (fxmapping-ref/default fxmap
              key
              default
            ) ;fxmapping-ref/default
      (assume (fxmapping? fxmap))
      (assume (valid-integer? key))
      (trie-assoc/default (fxmapping-trie fxmap)
        key
        default
      ) ;trie-assoc/default
    ) ;define

    (define (fxmapping-min fxmap)
      (assume (not (fxmapping-empty? fxmap)))
      (trie-min (fxmapping-trie fxmap))
    ) ;define

    (define (fxmapping-max fxmap)
      (assume (not (fxmapping-empty? fxmap)))
      (trie-max (fxmapping-trie fxmap))
    ) ;define

    ;; ;; Updaters

    (define fxmapping-adjoin/combinator
      (case-lambda
       ((fxmap combine key value)
        (raw-fxmapping (trie-insert/combine (fxmapping-trie fxmap)
                         key
                         value
                         combine
                       ) ;trie-insert/combine
        ) ;raw-fxmapping
       ) ;
       ((fxmap combine . ps)
        (raw-fxmapping (plist-fold (lambda (k v t)
                                     (trie-insert/combine t k v combine)
                                   ) ;lambda
                         (fxmapping-trie fxmap)
                         ps
                       ) ;plist-fold
        ) ;raw-fxmapping
       ) ;
      ) ;case-lambda
    ) ;define

    (define fxmapping-adjoin
      (case-lambda
       ((fxmap key value)
        (raw-fxmapping (trie-adjoin (fxmapping-trie fxmap)
                         key
                         value
                       ) ;trie-adjoin
        ) ;raw-fxmapping
       ) ;
       ((fxmap . ps)
        (raw-fxmapping (plist-fold (lambda (k v t) (trie-adjoin t k v))
                         (fxmapping-trie fxmap)
                         ps
                       ) ;plist-fold
        ) ;raw-fxmapping
       ) ;
      ) ;case-lambda
    ) ;define

    (define fxmapping-set
      (case-lambda
       ((fxmap key value)
        (raw-fxmapping (trie-insert (fxmapping-trie fxmap)
                         key
                         value
                       ) ;trie-insert
        ) ;raw-fxmapping
       ) ;
       ((fxmap . ps)
        (raw-fxmapping (plist-fold (lambda (k v t) (trie-insert t k v))
                         (fxmapping-trie fxmap)
                         ps
                       ) ;plist-fold
        ) ;raw-fxmapping
       ) ;
      ) ;case-lambda
    ) ;define

    (define (fxmapping-adjust fxmap key proc)
      (assume (fxmapping? fxmap))
      (assume (valid-integer? key))
      (assume (procedure? proc))
      (raw-fxmapping (trie-adjust (fxmapping-trie fxmap)
                       key
                       proc
                     ) ;trie-adjust
      ) ;raw-fxmapping
    ) ;define

    (define fxmapping-delete
      (case-lambda
       ((fxmap key)
        (assume (fxmapping? fxmap))
        (assume (valid-integer? key))
        (raw-fxmapping (trie-delete (fxmapping-trie fxmap) key)
        ) ;raw-fxmapping
       ) ;
       ((fxmap . keys)
        (fxmapping-delete-all fxmap keys)
       ) ;
      ) ;case-lambda
    ) ;define

    (define (fxmapping-delete-all fxmap keys)
      (assume (or (pair? keys) (null? keys)))
      (fold (lambda (k im) (fxmapping-delete im k))
        fxmap
        keys
      ) ;fold
    ) ;define

    (define (fxmapping-delete-min fxmap)
      (assume (fxmapping? fxmap))
      (assume (not (fxmapping-empty? fxmap)))
      (let-values (((k v trie)
                    (trie-pop-min (fxmapping-trie fxmap))
                   ) ;
                  ) ;
        (raw-fxmapping trie)
      ) ;let-values
    ) ;define

    (define (fxmapping-pop-min fxmap)
      (assume (fxmapping? fxmap))
      (assume (not (fxmapping-empty? fxmap)))
      (let-values (((k v trie)
                    (trie-pop-min (fxmapping-trie fxmap))
                   ) ;
                  ) ;
        (values k v (raw-fxmapping trie))
      ) ;let-values
    ) ;define

    (define (fxmapping-delete-max fxmap)
      (assume (fxmapping? fxmap))
      (assume (not (fxmapping-empty? fxmap)))
      (let-values (((k v trie)
                    (trie-pop-max (fxmapping-trie fxmap))
                   ) ;
                  ) ;
        (raw-fxmapping trie)
      ) ;let-values
    ) ;define

    (define (fxmapping-pop-max fxmap)
      (assume (fxmapping? fxmap))
      (assume (not (fxmapping-empty? fxmap)))
      (let-values (((k v trie)
                    (trie-pop-max (fxmapping-trie fxmap))
                   ) ;
                  ) ;
        (values k v (raw-fxmapping trie))
      ) ;let-values
    ) ;define

    (define (trie-pop-min trie)
      (let-values (((k v) (trie-min trie)))
        (values k v (trie-delete trie k))
      ) ;let-values
    ) ;define

    (define (trie-pop-max trie)
      (let-values (((k v) (trie-max trie)))
        (values k v (trie-delete trie k))
      ) ;let-values
    ) ;define

    ;; ;; The whole fxmapping

    (define (fxmapping-size fxmap)
      (assume (fxmapping? fxmap))
      (trie-size (fxmapping-trie fxmap))
    ) ;define

    (define fxmapping-find
      (case-lambda
       ((pred fxmap failure)
        (fxmapping-find pred
          fxmap
          failure
          values
        ) ;fxmapping-find
       ) ;
       ((pred fxmap failure success)
        (assume (procedure? pred))
        (assume (fxmapping? fxmap))
        (assume (procedure? failure))
        (assume (procedure? success))
        (trie-find pred
          (fxmapping-trie fxmap)
          failure
          success
        ) ;trie-find
       ) ;
      ) ;case-lambda
    ) ;define

    (define (fxmapping-count pred fxmap)
      (assume (procedure? pred))
      (fxmapping-fold (lambda (k v acc)
                        (if (pred k v) (+ 1 acc) acc)
                      ) ;lambda
        0
        fxmap
      ) ;fxmapping-fold
    ) ;define

    (define (fxmapping-any? pred fxmap)
      (assume (procedure? pred))
      (call-with-current-continuation (lambda (return)
                                        (fxmapping-fold (lambda (k v _)
                                                          (and (pred k v) (return #t))
                                                        ) ;lambda
                                          #f
                                          fxmap
                                        ) ;fxmapping-fold
                                      ) ;lambda
      ) ;call-with-current-continuation
    ) ;define

    (define (fxmapping-every? pred fxmap)
      (assume (procedure? pred))
      (call-with-current-continuation (lambda (return)
                                        (fxmapping-fold (lambda (k v _)
                                                          (or (pred k v) (return #f))
                                                        ) ;lambda
                                          #t
                                          fxmap
                                        ) ;fxmapping-fold
                                      ) ;lambda
      ) ;call-with-current-continuation
    ) ;define

    ;; ;; Mapping and folding

    (define (fxmapping-map proc fxmap)
      (assume (procedure? proc))
      (assume (fxmapping? fxmap))
      (raw-fxmapping (trie-map proc (fxmapping-trie fxmap))
      ) ;raw-fxmapping
    ) ;define

    (define (unspecified)
      (if #f #f)
    ) ;define

    (define (fxmapping-for-each proc fxmap)
      (assume (procedure? proc))
      (fxmapping-fold (lambda (k v _)
                        (proc k v)
                        (unspecified)
                      ) ;lambda
        (unspecified)
        fxmap
      ) ;fxmapping-fold
    ) ;define

    (define (fxmapping-fold proc nil fxmap)
      (assume (procedure? proc))
      (assume (fxmapping? fxmap))
      (let ((trie (fxmapping-trie fxmap)))
        (if (branch? trie)
          (if (negative? (branch-branching-bit trie))
            (trie-fold-left proc
              (trie-fold-left proc
                nil
                (branch-right trie)
              ) ;trie-fold-left
              (branch-left trie)
            ) ;trie-fold-left
            (trie-fold-left proc
              (trie-fold-left proc
                nil
                (branch-left trie)
              ) ;trie-fold-left
              (branch-right trie)
            ) ;trie-fold-left
          ) ;if
          (trie-fold-left proc nil trie)
        ) ;if
      ) ;let
    ) ;define

    (define (fxmapping-fold-right proc nil fxmap)
      (assume (procedure? proc))
      (assume (fxmapping? fxmap))
      (let ((trie (fxmapping-trie fxmap)))
        (if (branch? trie)
          (if (negative? (branch-branching-bit trie))
            (trie-fold-right proc
              (trie-fold-right proc
                nil
                (branch-left trie)
              ) ;trie-fold-right
              (branch-right trie)
            ) ;trie-fold-right
            (trie-fold-right proc
              (trie-fold-right proc
                nil
                (branch-right trie)
              ) ;trie-fold-right
              (branch-left trie)
            ) ;trie-fold-right
          ) ;if
          (trie-fold-right proc nil trie)
        ) ;if
      ) ;let
    ) ;define

    (define (fxmapping-map->list proc fxmap)
      (assume (procedure? proc))
      (fxmapping-fold-right (lambda (k v us) (cons (proc k v) us))
        '()
        fxmap
      ) ;fxmapping-fold-right
    ) ;define

    (define (fxmapping-filter pred fxmap)
      (assume (procedure? pred))
      (assume (fxmapping? fxmap))
      (raw-fxmapping (trie-filter pred
                       (fxmapping-trie fxmap)
                     ) ;trie-filter
      ) ;raw-fxmapping
    ) ;define

    (define (fxmapping-remove pred fxmap)
      (fxmapping-filter (lambda (k v) (not (pred k v)))
        fxmap
      ) ;fxmapping-filter
    ) ;define

    (define (fxmapping-partition pred fxmap)
      (assume (procedure? pred))
      (assume (fxmapping? fxmap))
      (let-values (((tin tout)
                    (trie-partition pred
                      (fxmapping-trie fxmap)
                    ) ;trie-partition
                   ) ;
                  ) ;
        (values (raw-fxmapping tin)
          (raw-fxmapping tout)
        ) ;values
      ) ;let-values
    ) ;define

    ;; ;; Conversion

    (define (fxmapping->alist fxmap)
      (fxmapping-fold-right (lambda (k v as) (cons (cons k v) as))
        '()
        fxmap
      ) ;fxmapping-fold-right
    ) ;define

    (define (fxmapping->decreasing-alist fxmap)
      (fxmapping-fold (lambda (k v as) (cons (cons k v) as))
        '()
        fxmap
      ) ;fxmapping-fold
    ) ;define

    (define (fxmapping-keys fxmap)
      (fxmapping-fold-right (lambda (k _ ks) (cons k ks))
        '()
        fxmap
      ) ;fxmapping-fold-right
    ) ;define

    (define (fxmapping-values fxmap)
      (fxmapping-fold-right (lambda (_ v vs) (cons v vs))
        '()
        fxmap
      ) ;fxmapping-fold-right
    ) ;define

    ;; ;; Comparison

    (define (comparator? x)
      (procedure? x)
    ) ;define

    (define (fxmapping=?
              comp
              fxmap1
              fxmap2
              .
              fxmaps
            ) ;
      (assume (comparator? comp))
      (assume (fxmapping? fxmap1))
      (let ((fxmap-eq1 (lambda (fxmap)
                         (assume (fxmapping? fxmap))
                         (or (eqv? fxmap1 fxmap)
                           (trie=? comp
                             (fxmapping-trie fxmap1)
                             (fxmapping-trie fxmap)
                           ) ;trie=?
                         ) ;or
                       ) ;lambda
            ) ;fxmap-eq1
           ) ;
        (and (fxmap-eq1 fxmap2)
          (or (null? fxmaps)
            (every fxmap-eq1 fxmaps)
          ) ;or
        ) ;and
      ) ;let
    ) ;define

    (define (fxmapping<?
              comp
              fxmap1
              fxmap2
              .
              fxmaps
            ) ;
      (assume (comparator? comp))
      (assume (fxmapping? fxmap1))
      (assume (fxmapping? fxmap2))
      (let lp
        ((t1 (fxmapping-trie fxmap1))
         (t2 (fxmapping-trie fxmap2))
         (fxmaps fxmaps)
        ) ;
        (and (trie-proper-subset? comp t1 t2)
          (or (null? fxmaps)
            (lp t2
              (fxmapping-trie (car fxmaps))
              (cdr fxmaps)
            ) ;lp
          ) ;or
        ) ;and
      ) ;let
    ) ;define

    (define (fxmapping>?
              comp
              fxmap1
              fxmap2
              .
              fxmaps
            ) ;
      (assume (comparator? comp))
      (assume (fxmapping? fxmap1))
      (assume (fxmapping? fxmap2))
      (let lp
        ((t1 (fxmapping-trie fxmap1))
         (t2 (fxmapping-trie fxmap2))
         (fxmaps fxmaps)
        ) ;
        (and (trie-proper-subset? comp t2 t1)
          (or (null? fxmaps)
            (lp t2
              (fxmapping-trie (car fxmaps))
              (cdr fxmaps)
            ) ;lp
          ) ;or
        ) ;and
      ) ;let
    ) ;define

    (define (fxmapping<=?
              comp
              fxmap1
              fxmap2
              .
              fxmaps
            ) ;
      (assume (comparator? comp))
      (assume (fxmapping? fxmap1))
      (assume (fxmapping? fxmap2))
      (let lp
        ((t1 (fxmapping-trie fxmap1))
         (t2 (fxmapping-trie fxmap2))
         (fxmaps fxmaps)
        ) ;
        (and (memv (trie-subset-compare comp t1 t2)
               '(less equal)
             ) ;memv
          (or (null? fxmaps)
            (lp t2
              (fxmapping-trie (car fxmaps))
              (cdr fxmaps)
            ) ;lp
          ) ;or
        ) ;and
      ) ;let
    ) ;define

    (define (fxmapping>=?
              comp
              fxmap1
              fxmap2
              .
              fxmaps
            ) ;
      (assume (comparator? comp))
      (assume (fxmapping? fxmap1))
      (assume (fxmapping? fxmap2))
      (let lp
        ((t1 (fxmapping-trie fxmap1))
         (t2 (fxmapping-trie fxmap2))
         (fxmaps fxmaps)
        ) ;
        (and (memv (trie-subset-compare comp t2 t1)
               '(less equal)
             ) ;memv
          (or (null? fxmaps)
            (lp t2
              (fxmapping-trie (car fxmaps))
              (cdr fxmaps)
            ) ;lp
          ) ;or
        ) ;and
      ) ;let
    ) ;define

    ;; ;; Set theory operations

    (define (fxmapping-union . args)
      (apply fxmapping-union/combinator
        first-arg
        args
      ) ;apply
    ) ;define

    (define (fxmapping-intersection . args)
      (apply fxmapping-intersection/combinator
        first-arg
        args
      ) ;apply
    ) ;define

    (define fxmapping-difference
      (case-lambda
       ((fxmap1 fxmap2)
        (assume (fxmapping? fxmap1))
        (assume (fxmapping? fxmap2))
        (raw-fxmapping (trie-difference (fxmapping-trie fxmap1)
                         (fxmapping-trie fxmap2)
                       ) ;trie-difference
        ) ;raw-fxmapping
       ) ;
       ((fxmap . rest)
        (assume (fxmapping? fxmap))
        (assume (pair? rest))
        (raw-fxmapping (trie-difference (fxmapping-trie fxmap)
                         (fxmapping-trie (apply fxmapping-union rest)
                         ) ;fxmapping-trie
                       ) ;trie-difference
        ) ;raw-fxmapping
       ) ;
      ) ;case-lambda
    ) ;define

    (define (fxmapping-xor fxmap1 fxmap2)
      (assume (fxmapping? fxmap1))
      (assume (fxmapping? fxmap2))
      (raw-fxmapping (trie-xor (fxmapping-trie fxmap1)
                       (fxmapping-trie fxmap2)
                     ) ;trie-xor
      ) ;raw-fxmapping
    ) ;define

    (define (fxmapping-union/combinator
              proc
              fxmap
              .
              rest
            ) ;
      (assume (procedure? proc))
      (assume (fxmapping? fxmap))
      (assume (pair? rest))
      (raw-fxmapping (fold (lambda (im t)
                             (assume (fxmapping? im))
                             (trie-merge proc t (fxmapping-trie im))
                           ) ;lambda
                       (fxmapping-trie fxmap)
                       rest
                     ) ;fold
      ) ;raw-fxmapping
    ) ;define

    (define (fxmapping-intersection/combinator
              proc
              fxmap
              .
              rest
            ) ;
      (assume (procedure? proc))
      (assume (fxmapping? fxmap))
      (assume (pair? rest))
      (raw-fxmapping (fold (lambda (im t)
                             (assume (fxmapping? im))
                             (trie-intersection proc
                               (fxmapping-trie im)
                               t
                             ) ;trie-intersection
                           ) ;lambda
                       (fxmapping-trie fxmap)
                       rest
                     ) ;fold
      ) ;raw-fxmapping
    ) ;define

    ;; ;; Subsets

    (define (fxsubmapping= fxmap key)
      (fxmapping-ref fxmap
        key
        fxmapping
        (lambda (v) (fxmapping key v))
      ) ;fxmapping-ref
    ) ;define

    (define (fxmapping-open-interval fxmap low high)
      (assume (fxmapping? fxmap))
      (assume (valid-integer? low))
      (assume (valid-integer? high))
      (assume (>= high low))
      (raw-fxmapping (subtrie-interval (fxmapping-trie fxmap)
                       low
                       high
                       #f
                       #f
                     ) ;subtrie-interval
      ) ;raw-fxmapping
    ) ;define

    (define (fxmapping-closed-interval fxmap
              low
              high
            ) ;fxmapping-closed-interval
      (assume (fxmapping? fxmap))
      (assume (valid-integer? low))
      (assume (valid-integer? high))
      (assume (>= high low))
      (raw-fxmapping (subtrie-interval (fxmapping-trie fxmap)
                       low
                       high
                       #t
                       #t
                     ) ;subtrie-interval
      ) ;raw-fxmapping
    ) ;define

    (define (fxmapping-open-closed-interval fxmap
              low
              high
            ) ;fxmapping-open-closed-interval
      (assume (fxmapping? fxmap))
      (assume (valid-integer? low))
      (assume (valid-integer? high))
      (assume (>= high low))
      (raw-fxmapping (subtrie-interval (fxmapping-trie fxmap)
                       low
                       high
                       #f
                       #t
                     ) ;subtrie-interval
      ) ;raw-fxmapping
    ) ;define

    (define (fxmapping-closed-open-interval fxmap
              low
              high
            ) ;fxmapping-closed-open-interval
      (assume (fxmapping? fxmap))
      (assume (valid-integer? low))
      (assume (valid-integer? high))
      (assume (>= high low))
      (raw-fxmapping (subtrie-interval (fxmapping-trie fxmap)
                       low
                       high
                       #t
                       #f
                     ) ;subtrie-interval
      ) ;raw-fxmapping
    ) ;define

    (define (fxsubmapping< fxmap key)
      (assume (fxmapping? fxmap))
      (assume (valid-integer? key))
      (raw-fxmapping (subtrie< (fxmapping-trie fxmap) key #f)
      ) ;raw-fxmapping
    ) ;define

    (define (fxsubmapping<= fxmap key)
      (assume (fxmapping? fxmap))
      (assume (valid-integer? key))
      (raw-fxmapping (subtrie< (fxmapping-trie fxmap) key #t)
      ) ;raw-fxmapping
    ) ;define

    (define (fxsubmapping> fxmap key)
      (assume (fxmapping? fxmap))
      (assume (valid-integer? key))
      (raw-fxmapping (subtrie> (fxmapping-trie fxmap) key #f)
      ) ;raw-fxmapping
    ) ;define

    (define (fxsubmapping>= fxmap key)
      (assume (fxmapping? fxmap))
      (assume (valid-integer? key))
      (raw-fxmapping (subtrie> (fxmapping-trie fxmap) key #t)
      ) ;raw-fxmapping
    ) ;define

    (define (subtrie< trie k inclusive)
      (letrec ((split (lambda (t)
                        (cond ((not t) #f)
                              ((leaf? t)
                               (let ((key (leaf-key t)))
                                 (if (or (and inclusive (= key k)) (< key k))
                                   t
                                   #f
                                 ) ;if
                               ) ;let
                              ) ;
                              (else (let ((p (branch-prefix t))
                                          (m (branch-branching-bit t))
                                          (l (branch-left t))
                                          (r (branch-right t))
                                         ) ;
                                      (if (match-prefix? k p m)
                                        (if (zero-bit? k m)
                                          (split l)
                                          (trie-union l (split r))
                                        ) ;if
                                        (and (< p k) t)
                                      ) ;if
                                    ) ;let
                              ) ;else
                        ) ;cond
                      ) ;lambda
               ) ;split
              ) ;
        (if (and (branch? trie)
              (negative? (branch-branching-bit trie))
            ) ;and
          (if (negative? k)
            (split (branch-right trie))
            (trie-union (split (branch-left trie))
              (branch-right trie)
            ) ;trie-union
          ) ;if
          (split trie)
        ) ;if
      ) ;letrec
    ) ;define

    (define (subtrie> trie k inclusive)
      (letrec ((split (lambda (t)
                        (cond ((not t) #f)
                              ((leaf? t)
                               (let ((key (leaf-key t)))
                                 (if (or (and inclusive (= key k)) (> key k))
                                   t
                                   #f
                                 ) ;if
                               ) ;let
                              ) ;
                              (else (let ((p (branch-prefix t))
                                          (m (branch-branching-bit t))
                                          (l (branch-left t))
                                          (r (branch-right t))
                                         ) ;
                                      (if (match-prefix? k p m)
                                        (if (zero-bit? k m)
                                          (trie-union (split l) r)
                                          (split r)
                                        ) ;if
                                        (and (> p k) t)
                                      ) ;if
                                    ) ;let
                              ) ;else
                        ) ;cond
                      ) ;lambda
               ) ;split
              ) ;
        (if (and (branch? trie)
              (negative? (branch-branching-bit trie))
            ) ;and
          (if (negative? k)
            (trie-union (split (branch-right trie))
              (branch-left trie)
            ) ;trie-union
            (split (branch-left trie))
          ) ;if
          (split trie)
        ) ;if
      ) ;letrec
    ) ;define

    (define (subtrie-interval trie
              a
              b
              low-inclusive
              high-inclusive
            ) ;subtrie-interval
      (letrec ((interval (lambda (t)
                           (cond ((not t) #f)
                                 ((leaf? t)
                                  (let ((key (leaf-key t)))
                                    (if (and (or low-inclusive (> key a))
                                          (or high-inclusive (< key b))
                                        ) ;and
                                      t
                                      #f
                                    ) ;if
                                  ) ;let
                                 ) ;
                                 (else (branch-interval t))
                           ) ;cond
                         ) ;lambda
               ) ;interval
               (branch-interval (lambda (t)
                                  (let ((p (branch-prefix t))
                                        (m (branch-branching-bit t))
                                        (l (branch-left t))
                                        (r (branch-right t))
                                       ) ;
                                    (if (match-prefix? a p m)
                                      (if (zero-bit? a m)
                                        (if (match-prefix? b p m)
                                          (if (zero-bit? b m)
                                            (interval l)
                                            (trie-union (subtrie> l a low-inclusive)
                                              (subtrie< r b high-inclusive)
                                            ) ;trie-union
                                          ) ;if
                                          (and (< b p)
                                            (trie-union (subtrie> l a low-inclusive)
                                              r
                                            ) ;trie-union
                                          ) ;and
                                        ) ;if
                                        (interval r)
                                      ) ;if
                                      (and (> p a)
                                        (subtrie< t b high-inclusive)
                                      ) ;and
                                    ) ;if
                                  ) ;let
                                ) ;lambda
               ) ;branch-interval
              ) ;
        (if (and (branch? trie)
              (negative? (branch-branching-bit trie))
            ) ;and
          (cond ((and (negative? a) (negative? b))
                 (interval (branch-right trie))
                ) ;
                ((and (positive? a) (positive? b))
                 (interval (branch-left trie))
                ) ;
                (else (trie-union (subtrie> (branch-right trie)
                                    a
                                    low-inclusive
                                  ) ;subtrie>
                        (subtrie< (branch-left trie)
                          b
                          high-inclusive
                        ) ;subtrie<
                      ) ;trie-union
                ) ;else
          ) ;cond
          (interval trie)
        ) ;if
      ) ;letrec
    ) ;define

    (define (fxmapping-split fxmap k)
      (assume (fxmapping? fxmap))
      (assume (integer? k))
      (let-values (((trie-low trie-high)
                    (trie-split (fxmapping-trie fxmap) k)
                   ) ;
                  ) ;
        (values (raw-fxmapping trie-low)
          (raw-fxmapping trie-high)
        ) ;values
      ) ;let-values
    ) ;define

    (define (trie-split trie k)
      (values (subtrie< trie k #f)
        (subtrie> trie k #t)
      ) ;values
    ) ;define

  ) ;begin
) ;define-library
