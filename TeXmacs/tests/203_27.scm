;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 203_27.scm
;; DESCRIPTION : Unit tests for partial color LaTeX export
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(check-set-mode! 'report-failed)

(define (export-as-latex-and-load path)
  (with path (string-append "$TEXMACS_PATH/tests/tmu/" path)
    (with tmpfile (url-temp)
      (load-buffer path)
      (buffer-export path tmpfile "latex")
      (string-replace (string-load tmpfile) "\r\n" "\n"))))

(define expected-latex
  (string-append
    "\\documentclass{article}\n"
    "\\usepackage{CJK}\n"
    "\\usepackage{xcolor}\n"
    "\n"
    "\\begin{document}\n"
    "\\begin{CJK*}{UTF8}{gbsn}\n"
    "\n"
    "\\[ {\\color{red}{d d d}} d d d \\]\n"
    "\\[ {\\color{#AA6666}{d d d}} d d d \\]\n"
    "\\[ {\\color{blue}{d d d}} d d d \\]\n"
    "\n"
    "\\end{CJK*}\n"
    "\\end{document}\n"))

(tm-define (test_203_27)
  (check (export-as-latex-and-load "203_27.tmu")
         => expected-latex)
  (check-report))
