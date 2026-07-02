;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0195.scm
;; DESCRIPTION : Test that (document ...) inside concat is simplified away
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report)

(tm-define (test_0195)
  ;; (concat (document "text")) should simplify to just "text"
  (let* ((input '(concat (document "hello world")))
         (result (tree->stree (tree-simplify (stree->tree input))))
        ) ;
    (check result => "hello world")
  ) ;let*

  ;; (concat (item) (document "text")) should simplify to (concat (item) "text")
  (let* ((input '(concat (item) (document "hello world")))
         (result (tree->stree (tree-simplify (stree->tree input))))
        ) ;
    (check result => '(concat (item) "hello world"))
  ) ;let*

  ;; (document (concat (item) (document "text"))) should simplify nested document
  (let* ((input '(document (concat (item) (document "hello world"))))
         (result (tree->stree (tree-simplify (stree->tree input))))
        ) ;
    (check result => '(document (concat (item) "hello world")))
  ) ;let*

  ;; (concat (document "a") (document "b")) - two single-child documents
  ;; Each document is unwrapped, then adjacent text is merged: "ab"
  (let* ((input '(concat (document "a") (document "b")))
         (result (tree->stree (tree-simplify (stree->tree input))))
        ) ;
    (check result => "ab")
  ) ;let*

  ;; (concat (document "a" "b")) - multi-child document should NOT be unwrapped
  ;; simplify_concat on the document returns (document "a" "b"), then
  ;; N(r)==1 so it becomes just the single child (document "a" "b")
  (let* ((input '(concat (document "a" "b")))
         (result (tree->stree (tree-simplify (stree->tree input))))
        ) ;
    (check result => '(document "a" "b"))
  ) ;let*

  (check-report)
) ;tm-define
