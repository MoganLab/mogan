;
; Copyright (C) 2020 Wolfgang Corcoran-Mathe
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

(define-library (srfi srfi-217)
  (export
    ;; Constructors
    iset iset-unfold make-range-iset
    ;; Predicates
    iset? iset-contains? iset-empty? iset-disjoint?
    ;; Accessors
    iset-member iset-min iset-max
    ;; Updaters
    iset-adjoin iset-adjoin! iset-delete iset-delete!
    iset-delete-all iset-delete-all! iset-search iset-search!
    iset-delete-min iset-delete-min! iset-delete-max iset-delete-max!
    ;; The whole iset
    iset-size iset-find iset-count iset-any? iset-every?
    ;; Mapping and folding
    iset-map iset-for-each iset-fold iset-fold-right
    iset-filter iset-filter! iset-remove iset-remove!
    iset-partition iset-partition!
    ;; Copying and conversion
    iset-copy iset->list list->iset list->iset!
    ;; Subsets
    iset=? iset<? iset>? iset<=? iset>=?
    ;; Set theory operations
    iset-union iset-union! iset-intersection iset-intersection!
    iset-difference iset-difference! iset-xor iset-xor!
    ;; Intervals and ranges
    iset-open-interval iset-closed-interval
    iset-open-closed-interval iset-closed-open-interval
    isubset= isubset< isubset<= isubset> isubset>=
  ) ;export

  (import (scheme base)
          (scheme case-lambda)
          (srfi srfi-1)
          (rename (liii bitwise)
                  (ash arithmetic-shift)
          ) ;rename
  ) ;import

  (begin

;;;; Utility

(define (assume condition . args)
  (if (not condition)
      (apply error args)
  ) ;if
) ;define

;;;; Trie implementation

;; This file implements integers sets as compressed binary radix
;; trees (AKA Patricia tries), as described by Chris Okasaki and
;; Andrew Gill in "Fast Mergeable Integer Maps" (1998).

;; A trie is represented by #f (the empty trie), a leaf, or a branch.

;; Record types for leaves and branches

(define-record-type <leaf>
  (raw-leaf prefix bitmap)
  leaf?
  (prefix leaf-prefix)
  (bitmap leaf-bitmap)
) ;define-record-type

(define (leaf prefix bitmap)
  (if (positive? bitmap)
      (raw-leaf prefix bitmap)
      #f
  ) ;if
) ;define

(define-record-type <branch>
  (raw-branch prefix branching-bit left right)
  branch?
  (prefix branch-prefix)
  (branching-bit branch-branching-bit)
  (left branch-left)
  (right branch-right)
) ;define-record-type

(define (branch prefix mask trie1 trie2)
  (cond ((not trie1) trie2)
        ((not trie2) trie1)
        (else (raw-branch prefix mask trie1 trie2))
  ) ;cond
) ;define

;;;; Bitwise constants and procedures

;; S7 Scheme: use 5-bit suffix (32 values per leaf) to avoid ash overflow
(define leaf-bitmap-size 32)

(define suffix-mask (- leaf-bitmap-size 1))
(define prefix-mask (lognot suffix-mask))

;; In S7 Scheme, all integers are fixnums
(define (valid-integer? x) (integer? x))

;; least-fixnum in S7 64-bit is -9223372036854775808
(define least-fixnum-val -9223372036854775808)

(define (mask k m)
  (if (= m least-fixnum-val)
      0
      (logand k (logxor (lognot (- m 1)) m))
  ) ;if
) ;define

(define (match-prefix? k p m)
  (= (mask k m) p)
) ;define

(define (branching-bit p1 m1 p2 m2)
  (if (negative? (logxor p1 p2))
      least-fixnum-val
      (highest-bit-mask (logxor p1 p2) (max 1 (* 2 (max m1 m2))))
  ) ;if
) ;define

(define (lowest-bit-mask b)
  (logand b (- b))
) ;define

(define (highest-bit-mask k guess-m)
  (let lp ((x (logand k (lognot (- guess-m 1)))))
    (let ((m (lowest-bit-mask x)))
      (if (= x m)
          m
          (lp (- x m))
      ) ;if
    ) ;let
  ) ;let
) ;define

(define (highest-set-bit k)
  (first-set-bit (highest-bit-mask k 1))
) ;define

(define (zero-bit? k m)
  (zero? (logand k m))
) ;define

(define (isuffix k)
  (logand k suffix-mask)
) ;define

(define (iprefix k)
  (logand k prefix-mask)
) ;define

(define (ibitmap k)
  (arithmetic-shift 1 (isuffix k))
) ;define

(define (bitmap-delete bitmap key)
  (logand bitmap (lognot (ibitmap key)))
) ;define

(define (bitmap-delete-min b)
  (logand b (lognot (lowest-bit-mask b)))
) ;define

(define (bitmap-delete-max b)
  (logand b (lognot (highest-bit-mask b (lowest-bit-mask b))))
) ;define

;;;; Predicates and accessors

(define (trie-contains? trie key)
  (and trie
       (if (leaf? trie)
           (and (= (iprefix key) (leaf-prefix trie))
                (not (zero? (logand (ibitmap key) (leaf-bitmap trie))))
           ) ;and
           (let ((p (branch-prefix trie))
                 (m (branch-branching-bit trie))
                 (l (branch-left trie))
                 (r (branch-right trie)))
             (and (match-prefix? key p m)
                  (if (zero-bit? key m)
                      (trie-contains? l key)
                      (trie-contains? r key)
                  ) ;if
             ) ;and
           ) ;let
       ) ;if
  ) ;and
) ;define

(define (trie-min trie)
  (letrec
   ((search
     (lambda (t)
       (and t
            (if (leaf? t)
                (+ (leaf-prefix t) (first-set-bit (leaf-bitmap t)))
                (search (branch-left t)))
            ) ;if
       ) ;and
     ) ;lambda
    ) ;search
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
  (letrec
   ((search
     (lambda (t)
       (and t
            (if (leaf? t)
                (+ (leaf-prefix t) (highest-set-bit (leaf-bitmap t)))
                (search (branch-right t)))
            ) ;if
       ) ;and
     ) ;lambda
    ) ;search
    (if (branch? trie)
        (if (negative? (branch-branching-bit trie))
            (search (branch-left trie))
            (search (branch-right trie))
        ) ;if
        (search trie)
    ) ;if
  ) ;letrec
) ;define

;;;; Insert

(define (%trie-insert-parts trie prefix bitmap)
  (letrec
   ((ins
     (lambda (t)
       (cond ((not t) (raw-leaf prefix bitmap))
             ((leaf? t)
              (let ((p (leaf-prefix t))
                    (b (leaf-bitmap t)))
                (if (= prefix p)
                    (raw-leaf prefix (logior b bitmap))
                    (%trie-join prefix 0 (raw-leaf prefix bitmap) p 0 t)
                ) ;if
              ) ;let
             ) ;
             (else
              (let ((p (branch-prefix t))
                    (m (branch-branching-bit t))
                    (l (branch-left t))
                    (r (branch-right t)))
                (if (match-prefix? prefix p m)
                    (if (zero-bit? prefix m)
                        (branch p m (ins l) r)
                        (branch p m l (ins r))
                    ) ;if
                    (%trie-join prefix 0 (raw-leaf prefix bitmap) p m t))
                ) ;if
              ) ;let
             ) ;else
       ) ;cond
     ) ;lambda
    ) ;ins
    (ins trie)
  ) ;letrec
) ;define

(define (trie-insert trie key)
  (%trie-insert-parts trie (iprefix key) (ibitmap key))
) ;define

;;;; Iterators and filters

(define (trie-fold proc nil trie)
  (letrec
   ((cata
     (lambda (b t)
       (cond ((not t) b)
             ((leaf? t)
              (fold-left-bits (leaf-prefix t) proc b (leaf-bitmap t))
             ) ;
             (else
              (cata (cata b (branch-left t)) (branch-right t)))
             ) ;else
       ) ;cond
     ) ;lambda
    ) ;cata
    (if (branch? trie)
        (let ((p (branch-prefix trie))
              (m (branch-branching-bit trie))
              (l (branch-left trie))
              (r (branch-right trie)))
          (if (negative? m)
              (cata (cata nil r) l)
              (cata (cata nil l) r)
          ) ;if
        ) ;let
        (cata nil trie)
    ) ;if
  ) ;letrec
) ;define

(define (fold-left-bits prefix proc nil bitmap)
  (let loop ((bm bitmap) (acc nil))
    (if (zero? bm)
        acc
        (let* ((mask (lowest-bit-mask bm))
               (bi (first-set-bit mask)))
          (loop (logxor bm mask) (proc (+ prefix bi) acc))
        ) ;let*
    ) ;if
  ) ;let
) ;define

(define (trie-fold-right proc nil trie)
  (letrec
   ((cata
     (lambda (b t)
       (cond ((not t) b)
             ((leaf? t)
              (fold-right-bits (leaf-prefix t) proc b (leaf-bitmap t))
             ) ;
             (else
              (cata (cata b (branch-right t)) (branch-left t)))
             ) ;else
       ) ;cond
     ) ;lambda
    ) ;cata
    (if (branch? trie)
        (let ((p (branch-prefix trie))
              (m (branch-branching-bit trie))
              (l (branch-left trie))
              (r (branch-right trie)))
          (if (negative? m)
              (cata (cata nil l) r)
              (cata (cata nil r) l)
          ) ;if
        ) ;let
        (cata nil trie)
    ) ;if
  ) ;letrec
) ;define

(define (fold-right-bits prefix proc nil bitmap)
  (let loop ((bm bitmap) (acc nil))
    (if (zero? bm)
        acc
        (let* ((mask (highest-bit-mask bm (lowest-bit-mask bm)))
               (bi (first-set-bit mask)))
          (loop (logxor bm mask) (proc (+ prefix bi) acc))
        ) ;let*
    ) ;if
  ) ;let
) ;define

(define (bitmap-partition pred prefix bitmap)
  (let loop ((i 0) (in 0) (out 0))
    (cond ((= i leaf-bitmap-size) (values in out))
          ((bit-set? i bitmap)
           (let ((bit (arithmetic-shift 1 i)))
             (if (pred (+ prefix i))
                 (loop (+ i 1) (logior in bit) out)
                 (loop (+ i 1) in (logior out bit))
             ) ;if
           ) ;let
          ) ;
          (else (loop (+ i 1) in out))
    ) ;cond
  ) ;let
) ;define

(define (trie-partition pred trie)
  (letrec
   ((part
     (lambda (t)
       (cond ((not t) (values #f #f))
             ((leaf? t)
              (let ((p (leaf-prefix t))
                    (bm (leaf-bitmap t)))
                (let-values (((in out) (bitmap-partition pred p bm)))
                  (values (leaf p in) (leaf p out))
                ) ;let-values
              ) ;let
             ) ;
             (else
              (let ((p (branch-prefix t))
                    (m (branch-branching-bit t))
                    (l (branch-left t))
                    (r (branch-right t)))
                (let-values (((il ol) (part l))
                             ((ir or) (part r)))
                  (values (branch p m il ir) (branch p m ol or)))
                ) ;let-values
              ) ;let
             ) ;else
       ) ;cond
     ) ;lambda
    ) ;part
    (part trie)
  ) ;letrec
) ;define

(define (bitmap-filter pred prefix bitmap)
  (let loop ((i 0) (res 0))
    (cond ((= i leaf-bitmap-size) res)
          ((and (bit-set? i bitmap) (pred (+ prefix i)))
           (loop (+ i 1) (logior res (arithmetic-shift 1 i)))
          ) ;
          (else (loop (+ i 1) res))
    ) ;cond
  ) ;let
) ;define

(define (trie-filter pred trie)
  (cond ((not trie) #f)
        ((leaf? trie)
         (let ((p (leaf-prefix trie))
               (bm (leaf-bitmap trie)))
           (leaf p (bitmap-filter pred p bm))
         ) ;let
        ) ;
        (else
         (branch (branch-prefix trie)
                 (branch-branching-bit trie)
                 (trie-filter pred (branch-left trie))
                 (trie-filter pred (branch-right trie))
         ) ;branch
        ) ;else
  ) ;cond
) ;define

;;;; Update operations

(define (trie-delete trie key)
  (letrec*
   ((prefix (iprefix key))
    (update
     (lambda (t)
       (cond ((not t) #f)
             ((leaf? t)
              (let ((p (leaf-prefix t))
                    (bm (leaf-bitmap t)))
                (if (= p prefix)
                    (leaf p (bitmap-delete bm key))
                    t
                ) ;if
              ) ;let
             ) ;
             (else
              (let ((p (branch-prefix t))
                    (m (branch-branching-bit t))
                    (l (branch-left t))
                    (r (branch-right t)))
                (if (match-prefix? prefix p m)
                    (if (zero-bit? prefix m)
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
   (update trie))
  ) ;letrec*
) ;define

(define (trie-delete-min trie)
  (letrec
   ((update/min
     (lambda (t)
       (cond ((not t) (error "Empty set"))
             ((leaf? t)
              (let ((p (leaf-prefix t))
                    (bm (leaf-bitmap t)))
                (values (+ p (first-set-bit bm))
                        (leaf p (bitmap-delete-min bm))
                ) ;values
              ) ;let
             ) ;
             (else
              (let ((p (branch-prefix t))
                    (m (branch-branching-bit t))
                    (l (branch-left t))
                    (r (branch-right t)))
                (let-values (((n l*) (update/min l)))
                  (values n (branch p m l* r)))
                ) ;let-values
              ) ;let
             ) ;else
       ) ;cond
     ) ;lambda
    ) ;update/min
    (if (branch? trie)
        (let ((p (branch-prefix trie))
              (m (branch-branching-bit trie))
              (l (branch-left trie))
              (r (branch-right trie)))
          (if (negative? m)
              (let-values (((n r*) (update/min r)))
                (values n (branch p m l r*))
              ) ;let-values
              (let-values (((n l*) (update/min l)))
                (values n (branch p m l* r))
              ) ;let-values
          ) ;if
        ) ;let
        (update/min trie)
    ) ;if
  ) ;letrec
) ;define

(define (trie-delete-max trie)
  (letrec
   ((update/max
     (lambda (t)
       (cond ((not t) (error "Empty set"))
             ((leaf? t)
              (let ((p (leaf-prefix t))
                    (bm (leaf-bitmap t)))
                (values (+ p (highest-set-bit bm))
                        (leaf p (bitmap-delete-max bm))
                ) ;values
              ) ;let
             ) ;
             (else
              (let ((p (branch-prefix t))
                    (m (branch-branching-bit t))
                    (l (branch-left t))
                    (r (branch-right t)))
                (let-values (((n r*) (update/max r)))
                  (values n (branch p m l r*)))
                ) ;let-values
              ) ;let
             ) ;else
       ) ;cond
     ) ;lambda
    ) ;update/max
    (if (branch? trie)
        (let ((p (branch-prefix trie))
              (m (branch-branching-bit trie))
              (l (branch-left trie))
              (r (branch-right trie)))
          (if (negative? m)
              (let-values (((n l*) (update/max l)))
                (values n (branch p m l* r))
              ) ;let-values
              (let-values (((n r*) (update/max r)))
                (values n (branch p m l r*))
              ) ;let-values
          ) ;if
        ) ;let
        (update/max trie)
    ) ;if
  ) ;letrec
) ;define

(define (trie-search trie key failure success)
  (let* ((kp (iprefix key))
         (key-leaf (raw-leaf kp (ibitmap key))))
    (letrec
     ((search
       (lambda (t build)
         (cond
           ((not t)
            (failure (lambda (obj) (build key-leaf obj))
                     (lambda (obj) (build #f obj))
            ) ;failure
           ) ;
           ((leaf? t)
            (leaf-search t key failure success build)
           ) ;
           (else
            (let ((p (branch-prefix t))
                  (m (branch-branching-bit t))
                  (l (branch-left t))
                  (r (branch-right t)))
              (if (match-prefix? key p m)
                  (if (zero-bit? key m)
                      (search l (lambda (l* obj)
                                  (build (branch p m l* r) obj))
                      ) ;search
                      (search r (lambda (r* obj)
                                  (build (branch p m l r*) obj))
                      ) ;search
                  ) ;if
                  (failure (lambda (obj)
                             (build (%trie-join kp 0 key-leaf p m t)
                                    obj)
                             ) ;build
                           (lambda (obj) (build t obj)))
                  ) ;failure
              ) ;if
            ) ;let
           ) ;else
         ) ;cond
       ) ;lambda
      ) ;search
      (if (branch? trie)
          (let ((p (branch-prefix trie))
                (m (branch-branching-bit trie))
                (l (branch-left trie))
                (r (branch-right trie)))
            (if (negative? m)
                (if (negative? key)
                    (let-values (((r* obj) (search r values)))
                      (values (branch p m l r*) obj)
                    ) ;let-values
                    (let-values (((l* obj) (search l values)))
                      (values (branch p m l* r) obj)
                    ) ;let-values
                ) ;if
                (search trie values)
            ) ;if
          ) ;let
          (search trie values)
      ) ;if
    ) ;letrec
  ) ;let*
) ;define

(define (leaf-search lf key failure success build)
  (let ((kp (iprefix key)) (kb (ibitmap key)))
    (let ((p (leaf-prefix lf))
          (bm (leaf-bitmap lf)))
      (if (= kp p)
          (if (zero? (logand kb bm))
              (failure (lambda (obj)
                         (build (raw-leaf p (logior kb bm)) obj))
                       (lambda (obj) (build lf obj))
              ) ;failure
              (success key
                       (lambda (elt obj)
                         (assume (eqv? key elt) "invalid new element")
                         (build lf obj)
                       ) ;lambda
                       (lambda (obj)
                         (build (leaf p (bitmap-delete bm key)) obj)
                       ) ;lambda
              ) ;success
          ) ;if
          (failure (lambda (obj)
                     (build (%trie-join kp 0 (raw-leaf kp kb) p 0 lf)
                            obj)
                     ) ;build
                   (lambda (obj) (build lf obj))
          ) ;failure
      ) ;if
    ) ;let
  ) ;let
) ;define

;;;; Set-theoretical operations

(define (%trie-join prefix1 mask1 trie1 prefix2 mask2 trie2)
  (let ((m (branching-bit prefix1 mask1 prefix2 mask2)))
    (if (zero-bit? prefix1 m)
        (raw-branch (mask prefix1 m) m trie1 trie2)
        (raw-branch (mask prefix1 m) m trie2 trie1)
    ) ;if
  ) ;let
) ;define

(define (branching-bit-higher? mask1 mask2)
  (if (negative? (logxor mask1 mask2))
      (negative? mask1)
      (> mask1 mask2)
  ) ;if
) ;define

(define (%trie-merge insert-leaf trie1 trie2)
  (letrec
    ((merge
      (lambda (s t)
        (cond ((not s) t)
              ((not t) s)
              ((leaf? s) (insert-leaf t s))
              ((leaf? t) (insert-leaf s t))
              (else (merge-branches s t)))
        ) ;cond
      ) ;lambda
     (merge-branches
      (lambda (s t)
        (let ((p (branch-prefix s))
              (m (branch-branching-bit s))
              (s1 (branch-left s))
              (s2 (branch-right s))
              (q (branch-prefix t))
              (n (branch-branching-bit t))
              (t1 (branch-left t))
              (t2 (branch-right t)))
          (cond
            ((and (= m n) (= p q))
             (branch p m (merge s1 t1) (merge s2 t2))
            ) ;
            ((and (branching-bit-higher? m n) (match-prefix? q p m))
             (if (zero-bit? q m)
                 (branch p m (merge s1 t) s2)
                 (branch p m s1 (merge s2 t))
             ) ;if
            ) ;
            ((and (branching-bit-higher? n m) (match-prefix? p q n))
             (if (zero-bit? p n)
                 (branch q n (merge s t1) t2)
                 (branch q n t1 (merge s t2))
             ) ;if
            ) ;
            (else
             (%trie-join p m s q n t)
            ) ;else
          ) ;cond
        ) ;let
      ) ;lambda
     ) ;merge-branches
    ) ;
    (merge trie1 trie2)
  ) ;letrec
) ;define

(define (trie-union trie1 trie2)
  (%trie-merge (lambda (s t) (insert-leaf/proc logior s t))
               trie1
               trie2
  ) ;%trie-merge
) ;define

(define (trie-xor trie1 trie2)
  (%trie-merge (lambda (s t) (insert-leaf/proc logxor s t))
               trie1
               trie2
  ) ;%trie-merge
) ;define

(define (insert-leaf/proc fxcombine trie lf)
  (let ((p (leaf-prefix lf))
        (bm (leaf-bitmap lf)))
    (letrec
     ((ins
       (lambda (t)
         (cond ((not t) lf)
               ((leaf? t)
                (let ((q (leaf-prefix t))
                      (bm* (leaf-bitmap t)))
                  (if (= p q)
                      (leaf p (fxcombine bm bm*))
                      (%trie-join p 0 lf q 0 t)
                  ) ;if
                ) ;let
               ) ;
               (else
                (let ((q (branch-prefix t))
                      (m (branch-branching-bit t))
                      (l (branch-left t))
                      (r (branch-right t)))
                  (if (match-prefix? p q m)
                      (if (zero-bit? p m)
                          (raw-branch q m (ins l) r)
                          (raw-branch q m l (ins r))
                      ) ;if
                      (%trie-join p 0 lf q 0 t))
                  ) ;if
                ) ;let
               ) ;else
         ) ;cond
       ) ;lambda
      ) ;ins
      (ins trie)
    ) ;letrec
  ) ;let
) ;define

(define (trie-intersection trie1 trie2)
  (letrec
   ((intersect
     (lambda (s t)
       (cond ((or (not s) (not t)) #f)
             ((leaf? s) (intersect/leaf s t))
             ((leaf? t) (intersect/leaf t s))
             (else (intersect-branches s t)))
       ) ;cond
     ) ;lambda
    (intersect/leaf
     (lambda (l t)
       (let ((p (leaf-prefix l))
             (bm (leaf-bitmap l)))
         (let lp ((t t))
           (cond ((not t) #f)
                 ((leaf? t)
                  (if (= p (leaf-prefix t))
                      (leaf p (logand bm (leaf-bitmap t)))
                      #f
                  ) ;if
                 ) ;
                 (else
                  (let ((q (branch-prefix t))
                        (m (branch-branching-bit t))
                        (l (branch-left t))
                        (r (branch-right t)))
                    (if (match-prefix? p q m)
                        (if (zero-bit? p m) (lp l) (lp r))
                        #f
                    ) ;if
                  ) ;let
                 ) ;else
           ) ;cond
         ) ;let
       ) ;let
     ) ;lambda
    ) ;intersect/leaf
    (intersect-branches
     (lambda (s t)
       (let ((p (branch-prefix s))
             (m (branch-branching-bit s))
             (sl (branch-left s))
             (sr (branch-right s))
             (q (branch-prefix t))
             (n (branch-branching-bit t))
             (tl (branch-left t))
             (tr (branch-right t)))
         (cond
           ((branching-bit-higher? m n)
            (and (match-prefix? q p m)
                 (if (zero-bit? q m)
                     (intersect sl t)
                     (intersect sr t))
                 ) ;if
            ) ;and
           ((branching-bit-higher? n m)
            (and (match-prefix? p q n)
                 (if (zero-bit? p n)
                     (intersect s tl)
                     (intersect s tr)
                 ) ;if
            ) ;and
           ) ;
           ((= p q)
            (branch p m (intersect sl tl) (intersect sr tr))
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
  (letrec
   ((difference
     (lambda (s t)
       (cond ((not s) #f)
             ((not t) s)
             ((leaf? s) (diff/leaf s t))
             ((leaf? t)
              (%trie-delete-bitmap s (leaf-prefix t) (leaf-bitmap t))
             ) ;
             (else (branch-difference s t)))
       ) ;cond
     ) ;lambda
    (diff/leaf
     (lambda (lf t)
       (let ((p (leaf-prefix lf))
             (bm (leaf-bitmap lf)))
         (let lp ((t t))
           (cond ((not t) lf)
                 ((leaf? t)
                  (let ((q (leaf-prefix t))
                        (c (leaf-bitmap t)))
                    (if (= p q)
                        (leaf p (logand bm (lognot c)))
                        lf
                    ) ;if
                  ) ;let
                 ) ;
                 (else
                  (let ((q (branch-prefix t))
                        (m (branch-branching-bit t))
                        (l (branch-left t))
                        (r (branch-right t)))
                    (if (match-prefix? p q m)
                        (if (zero-bit? p m) (lp l) (lp r))
                        lf
                    ) ;if
                  ) ;let
                 ) ;else
           ) ;cond
         ) ;let
       ) ;let
     ) ;lambda
    ) ;diff/leaf
    (branch-difference
     (lambda (s t)
       (let ((p (branch-prefix s))
             (m (branch-branching-bit s))
             (sl (branch-left s))
             (sr (branch-right s))
             (q (branch-prefix t))
             (n (branch-branching-bit t))
             (tl (branch-left t))
             (tr (branch-right t)))
         (cond
           ((and (= m n) (= p q))
            (branch p m (difference sl tl) (difference sr tr))
           ) ;
           ((and (branching-bit-higher? m n) (match-prefix? q p m))
            (if (zero-bit? q m)
                (branch p m (difference sl t) sr)
                (branch p m sl (difference sr t))
            ) ;if
           ) ;
           ((and (branching-bit-higher? n m) (match-prefix? p q n))
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

(define (%trie-delete-bitmap trie prefix bitmap)
  (cond ((not trie) #f)
        ((leaf? trie)
         (if (= prefix (leaf-prefix trie))
             (leaf prefix (logand (leaf-bitmap trie) (lognot bitmap)))
             trie
         ) ;if
        ) ;
        (else
         (let ((p (branch-prefix trie))
               (m (branch-branching-bit trie))
               (l (branch-left trie))
               (r (branch-right trie)))
           (if (match-prefix? prefix p m)
               (if (zero-bit? prefix m)
                   (branch p m (%trie-delete-bitmap l prefix bitmap) r)
                   (branch p m l (%trie-delete-bitmap r prefix bitmap))
               ) ;if
               trie
           ) ;if
         ) ;let
        ) ;else
  ) ;cond
) ;define

;;;; Copying

(define (copy-trie trie)
  (cond ((not trie) #f)
        ((leaf? trie) (raw-leaf (leaf-prefix trie) (leaf-bitmap trie)))
        (else
         (raw-branch (branch-prefix trie)
                     (branch-branching-bit trie)
                     (copy-trie (branch-left trie))
                     (copy-trie (branch-right trie))
         ) ;raw-branch
        ) ;else
  ) ;cond
) ;define

;;;; Size

(define (trie-size trie)
  (let accum ((siz 0) (t trie))
    (cond ((not t) siz)
          ((leaf? t) (+ siz (bit-count (leaf-bitmap t))))
          (else
           (accum (accum siz (branch-left t))
                  (branch-right t)
           ) ;accum
          ) ;else
    ) ;cond
  ) ;let
) ;define

;;;; Comparisons

(define (trie=? trie1 trie2)
  (cond ((not (or trie1 trie2)) #t)
        ((and (leaf? trie1) (leaf? trie2))
         (and (= (leaf-prefix trie1) (leaf-prefix trie2))
              (= (leaf-bitmap trie1) (leaf-bitmap trie2))
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
               (r2 (branch-right trie2)))
           (and (= m n) (= p q) (trie=? l1 l2) (trie=? r1 r2))
         ) ;let
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (subset-compare-leaves l1 l2)
  (let ((p (leaf-prefix l1))
        (b (leaf-bitmap l1))
        (q (leaf-prefix l2))
        (c (leaf-bitmap l2)))
    (if (= p q)
        (if (= b c)
            'equal
            (if (zero? (logand b (lognot c)))
                'less
                'greater
            ) ;if
        ) ;if
        'greater
    ) ;if
  ) ;let
) ;define

(define (trie-subset-compare trie1 trie2)
  (letrec
   ((compare
     (lambda (s t)
       (cond ((eqv? s t) 'equal)
             ((not s) 'less)
             ((not t) 'greater)
             ((and (leaf? s) (leaf? t)) (subset-compare-leaves s t))
             ((leaf? s)
              (let ((p (leaf-prefix s)))
                (let ((q (branch-prefix t))
                      (m (branch-branching-bit t))
                      (l (branch-left t))
                      (r (branch-right t)))
                  (if (match-prefix? p q m)
                      (case (compare s (if (zero-bit? p m) l r))
                        ((greater) 'greater)
                        (else 'less)
                      ) ;case
                      'greater
                  ) ;if
                ) ;let
              ) ;let
             ) ;
             ((leaf? t) 'greater)
             (else (compare-branches s t)))
       ) ;cond
     ) ;lambda
    (compare-branches
     (lambda (s t)
       (let ((p (branch-prefix s))
             (m (branch-branching-bit s))
             (sl (branch-left s))
             (sr (branch-right s))
             (q (branch-prefix t))
             (n (branch-branching-bit t))
             (tl (branch-left t))
             (tr (branch-right t)))
         (cond ((branching-bit-higher? m n) 'greater)
               ((branching-bit-higher? n m)
                (if (match-prefix? p q n)
                    (let ((comp (if (zero-bit? p n)
                                    (compare s tl)
                                    (compare s tr))))
                      (if (eqv? comp 'greater) comp 'less)
                    ) ;let
                    'greater
                ) ;if
               ) ;
               ((= p q)
                (let ((cl (compare sl tl)) (cr (compare sr tr)))
                  (cond
                    ((or (eqv? cl 'greater) (eqv? cr 'greater))
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

(define (trie-proper-subset? trie1 trie2)
  (eqv? (trie-subset-compare trie1 trie2) 'less)
) ;define

(define (trie-disjoint? trie1 trie2)
  (letrec
   ((disjoint?
     (lambda (s t)
       (or (not s)
           (not t)
           (cond ((and (leaf? s) (leaf? t)) (disjoint/leaf? s t))
                 ((leaf? s) (disjoint/leaf? s t))
                 ((leaf? t) (disjoint/leaf? t s))
                 (else (branches-disjoint? s t)))
           ) ;cond
       ) ;or
     ) ;lambda
    (disjoint/leaf?
     (lambda (lf t)
       (let ((p (leaf-prefix lf))
             (bm (leaf-bitmap lf)))
         (let lp ((t t))
           (if (leaf? t)
               (if (= p (leaf-prefix t))
                   (zero? (logand bm (leaf-bitmap t)))
                   #t
               ) ;if
               (let ((q (branch-prefix t))
                     (m (branch-branching-bit t))
                     (l (branch-left t))
                     (r (branch-right t)))
                 (if (match-prefix? p q m)
                     (if (zero-bit? p m) (lp l) (lp r))
                     #t
                 ) ;if
               ) ;let
           ) ;if
         ) ;let
       ) ;let
     ) ;lambda
    ) ;disjoint/leaf?
    (branches-disjoint?
     (lambda (s t)
       (let ((p (branch-prefix s))
             (m (branch-branching-bit s))
             (sl (branch-left s))
             (sr (branch-right s))
             (q (branch-prefix t))
             (n (branch-branching-bit t))
             (tl (branch-left t))
             (tr (branch-right t)))
         (cond
           ((and (= m n) (= p q))
            (and (disjoint? sl tl) (disjoint? sr tr))
           ) ;
           ((and (branching-bit-higher? m n) (match-prefix? q p m))
            (if (zero-bit? q m)
                (disjoint? sl t)
                (disjoint? sr t)
            ) ;if
           ) ;
           ((and (branching-bit-higher? n m) (match-prefix? p q n))
            (if (zero-bit? p n)
                (disjoint? s tl)
                (disjoint? s tr)
            ) ;if
           ) ;
           (else #t)
         ) ;cond
       ) ;let
     ) ;lambda
    ) ;branches-disjoint?
   ) ;
    (disjoint? trie1 trie2)
  ) ;letrec
) ;define

;;;; Subtrie operations

(define (subtrie< trie k inclusive)
  (letrec
    ((split
      (lambda (t)
        (cond ((not t) #f)
              ((leaf? t)
               (let ((p (leaf-prefix t))
                     (bm (leaf-bitmap t)))
                 (leaf p (bitmap-split< k inclusive p bm))
               ) ;let
              ) ;
              (else
               (let ((p (branch-prefix t))
                     (m (branch-branching-bit t))
                     (l (branch-left t))
                     (r (branch-right t)))
                 (if (match-prefix? k p m)
                     (if (zero-bit? k m)
                         (split l)
                         (trie-union l (split r))
                     ) ;if
                     (and (< p k) t))
                 ) ;if
               ) ;let
              ) ;else
        ) ;cond
      ) ;lambda
     ) ;split
    (if (and (branch? trie) (negative? (branch-branching-bit trie)))
        (if (negative? k)
            (split (branch-right trie))
            (trie-union (split (branch-left trie)) (branch-right trie))
        ) ;if
        (split trie)
    ) ;if
  ) ;letrec
) ;define

(define (bitmap-split< k inclusive prefix bitmap)
  (let ((kp (iprefix k)) (kb (ibitmap k)))
    (cond ((> kp prefix) bitmap)
          ((= kp prefix)
           (logand bitmap
                   (- (if inclusive
                          (arithmetic-shift kb 1)
                          kb)
                      1
                   ) ;-
           ) ;logand
          ) ;
          (else 0)
    ) ;cond
  ) ;let
) ;define

(define (subtrie> trie k inclusive)
  (letrec
   ((split
     (lambda (t)
       (cond ((not t) #f)
             ((leaf? t)
              (let ((p (leaf-prefix t))
                    (bm (leaf-bitmap t)))
                (leaf p (bitmap-split> k inclusive p bm))
              ) ;let
             ) ;
             (else
              (let ((p (branch-prefix t))
                    (m (branch-branching-bit t))
                    (l (branch-left t))
                    (r (branch-right t)))
                (if (match-prefix? k p m)
                    (if (zero-bit? k m)
                        (trie-union (split l) r)
                        (split r)
                    ) ;if
                    (and (> p k) t))
                ) ;if
              ) ;let
             ) ;else
       ) ;cond
     ) ;lambda
    ) ;split
    (if (and (branch? trie) (negative? (branch-branching-bit trie)))
        (if (negative? k)
            (trie-union (split (branch-right trie)) (branch-left trie))
            (split (branch-left trie))
        ) ;if
        (split trie)
    ) ;if
  ) ;letrec
) ;define

(define (bitmap-split> k inclusive prefix bitmap)
  (let ((kp (iprefix k)) (kb (ibitmap k)))
    (cond ((< kp prefix) bitmap)
          ((= kp prefix)
           (logand bitmap
                   (- (if inclusive
                          kb
                          (arithmetic-shift kb 1))
                   ) ;-
           ) ;logand
          ) ;
          (else 0)
    ) ;cond
  ) ;let
) ;define

(define (subtrie-interval trie a b low-inclusive high-inclusive)
  (letrec
   ((interval
     (lambda (t)
       (cond ((not t) #f)
             ((leaf? t)
              (let ((p (leaf-prefix t))
                    (bm (leaf-bitmap t)))
                (leaf p
                      (bitmap-interval p bm a b low-inclusive high-inclusive)
                ) ;leaf
              ) ;let
             ) ;
             (else (branch-interval t)))
       ) ;cond
     ) ;lambda
    (branch-interval
     (lambda (t)
       (let ((p (branch-prefix t))
             (m (branch-branching-bit t))
             (l (branch-left t))
             (r (branch-right t)))
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
                          (trie-union (subtrie> l a low-inclusive) r)
                     ) ;and
                 ) ;if
                 (interval r)
             ) ;if
             (and (> p a) (subtrie< t b high-inclusive))
         ) ;if
       ) ;let
     ) ;lambda
    ) ;branch-interval
   ) ;
    (if (and (branch? trie) (negative? (branch-branching-bit trie)))
        (cond
          ((and (negative? a) (negative? b))
           (interval (branch-right trie))
          ) ;
          ((and (positive? a) (positive? b))
           (interval (branch-left trie))
          ) ;
          (else
           (trie-union
            (subtrie> (branch-right trie) a low-inclusive)
            (subtrie< (branch-left trie) b high-inclusive)
           ) ;trie-union
          ) ;else
        ) ;cond
        (interval trie)
    ) ;if
  ) ;letrec
) ;define

(define (bitmap-interval prefix bitmap low high low-inclusive high-inclusive)
  (let ((lp (iprefix low))
        (lb (ibitmap low))
        (hp (iprefix high))
        (hb (ibitmap high)))
    (let
      ((low-mask
         (- (if low-inclusive
                lb
                (arithmetic-shift lb 1))
         ) ;-
       ) ;low-mask
       (high-mask
         (- (if high-inclusive
                (arithmetic-shift hb 1)
                hb)
             1)
         ) ;-
       ) ;high-mask
      (cond
        ((< prefix hp)
         (cond ((< prefix lp) 0)
               ((> prefix lp) bitmap)
               (else (logand low-mask bitmap))
         ) ;cond
        ) ;
        ((> prefix hp) 0)
        (else (logand (logand low-mask high-mask) bitmap))
      ) ;cond
    ) ;let
  ) ;let
) ;define

;;;; ISet record type

(define-record-type <iset>
  (raw-iset trie)
  iset?
  (trie iset-trie)
) ;define-record-type

;;;; Constructors

(define (iset . args)
  (list->iset args)
) ;define

(define (pair-or-null? x)
  (or (pair? x) (null? x))
) ;define

(define (list->iset ns)
  (assume (pair-or-null? ns))
  (raw-iset
   (fold (lambda (n t)
           (assume (valid-integer? n))
           (trie-insert t n))
         #f
         ns
   ) ;fold
  ) ;raw-iset
) ;define

(define (list->iset! set ns)
  (assume (iset? set))
  (assume (pair-or-null? ns))
  (raw-iset (fold (lambda (n t)
                    (assume (valid-integer? n))
                    (trie-insert t n))
                  (iset-trie set)
                  ns)
  ) ;raw-iset
) ;define

(define (iset-unfold stop? mapper successor seed)
  (assume (procedure? stop?))
  (assume (procedure? mapper))
  (assume (procedure? successor))
  (let lp ((trie #f) (seed seed))
    (if (stop? seed)
        (raw-iset trie)
        (let ((n (mapper seed)))
          (assume (valid-integer? n))
          (lp (trie-insert trie n) (successor seed))
        ) ;let
    ) ;if
  ) ;let
) ;define

(define make-range-iset
  (case-lambda
    ((start end) (make-range-iset start end 1))
    ((start end step)
     (assume (valid-integer? start))
     (assume (valid-integer? end))
     (assume (valid-integer? step))
     (assume (if (< end start)
                 (negative? step)
                 (not (zero? step)))
             "Invalid step value."
     ) ;assume
     (let ((stop? (if (positive? step)
                      (lambda (i) (>= i end))
                      (lambda (i) (<= i end)))))
       (iset-unfold stop?
                    values
                    (lambda (i) (+ i step))
                    start
       ) ;iset-unfold
     ) ;let
    ) ;
  ) ;case-lambda
) ;define

;;;; Predicates

(define (iset-contains? set n)
  (assume (iset? set))
  (assume (valid-integer? n))
  (trie-contains? (iset-trie set) n)
) ;define

(define (iset-empty? set)
  (assume (iset? set))
  (not (iset-trie set))
) ;define

(define (iset-disjoint? set1 set2)
  (assume (iset? set1))
  (assume (iset? set2))
  (trie-disjoint? (iset-trie set1) (iset-trie set2))
) ;define

;;;; Accessors

(define (iset-member set elt default)
  (if (iset-contains? set elt)
      elt
      default
  ) ;if
) ;define

(define (iset-min set)
  (assume (iset? set))
  (trie-min (iset-trie set))
) ;define

(define (iset-max set)
  (assume (iset? set))
  (trie-max (iset-trie set))
) ;define

;;;; Updaters

(define iset-adjoin
  (case-lambda
    ((set n)
     (assume (iset? set))
     (assume (valid-integer? n))
     (raw-iset (trie-insert (iset-trie set) n))
    ) ;
    ((set . ns)
     (raw-iset
      (fold (lambda (n t)
              (assume (valid-integer? n))
              (trie-insert t n))
            (iset-trie set)
            ns
      ) ;fold
     ) ;raw-iset
    ) ;
  ) ;case-lambda
) ;define

(define (iset-adjoin! set . ns)
  (apply iset-adjoin set ns)
) ;define

(define iset-delete
  (case-lambda
    ((set n)
     (assume (iset? set))
     (assume (valid-integer? n))
     (raw-iset (trie-delete (iset-trie set) n))
    ) ;
    ((set . ns) (iset-delete-all set ns))
  ) ;case-lambda
) ;define

(define (iset-delete! set n) (iset-delete set n))

(define (iset-delete-all set ns)
  (assume (iset? set))
  (assume (or (pair? ns) (null? ns)))
  (iset-difference set (list->iset ns))
) ;define

(define (iset-delete-all! set ns)
  (iset-delete-all set ns)
) ;define

(define (iset-search set elt failure success)
  (assume (iset? set))
  (assume (valid-integer? elt))
  (assume (procedure? failure))
  (assume (procedure? success))
  (call-with-current-continuation
   (lambda (return)
     (let-values
      (((trie obj)
        (trie-search (iset-trie set)
                     elt
                     (lambda (insert ignore)
                       (failure insert
                                (lambda (obj)
                                  (return set obj)
                                ) ;lambda
                       ) ;failure
                     ) ;lambda
                     (lambda (key update remove)
                       (success
                        key
                        (lambda (new obj)
                          (assume (valid-integer? new))
                          (if (= key new)
                              (update new obj)
                              (return (iset-adjoin (iset-delete set key)
                                                   new)
                                      obj
                              ) ;return
                          ) ;if
                        ) ;lambda
                        remove)
                       ) ;success
                     ) ;lambda
        ) ;trie-search
      ) ;
       (values (raw-iset trie) obj)
     ) ;let-values
   ) ;lambda
  ) ;call-with-current-continuation
) ;define

(define (iset-search! set elt failure success)
  (iset-search set elt failure success)
) ;define

(define (iset-delete-min set)
  (assume (iset? set))
  (let ((trie (iset-trie set)))
    (let-values (((n trie*) (trie-delete-min trie)))
      (values n (raw-iset trie*))
    ) ;let-values
  ) ;let
) ;define

(define (iset-delete-max set)
  (assume (iset? set))
  (let ((trie (iset-trie set)))
    (let-values (((n trie*) (trie-delete-max trie)))
      (values n (raw-iset trie*))
    ) ;let-values
  ) ;let
) ;define

(define (iset-delete-min! set) (iset-delete-min set))
(define (iset-delete-max! set) (iset-delete-max set))

;;;; The whole iset

(define (iset-size set)
  (assume (iset? set))
  (trie-size (iset-trie set))
) ;define

(define (iset-find pred set failure)
  (assume (procedure? failure))
  (call-with-current-continuation
   (lambda (return)
     (or (iset-fold (lambda (n _)
                      (and (pred n) (return n)))
                    #f
                    set)
         (failure)
     ) ;or
   ) ;lambda
  ) ;call-with-current-continuation
) ;define

(define (iset-count pred set)
  (assume (procedure? pred))
  (iset-fold (lambda (n acc)
               (if (pred n) (+ 1 acc) acc))
             0
             set
  ) ;iset-fold
) ;define

(define (iset-any? pred set)
  (assume (procedure? pred))
  (call-with-current-continuation
   (lambda (return)
     (iset-fold (lambda (n _)
                  (and (pred n) (return #t)))
                #f
                set
     ) ;iset-fold
   ) ;lambda
  ) ;call-with-current-continuation
) ;define

(define (iset-every? pred set)
  (assume (procedure? pred))
  (call-with-current-continuation
   (lambda (return)
     (iset-fold (lambda (n _)
                  (if (pred n) #t (return #f)))
                #t
                set
     ) ;iset-fold
   ) ;lambda
  ) ;call-with-current-continuation
) ;define

;;;; Mapping and folding

(define (iset-map proc set)
  (assume (procedure? proc))
  (raw-iset
   (iset-fold (lambda (n t)
                (let ((n* (proc n)))
                  (assume (valid-integer? n*))
                  (trie-insert t (proc n)))
                ) ;let
              #f
              set
   ) ;iset-fold
  ) ;raw-iset
) ;define

(define (unspecified)
  (if #f #f)
) ;define

(define (iset-for-each proc set)
  (assume (procedure? proc))
  (iset-fold (lambda (n _)
               (proc n)
               (unspecified))
             (unspecified)
             set
  ) ;iset-fold
) ;define

(define (iset-fold proc nil set)
  (assume (procedure? proc))
  (assume (iset? set))
  (trie-fold proc nil (iset-trie set))
) ;define

(define (iset-fold-right proc nil set)
  (assume (procedure? proc))
  (assume (iset? set))
  (trie-fold-right proc nil (iset-trie set))
) ;define

(define (iset-filter pred set)
  (assume (procedure? pred))
  (assume (iset? set))
  (raw-iset (trie-filter pred (iset-trie set)))
) ;define

(define (iset-remove pred set)
  (assume (procedure? pred))
  (assume (iset? set))
  (raw-iset (trie-filter (lambda (n) (not (pred n))) (iset-trie set)))
) ;define

(define (iset-partition pred set)
  (assume (procedure? pred))
  (assume (iset? set))
  (let-values (((tin tout) (trie-partition pred (iset-trie set))))
    (values (raw-iset tin) (raw-iset tout))
  ) ;let-values
) ;define

(define (iset-partition! pred set)
  (iset-partition pred set)
) ;define

(define (iset-filter! pred set)
  (iset-filter pred set)
) ;define

(define (iset-remove! pred set)
  (iset-remove pred set)
) ;define

;;;; Copying and conversion

(define (iset-copy set)
  (assume (iset? set))
  (raw-iset (copy-trie (iset-trie set)))
) ;define

(define (iset->list set)
  (iset-fold-right cons '() set)
) ;define

;;;; Comparison

(define (iset=? set1 set2 . sets)
  (assume (iset? set1))
  (let
    ((iset-eq1 (lambda (set)
                 (assume (iset? set))
                 (or (eqv? set1 set)
                     (trie=? (iset-trie set1) (iset-trie set))))
                 ) ;or
     ) ;iset-eq1
    (and (iset-eq1 set2)
         (or (null? sets)
             (every iset-eq1 sets)
         ) ;or
    ) ;and
  ) ;let
) ;define

(define (iset<? set1 set2 . sets)
  (assume (iset? set1))
  (assume (iset? set2))
  (let lp ((t1 (iset-trie set1)) (t2 (iset-trie set2)) (sets sets))
    (and (trie-proper-subset? t1 t2)
         (or (null? sets)
             (lp t2 (iset-trie (car sets)) (cdr sets))
         ) ;or
    ) ;and
  ) ;let
) ;define

(define (iset>? set1 set2 . sets)
  (assume (iset? set1))
  (assume (iset? set2))
  (let lp ((t1 (iset-trie set1)) (t2 (iset-trie set2)) (sets sets))
    (and (trie-proper-subset? t2 t1)
         (or (null? sets)
             (lp t2 (iset-trie (car sets)) (cdr sets))
         ) ;or
    ) ;and
  ) ;let
) ;define

(define (iset<=? set1 set2 . sets)
  (assume (iset? set1))
  (assume (iset? set2))
  (let lp ((t1 (iset-trie set1)) (t2 (iset-trie set2)) (sets sets))
    (and (memv (trie-subset-compare t1 t2) '(less equal))
         (or (null? sets)
             (lp t2 (iset-trie (car sets)) (cdr sets))
         ) ;or
    ) ;and
  ) ;let
) ;define

(define (iset>=? set1 set2 . sets)
     (assume (iset? set1))
     (assume (iset? set2))
     (let lp ((t1 (iset-trie set1)) (t2 (iset-trie set2)) (sets sets))
       (and (memv (trie-subset-compare t1 t2) '(greater equal))
            (or (null? sets)
                (lp t2 (iset-trie (car sets)) (cdr sets))
            ) ;or
       ) ;and
     ) ;let
) ;define

;;;; Set theory operations

(define iset-union
  (case-lambda
    ((set1 set2)
     (assume (iset? set1))
     (assume (iset? set2))
     (raw-iset (trie-union (iset-trie set1) (iset-trie set2)))
    ) ;
    ((set . rest)
     (raw-iset (fold (lambda (s t)
                       (assume (iset? s))
                       (trie-union (iset-trie s) t))
                     (iset-trie set)
                     rest)
     ) ;raw-iset
    ) ;
  ) ;case-lambda
) ;define

(define (iset-union! set . rest)
  (apply iset-union set rest)
) ;define

(define iset-intersection
  (case-lambda
    ((set1 set2)
     (assume (iset? set1))
     (assume (iset? set2))
     (raw-iset (trie-intersection (iset-trie set1) (iset-trie set2)))
    ) ;
    ((set . rest)
     (assume (iset? set))
     (raw-iset (fold (lambda (s t)
                       (assume (iset? s))
                       (trie-intersection (iset-trie s) t))
               (iset-trie set)
               rest)
     ) ;raw-iset
    ) ;
  ) ;case-lambda
) ;define

(define (iset-intersection! set . rest)
  (apply iset-intersection set rest)
) ;define

(define iset-difference
  (case-lambda
    ((set1 set2)
     (assume (iset? set1))
     (assume (iset? set2))
     (raw-iset (trie-difference (iset-trie set1) (iset-trie set2)))
    ) ;
    ((set . rest)
     (assume (iset? set))
     (raw-iset
      (trie-difference (iset-trie set)
                       (iset-trie (apply iset-union rest))
      ) ;trie-difference
     ) ;raw-iset
    ) ;
  ) ;case-lambda
) ;define

(define (iset-difference! set . rest)
  (apply iset-difference set rest)
) ;define

(define (iset-xor set1 set2)
  (assume (iset? set1))
  (assume (iset? set2))
  (if (eqv? set1 set2)
      (iset)
      (raw-iset
       (trie-xor (iset-trie set1) (iset-trie set2))
      ) ;raw-iset
  ) ;if
) ;define

(define (iset-xor! set1 set2) (iset-xor set1 set2))

;;;; Subsets

(define (isubset= set k)
  (if (iset-contains? set k) (iset k) (iset))
) ;define

(define (iset-open-interval set low high)
  (assume (valid-integer? low))
  (assume (valid-integer? high))
  (assume (>= high low))
  (raw-iset (subtrie-interval (iset-trie set) low high #f #f))
) ;define

(define (iset-closed-interval set low high)
  (assume (valid-integer? low))
  (assume (valid-integer? high))
  (assume (>= high low))
  (raw-iset (subtrie-interval (iset-trie set) low high #t #t))
) ;define

(define (iset-open-closed-interval set low high)
  (assume (valid-integer? low))
  (assume (valid-integer? high))
  (assume (>= high low))
  (raw-iset (subtrie-interval (iset-trie set) low high #f #t))
) ;define

(define (iset-closed-open-interval set low high)
  (assume (valid-integer? low))
  (assume (valid-integer? high))
  (assume (>= high low))
  (raw-iset (subtrie-interval (iset-trie set) low high #t #f))
) ;define

(define (isubset< set k)
  (assume (iset? set))
  (assume (valid-integer? k))
  (raw-iset (subtrie< (iset-trie set) k #f))
) ;define

(define (isubset<= set k)
  (assume (iset? set))
  (assume (valid-integer? k))
  (raw-iset (subtrie< (iset-trie set) k #t))
) ;define

(define (isubset> set k)
  (assume (iset? set))
  (assume (valid-integer? k))
  (raw-iset (subtrie> (iset-trie set) k #f))
) ;define

(define (isubset>= set k)
  (assume (iset? set))
  (assume (valid-integer? k))
  (raw-iset (subtrie> (iset-trie set) k #t))
) ;define

) ;define-library
