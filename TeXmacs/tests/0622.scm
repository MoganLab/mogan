;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0622.scm
;; DESCRIPTION : Unit and Integration tests for MathML/HTML export of dfrac, tfrac, cfrac
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

(define (test-dfrac-html-export-integration)
  (display "Verifying end-to-end HTML/MathML export with buffer-export...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0622_infinity_sizes.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html)))
    ;; Under full buffer export, the fixed dfrac must properly expand to mfrac in MathML!
    (check (string-contains? html-content "mfrac") => #t)
    (check (string-contains? html-content "偶数") => #t)
    (check (string-contains? html-content "奇数") => #t)
    
    ;; Also verify that LaTeX raw spacing artifacts like [6pt] have been cleanly removed!
    (check (string-contains? html-content "6pt") => #f)
    (check (string-contains? html-content "2pt") => #f)))

(tm-define (test_0622)
  (test-dfrac-env-patch-exclusion)
  (test-dfrac-html-export-integration)
  (check-report))
