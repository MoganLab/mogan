;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0610.scm
;; DESCRIPTION : Extensive Unit and Integration tests for HTML export/import of all list styles
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(use-modules (convert html tmhtml))
(use-modules (convert html tmhtml-expand))

(check-set-mode! 'report-failed)

(define (patch-has-macro? patch macro-name)
  (let loop
    ((lst (cdr patch)))
    (cond ((null? lst) #f)
          ((and (pair? (car lst))
             (eq? (caar lst) 'associate)
             (string=? (cadar lst) macro-name)
           ) ;and
           #t
          ) ;
          (else (loop (cdr lst)))
    ) ;cond
  ) ;let
) ;define

(define (test-env-patch-registration tag-name)
  (let ((patch (tmhtml-env-patch)))
    (check (patch-has-macro? patch tag-name) => #t)
  ) ;let
) ;define

(define (test-logic-table-dispatch tag-name expected-tag expected-class)
  (let* ((input `(,(string->symbol tag-name) (document (item "test"))))
         (output (tmhtml input))
        ) ;
    ;; Verify dispatch successfully parsed and generated non-empty structure
    (check (null? output) => #f)
    ;; Verify correct HTML element tag
    (check (caar output) => expected-tag)
    ;; Verify correct class attribute
    (let* ((attrs (cadar output))
           (class-pair (and (pair? attrs) (eq? (car attrs) '@) (assoc 'class (cdr attrs))))
          ) ;
      (check class-pair => `(class ,expected-class))
    ) ;let*
  ) ;let*
) ;define

(define (test-0610-all-list-environments-registration)
  (display "Verifying env-patch registration of all 24 list types (24 checks)...\n"
  ) ;display
  ;; 14 Ordered List Types
  (test-env-patch-registration "enumerate")
  (test-env-patch-registration "enumerate-numeric")
  (test-env-patch-registration "enumerate-numeric-bracket")
  (test-env-patch-registration "enumerate-numeric-paren")
  (test-env-patch-registration "enumerate-roman")
  (test-env-patch-registration "enumerate-roman-bracket")
  (test-env-patch-registration "enumerate-roman-paren")
  (test-env-patch-registration "enumerate-Roman")
  (test-env-patch-registration "enumerate-alpha")
  (test-env-patch-registration "enumerate-alpha-bracket")
  (test-env-patch-registration "enumerate-alpha-full-paren")
  (test-env-patch-registration "enumerate-Alpha")
  (test-env-patch-registration "enumerate-circle")
  (test-env-patch-registration "enumerate-hanzi")

  ;; 4 Unordered List Types
  (test-env-patch-registration "itemize")
  (test-env-patch-registration "itemize-minus")
  (test-env-patch-registration "itemize-dot")
  (test-env-patch-registration "itemize-arrow")

  ;; 6 Description List Types
  (test-env-patch-registration "description")
  (test-env-patch-registration "description-compact")
  (test-env-patch-registration "description-dash")
  (test-env-patch-registration "description-aligned")
  (test-env-patch-registration "description-long")
  (test-env-patch-registration "description-paragraphs")
) ;define

(define (test-0610-all-list-environments-dispatch)
  (display "Verifying SXML dispatch tag and class output for all 24 list types (72 checks)...\n"
  ) ;display
  ;; 14 Ordered List Types (expected h:ol)
  (test-logic-table-dispatch "enumerate" 'h:ol "enumerate")
  (test-logic-table-dispatch "enumerate-numeric" 'h:ol "enumerate-numeric")
  (test-logic-table-dispatch "enumerate-numeric-bracket"
    'h:ol
    "enumerate-numeric-bracket"
  ) ;test-logic-table-dispatch
  (test-logic-table-dispatch "enumerate-numeric-paren"
    'h:ol
    "enumerate-numeric-paren"
  ) ;test-logic-table-dispatch
  (test-logic-table-dispatch "enumerate-roman" 'h:ol "enumerate-roman")
  (test-logic-table-dispatch "enumerate-roman-bracket"
    'h:ol
    "enumerate-roman-bracket"
  ) ;test-logic-table-dispatch
  (test-logic-table-dispatch "enumerate-roman-paren"
    'h:ol
    "enumerate-roman-paren"
  ) ;test-logic-table-dispatch
  (test-logic-table-dispatch "enumerate-Roman" 'h:ol "enumerate-Roman")
  (test-logic-table-dispatch "enumerate-alpha" 'h:ol "enumerate-alpha")
  (test-logic-table-dispatch "enumerate-alpha-bracket"
    'h:ol
    "enumerate-alpha-bracket"
  ) ;test-logic-table-dispatch
  (test-logic-table-dispatch "enumerate-alpha-full-paren"
    'h:ol
    "enumerate-alpha-full-paren"
  ) ;test-logic-table-dispatch
  (test-logic-table-dispatch "enumerate-Alpha" 'h:ol "enumerate-Alpha")
  (test-logic-table-dispatch "enumerate-circle" 'h:ol "enumerate-circle")
  (test-logic-table-dispatch "enumerate-hanzi" 'h:ol "enumerate-hanzi")

  ;; 4 Unordered List Types (expected h:ul)
  (test-logic-table-dispatch "itemize" 'h:ul "itemize")
  (test-logic-table-dispatch "itemize-minus" 'h:ul "itemize-minus")
  (test-logic-table-dispatch "itemize-dot" 'h:ul "itemize-dot")
  (test-logic-table-dispatch "itemize-arrow" 'h:ul "itemize-arrow")

  ;; 6 Description List Types (expected h:dl)
  (test-logic-table-dispatch "description" 'h:dl "description")
  (test-logic-table-dispatch "description-compact" 'h:dl "description-compact")
  (test-logic-table-dispatch "description-dash" 'h:dl "description-dash")
  (test-logic-table-dispatch "description-aligned" 'h:dl "description-aligned")
  (test-logic-table-dispatch "description-long" 'h:dl "description-long")
  (test-logic-table-dispatch "description-paragraphs"
    'h:dl
    "description-paragraphs"
  ) ;test-logic-table-dispatch
) ;define

(define (test-0610-html-export-css-verification)
  (display "Verifying CSS header injection for custom ordered lists (10+ checks)...\n"
  ) ;display
  (let ((css (tmhtml-css-header)))
    ;; Check key custom CSS counter rule injections
    (check (string-contains? css
             "ol.enumerate-numeric-bracket > li::marker { content: counter(list-item) \") \"; }"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (string-contains? css
             "ol.enumerate-numeric-paren > li::marker { content: \"(\" counter(list-item) \") \"; }"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (string-contains? css
             "ol.enumerate-roman-bracket > li::marker { content: counter(list-item, lower-roman) \") \"; }"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (string-contains? css
             "ol.enumerate-roman-paren > li::marker { content: \"(\" counter(list-item, lower-roman) \") \"; }"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (string-contains? css
             "ol.enumerate-alpha-bracket > li::marker { content: counter(list-item, lower-alpha) \") \"; }"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (string-contains? css
             "ol.enumerate-alpha-full-paren > li::marker { content: \"(\" counter(list-item, lower-alpha) \") \"; }"
           ) ;string-contains?
      =>
      #t
    ) ;check
    ;; Check native style rules
    (check (string-contains? css "ol.enumerate-roman { list-style-type: lower-roman; }")
      =>
      #t
    ) ;check
    (check (string-contains? css "ol.enumerate-Roman { list-style-type: upper-roman; }")
      =>
      #t
    ) ;check
    (check (string-contains? css "ol.enumerate-alpha { list-style-type: lower-alpha; }")
      =>
      #t
    ) ;check
    (check (string-contains? css "ol.enumerate-Alpha { list-style-type: upper-alpha; }")
      =>
      #t
    ) ;check
    (check (string-contains? css
             "ol.enumerate-hanzi { list-style-type: simp-chinese-informal; }"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (string-contains? css "ol.enumerate-circle { list-style-type: decimal; }")
      =>
      #t
    ) ;check
  ) ;let
) ;define

(define (test-0610-html-export-integration)
  (display "Verifying HTML export of nested lists and output tag classes (5+ checks)...\n"
  ) ;display
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0610.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html))
        ) ;
    ;; Verify correct hierarchical nested ol structures are exported with respective classes
    (check (string-contains? html-content "class=\"enumerate-numeric\"") => #t)
    (check (string-contains? html-content "class=\"enumerate-numeric-paren\"")
      =>
      #t
    ) ;check
    (check (string-contains? html-content "class=\"enumerate-alpha\"") => #t)
    ;; Check that CSS counter rules are included in the HTML's style header
    (check (string-contains? html-content "ol.enumerate-numeric-paren") => #t)
    (check (string-contains? html-content "counter(list-item)") => #t)
    (check (string-contains? html-content "ol.enumerate-alpha") => #t)
    ;; Check actual item content
    (check (string-contains? html-content "误差分析") => #t)
    (check (string-contains? html-content "螺=") => #t)
    (check (string-contains? html-content "3hhh") => #t)
  ) ;let*
) ;define

(define (test-0610-html-import-integration)
  (display "Verifying HTML import of nested lists to exact TeXmacs environments...\n"
  ) ;display
  (let* ((html-path "$TEXMACS_PATH/tests/html/0610_import_test.html")
         (imported-tree (tree-import html-path "html"))
         (stree (tree->stree imported-tree))
         (stree-str (object->string stree))
        ) ;
    ;; Check that imported document contains our custom nested list environments!
    (check (string-contains? stree-str "enumerate-numeric") => #t)
    (check (string-contains? stree-str "enumerate-numeric-paren") => #t)
    (check (string-contains? stree-str "enumerate-alpha") => #t)
    (check (string-contains? stree-str "enumerate-hanzi") => #t)
    (check (string-contains? stree-str "enumerate-circle") => #t)
    (check (string-contains? stree-str "enumerate-Alpha") => #t)
    (check (string-contains? stree-str "enumerate-alpha-full-paren") => #t)
    (check (string-contains? stree-str "enumerate-numeric-bracket") => #t)
    (check (string-contains? stree-str "enumerate-Roman") => #t)
    (check (string-contains? stree-str "enumerate-roman-bracket") => #t)
    (check (string-contains? stree-str "enumerate-roman") => #t)
  ) ;let*
) ;define

(tm-define (test_0610)
  (test-0610-all-list-environments-registration)
  (test-0610-all-list-environments-dispatch)
  (test-0610-html-export-css-verification)
  (test-0610-html-export-integration)
  (test-0610-html-import-integration)
  (check-report)
) ;tm-define
