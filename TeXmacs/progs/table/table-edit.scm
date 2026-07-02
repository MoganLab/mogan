
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : table-edit.scm
;; DESCRIPTION : routines for manipulating tables
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (table table-edit)
  (:use (utils library tree)
    (utils base environment)
    (utils edit variants)
    (utils edit selections)
    (utils library cursor)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Some drd properties, which should go into table-drd.scm later on
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-group variant-tag (table-tag) (wide-table-tag))
(define-group similar-tag (table-tag) (wide-table-tag))

(define-group table-tag tabular tabular* block block*)

(define-group wide-table-tag wide-tabular wide-block)

(tm-define (any-table-tag? l)
  (with t
    (get-env-tree (if (symbol? l) (symbol->string l) l))
    (and (tree-func? t 'macro 2)
      (nnull? (tree-search (tree-ref t 1)
                (lambda (st)
                  (and (tree-func? st 'tformat)
                    (tree-func? (tm-ref st :last) 'arg)
                    (tm-equal? (tm-ref st :last 0) (tm-ref t 0))
                  ) ;and
                ) ;lambda
              ) ;tree-search
      ) ;nnull?
    ) ;and
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Supplementary routines for cetting cell and table formats
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (cell-set-format* var val)
  (when val
    (keep-table-selection (cell-set-format var val)
      (cond ((and (== var "cell-hmode") (== val "auto")) (cell-set-format "cell-width" ""))
            ((and (== var "cell-width")
               (!= val "")
               (== (cell-get-format "cell-hmode") "auto")
             ) ;and
             (cell-set-format "cell-hmode" "exact")
            ) ;
            ((and (== var "cell-vmode") (== val "auto")) (cell-set-format "cell-height" ""))
            ((and (== var "cell-height")
               (!= val "")
               (== (cell-get-format "cell-vmode") "auto")
             ) ;and
             (cell-set-format "cell-vmode" "exact")
            ) ;
      ) ;cond
      ;; (refresh-now "cell-properties")
    ) ;keep-table-selection
  ) ;when
) ;tm-define

(tm-define (cell-set-format-list vars vals)
  (if (selection-active-any?)
    (let ((sp (position-new)) (ep (position-new)))
      (position-set sp (selection-get-start))
      (position-set ep (selection-get-end))
      (map (lambda (var val)
             (selection-set (position-get sp) (position-get ep))
             (cell-set-format var val)
           ) ;lambda
        vars
        vals
      ) ;map
      (position-delete sp)
      (position-delete ep)
    ) ;let
    (map cell-set-format vars vals)
  ) ;if
) ;tm-define

(tm-define (cell-set-format-list* vars vals)
  (keep-table-selection (cell-set-format-list vars vals)
    ;; (refresh-now "cell-properties")
  ) ;keep-table-selection
) ;tm-define

(tm-define (table-set-format* var val)
  (when val
    (table-set-format var val)
    (cond ((and (== var "table-hmode") (== val "auto"))
           (table-set-format "table-width" "")
          ) ;
          ((and (== var "table-width")
             (!= val "")
             (== (table-get-format "table-hmode") "auto")
           ) ;and
           (table-set-format "table-hmode" "exact")
          ) ;
          ((and (== var "table-vmode") (== val "auto"))
           (table-set-format "table-height" "")
          ) ;
          ((and (== var "table-height")
             (!= val "")
             (== (table-get-format "table-vmode") "auto")
           ) ;and
           (table-set-format "table-vmode" "exact")
          ) ;
    ) ;cond
    ;; (refresh-now "table-properties")
  ) ;when
) ;tm-define

(tm-define (table-set-format-list vars vals) (map table-set-format vars vals))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Inserting rows and columns
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-enter t shift?)
  (:require (table-markup-context? t))
  (let ((x (inside-which '(table document))))
    (cond ((== x 'document) (insert-return))
          (else (table-insert-row #t) (table-go-to (table-which-row) 1))
    ) ;cond
  ) ;let
) ;tm-define

(tm-define (structured-insert-horizontal t forwards?)
  (:require (table-markup-context? t))
  (table-insert-column forwards?)
) ;tm-define

(tm-define (structured-insert-vertical t downwards?)
  (:require (table-markup-context? t))
  (table-insert-row downwards?)
) ;tm-define

(tm-define (structured-remove-horizontal t forwards?)
  (:require (table-markup-context? t))
  (table-remove-column forwards?)
) ;tm-define

(tm-define (structured-remove-vertical t downwards?)
  (:require (table-markup-context? t))
  (table-remove-row downwards?)
) ;tm-define

(tm-define (table-resize-notify t)
  (when (chat-input-buffer? (current-buffer-url))
    (qt-chat-notify-input-height)
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Posititioning
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (geometry-default t)
  (:require (table-markup-context? t))
  (with-focus-after t (cell-del-format ""))
) ;tm-define

(tm-define (geometry-horizontal t forward?)
  (:require (table-markup-context? t))
  (with-focus-after t (if forward? (cell-halign-right) (cell-halign-left)))
) ;tm-define

(tm-define (geometry-vertical t down?)
  (:require (table-markup-context? t))
  (with-focus-after t (if down? (cell-valign-down) (cell-valign-up)))
) ;tm-define

(tm-define (swipe-horizontal t forward?)
  (:require (table-markup-context? t))
  (geometry-horizontal t forward?)
) ;tm-define

(tm-define (swipe-vertical t down?)
  (:require (table-markup-context? t))
  (geometry-vertical t down?)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured traversal
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (cell-search-downwards t)
  (if (tree-is? t 'cell)
    t
    (and (tree-down t) (cell-search-downwards (tree-down t)))
  ) ;if
) ;define

(define (table-non-extremal-context? t downwards?)
  (and (table-markup-context? t)
    (and-with c
      (cell-search-downwards t)
      (and-with i
        (tree-index (tree-up c))
        (if downwards? (< i (- (tree-arity (tree-up c 2)) 1)) (> i 0))
      ) ;and-with
    ) ;and-with
  ) ;and
) ;define

(define (cell-move-absolute c row col)
  (let* ((r (tree-up c)) (t (tree-up r)))
    (if (and (>= row 0) (< row (tree-arity t)) (>= col 0) (< col (tree-arity r)))
      (begin
        (tree-go-to c :start)
        (table-go-to (+ row 1) (+ col 1))
      ) ;begin
    ) ;if
  ) ;let*
) ;define

(define (cell-move-relative c drow dcol)
  (let* ((r (tree-up c))
         (t (tree-up r))
         (row (+ (tree-index r) drow))
         (col (+ (tree-index c) dcol))
        ) ;
    (cell-move-absolute c row col)
  ) ;let*
) ;define

(tm-define (traverse-vertical t downwards?)
  (:require (table-non-extremal-context? t downwards?))
  (and-with c
    (cell-search-downwards t)
    (cell-move-relative c (if downwards? 1 -1) 0)
  ) ;and-with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured movements
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-horizontal t forwards?)
  (:require (table-markup-context? t))
  (with-focus-after t
    (and-with c
      (cell-search-downwards t)
      (cell-move-relative c 0 (if forwards? 1 -1))
    ) ;and-with
  ) ;with-focus-after
) ;tm-define

(tm-define (structured-vertical t downwards?)
  (:require (table-markup-context? t))
  (with-focus-after t
    (and-with c
      (cell-search-downwards t)
      (cell-move-relative c (if downwards? 1 -1) 0)
    ) ;and-with
  ) ;with-focus-after
) ;tm-define

(tm-define (structured-inner-extremal t forwards?)
  (:require (table-markup-context? t))
  (with-focus-after t
    (and-with c (cell-search-downwards t) (tree-go-to c (if forwards? :end :start)))
  ) ;with-focus-after
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Commands for tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (table-interactive-set var)
  (:interactive #t)
  (interactive (lambda (s) (table-set-format* var s))
    (logic-ref env-var-description% var)
  ) ;interactive
) ;tm-define

(tm-define (table-set-rows nr)
  (:synopsis "Set the number of table rows")
  (:argument nr "Number of rows")
  (table-set-extents (string->number nr) (table-nr-columns))
) ;tm-define

(tm-define (table-set-columns nr)
  (:synopsis "Set the number of table columns")
  (:argument nr "Number of columns")
  (table-set-extents (table-nr-rows) (string->number nr))
) ;tm-define

(define (table-get-width)
  (table-get-format "table-width")
) ;define

(define (table-get-hmode)
  (table-get-format "table-hmode")
) ;define

(define (table-test-automatic-width?)
  (or (== (table-get-width) "") (== (table-get-hmode) "auto"))
) ;define
(tm-define (table-set-automatic-width)
  (:synopsis "Automatic determination of table width")
  (:check-mark "o" table-test-automatic-width?)
  (table-set-format-list '("table-width" "table-hmode") '("" "auto"))
) ;tm-define

(tm-define (table-test-minimal-width? . args)
  (and (!= (table-get-width) "") (== (table-get-hmode) "max"))
) ;tm-define
(tm-define (table-set-minimal-width w)
  (:synopsis "Set minimal table width")
  (:argument w "Minimal table width")
  (:check-mark "o" table-test-minimal-width?)
  (table-set-format-list '("table-width" "table-hmode") `(,w ,"max"))
) ;tm-define
(tm-define (table-ia-minimal-width)
  (:interactive #t)
  (:check-mark "o" table-test-minimal-width?)
  (interactive table-set-minimal-width)
) ;tm-define

(tm-define (table-test-exact-width? . args)
  (and (if (null? args) (!= (table-get-width) "") (== (table-get-width) (car args)))
    (== (table-get-hmode) "exact")
  ) ;and
) ;tm-define
(tm-define (table-set-exact-width w)
  (:synopsis "Set table width")
  (:argument w "Table width")
  (:check-mark "o" table-test-exact-width?)
  (table-set-format-list '("table-width" "table-hmode") `(,w ,"exact"))
) ;tm-define
(tm-define (table-ia-exact-width)
  (:interactive #t)
  (:check-mark "o" table-test-exact-width?)
  (interactive table-set-exact-width)
) ;tm-define

(tm-define (table-test-maximal-width? . args)
  (and (!= (table-get-width) "") (== (table-get-hmode) "min"))
) ;tm-define
(tm-define (table-set-maximal-width w)
  (:synopsis "Set maximal table width")
  (:argument w "Maximal table width")
  (:check-mark "o" table-test-maximal-width?)
  (table-set-format-list '("table-width" "table-hmode") `(,w ,"min"))
) ;tm-define
(tm-define (table-ia-maximal-width)
  (:interactive #t)
  (:check-mark "o" table-test-maximal-width?)
  (interactive table-set-maximal-width)
) ;tm-define

(tm-define (table-test-parwidth?) (== (table-get-width) "1par"))
(tm-define (table-toggle-parwidth)
  (:check-mark "o" table-test-parwidth?)
  (if (table-test-parwidth?)
    (table-set-format-list '("table-width" "table-hmode") '("" ""))
    (table-set-format-list '("table-width" "table-hmode") '("1par" "exact"))
  ) ;if
) ;tm-define

(define (table-get-height)
  (table-get-format "table-height")
) ;define

(define (table-get-vmode)
  (table-get-format "table-vmode")
) ;define

(define (table-test-automatic-height?)
  (or (== (table-get-height) "") (== (table-get-vmode) "auto"))
) ;define
(tm-define (table-set-automatic-height)
  (:synopsis "Automatic determination of table height")
  (:check-mark "o" table-test-automatic-height?)
  (table-set-format-list '("table-height" "table-vmode") '("" "auto"))
) ;tm-define

(tm-define (table-test-minimal-height? . args)
  (and (!= (table-get-height) "") (== (table-get-vmode) "max"))
) ;tm-define
(tm-define (table-set-minimal-height h)
  (:synopsis "Set minimal table height")
  (:argument h "Minimal table height")
  (:check-mark "o" table-test-minimal-height?)
  (table-set-format-list '("table-height" "table-vmode") `(,h ,"max"))
) ;tm-define
(tm-define (table-ia-minimal-height)
  (:interactive #t)
  (:check-mark "o" table-test-minimal-height?)
  (interactive table-set-minimal-height)
) ;tm-define

(tm-define (table-test-exact-height? . args)
  (and (!= (table-get-height) "") (== (table-get-vmode) "exact"))
) ;tm-define
(tm-define (table-set-exact-height h)
  (:synopsis "Set table height")
  (:argument h "Table height")
  (:check-mark "o" table-test-exact-height?)
  (table-set-format-list '("table-height" "table-vmode") `(,h ,"exact"))
) ;tm-define
(tm-define (table-ia-exact-height)
  (:interactive #t)
  (:check-mark "o" table-test-exact-height?)
  (interactive table-set-exact-height)
) ;tm-define

(tm-define (table-test-maximal-height? . args)
  (and (!= (table-get-height) "") (== (table-get-vmode) "min"))
) ;tm-define
(tm-define (table-set-maximal-height h)
  (:synopsis "Set maximal table height")
  (:argument h "Maximal table height")
  (:check-mark "o" table-test-maximal-height?)
  (table-set-format-list '("table-height" "table-vmode") `(,h ,"min"))
) ;tm-define
(tm-define (table-ia-maximal-height)
  (:interactive #t)
  (:check-mark "o" table-test-maximal-height?)
  (interactive table-set-maximal-height)
) ;tm-define

(tm-define (table-set-padding padding)
  (:argument padding "Padding")
  (table-set-format-list (list "table-lsep" "table-rsep" "table-bsep" "table-tsep")
    (list padding padding padding padding)
  ) ;table-set-format-list
) ;tm-define

(tm-define (table-set-border border)
  (:argument border "Border width")
  (table-set-format-list (list "table-lborder" "table-rborder" "table-bborder" "table-tborder")
    (list border border border border)
  ) ;table-set-format-list
) ;tm-define

(define (table-get-border-color)
  (table-get-format "table-border-color")
) ;define

(define (table-test-border-color? s)
  (== (table-get-border-color) s)
) ;define
(tm-define (table-set-border-color s)
  (:synopsis "Set border color of table")
  (:argument s "Table border color")
  (:check-mark "o" table-test-border-color?)
  (table-set-format* "table-border-color" s)
) ;tm-define

(define (table-get-halign)
  (table-get-format "table-halign")
) ;define

(define (table-test-halign? s)
  (== (table-get-halign) s)
) ;define
(tm-define (table-set-halign s)
  (:synopsis "Set horizontal table alignment")
  (:check-mark "*" table-test-halign?)
  (table-set-format* "table-halign" s)
) ;tm-define

(define (table-test-specific-halign? . l)
  (== (table-get-halign) "O")
) ;define
(tm-define (table-specific-halign col)
  (:synopsis "Align horizontally at the baseline of a specific column")
  (:check-mark "*" table-test-specific-halign?)
  (:argument col "Align at column")
  (table-set-format-list (list "table-col-origin" "table-halign") (list col "O"))
) ;tm-define

(define (table-get-valign)
  (table-get-format "table-valign")
) ;define

(define (table-test-valign? s)
  (== (table-get-valign) s)
) ;define
(tm-define (table-set-valign s)
  (:synopsis "Set vertical table alignment")
  (:check-mark "*" table-test-valign?)
  (table-set-format* "table-valign" s)
) ;tm-define

(define (table-test-specific-valign? . l)
  (== (table-get-valign) "O")
) ;define
(tm-define (table-specific-valign row)
  (:synopsis "Align vertically at the baseline of a specific row")
  (:check-mark "*" table-test-specific-valign?)
  (:argument row "Align at row")
  (table-set-format-list (list "table-row-origin" "table-valign") (list row "O"))
) ;tm-define

(define (table-hyphen?)
  (== "y" (table-get-format "table-hyphen"))
) ;define

(define (table-set-hyphen s)
  (table-set-format* "table-hyphen" s)
) ;define
(tm-define (toggle-table-hyphen)
  (:synopsis "Toggle table hyphenation")
  (:check-mark "v" table-hyphen?)
  (table-set-hyphen (if (table-hyphen?) "n" "y"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Commands for cells in tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-cell-mode? s)
  (== (get-cell-mode) s)
) ;define
(tm-property (set-cell-mode s) (:check-mark "*" test-cell-mode?))

(tm-define (cell-interactive-set var)
  (:interactive #t)
  (interactive (lambda (s) (cell-set-format* var s))
    (logic-ref env-var-description% var)
  ) ;interactive
) ;tm-define

(tm-define (table-insert-blank-row h)
  (:synopsis "Insert a blank row below cursor")
  (:argument h "Height of row")
  (:default h "0.5spc")
  (with old-mode
    (get-cell-mode)
    (set-cell-mode "row")
    (table-insert-row #t)
    (cell-set-format "cell-background" "")
    (cell-set-format "cell-lborder" "0ln")
    (cell-set-format "cell-rborder" "0ln")
    (cell-set-format "cell-lsep" "0ln")
    (cell-set-format "cell-rsep" "0ln")
    (cell-set-format "cell-tsep" "0ln")
    (cell-set-format "cell-bsep" "0ln")
    (cell-set-format "cell-vcorrect" "n")
    (cell-set-format "cell-vmode" "exact")
    (cell-set-format "cell-height" h)
    (set-cell-mode old-mode)
  ) ;with
) ;tm-define

(tm-define (table-insert-blank-column w)
  (:synopsis "Insert a blank column at the right of the cursor")
  (:argument w "Width of the column")
  (:default w "0.5spc")
  (with old-mode
    (get-cell-mode)
    (set-cell-mode "column")
    (table-insert-column #t)
    (cell-set-format "cell-background" "")
    (cell-set-format "cell-tborder" "0ln")
    (cell-set-format "cell-bborder" "0ln")
    (cell-set-format "cell-lsep" "0ln")
    (cell-set-format "cell-rsep" "0ln")
    (cell-set-format "cell-tsep" "0ln")
    (cell-set-format "cell-bsep" "0ln")
    (cell-set-format "cell-vcorrect" "n")
    (cell-set-format "cell-hmode" "exact")
    (cell-set-format "cell-width" w)
    (set-cell-mode old-mode)
  ) ;with
) ;tm-define

(define (cell-get-width)
  (cell-get-format "cell-width")
) ;define

(define (cell-get-hmode)
  (cell-get-format "cell-hmode")
) ;define

(define (cell-test-automatic-width?)
  (or (== (cell-get-width) "") (== (cell-get-hmode) "auto"))
) ;define
(tm-define (cell-set-automatic-width)
  (:synopsis "Automatic determination of cell width")
  (:check-mark "o" cell-test-automatic-width?)
  (cell-set-format-list '("cell-width" "cell-hmode") '("" "auto"))
) ;tm-define

(tm-define (cell-test-minimal-width? . args)
  (and (!= (cell-get-width) "") (== (cell-get-hmode) "max"))
) ;tm-define
(tm-define (cell-set-minimal-width w)
  (:synopsis "Set minimal cell width")
  (:argument w "Minimal cell width")
  (:check-mark "o" cell-test-minimal-width?)
  (cell-set-format-list '("cell-width" "cell-hmode") `(,w ,"max"))
) ;tm-define
(tm-define (cell-ia-minimal-width)
  (:interactive #t)
  (:check-mark "o" cell-test-minimal-width?)
  (interactive cell-set-minimal-width)
) ;tm-define

(tm-define (cell-test-exact-width? . args)
  (and (!= (cell-get-width) "") (== (cell-get-hmode) "exact"))
) ;tm-define
(tm-define (cell-set-exact-width w)
  (:synopsis "Set cell width")
  (:argument w "Cell width")
  (:check-mark "o" cell-test-exact-width?)
  (cell-set-format-list '("cell-width" "cell-hmode") `(,w ,"exact"))
) ;tm-define
(tm-define (cell-ia-exact-width)
  (:interactive #t)
  (:check-mark "o" cell-test-exact-width?)
  (interactive cell-set-exact-width)
) ;tm-define

(tm-define (cell-test-maximal-width? . args)
  (and (!= (cell-get-width) "") (== (cell-get-hmode) "min"))
) ;tm-define
(tm-define (cell-set-maximal-width w)
  (:synopsis "Set maximal cell width")
  (:argument w "Maximal cell width")
  (:check-mark "o" cell-test-maximal-width?)
  (cell-set-format-list '("cell-width" "cell-hmode") `(,w ,"min"))
) ;tm-define
(tm-define (cell-ia-maximal-width)
  (:interactive #t)
  (:check-mark "o" cell-test-maximal-width?)
  (interactive cell-set-maximal-width)
) ;tm-define

(define (cell-get-height)
  (cell-get-format "cell-height")
) ;define

(define (cell-get-vmode)
  (cell-get-format "cell-vmode")
) ;define

(define (cell-test-automatic-height?)
  (or (== (cell-get-height) "") (== (cell-get-vmode) "auto"))
) ;define
(tm-define (cell-set-automatic-height)
  (:synopsis "Automatic determination of cell height")
  (:check-mark "o" cell-test-automatic-height?)
  (cell-set-format-list '("cell-height" "cell-vmode") '("" "auto"))
) ;tm-define

(tm-define (cell-test-minimal-height? . args)
  (and (!= (cell-get-height) "") (== (cell-get-vmode) "max"))
) ;tm-define
(tm-define (cell-set-minimal-height h)
  (:synopsis "Set minimal cell height")
  (:argument h "Minimal cell height")
  (:check-mark "o" cell-test-minimal-height?)
  (cell-set-format-list '("cell-height" "cell-vmode") `(,h ,"max"))
) ;tm-define
(tm-define (cell-ia-minimal-height)
  (:interactive #t)
  (:check-mark "o" cell-test-minimal-height?)
  (interactive cell-set-minimal-height)
) ;tm-define

(tm-define (cell-test-exact-height? . args)
  (and (!= (cell-get-height) "") (== (cell-get-vmode) "exact"))
) ;tm-define
(tm-define (cell-set-exact-height h)
  (:synopsis "Set cell height")
  (:argument h "Cell height")
  (:check-mark "o" cell-test-exact-height?)
  (cell-set-format-list '("cell-height" "cell-vmode") `(,h ,"exact"))
) ;tm-define
(tm-define (cell-ia-exact-height)
  (:interactive #t)
  (:check-mark "o" cell-test-exact-height?)
  (interactive cell-set-exact-height)
) ;tm-define

(tm-define (cell-test-maximal-height? . args)
  (and (!= (cell-get-height) "") (== (cell-get-vmode) "min"))
) ;tm-define
(tm-define (cell-set-maximal-height h)
  (:synopsis "Set maximal cell height")
  (:argument h "Maximal cell height")
  (:check-mark "o" cell-test-maximal-height?)
  (cell-set-format-list '("cell-height" "cell-vmode") `(,h ,"min"))
) ;tm-define
(tm-define (cell-ia-maximal-height)
  (:interactive #t)
  (:check-mark "o" cell-test-maximal-height?)
  (interactive cell-set-maximal-height)
) ;tm-define

(tm-define (cell-set-padding padding)
  (:argument padding "Cell padding")
  (cell-set-format-list '("cell-lsep" "cell-rsep" "cell-bsep" "cell-tsep")
    (make-list 4 padding)
  ) ;cell-set-format-list
) ;tm-define

(tm-define (cell-set-hpadding padding)
  (:argument padding "Horizontal cell padding")
  (cell-set-format-list '("cell-lsep" "cell-rsep") (make-list 2 padding))
) ;tm-define

(tm-define (cell-set-vpadding padding)
  (:argument padding "Vertical cell padding")
  (cell-set-format-list '("cell-bsep" "cell-tsep") (make-list 2 padding))
) ;tm-define

(tm-define (cell-set-span rs cs)
  (:argument rs "Row span")
  (:argument cs "Column span")
  (cell-set-format-list '("cell-row-span" "cell-col-span") (list rs cs))
) ;tm-define

(tm-define (cell-set-row-span rs)
  (:argument rs "Row span")
  (cell-set-format "cell-row-span" rs)
) ;tm-define

(tm-define (cell-set-column-span cs)
  (:argument cs "Column span")
  (cell-set-format "cell-col-span" cs)
) ;tm-define

(tm-define (cell-set-span-selection)
  (:synopsis "Sets the upper-left cell of a selection to span all of it")
  (when (selection-active-table?)
    (with (srow erow scol ecol)
      (table-which-cells)
      (table-go-to srow scol)
      (selection-cancel)
      (cell-set-row-span (number->string (- (+ erow 1) srow)))
      (cell-set-column-span (number->string (- (+ ecol 1) scol)))
    ) ;with
  ) ;when
) ;tm-define

(tm-define (cell-reset-span) (cell-set-span "1" "1"))

(tm-define (cell-spans-more?)
  (or (!= (cell-get-format "cell-row-span") "1")
    (!= (cell-get-format "cell-col-span") "1")
  ) ;or
) ;tm-define

(define (cell-get-halign)
  (cell-get-format "cell-halign")
) ;define

(define (cell-test-halign? s)
  (== (cell-get-halign) s)
) ;define
(tm-define (cell-set-halign s)
  (:synopsis "Set horizontal cell alignment")
  (:check-mark "o" cell-test-halign?)
  (cell-set-format* "cell-halign" s)
) ;tm-define

(define (cell-get-valign)
  (cell-get-format "cell-valign")
) ;define

(define (cell-test-valign? s)
  (== (cell-get-valign) s)
) ;define
(tm-define (cell-set-valign s)
  (:synopsis "Set vertical cell alignment")
  (:check-mark "o" cell-test-valign?)
  (cell-set-format* "cell-valign" s)
) ;tm-define

(define (cell-get-background)
  (cell-get-format "cell-background")
) ;define

(define (cell-test-background? s)
  (== (cell-get-background) s)
) ;define
(tm-define (cell-set-background s)
  (:synopsis "Set background color of cell")
  (:argument s "Cell color")
  (:check-mark "o" cell-test-background?)
  (cell-set-format* "cell-background" s)
) ;tm-define

(define (cell-get-border-color)
  (cell-get-format "cell-border-color")
) ;define

(define (cell-test-border-color? s)
  (== (cell-get-border-color) s)
) ;define
(tm-define (cell-set-border-color s)
  (:synopsis "Set border color of cell")
  (:argument s "Cell border color")
  (:check-mark "o" cell-test-border-color?)
  (cell-set-format* "cell-border-color" s)
) ;tm-define

(define (cell-get-vcorrect)
  (cell-get-format "cell-vcorrect")
) ;define

(define (cell-test-vcorrect? s)
  (== (cell-get-vcorrect) s)
) ;define
(tm-define (cell-set-vcorrect s)
  (:synopsis "Set vertical correction mode for cell")
  (:check-mark "o" cell-test-vcorrect?)
  (cell-set-format* "cell-vcorrect" s)
) ;tm-define

(define (cell-get-hyphen)
  (cell-get-format "cell-hyphen")
) ;define

(define (cell-test-hyphen? s)
  (== (cell-get-hyphen) s)
) ;define
(tm-define (cell-set-hyphen s)
  (:synopsis "Set cell wrapping mode")
  (:check-mark "o" cell-test-hyphen?)
  (cell-set-format* "cell-hyphen" s)
) ;tm-define

(tm-define (cell-test-wrap?) (!= (cell-get-hyphen) "n"))
(tm-define (cell-toggle-wrap)
  (:synopsis "Toggle cell wrapping mode")
  (:check-mark "o" cell-test-wrap?)
  (if (cell-test-wrap?)
    (cell-set-format* "cell-hyphen" "n")
    (cell-set-format* "cell-hyphen" "t")
  ) ;if
) ;tm-define

(define (cell-get-block)
  (cell-get-format "cell-block")
) ;define

(define (cell-test-block? s)
  (== (cell-get-block) s)
) ;define
(tm-define (cell-set-block s)
  (:synopsis "Does the cell contain block content?")
  (:check-mark "o" cell-test-block?)
  (cell-set-format* "cell-block" s)
) ;tm-define

(tm-define (cell-halign-left)
  (let* ((var "cell-halign") (old (cell-get-format var)))
    (cond ((== old "r") (cell-set-format* var "c"))
          (else (cell-set-format* var "l"))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (cell-halign-right)
  (let* ((var "cell-halign") (old (cell-get-format var)))
    (cond ((== old "l") (cell-set-format* var "c"))
          (else (cell-set-format* var "r"))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (cell-valign-down)
  (let* ((var "cell-valign") (old (cell-get-format var)))
    (cond ((== old "c") (cell-set-format* var "B"))
          ((== old "t") (cell-set-format* var "c"))
          (else (cell-set-format* var "b"))
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (cell-valign-up)
  (let* ((var "cell-valign") (old (cell-get-format var)))
    (cond ((== old "b") (cell-set-format* var "B"))
          ((== old "B") (cell-set-format* var "c"))
          (else (cell-set-format* var "t"))
    ) ;cond
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Set cell borders
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (table-select-cells r1 r2 c1 c2)
  (let* ((p1 (table-cell-path r1 c1)) (p2 (table-cell-path r2 c2)))
    (when (and (pair? p1) (pair? p2))
      (let* ((q1 (rcons (cDr p1) 0)) (q2 (rcons (cDr p2) 1)))
        (selection-set q1 q2)
      ) ;let*
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (cell-set-borders T B L R t b l r)
  (:argument T "Outer top border width")
  (:argument B "Outer bottom border width")
  (:argument L "Outer left border width")
  (:argument R "Outer right border width")
  (:argument t "Inner top border width")
  (:argument b "Inner bottom border width")
  (:argument l "Inner left border width")
  (:argument r "Inner right border width")
  (keep-table-selection (when (nnull? (table-get-extents))
                          (with (rows cols)
                            (table-get-extents)
                            (with (r1 r2 c1 c2)
                              (table-which-cells)
                              (let* ((vars (list "cell-tborder" "cell-bborder" "cell-lborder" "cell-rborder"))
                                     (vals (list t b l r))
                                     (and* (lambda (a b) (and b a)))
                                     (vars* (list-filter (map and* vars vals) identity))
                                     (vals* (list-filter vals identity))
                                    ) ;
                                (cell-set-format-list vars* vals*)
                              ) ;let*
                              (when T
                                (when (!= T t)
                                  (table-select-cells r1 r1 c1 c2)
                                  (cell-set-format "cell-tborder" T)
                                ) ;when
                                (when (> r1 1)
                                  (table-select-cells (- r1 1) (- r1 1) c1 c2)
                                  (cell-set-format "cell-bborder" T)
                                ) ;when
                              ) ;when
                              (when B
                                (when (!= B b)
                                  (table-select-cells r2 r2 c1 c2)
                                  (cell-set-format "cell-bborder" B)
                                ) ;when
                                (when (< r2 rows)
                                  (table-select-cells (+ r2 1) (+ r2 1) c1 c2)
                                  (cell-set-format "cell-tborder" B)
                                ) ;when
                              ) ;when
                              (when L
                                (when (!= L l)
                                  (table-select-cells r1 r2 c1 c1)
                                  (cell-set-format "cell-lborder" L)
                                ) ;when
                                (when (> c1 1)
                                  (table-select-cells r1 r2 (- c1 1) (- c1 1))
                                  (cell-set-format "cell-rborder" L)
                                ) ;when
                              ) ;when
                              (when R
                                (when (!= R r)
                                  (table-select-cells r1 r2 c2 c2)
                                  (cell-set-format "cell-rborder" R)
                                ) ;when
                                (when (< c2 cols)
                                  (table-select-cells r1 r2 (+ c2 1) (+ c2 1))
                                  (cell-set-format "cell-lborder" R)
                                ) ;when
                              ) ;when
                            ) ;with
                          ) ;with
                        ) ;when
  ) ;keep-table-selection
) ;tm-define

(tm-define (cell-set-border b)
  (:argument b "Border width")
  (cell-set-borders b b b b b b b b)
) ;tm-define

(tm-define (cell-set-hborder b)
  (:argument b "Horizontal border width")
  (cell-set-borders #f #f b b #f #f b b)
) ;tm-define

(tm-define (cell-set-vborder b)
  (:argument b "Vertical border width")
  (cell-set-borders b b #f #f b b #f #f)
) ;tm-define

(tm-define (cell-set-lborder b)
  (:argument b "Left border width")
  (cell-set-borders #f #f b #f #f #f b #f)
) ;tm-define

(tm-define (cell-set-rborder b)
  (:argument b "Right border width")
  (cell-set-borders #f #f #f b #f #f #f b)
) ;tm-define

(tm-define (cell-set-bborder b)
  (:argument b "Bottom border width")
  (cell-set-borders #f b #f #f #f b #f #f)
) ;tm-define

(tm-define (cell-set-tborder b)
  (:argument b "Top border width")
  (cell-set-borders b #f #f #f b #f #f #f)
) ;tm-define

(tm-define (cell-set-dborder b)
  (:argument b "Diagonal border width")
  (cell-set-format* "cell-dborder" b)
) ;tm-define

(tm-define (cell-set-aborder b)
  (:argument b "Anti-diagonal border width")
  (cell-set-format* "cell-aborder" b)
) ;tm-define

(define cell-current-pen-width "1ln")

(define (cell-test-pen-width? pen)
  (== cell-current-pen-width pen)
) ;define

(tm-define (cell-get-pen-width) cell-current-pen-width)

(tm-define (cell-set-pen-width pen)
  (:argument pen "Pen width")
  (:check-mark "*" cell-test-pen-width?)
  (set! cell-current-pen-width pen)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special commands for full width math tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (tree-search-subtree-sub t u i)
  (if (>= i (tree-arity t))
    #f
    (let ((r (tree-search-subtree (tree-ref t i) u)))
      (if r r (tree-search-subtree-sub t u (+ i 1)))
    ) ;let
  ) ;if
) ;define

(define (tree-search-subtree t u)
  (cond ((== t u) t)
        ((tree-atomic? t) #f)
        (else (tree-search-subtree-sub t u 0))
  ) ;cond
) ;define

(define (table-search-number-equation)
  (let* ((row (table-which-row)) (st (table-cell-tree row -1)))
    (tree-search-subtree st (stree->tree '(eq-number)))
  ) ;let*
) ;define

(tm-define (numbered-numbered? t)
  (:require (tree-in? t '(eqnarray eqnarray* align align*)))
  (and (== t (tree-innermost '(eqnarray eqnarray* align align*)))
    (if (table-search-number-equation) #t #f)
  ) ;and
) ;tm-define

(tm-define (table-number-equation)
  (let* ((row (table-which-row)) (st (table-cell-tree row -1)))
    (tree-go-to st :end)
    (insert-go-to '(eq-number) '(0))
  ) ;let*
) ;tm-define

(tm-define (table-nonumber-equation)
  (and-with r (table-search-number-equation) (tree-cut r))
) ;tm-define

(define (table-inside-sub? t1 t2)
  (or (== t1 t2)
    (and (tree-in? t2 '(tformat document)) (table-inside-sub? t1 (tree-up t2)))
  ) ;or
) ;define

(tm-define (table-inside? which)
  (let* ((t1 (tree-innermost which)) (t2 (tree-innermost 'table)))
    (and t1 t2 (tree-inside? t2 t1) (table-inside-sub? t1 (tree-up t2)))
  ) ;let*
) ;tm-define

(tm-define (numbered-toggle t)
  (:require (tree-in? t '(eqnarray eqnarray* align align*)))
  (when (== t (tree-innermost '(eqnarray eqnarray* align align*)))
    (if (table-search-number-equation)
      (table-nonumber-equation)
      (table-number-equation)
    ) ;if
  ) ;when
) ;tm-define
