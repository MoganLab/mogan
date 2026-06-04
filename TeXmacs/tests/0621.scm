;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0621.scm
;; DESCRIPTION : Unit and Integration tests for align HTML export layout
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(use-modules (convert html tmhtml))

(check-set-mode! 'report-failed)

(define (test-0621-html-export-integration)
  (display "Verifying HTML export of align and equation layouts...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0621.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html))
        ) ;
    ;; We want to make sure the align environment is rendered as a table with style="width: 100%" or width="100%"
    ;; We also want to assert that the right-aligned equation number has proper styling to be far-right.
    (check (string-contains? html-content "style=\"width: 100%\"") => #t)
    (check (string-contains? html-content "text-align: right") => #t)
    ;; Ensure all formula numbers are exported and preserved
    (check (string-contains? html-content "(1)") => #t)
    (check (string-contains? html-content "(2)") => #t)
    (check (string-contains? html-content "(3)") => #t)
    (check (string-contains? html-content "(4)") => #t)
  ) ;let*
) ;define

(tm-define (test_0621)
  (test-0621-html-export-integration)
  (check-report)
) ;tm-define
