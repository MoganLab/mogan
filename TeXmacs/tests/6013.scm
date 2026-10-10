;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6013.scm
;; DESCRIPTION : Test document outline tree generation and navigation
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/text/text-outline.scm")
(import (liii check))

(define (test_6013)
  (check-set-mode! 'report-failed)

  ;; 1. Test section-level mappings
  (check (section-level (stree->tree '(part "Part 1"))) => 0)
  (check (section-level (stree->tree '(part* "Part *"))) => 0)
  (check (section-level (stree->tree '(chapter "Chap 1"))) => 1)
  (check (section-level (stree->tree '(chapter* "Chap *"))) => 1)
  (check (section-level (stree->tree '(appendix "App A"))) => 1)
  (check (section-level (stree->tree '(section "Sec 1"))) => 2)
  (check (section-level (stree->tree '(section* "Sec *"))) => 2)
  (check (section-level (stree->tree '(subsection "Sub 1"))) => 3)
  (check (section-level (stree->tree '(subsubsection "SubSub 1"))) => 4)

  ;; 2. Test build-outline-tree pure logic
  ;; Empty list
  (check (build-outline-tree '()) => '())

  ;; Single item
  (check (build-outline-tree '((2 "Sec 1" "0:0"))) => '(("Sec 1" "0:0" ())))

  ;; Standard nested hierarchy: Chapter -> Section -> Subsection
  (let* ((items '((1 "Chapter 1" "0:0")
                  (2 "Section 1.1" "0:1")
                  (3 "Subsection 1.1.1" "0:2")
                  (2 "Section 1.2" "0:3")
                  (1 "Chapter 2" "0:4"))
         ) ;items
         (tree (build-outline-tree items))
        ) ;
    (check (length tree) => 2)
    ;; Chapter 1 has 2 sections
    (check (car (car tree)) => "Chapter 1")
    (check (cadr (car tree)) => "0:0")
    (let ((c1-children (caddr (car tree))))
      (check (length c1-children) => 2)
      (check (car (car c1-children)) => "Section 1.1")
      ;; Section 1.1 has 1 subsection
      (check (length (caddr (car c1-children))) => 1)
      (check
        (car (car (caddr (car c1-children))))
        =>
        "Subsection 1.1.1"
      ) ;check
      (check (car (cadr c1-children)) => "Section 1.2")
    ) ;let
    ;; Chapter 2 has 0 children
    (check (car (cadr tree)) => "Chapter 2")
    (check (caddr (cadr tree)) => '())
  ) ;let*

  ;; Level jump: Section (2) followed by Subsubsection (4)
  (let* ((items-jump '((2 "Sec 1" "0:0") (4 "Deep 1" "0:1") (2 "Sec 2" "0:2")))
         (tree-jump (build-outline-tree items-jump))
        ) ;
    (check (length tree-jump) => 2)
    (check (length (caddr (car tree-jump))) => 1)
    (check
      (car (car (caddr (car tree-jump))))
      =>
      "Deep 1"
    ) ;check
  ) ;let*

  ;; 3. Test with real buffer: 1322.stem
  (let* ((fixture (url-append (get-texmacs-path) "tests/stem/1322.stem")))
    (load-buffer fixture)
    (let ((outline (document-outline)))
      ;; 1322.stem has 45 chapter* items
      (check (length outline) => 45)
      ;; First item title
      (check (car (car outline)) => "Chapter 1")
      ;; Test outline-go-to on valid path without crashing
      (outline-go-to (cadr (car outline)))
    ) ;let
  ) ;let*

  (check-report)
) ;define
