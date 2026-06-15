
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

(texmacs-module (generic generic-edit-test) (:use (generic generic-edit)))

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for magic-paste-excluded? logic
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (magic-paste-excluded? fm)
  (or (== fm "image") (== fm "verbatim") (== fm "internal"))
) ;define

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
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for check-magic-paste result handling
;; Only 401 and 403 block; all other status codes and exceptions pass through
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Simulates the result from check-magic-paste for a given HTTP status

(define (check-result-for-status status)
  (cond ((= status 200) "allowed")
        ((= status 401) "not-logged-in")
        ((= status 403) "limit-exceeded")
        (else "allowed")
  ) ;cond
) ;define

;; Simulates the classification logic of with-magic-paste-check

(define (classify-paste-action result)
  (cond ((== result "allowed") 'proceed)
        ((== result "not-logged-in") 'ask-login)
        ((== result "limit-exceeded") 'ask-upgrade)
        (else 'proceed)
  ) ;cond
) ;define

(define (test-check-result-for-status)
  ;; 200, 401, 403 have specific mappings
  (check (check-result-for-status 200) => "allowed")
  (check (check-result-for-status 401) => "not-logged-in")
  (check (check-result-for-status 403) => "limit-exceeded")
  ;; All other HTTP status codes -> "allowed" (pass-through)
  (check (check-result-for-status 400) => "allowed")
  (check (check-result-for-status 404) => "allowed")
  (check (check-result-for-status 408) => "allowed")
  (check (check-result-for-status 429) => "allowed")
  (check (check-result-for-status 500) => "allowed")
  (check (check-result-for-status 502) => "allowed")
  (check (check-result-for-status 503) => "allowed")
  (check (check-result-for-status 504) => "allowed")
  ;; Timeout (status 0)
  (check (check-result-for-status 0) => "allowed")
) ;define

(define (test-paste-action)
  (check (classify-paste-action "allowed") => 'proceed)
  (check (classify-paste-action "not-logged-in") => 'ask-login)
  (check (classify-paste-action "limit-exceeded") => 'ask-upgrade)
) ;define

(define (test-network-error-pass-through)
  ;; HTTP server errors -> "allowed" -> proceed
  (check (classify-paste-action (check-result-for-status 500)) => 'proceed)
  (check (classify-paste-action (check-result-for-status 502)) => 'proceed)
  (check (classify-paste-action (check-result-for-status 503)) => 'proceed)
  (check (classify-paste-action (check-result-for-status 504)) => 'proceed)
  ;; Timeout -> "allowed" -> proceed
  (check (classify-paste-action (check-result-for-status 0)) => 'proceed)
  ;; Scheme catch handler returns "allowed" for any exception
  (check (classify-paste-action "allowed") => 'proceed)
) ;define

(tm-define (regtest-generic-edit)
  (test-magic-paste-excluded?)
  (test-check-result-for-status)
  (test-paste-action)
  (test-network-error-pass-through)
  (check-report)
) ;tm-define
