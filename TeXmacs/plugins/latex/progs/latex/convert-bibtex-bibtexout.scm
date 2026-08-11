
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : bibtexout.scm
;; DESCRIPTION : generation of BibTeX from scheme expressions
;; COPYRIGHT   : (C) 2010  David MICHEL
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (latex convert-bibtex-bibtexout)
  (:use (convert tools output))
  (:use (latex convert-latex-texout))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Entries output
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bibtex-remove-keepcase x)
  ;; (display* "REMOVE KEEPCASE: " x "\n")
  (cond ((list? x)
         (if (not (null? x))
           (if (equal? (car x) 'keepcase)
             (if (null? (cdr x)) "{}" `(rigid ,(bibtex-remove-keepcase (cadr x))))
             (cons (car x) (map bibtex-remove-keepcase (cdr x)))
           ) ;if
           ()
         ) ;if
        ) ;
        ((string? x) x)
  ) ;cond
) ;define

(define (bibtex-latex x)
  (let ((options (list (cons "texmacs->latex:replace-style" "on")
                   (cons "texmacs->latex:expand-macros" "on")
                   (cons "texmacs->latex:expand-user-macros" "off")
                   (cons "texmacs->latex:indirect-bib" "off")
                   (cons "texmacs->latex:encoding" "UTF-8")
                   (cons "texmacs->latex:use-macros" "off")
                 ) ;list
        ) ;options
       ) ;
    (with old-exact
      (output-set-exact #t)
      (output-flush)
      (texout (texmacs->latex (bibtex-remove-keepcase x) options))
      (output-set-exact old-exact)
    ) ;with
  ) ;let
) ;define

(define (bibtex-match l s a)
  (and (list? l) (>= (length l) (+ a 1)) (equal? (car l) s))
) ;define

(define (bibtex-flat x)
  (cond ((list? x)
         (if (not (null? x)) (begin (bibtex-flat (car x)) (bibtex-flat (cdr x))))
        ) ;
        ((string? x) (output-verbatim x))
  ) ;cond
) ;define

(define (bibtex-has-var x)
  (if (list? x)
    (cond ((null? x) #f)
          ((bibtex-match (car x) 'bib-var 1) #t)
          (else (bibtex-has-var (cdr x)))
    ) ;cond
    #f
  ) ;if
) ;define

(define (bibtex-arg-var x)
  ;; (display* "BIBTEX ARG VAR: " x "\n")
  (if (not (null? x))
    (let ((head (car x)) (tail (cdr x)))
      (begin
        (cond ((bibtex-match head 'bib-var 1) (output-verbatim (cadr head)))
              ((string? head) (output-verbatim "{" head "}"))
              (else (begin (output-verbatim "{") (bibtex-latex head) (output-verbatim "}")))
        ) ;cond
        (if (not (null? tail)) (begin (output-verbatim " # ") (bibtex-arg-var tail)))
      ) ;begin
    ) ;let
  ) ;if
) ;define

(define (bibtex-name x)
  ;; (display* "BIBTEX NAME: " x "\n")
  (if (bibtex-match x 'bib-name 4)
    (let ((first (list-ref x 1))
          (von (list-ref x 2))
          (last (list-ref x 3))
          (jr (list-ref x 4))
         ) ;
      (begin
        (if (not (equal? von "")) (begin (bibtex-latex von) (output-verbatim " ")))
        (bibtex-latex last)
        (if (not (equal? first "")) (begin (output-verbatim ", ") (bibtex-latex first)))
        (if (not (equal? jr "")) (begin (output-verbatim ", ") (bibtex-latex jr)))
      ) ;begin
    ) ;let
  ) ;if
) ;define

(define (bibtex-names x)
  ;; (display* "BIBTEX NAMES: " x "\n")
  (if (not (null? x))
    (let ((head (car x)) (tail (cdr x)))
      (if (bibtex-match head 'bib-name 4)
        (begin
          (bibtex-name head)
          (if (> (length tail) 0) (output-verbatim " and "))
          (bibtex-names tail)
        ) ;begin
        (bibtex-names tail)
      ) ;if
    ) ;let
  ) ;if
) ;define

(define (bibtex-arg x)
  ;; (display* "BIBTEX ARG: " x "\n")
  (cond ((bibtex-match x 'bib-var 1) (output-verbatim (cadr x)))
        ((bibtex-match x 'bib-names 0)
         (begin
           (output-verbatim "{")
           (bibtex-names (cdr x))
           (output-verbatim "}")
         ) ;begin
        ) ;
        ((bibtex-match x 'bib-pages 2)
         (output-verbatim "{" (cadr x) "--" (caddr x) "}")
        ) ;
        ((bibtex-match x 'bib-pages 1) (output-verbatim "{" (cadr x) "}"))
        ((string? x) (output-verbatim "{" x "}"))
        ((if (bibtex-has-var x)
           (bibtex-arg-var (cdr x))
           (begin
             (output-verbatim "{")
             (bibtex-latex x)
             (output-verbatim "}")
           ) ;begin
         ) ;if
        ) ;
  ) ;cond
) ;define

(define (bibtex-preamble pre x)
  ;; (display* "PREAMBLE: " x "\n")
  (begin
    (output-verbatim pre "preamble{ ")
    (bibtex-arg x)
    (output-verbatim " }")
    (output-lf-verbatim)
  ) ;begin
) ;define

(define (bibtex-preambles pre)
  (lambda (x)
    (if (list? x)
      (cond ((func? x 'document) (for-each (bibtex-preambles pre) (cdr x)))
            ((func? x 'bib-latex)
             (begin
               (bibtex-preamble pre (cadr x))
               (output-lf-verbatim)
             ) ;begin
            ) ;
            ((func? x 'bib-comment) (for-each (bibtex-preambles "") (cdr x)))
      ) ;cond
    ) ;if
  ) ;lambda
) ;define

(define (bibtex-string pre x)
  (if (list? x)
    (begin
      (output-verbatim pre "string{ " (car x) " = ")
      (bibtex-arg (cadr x))
      (output-verbatim " }")
      (output-lf-verbatim)
    ) ;begin
  ) ;if
) ;define

(define (bibtex-strings pre)
  (lambda (x)
    (if (list? x)
      (cond ((func? x 'document) (for-each (bibtex-strings pre) (cdr x)))
            ((func? x 'bib-assign) (begin (bibtex-string pre (cdr x)) (output-lf-verbatim)))
            ((func? x 'bib-comment) (for-each (bibtex-strings "") (cdr x)))
      ) ;cond
    ) ;if
  ) ;lambda
) ;define

(define (bibtex-padded s)
  (cond ((nstring? s) s)
        ((>= (string-length s) 12) s)
        (else (bibtex-padded (string-append s " ")))
  ) ;cond
) ;define

(define (bibtex-field x)
  (if (and (list? x) (= 2 (length x)))
    (begin
      (output-verbatim "  " (bibtex-padded (car x)) " = ")
      (bibtex-arg (cadr x))
    ) ;begin
  ) ;if
) ;define

(define (bibtex-fields x)
  (if (and (list? x) (not (null? x)))
    (cond ((func? (car x) 'document) (bibtex-fields (cdar x)))
          ((func? (car x) 'bib-field)
           (begin
             (bibtex-field (cdar x))
             (if (not (null? (cdr x))) (output-verbatim ","))
             (output-lf-verbatim)
             (bibtex-fields (cdr x))
           ) ;begin
          ) ;
    ) ;cond
  ) ;if
) ;define

(define (bibtex-entry pre x)
  (let ((type (cadr x)) (id (caddr x)) (fields (cdddr x)))
    (begin
      (output-verbatim pre (upcase-first type) "{" (cork->utf8 id))
      (if (not (null? fields))
        (begin
          (output-verbatim ",")
          (output-lf-verbatim)
          (bibtex-fields fields)
        ) ;begin
      ) ;if
      (output-verbatim "}")
      (output-lf-verbatim)
    ) ;begin
  ) ;let
) ;define

(define (bibtex-comment x)
  (cond ((list? x)
         (if (not (null? x))
           (begin
             (cond ((func? (car x) 'document) (bibtex-comment (cdar x)))
                   ((func? (car x) 'bib-entry) (bibtex-entry "" (car x)))
                   ((func? (car x) 'bib-latex) (bibtex-preamble "" (cadar x)))
                   ((func? (car x) 'bib-assign) (bibtex-string "" (cdar x)))
                   (else (begin (output-verbatim "%") (bibtex-flat (car x)) (output-lf-verbatim)))
             ) ;cond
             (bibtex-comment (cdr x))
           ) ;begin
         ) ;if
        ) ;
        ((string? x) (begin (output-verbatim "%" x) (output-lf-verbatim)))
  ) ;cond
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main output routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bibtexout x)
  (cond ((string? x) (output-verbatim x))
        ((func? x 'document) (for-each bibtexout (cdr x)))
        ((func? x 'bib-string) (for-each (bibtex-strings "@") (cdr x)))
        ((func? x 'bib-preamble) (for-each (bibtex-preambles "@") (cdr x)))
        ((func? x 'bib-entry) (bibtex-entry "@" x) (output-lf-verbatim))
        ((func? x 'bib-comment) (bibtex-comment (cdr x)) (output-lf-verbatim))
        ((func? x 'bib-field)
         (bibtex-field (cdr x))
         (output-verbatim ",")
         (output-lf-verbatim)
        ) ;
        ((func? x 'bib-assign) (bibtex-string "@" (cdr x)) (output-lf-verbatim))
        ((func? x 'bib-line) (bibtex-comment (cdr x)))
        ((func? x 'bib-var) (cdr x))
        ((func? x 'bib-names) (bibtex-names (cdr x)) (output-lf-verbatim))
        ((func? x 'bib-name) (bibtex-name x))
        ((func? x 'bib-latex) (bibtex-preamble "@" (cadr x)) (output-lf-verbatim))
  ) ;cond
) ;define

(tm-define (serialize-bibtex x)
  (with old-line-length
    (output-set-line-length 999999)
    (bibtexout x)
    (output-set-line-length old-line-length)
    (output-produce)
  ) ;with
) ;tm-define

(tm-define (serialize-bibtex-arg x)
  (with old-line-length
    (output-set-line-length 999999)
    (bibtex-arg x)
    (output-set-line-length old-line-length)
    (with r
      (output-produce)
      (if (and (string-starts? r "{") (string-ends? r "}"))
        (substring r 1 (- (string-length r) 1))
        r
      ) ;if
    ) ;with
  ) ;with
) ;tm-define
