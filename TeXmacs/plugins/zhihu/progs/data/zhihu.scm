;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : zhihu.scm
;; DESCRIPTION : zhihu data format (export snippet)
;; COPYRIGHT   : (C) 2026  Liii Network
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data zhihu)
  (:use (kernel gui menu-widget)
        (utils edit selections)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Zhihu format definition
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-format zhihu
  (:name "知乎")
  (:suffix "zhihu"))

(define (tree->document-stree t)
  (let ((st (tree->stree t)))
    (if (and (pair? st) (eq? (car st) 'document))
        st
        `(document ,st))))

(define (zhihu-tip-tree)
  (let* ((latex-code (string-load (unix->url "$TEXMACS_PATH/plugins/account/data/copy_to_zhihu.tex")))
         (parsed-latex (parse-latex latex-code)))
    (latex->texmacs parsed-latex)))

(define (zhihu-tree-with-tip t)
  (let* ((content-st (tree->document-stree t))
         (tip-st (tree->document-stree (zhihu-tip-tree)))
         (merged-st (cons 'document
                          (append (cdr content-st)
                                  (list "")
                                  (cdr tip-st)))))
    (stree->tree merged-st)))

(tm-define (texmacs-tree->zhihu-snippet t)
  (:synopsis "Export TeXmacs snippet as plain text and append zhihu tip")
  (texmacs->generic (zhihu-tree-with-tip t) "verbatim-snippet"))

(converter texmacs-tree zhihu-snippet
  (:function texmacs-tree->zhihu-snippet))

;; Keep "Copy to -> Zhihu" as structured clipboard data (like Ctrl+C),
;; while appending the zhihu tip content at the end of the copied tree.
(tm-define (clipboard-copy-export format which)
  (:require (== format "zhihu"))
  (let ((temp (clipboard-get-export)))
    (clipboard-set-export "default")
    (clipboard-set which (zhihu-tree-with-tip (selection-tree)))
    (clipboard-set-export temp)))
