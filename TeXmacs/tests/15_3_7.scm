(import (liii check))

(check-set-mode! 'report-failed)

(define (test-url-format)
  (check (url-format "$TEXMACS_PATH/progs/init-texmacs.scm") => "scheme")
) ;define

(tm-define (test_15_3_7) (test-url-format) (check-report))
