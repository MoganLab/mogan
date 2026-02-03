;;;; SPDX-FileCopyrightText: 2013 John Cowan <cowan@ccil.org>
;;;;
;;;; SPDX-License-Identifier: MIT
;
; Permission is hereby granted, free of charge, to any person obtaining a copy of
; this software and associated documentation files (the "Software"), to deal in
; the Software without restriction, including without limitation the rights to
; use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
; of the Software, and to permit persons to whom the Software is furnished to do
; so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;

(define-library (srfi srfi-113)
  (import (scheme base)
          (scheme case-lambda)
          (liii hash-table)
          (liii error)
          (srfi srfi-1)
          (srfi srfi-128))
  (export set set-unfold list->set list->set! set-copy set->list
          set? set-contains? set-empty? set-disjoint?
          set-element-comparator set-size
          set=? set<? set>? set<=? set>=?
          set-any? set-every? set-find set-count set-member set-search! set-map
          set-for-each set-fold set-filter set-filter! set-remove set-remove!
          set-partition set-partition! set-union set-intersection set-difference set-xor
          set-union! set-intersection! set-difference! set-xor!
          set-adjoin set-adjoin! set-replace set-replace!
          set-delete set-delete! set-delete-all set-delete-all!
          bag bag-unfold bag-member bag-comparator bag->list
          bag-copy list->bag list->bag!
          bag? bag-contains? bag-empty? bag-disjoint?
          bag-size bag-find bag-count bag-any? bag-every?
          bag=? bag<? bag>? bag<=? bag>=?
          bag-union bag-intersection bag-difference bag-xor
          bag-union! bag-intersection! bag-difference! bag-xor!
          bag-adjoin bag-adjoin! bag-replace bag-replace!
          bag-delete bag-delete! bag-delete-all bag-delete-all!
          bag-search!)
  (begin

    (define-record-type set-impl
      (%make-set hash-table comparator) 
      set?                             
      (hash-table set-hash-table)      
      (comparator set-element-comparator)) 

    (define (check-set obj)
      (if (not (set? obj)) (type-error "not a set" obj)))

    (define (check-same-comparator a b)
      (if (not (eq? (set-element-comparator a) (set-element-comparator b)))
          (value-error "different comparators" a b)))

    (define (make-set/comparator comparator)
      (%make-set (make-hash-table comparator) comparator))

    (define (set-add! s element)
      (hash-table-set! (set-hash-table s) element element))

    (define (set comparator . elements)
      (let ((result (make-set/comparator comparator)))
        (for-each (lambda (x) (set-add! result x)) elements)
        result))
    
    (define (list->set comparator elements)
      (apply set comparator elements))

    (define (list->set! s elements)
      (check-set s)
      (for-each (lambda (x) (set-add! s x)) elements)
      s)

    (define (set-unfold stop? mapper successor seed comparator)
      (let ((result (make-set/comparator comparator)))
        (let loop ((seed seed))
          (if (stop? seed)
              result
              (begin
                (set-add! result (mapper seed))
                (loop (successor seed)))))))
    
    (define (set-copy s)
      (check-set s)
      (list->set (set-element-comparator s) (hash-table-keys (set-hash-table s))))

    (define (set->list s)
      (check-set s)
      (hash-table-keys (set-hash-table s)))

    
    (define (set-size s)
      (check-set s)
      (hash-table-size (set-hash-table s)))


    (define (set-contains? s member)
      (check-set s)
      (hash-table-contains? (set-hash-table s) member))

    (define (set-empty? s)
      (check-set s)
      (hash-table-empty? (set-hash-table s)))

    (define (set-disjoint? a b)
      (check-set a)
      (check-set b)
      (check-same-comparator a b)
      (let ((na (set-size a))
            (nb (set-size b)))
        (if (< na nb)
            (not (any-in-other? a b))
            (not (any-in-other? b a)))))
            
    (define (any-in-other? small big)
      (let ((ht-small (set-hash-table small))
            (ht-big (set-hash-table big)))
         (call/cc (lambda (return)
            (hash-table-for-each
               (lambda (k v)
                  (if (hash-table-contains? ht-big k) (return #t)))
               ht-small)
            #f))))


    (define (binary-set<=? s1 s2)
      (check-set s1)
      (check-set s2)
      (check-same-comparator s1 s2)
      (let ((n1 (set-size s1))
            (n2 (set-size s2)))
        (cond
          ((> n1 n2) #f)
          (else
           (let ((ht1 (set-hash-table s1))
                 (ht2 (set-hash-table s2)))
             (call/cc
              (lambda (return)
                (hash-table-for-each
                 (lambda (k v)
                   (unless (hash-table-contains? ht2 k)
                     (return #f)))
                 ht1)
                #t)))))))
                
    (define (set<=? . sets)
      (if (null? sets)
          #t
          (let loop ((head (car sets)) (tail (cdr sets)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-set<=? head next)
                       (loop next (cdr tail))))))))
                       
    (define (set=? . sets)
      (if (null? sets)
          #t
          (let loop ((head (car sets)) (tail (cdr sets)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-set=? head next)
                       (loop next (cdr tail))))))))
                       
    (define (binary-set=? s1 s2)
       (check-set s1)
       (check-set s2)
       (check-same-comparator s1 s2)
       (let ((n1 (set-size s1))
             (n2 (set-size s2)))
         (and (= n1 n2)
              (binary-set<=? s1 s2))))
              
    (define (set<? . sets)
      (if (null? sets)
          #t
          (let loop ((head (car sets)) (tail (cdr sets)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-set<? head next)
                       (loop next (cdr tail))))))))

    (define (binary-set<? s1 s2)
      (check-set s1)
      (check-set s2)
      (check-same-comparator s1 s2)
      (and (< (set-size s1) (set-size s2))
           (binary-set<=? s1 s2)))

    (define (set>=? . sets)
      (if (null? sets)
          #t
          (let loop ((head (car sets)) (tail (cdr sets)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-set<=? next head)
                       (loop next (cdr tail))))))))

    (define (set>? . sets)
      (if (null? sets)
          #t
          (let loop ((head (car sets)) (tail (cdr sets)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-set<? next head)
                       (loop next (cdr tail))))))))

    (define (set-any? predicate set)
      (check-set set)
      (let ((ht (set-hash-table set)))
        (call/cc
         (lambda (return)
           (hash-table-for-each
            (lambda (k v)
              (if (predicate k) (return #t)))
            ht)
           #f))))

    (define (set-every? predicate set)
      (check-set set)
      (let ((ht (set-hash-table set)))
        (call/cc
         (lambda (return)
           (hash-table-for-each
            (lambda (k v)
              (if (not (predicate k)) (return #f)))
            ht)
           #t))))

    (define (set-find predicate set failure)
      (check-set set)
      (let ((ht (set-hash-table set)))
        (call/cc
         (lambda (return)
           (hash-table-for-each
            (lambda (k v)
              (if (predicate k) (return k)))
            ht)
           (failure)))))

    (define (set-count predicate set)
      (check-set set)
      (let ((ht (set-hash-table set))
            (count 0))
        (hash-table-for-each
         (lambda (k v)
           (if (predicate k) (set! count (+ count 1))))
         ht)
        count))

    (define (set-member set element default)
      (check-set set)
      (hash-table-ref/default (set-hash-table set) element default))

    (define (set-search! set element failure success)
      (check-set set)
      (let* ((ht (set-hash-table set))
             (not-found (list 'not-found))
             (found (hash-table-ref/default ht element not-found)))
        (if (eq? found not-found)
            (failure (lambda (obj)
                       (set-add! set element)
                       (values set obj))
                     (lambda (obj)
                       (values set obj)))
            (success found
                     (lambda (new-element obj)
                       (hash-table-delete! ht found)
                       (set-add! set new-element)
                       (values set obj))
                     (lambda (obj)
                       (hash-table-delete! ht found)
                       (values set obj))))))

    (define (set-map comparator proc set)
      (check-set set)
      (let ((result (make-set/comparator comparator))
            (ht (set-hash-table set)))
        (hash-table-for-each
         (lambda (k v)
           (set-add! result (proc k)))
         ht)
        result))

    (define (set-for-each proc set)
      (check-set set)
      (hash-table-for-each
       (lambda (k v)
         (proc k))
       (set-hash-table set))
      (if #f #f))

    (define (set-fold proc nil set)
      (check-set set)
      (let ((result nil))
        (hash-table-for-each
         (lambda (k v)
           (set! result (proc k result)))
         (set-hash-table set))
        result))

    (define (set-filter predicate set)
      (check-set set)
      (let ((result (make-set/comparator (set-element-comparator set)))
            (ht (set-hash-table set)))
        (hash-table-for-each
         (lambda (k v)
           (when (predicate k)
             (set-add! result k)))
         ht)
        result))

    (define (set-filter! predicate set)
      (check-set set)
      (let ((ht (set-hash-table set)))
        (hash-table-for-each
          (lambda (k v)
           (unless (predicate k)
             (hash-table-delete! ht k)))
          ht)
        set))

    (define (set-remove predicate set)
      (check-set set)
      (let ((result (make-set/comparator (set-element-comparator set)))
            (ht (set-hash-table set)))
        (hash-table-for-each
         (lambda (k v)
           (unless (predicate k)
             (set-add! result k)))
         ht)
        result))

    (define (set-remove! predicate set)
      (check-set set)
      (let ((ht (set-hash-table set)))
        (hash-table-for-each
         (lambda (k v)
           (when (predicate k)
             (hash-table-delete! ht k)))
         ht)
        set))

    (define (set-partition predicate set)
      (check-set set)
      (let ((yes (make-set/comparator (set-element-comparator set)))
            (no (make-set/comparator (set-element-comparator set)))
            (ht (set-hash-table set)))
        (hash-table-for-each
         (lambda (k v)
           (if (predicate k)
               (set-add! yes k)
               (set-add! no k)))
         ht)
        (values yes no)))

    (define (set-partition! predicate set)
      (check-set set)
      (let ((ht (set-hash-table set))
            (removed (make-set/comparator (set-element-comparator set))))
        (hash-table-for-each
         (lambda (k v)
           (unless (predicate k)
             (set-add! removed k)
             (hash-table-delete! ht k)))
         ht)
        (values set removed)))

    (define (set-union set1 . sets)
      (check-set set1)
      (let* ((result (set-copy set1))
             (ht-result (set-hash-table result)))
        (for-each
         (lambda (s)
           (check-set s)
           (check-same-comparator set1 s)
           (hash-table-for-each
            (lambda (k v)
              (unless (hash-table-contains? ht-result k)
                (set-add! result k)))
            (set-hash-table s)))
         sets)
        result))

    (define (set-intersection set1 . sets)
      (check-set set1)
      (for-each
       (lambda (s)
         (check-set s)
         (check-same-comparator set1 s))
       sets)
      (let ((result (make-set/comparator (set-element-comparator set1)))
            (ht1 (set-hash-table set1))
            (other-hts (map set-hash-table sets)))
        (define (all-contains? key)
          (every (lambda (ht) (hash-table-contains? ht key)) other-hts))
        (hash-table-for-each
         (lambda (k v)
           (when (all-contains? k)
             (set-add! result k)))
         ht1)
        result))

    (define (set-difference set1 . sets)
      (check-set set1)
      (for-each
       (lambda (s)
         (check-set s)
         (check-same-comparator set1 s))
       sets)
      (let ((result (make-set/comparator (set-element-comparator set1)))
            (ht1 (set-hash-table set1))
            (other-hts (map set-hash-table sets)))
        (define (any-contains? key)
          (any (lambda (ht) (hash-table-contains? ht key)) other-hts))
        (hash-table-for-each
         (lambda (k v)
           (unless (any-contains? k)
             (set-add! result k)))
         ht1)
        result))

    (define (set-xor set1 set2)
      (check-set set1)
      (check-set set2)
      (check-same-comparator set1 set2)
      (let ((result (make-set/comparator (set-element-comparator set1)))
            (ht1 (set-hash-table set1))
            (ht2 (set-hash-table set2)))
        (hash-table-for-each
         (lambda (k v)
           (unless (hash-table-contains? ht2 k)
             (set-add! result k)))
         ht1)
        (hash-table-for-each
         (lambda (k v)
           (unless (hash-table-contains? ht1 k)
             (set-add! result k)))
         ht2)
        result))

    (define (set-union! set1 . sets)
      (check-set set1)
      (let ((ht1 (set-hash-table set1)))
        (for-each
         (lambda (s)
           (check-set s)
           (check-same-comparator set1 s)
           (hash-table-for-each
            (lambda (k v)
              (unless (hash-table-contains? ht1 k)
                (set-add! set1 k)))
            (set-hash-table s)))
         sets)
        set1))

    (define (set-intersection! set1 . sets)
      (check-set set1)
      (for-each
       (lambda (s)
         (check-set s)
         (check-same-comparator set1 s))
       sets)
      (let ((ht1 (set-hash-table set1))
            (other-hts (map set-hash-table sets)))
        (define (all-contains? key)
          (every (lambda (ht) (hash-table-contains? ht key)) other-hts))
        (hash-table-for-each
         (lambda (k v)
           (unless (all-contains? k)
             (hash-table-delete! ht1 k)))
         ht1)
        set1))

    (define (set-difference! set1 . sets)
      (check-set set1)
      (for-each
       (lambda (s)
         (check-set s)
         (check-same-comparator set1 s))
       sets)
      (let ((ht1 (set-hash-table set1))
            (other-hts (map set-hash-table sets)))
        (define (any-contains? key)
          (any (lambda (ht) (hash-table-contains? ht key)) other-hts))
        (hash-table-for-each
         (lambda (k v)
           (when (any-contains? k)
             (hash-table-delete! ht1 k)))
         ht1)
        set1))

    (define (set-xor! set1 set2)
      (check-set set1)
      (check-set set2)
      (check-same-comparator set1 set2)
      (let ((ht1 (set-hash-table set1))
            (ht2 (set-hash-table set2)))
        (hash-table-for-each
         (lambda (k v)
           (if (hash-table-contains? ht1 k)
               (hash-table-delete! ht1 k)
               (set-add! set1 k)))
         ht2)
        set1))

    (define (set-adjoin set . elements)
      (check-set set)
      (let ((new-set (set-copy set)))
        (for-each (lambda (x) (set-add! new-set x)) elements)
        new-set))

    (define (set-adjoin! set . elements)
      (check-set set)
      (for-each (lambda (x) (set-add! set x)) elements)
      set)

    (define (set-replace set element)
      (check-set set)
      (if (set-contains? set element)
          (let ((new-set (set-copy set)))
            (hash-table-delete! (set-hash-table new-set) element)
            (set-add! new-set element)
            new-set)
          set))

    (define (set-replace! set element)
      (check-set set)
      (when (set-contains? set element)
        (hash-table-delete! (set-hash-table set) element)
        (set-add! set element))
      set)

    (define (set-delete! set . elements)
      (check-set set)
      (for-each (lambda (x) (hash-table-delete! (set-hash-table set) x)) elements)
      set)

    (define (set-delete set . elements)
      (apply set-delete! (set-copy set) elements))

    (define (set-delete-all! set element-list)
      (apply set-delete! set element-list))

    (define (set-delete-all set element-list)
      (apply set-delete set element-list))

    (define-record-type bag-impl
      (%make-bag entries comparator)
      bag?
      (entries bag-entries set-bag-entries!)
      (comparator bag-comparator))

    (define (check-bag obj)
      (when (not (bag? obj)) (type-error "not a bag" obj)))

    (define (check-same-bag-comparator a b)
      (if (not (eq? (bag-comparator a) (bag-comparator b)))
          (value-error "different comparators" a b)))

    (define (make-bag/comparator comparator)
      (if (comparator? comparator)
          (%make-bag (make-hash-table comparator) comparator)
          (type-error "make-bag/comparator")))

    (define (bag-increment! bag element count)
      (check-bag bag)
      (unless (and (exact-integer? count) (>= count 0))
          (type-error "bag-increment!" count))
      (if (= count 0)
          bag
          (let* ((entries (bag-entries bag))
                 (entry (hash-table-ref/default entries element 0)))
            (hash-table-set! entries element (+ count entry))
            bag)))

    (define (bag-decrement! bag element count)
      (check-bag bag)
      (if (not (and (exact-integer? count) (>= count 0)))
          (type-error "bag-decrement!" count))
      (if (= count 0)
          bag
          (let* ((entries (bag-entries bag))
                 (entry (hash-table-ref/default entries element 0)))
              (if (> entry count)
                  (hash-table-set! entries element (- entry count))
                  (hash-table-delete! entries element))
            bag)))

    (define (bag-contains? bag element)
      (check-bag bag)
      (hash-table-contains? (bag-entries bag) element))

    (define (bag-empty? bag)
      (check-bag bag)
      (hash-table-empty? (bag-entries bag)))

    (define (bag-disjoint? a b)
      (check-bag a)
      (check-bag b)
      (let ((entries-a (bag-entries a))
            (entries-b (bag-entries b)))
        (call/cc
          (lambda (return)
            (hash-table-for-each
             (lambda (k entry)
               (when (hash-table-contains? entries-b k)
                 (return #f)))
             entries-a)
            #t))))

    (define (bag comparator . elements)
      (let ((result (make-bag/comparator comparator)))
        (for-each (lambda (x) (bag-increment! result x 1)) elements)
        result))

    (define (bag-unfold stop? mapper successor seed comparator)
      (let ((result (make-bag/comparator comparator)))
        (let loop ((seed seed))
          (if (stop? seed)
              result
              (begin
                (bag-increment! result (mapper seed) 1)
                (loop (successor seed)))))))

    (define (list->bag comparator elements)
      (apply bag comparator elements))

    (define (list->bag! bag elements)
      (check-bag bag)
      (for-each (lambda (x) (bag-increment! bag x 1)) elements)
      bag)

    (define (bag-copy bag)
      (check-bag bag)
      (let ((entries (make-hash-table (bag-comparator bag))))
        (hash-table-for-each
          (lambda (k entry)
            (hash-table-set! entries k entry))
          (bag-entries bag))
        (%make-bag entries (bag-comparator bag))))


    (define (bag-member bag element default)
      (check-bag bag)
      (if (hash-table-contains? (bag-entries bag) element)
          element
          default))

    (define (bag->list bag)
      (check-bag bag)
      (let ((result '()))
        (hash-table-for-each
         (lambda (k entry)
           (let loop ((i 0))
             (when (< i entry)
               (set! result (cons k result))
               (loop (+ i 1)))))
         (bag-entries bag))
        result))

    (define (bag-size bag)
      (bag-count (lambda (x) #t) bag))

    (define (bag-find predicate bag failure)
      (check-bag bag)
      (let ((found (find predicate (hash-table-keys (bag-entries bag)))))
        (or found (failure))))

    (define (bag-count predicate bag)
      (check-bag bag)
      (let ((entries (bag-entries bag)))
        (hash-table-fold
         (lambda (k entry acc)
           (if (predicate k)
               (+ acc entry)
               acc))
         0
         entries)))

    (define (bag-any? predicate bag)
      (check-bag bag)
      (let ((found
             (hash-table-find
              (lambda (k entry) (predicate k))
              (bag-entries bag)
              #f)))
        (if found #t #f)))

    (define (bag-every? predicate bag)
      (check-bag bag)
      (let ((found
             (hash-table-find
              (lambda (k entry) (not (predicate k)))
              (bag-entries bag)
              #f)))
        (if found #f #t)))

    (define (bag<=? . bags)
      (if (null? bags)
          #t
          (let loop ((head (car bags)) (tail (cdr bags)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-bag<=? head next)
                       (loop next (cdr tail))))))))

    (define (bag=? . bags)
      (if (null? bags)
          #t
          (let loop ((head (car bags)) (tail (cdr bags)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-bag=? head next)
                       (loop next (cdr tail))))))))

    (define (binary-bag=? b1 b2)
      (check-bag b1)
      (check-bag b2)
      (check-same-bag-comparator b1 b2)
      (let ((e1 (bag-entries b1))
            (e2 (bag-entries b2)))
        (and (= (hash-table-size e1) (hash-table-size e2))
             (call/cc
              (lambda (return)
                (hash-table-for-each
                 (lambda (k count1)
                   (if (not (= count1 (hash-table-ref/default e2 k 0)))
                       (return #f)))
                 e1)
                #t)))))

    (define (binary-bag<=? b1 b2)
      (check-bag b1)
      (check-bag b2)
      (check-same-bag-comparator b1 b2)
      (let ((e1 (bag-entries b1))
            (e2 (bag-entries b2)))
        (if (> (hash-table-size e1) (hash-table-size e2))
            #f
            (call/cc
             (lambda (return)
               (hash-table-for-each
                (lambda (k count1)
                  (if (not (<= count1 (hash-table-ref/default e2 k 0)))
                      (return #f)))
                e1)
               #t)))))

    (define (bag<? . bags)
      (if (null? bags)
          #t
          (let loop ((head (car bags)) (tail (cdr bags)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-bag<? head next)
                       (loop next (cdr tail))))))))

    (define (binary-bag<? b1 b2)
      (check-bag b1)
      (check-bag b2)
      (check-same-bag-comparator b1 b2)
      (call/cc
       (lambda (return)
         (let ((e1 (bag-entries b1))
               (e2 (bag-entries b2)))
           (let ((smaller-count
                  (cond
                   ((< (hash-table-size e1) (hash-table-size e2)) 1)
                   ((= (hash-table-size e1) (hash-table-size e2)) 0)
                   (else (return #f)))))
             (hash-table-for-each
              (lambda (k count1)
                (let ((count2 (hash-table-ref/default e2 k 0)))
                  (if (not (<= count1 count2))
                      (return #f)
                      (when (< count1 count2)
                        (set! smaller-count (+ smaller-count 1))))))
              e1)
             (positive? smaller-count))))))

    (define (bag>=? . bags)
      (if (null? bags)
          #t
          (let loop ((head (car bags)) (tail (cdr bags)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-bag<=? next head)
                       (loop next (cdr tail))))))))

    (define (bag>? . bags)
      (if (null? bags)
          #t
          (let loop ((head (car bags)) (tail (cdr bags)))
            (if (null? tail)
                #t
                (let ((next (car tail)))
                  (and (binary-bag<? next head)
                       (loop next (cdr tail))))))))

    (define (bag-union bag1 . bags)
      (check-bag bag1)
      (for-each
       (lambda (b)
         (check-bag b)
         (check-same-bag-comparator bag1 b))
       bags)
      (let ((result (bag-copy bag1)))
        (for-each (lambda (b) (bag-union-into! result b)) bags)
        result))

    (define (bag-union-into! result other)
      (let ((result-entries (bag-entries result))
            (other-entries (bag-entries other)))
        (hash-table-for-each
         (lambda (k count2)
           (let ((count1 (hash-table-ref/default result-entries k 0)))
             (when (> count2 count1)
               (hash-table-set! result-entries k count2))))
         other-entries)
        result))

    (define (bag-intersection bag1 . bags)
      (check-bag bag1)
      (for-each
       (lambda (b)
         (check-bag b)
         (check-same-bag-comparator bag1 b))
       bags)
      (let ((result (make-bag/comparator (bag-comparator bag1)))
            (entries1 (bag-entries bag1))
            (other-entries (map bag-entries bags)))
        (define (min-count key count1)
          (let loop ((rest other-entries) (minc count1))
            (if (null? rest)
                minc
                (loop (cdr rest)
                      (min minc (hash-table-ref/default (car rest) key 0))))))
        (hash-table-for-each
         (lambda (k count1)
           (let ((m (min-count k count1)))
             (when (> m 0)
               (hash-table-set! (bag-entries result) k m))))
         entries1)
        result))

    (define (bag-difference bag1 . bags)
      (check-bag bag1)
      (for-each
       (lambda (b)
         (check-bag b)
         (check-same-bag-comparator bag1 b))
       bags)
      (let ((result (make-bag/comparator (bag-comparator bag1)))
            (entries1 (bag-entries bag1))
            (other-entries (map bag-entries bags)))
        (define (sub-count key count1)
          (let loop ((rest other-entries) (acc count1))
            (if (null? rest)
                acc
                (loop (cdr rest)
                      (- acc (hash-table-ref/default (car rest) key 0))))))
        (hash-table-for-each
         (lambda (k count1)
           (let ((r (sub-count k count1)))
             (when (> r 0)
               (hash-table-set! (bag-entries result) k r))))
         entries1)
        result))

    (define (bag-xor bag1 bag2)
      (check-bag bag1)
      (check-bag bag2)
      (check-same-bag-comparator bag1 bag2)
      (let ((result (make-bag/comparator (bag-comparator bag1)))
            (e1 (bag-entries bag1))
            (e2 (bag-entries bag2)))
        (hash-table-for-each
         (lambda (k count2)
           (let ((count1 (hash-table-ref/default e1 k 0)))
             (when (= count1 0)
               (hash-table-set! (bag-entries result) k count2))))
         e2)
        (hash-table-for-each
         (lambda (k count1)
           (let* ((count2 (hash-table-ref/default e2 k 0))
                  (diff (abs (- count1 count2))))
             (when (> diff 0)
               (hash-table-set! (bag-entries result) k diff))))
         e1)
        result))

    (define (bag-union! bag1 . bags)
      (check-bag bag1)
      (for-each
       (lambda (b)
         (check-bag b)
         (check-same-bag-comparator bag1 b)
         (bag-union-into! bag1 b))
       bags)
      bag1)

    (define (bag-intersection! bag1 . bags)
      (check-bag bag1)
      (for-each
       (lambda (b)
         (check-bag b)
         (check-same-bag-comparator bag1 b))
       bags)
      (let ((entries1 (bag-entries bag1))
            (other-entries (map bag-entries bags)))
        (define (min-count key count1)
          (let loop ((rest other-entries) (minc count1))
            (if (null? rest)
                minc
                (loop (cdr rest)
                      (min minc (hash-table-ref/default (car rest) key 0))))))
        (hash-table-for-each
         (lambda (k count1)
           (let ((m (min-count k count1)))
             (if (> m 0)
                 (hash-table-set! entries1 k m)
                 (hash-table-delete! entries1 k))))
         entries1)
        bag1))

    (define (bag-difference! bag1 . bags)
      (check-bag bag1)
      (for-each
       (lambda (b)
         (check-bag b)
         (check-same-bag-comparator bag1 b))
       bags)
      (let ((entries1 (bag-entries bag1))
            (other-entries (map bag-entries bags)))
        (define (sub-count key count1)
          (let loop ((rest other-entries) (acc count1))
            (if (null? rest)
                acc
                (loop (cdr rest)
                      (- acc (hash-table-ref/default (car rest) key 0))))))
        (hash-table-for-each
         (lambda (k count1)
           (let ((r (sub-count k count1)))
             (if (> r 0)
                 (hash-table-set! entries1 k r)
                 (hash-table-delete! entries1 k))))
         entries1)
        bag1))

    (define (bag-xor! bag1 bag2)
      (check-bag bag1)
      (check-bag bag2)
      (check-same-bag-comparator bag1 bag2)
      (let ((result (bag-xor bag1 bag2)))
        (set-bag-entries! bag1 (bag-entries result))
        bag1))

    (define (bag-adjoin! bag . elements)
      (check-bag bag)
      (for-each (lambda (x) (bag-increment! bag x 1)) elements)
      bag)

    (define (bag-adjoin bag . elements)
      (check-bag bag)
      (let ((result (bag-copy bag)))
        (for-each (lambda (x) (bag-increment! result x 1)) elements)
        result))

    (define (bag-replace! bag element)
      (check-bag bag)
      (let ((entries (bag-entries bag)))
        (when (hash-table-contains? entries element)
          (let ((count (hash-table-ref/default entries element 0)))
            (hash-table-delete! entries element)
            (hash-table-set! entries element count))))
      bag)

    (define (bag-replace bag element)
      (check-bag bag)
      (if (bag-contains? bag element)
          (let ((result (bag-copy bag)))
            (bag-replace! result element)
            result)
          bag))

    (define (bag-delete! bag . elements)
      (check-bag bag)
      (for-each (lambda (x) (bag-decrement! bag x 1)) elements)
      bag)

    (define (bag-delete bag . elements)
      (apply bag-delete! (bag-copy bag) elements))

    (define (bag-delete-all! bag element-list)
      (check-bag bag)
      (for-each (lambda (x) (bag-decrement! bag x 1)) element-list)
      bag)

    (define (bag-delete-all bag element-list)
      (bag-delete-all! (bag-copy bag) element-list))

    (define (bag-search! bag element failure success)
      (check-bag bag)
      (let* ((comp (bag-comparator bag))
             (same? (comparator-equality-predicate comp))
             (entries (bag-entries bag))
             (not-found (list 'not-found))
             (found (call/cc
                     (lambda (return)
                       (hash-table-for-each
                        (lambda (k entry)
                          (when (same? k element) (return k)))
                        entries)
                       not-found))))
        (if (eq? found not-found)
            (failure (lambda (obj)
                       (bag-increment! bag element 1)
                       (values bag obj))
                     (lambda (obj)
                       (values bag obj)))
            (success found
                     (lambda (new-element obj)
                       (bag-decrement! bag found 1)
                       (bag-increment! bag new-element 1)
                       (values bag obj))
                     (lambda (obj)
                       (bag-decrement! bag found 1)
                       (values bag obj))))))

    ) ; end of begin
  ) ; end of define-library
