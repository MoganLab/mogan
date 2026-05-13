;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0605.scm
;; DESCRIPTION : Tests for stack to substack latex export
;; COPYRIGHT   : (C) 2026  Jack Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(define (export-as-latex-and-load path)
  (with path (string-append "$TEXMACS_PATH/tests/tmu/" path)
    (with tmpfile (url-temp)
      (load-buffer path)
      (buffer-export path tmpfile "latex")
      (string-load tmpfile))))
  
(define (load-latex path)
  (with path (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path)  "\r\n" "\n")))


(define (test_0605)
  (check (export-as-latex-and-load "0605.tmu") => (load-latex "0605_stack_export.tex"))
  (check-report))
