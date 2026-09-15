(define-library (liii generator)
  (import (srfi srfi-158))
  (export generator
    circular-generator
    make-iota-generator
    make-range-generator
    make-coroutine-generator
    make-for-each-generator
    make-unfold-generator

    list->generator
    vector->generator
    reverse-vector->generator
    string->generator
    bytevector->generator

    generator->list
    generator->reverse-list
    generator->vector
    generator->vector!
    generator->string
    generator-map->list

    gcons*
    gappend
    gflatten
    ggroup
    gmerge
    gmap
    gcombine
    gfilter
    gremove
    gstate-filter
    gtake
    gdrop
    gtake-while
    gdrop-while
    gdelete
    gdelete-neighbor-dups
    gindex
    gselect
    generator-fold
    generator-for-each
    generator-find
    generator-count
    generator-any
    generator-every
    generator-unfold

    make-accumulator
    list-accumulator
    reverse-list-accumulator
    vector-accumulator
    reverse-vector-accumulator
    vector-accumulator!
    string-accumulator
    bytevector-accumulator
    bytevector-accumulator!
    sum-accumulator
    product-accumulator
  ) ;export
) ;define-library
