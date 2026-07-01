;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0653.scm
;; DESCRIPTION : Unit tests for accent pseudo-variants and focus tag names
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (tests 653) (:use (math math-menu)))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-accent-focus-tag-names)
  (check (focus-tag-name 'tilde) => "Tilda ~")
  (check (focus-tag-name 'hat) => "Hat ^")
  (check (focus-tag-name 'overbrace) => "Overbrace ⏞")
  (check (focus-tag-name 'rightarrow) => "Right arrow →")
  (let ((t (tm->tree '(wide "x" "~"))))
    (check (get-accent-variant t) => 'tilde)
    (check (get-accent-variants-list t)
      =>
      '(tilde hat bar vector check breve invbreve)
    ) ;check
    (variant-set t 'hat)
    (check (tree->string (tree-ref t 1)) => "^")
    (check (get-accent-variant t) => 'hat)
  ) ;let
  (let ((t* (tm->tree '(wide* "x" "~"))))
    (check (get-accent-variant t*) => 'tilde)
    (check (get-accent-variants-list t*)
      =>
      '(tilde hat bar vector check breve invbreve)
    ) ;check
    (variant-set t* 'hat)
    (check (tree->string (tree-ref t* 1)) => "^")
    (check (get-accent-variant t*) => 'hat)
  ) ;let
) ;define

(tm-define (test_0653) (test-accent-focus-tag-names) (check-report))
