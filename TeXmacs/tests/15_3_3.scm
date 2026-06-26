(import (liii check))

(check-set-mode! 'report-failed)

(define (test-string-alpha?)
  (check (string-alpha? "a") => #t)
  (check (string-alpha? "b") => #t)
  (check (string-alpha? "Z") => #t)
  (check (string-alpha? "?") => #f)
  (check (string-alpha? "") => #f)
) ;define

(define (test-string-occurs?)
  (define (f x)
    (string-occurs? x "This is a love letter to S7 scheme.")
  ) ;define
  (check (f "This") => #t)
  (check (f "S7") => #t)
  (check (f "ve l") => #t)
  (check (f "notcontain") => #f)
  (check (f "") => #t)
) ;define

(tm-define (test_15_3_3)
  (test-string-alpha?)
  (test-string-occurs?)
  (check-report)
) ;tm-define
