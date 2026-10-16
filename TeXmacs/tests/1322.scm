;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1322.scm
;; DESCRIPTION : Test document outline with chapter* documents (issue 1322)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/text/text-outline.scm")
(import (liii check))

(define (test_1322)
  (check-set-mode! 'report-failed)
  ;; Load 1322.stem fixture which has 45 (> 42) chapter* items
  (let* ((fixture (url-append (get-texmacs-path) "tests/stem/1322.stem")))
    (load-buffer fixture)
    (let* ((raw (tree-search-sections (buffer-tree))) (outline (document-outline)))
      (check (length raw) => 45)
      (check (length outline) => 45)
      (check (car (car outline)) => "Chapter 1")
    ) ;let*
  ) ;let*
  (check-report)
) ;define
