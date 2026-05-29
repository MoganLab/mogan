;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0628.scm
;; DESCRIPTION : Tests for frame/framed export and mdframed LaTeX import mapping
;; COPYRIGHT   : (C) 2026  Jack Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(define (export-as-latex-and-load path)
  (with path
    (string-append "$TEXMACS_PATH/tests/tmu/" path)
    (with tmpfile
      (url-temp)
      (load-buffer path)
      (buffer-export path tmpfile "latex")
      (string-load tmpfile)
    ) ;with
  ) ;with
) ;define

(define (load-latex path)
  (with path
    (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")
  ) ;with
) ;define

(define (test-mdframed-import)
  (let* ((tex-content (string-replace (string-load "$TEXMACS_PATH/tests/tex/0628_mdframed_import.tex")
                        "\r\n"
                        "\n"
                      ) ;string-replace
         ) ;tex-content
         (parsed (parse-latex tex-content))
        ) ;
    (check (tree->stree (latex->texmacs parsed))
      =>
      '(document (mdframed (document "hello")))
    ) ;check
  ) ;let*
) ;define

(tm-define (test_0628)
  (check (export-as-latex-and-load "0628.tmu")
    =>
    (load-latex "0628_frame_export.tex")
  ) ;check
  (test-mdframed-import)
  (check-report)
) ;tm-define
