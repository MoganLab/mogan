;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 24_15.scm
;; DESCRIPTION : Test for docx.scm
;; COPYRIGHT   : (C) 2024  ATQlove
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (srfi srfi-78))
(use-modules (docx docx-format))

(define (tm->docx tm-file-url docx-file-url)
  (load-buffer tm-file-url)

  (export-buffer-main (current-buffer) docx-file-url "docx" ())

  (if (url-exists? docx-file-url)
    (display* "Export successful: " docx-file-url "\n")
    (display* "Export failed: " docx-file-url "\n")
  ) ;if

  (url-exists? docx-file-url)
) ;define

(define (test_24_15)
  (when (os-macos?)
    (exit 0)
  ) ;when
  (display (url-exists? (system->url "$TEXMACS_PATH/tests/tm/24_15.tm")))
  (let* ((tm-url "$TEXMACS_PATH/tests/tm/24_15.tm")
         (docx-url "$TEXMACS_PATH/tests/tm/24_15.docx")
         (result (tm->docx tm-url docx-url))
        ) ;
    (check result => #t)
    (check-report)
    (if (check-failed?) (exit -1))
  ) ;let*
) ;define
