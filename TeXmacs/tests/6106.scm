;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6106.scm
;; DESCRIPTION : Integration test for JSON language syntax highlighting and document
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/generic/document-edit.scm")
(load "./TeXmacs/plugins/json/progs/json/json-format.scm")
(load "./TeXmacs/plugins/json/progs/json/json-edit.scm")
(load "./TeXmacs/plugins/json/progs/json/json-lang.scm")

(import (liii check))

(define (test_6106)
  (check-set-mode! 'report-failed)

  ;; 1. Verify JSON format and parser configuration
  (check (format? "json") => #t)
  (check (pair? (parser-feature "json" "keyword")) => #t)
  (check (pair? (parser-feature "json" "operator")) => #t)
  (check (pair? (parser-feature "json" "number")) => #t)
  (check (pair? (parser-feature "json" "string")) => #t)
  (check (pair? (parser-feature "json" "comment")) => #t)

  ;; 2. Verify JSON syntax preferences
  (check (get-preference "syntax:json:variable_identifier") => "json-key-color")
  (check (get-preference "syntax:json:constant_string") => "json-string-color")
  (check (get-preference "syntax:json:constant_number") => "json-number-color")
  (check (get-preference "syntax:json:constant") => "json-constant-color")
  (check (get-preference "syntax:json:operator") => "json-operator-color")
  (check (get-preference "syntax:json:operator_openclose")
    =>
    "json-openclose-color"
  ) ;check

  ;; 3. Load the rich JSON showcase document fixture in default (day) mode
  (let* ((tmu-path (url-append (url-pwd) "TeXmacs/tests/tmu/6106.tmu")))
    (load-buffer tmu-path)
    (update-forced)
    (let ((doc-tree (buffer-get-body (current-buffer))))
      ;; Verify that json and json-code nodes are present in the loaded document
      (check (tree? doc-tree) => #t)
      (check
        (nnull? (tree-search doc-tree (lambda (t) (tm-func? t 'json-code))))
        =>
        #t
      ) ;check
      (check
        (nnull? (tree-search doc-tree (lambda (t) (tm-func? t 'json))))
        =>
        #t
      ) ;check
    ) ;let

    ;; 4. Test switching to dark theme (night mode)
    (add-style-package "dark")
    (update-forced)
    (check (has-style-package? "dark") => #t)

    ;; 5. Test switching back to day mode
    (remove-style-package "dark")
    (update-forced)
    (check (has-style-package? "dark") => #f)
  ) ;let*

  (check-report)
) ;define
