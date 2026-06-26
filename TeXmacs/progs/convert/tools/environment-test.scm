
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : xmltm-test.scm
;; DESCRIPTION : Test suite for converter environments.
;; COPYRIGHT   : (C) 2003  David Allouche
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (convert tools environment-test)
  (:use (convert tools environment))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dynamic environments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-environment-base)
  (check (let ((env (environment)))
           (with-environment env
            ((foo "bar") (spam (string-append "eg" "gs")))
            (list (environment-ref env foo) (environment-ref env spam))
           ) ;with-environment
         ) ;let
    =>
    '("bar" "eggs")
  ) ;check
  (check (let ((env (environment)) (out '()))
           (with-environment env
            ((foo "bar"))
            (set-rcons! out (environment-ref env foo))
            (with-environment env ((foo "baz")) (set-rcons! out (environment-ref env foo)))
            (set-rcons! out (environment-ref env foo))
           ) ;with-environment
           out
         ) ;let
    =>
    '("bar" "baz" "bar")
  ) ;check
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-environment) (test-environment-base) (check-report))
