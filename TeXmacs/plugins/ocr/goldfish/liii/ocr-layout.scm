;;
;; MODULE      : ocr-layout.scm
;; DESCRIPTION : ocr-layout mock stub for community edition
;;

(define-library (liii ocr-layout)
  (export ocr-blocks->layout
    node-tag
    node-attrs
    node-children
    attr-ref
  ) ;export
  (import (scheme base))
  (begin
    (define (ocr-blocks->layout blocks)
      (cons 'layout (cons '() '()))
    ) ;define

    (define (node-tag node)
      (and (pair? node) (car node))
    ) ;define

    (define (node-attrs node)
      (if (and (pair? node) (pair? (cdr node))) (cadr node) '())
    ) ;define

    (define (node-children node)
      (if (and (pair? node) (pair? (cdr node))) (cddr node) '())
    ) ;define

    (define (attr-ref attrs key default)
      (let ((entry (assoc key attrs)))
        (if entry (cdr entry) default)
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
