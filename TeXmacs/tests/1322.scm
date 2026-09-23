;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1322.scm
;; DESCRIPTION : Test TOC / all-sections with chapter* documents (issue 1322)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/text/text-menu.scm")
(import (liii check))

(define (test_1322)
  (check-set-mode! 'report-failed)
  ;; Check predicate behavior directly
  (check (is-book-top-level (stree->tree '(chapter* "Test"))) => #t)
  (check (is-book-top-level (stree->tree '(appendix* "Test"))) => #t)
  (check (is-book-top-level (stree->tree '(part* "Test"))) => #t)
  (check (is-section-top-level (stree->tree '(section* "Test"))) => #t)

  ;; Load 1322.stem fixture which has 45 (> 42) chapter* items
  (let* ((fixture (url-append (get-texmacs-path) "tests/stem/1322.stem")))
    (load-buffer fixture)
    (let* ((raw (tree-search-sections (buffer-tree))) (sections (all-sections)))
      (check (length raw) => 45)
      (check (length sections) => 45)
    ) ;let*
  ) ;let*
  (check-report)
) ;define
