;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0655.scm
;; DESCRIPTION : Unit test for HTML (MathML) export of mathscr/cal* characters
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-cal-html-export)
  (display "Verifying HTML export of cal* characters...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0655.tmu")
         (html-file (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (begin
                   (set-preference "texmacs->html:mathml" "on")
                   (buffer-export tmu-path html-file "html"))))
    (with s (string-load html-file)
      ;; Verify that cal*-A, cal*-B, cal*-C are exported as mathvariant="script"
      (check (string-occurs? "mathvariant=\"script\"" s) => #t)
      (check (string-occurs? "mathvariant=\"script\">A" s) => #t)
      (check (string-occurs? "mathvariant=\"script\">B" s) => #t)
      (check (string-occurs? "mathvariant=\"script\">C" s) => #t)
      (check (string-occurs? "?" s) => #f)
      (url-remove html-file)
    ) ;with
  ) ;let*
) ;define

(tm-define (test_0655)
  (test-cal-html-export)
  (check-report))
