;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 222_76.scm
;; DESCRIPTION : Unit tests for toggle-bold selection target resolution
;; COPYRIGHT   : (C) 2026 Mogan
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic format-edit))

(import (liii check))

(check-set-mode! 'report-failed)

(tm-define (test_222_76)
  (let* ((bold-tree (tm->tree '(with "font-series" "bold" "你好")))
         (bold-body (tree-ref bold-tree :last))
         (italic-tree (tm->tree '(with "font-shape" "italic" "你好")))
         (italic-body (tree-ref italic-tree :last)))
    (check (== (with-like-selection-target bold-tree '(with "font-series" "bold" ""))
               bold-tree)
           => #t)
    (check (== (with-like-selection-parent-target
                bold-body bold-tree '(with "font-series" "bold" ""))
               bold-tree)
           => #t)
    (check (with-like-selection-parent-target
            italic-body italic-tree '(with "font-series" "bold" ""))
           => #f)
    (check (== (with-like-selection-target italic-body '(with "font-series" "bold" ""))
               italic-body)
           => #t)
    (check (== (with-like-selection-parent-target
                italic-body italic-tree '(with "font-shape" "italic" ""))
               italic-tree)
           => #t))
  (check-report))
