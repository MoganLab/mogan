
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tmlength-test.scm
;; DESCRIPTION : test suite for length library
;; COPYRIGHT   : (C) 2002  David Allouche
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (convert tools tmlength-test) (:use (convert tools tmlength)))

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for string->tmlength
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-string->tmlength)
  (define (list->tmlength l)
    (apply tmlength l)
  ) ;define
  (check (string->tmlength "") => (list->tmlength '()))
  (check (string->tmlength "0cm") => (list->tmlength '(0 cm)))
  (check (string->tmlength "px") => (list->tmlength '(0 px)))
  (check (string->tmlength "123.456mm") => (list->tmlength '(123.456 mm)))
  (check (string->tmlength "-1in") => (list->tmlength '(-1 in)))
  (check (string->tmlength "--2fns") => (list->tmlength '(2 fns)))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for length-decode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-length-decode)
  (check (length-decode "1in") => 153600)
  (check (length-decode "1tmpt") => 1)
  (check (length-decode "1cm") => 60472)
  (check (length-decode "1mm") => 6047)
  (check (length-decode "1pt") => 2125)
  (check (length-decode "1bp") => 2133)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-tmlength)
  (test-string->tmlength)
  (test-length-decode)
  (check-report)
) ;tm-define
