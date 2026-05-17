
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-define-test.scm
;; DESCRIPTION : Test suite for tm-define
;; COPYRIGHT   : (C) 2021  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (prog prog-format-test) (:use (kernel texmacs tm-define)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-define-procedure)
  (display "This is a test procedure defined with define\n")
) ;define

(tm-define (test-tm-define-procedure)
  :synopsis
  "Test procedure defined via tm-define"
  (display "This is a test procedure defined with tm-define\n")
) ;tm-define

(tm-define alias-car car)

(define (test-procedure-name)
  ;; (tm-define s2f string->float)
  (check (procedure-name string->float) => 's2f)
  (check (procedure-name car) => 'alias-car)
  (check (procedure-name utf8->cork) => 'utf8->cork)
  (check (procedure-name system) => 'system)
  (check (procedure-name test-define-procedure) => 'test-define-procedure)
  (check (procedure-name test-tm-define-procedure) => 'test-tm-define-procedure)
  (check (procedure-name 1) => #f)
  ;; Test lambda serialization - anonymous lambdas should return #f
  (let ((lambda-func (lambda (x) (+ x 1))))
    (check (procedure-name lambda-func) => #f)
  ) ;let
  ;; Test that different anonymous lambdas all return #f
  (let ((lambda1 (lambda () 1)) (lambda2 (lambda () 2)))
    (check (procedure-name lambda1) => #f)
    (check (procedure-name lambda2) => #f)
  ) ;let
) ;define

(tm-define (regtest-tm-define) (test-procedure-name) (check-report))
