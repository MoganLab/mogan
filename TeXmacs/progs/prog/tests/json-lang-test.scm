;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : json-lang-test.scm
;; DESCRIPTION : Pure logic unit tests for JSON language parser features and preferences
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r json-lang-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/json/progs/json/json-lang.scm")

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

;; 1. Keyword & Constant parser-features

(define (test-json-keywords)
  (let ((kw (parser-feature "json" "keyword")))
    (check (pair? kw) => #t)
    (check (car kw) => 'keyword)

    ;; Constants in JSON
    (check (feature-has-val? kw 'constant "true") => #t)
    (check (feature-has-val? kw 'constant "false") => #t)
    (check (feature-has-val? kw 'constant "null") => #t)
  ) ;let
) ;define

;; 2. Operator & Delimiter parser-features

(define (test-json-operators)
  (let ((op (parser-feature "json" "operator")))
    (check (pair? op) => #t)
    (check (car op) => 'operator)

    (let ((opers (feature-get-group op 'operator)))
      (check (group-contains? opers "+") => #t)
      (check (group-contains? opers "-") => #t)
      (check (group-contains? opers ":") => #t)
      (check (group-contains? opers ",") => #t)
    ) ;let

    (let ((delims (feature-get-group op 'operator_openclose)))
      (check (group-contains? delims "{") => #t)
      (check (group-contains? delims "}") => #t)
      (check (group-contains? delims "[") => #t)
      (check (group-contains? delims "]") => #t)
      (check (group-contains? delims "(") => #t)
      (check (group-contains? delims ")") => #t)
    ) ;let
  ) ;let
) ;define

;; 3. Number, String, and Comment parser-features

(define (test-json-literals-and-comments)
  ;; Numbers
  (let ((num (parser-feature "json" "number")))
    (check (pair? num) => #t)
    (check (car num) => 'number)
    (let ((bool-f (feature-get-group num 'bool_features)))
      (check (group-contains? bool-f "sci_notation") => #t)
    ) ;let
  ) ;let

  ;; Strings
  (let ((str (parser-feature "json" "string")))
    (check (pair? str) => #t)
    (check (car str) => 'string)
    (let ((bool-f (feature-get-group str 'bool_features)))
      (check (group-contains? bool-f "escape_char_after_backslash") => #t)
      (check (group-contains? bool-f "unicode_escape") => #t)
    ) ;let
    (let ((esc (feature-get-group str 'escape_sequences)))
      (check (group-contains? esc "\\") => #t)
      (check (group-contains? esc "/") => #t)
      (check (group-contains? esc "\"") => #t)
      (check (group-contains? esc "b") => #t)
      (check (group-contains? esc "f") => #t)
      (check (group-contains? esc "n") => #t)
      (check (group-contains? esc "r") => #t)
      (check (group-contains? esc "t") => #t)
      (check (group-contains? esc "u") => #t)
    ) ;let
  ) ;let

  ;; Comments (JSONC support)
  (let ((cmt (parser-feature "json" "comment")))
    (check (pair? cmt) => #t)
    (check (car cmt) => 'comment)
    (let ((inl (feature-get-group cmt 'inline)))
      (check (group-contains? inl "//") => #t)
    ) ;let
    (let ((blk (feature-get-group cmt 'block_comment)))
      (check (pair? blk) => #t)
      (check (cadr blk) => "/*")
      (check (caddr blk) => "*/")
    ) ;let
  ) ;let
) ;define

;; 4. JSON syntax preferences (adaptive day/night theme colors)

(define (test-json-preferences)
  (check (get-preference "syntax:json:variable_identifier") => "json-key-color")
  (check (get-preference "syntax:json:constant_string") => "json-string-color")
  (check (get-preference "syntax:json:constant_char") => "json-string-color")
  (check (get-preference "syntax:json:constant_number") => "json-number-color")
  (check (get-preference "syntax:json:constant") => "json-constant-color")
  (check (get-preference "syntax:json:operator") => "json-operator-color")
  (check (get-preference "syntax:json:operator_openclose") => "json-openclose-color")
  (check (get-preference "syntax:json:keyword") => "json-constant-color")
  (check (get-preference "syntax:json:comment") => "comment-color")
) ;define

(tm-define (regtest-json-lang)
  (test-json-keywords)
  (test-json-operators)
  (test-json-literals-and-comments)
  (test-json-preferences)
  (check-report)
) ;tm-define
