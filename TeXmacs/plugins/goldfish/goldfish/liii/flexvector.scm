(define-library (liii flexvector)
  (import (srfi srfi-214))
  (export
    ;; Constructors
    make-flexvector
    flexvector
    ;; Predicates
    flexvector?
    flexvector-empty?
    ;; Accessors
    flexvector-ref
    flexvector-front
    flexvector-back
    flexvector-length
    ;; Mutators
    flexvector-set!
    flexvector-add!
    flexvector-add-back!
    flexvector-add-front!
    flexvector-remove!
    flexvector-remove-back!
    flexvector-remove-front!
    flexvector-remove-range!
    flexvector-clear!
    flexvector-fill!
    flexvector-swap!
    flexvector-reverse!
    ;; Conversion
    flexvector->vector
    vector->flexvector
    flexvector->list
    list->flexvector
    reverse-flexvector->list
    reverse-list->flexvector
    flexvector->string
    string->flexvector
    ;; Copying
    flexvector-copy
    flexvector-copy!
    flexvector-reverse-copy
    flexvector-reverse-copy!
    ;; Iteration
    flexvector-for-each
    flexvector-for-each/index
    flexvector-map
    flexvector-map!
    flexvector-map/index
    flexvector-map/index!
    flexvector-fold
    flexvector-fold-right
    flexvector-filter
    flexvector-filter!
    flexvector-filter/index
    flexvector-filter/index!
    flexvector-append-map
    flexvector-append-map/index
    flexvector-count
    flexvector-cumulate
    ;; Searching
    flexvector-index
    flexvector-index-right
    flexvector-skip
    flexvector-skip-right
    flexvector-any
    flexvector-every
    flexvector-binary-search
    ;; Partitioning
    flexvector-partition
    ;; Concatenation
    flexvector-append
    flexvector-concatenate
    flexvector-append-subvectors
    flexvector-append!
    ;; Comparison
    flexvector=?
    ;; Unfolding
    flexvector-unfold
    flexvector-unfold-right
    ;; Generators
    flexvector->generator
    generator->flexvector
  ) ;export
) ;define-library
