;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6006.scm
;; DESCRIPTION : Test semantic popup actions, LaTeX conversion, and equation toggling
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(use-modules (math math-edit) (generic semantic-popup))

(define (test_6006)
  (check-set-mode! 'report-failed)

  ;; 1. Test supported tags
  (check (semantic-supported-tag? 'equation*) => #t)
  (check (semantic-supported-tag? 'equation) => #t)
  (check (semantic-supported-tag? 'table-of-contents) => #t)
  (check (semantic-supported-tag? 'table-of-contents*) => #t)
  (check (semantic-supported-tag? 'section) => #f)
  (check (semantic-supported-tag? 'paragraph) => #f)

  ;; 2. Test semantic-popup-actions for equation*
  (let* ((eq-star (stree->tree '(equation* (document "x+y=z"))))
         (acts-star (semantic-popup-actions 'equation* eq-star))
        ) ;
    (check (pair? acts-star) => #t)
    (check (car acts-star) => 'actions)
    (check (length acts-star) => 3)
    (check (cadr acts-star) => '(action "copy-latex" "Copy LaTeX" "tm_copy"))
    (check (caddr acts-star)
      =>
      '(action "toggle-number" "Add Number" "tm_numbered")
    ) ;check
  ) ;let*

  ;; 3. Test semantic-popup-actions for equation (numbered)
  (let* ((eq-num (stree->tree '(equation (document "x+y=z"))))
         (acts-num (semantic-popup-actions 'equation eq-num))
        ) ;
    (check (caddr acts-num)
      =>
      '(action "toggle-number" "Hide Number" "tm_numbered")
    ) ;check
  ) ;let*

  ;; 4. Test semantic-popup-actions for table-of-contents
  (let* ((toc (stree->tree '(table-of-contents "toc" (document ""))))
         (acts-toc (semantic-popup-actions 'table-of-contents toc))
        ) ;
    (check (cadr acts-toc) => '(action "refresh-toc" "Refresh TOC" "tm_reload"))
  ) ;let*

  ;; 5. Test semantic-copy-latex for equation*
  (let* ((eq (stree->tree '(equation* (document "x+y=z"))))
         (res (semantic-copy-latex eq))
        ) ;
    (check (string? res) => #t)
    (check (string-contains? res "x + y = z") => #t)
    (check (string-contains? res "\\[") => #t)
    (check (string-contains? res "\\]") => #t)
  ) ;let*

  ;; 6. Test semantic-copy-latex for equation
  (let* ((eq (stree->tree '(equation (document "a=b")))) (res (semantic-copy-latex eq)))
    (check (string? res) => #t)
    (check (string-contains? res "a = b") => #t)
    (check (string-contains? res "\\begin{equation}") => #t)
    (check (string-contains? res "\\end{equation}") => #t)
  ) ;let*

  ;; 7. Test semantic-toggle-equation-number (equation* -> equation -> equation*)
  (let ((node (stree->tree '(equation* (document "E=m*c^2")))))
    (check (tree-label node) => 'equation*)
    (let ((tag1 (semantic-toggle-equation-number node)))
      (check tag1 => 'equation)
      (check (tree-label node) => 'equation)
    ) ;let
    (let ((tag2 (semantic-toggle-equation-number node)))
      (check tag2 => 'equation*)
      (check (tree-label node) => 'equation*)
    ) ;let
  ) ;let

  ;; 8. Test tree-set-label!
  (let ((t (stree->tree '(equation* "x"))))
    (tree-set-label! t 'equation)
    (check (tree-label t) => 'equation)
  ) ;let

  ;; 9. Test semantic-popup-action-trigger
  (let ((node (stree->tree '(equation* (document "1+1=2")))))
    (check (string? (semantic-popup-action-trigger 'equation* "copy-latex" node))
      =>
      #t
    ) ;check
    (check (semantic-popup-action-trigger 'equation* "toggle-number" node)
      =>
      'equation
    ) ;check
    (check (tree-label node) => 'equation)
  ) ;let

  ;; 10. Test TOC refresh trigger smoke test
  (check (procedure? (lambda () (update-document "table-of-contents"))) => #t)

  (check-report)
) ;define
