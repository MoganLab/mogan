;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0630.scm
;; DESCRIPTION : Tests for amssymb symbols dependency latex export
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
  (with path
    (string-append "$TEXMACS_PATH/tests/tmu/" path)
    (with tmpfile
      (url-temp)
      (load-buffer path)
      (buffer-export path tmpfile "latex")
      (string-load tmpfile)
    ) ;with
  ) ;with
) ;define

(define (load-latex path)
  (with path
    (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")
  ) ;with
) ;define


(define (test_0630)
  (check (export-as-latex-and-load "0630.tmu")
    =>
    (load-latex "0630_three_line_table_export.tex")
  ) ;check
  (check-report)
) ;define
