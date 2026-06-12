
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-edit-test.scm
;; DESCRIPTION : Test suite for magic paste limit
;; COPYRIGHT   : (C) 2025  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic generic-edit-test)
  (:use (generic generic-edit))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for magic-paste-excluded? logic
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (magic-paste-excluded? fm)
  (or (== fm "image") (== fm "verbatim") (== fm "internal")))

(define (test-magic-paste-excluded?)
  (check (magic-paste-excluded? "image") => #t)
  (check (magic-paste-excluded? "verbatim") => #t)
  (check (magic-paste-excluded? "internal") => #t)
  (check (magic-paste-excluded? "md") => #f)
  (check (magic-paste-excluded? "latex") => #f)
  (check (magic-paste-excluded? "html") => #f)
  (check (magic-paste-excluded? "ocr") => #f)
  (check (magic-paste-excluded? "mathml") => #f)
  (check (magic-paste-excluded? "code") => #f)
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for error results and paste action classification
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (classify result)
  (cond ((== result "allowed") 'allowed)
        ((== result "not-logged-in") 'not-logged-in)
        ((== result "limit-exceeded") 'limit-exceeded)
        ((string-starts? result "error:") 'error)
        (else 'unknown)))

(define (test-error-results)
  ;; HTTP status code errors
  (check (classify "error:400") => 'error)
  (check (classify "error:404") => 'error)
  (check (classify "error:408") => 'error)
  (check (classify "error:429") => 'error)
  (check (classify "error:500") => 'error)
  (check (classify "error:502") => 'error)
  (check (classify "error:503") => 'error)
  (check (classify "error:504") => 'error)
  ;; Scheme exception keys
  (check (classify "error:http-error") => 'error)
  (check (classify "error:system-error") => 'error)
  (check (classify "error:read-error") => 'error)
  ;; Non-error results
  (check (classify "allowed") => 'allowed)
  (check (classify "not-logged-in") => 'not-logged-in)
  (check (classify "limit-exceeded") => 'limit-exceeded)
)

(define (classify-paste-action result)
  (cond ((== result "allowed") 'proceed)
        ((== result "not-logged-in") 'ask-login)
        ((== result "limit-exceeded") 'ask-upgrade)
        ((string-starts? result "error:") 'show-error)
        (else 'unknown)))

(define (test-paste-action-on-error)
  ;; HTTP errors -> show error dialog
  (check (classify-paste-action "error:400") => 'show-error)
  (check (classify-paste-action "error:404") => 'show-error)
  (check (classify-paste-action "error:500") => 'show-error)
  (check (classify-paste-action "error:502") => 'show-error)
  (check (classify-paste-action "error:503") => 'show-error)
  (check (classify-paste-action "error:504") => 'show-error)
  (check (classify-paste-action "error:429") => 'show-error)
  ;; Scheme exceptions -> show error dialog
  (check (classify-paste-action "error:http-error") => 'show-error)
  (check (classify-paste-action "error:system-error") => 'show-error)
  (check (classify-paste-action "error:read-error") => 'show-error)
  ;; Known non-error results
  (check (classify-paste-action "allowed") => 'proceed)
  (check (classify-paste-action "not-logged-in") => 'ask-login)
  (check (classify-paste-action "limit-exceeded") => 'ask-upgrade)
)

(tm-define (regtest-generic-edit)
  (test-magic-paste-excluded?)
  (test-error-results)
  (test-paste-action-on-error)
  (check-report))
