;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0616.scm
;; DESCRIPTION : Integration tests for PR 0616 differential conversion (LaTeX)
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

(define (stree-has-spaced-differential? t)
  ;; Check for pattern: "d" followed by " " followed by x/y/z/r/<rho>/<theta> etc.
  (define (check-list lst)
    (if (or (null? lst) (null? (cdr lst)) (null? (cddr lst)))
        #f
        (let ((a (car lst))
              (b (cadr lst))
              (c (caddr lst)))
          (if (and (string? a) (string=? a "d")
                   (string? b) (string=? b " ")
                   (string? c)
                   (or (string=? c "x") (string=? c "y") (string=? c "z") (string=? c "r")
                       (string=? c "<rho>") (string=? c "<varrho>")
                       (string=? c "<theta>") (string=? c "<vartheta>")))
              #t
              (check-list (cdr lst))))))
  (define (check-children lst)
    (if (null? lst)
        #f
        (or (stree-has-spaced-differential? (car lst))
            (check-children (cdr lst)))))
  (cond ((not (pair? t)) #f)
        ((eq? (car t) 'concat)
         (or (check-list (cdr t))
             (check-children (cdr t))))
        (else (check-children (cdr t)))))

(define (load-latex path)
  (with path (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")))

(define (test-latex-document-differentials)
  (display "Testing space insertion for differentials in LaTeX document import...\n")
  (let* ((latex-content (load-latex "0616_differential_test.tex"))
         (texmacs-tree (latex-document->texmacs latex-content))
         (st (tree->stree texmacs-tree)))
    (display* "LaTeX Document converted tree TMU: " (serialize-tmu texmacs-tree) "\n")
    (check (stree-has-spaced-differential? st) => #t)))

(tm-define (test_0616)
  (test-latex-document-differentials)
  (check-report))
