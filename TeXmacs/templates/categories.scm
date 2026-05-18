
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
  '(((categoryKey . "university-thesis")
     (name . "高校论文")
     (nameEn . "University Thesis")
     (description . "各高校学位论文模板")
     (order . 1)
     (templateCount . 15))

    ((categoryKey . "lab-report")
     (name . "实验报告")
     (nameEn . "Lab Report")
     (description . "各类实验报告模板")
     (order . 2)
     (templateCount . 10))

    ((categoryKey . "math-modeling")
     (name . "数学建模")
     (nameEn . "Math Modeling")
     (description . "数学建模竞赛论文模板")
     (order . 3)
     (templateCount . 8))))

(tm-define (template-get-category-name category-id)
  (:synopsis "Get the display name for a category")
  (let ((cat (list-find template-default-categories
                        (lambda (c)
                          (equal? (assoc-ref c 'categoryKey) category-id)))))
    (if cat
        (assoc-ref cat 'name)
        category-id)))

(tm-define (template-get-categories)
  (:synopsis "Get list of all template categories, sorted by order")
  (sort template-default-categories
        (lambda (a b)
          (< (assoc-ref a 'order)
             (assoc-ref b 'order)))))
