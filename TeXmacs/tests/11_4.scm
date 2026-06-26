
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : font-test.scm
;; DESCRIPTION : Test suite for Fonts
;; COPYRIGHT   : (C) 2022  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(texmacs-module (fonts fonts-test) (:use (kernel texmacs tm-define)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-default-chinese-font)
  (cond ((os-macos?) (check (default-chinese-font) => "Singti SC"))
        ((os-mingw?) (check (default-chinese-font) => "simsun"))
        (else (check (default-chinese-font) => "Noto CJK SC"))
  ) ;cond
) ;define

(define (test-family-and-master)
  (if (font-exists-in-tt? "NotoSerifCJK-Regular")
    (check (font-family->master "Noto Serif CJK SC") => "Noto CJK SC")
    (check #t => #t)
  ) ;if
  (if (font-exists-in-tt? "Songti")
    (check (font-family->master "Songti SC") => "Songti SC")
    (check #t => #t)
  ) ;if
) ;define

(define (test_11_4)
  (test-default-chinese-font)
  (test-family-and-master)
  (check-report)
) ;define
