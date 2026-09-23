;;
;; MODULE      : ocr-render.scm
;; DESCRIPTION : ocr-render mock stub for community edition
;;

(define-library (liii ocr-render)
  (export latex-fragment->texmacs
    equation->texmacs-tree
    image-content->texmacs-tree
    html-block->texmacs-tree
    layout-group->texmacs-tree
    layout-node-tag
    layout-node-attrs
    layout-node-children
    layout-attr-ref
    set-latex-conv!
    set-html-conv!
    set-markdown-conv!
  ) ;export
  (import (scheme base))
  (begin
    (define (set-latex-conv! proc)
      #t
    ) ;define

    (define (set-html-conv! proc)
      #t
    ) ;define

    (define (set-markdown-conv! proc)
      #t
    ) ;define

    (define (latex-fragment->texmacs s)
      ""
    ) ;define

    (define (equation->texmacs-tree s)
      `(equation* ,s)
    ) ;define

    (define (image-content->texmacs-tree content w h caption)
      `(image ,content)
    ) ;define

    (define (html-block->texmacs-tree html . note)
      `(document ,html)
    ) ;define

    (define (layout-group->texmacs-tree node)
      `(document)
    ) ;define

    (define (layout-node-tag node)
      (and (pair? node) (car node))
    ) ;define

    (define (layout-node-attrs node)
      (if (and (pair? node) (pair? (cdr node))) (cadr node) '())
    ) ;define

    (define (layout-node-children node)
      (if (and (pair? node) (pair? (cdr node))) (cddr node) '())
    ) ;define

    (define (layout-attr-ref attrs key default)
      (let ((entry (assoc key attrs)))
        (if entry (cdr entry) default)
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
