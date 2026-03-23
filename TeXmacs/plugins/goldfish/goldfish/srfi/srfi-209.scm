; SPDX-License-Identifier: MIT
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
; Implementation of SRFI 209: Enums and Enum Sets

(define-library (srfi srfi-209)
  (import (scheme base)
          (scheme case-lambda)
          (srfi srfi-1)
          (srfi srfi-128)
          (liii hash-table)
  ) ;import
  (export
    ; Predicates
    enum-type? enum? enum-type-contains?
    enum=? enum<? enum>? enum<=? enum>=?
    ; Enum type constructor
    make-enum-type
    ; Enum accessors
    enum-type enum-name enum-ordinal enum-value
    ; Enum finders
    enum-name->enum enum-ordinal->enum
    enum-name->ordinal enum-name->value
    enum-ordinal->name enum-ordinal->value
    ; Enum type accessors
    enum-type-size enum-min enum-max
    enum-type-enums enum-type-names enum-type-values
    ; Enum operations
    enum-next enum-prev
    ; Comparators
    make-enum-comparator
    ; Enum set constructors
    enum-empty-set enum-type->enum-set enum-set list->enum-set
    enum-set-projection enum-set-copy
    make-enumeration enum-set-universe enum-set-constructor enum-set-indexer
    ; Enum set predicates
    enum-set? enum-set-contains? enum-set-member? enum-set-empty?
    enum-set-disjoint? enum-set=? enum-set<? enum-set>?
    enum-set<=? enum-set>=? enum-set-subset?
    enum-set-any? enum-set-every?
    ; Enum set accessors
    enum-set-type
    ; Enum set mutators
    enum-set-adjoin enum-set-adjoin!
    enum-set-delete enum-set-delete!
    enum-set-delete-all enum-set-delete-all!
    ; Enum set operations
    enum-set-size enum-set->enum-list enum-set->list enum-set-map->list
    enum-set-count enum-set-filter enum-set-filter!
    enum-set-remove enum-set-remove!
    enum-set-for-each enum-set-fold
    ; Enum set logical operations
    enum-set-union enum-set-union!
    enum-set-intersection enum-set-intersection!
    enum-set-difference enum-set-difference!
    enum-set-xor enum-set-xor!
    enum-set-complement enum-set-complement!
  ) ;export
  (begin

    ;;; Utility

    (define (exact-natural? obj)
      (and (exact-integer? obj) (not (negative? obj)))
    ) ;define

    ;;; Types

    (define-record-type <enum-type>
      (make-raw-enum-type enum-vector name-table comparator)
      enum-type?
      (enum-vector enum-type-enum-vector set-enum-type-enum-vector!)
      (name-table enum-type-name-table set-enum-type-name-table!)
      (comparator enum-type-comparator set-enum-type-comparator!)
    ) ;define-record-type

    (define-record-type <enum>
      (make-enum type name ordinal value)
      enum?
      (type enum-type)
      (name enum-name)
      (ordinal enum-ordinal)
      (value enum-value)
    ) ;define-record-type

    (define (make-enum-type names+vals)
      (let* ((type (make-raw-enum-type #f #f #f))
             (enums (generate-enums type names+vals)))
        (set-enum-type-enum-vector! type (list->vector enums))
        (set-enum-type-name-table! type (make-name-table enums))
        (set-enum-type-comparator! type (make-enum-comparator type))
        type
      ) ;let*
    ) ;define

    (define (generate-enums type names+vals)
      (let loop ((elts names+vals) (ord 0) (result '()))
        (if (null? elts)
          (reverse result)
          (let ((elt (car elts)))
            (cond
              ((and (pair? elt) (= 2 (length elt)) (symbol? (car elt)))
                (loop (cdr elts) (+ ord 1)
                      (cons (make-enum type (car elt) ord (cadr elt)) result)
                ) ;loop
              ) ;
              ((symbol? elt)
                (loop (cdr elts) (+ ord 1)
                      (cons (make-enum type elt ord ord) result)
                ) ;loop
              ) ;
              (else
                (error "make-enum-type: invalid argument" elt)
              ) ;else
            ) ;cond
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (make-name-table enums)
      (let ((ht (make-hash-table)))
        (for-each
          (lambda (enum)
            (hash-table-set! ht (enum-name enum) enum)
          ) ;lambda
          enums
        ) ;for-each
        ht
      ) ;let
    ) ;define

    (define (%enum-type=? etype1 etype2)
      (eqv? etype1 etype2)
    ) ;define

    (define (make-enum-comparator type)
      (make-comparator
        (lambda (obj)
          (and (enum? obj) (eq? (enum-type obj) type))
        ) ;lambda
        eq?
        (lambda (enum1 enum2)
          (< (enum-ordinal enum1) (enum-ordinal enum2))
        ) ;lambda
        (lambda (enum)
          (symbol-hash (enum-name enum))
        ) ;lambda
      ) ;make-comparator
    ) ;define

    ;;; Predicates

    (define (enum-type-contains? type enum)
      (and (enum-type? type)
           (enum? enum)
           ((comparator-type-test-predicate (enum-type-comparator type)) enum)
      ) ;and
    ) ;define

    (define (%enum-type-contains?/no-assert type enum)
      ((comparator-type-test-predicate (enum-type-comparator type)) enum)
    ) ;define

    (define (%well-typed-enum? type obj)
      (and (enum? obj) (%enum-type-contains?/no-assert type obj))
    ) ;define

    (define (%compare-enums compare enums)
      (let ((type (enum-type (car enums))))
        (apply compare (enum-type-comparator type) enums)
      ) ;let
    ) ;define

    (define (enum=? enum1 enum2 . enums)
      (let* ((type (enum-type enum1))
             (comp (enum-type-comparator type)))
        (if (null? enums)
          ((comparator-equality-predicate comp) enum1 enum2)
          (apply =? comp enum1 enum2 enums)
        ) ;if
      ) ;let*
    ) ;define

    (define (enum<? . enums) (%compare-enums <? enums))
    (define (enum>? . enums) (%compare-enums >? enums))
    (define (enum<=? . enums) (%compare-enums <=? enums))
    (define (enum>=? . enums) (%compare-enums >=? enums))

    ;;; Enum finders

    (define (enum-name->enum type name)
      (hash-table-ref/default (enum-type-name-table type) name #f)
    ) ;define

    (define (enum-ordinal->enum enum-type ordinal)
      (and (< ordinal (enum-type-size enum-type))
           (vector-ref (enum-type-enum-vector enum-type) ordinal)
      ) ;and
    ) ;define

    (define (%enum-ordinal->enum-no-assert enum-type ordinal)
      (vector-ref (enum-type-enum-vector enum-type) ordinal)
    ) ;define

    (define (%enum-project type finder key proc)
      (cond ((finder type key) => proc)
            (else (error "no enum found" type key))
      ) ;cond
    ) ;define

    (define (enum-name->ordinal type name)
      (%enum-project type enum-name->enum name enum-ordinal)
    ) ;define

    (define (enum-name->value type name)
      (%enum-project type enum-name->enum name enum-value)
    ) ;define

    (define (enum-ordinal->name type ordinal)
      (%enum-project type %enum-ordinal->enum-no-assert ordinal enum-name)
    ) ;define

    (define (enum-ordinal->value type ordinal)
      (%enum-project type %enum-ordinal->enum-no-assert ordinal enum-value)
    ) ;define

    ;;; Enum type accessors

    (define (enum-type-size type)
      (vector-length (enum-type-enum-vector type))
    ) ;define

    (define (enum-min type)
      (vector-ref (enum-type-enum-vector type) 0)
    ) ;define

    (define (enum-max type)
      (let ((vec (enum-type-enum-vector type)))
        (vector-ref vec (- (vector-length vec) 1))
      ) ;let
    ) ;define

    (define (enum-type-enums type)
      (vector->list (enum-type-enum-vector type))
    ) ;define

    (define (enum-type-names type)
      (let ((vec (enum-type-enum-vector type)))
        (let loop ((i 0) (result '()))
          (if (= i (vector-length vec))
            (reverse result)
            (loop (+ i 1) (cons (enum-name (vector-ref vec i)) result))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (enum-type-values type)
      (let ((vec (enum-type-enum-vector type)))
        (let loop ((i 0) (result '()))
          (if (= i (vector-length vec))
            (reverse result)
            (loop (+ i 1) (cons (enum-value (vector-ref vec i)) result))
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    ;;; Enum object procedures

    (define (enum-next enum)
      (enum-ordinal->enum (enum-type enum) (+ (enum-ordinal enum) 1))
    ) ;define

    (define (enum-prev enum)
      (let ((ord (enum-ordinal enum)))
        (and (> ord 0)
             (enum-ordinal->enum (enum-type enum) (- ord 1))
        ) ;and
      ) ;let
    ) ;define

    ;;; Enum set constructors

    ; We use a vector of booleans instead of bitvector
    (define-record-type <enum-set>
      (make-enum-set type bits)
      enum-set?
      (type enum-set-type)
      (bits enum-set-bits set-enum-set-bits!)
    ) ;define-record-type

    (define (make-bits size init)
      (make-vector size init)
    ) ;define

    (define (bits-ref bits i)
      (vector-ref bits i)
    ) ;define

    (define (bits-set! bits i val)
      (vector-set! bits i val)
    ) ;define

    (define (bits-copy bits)
      (vector-copy bits)
    ) ;define

    (define (bits-length bits)
      (vector-length bits)
    ) ;define

    (define (bits-count val bits)
      (let ((len (vector-length bits))
            (count 0))
        (let loop ((i 0) (count 0))
          (if (= i len)
            count
            (loop (+ i 1)
                  (if (eqv? (vector-ref bits i) val) (+ count 1) count)
            ) ;loop
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (bits=? bits1 bits2)
      (let ((len (vector-length bits1)))
        (let loop ((i 0))
          (cond
            ((= i len) #t)
            ((not (eqv? (vector-ref bits1 i) (vector-ref bits2 i))) #f)
            (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (bits-subset? bits1 bits2)
      ; bits1 is subset of bits2 if every 1 in bits1 is 1 in bits2
      (let ((len (vector-length bits1)))
        (let loop ((i 0))
          (cond
            ((= i len) #t)
            ((and (vector-ref bits1 i) (not (vector-ref bits2 i))) #f)
            (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (bits-disjoint? bits1 bits2)
      (let ((len (vector-length bits1)))
        (let loop ((i 0))
          (cond
            ((= i len) #t)
            ((and (vector-ref bits1 i) (vector-ref bits2 i)) #f)
            (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    ; Logical operations on bits
    (define (bits-ior! bits1 bits2)
      (let ((len (vector-length bits1)))
        (let loop ((i 0))
          (when (< i len)
            (vector-set! bits1 i (or (vector-ref bits1 i) (vector-ref bits2 i)))
            (loop (+ i 1))
          ) ;when
        ) ;let
      ) ;let
      bits1
    ) ;define

    (define (bits-and! bits1 bits2)
      (let ((len (vector-length bits1)))
        (let loop ((i 0))
          (when (< i len)
            (vector-set! bits1 i (and (vector-ref bits1 i) (vector-ref bits2 i)))
            (loop (+ i 1))
          ) ;when
        ) ;let
      ) ;let
      bits1
    ) ;define

    (define (bits-andc2! bits1 bits2)
      ; bits1 and (not bits2)
      (let ((len (vector-length bits1)))
        (let loop ((i 0))
          (when (< i len)
            (vector-set! bits1 i (and (vector-ref bits1 i) (not (vector-ref bits2 i))))
            (loop (+ i 1))
          ) ;when
        ) ;let
      ) ;let
      bits1
    ) ;define

    (define (bits-xor! bits1 bits2)
      (let ((len (vector-length bits1)))
        (let loop ((i 0))
          (when (< i len)
            (vector-set! bits1 i
              (not (eqv? (vector-ref bits1 i) (vector-ref bits2 i)))
            ) ;vector-set!
            (loop (+ i 1))
          ) ;when
        ) ;let
      ) ;let
      bits1
    ) ;define

    (define (bits-not! bits)
      (let ((len (vector-length bits)))
        (let loop ((i 0))
          (when (< i len)
            (vector-set! bits i (not (vector-ref bits i)))
            (loop (+ i 1))
          ) ;when
        ) ;let
      ) ;let
      bits
    ) ;define

    (define (enum-empty-set type)
      (make-enum-set type (make-bits (enum-type-size type) #f))
    ) ;define

    (define (enum-type->enum-set type)
      (make-enum-set type (make-bits (enum-type-size type) #t))
    ) ;define

    (define (enum-set type . enums)
      (list->enum-set type enums)
    ) ;define

    (define (list->enum-set type enums)
      (let ((vec (make-bits (enum-type-size type) #f)))
        (for-each
          (lambda (e)
            (bits-set! vec (enum-ordinal e) #t)
          ) ;lambda
          enums
        ) ;for-each
        (make-enum-set type vec)
      ) ;let
    ) ;define

    (define (enum-set-projection src eset)
      (let ((type (if (enum-type? src) src (enum-set-type src))))
        (list->enum-set
          type
          (enum-set-map->list
            (lambda (enum)
              (let ((name (enum-name enum)))
                (or (enum-name->enum type name)
                    (error "enum name not found in type" name type)
                ) ;or
              ) ;let
            ) ;lambda
            eset
          ) ;enum-set-map->list
        ) ;list->enum-set
      ) ;let
    ) ;define

    (define (enum-set-copy eset)
      (make-enum-set (enum-set-type eset)
                     (bits-copy (enum-set-bits eset))
      ) ;make-enum-set
    ) ;define

    ; Deprecated R6RS compatibility
    (define (make-enumeration names)
      (enum-type->enum-set (make-enum-type (map (lambda (n) (list n n)) names)))
    ) ;define

    (define (enum-set-universe eset)
      (enum-type->enum-set (enum-set-type eset))
    ) ;define

    (define (enum-set-constructor eset)
      (let ((type (enum-set-type eset)))
        (lambda (names)
          (list->enum-set
            type
            (map (lambda (sym)
                   (or (enum-name->enum type sym)
                       (error "invalid enum name" sym))
                   ) ;or
                 names
            ) ;map
          ) ;list->enum-set
        ) ;lambda
      ) ;let
    ) ;define

    (define (enum-set-indexer eset)
      (let ((type (enum-set-type eset)))
        (lambda (name)
          (cond ((enum-name->enum type name) => enum-ordinal)
                (else #f)
          ) ;cond
        ) ;lambda
      ) ;let
    ) ;define

    ;;; Enum set predicates

    (define (enum-set-contains? eset enum)
      (bits-ref (enum-set-bits eset) (enum-ordinal enum))
    ) ;define

    (define (enum-set-member? name eset)
      (bits-ref (enum-set-bits eset)
                (enum-name->ordinal (enum-set-type eset) name)
      ) ;bits-ref
    ) ;define

    (define (%enum-set-type=? eset1 eset2)
      (%enum-type=? (enum-set-type eset1) (enum-set-type eset2))
    ) ;define

    (define (enum-set-empty? eset)
      (zero? (bits-count #t (enum-set-bits eset)))
    ) ;define

    (define (enum-set-disjoint? eset1 eset2)
      (bits-disjoint? (enum-set-bits eset1) (enum-set-bits eset2))
    ) ;define

    (define (enum-set=? eset1 eset2)
      (bits=? (enum-set-bits eset1) (enum-set-bits eset2))
    ) ;define

    (define (enum-set<? eset1 eset2)
      (and (bits-subset? (enum-set-bits eset1) (enum-set-bits eset2))
           (not (bits=? (enum-set-bits eset1) (enum-set-bits eset2)))
      ) ;and
    ) ;define

    (define (enum-set>? eset1 eset2)
      (and (bits-subset? (enum-set-bits eset2) (enum-set-bits eset1))
           (not (bits=? (enum-set-bits eset1) (enum-set-bits eset2)))
      ) ;and
    ) ;define

    (define (enum-set<=? eset1 eset2)
      (bits-subset? (enum-set-bits eset1) (enum-set-bits eset2))
    ) ;define

    (define (enum-set>=? eset1 eset2)
      (bits-subset? (enum-set-bits eset2) (enum-set-bits eset1))
    ) ;define

    (define (enum-set-subset? eset1 eset2)
      ; Check if every name in eset1 is also in eset2
      (let ((names1 (enum-set-map->list enum-name eset1))
            (names2 (enum-set-map->list enum-name eset2)))
        (let loop ((rest names1))
          (cond
            ((null? rest) #t)
            ((not (member (car rest) names2)) #f)
            (else (loop (cdr rest)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (enum-set-any? pred eset)
      (call-with-current-continuation
        (lambda (return)
          (enum-set-fold (lambda (e _) (and (pred e) (return #t)))
                         #f
                         eset
          ) ;enum-set-fold
        ) ;lambda
      ) ;call-with-current-continuation
    ) ;define

    (define (enum-set-every? pred eset)
      (call-with-current-continuation
        (lambda (return)
          (enum-set-fold (lambda (e _) (or (pred e) (return #f)))
                         #t
                         eset
          ) ;enum-set-fold
        ) ;lambda
      ) ;call-with-current-continuation
    ) ;define

    ;;; Enum set mutators

    (define (enum-set-adjoin eset . enums)
      (apply enum-set-adjoin! (enum-set-copy eset) enums)
    ) ;define

    (define enum-set-adjoin!
      (case-lambda
        ((eset enum)
         (bits-set! (enum-set-bits eset) (enum-ordinal enum) #t)
         eset
        ) ;
        ((eset . enums)
         (let ((vec (enum-set-bits eset)))
           (for-each (lambda (e) (bits-set! vec (enum-ordinal e) #t)) enums)
           eset
         ) ;let
        ) ;
      ) ;case-lambda
    ) ;define

    (define (enum-set-delete eset . enums)
      (apply enum-set-delete! (enum-set-copy eset) enums)
    ) ;define

    (define enum-set-delete!
      (case-lambda
        ((eset enum)
         (bits-set! (enum-set-bits eset) (enum-ordinal enum) #f)
         eset
        ) ;
        ((eset . enums)
         (enum-set-delete-all! eset enums)
        ) ;
      ) ;case-lambda
    ) ;define

    (define (enum-set-delete-all eset enums)
      (enum-set-delete-all! (enum-set-copy eset) enums)
    ) ;define

    (define (enum-set-delete-all! eset enums)
      (let ((vec (enum-set-bits eset)))
        (for-each (lambda (e) (bits-set! vec (enum-ordinal e) #f)) enums)
        eset
      ) ;let
    ) ;define

    ;;; Enum set operations

    (define (enum-set-size eset)
      (bits-count #t (enum-set-bits eset))
    ) ;define

    (define (enum-set->enum-list eset)
      (enum-set-map->list values eset)
    ) ;define

    (define (enum-set->list eset)
      (enum-set-map->list enum-name eset)
    ) ;define

    (define (enum-set-map->list proc eset)
      (let* ((vec (enum-set-bits eset))
             (len (bits-length vec))
             (type (enum-set-type eset)))
        (let loop ((i 0) (result '()))
          (cond
            ((= i len) (reverse result))
            ((bits-ref vec i)
             (loop (+ i 1) (cons (proc (%enum-ordinal->enum-no-assert type i)) result))
            ) ;
            (else (loop (+ i 1) result))
          ) ;cond
        ) ;let
      ) ;let*
    ) ;define

    (define (enum-set-count pred eset)
      (enum-set-fold (lambda (e n) (if (pred e) (+ n 1) n)) 0 eset)
    ) ;define

    (define (enum-set-filter pred eset)
      (enum-set-filter! pred (enum-set-copy eset))
    ) ;define

    (define (enum-set-filter! pred eset)
      (let* ((type (enum-set-type eset))
             (vec (enum-set-bits eset)))
        (let loop ((i (- (bits-length vec) 1)))
          (cond
            ((< i 0) eset)
            ((and (bits-ref vec i)
                  (not (pred (%enum-ordinal->enum-no-assert type i))))
             (bits-set! vec i #f)
             (loop (- i 1))
            ) ;
            (else (loop (- i 1)))
          ) ;cond
        ) ;let
      ) ;let*
    ) ;define

    (define (enum-set-remove pred eset)
      (enum-set-remove! pred (enum-set-copy eset))
    ) ;define

    (define (enum-set-remove! pred eset)
      (let* ((type (enum-set-type eset))
             (vec (enum-set-bits eset)))
        (let loop ((i (- (bits-length vec) 1)))
          (cond
            ((< i 0) eset)
            ((and (bits-ref vec i)
                  (pred (%enum-ordinal->enum-no-assert type i)))
             (bits-set! vec i #f)
             (loop (- i 1))
            ) ;
            (else (loop (- i 1)))
          ) ;cond
        ) ;let
      ) ;let*
    ) ;define

    (define (enum-set-for-each proc eset)
      (enum-set-fold (lambda (e _) (proc e)) '() eset)
    ) ;define

    (define (enum-set-fold proc nil eset)
      (let ((type (enum-set-type eset)))
        (let* ((vec (enum-set-bits eset))
               (len (bits-length vec)))
          (let loop ((i 0) (state nil))
            (cond
              ((= i len) state)
              ((bits-ref vec i)
               (loop (+ i 1)
                     (proc (%enum-ordinal->enum-no-assert type i) state)
               ) ;loop
              ) ;
              (else (loop (+ i 1) state))
            ) ;cond
          ) ;let
        ) ;let*
      ) ;let
    ) ;define

    ;;; Enum set logical operations

    (define (%enum-set-logical-op! bv-proc eset1 eset2)
      (bv-proc (enum-set-bits eset1) (enum-set-bits eset2))
      eset1
    ) ;define

    (define (enum-set-union eset1 eset2)
      (%enum-set-logical-op! bits-ior! (enum-set-copy eset1) eset2)
    ) ;define

    (define (enum-set-intersection eset1 eset2)
      (%enum-set-logical-op! bits-and! (enum-set-copy eset1) eset2)
    ) ;define

    (define (enum-set-difference eset1 eset2)
      (%enum-set-logical-op! bits-andc2! (enum-set-copy eset1) eset2)
    ) ;define

    (define (enum-set-xor eset1 eset2)
      (%enum-set-logical-op! bits-xor! (enum-set-copy eset1) eset2)
    ) ;define

    (define (enum-set-union! eset1 eset2)
      (%enum-set-logical-op! bits-ior! eset1 eset2)
    ) ;define

    (define (enum-set-intersection! eset1 eset2)
      (%enum-set-logical-op! bits-and! eset1 eset2)
    ) ;define

    (define (enum-set-difference! eset1 eset2)
      (%enum-set-logical-op! bits-andc2! eset1 eset2)
    ) ;define

    (define (enum-set-xor! eset1 eset2)
      (%enum-set-logical-op! bits-xor! eset1 eset2)
    ) ;define

    (define (enum-set-complement eset)
      (enum-set-complement! (enum-set-copy eset))
    ) ;define

    (define (enum-set-complement! eset)
      (bits-not! (enum-set-bits eset))
      eset
    ) ;define

  ) ;begin
) ;define-library
