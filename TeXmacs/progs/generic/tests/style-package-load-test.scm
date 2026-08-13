;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : style-package-load-test.scm
;; DESCRIPTION : Verify .stem packages can be loaded via add-style-package
;; COPYRIGHT   : (C) 2026  Da Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)

;; Test 1: stem package file should exist

(define (test-stem-package-exists)
  (check (url-exists? (url-append "$TEXMACS_STYLE_PATH" "python.stem")) => #t)
) ;define

;; Test 2: ts package should NOT exist (was deleted in 1131)

(define (test-ts-package-absent)
  (check (url-exists? (url-append "$TEXMACS_STYLE_PATH" "python.ts")) => #f)
) ;define

;; Test 3: env package converted to .stem in 1202

(define (test-env-stem-package-exists)
  (check (url-exists? (url-append "$TEXMACS_STYLE_PATH" "env.stem")) => #t)
  (check (url-exists? (url-append "$TEXMACS_STYLE_PATH" "env-base.stem")) => #t)
) ;define

;; Test 4: env .ts packages should NOT exist (deleted in 1202)

(define (test-env-ts-package-absent)
  (check (url-exists? (url-append "$TEXMACS_STYLE_PATH" "env.ts")) => #f)
  (check (url-exists? (url-append "$TEXMACS_STYLE_PATH" "env-base.ts")) => #f)
) ;define

(tm-define (regtest-style-package-load)
  (display "=== Running style-package-load tests ===\n")
  (test-stem-package-exists)
  (test-ts-package-absent)
  (test-env-stem-package-exists)
  (test-env-ts-package-absent)
  (check-report)
) ;tm-define
