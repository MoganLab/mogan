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

(check-set-mode! 'report-failed)

(define (normalize-latex latex)
  (set! latex (string-replace latex "\r\n" "\n"))
  (set! latex (string-replace latex "\r" ""))
  (set! latex (string-replace latex "\n" ""))
  (set! latex (string-replace latex "\t" ""))
  (string-replace latex " " ""))

(define (test-export-latex-partial-color)
  (use-modules
    (data latex)
    (data tmu))
  (let* ((tmu-doc (string-load "$TEXMACS_PATH/tests/tmu/203_27.tmu"))
         (latex (normalize-latex
                 (texmacs->latex-document (tmu->texmacs tmu-doc) '()))))
    (check (string-contains? latex "\\color{red}{ddd}") => #t)
    (check (string-contains? latex
                             (string-append "\\color{" "#" "AA6666}{ddd}"))
           => #t)
    (check (string-contains? latex "\\color{blue}{ddd}") => #t)
    (check (string-contains? latex
                             (string-append "\\color[HTML]{" "AA6666}"))
           => #f)
    (check (string-contains? latex
                             (string-append "\\color{" "#" "AA6666}ddd"))
           => #f)))

(tm-define (test_203_27)
  (test-export-latex-partial-color)
  (check-report))
