(define-library (liii set)
  (import (rename (srfi srfi-113)
            (set make-set-with-comparator)
            (list->set list->set-with-comparator)
          ) ;rename
    (srfi srfi-128)
  ) ;import
  (export set
    set-unfold
    list->set
    list->set!
    set-copy
    set->list
    list->set-with-comparator
    make-set-with-comparator
    set?
    set-contains?
    set-empty?
    set-disjoint?
    set-element-comparator
    set-size
    set=?
    set<?
    set>?
    set<=?
    set>=?
    set-any?
    set-every?
    set-find
    set-count
    set-member
    set-search!
    set-map
    set-for-each
    set-fold
    set-filter
    set-filter!
    set-remove
    set-remove!
    set-partition
    set-partition!
    set-union
    set-intersection
    set-difference
    set-xor
    set-union!
    set-intersection!
    set-difference!
    set-xor!
    set-adjoin
    set-adjoin!
    set-replace
    set-replace!
    set-delete
    set-delete!
    set-delete-all
    set-delete-all!
  ) ;export

  (define comp (make-default-comparator))

  (define (set . elements)
    (apply make-set-with-comparator
      comp
      elements
    ) ;apply
  ) ;define

  (define (list->set elements)
    (list->set-with-comparator comp
      elements
    ) ;list->set-with-comparator
  ) ;define

) ;define-library
