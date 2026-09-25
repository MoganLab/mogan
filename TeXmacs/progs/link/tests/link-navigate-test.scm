;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : link-navigate-test.scm
;; DESCRIPTION : Test link navigation routines and lazy symbol contracts
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (link tests link-navigate-test) (:use (link link-navigate)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-link-mouse-ids)
  ;; link-mouse-ids should filter out focus event identifiers and return empty on null
  (check (link-mouse-ids '()) => '())
) ;define

(define (test-link-contains-inner-link?)
  ;; Empty ids list has no url vertex, so contains inner link returns #t
  (check (link-contains-inner-link? '()) => #t)
) ;define

(define (test-show-hlink-tooltip)
  ;; show-hlink-tooltip on empty list should be no-op and not fail
  (show-hlink-tooltip '())
  (check #t => #t)
) ;define

(tm-define (regtest-link-navigate)
  (test-link-mouse-ids)
  (test-link-contains-inner-link?)
  (test-show-hlink-tooltip)
  (check-report)
) ;tm-define
