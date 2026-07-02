;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0339.scm
;; DESCRIPTION : Tests for \tag LaTeX import
;; COPYRIGHT   : (C) 2026 Mogan Team
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-parse-latex-tag)
  (check
   (tree->stree
    (latex->texmacs (parse-latex "\\[ A=BC\\tag{6} \\]")))
   => '(document
         (equation*
          (document (concat "A=B*C" (no-number) (eq-lab "6"))))))
  (check
   (tree->stree
    (latex->texmacs (parse-latex "\\begin{align}\nA=BC\\tag{6}\n\\end{align}")))
   => '(document
         (align
          (document
           (tformat (table (row (cell (concat "A=B*C" (eq-lab "6"))) (cell ""))))))))
)

(tm-define (test_0339)
  (test-parse-latex-tag)
  (check-report))
