;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-test.scm
;; DESCRIPTION : Test suite for generic
;; COPYRIGHT   : (C) 2022  Yufeng Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic generic-test)
  (:use (generic generic-menu)
        (table table-menu) ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; generic menu functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (regtest-focus-tag-name)
  (regression-test-group
   "focus-tag-name" "string"
   focus-tag-name :none
   (test "bmatrix" 'bmatrix "bmatrix")
   (test "Bmatrix" 'Bmatrix "Bmatrix")
   (test "tabular" 'tabular "tabular")
   (test "tabular*" 'tabular* "centered tabular")
   (test "block" 'block "block")
   (test "block*" 'block* "centered block")
   (test "big-table" 'big-table "big table")
  ))

(tm-define (regtest-generic)
  (let ((n (+ (regtest-focus-tag-name)
              )))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of generic: ok\n")))
