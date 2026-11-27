;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6104.scm
;; DESCRIPTION : Integration test for SQL language syntax highlighting and document
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/generic/document-edit.scm")
(load "./TeXmacs/plugins/sql/progs/sql/sql-format.scm")
(load "./TeXmacs/plugins/sql/progs/sql/sql-edit.scm")
(load "./TeXmacs/plugins/sql/progs/sql/sql-lang.scm")

(import (liii check))

(define (test_6104)
  (check-set-mode! 'report-failed)

  ;; 1. Verify SQL format and parser configuration
  (check (format? "sql") => #t)
  (check (pair? (parser-feature "sql" "keyword")) => #t)
  (check (pair? (parser-feature "sql" "operator")) => #t)
  (check (pair? (parser-feature "sql" "number")) => #t)
  (check (pair? (parser-feature "sql" "string")) => #t)
  (check (pair? (parser-feature "sql" "comment")) => #t)

  ;; 2. Load the rich SQL showcase document fixture
  (let* ((tmu-path (url-append (url-pwd) "TeXmacs/tests/tmu/6104.tmu")))
    (load-buffer tmu-path)
    (update-forced)
    (let ((doc-tree (buffer-get-body (current-buffer))))
      ;; Verify that sql and sql-code nodes are present in the loaded document
      (check (tree? doc-tree) => #t)
      (check
        (nnull? (tree-search doc-tree (lambda (t) (tm-func? t 'sql-code))))
        =>
        #t
      ) ;check
      (check
        (nnull? (tree-search doc-tree (lambda (t) (tm-func? t 'sql))))
        =>
        #t
      ) ;check
    ) ;let
  ) ;let*

  (check-report)
) ;define
