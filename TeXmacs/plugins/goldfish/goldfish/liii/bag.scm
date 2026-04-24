(define-library (liii bag)
  (import (rename (srfi srfi-113)
            (bag make-bag-with-comparator)
            (list->bag list->bag-with-comparator)
          ) ;rename
    (only (srfi srfi-113)
      bag-unfold
      bag-member
      bag-comparator
      bag->list
      list->bag!
      bag-copy
      bag?
      bag-contains?
      bag-empty?
      bag-disjoint?
      bag-size
      bag-find
      bag-count
      bag-any?
      bag-every?
      bag=?
      bag<?
      bag>?
      bag<=?
      bag>=?
      bag-union
      bag-intersection
      bag-difference
      bag-xor
      bag-union!
      bag-intersection!
      bag-difference!
      bag-xor!
      bag-adjoin
      bag-adjoin!
      bag-replace
      bag-replace!
      bag-delete
      bag-delete!
      bag-delete-all
      bag-delete-all!
      bag-search!
    ) ;only
    (srfi srfi-128)
  ) ;import
  (export bag
    bag-unfold
    bag-member
    bag-comparator
    bag->list
    list->bag
    list->bag!
    bag-copy
    bag?
    bag-contains?
    bag-empty?
    bag-disjoint?
    bag-size
    bag-find
    bag-count
    bag-any?
    bag-every?
    bag=?
    bag<?
    bag>?
    bag<=?
    bag>=?
    bag-union
    bag-intersection
    bag-difference
    bag-xor
    bag-union!
    bag-intersection!
    bag-difference!
    bag-xor!
    bag-adjoin
    bag-adjoin!
    bag-replace
    bag-replace!
    bag-delete
    bag-delete!
    bag-delete-all
    bag-delete-all!
    bag-search!
  ) ;export

  (define comp (make-default-comparator))

  (define (bag . elements)
    (apply make-bag-with-comparator
      comp
      elements
    ) ;apply
  ) ;define

  (define (list->bag elements)
    (list->bag-with-comparator comp
      elements
    ) ;list->bag-with-comparator
  ) ;define

) ;define-library
