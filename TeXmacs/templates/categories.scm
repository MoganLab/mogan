
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : categories.scm
;; DESCRIPTION : Template categories for Liii STEM/Mogan Template Center
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (templates categories))

(tm-define template-default-categories
  '((id "thesis"
        name "Thesis"
        icon "template-thesis"
        order 1)

    (id "lab-report"
        name "Lab Report"
        icon "template-lab"
        order 2)

    (id "math-modeling"
        name "Math Modeling"
        icon "template-math"
        order 3)))

(tm-define (template-get-category-name category-id)
  (:synopsis "Get the display name for a category")
  (let ((cat (assoc category-id template-default-categories)))
    (if cat
        (assoc-ref cat 'name)
        category-id)))

(tm-define (template-get-categories)
  (:synopsis "Get list of all template categories, sorted by order")
  (sort template-default-categories
        (lambda (a b)
          (< (assoc-ref a 'order)
             (assoc-ref b 'order)))))
