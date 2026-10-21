;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 7003.scm
;; DESCRIPTION : LaTeX import of \begin{proof}[Title]
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

(define (import-body src)
  (tree->stree (latex->texmacs (parse-latex-document (string-append "\\documentclass{article}\\begin{document}"
                                                       src
                                                       "\\end{document}"
                                                     ) ;string-append
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

(define (test-titled)
  (with t
    (import-body "\\begin{proof}[Proof of Theorem 1]Body.\\end{proof}")
    (check (contains? t 'render-proof) => #t)
    (check (contains? t "Proof of Theorem 1") => #t)
    (check (contains? t "Body.") => #t)
    (check (contains? t 'proof) => #f)
  ) ;with
) ;define

(define (test-plain)
  (with t
    (import-body "\\begin{proof}Body.\\end{proof}")
    (check (contains? t 'proof) => #t)
    (check (contains? t 'render-proof) => #f)
  ) ;with
) ;define

;; 断言整棵树而非仅搜索标签：带标题的 proof 由 render-proof 承载标题，
;; 无标题的保持 proof，两者嵌套时各自闭合到正确的标签上。

(define (test-nested)
  (with t
    (import-body "\\begin{proof}[Outer]a\\begin{proof}b\\end{proof}c\\end{proof}")
    (check t
      =>
      '(document (TeXmacs "2.1.4")
         (style (tuple "article" "std-latex"))
         (body (document (render-proof "Outer"
                           (document "a" (proof (document "b")) "c")))))
    ) ;check
  ) ;with
  (with t
    (import-body "\\begin{proof}a\\begin{proof}[Inner]b\\end{proof}c\\end{proof}")
    (check t
      =>
      '(document (TeXmacs "2.1.4")
         (style (tuple "article" "std-latex"))
         (body (document (proof (document "a" (render-proof "Inner" "b") "c")))))
    ) ;check
  ) ;with
) ;define

(define (export-doc stree)
  (serialize-latex (texmacs->latex (stree->tree stree) '(("texmacs->latex:encoding"
                                                          . "utf-8")))
  ) ;serialize-latex
) ;define

(define (test-export)
  (with s
    (export-doc '(document (render-proof "Proof of Theorem 1"
                             (document "Body."))))
    (check (string-contains? s "\\begin{proof}[Proof of Theorem 1]") => #t)
    (check (string-contains? s "\\end{proof}") => #t)
    (check (string-contains? s "proof*") => #f)
    (check (string-contains? s "newenvironment") => #f)
  ) ;with
  (with s
    (export-doc '(document (render-proof "" (document "Body."))))
    (check (string-contains? s "\\begin{proof}") => #t)
    (check (string-contains? s "[") => #f)
  ) ;with
  (with s
    (export-doc '(document (proof (document "Body."))))
    (check (string-contains? s "\\begin{proof}") => #t)
  ) ;with
) ;define

(define (test-round-trip)
  (with s
    (export-doc (import-body "\\begin{proof}[Proof of Theorem 1]Body.\\end{proof}"))
    (check (string-contains? s "\\begin{proof}[Proof of Theorem 1]") => #t)
    (check (string-contains? s "proof*") => #f)
  ) ;with
) ;define

(tm-define (test_7003)
  (test-export)
  (test-round-trip)
  (test-titled)
  (test-plain)
  (test-nested)
  (check-report)
) ;tm-define
