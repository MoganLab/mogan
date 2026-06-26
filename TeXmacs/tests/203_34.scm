;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 203_34.scm
;; DESCRIPTION : Test LaTeX export of algo-state macro to standard \State command
;; COPYRIGHT   : (C) 2026
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(define (snippet->latex snippet opts)
  (serialize-latex (texmacs->latex snippet opts)))

(tm-define (test_203_34)
  (with result (snippet->latex '(algo-state "x = 0") '())
    (display* ";;; algo-state: " result "\n")
    (check (string-contains? result "\\State") => #t)
    (check (string-contains? result "\\algostate") => #f)))

(check-report)
