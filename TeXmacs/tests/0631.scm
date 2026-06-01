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
  (with path
    (string-append "$TEXMACS_PATH/tests/tex/" path)
    (string-replace (string-load path) "\r\n" "\n")
  ) ;with
) ;define

(define (has-cwith-property? options
          row-start
          row-end
          col-start
          col-end
          property
          value-pred
        ) ;has-cwith-property?
  (cond ((null? options) #f)
        ((and (pair? (car options)) (eq? (caar options) 'cwith))
         (let* ((opt (car options))
                (r-start (and (> (length opt) 1) (list-ref opt 1)))
                (r-end (and (> (length opt) 2) (list-ref opt 2)))
                (c-start (and (> (length opt) 3) (list-ref opt 3)))
                (c-end (and (> (length opt) 4) (list-ref opt 4)))
                (prop (and (> (length opt) 5) (list-ref opt 5)))
                (val (and (> (length opt) 6) (list-ref opt 6)))
               ) ;
           (if (and (or (not row-start) (equal? r-start row-start))
                 (or (not row-end) (equal? r-end row-end))
                 (or (not col-start) (equal? c-start col-start))
                 (or (not col-end) (equal? c-end col-end))
                 (or (not property) (equal? prop property))
                 (and val (value-pred val))
               ) ;and
             #t
             (has-cwith-property? (cdr options)
               row-start
               row-end
               col-start
               col-end
               property
               value-pred
             ) ;has-cwith-property?
           ) ;if
         ) ;let*
        ) ;
        (else (has-cwith-property? (cdr options)
                row-start
                row-end
                col-start
                col-end
                property
                value-pred
              ) ;has-cwith-property?
        ) ;else
  ) ;cond
) ;define

(define (has-cwith-in-tree? x row-start row-end col-start col-end property value-pred)
  (cond ((null? x) #f)
        ((and (pair? x) (eq? (car x) 'tformat))
         (or (has-cwith-property? (cdr x)
               row-start
               row-end
               col-start
               col-end
               property
               value-pred
             ) ;has-cwith-property?
           (let loop-children
             ((children (cdr x)))
             (cond ((null? children) #f)
                   ((has-cwith-in-tree? (car children)
                      row-start
                      row-end
                      col-start
                      col-end
                      property
                      value-pred
                    ) ;has-cwith-in-tree?
                    #t
                   ) ;
                   (else (loop-children (cdr children)))
             ) ;cond
           ) ;let
         ) ;or
        ) ;
        ((pair? x)
         (or (has-cwith-in-tree? (car x)
               row-start
               row-end
               col-start
               col-end
               property
               value-pred
             ) ;has-cwith-in-tree?
           (has-cwith-in-tree? (cdr x)
             row-start
             row-end
             col-start
             col-end
             property
             value-pred
           ) ;has-cwith-in-tree?
         ) ;or
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (find-table-num-rows children)
  (cond ((null? children) 0)
        ((and (pair? (car children)) (eq? (caar children) 'table))
         (length (cdar children))
        ) ;
        (else (find-table-num-rows (cdr children)))
  ) ;cond
) ;define

(define (is-three-line-table-tformat? x)
  (if (and (pair? x) (eq? (car x) 'tformat))
    (let* ((options (cdr x)) (num-rows (find-table-num-rows options)))
      (if (> num-rows 0)
        (let* ((has-top? (has-cwith-property? options
                           "1"
                           "1"
                           #f
                           #f
                           "cell-tborder"
                           (lambda (v) (not (equal? v "0ln")))
                         ) ;has-cwith-property?
               ) ;has-top?
               (has-bottom? (has-cwith-property? options
                              (number->string num-rows)
                              (number->string num-rows)
                              #f
                              #f
                              "cell-bborder"
                              (lambda (v) (not (equal? v "0ln")))
                            ) ;has-cwith-property?
               ) ;has-bottom?
               (has-vertical? (has-cwith-property? options
                                #f
                                #f
                                #f
                                #f
                                "cell-lborder"
                                (lambda (v) (not (equal? v "0ln")))
                              ) ;has-cwith-property?
               ) ;has-vertical?
               (has-vertical-r? (has-cwith-property? options
                                  #f
                                  #f
                                  #f
                                  #f
                                  "cell-rborder"
                                  (lambda (v) (not (equal? v "0ln")))
                                ) ;has-cwith-property?
               ) ;has-vertical-r?
              ) ;
          (and has-top? has-bottom? (not has-vertical?) (not has-vertical-r?))
        ) ;let*
        #f
      ) ;if
    ) ;let*
    #f
  ) ;if
) ;define

(define (transform-three-line-tables x)
  (cond ((null? x) '())
        ((and (pair? x) (eq? (car x) 'tformat))
         (let ((transformed-args (map transform-three-line-tables (cdr x))))
           (let ((new-tformat (cons 'tformat transformed-args)))
             (if (is-three-line-table-tformat? new-tformat)
               (list 'three-line-table new-tformat)
               new-tformat
             ) ;if
           ) ;let
         ) ;let
        ) ;
        ((pair? x)
         (cons (transform-three-line-tables (car x))
           (transform-three-line-tables (cdr x))
         ) ;cons
        ) ;
        (else x)
  ) ;cond
) ;define

(define (clean-multirow t)
  (cond ((null? t) (cons '() #f))
        ((and (pair? t) (eq? (car t) 'multirow))
         (let ((n (list-ref t 1))
               (w (list-ref t 2))
               (text (if (> (length t) 3) (list-ref t 3) ""))
              ) ;
           (cons text (cons n w))
         ) ;let
        ) ;
        ((pair? t)
         (let* ((res-car (clean-multirow (car t))) (res-cdr (clean-multirow (cdr t))))
           (cond ((cdr res-car) (cons (cons (car res-car) (car res-cdr)) (cdr res-car)))
                 ((cdr res-cdr) (cons (cons (car res-car) (car res-cdr)) (cdr res-cdr)))
                 (else (cons (cons (car res-car) (car res-cdr)) #f))
           ) ;cond
         ) ;let*
        ) ;
        (else (cons t #f))
  ) ;cond
) ;define

(define (process-row-cells cells r c options-acc new-cells-acc)
  (cond ((null? cells) (cons (reverse new-cells-acc) options-acc))
        (else (let* ((cell (car cells))
                     (cleaned-res (clean-multirow cell))
                     (new-cell (car cleaned-res))
                     (info (cdr cleaned-res))
                    ) ;
                (if info
                  (let* ((n (car info))
                         (row-str (number->string r))
                         (col-str (number->string c))
                         (new-opt1 (list 'cwith row-str row-str col-str col-str "cell-row-span" n))
                         (new-opt2 (list 'cwith row-str row-str col-str col-str "cell-valign" "c"))
                        ) ;
                    (process-row-cells (cdr cells)
                      r
                      (+ c 1)
                      (cons new-opt1 (cons new-opt2 options-acc))
                      (cons new-cell new-cells-acc)
                    ) ;process-row-cells
                  ) ;let*
                  (process-row-cells (cdr cells) r (+ c 1) options-acc (cons cell new-cells-acc))
                ) ;if
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (process-table-rows rows r options-acc new-rows-acc)
  (cond ((null? rows) (cons (reverse new-rows-acc) options-acc))
        (else (let* ((row (car rows))
                     (cells (cdr row))
                     (res-cells (process-row-cells cells r 1 '() '()))
                    ) ;
                (process-table-rows (cdr rows)
                  (+ r 1)
                  (append options-acc (cdr res-cells))
                  (cons (cons 'row (car res-cells)) new-rows-acc)
                ) ;process-table-rows
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (filter-table options)
  (cond ((null? options) '())
        ((and (pair? (car options)) (eq? (caar options) 'table))
         (filter-table (cdr options))
        ) ;
        (else (cons (car options) (filter-table (cdr options))))
  ) ;cond
) ;define

(define (collect-all-regions options num-rows)
  (let loop-r
    ((r 1) (regions '()))
    (if (> r num-rows)
      regions
      (let* ((r-str (number->string r))
             (row-regions (let loop-c
                            ((c 1) (c-acc '()))
                            (if (> c 50)
                              c-acc
                              (let* ((c-str (number->string c))
                                     (h-val (let loop-opt
                                              ((lst options))
                                              (cond ((null? lst) #f)
                                                    ((and (pair? (car lst))
                                                       (eq? (caar lst) 'cwith)
                                                       (equal? (list-ref (car lst) 1) r-str)
                                                       (equal? (list-ref (car lst) 3) c-str)
                                                       (equal? (list-ref (car lst) 5) "cell-row-span")
                                                     ) ;and
                                                     (list-ref (car lst) 6)
                                                    ) ;
                                                    (else (loop-opt (cdr lst)))
                                              ) ;cond
                                            ) ;let
                                     ) ;h-val
                                     (w-val (let loop-opt
                                              ((lst options))
                                              (cond ((null? lst) #f)
                                                    ((and (pair? (car lst))
                                                       (eq? (caar lst) 'cwith)
                                                       (equal? (list-ref (car lst) 1) r-str)
                                                       (equal? (list-ref (car lst) 3) c-str)
                                                       (equal? (list-ref (car lst) 5) "cell-col-span")
                                                     ) ;and
                                                     (list-ref (car lst) 6)
                                                    ) ;
                                                    (else (loop-opt (cdr lst)))
                                              ) ;cond
                                            ) ;let
                                     ) ;w-val
                                     (h (if h-val (string->number h-val) 1))
                                     (w (if w-val (string->number w-val) 1))
                                    ) ;
                                (if (or (> h 1) (> w 1))
                                  (loop-c (+ c 1) (cons (list r c h w) c-acc))
                                  (loop-c (+ c 1) c-acc)
                                ) ;if
                              ) ;let*
                            ) ;if
                          ) ;let
             ) ;row-regions
            ) ;
        (loop-r (+ r 1) (append regions row-regions))
      ) ;let*
    ) ;if
  ) ;let
) ;define

(define (is-cell-covered? ri ci regions)
  (cond ((null? regions) #f)
        (else (let* ((reg (car regions))
                     (r (list-ref reg 0))
                     (c (list-ref reg 1))
                     (h (list-ref reg 2))
                     (w (list-ref reg 3))
                    ) ;
                (if (and (>= ri r)
                      (< ri (+ r h))
                      (>= ci c)
                      (< ci (+ c w))
                      (not (and (= ri r) (= ci c)))
                    ) ;and
                  #t
                  (is-cell-covered? ri ci (cdr regions))
                ) ;if
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (clean-covered-cells-in-row cells r c regions new-cells-acc)
  (cond ((null? cells) (reverse new-cells-acc))
        (else (let* ((cell (car cells))
                     (new-cell (if (is-cell-covered? r c regions) '(cell "") cell))
                    ) ;
                (clean-covered-cells-in-row (cdr cells)
                  r
                  (+ c 1)
                  regions
                  (cons new-cell new-cells-acc)
                ) ;clean-covered-cells-in-row
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (clean-covered-cells-in-rows rows r regions new-rows-acc)
  (cond ((null? rows) (reverse new-rows-acc))
        (else (let* ((row (car rows))
                     (cells (cdr row))
                     (new-cells (clean-covered-cells-in-row cells r 1 regions '()))
                     (new-row (cons 'row new-cells))
                    ) ;
                (clean-covered-cells-in-rows (cdr rows)
                  (+ r 1)
                  regions
                  (cons new-row new-rows-acc)
                ) ;clean-covered-cells-in-rows
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (transform-multirow-tformat x)
  (if (and (pair? x) (eq? (car x) 'tformat))
    (let* ((options (cdr x))
           (table-cell-pair (let loop
                              ((lst options))
                              (cond ((null? lst) #f)
                                    ((and (pair? (car lst)) (eq? (caar lst) 'table)) (car lst))
                                    (else (loop (cdr lst)))
                              ) ;cond
                            ) ;let
           ) ;table-cell-pair
          ) ;
      (if table-cell-pair
        (let* ((table-rows (cdr table-cell-pair))
               (processed (process-table-rows table-rows 1 '() '()))
               (new-rows-temp (car processed))
               (new-options (cdr processed))
               (all-options (append (filter-table options) new-options))
               (num-rows (length new-rows-temp))
               (regions (collect-all-regions all-options num-rows))
               (new-rows (clean-covered-cells-in-rows new-rows-temp 1 regions '()))
               (rebuilt-options (append all-options (list (cons 'table new-rows))))
              ) ;
          (cons 'tformat rebuilt-options)
        ) ;let*
        x
      ) ;if
    ) ;let*
    x
  ) ;if
) ;define

(define (transform-multirow x)
  (cond ((null? x) '())
        ((and (pair? x) (eq? (car x) 'tformat))
         (let* ((new-t (transform-multirow-tformat x))
                (transformed-args (map transform-multirow (cdr new-t)))
               ) ;
           (cons 'tformat transformed-args)
         ) ;let*
        ) ;
        ((pair? x) (cons (transform-multirow (car x)) (transform-multirow (cdr x))))
        (else x)
  ) ;cond
) ;define

(define (stree-contains? x target)
  (cond ((null? x) #f)
        ((equal? x target) #t)
        ((pair? x)
         (or (stree-contains? (car x) target) (stree-contains? (cdr x) target))
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (test-latex-table-import)
  (display "Testing 45 extreme cases of LaTeX table import...\n")
  (let* ((latex-content (load-latex "0631_table_import.tex"))
         (parsed (parse-latex-document latex-content))
         (texmacs-tree (latex->texmacs parsed))
         (st (tree->stree texmacs-tree))
        ) ;

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

    ;; 7. Check multirow resolution and cleanliness
    ;; There must be NO undefined multirow macro in the final tree
    (check (stree-contains? st 'multirow) => #f)
    ;; Check new extreme cases of multirow combined/nested
    (check (stree-contains? st "DualHeader") => #t)
    (check (stree-contains? st "Multi 1") => #t)
    (check (stree-contains? st "Span Four Rows") => #t)
    (check (stree-contains? st "Combined MultiRowCol Width") => #t)
    (check (stree-contains? st "Extreme Nested Combined") => #t)

    ;; 8. Check column width extraction from p{width} specifications
    (check (has-cwith-in-tree? st
             "1"
             "-1"
             "1"
             "1"
             "cell-width"
             (lambda (v) (equal? v "3cm"))
           ) ;has-cwith-in-tree?
      =>
      #t
    ) ;check
    (check (has-cwith-in-tree? st
             "1"
             "-1"
             "2"
             "2"
             "cell-width"
             (lambda (v) (equal? v "4.5cm"))
           ) ;has-cwith-in-tree?
      =>
      #t
    ) ;check
  ) ;let*
) ;define

(tm-define (test_0631) (test-latex-table-import) (check-report))
