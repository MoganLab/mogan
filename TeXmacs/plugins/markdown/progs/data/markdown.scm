;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : markdown.scm
;; DESCRIPTION : markdown data format (export snippet)
;; COPYRIGHT   : (C) 2026  Liii Network
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data markdown)
  (:use (kernel gui menu-widget)
        (utils edit selections)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Markdown format definition
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format markdown
  (:name "Markdown")
  (:suffix "md" "markdown"))

(define (tree->document-stree t)
  (let ((st (tree->stree t)))
    (if (and (pair? st) (eq? (car st) 'document))
        st
        `(document ,st))))

(define (markdown-tip-tree)
  (let* ((latex-code (string-load (unix->url "$TEXMACS_PATH/plugins/account/data/copy_to_md.tex")))
         (parsed-latex (parse-latex latex-code)))
    (latex->texmacs parsed-latex)))

(define (markdown-tree-with-tip t)
  (let* ((content-st (tree->document-stree t))
         (tip-st (tree->document-stree (markdown-tip-tree)))
         (merged-st (cons 'document
                          (append (cdr content-st)
                                  (list "")
                                  (cdr tip-st)))))
    (stree->tree merged-st)))

(define (texmacs-tree->markdown-snippet-with-tip t)
  (texmacs->generic (markdown-tree-with-tip t) "verbatim-snippet"))

(tm-define (texmacs-tree->markdown-snippet t)
  (:synopsis "Export TeXmacs snippet as plain text and append markdown tip")
  (texmacs-tree->markdown-snippet-with-tip t))

(converter texmacs-tree markdown-snippet
  (:function texmacs-tree->markdown-snippet))

;; Keep "Copy to -> Markdown" as structured clipboard data (like Ctrl+C),
;; while appending the markdown tip content at the end of the copied tree.
(tm-define (clipboard-copy-export format which)
  (:require (== format "markdown"))
  (let ((temp (clipboard-get-export)))
    (clipboard-set-export "default")
    (clipboard-set which (markdown-tree-with-tip (selection-tree)))
    (clipboard-set-export temp)))
