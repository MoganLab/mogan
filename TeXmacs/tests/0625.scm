;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0625.scm
;; DESCRIPTION : Unit and Integration tests for MathML/HTML export of space/hspace in math mode
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check)
        (liii path))

(check-set-mode! 'report-failed)

(define (test-math-space-export-integration)
  (display "Verifying end-to-end HTML/MathML export with space/hspace in math mode...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0625_piecewise.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html)))
    (path-write-text "0625_out.html" html-content)
    (display* "Exported HTML length: " (string-length html-content) "\n")
    ;; In MathML mode, the <space|1em> should be exported as `<mspace width="1em"/>` or `<m:mspace width="1em"/>`!
    (check (string-contains? html-content "mspace") => #t)
    (check (string-contains? html-content "width=\"1em\"") => #t)
    
    ;; Also verify that the left brace of the piecewise choice block has stretchy="true" to scale vertically!
    (check (string-contains? html-content "stretchy=\"true\"") => #t)))

(tm-define (test_0625)
  (test-math-space-export-integration)
  (check-report))
