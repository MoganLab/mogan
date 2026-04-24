(define-library (liii range)
  (import (srfi srfi-196))
  (export range
    numeric-range
    vector-range
    string-range
    range-append
    iota-range
    range?
    range=?
    range-length
    range-ref
    range-first
    range-last
    subrange
    range-segment
    range-split-at
    range-take
    range-take-right
    range-drop
    range-drop-right
    range-count
    range-map->list
    range-for-each
    range-fold
    range-fold-right
    range-any
    range-every
    range-filter->list
    range-remove->list
    range-reverse
    range-map->vector
    range-filter->vector
    range-remove->vector
    vector->range
    range->list
    range->vector
    range->string
    range->generator
  ) ;export
) ;define-library
