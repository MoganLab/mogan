
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

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Example of a grammar for regular expressions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-regexp-grammar (:a a b c) (:b (:repeat :a)))

;; (match? '(a b c a b c x) '(:b x))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Regtest routines for regexp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-regexp-match)
  ;; Basic test to ensure regexp module loads without errors
  (check #t => #t)
  (check (defined? 'match?) => #t)
  (check (defined? 'define-regexp-grammar) => #t)
  (check (defined? 'match) => #t)
  (check (defined? 'match-term) => #t)
  (check (defined? 'bindings-add) => #t)
  (check (defined? 'define-regexp-grammar-decls) => #t)
  (check (defined? 'display*) => #t)
  (check (defined? 'cons) => #t)
  (check (defined? 'list) => #t)
  (check (defined? 'append) => #t)
  (check (defined? 'for-each) => #t)
  (check (defined? 'ahash-set!) => #t)
  (check (defined? 'ahash-ref) => #t)
  (check (defined? 'make-ahash-table) => #t)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-regexp) (test-regexp-match) (check-report))
