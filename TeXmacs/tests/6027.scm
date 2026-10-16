;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6027.scm
;; DESCRIPTION : Integration test for outline default expansion rules 1 & 2
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/text/text-outline.scm")
(import (liii check))

(define (load-outline-fixture name)
  (let* ((p1 (url-append (get-texmacs-path) (string-append "tests/stem/" name)))
         (p2 (string-append "./TeXmacs/tests/stem/" name))
         (fixture (if (url-exists? p1) p1 p2))
        ) ;
    (load-buffer fixture)
    (document-outline)
  ) ;let*
) ;define

(define (test_6027)
  (check-set-mode! 'report-failed)

  ;; Case 1: 6027_01_single_child.stem
  ;; Rule 1 base: Part I expands, Chapter 1 & 2 visible
  (let ((outline (load-outline-fixture "6027_01_single_child.stem")))
    (check (length outline) => 1)
    (check (outline-default-expanded-titles outline) => '("Part I"))
    (check (cdr (outline-default-expanded-state outline)) => 3)
  ) ;let

  ;; Case 2: 6027_02_single_child_chain.stem
  ;; Rule 1 recursive: Part I and Chapter 1 expand, Sections visible
  (let ((outline (load-outline-fixture "6027_02_single_child_chain.stem")))
    (check (length outline) => 1)
    (check (outline-default-expanded-titles outline) => '("Part I" "Chapter 1"))
    (check (cdr (outline-default-expanded-state outline)) => 4)
  ) ;let

  ;; Case 3: 6027_03_has_leaf_sibling.stem
  ;; Rule 1 boundary: sibling leaf blocks penetration; 30 sections exceed threshold
  (let ((outline (load-outline-fixture "6027_03_has_leaf_sibling.stem")))
    (check (length outline) => 2)
    (check (outline-default-expanded-titles outline) => '())
    (check (cdr (outline-default-expanded-state outline)) => 2)
  ) ;let

  ;; Case 4: 6027_04_large_book.stem
  ;; Rule 2: 60 items > 30, Layer 2 remains collapsed
  (let ((outline (load-outline-fixture "6027_04_large_book.stem")))
    (check (length outline) => 10)
    (check (outline-default-expanded-titles outline) => '())
    (check (cdr (outline-default-expanded-state outline)) => 10)
  ) ;let

  ;; Case 5: 6027_05_small_article.stem
  ;; Rule 2: 9 items <= 30, all chapters expand
  (let ((outline (load-outline-fixture "6027_05_small_article.stem")))
    (check (length outline) => 3)
    (check (outline-default-expanded-titles outline)
      =>
      '("Chapter 1" "Chapter 2" "Chapter 3")
    ) ;check
    (check (cdr (outline-default-expanded-state outline)) => 9)
  ) ;let

  ;; Case 6: 6027_06_boundary_exact_30.stem
  ;; Rule 2 boundary: exactly 30 items <= 30, all chapters expand
  (let ((outline (load-outline-fixture "6027_06_boundary_exact_30.stem")))
    (check (length outline) => 5)
    (check (outline-default-expanded-titles outline)
      =>
      '("Chapter 1" "Chapter 2" "Chapter 3" "Chapter 4" "Chapter 5")
    ) ;check
    (check (cdr (outline-default-expanded-state outline)) => 30)
  ) ;let

  ;; Case 7: 6027_07_boundary_over_31.stem
  ;; Rule 2 boundary: 31 items > 30, chapters remain collapsed
  (let ((outline (load-outline-fixture "6027_07_boundary_over_31.stem")))
    (check (length outline) => 5)
    (check (outline-default-expanded-titles outline) => '())
    (check (cdr (outline-default-expanded-state outline)) => 5)
  ) ;let

  ;; Case 8: 6027_08_deep_three_levels.stem
  ;; Rule 2 deep: 14 items <= 30, chapters and sections expand
  (let ((outline (load-outline-fixture "6027_08_deep_three_levels.stem")))
    (check (length outline) => 2)
    (check (outline-default-expanded-titles outline)
      =>
      '("Chapter 1" "Chapter 2" "Section 1.1" "Section 1.2" "Section 2.1"
        "Section 2.2")
    ) ;check
    (check (cdr (outline-default-expanded-state outline)) => 14)
  ) ;let

  ;; Case 9: 6027_09_flat_leaves.stem
  ;; Flat leaves: 5 sections, none expandable
  (let ((outline (load-outline-fixture "6027_09_flat_leaves.stem")))
    (check (length outline) => 5)
    (check (outline-default-expanded-titles outline) => '())
    (check (cdr (outline-default-expanded-state outline)) => 5)
  ) ;let

  ;; Case 10: 6027_10_empty_outline.stem
  ;; Empty outline: 0 items
  (let ((outline (load-outline-fixture "6027_10_empty_outline.stem")))
    (check (length outline) => 0)
    (check (outline-default-expanded-titles outline) => '())
    (check (cdr (outline-default-expanded-state outline)) => 0)
  ) ;let

  (check-report)
) ;define
