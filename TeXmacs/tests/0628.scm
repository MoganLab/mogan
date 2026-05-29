;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0628.scm
;; DESCRIPTION : Tests for mdframed LaTeX import mapping
;; COPYRIGHT   : (C) 2026  Jack Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(define (test-mdframed-import)
  (check (tree->stree (latex->texmacs (parse-latex "\\begin{mdframed}hello\\end{mdframed}"))
         ) ;tree->stree
    =>
    '(document (mdframed (document "hello")))
  ) ;check
) ;define

(tm-define (test_0628) (test-mdframed-import) (check-report))
