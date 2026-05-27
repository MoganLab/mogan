;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0622.scm
;; DESCRIPTION : Unit tests for PR 0622 MathML/HTML export environment patch
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/html/progs/convert/html/tmhtml-expand.scm")

(check-set-mode! 'report-failed)

(define (patch-has-macro? patch macro-name)
  (let loop ((lst (cdr patch)))
    (cond ((null? lst) #f)
          ((and (pair? (car lst))
                (eq? (caar lst) 'associate)
                (string=? (cadar lst) macro-name))
           #t)
          (else (loop (cdr lst))))))

(define (test-dfrac-env-patch-exclusion)
  (display "Verifying that dfrac, tfrac, cfrac are excluded from tmhtml-env-patch...\n")
  (let ((patch (tmhtml-env-patch)))
    ;; They must NOT be in the environment patch, so that they expand normally to frac during export
    (check (patch-has-macro? patch "dfrac") => #f)
    (check (patch-has-macro? patch "tfrac") => #f)
    (check (patch-has-macro? patch "cfrac") => #f)
    
    ;; Some other standard environment macros (like TeXmacs, binom, etc.) should still be present
    (check (patch-has-macro? patch "TeXmacs") => #t)
    (check (patch-has-macro? patch "binom") => #t)))

(tm-define (test_0622)
  (test-dfrac-env-patch-exclusion)
  (check-report))
