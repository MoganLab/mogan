(import (liii check))

(check-set-mode! 'report-failed)

(define (test-cpp-string-number?)
  (check (cpp-string-number? "1.1") => #t)
  (check (cpp-string-number? "1") => #t)
  (check (cpp-string-number? "") => #f)
) ;define

(tm-define (test_15_3_1) (test-cpp-string-number?) (check-report))
