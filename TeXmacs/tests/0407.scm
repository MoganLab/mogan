;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 222_85.scm
;; DESCRIPTION : Tests for \varOmega LaTeX import
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-parse-latex-varOmega)
  (check (tree->stree (latex->texmacs (parse-latex "\\( \\varOmega \\)")))
         => '(math "<varOmega>"))
  (check (tree->stree (latex->texmacs (parse-latex "\\varOmega")))
         => "<varOmega>"))

(tm-define (test_0407)
  (test-parse-latex-varOmega)
  (check-report))
