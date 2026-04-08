
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
  '(((id . "university-thesis")
     (name . "University Thesis")
     (icon . "🎓")
     (order . 1))

    ((id . "lab-report")
     (name . "Lab Report")
     (icon . "📊")
     (order . 2))

    ((id . "math-modeling")
     (name . "Math Modeling")
     (icon . "🧪")
     (order . 3))))

(tm-define (template-get-category-name category-id)
  (:synopsis "Get the display name for a category")
  (let ((cat (list-find template-default-categories
                        (lambda (c) (equal? (assoc-ref c 'id) category-id)))))
    (if cat
        (assoc-ref cat 'name)
        category-id)))

(tm-define (template-get-categories)
  (:synopsis "Get list of all template categories, sorted by order")
  (sort template-default-categories
        (lambda (a b)
          (< (assoc-ref a 'order)
             (assoc-ref b 'order)))))
