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

(define (import-doc src)
  (tree->stree (latex->texmacs (parse-latex-document (string-append "\\documentclass{article}"
                                                       "\\newtheorem{theorem}{Theorem}\n"
                                                       "\\begin{document}" src
                                                       "\\end{document}"
                                                     ) ;string-append
                               ) ;parse-latex-document
               ) ;latex->texmacs
  ) ;tree->stree
) ;define

(define (stree-contains? x target)
  (cond ((null? x) #f)
        ((equal? x target) #t)
        ((pair? x)
         (or (stree-contains? (car x) target) (stree-contains? (cdr x) target))
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (test-named-theorem)
  (with t
    (import-doc "\\begin{theorem}[Foo]Body.\\end{theorem}")
    (check (stree-contains? t 'dueto) => #t)
    (check (stree-contains? t "Foo") => #t)
    (check (stree-contains? t "[Foo]Body.") => #f)
  ) ;with
) ;define

(define (test-unnamed-theorem)
  (with t
    (import-doc "\\begin{theorem}Body.\\end{theorem}")
    (check (stree-contains? t 'dueto) => #f)
    (check (stree-contains? t 'theorem) => #t)
  ) ;with
) ;define

(tm-define (test_7004)
  (test-named-theorem)
  (test-unnamed-theorem)
  (check-report)
) ;tm-define
