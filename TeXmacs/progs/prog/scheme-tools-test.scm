
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : scheme-tools-test.scm
;; DESCRIPTION : Test suite for scheme tools
;; COPYRIGHT   : (C) 2023   Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (prog scheme-tools-test) (:use (prog scheme-tools-test)))

(define (regtest-word-at)
  (define (test-word-at i)
    (word-at "(sys)" i)
  ) ;define
  (regression-test-group "index"
    "word"
    test-word-at
    :none
    (test "index 0" 0 "")
    (test "index 1" 1 "sys")
    (test "index 2" 2 "sys")
    (test "index 3" 3 "sys")
    (test "index 4" 4 "sys")
    (test "index 5" 5 "")
    (test "invalid index" 6 "")
  ) ;regression-test-group
) ;define

(tm-define (regtest-scheme-tools)
  (let ((n (+ (regtest-word-at))))
    (display* "Total: " (object->string n) " tests.\n")
    (display "Test suite of scheme-tools: ok\n")
  ) ;let
) ;tm-define
