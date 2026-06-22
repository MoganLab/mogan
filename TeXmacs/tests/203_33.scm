(import (liii check))

(check-set-mode! 'report-failed)

(define (test-parse-latex-varlim)
  (check (tree->stree (latex->texmacs (parse-latex "\\varlimsup"))) => "varlimsup")
  (check (tree->stree (latex->texmacs (parse-latex "\\varliminf"))) => "varliminf"))

(tm-define (test_varlim)
  (test-parse-latex-varlim)
  (check-report))
