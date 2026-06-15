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

(load "./TeXmacs/plugins/latex/progs/init-latex.scm")

(check-set-mode! 'report-failed)

(define (collect-spaced-differentials t)
  (define (collect-list lst)
    (if (or (null? lst) (null? (cdr lst)) (null? (cddr lst)))
      '()
      (let ((a (car lst)) (b (cadr lst)) (c (caddr lst)))
        (if (and (string? a) (string=? a "d") (string? b) (string=? b " ") (string? c))
          (cons c (collect-list (cddr lst)))
          (collect-list (cdr lst))
        ) ;if
      ) ;let
    ) ;if
  ) ;define
  (define (collect-children lst)
    (if (null? lst)
      '()
      (append (collect-spaced-differentials (car lst)) (collect-children (cdr lst)))
    ) ;if
  ) ;define
  (cond ((not (pair? t)) '())
        ((eq? (car t) 'concat)
         (append (collect-list (cdr t)) (collect-children (cdr t)))
        ) ;
        (else (collect-children (cdr t)))
  ) ;cond
) ;define

(define (stree-has-all-spaced-differentials? t)
  (let ((collected (collect-spaced-differentials t))
        (expected '("x"
                    "y"
                    "z"
                    "r"
                    "<rho>"
                    "<varrho>"
                    "<theta>"
                    "<vartheta>"
                    "t"
                    "u"
                    "v"
                    "w"
                    "<tau>"
                    "<upsilon>"
                    "<phi>"
                    "<varphi>"
                    "<omega>")
        ) ;expected
       ) ;
    (define (all-expected? lst)
      (if (null? lst) #t (and (member (car lst) collected) (all-expected? (cdr lst))))
    ) ;define
    (display* "Collected spaced differentials: " collected "\n")
    (all-expected? expected)
  ) ;let
) ;define

(define (load-latex path)
  (with path
    (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")
  ) ;with
) ;define

(define (test-latex-document-differentials)
  (display "Testing space insertion for differentials in LaTeX document import...\n"
  ) ;display
  (let* ((latex-content (load-latex "0616_differential_test.tex"))
         (texmacs-tree (latex-document->texmacs latex-content))
         (st (tree->stree texmacs-tree))
        ) ;
    (display* "LaTeX Document converted tree TMU: "
      (serialize-tmu texmacs-tree)
      "\n"
    ) ;display*
    (check (stree-has-all-spaced-differentials? st) => #t)
  ) ;let*
) ;define

(tm-define (test_0616) (test-latex-document-differentials) (check-report))
