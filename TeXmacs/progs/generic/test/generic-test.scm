;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-test.scm
;; DESCRIPTION : Test suite for generic
;; COPYRIGHT   : (C) 2022  Yufeng Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic test generic-test)
  (:use (generic generic-menu) (table table-menu))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for focus-tag-name
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-focus-tag-name)
  (check (focus-tag-name 'bmatrix) => "bmatrix")
  (check (focus-tag-name 'Bmatrix) => "Bmatrix")
  (check (focus-tag-name 'tabular) => "tabular")
  (check (focus-tag-name 'tabular*) => "centered tabular")
  (check (focus-tag-name 'block) => "block")
  (check (focus-tag-name 'block*) => "centered block")
  (check (focus-tag-name 'big-table) => "big table")
) ;define

(define (beamer-switch-allowed? empty? dirty? already-beamer?)
  (or empty? dirty? already-beamer?)
) ;define

(define (test-beamer-switch-allowed?)
  ;; Empty documents may always switch to Beamer.
  (check (beamer-switch-allowed? #t #f #f) => #t)
  ;; A dirty buffer is allowed to switch back to Beamer without requiring a save.
  (check (beamer-switch-allowed? #f #t #f) => #t)
  ;; A document that is already Beamer should not be blocked.
  (check (beamer-switch-allowed? #f #f #t) => #t)
  ;; A non-empty, clean, non-Beamer document remains blocked.
  (check (beamer-switch-allowed? #f #f #f) => #f)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-generic)
  (test-focus-tag-name)
  (test-beamer-switch-allowed?)
  (check-report)
) ;tm-define
