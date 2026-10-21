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

  ;; 3. Test outline default expansion algorithm (pure logic)
  ;; 3.1 Empty outline
  (check (outline-default-expanded-paths '()) => '())
  (check (cdr (outline-default-expanded-state '())) => 0)

  ;; 3.2 Rule 1: Single child penetration (Part -> Chapter 1 & 2)
  (let* ((tree-single
           '(("Part I" "0" (("Chapter 1" "0:0" ()) ("Chapter 2" "0:1" ()))))
         ) ;tree-single
        ) ;
    (check (outline-default-expanded-titles tree-single) => '("Part I"))
    (check (outline-default-expanded-paths tree-single) => '("0"))
    (check (cdr (outline-default-expanded-state tree-single)) => 3)
  ) ;let*

  ;; 3.3 Rule 1: Single child chain penetration (Part -> Chapter -> 2 Sections)
  (let* ((tree-chain
           '(("Part I"
              "0"
              (("Chapter 1" "0:0" (("Sec 1" "0:0:0" ()) ("Sec 2" "0:0:1" ()))))))
         ) ;tree-chain
        ) ;
    (check (outline-default-expanded-titles tree-chain) => '("Part I"
                                                             "Chapter 1"))
    (check (outline-default-expanded-paths tree-chain) => '("0" "0:0"))
    (check (cdr (outline-default-expanded-state tree-chain)) => 4)
  ) ;let*

  ;; 3.4 Rule 1 Boundary: Sibling leaf node blocks penetration
  (let* ((tree-leaf-sibling
           '(("Chapter 1" "0" (("Sec 1" "0:0" ()) ("Sec 2" "0:1" ())))
             ("Appendix" "1" ()))
         ) ;tree-leaf-sibling
        ) ;
    ;; Chapter 1 has 2 sections <= 30, so Rule 2 will expand it!
    (check (outline-default-expanded-titles tree-leaf-sibling) => '("Chapter 1"))
    (check (cdr (outline-default-expanded-state tree-leaf-sibling)) => 4)
  ) ;let*

  ;; 3.5 Rule 2 Boundary: 30 items expands, 31 items stays collapsed
  (let* ((chaps-exact-30
           (map
             (lambda (c)
               (list (string-append "Chap " (number->string c))
                 (number->string c)
                 (map
                   (lambda (s) (list (string-append "Sec " (number->string s)) "p" '()))
                   '(0 1 2 3 4)
                 ) ;map
               ) ;list
             ) ;lambda
             '(0 1 2 3 4)
           ) ;map
         ) ;chaps-exact-30
         (chaps-over-31
           (append
             (map
               (lambda (c)
                 (list (string-append "Chap " (number->string c))
                   (number->string c)
                   (map
                     (lambda (s) (list (string-append "Sec " (number->string s)) "p" '()))
                     '(0 1 2 3 4)
                   ) ;map
                 ) ;list
               ) ;lambda
               '(0 1 2 3)
             ) ;map
             (list
               (list "Chap 4"
                 "4"
                 (map
                   (lambda (s) (list (string-append "Sec " (number->string s)) "p" '()))
                   '(0 1 2 3 4 5)
                 ) ;map
               ) ;list
             ) ;list
           ) ;append
         ) ;chaps-over-31
        ) ;
    ;; Exact 30: 5 chapters + 25 sections = 30 -> all expanded
    (check (outline-default-expanded-paths chaps-exact-30)
      =>
      '("0" "1" "2" "3" "4")
    ) ;check
    (check (cdr (outline-default-expanded-state chaps-exact-30)) => 30)
    ;; Over 31: 5 chapters + 26 sections = 31 -> threshold exceeded, collapsed
    (check (outline-default-expanded-paths chaps-over-31) => '())
    (check (cdr (outline-default-expanded-state chaps-over-31)) => 5)
  ) ;let*

  ;; 4. Test with real buffer: 1322.stem
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
