;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0616_indices_test.scm
;; DESCRIPTION : Integration tests for math index splitting (LaTeX Import)
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(check-set-mode! 'report-failed)

(define (load-latex path)
  (with path
    (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")
  ) ;with
) ;define

(define (collect-rsub-strings t)
  (define (collect-children lst)
    (if (null? lst)
      '()
      (append (collect-rsub-strings (car lst)) (collect-children (cdr lst)))
    ) ;if
  ) ;define
  (cond ((not (pair? t)) '())
        ((eq? (car t) 'rsub) (list (cadr t)))
        (else (collect-children (cdr t)))
  ) ;cond
) ;define

(define (test-latex-indices)
  (display "Testing space and layout splitting for indices in LaTeX import...\n")
  (let* ((latex-content (load-latex "0616_indices_test.tex"))
         (texmacs-tree (latex-document->texmacs latex-content))
         (st (tree->stree texmacs-tree))
         (rsub-nodes (collect-rsub-strings st))
        ) ;
    (display* "Converted tree TMU: " (serialize-tmu texmacs-tree) "\n")
    (display* "Collected rsub nodes: " rsub-nodes "\n")

    ;; Each should be split into individual character concat list or components
    ;; instead of single strings like "ij", "jk" etc.
    (check (member "ij" rsub-nodes) => #f)
    (check (member "jk" rsub-nodes) => #f)
    (check (member "ijk" rsub-nodes) => #f)
    (check (member "kl" rsub-nodes) => #f)
    (check (member "lm" rsub-nodes) => #f)
    (check (member "mn" rsub-nodes) => #f)
    (check (member "pq" rsub-nodes) => #f)
    (check (member "rs" rsub-nodes) => #f)
    (check (member "xy" rsub-nodes) => #f)
    (check (member "ab" rsub-nodes) => #f)
    (check (member "cd" rsub-nodes) => #f)
    (check (member "uv" rsub-nodes) => #f)
    (check (member "xyz" rsub-nodes) => #f)
  ) ;let*
) ;define

(tm-define (test_0616_indices_test) (test-latex-indices) (check-report))
