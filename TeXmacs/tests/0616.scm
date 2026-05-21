;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0616.scm
;; DESCRIPTION : Integration tests for PR 0616 differential conversion (LaTeX/Markdown)
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/convert/latex/init-latex.scm")

(check-set-mode! 'report-failed)

(define (stree-has-mathd? t)
  (cond ((string? t) (string=? t "<mathd>"))
        ((pair? t)
         (or (stree-has-mathd? (car t))
             (stree-has-mathd? (cdr t))))
        (else #f)))

(define (load-latex path)
  (with path (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")))

(define (load-markdown path)
  (with path (string-append "$TEXMACS_PATH/tests/md/" path)
    (string-load path)))

(define (test-latex-document-differentials)
  (display "Testing differentials in LaTeX document import...\n")
  (let* ((latex-content (load-latex "0616_differential_test.tex"))
         (texmacs-tree (latex-document->texmacs latex-content))
         (st (tree->stree texmacs-tree)))
    (display* "LaTeX Document converted tree stree: " st "\n")
    (check (stree-has-mathd? st) => #t)))

(define (test-markdown-differentials)
  (display "Testing differentials inside Markdown math snippets...\n")
  ;; We check that each TeX math snippet (wrapped in math mode) from our
  ;; Markdown file is correctly converted to include the upright <mathd> operator.
  (let* ((snippets '("\\( dx \\)"
                     "\\( dy = dz \\)"
                     "\\( dr \\)"
                     "\\( d\\rho \\)"
                     "\\( d\\theta \\)"
                     "\\( x dx + y dy = z dz \\)"
                     "\\( \\sin \\theta d\\theta \\)")))
    (for-each (lambda (s)
                (let* ((parsed (parse-latex s))
                       (converted (latex->texmacs parsed))
                       (st (tree->stree converted)))
                  (display* "Snippet: " s " => " st "\n")
                  (check (stree-has-mathd? st) => #t)))
              snippets)))

(tm-define (test_0616)
  (test-latex-document-differentials)
  (test-markdown-differentials)
  (check-report))
