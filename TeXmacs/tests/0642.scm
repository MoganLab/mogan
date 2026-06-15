;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0642.scm
;; DESCRIPTION : Unit test for HTML export of double (iint) and triple (iiint) integrals
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-iint-iiint-html-export)
  (display "Verifying HTML export of iint and iiint...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0642.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html))
        ) ;
    (display "Exported HTML content:\n")
    (display html-content)
    (display "\n")
    ;; The HTML export must render iint and iiint correctly.
    ;; If the bug is present, they will render as '<mo>iint</mo>' and '<mo>iiint</mo>'
    (check (string-contains? html-content "<mo>iint</mo>") => #f)
    (check (string-contains? html-content "<mo>iiint</mo>") => #f)
    (check (or (string-contains? html-content "&Int;")
               (string-contains? html-content "&iint;")
               (string-contains? html-content "∬")
               (string-contains? html-content "&#x222C;")
               (string-contains? html-content "&#8748;")) => #t)
    (check (or (string-contains? html-content "&iiint;")
               (string-contains? html-content "∭")
               (string-contains? html-content "&#x222D;")
               (string-contains? html-content "&#8749;")) => #t)
    (url-remove tmp-html)
  ) ;let*
) ;define

(tm-define (test_0642)
  (test-iint-iiint-html-export)
  (check-report)
) ;tm-define
