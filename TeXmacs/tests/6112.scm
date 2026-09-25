;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6112.scm
;; DESCRIPTION : Test link-navigate lazy-define symbols (link-mouse-ids, etc.)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(define (test_6112)
  (check-set-mode! 'report-failed)

  ;; 1. Verify symbols in (link link-navigate) are defined via lazy-define
  (check (defined? 'link-mouse-ids) => #t)
  (check (defined? 'link-contains-inner-link?) => #t)
  (check (defined? 'show-hlink-tooltip) => #t)
  (check (defined? 'link-active-upwards) => #t)
  (check (defined? 'link-active-ids) => #t)
  (check (defined? 'link-follow-ids) => #t)

  ;; 2. Verify invoking link-mouse-ids lazily resolves and executes correctly
  (check (link-mouse-ids '()) => '())
  (check (link-mouse-ids '("test-dummy-id")) => '())

  ;; 3. Verify invoking link-contains-inner-link? lazily resolves and executes
  (check (link-contains-inner-link? '()) => #t)
  (check (link-contains-inner-link? '("test-dummy-id")) => #t)

  ;; 4. Verify show-hlink-tooltip can be called safely without unbound errors
  (show-hlink-tooltip '())

  (check-report)
) ;define
