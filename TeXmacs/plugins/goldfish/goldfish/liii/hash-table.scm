(define-library (liii hash-table)
  (import (srfi srfi-125) (srfi srfi-128))
  (export make-hash-table
    hash-table
    hash-table-unfold
    alist->hash-table
    hash-table?
    hash-table-contains?
    hash-table-empty?
    hash-table=?
    hash-table-mutable?
    hash-table-ref
    hash-table-ref/default
    hash-table-set!
    hash-table-delete!
    hash-table-intern!
    hash-table-update!
    hash-table-update!/default
    hash-table-pop!
    hash-table-clear!
    hash-table-size
    hash-table-keys
    hash-table-values
    hash-table-entries
    hash-table-find
    hash-table-count
    hash-table-fold
    hash-table-for-each
    hash-table-map->list
    hash-table->alist
    hash-table-copy
  ) ;export
  (begin
  ) ;begin
) ;define-library
