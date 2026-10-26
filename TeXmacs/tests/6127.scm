;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6127.scm
;; DESCRIPTION : Test suite for \colon macro and LaTeX import/export
;; COPYRIGHT   : (C) 2026 Darcy Shen, MoonL79
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

(define (import-body src)
  (tree->stree (latex->texmacs (parse-latex-document (string-append "\\documentclass{article}\\begin{document}"
                                                       src
                                                       "\\end{document}"
                                                     ) ;string-append
                               ) ;parse-latex-document
               ) ;latex->texmacs
  ) ;tree->stree
) ;define

(define (export-doc stree)
  (serialize-latex (texmacs->latex (stree->tree stree) '(("texmacs->latex:encoding"
                                                          . "utf-8")))
  ) ;serialize-latex
) ;define

(define (contains? t x)
  (cond ((== t x) #t)
        ((pair? t) (or (contains? (car t) x) (contains? (cdr t) x)))
        (else #f)
  ) ;cond
) ;define

(define (test-import-colon)
  (with t
    (import-body "$f\\colon X$")
    (check (contains? t 'colon) => #t)
    (check (contains? t "f") => #t)
    (check (contains? t "X") => #t)
  ) ;with
  (with t
    (import-body "$\\colon A$")
    (check (contains? t 'colon) => #t)
    (check (contains? t "A") => #t)
  ) ;with
  (with t
    (import-body "$f\\colon X \\to Y$")
    (check (contains? t 'colon) => #t)
    (check (contains? t "f") => #t)
    (check (contains? t "X<to>Y") => #t)
  ) ;with
) ;define

(define (test-export-colon)
  (with s
    (export-doc
      '(document (para (math (concat "f" (colon) "X"))))
    ) ;export-doc
    (check (string-contains? s "\\colon") => #t)
  ) ;with
  (with s
    (export-doc
      '(document (para (math (concat (colon) "A"))))
    ) ;export-doc
    (check (string-contains? s "\\colon") => #t)
  ) ;with
) ;define

(define (test-round-trip-colon)
  (with s
    (export-doc (import-body "$f\\colon X$"))
    (check (string-contains? s "\\colon") => #t)
  ) ;with
  (with s
    (export-doc (import-body "$\\colon A$"))
    (check (string-contains? s "\\colon") => #t)
  ) ;with
  (with s
    (export-doc (import-body "$f\\colon X \\to Y$"))
    (check (string-contains? s "\\colon") => #t)
    (check (string-contains? s "\\to") => #t)
  ) ;with
) ;define

(tm-define (test_6127)
  (test-import-colon)
  (test-export-colon)
  (test-round-trip-colon)
  (check-report)
) ;tm-define
