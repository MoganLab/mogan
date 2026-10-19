;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 7004.scm
;; DESCRIPTION : LaTeX import of \begin{theorem}[Name] after \newtheorem
;; COPYRIGHT   : (C) 2026
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")
(import-from (latex latex-format))

(define (import-doc src)
  (tree->stree
    (latex->texmacs
      (parse-latex-document
        (string-append "\\documentclass{article}"
                       "\\newtheorem{theorem}{Theorem}\n"
                       "\\begin{document}" src "\\end{document}")
      ) ;parse-latex-document
    ) ;latex->texmacs
  ) ;tree->stree
) ;define

(define (contains? t x)
  (cond ((== t x) #t)
        ((pair? t) (or (contains? (car t) x) (contains? (cdr t) x)))
        (else #f)
  ) ;cond
) ;define

(define (test-named-theorem)
  (with t (import-doc "\\begin{theorem}[Foo]Body.\\end{theorem}")
    (check (contains? t 'dueto) => #t)
    (check (contains? t "Foo") => #t)
    (check (contains? t "[Foo] Body.") => #f)
  ) ;with
) ;define

(define (test-unnamed-theorem)
  (with t (import-doc "\\begin{theorem}Body.\\end{theorem}")
    (check (contains? t 'dueto) => #f)
    (check (contains? t 'theorem) => #t)
  ) ;with
) ;define

(tm-define (test_7004)
  (test-named-theorem)
  (test-unnamed-theorem)
  (check-report)
) ;tm-define
