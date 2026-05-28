
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


(texmacs-module (fonts fonts-test)
  (:use (kernel texmacs tm-define)))

(define (regtest-default-chinese-font)
  (cond ((os-macos?)
         (regression-test-group
          "fun" "result"
          (default-chinese-font) :none
          (test "macOS" :none "Singti SC")))
        ((or (os-win32?) (os-mingw?))
         (regression-test-group
          "fun" "result"
          (default-chinese-font) :none
          (test "Windows" :none "simsun")))
        (else
         (regression-test-group
          "fun" "result"
          (default-chinese-font) :none
          (test "GNU Linux" :none "Noto CJK SC")))))

(define (regtest-family-and-master)
  (+ (if (font-exists-in-tt? "NotoSerifCJK-Regular")
      (regression-test-group
       "font-family" "font-master"
       font-family->master :none
       (test "Noto CJK" "Noto Serif CJK SC" "Noto CJK SC"))
      0)
     (if (font-exists-in-tt? "Songti")
      (regression-test-group
       "font-family" "font-master"
       font-family->master :none
       (test "Songti" "Songti SC" "Songti SC"))
      0)))

(define (test_11_4)
  (let ((n (+ (regtest-family-and-master))))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of fonts: ok\n")))
