(import (liii check))

(check-set-mode! 'report-failed)

(define (test-url-none?)
  (check (url-none? (url-none)) => #t)
  (check (url-none? (system->url "/tmp")) => #f)
) ;define

(tm-define (test_15_3_5) (test-url-none?) (check-report))
