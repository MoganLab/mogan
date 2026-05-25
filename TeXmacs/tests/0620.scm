;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0620.scm
;; DESCRIPTION : Integration tests for PR 0620 LaTeX selective paste preamble filtering
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/convert/latex/init-latex.scm")

(check-set-mode! 'report-failed)

(define (test-latex-snippet-preamble-filter)
  (display "Testing preamble filtering for selective LaTeX paste/snippet...\n")
  (let* ((latex-content "\\documentclass{article}\n\\usepackage{amsmath}\n\\begin{document}\na+b\n\\end{document}")
         (parsed (parse-latex latex-content))
         (texmacs-tree (latex->texmacs parsed))
         (st (tree->stree texmacs-tree))
         (st-str (object->string st)))
    (display* "LaTeX Snippet converted tree stree: " st-str "\n")
    ;; The resulting stree should NOT contain 'documentclass, 'usepackage, or 'begin with 'document
    (check (string-contains? st-str "documentclass") => #f)
    (check (string-contains? st-str "usepackage") => #f)
    (check (string-contains? st-str "begin") => #f)))

(tm-define (test_0620)
  (test-latex-snippet-preamble-filter)
  (check-report))
