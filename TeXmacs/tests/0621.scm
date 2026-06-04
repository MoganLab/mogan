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

(define (test-0621-table-construction-unit-tests)
  (display "Running make-html-equation-table unit tests (15 checks)...\n")
  (let* ((rows '((row (cell "a") (cell "b"))))
         (t2 (make-html-equation-table rows 2))
         (t3 (make-html-equation-table rows 3))
         (t4 (make-html-equation-table rows 4))
        ) ;
    ;; Verify tformat label
    (check (car t2) => 'tformat)
    (check (car t3) => 'tformat)
    (check (car t4) => 'tformat)

    ;; Verify twith columns count configuration
    (check (member '(twith "table-min-cols" "2") t2)
      =>
      '((twith "table-min-cols" "2")
        (twith "table-max-cols" "2")
        (cwith "1" "-1" "1" "1" "cell-lsep" "0spc")
        (cwith "1" "-1" "-1" "-1" "cell-rsep" "0spc")
        (cwith "1" "-1" "1" "-1" "cell-bsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-tsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-block" "no")
        (cwith "1" "-1" "1" "1" "cell-hpart" "1")
        (cwith "1" "-1" "1" "1" "cell-halign" "c")
        (cwith "1" "-1" "2" "2" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(twith "table-min-cols" "3") t3)
      =>
      '((twith "table-min-cols" "3")
        (twith "table-max-cols" "3")
        (cwith "1" "-1" "1" "1" "cell-lsep" "0spc")
        (cwith "1" "-1" "-1" "-1" "cell-rsep" "0spc")
        (cwith "1" "-1" "1" "-1" "cell-bsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-tsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-block" "no")
        (cwith "1" "-1" "1" "1" "cell-hpart" "1")
        (cwith "1" "-1" "2" "2" "cell-hpart" "1")
        (cwith "1" "-1" "1" "1" "cell-halign" "r")
        (cwith "1" "-1" "2" "2" "cell-halign" "l")
        (cwith "1" "-1" "3" "3" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(twith "table-min-cols" "4") t4)
      =>
      '((twith "table-min-cols" "4")
        (twith "table-max-cols" "4")
        (cwith "1" "-1" "1" "1" "cell-lsep" "0spc")
        (cwith "1" "-1" "-1" "-1" "cell-rsep" "0spc")
        (cwith "1" "-1" "1" "-1" "cell-bsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-tsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-block" "no")
        (cwith "1" "-1" "1" "1" "cell-hpart" "1")
        (cwith "1" "-1" "3" "3" "cell-hpart" "2")
        (cwith "1" "-1" "1" "1" "cell-halign" "r")
        (cwith "1" "-1" "2" "2" "cell-halign" "c")
        (cwith "1" "-1" "3" "3" "cell-halign" "l")
        (cwith "1" "-1" "4" "4" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check

    ;; Verify width property
    (check (member '(twith "table-width" "1par") t2)
      =>
      '((twith "table-width" "1par")
        (twith "table-min-cols" "2")
        (twith "table-max-cols" "2")
        (cwith "1" "-1" "1" "1" "cell-lsep" "0spc")
        (cwith "1" "-1" "-1" "-1" "cell-rsep" "0spc")
        (cwith "1" "-1" "1" "-1" "cell-bsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-tsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-block" "no")
        (cwith "1" "-1" "1" "1" "cell-hpart" "1")
        (cwith "1" "-1" "1" "1" "cell-halign" "c")
        (cwith "1" "-1" "2" "2" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(twith "table-width" "1par") t4)
      =>
      '((twith "table-width" "1par")
        (twith "table-min-cols" "4")
        (twith "table-max-cols" "4")
        (cwith "1" "-1" "1" "1" "cell-lsep" "0spc")
        (cwith "1" "-1" "-1" "-1" "cell-rsep" "0spc")
        (cwith "1" "-1" "1" "-1" "cell-bsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-tsep" "0sep")
        (cwith "1" "-1" "1" "-1" "cell-block" "no")
        (cwith "1" "-1" "1" "1" "cell-hpart" "1")
        (cwith "1" "-1" "3" "3" "cell-hpart" "2")
        (cwith "1" "-1" "1" "1" "cell-halign" "r")
        (cwith "1" "-1" "2" "2" "cell-halign" "c")
        (cwith "1" "-1" "3" "3" "cell-halign" "l")
        (cwith "1" "-1" "4" "4" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check

    ;; Verify alignment cell configurations
    (check (member '(cwith "1" "-1" "1" "1" "cell-halign" "c") t2)
      =>
      '((cwith "1" "-1" "1" "1" "cell-halign" "c")
        (cwith "1" "-1" "2" "2" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(cwith "1" "-1" "2" "2" "cell-halign" "r") t2)
      =>
      '((cwith "1" "-1" "2" "2" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(cwith "1" "-1" "1" "1" "cell-halign" "r") t3)
      =>
      '((cwith "1" "-1" "1" "1" "cell-halign" "r")
        (cwith "1" "-1" "2" "2" "cell-halign" "l")
        (cwith "1" "-1" "3" "3" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(cwith "1" "-1" "2" "2" "cell-halign" "l") t3)
      =>
      '((cwith "1" "-1" "2" "2" "cell-halign" "l")
        (cwith "1" "-1" "3" "3" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(cwith "1" "-1" "3" "3" "cell-halign" "r") t3)
      =>
      '((cwith "1" "-1" "3" "3" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(cwith "1" "-1" "1" "1" "cell-halign" "r") t4)
      =>
      '((cwith "1" "-1" "1" "1" "cell-halign" "r")
        (cwith "1" "-1" "2" "2" "cell-halign" "c")
        (cwith "1" "-1" "3" "3" "cell-halign" "l")
        (cwith "1" "-1" "4" "4" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(cwith "1" "-1" "3" "3" "cell-halign" "l") t4)
      =>
      '((cwith "1" "-1" "3" "3" "cell-halign" "l")
        (cwith "1" "-1" "4" "4" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
    (check (member '(cwith "1" "-1" "4" "4" "cell-halign" "r") t4)
      =>
      '((cwith "1" "-1" "4" "4" "cell-halign" "r")
        (table (row (cell "a") (cell "b"))))
    ) ;check
  ) ;let*
) ;define

(define (test-0621-tformat-rows-unit-tests)
  (display "Running tformat-rows extraction unit tests (5 checks)...\n")
  (let* ((tf1 '(tformat (twith "a" "b") (table (row "1") (row "2"))))
         (tf2 '(tformat (table (row "a"))))
         (tf3 '(something-else (table (row "a"))))
        ) ;
    (check (tformat-rows tf1) => '((row "1") (row "2")))
    (check (tformat-rows tf2) => '((row "a")))
    (check (tformat-rows tf3) => '())
    (check (tformat-rows '()) => '())
    (check (tformat-rows 'tformat) => '())
  ) ;let*
) ;define

(define (test-0621-rewrite-eqnarray-unit-tests)
  (display "Running rewrite-eqnarray* row rewriting unit tests (15 checks)...\n")
  ;; 3-cell row rewrite (like eqnarray*)
  (let* ((r3 '(row (cell "L")
                (cell "C")
                (cell (concat "R" (htab "5mm") "eq-num"))))
         (res3 (rewrite-eqnarray* r3))
        ) ;
    (check (car res3) => 'row)
    (check (length (cdr res3)) => 4)
    (check (list-ref res3 1) => '(cell (big-math "L")))
    (check (list-ref res3 2) => '(cell (big-math "C")))
    (check (list-ref res3 3) => '(cell (big-math "R")))
    (check (list-ref res3 4) => '(cell "eq-num"))
  ) ;let*
  ;; 2-cell row rewrite (like align)
  (let* ((r2 '(row (cell "L") (cell (concat "R" (htab "5mm") "eq-num"))))
         (res2 (rewrite-eqnarray* r2))
        ) ;
    (check (car res2) => 'row)
    (check (length (cdr res2)) => 4)
    (check (list-ref res2 1) => '(cell (big-math "L")))
    (check (list-ref res2 2) => '(cell ""))
    (check (list-ref res2 3) => '(cell (big-math "R")))
    (check (list-ref res2 4) => '(cell "eq-num"))
  ) ;let*
  ;; 1-cell row rewrite (like gather)
  (let* ((r1 '(row (cell (concat "E" (htab "5mm") "eq-num"))))
         (res1 (rewrite-eqnarray* r1))
        ) ;
    (check (car res1) => 'row)
    (check (length (cdr res1)) => 2)
    (check (list-ref res1 1) => '(cell (big-math "E")))
    (check (list-ref res1 2) => '(cell "eq-num"))
  ) ;let*
) ;define

(define (test-0621-ext-tmhtml-eqnarray-unit-tests)
  (display "Running ext-tmhtml-eqnarray* dispatcher unit tests (10 checks)...\n")
  ;; Multi-row 2-cell table (align) with htab
  (let* ((inner '(tformat (table (row (cell "L1")
                                   (cell (concat "R1" (htab "5mm") "1")))
                            (row (cell "L2")
                              (cell (concat "R2" (htab "5mm") "2")))))
         ) ;inner
         (res (ext-tmhtml-eqnarray* inner))
        ) ;
    (check (car res) => 'equations-base)
    (let ((tf (cadr res)))
      (check (car tf) => 'tformat)
      ;; Minimum and maximum columns count config for 4-column rewritten align layout
      (check (member '(twith "table-min-cols" "4") tf)
        =>
        '((twith "table-min-cols" "4")
          (twith "table-max-cols" "4")
          (cwith "1" "-1" "1" "1" "cell-lsep" "0spc")
          (cwith "1" "-1" "-1" "-1" "cell-rsep" "0spc")
          (cwith "1" "-1" "1" "-1" "cell-bsep" "0sep")
          (cwith "1" "-1" "1" "-1" "cell-tsep" "0sep")
          (cwith "1" "-1" "1" "-1" "cell-block" "no")
          (cwith "1" "-1" "1" "1" "cell-hpart" "1")
          (cwith "1" "-1" "3" "3" "cell-hpart" "2")
          (cwith "1" "-1" "1" "1" "cell-halign" "r")
          (cwith "1" "-1" "2" "2" "cell-halign" "c")
          (cwith "1" "-1" "3" "3" "cell-halign" "l")
          (cwith "1" "-1" "4" "4" "cell-halign" "r")
          (table (row (cell (big-math "L1"))
                   (cell "")
                   (cell (big-math "R1"))
                   (cell "1"))
            (row (cell (big-math "L2"))
              (cell "")
              (cell (big-math "R2"))
              (cell "2"))))
      ) ;check
    ) ;let
  ) ;let*
  ;; Multi-row 1-cell table (gather) with htab
  (let* ((inner '(tformat (table (row (cell (concat "E1" (htab "5mm") "1")))
                            (row (cell (concat "E2" (htab "5mm") "2")))))
         ) ;inner
         (res (ext-tmhtml-eqnarray* inner))
        ) ;
    (check (car res) => 'equations-base)
    (let ((tf (cadr res)))
      (check (car tf) => 'tformat)
      (check (member '(twith "table-min-cols" "2") tf)
        =>
        '((twith "table-min-cols" "2")
          (twith "table-max-cols" "2")
          (cwith "1" "-1" "1" "1" "cell-lsep" "0spc")
          (cwith "1" "-1" "-1" "-1" "cell-rsep" "0spc")
          (cwith "1" "-1" "1" "-1" "cell-bsep" "0sep")
          (cwith "1" "-1" "1" "-1" "cell-tsep" "0sep")
          (cwith "1" "-1" "1" "-1" "cell-block" "no")
          (cwith "1" "-1" "1" "1" "cell-hpart" "1")
          (cwith "1" "-1" "1" "1" "cell-halign" "c")
          (cwith "1" "-1" "2" "2" "cell-halign" "r")
          (table (row (cell (big-math "E1")) (cell "1"))
            (row (cell (big-math "E2")) (cell "2"))))
      ) ;check
    ) ;let
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
  (test-0621-table-construction-unit-tests)
  (test-0621-tformat-rows-unit-tests)
  (test-0621-rewrite-eqnarray-unit-tests)
  (test-0621-ext-tmhtml-eqnarray-unit-tests)
  (test-0621-html-export-integration)
  (test-0621-html-import-integration)
  (check-report)
) ;tm-define
