(import (liii check))

(check-set-mode! 'report-failed)

(define (test-tree-atomic?)
  (check (tree-atomic? (string->tree "a string")) => #t)
) ;define

(tm-define (test_15_3_2) (test-tree-atomic?) (check-report))
