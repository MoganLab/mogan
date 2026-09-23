;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : semantic-popup.scm
;; DESCRIPTION : Specifications and action binding for QTMSemanticPopup
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic semantic-popup)
  (:use (kernel texmacs tm-define)
        (kernel library base)
        (kernel library content)
        (kernel library tree)
        (math math-edit)
        (generic document-edit)))

(define (semantic-normalize-tag tag)
  (if (string? tag) (string->symbol tag) tag))

(tm-define (semantic-supported-tag? raw-tag)
  (let ((tag (semantic-normalize-tag raw-tag)))
    (in? tag '(equation equation* table-of-contents table-of-contents*))))

(tm-define (semantic-popup-actions raw-tag t)
  (let ((tag (semantic-normalize-tag raw-tag)))
    (cond
      ((== tag 'equation*)
       '(actions
          (action "copy-latex" "Copy LaTeX" "tm_copy")
          (action "toggle-number" "Add Number" "tm_numbered")))
      ((== tag 'equation)
       '(actions
          (action "copy-latex" "Copy LaTeX" "tm_copy")
          (action "toggle-number" "Hide Number" "tm_numbered")))
      ((in? tag '(table-of-contents table-of-contents*))
       '(actions
          (action "refresh-toc" "Refresh TOC" "tm_reload")))
      (else '(actions)))))

(tm-define (semantic-popup-action-trigger raw-tag action-id t)
  (let ((tag (semantic-normalize-tag raw-tag)))
    (cond
      ((== action-id "copy-latex")
       (semantic-copy-latex t))
      ((== action-id "toggle-number")
       (semantic-toggle-equation-number t))
      ((== action-id "refresh-toc")
       (update-document "table-of-contents"))
      (else #f))))
