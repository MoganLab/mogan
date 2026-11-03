;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-test.scm
;; DESCRIPTION : Test suite for generic
;; COPYRIGHT   : (C) 2022  Yufeng Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic test generic-test)
  (:use (generic generic-menu) (table table-menu) (generic generic-edit))
) ;texmacs-module

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for focus-tag-name
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-focus-tag-name)
  (check (focus-tag-name 'bmatrix) => "bmatrix")
  (check (focus-tag-name 'Bmatrix) => "Bmatrix")
  (check (focus-tag-name 'tabular) => "tabular")
  (check (focus-tag-name 'tabular*) => "centered tabular")
  (check (focus-tag-name 'block) => "block")
  (check (focus-tag-name 'block*) => "centered block")
  (check (focus-tag-name 'big-table) => "big table")
) ;define

(define (test-structured-horizontal)
  (let* ((tbl
           (tm->tree
             '(three-line-table (tformat (table (row (cell "a")))))
           ) ;tm->tree
         ) ;tbl
         (plain-tbl
           (tm->tree '(table (row (cell "a"))))
         ) ;plain-tbl
         (list-item
           (tm->tree '(itemize (document (item) "hello")))
         ) ;list-item
        ) ;
    (check (table-markup-context? tbl) => #t)
    (check (table-markup-context? plain-tbl) => #t)
    (check (structured-horizontal? tbl) => #t)
    (check (structured-vertical? tbl) => #t)
    (check (table-markup-context? list-item) => #f)
    (check (structured-horizontal? list-item) => #f)
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Test entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (regtest-generic)
  (test-focus-tag-name)
  (test-structured-horizontal)
  (check-report)
) ;tm-define
