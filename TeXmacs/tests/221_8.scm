;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 221_8.scm
;; DESCRIPTION : Unit tests for toggle-bold selection target resolution
;; COPYRIGHT   : (C) 2026 Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic format-edit))

(import (liii check))

(check-set-mode! 'report-failed)

(tm-define (test_221_8)
  (let* ((bold-tree (tm->tree '(with "font-series" "bold" "你好")))
         (bold-body (tree-ref bold-tree :last))
         (italic-tree (tm->tree '(with "font-shape" "italic" "你好")))
         (italic-body (tree-ref italic-tree :last))
         (partial-bold (tm->tree '(with "font-series" "bold" "你好，世界")))
         (partial-italic (tm->tree '(with "font-shape" "italic" "你好，世界")))
         (paragraphs-mixed
          (tm->tree '(document
                       (with "font-series" "bold" "第一段")
                       "第二段"
                       (with "font-series" "bold" "第三段"))))
         (paragraphs-bold
          (tm->tree '(document
                       (with "font-series" "bold" "第一段")
                       (with "font-series" "bold" "第二段")
                       (with "font-series" "bold" "第三段"))))
         (inline-mixed
          (tm->tree '(concat
                       "前"
                       (with "font-series" "bold" "中")
                       "后")))
         (inline-bold
          (tm->tree '(with "font-series" "bold"
                       (concat "前" "中" "后")))))
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
           => #t)
    (check (tm-equal?
            (with-like-partial-toggle-result partial-bold "font-series" 6 15)
            '(concat (with "font-series" "bold" "你好") "，世界"))
           => #t)
    (check (tm-equal?
            (with-like-partial-toggle-result partial-italic "font-shape" 6 15)
            '(concat (with "font-shape" "italic" "你好") "，世界"))
           => #t)
    (check (tm-equal?
            (with-like-uniform-toggle-result
             paragraphs-mixed '(with "font-series" "bold" ""))
            '(document
               (with "font-series" "bold" "第一段")
               (with "font-series" "bold" "第二段")
               (with "font-series" "bold" "第三段")))
           => #t)
    (check (tm-equal?
            (with-like-uniform-toggle-result
             paragraphs-bold '(with "font-series" "bold" ""))
            '(document "第一段" "第二段" "第三段"))
           => #t)
    (check (tm-equal?
            (with-like-uniform-toggle-result
             inline-mixed '(with "font-series" "bold" ""))
            '(with "font-series" "bold"
               (concat "前" "中" "后")))
           => #t)
    (check (tm-equal?
            (with-like-node-without
             inline-bold
             '(with "font-series" "bold" ""))
            '(concat "前" "中" "后"))
           => #t))
  (check-report))
