;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0409.scm
;; DESCRIPTION : Tests for \varomega LaTeX import
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-parse-latex-varomega)
  (check (tree->stree (latex->texmacs (parse-latex "\\( \\varomega \\)")))
         => '(math "<varomega>"))
  (check (tree->stree (latex->texmacs (parse-latex "\\varomega")))
         => "<varomega>"))

(tm-define (test_0409)
  (test-parse-latex-varomega)
  (check-report))
