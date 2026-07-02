;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0640.scm
;; DESCRIPTION : Integration test for whole file LaTeX export preamble preservation
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(tm-define (test_0640)
  (display "Testing whole file LaTeX export preamble preservation...\n")
  (with path
    (string-append "$TEXMACS_PATH/tests/tmu/0640.tmu")
    (with tmpfile
      (url-temp)
      (load-buffer path)
      (buffer-export path tmpfile "latex")
      (with content
        (string-load tmpfile)
        (display* "Exported LaTeX Content:\n" content "\n")
        (check (string-contains? content "documentclass") => #t)
        (check (string-contains? content "begin{document}") => #t)
        (check (string-contains? content "end{document}") => #t)
      ) ;with
    ) ;with
  ) ;with

  (display "Testing whole file LaTeX export when latex-source has no preamble...\n"
  ) ;display
  (with doc
    '(document (TeXmacs "1.1.0")
       (style "article")
       (body "Hello World")
       (attachments (collection (associate "latex-source" "Hello World")
                      (associate "latex-target" (document "Hello World")))))
    (with tmpfile
      (url-append (url-temp-dir) "test.tmu")
      (string-save (serialize-tmu (stree->tree doc)) tmpfile)
      (load-buffer tmpfile)
      (with dest
        (url-append (url-temp-dir) "test.tex")
        (buffer-export tmpfile dest "latex")
        (with content
          (string-load dest)
          (display* "Exported LaTeX Content with snippet source:\n" content "\n")
          (check (string-contains? content "documentclass") => #t)
          (check (string-contains? content "begin{document}") => #t)
          (check (string-contains? content "end{document}") => #t)
        ) ;with
      ) ;with
    ) ;with
  ) ;with

  (display "Testing whole file LaTeX export via texmacs->latex-document directly...\n")
  (with path (string-append "$TEXMACS_PATH/tests/tmu/0640.tmu")
    (load-buffer path)
    (switch-to-buffer* path)
    (display* "STREE-DOC: " (tm->stree (buffer-get (current-buffer))) "\n")
    (with content (texmacs->latex-document (buffer-get (current-buffer)) '(("texmacs->latex:progress" . "on")))
      (display* "Direct Scheme Exported LaTeX Content:\n" content "\n")
      (check (string-contains? content "documentclass") => #t)
      (check (string-contains? content "begin{document}") => #t)
      (check (string-contains? content "end{document}") => #t)
    ) ;with
  ) ;with

  (check-report)
) ;tm-define
