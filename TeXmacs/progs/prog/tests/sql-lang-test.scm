;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : sql-lang-test.scm
;; DESCRIPTION : Pure logic unit tests for SQL language parser features and editing
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r sql-lang-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/sql/progs/sql/sql-lang.scm")
(load "./TeXmacs/plugins/sql/progs/sql/sql-edit.scm")

;; Helper: find an option entry by label in a parser-feature result list

(define (feature-get-group feat group-name)
  (let loop
    ((rest (cdr feat)))
    (cond ((null? rest) #f)
          ((equal? (car (car rest)) group-name) (car rest))
          (else (loop (cdr rest)))
    ) ;cond
  ) ;let
) ;define

(define (group-contains? group val)
  (and (pair? group) (pair? (member val (cdr group))))
) ;define

(define (feature-has-val? feat group-name val)
  (let loop
    ((rest (cdr feat)))
    (cond ((null? rest) #f)
          ((and (equal? (car (car rest)) group-name) (group-contains? (car rest) val)) #t)
          (else (loop (cdr rest)))
    ) ;cond
  ) ;let
) ;define

;; 1. Keyword parser-features

(define (test-sql-keywords)
  (let ((kw (parser-feature "sql" "keyword")))
    (check (pair? kw) => #t)
    (check (car kw) => 'keyword)

    ;; extra_chars allows underscore in keywords / identifiers
    (check (feature-has-val? kw 'extra_chars "_") => #t)

    ;; Constants
    (check (feature-has-val? kw 'constant "TRUE") => #t)
    (check (feature-has-val? kw 'constant "FALSE") => #t)
    (check (feature-has-val? kw 'constant "NULL") => #t)
    (check (feature-has-val? kw 'constant "null") => #t)

    ;; DDL types & objects
    (check (feature-has-val? kw 'declare_type "CREATE") => #t)
    (check (feature-has-val? kw 'declare_type "TABLE") => #t)
    (check (feature-has-val? kw 'declare_type "VIEW") => #t)
    (check (feature-has-val? kw 'declare_type "INDEX") => #t)
    (check (feature-has-val? kw 'declare_type "TRUNCATE") => #t)

    ;; Identifier declarations / constraints
    (check (feature-has-val? kw 'declare_identifier "PRIMARY") => #t)
    (check (feature-has-val? kw 'declare_identifier "FOREIGN") => #t)
    (check (feature-has-val? kw 'declare_identifier "REFERENCES") => #t)
    (check (feature-has-val? kw 'declare_identifier "UNIQUE") => #t)
    (check (feature-has-val? kw 'declare_identifier "CHECK") => #t)
    (check (feature-has-val? kw 'declare_identifier "AUTO_INCREMENT") => #t)
    (check (feature-has-val? kw 'declare_identifier "SERIAL") => #t)

    ;; Conditional keywords
    (check (feature-has-val? kw 'keyword_conditional "CASE") => #t)
    (check (feature-has-val? kw 'keyword_conditional "WHEN") => #t)
    (check (feature-has-val? kw 'keyword_conditional "THEN") => #t)
    (check (feature-has-val? kw 'keyword_conditional "ELSE") => #t)
    (check (feature-has-val? kw 'keyword_conditional "END") => #t)

    ;; Control keywords (transactions)
    (check (feature-has-val? kw 'keyword_control "BEGIN") => #t)
    (check (feature-has-val? kw 'keyword_control "COMMIT") => #t)
    (check (feature-has-val? kw 'keyword_control "ROLLBACK") => #t)
    (check (feature-has-val? kw 'keyword_control "SAVEPOINT") => #t)
    (check (feature-has-val? kw 'keyword_control "START") => #t)

    ;; Core DQL / DML keywords & window clauses
    (check (feature-has-val? kw 'keyword "SELECT") => #t)
    (check (feature-has-val? kw 'keyword "FROM") => #t)
    (check (feature-has-val? kw 'keyword "WHERE") => #t)
    (check (feature-has-val? kw 'keyword "GROUP") => #t)
    (check (feature-has-val? kw 'keyword "HAVING") => #t)
    (check (feature-has-val? kw 'keyword "ORDER") => #t)
    (check (feature-has-val? kw 'keyword "JOIN") => #t)
    (check (feature-has-val? kw 'keyword "INSERT") => #t)
    (check (feature-has-val? kw 'keyword "UPDATE") => #t)
    (check (feature-has-val? kw 'keyword "DELETE") => #t)
    (check (feature-has-val? kw 'keyword "MERGE") => #t)
    (check (feature-has-val? kw 'keyword "UNION") => #t)
    (check (feature-has-val? kw 'keyword "WITH") => #t)
    (check (feature-has-val? kw 'keyword "OVER") => #t)
    (check (feature-has-val? kw 'keyword "PARTITION") => #t)
    ;; Built-in functions & types in keyword group
    (check (feature-has-val? kw 'keyword "COUNT") => #t)
    (check (feature-has-val? kw 'keyword "SUM") => #t)
    (check (feature-has-val? kw 'keyword "AVG") => #t)
    (check (feature-has-val? kw 'keyword "ROW_NUMBER") => #t)
    (check (feature-has-val? kw 'keyword "RANK") => #t)
    (check (feature-has-val? kw 'keyword "COALESCE") => #t)
    (check (feature-has-val? kw 'keyword "CURRENT_TIMESTAMP") => #t)
    (check (feature-has-val? kw 'keyword "VARCHAR") => #t)
    (check (feature-has-val? kw 'keyword "INT") => #t)
    (check (feature-has-val? kw 'keyword "JSON") => #t)
  ) ;let
) ;define

;; 2. Operator parser-features

(define (test-sql-operators)
  (let ((op (parser-feature "sql" "operator")))
    (check (pair? op) => #t)
    (check (car op) => 'operator)
    (let ((opers (feature-get-group op 'operator)))
      (check (group-contains? opers "+") => #t)
      (check (group-contains? opers "-") => #t)
      (check (group-contains? opers "*") => #t)
      (check (group-contains? opers "/") => #t)
      (check (group-contains? opers "||") => #t)
      (check (group-contains? opers "=") => #t)
      (check (group-contains? opers "<>") => #t)
      (check (group-contains? opers "!=") => #t)
      (check (group-contains? opers "::") => #t)
      (check (group-contains? opers "->") => #t)
      (check (group-contains? opers "->>") => #t)
      (check (group-contains? opers "LIKE") => #t)
      (check (group-contains? opers "BETWEEN") => #t)
      (check (group-contains? opers "IN") => #t)
      (check (group-contains? opers "AND") => #t)
      (check (group-contains? opers "OR") => #t)
      (check (group-contains? opers "NOT") => #t)
    ) ;let
    (let ((brackets (feature-get-group op 'operator_openclose)))
      (check (group-contains? brackets "(") => #t)
      (check (group-contains? brackets ")") => #t)
      (check (group-contains? brackets ";") => #t)
      (check (group-contains? brackets ",") => #t)
    ) ;let
  ) ;let
) ;define

;; 3. Number, String, and Comment parser-features

(define (test-sql-literals-and-comments)
  ;; Numbers
  (let ((num (parser-feature "sql" "number")))
    (check (pair? num) => #t)
    (check (car num) => 'number)
    (let ((bool-f (feature-get-group num 'bool_features)))
      (check (group-contains? bool-f "prefix_0x") => #t)
      (check (group-contains? bool-f "prefix_0b") => #t)
      (check (group-contains? bool-f "sci_notation") => #t)
    ) ;let
  ) ;let

  ;; Strings
  (let ((str (parser-feature "sql" "string")))
    (check (pair? str) => #t)
    (check (car str) => 'string)
    (let ((de (feature-get-group str 'double_escape)))
      (check (pair? de) => #t)
      (check (group-contains? de "'") => #t)
    ) ;let
    (let ((esc (feature-get-group str 'escape_sequences)))
      (check (group-contains? esc "\\") => #t)
      (check (group-contains? esc "'") => #t)
      (check (group-contains? esc "\"") => #t)
    ) ;let
  ) ;let

  ;; Comments
  (let ((cmt (parser-feature "sql" "comment")))
    (check (pair? cmt) => #t)
    (check (car cmt) => 'comment)
    (let ((inl (feature-get-group cmt 'inline)))
      (check (group-contains? inl "--") => #t)
      (check (group-contains? inl "#") => #t)
    ) ;let
    (let ((blk (feature-get-group cmt 'block_comment)))
      (check (pair? blk) => #t)
      (check (cadr blk) => "/*")
      (check (caddr blk) => "*/")
    ) ;let
  ) ;let
) ;define

;; 4. SQL edit / indentation helpers

(define (test-sql-editing-helpers)
  (check (sql-tabstop) => 2)
  (check (string-strip-left "   SELECT") => "SELECT")
  (check (string-strip-right "SELECT   ") => "SELECT")
  (check (string-strip "   SELECT   ") => "SELECT")
  (check (starts-with-keyword? "SELECT * FROM t" '("SELECT" "INSERT")) => #t)
  (check (starts-with-keyword? "UPDATE t SET a=1" '("SELECT" "INSERT")) => #f)
  (check (ends-with-keyword? "BEGIN" '("BEGIN" "END")) => #t)
  (check (ends-with-keyword? "t;" '("BEGIN" "END")) => #f)
) ;define

(tm-define (regtest-sql-lang)
  (test-sql-keywords)
  (test-sql-operators)
  (test-sql-literals-and-comments)
  (test-sql-editing-helpers)
  (check-report)
) ;tm-define
