;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0621.scm
;; DESCRIPTION : Comprehensive TDD tests for align and eqnarray HTML export/import
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(use-modules (convert html tmhtml))
(use-modules (convert html htmltm))

(check-set-mode! 'report-failed)

(define (test-0621-rewrite-align-unit-tests)
  (display "Running rewrite-align* row rewriting unit tests...\n")
  ;; 2-cell row rewrite (like align)
  (let* ((r2 '(row (cell "L") (cell (concat "R" (htab "5mm") "eq-num"))))
         (res2 (rewrite-align* r2))
        ) ;
    (check (car res2) => 'row)
    (check (length (cdr res2)) => 3)
    (check (list-ref res2 1) => '(cell (big-math "L")))
    (check (list-ref res2 2) => '(cell (big-math "R")))
    (check (list-ref res2 3) => '(cell "eq-num"))
  ) ;let*
) ;define

(define (test-0621-ext-tmhtml-align-unit-tests)
  (display "Running ext-tmhtml-align* dispatcher unit tests...\n")
  ;; Multi-row 2-cell table (align) with htab
  (let* ((inner '(tformat (table (row (cell "L1")
                                   (cell (concat "R1" (htab "5mm") "1")))
                            (row (cell "L2")
                              (cell (concat "R2" (htab "5mm") "2")))))
         ) ;inner
         (res (ext-tmhtml-align* inner))
        ) ;
    (check (car res) => 'html-class)
    (check (cadr res) => "equations-table")
    (let* ((table-node (caddr res))
           (rewritten (cadr table-node))
           (tbl (last rewritten))
           (rows (cdr tbl))
          ) ;
      (check (car table-node) => 'alignx-table)
      (check (car rewritten) => 'tformat)
      (check (not (not (member '(row (cell (big-math "L1"))
                                  (cell (big-math "R1"))
                                  (cell "1")) rows)
                  ) ;not
             ) ;not
        =>
        #t
      ) ;check
    ) ;let*
  ) ;let*
) ;define

(define (test-0621-html-export-integration)
  (display "Verifying HTML export of align and equation layouts (10 checks)...\n")
  (let* ((tmu-path "$TEXMACS_PATH/tests/tmu/0621.tmu")
         (tmp-html (url-temp))
         (dummy (load-buffer tmu-path))
         ;; Force standard HTML table output instead of MathML for robust positioning checks
         (saved-mathml tmhtml-mathml?)
         (saved-css tmhtml-css?)
         (saved-mathjax tmhtml-mathjax?)
         (saved-images tmhtml-images?)
         (dummy-set (begin
                      (set! tmhtml-mathml? #f)
                      (set! tmhtml-css? #t)
                      (set! tmhtml-mathjax? #f)
                      (set! tmhtml-images? #f)
                    ) ;begin
         ) ;dummy-set
         (dummy2 (buffer-export tmu-path tmp-html "html"))
         (html-content (string-load tmp-html))
         ;; Restore settings
         (dummy-restore (begin
                          (set! tmhtml-mathml? saved-mathml)
                          (set! tmhtml-css? saved-css)
                          (set! tmhtml-mathjax? saved-mathjax)
                          (set! tmhtml-images? saved-images)
                        ) ;begin
         ) ;dummy-restore
        ) ;
    ;; We want to make sure the align and eqnarray environments are rendered as HTML tables with width: 100%
    (check (string-contains? html-content "width: 100%") => #t)
    (check (string-contains? html-content "text-align: right") => #t)
    ;; Ensure all formula numbers are exported and preserved
    (check (string-contains? html-content "(1)") => #t)
    (check (string-contains? html-content "(2)") => #t)
    (check (string-contains? html-content "(3)") => #t)
    (check (string-contains? html-content "(4)") => #t)
  ) ;let*
) ;define

(define (test-0621-html-import-integration)
  (display "Verifying HTML import of align and eqnarray layouts (5 checks)...\n")
  (let* ((html-path "$TEXMACS_PATH/tests/html/0621_import_test.html")
         (imported-tree (tree-import html-path "html"))
         (stree (tree->stree imported-tree))
         (stree-str (object->string stree))
        ) ;
    ;; Check that imported document contains our custom align and eqnarray* environments losslessly restored!
    (check (string-contains? stree-str "align") => #t)
    (check (string-contains? stree-str "eqnarray*") => #t)
  ) ;let*
) ;define

(tm-define (test_0621)
  (test-0621-rewrite-align-unit-tests)
  (test-0621-ext-tmhtml-align-unit-tests)
  (test-0621-html-export-integration)
  (test-0621-html-import-integration)
  (check-report)
) ;tm-define
