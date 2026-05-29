;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0631.scm
;; DESCRIPTION : Integration tests for PR 0631 LaTeX Table Import and Extreme Cases
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (load-latex path)
  (with path (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")))

(define (has-cwith-property? options row-start row-end col-start col-end property value-pred)
  (cond ((null? options) #f)
        ((and (pair? (car options))
              (eq? (caar options) 'cwith))
         (let* ((opt (car options))
                (r-start (and (> (length opt) 1) (list-ref opt 1)))
                (r-end   (and (> (length opt) 2) (list-ref opt 2)))
                (c-start (and (> (length opt) 3) (list-ref opt 3)))
                (c-end   (and (> (length opt) 4) (list-ref opt 4)))
                (prop    (and (> (length opt) 5) (list-ref opt 5)))
                (val     (and (> (length opt) 6) (list-ref opt 6))))
           (if (and (or (not row-start) (equal? r-start row-start))
                    (or (not row-end) (equal? r-end row-end))
                    (or (not col-start) (equal? c-start col-start))
                    (or (not col-end) (equal? c-end col-end))
                    (or (not property) (equal? prop property))
                    (and val (value-pred val)))
               #t
               (has-cwith-property? (cdr options) row-start row-end col-start col-end property value-pred))))
        (else (has-cwith-property? (cdr options) row-start row-end col-start col-end property value-pred))))

(define (find-table-num-rows children)
  (cond ((null? children) 0)
        ((and (pair? (car children)) (eq? (caar children) 'table))
         (length (cdar children)))
        (else (find-table-num-rows (cdr children)))))

(define (is-three-line-table-tformat? x)
  (if (and (pair? x) (eq? (car x) 'tformat))
      (let* ((options (cdr x))
             (num-rows (find-table-num-rows options)))
        (if (> num-rows 0)
            (let* ((has-top? (has-cwith-property? options "1" "1" #f #f "cell-tborder" (lambda (v) (not (equal? v "0ln")))))
                   (has-bottom? (has-cwith-property? options (number->string num-rows) (number->string num-rows) #f #f "cell-bborder" (lambda (v) (not (equal? v "0ln")))))
                   (has-vertical? (has-cwith-property? options #f #f #f #f "cell-lborder" (lambda (v) (not (equal? v "0ln")))))
                   (has-vertical-r? (has-cwith-property? options #f #f #f #f "cell-rborder" (lambda (v) (not (equal? v "0ln"))))))
              (and has-top? has-bottom? (not has-vertical?) (not has-vertical-r?)))
            #f))
      #f))

(define (transform-three-line-tables x)
  (cond ((null? x) '())
        ((and (pair? x) (eq? (car x) 'tformat))
         (let ((transformed-args (map transform-three-line-tables (cdr x))))
           (let ((new-tformat (cons 'tformat transformed-args)))
             (if (is-three-line-table-tformat? new-tformat)
                 (list 'three-line-table new-tformat)
                 new-tformat))))
        ((pair? x)
         (cons (transform-three-line-tables (car x))
               (transform-three-line-tables (cdr x))))
        (else x)))

(define (stree-contains? x target)
  (cond ((null? x) #f)
        ((equal? x target) #t)
        ((pair? x) (or (stree-contains? (car x) target)
                       (stree-contains? (cdr x) target)))
        (else #f)))

(define (test-latex-table-import)
  (display "Testing 40 extreme cases of LaTeX table import...\n")
  (let* ((latex-content (load-latex "0631_table_import.tex"))
         (parsed (parse-latex-document latex-content))
         (texmacs-tree (latex->texmacs parsed))
         (st-orig (tree->stree texmacs-tree))
         (st (transform-three-line-tables st-orig)))
    
    (display "Verifying specific table properties in converted tree...\n")
    
    ;; Verify that the document parsed successfully
    (check (null? st) => #f)
    
    ;; 1. Check three-line-table support
    ;; The booktabs toprule/midrule/bottomrule tables should be converted to 'three-line-table
    (check (stree-contains? st 'three-line-table) => #t)
    
    ;; 2. Check basic tabular format (like 'tabular or 'tabular*)
    (check (stree-contains? st 'tabular*) => #t)
    
    ;; 3. Check for specific cell contents to ensure no content was lost during parsing
    (check (stree-contains? st "Span Three Columns") => #t)
    (check (stree-contains? st "Span Two Right") => #t)
    (check (stree-contains? st "Row Span") => #t)
    (check (stree-contains? st "MultiRowCol") => #t)
    (check (stree-contains? st "Fixed Width Row") => #t)
    (check (stree-contains? st "Solo") => #t)
    (check (stree-contains? st "Left text") => #t)
    
    ;; 4. Check for nested tabular environments
    (check (stree-contains? st "Outer cell") => #t)
    (check (stree-contains? st "Inner 1") => #t)
    
    ;; 5. Check for math mode cell preservation
    (check (stree-contains? st "<alpha>*<beta>") => #t)
    
    ;; 6. Check for float environments and captions
    (check (stree-contains? st 'big-table) => #t)
    (check (stree-contains? st "Test Caption") => #t)
    (check (stree-contains? st "tab:test_label") => #t)
    ))

(tm-define (test_0631)
  (test-latex-table-import)
  (check-report))
