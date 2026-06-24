
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 203_6.scm
;; DESCRIPTION : Tests for latex->texmacs conversion
;; COPYRIGHT   : (C) 2025  JimZhouZZY
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-latex-to-texmacs)
  (check (tree->stree (latex->texmacs (parse-latex "$\\frac{a}{b}$")))
         => '(math (frac "a" "b")))
  (check (tree->stree (latex->texmacs (parse-latex "\\begin{align} \n A=BC \n \\end{align}")))
         => '(document (align (document (tformat (table (row (cell "A=B*C") (cell (eq-number)))))))))
  (check (tree->stree (latex->texmacs (parse-latex "\\begin{align}\n \\sigma_{xx}^{(1)}=\\frac{\\sigma_1}{1+(\\mu_1 B)^2}, \\sigma_{xy}^{(1)}=\\frac{\\sigma_1 \\mu_1 B}{1+(\\mu_1 B)^2};  \\nonumber \\\\ \\sigma_{xx}^{(2)}=\\frac{\\sigma_2}{1+(\\mu_2 B)^2}, \\sigma_{xy}^{(2)}=\\frac{\\sigma_2 \\mu_2 B}{1+(\\mu_2 B)^2},\n \\end{align}\n")))
         => '(document (align (document (tformat (table (row (cell (concat "<sigma>" (rsub "x*x") (rsup (around "(" "1" ")")) "=" (frac (concat "<sigma>" (rsub "1")) (concat "1+" (around "(" (concat "<mu>" (rsub "1") "*B") ")") (rsup "2"))) ",<sigma>" (rsub "x*y") (rsup (around "(" "1" ")")) "=" (frac (concat "<sigma>" (rsub "1") "*<mu>" (rsub "1") "*B") (concat "1+" (around "(" (concat "<mu>" (rsub "1") "*B") ")") (rsup "2"))) ";")) (cell "")) (row (cell (concat "<sigma>" (rsub "x*x") (rsup (around "(" "2" ")")) "=" (frac (concat "<sigma>" (rsub "2")) (concat "1+" (around "(" (concat "<mu>" (rsub "2") "*B") ")") (rsup "2"))) ",<sigma>" (rsub "x*y") (rsup (around "(" "2" ")")) "=" (frac (concat "<sigma>" (rsub "2") "*<mu>" (rsub "2") "*B") (concat "1+" (around "(" (concat "<mu>" (rsub "2") "*B") ")") (rsup "2"))) ",")) (cell (eq-number)))))))))
)

(tm-define (test_203_6)
  (test-latex-to-texmacs)
  (check-report))
