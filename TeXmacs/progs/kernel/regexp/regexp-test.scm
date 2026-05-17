
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : regexp-test.scm
;; DESCRIPTION : examples of regular expression grammars
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel regexp regexp-test) (:use (kernel regexp regexp-match)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Example of a grammar for regular expressions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-regexp-grammar (:a a b c) (:b (:repeat :a)))

;; (match? '(a b c a b c x) '(:b x))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Regtest routines for regexp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (regtest-regexp-match)
  ;; Basic test to ensure regexp module loads without errors
  (regression-test-group "regexp"
    "match"
    values
    values
    (test "module loaded successfully" #t #t)
    (test "match? function available" (defined? 'match?) #t)
    (test "define-regexp-grammar macro available"
      (defined? 'define-regexp-grammar)
      #t
    ) ;test
    (test "match function available" (defined? 'match) #t)
    (test "match-term table available" (defined? 'match-term) #t)
    (test "bindings-add function available" (defined? 'bindings-add) #t)
    (test "define-regexp-grammar-decls function available"
      (defined? 'define-regexp-grammar-decls)
      #t
    ) ;test
    (test "display* function available" (defined? 'display*) #t)
    (test "cons function available" (defined? 'cons) #t)
    (test "list function available" (defined? 'list) #t)
    (test "append function available" (defined? 'append) #t)
    (test "for-each function available" (defined? 'for-each) #t)
    (test "ahash-set! function available" (defined? 'ahash-set!) #t)
    (test "ahash-ref function available" (defined? 'ahash-ref) #t)
    (test "make-ahash-table function available" (defined? 'make-ahash-table) #t)
  ) ;regression-test-group
) ;define

(tm-define (regtest-regexp)
  (let ((n (regtest-regexp-match)))
    (display* "Total: " (number->string n) " tests.\n")
    (display "Test suite of regtest-regexp: ok\n")
  ) ;let
) ;tm-define
