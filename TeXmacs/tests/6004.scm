;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6004.scm
;; DESCRIPTION : Test TOC in unpaginated (papyrus) vs paginated (paper) modes
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/generic/document-edit.scm")
(import (liii check))

(define (test_6004)
  (check-set-mode! 'report-failed)

  (let* ((fixture (url-append (get-texmacs-path) "tests/stem/6004.stem")))
    (load-buffer fixture)

    ;; 1. Unpaginated mode (continuous scroll / papyrus):
    ;; In this mode, TOC titles have hyperlinks (locus) but no dots (datoms) and no page numbers (pageref).
    (update-forced)
    (generate-aux "table-of-contents")
    (let ((toc-t (get-auxiliary "toc")))
      (check
        (nnull? (tree-search toc-t (lambda (t) (tm-func? t 'locus))))
        =>
        #t
      ) ;check
      (check
        (nnull? (tree-search toc-t (lambda (t) (tm-func? t 'pageref))))
        =>
        #f
      ) ;check
      (check
        (nnull? (tree-search toc-t (lambda (t) (tm-func? t 'datoms))))
        =>
        #f
      ) ;check
    ) ;let

    ;; 2. Paginated mode (single page / paper):
    ;; In this mode, TOC titles have hyperlinks, dots, and page references.
    (init-page-rendering "paper")
    (update-current-buffer)
    (update-forced)
    (generate-aux "table-of-contents")
    (let ((toc-t (get-auxiliary "toc")))
      (check
        (nnull? (tree-search toc-t (lambda (t) (tm-func? t 'locus))))
        =>
        #t
      ) ;check
      (check
        (nnull? (tree-search toc-t (lambda (t) (tm-func? t 'pageref))))
        =>
        #t
      ) ;check
      (check
        (nnull? (tree-search toc-t (lambda (t) (tm-func? t 'datoms))))
        =>
        #t
      ) ;check
    ) ;let
  ) ;let*

  (check-report)
) ;define
