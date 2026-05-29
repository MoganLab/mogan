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
         (st (tree->stree texmacs-tree)))
    
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
