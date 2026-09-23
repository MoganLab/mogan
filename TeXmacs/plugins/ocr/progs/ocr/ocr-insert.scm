;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ocr-insert.scm
;; DESCRIPTION : OCR 识别结果的编辑器插入遍历（insert/kbd-return/cursor）
;; COPYRIGHT   : (C) 2026  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ocr ocr-insert))

;; 编辑器交互符号（insert/kbd-return/tree-go-to/get-env）属 GPL 层，插入
;; 遍历自 goldfish 的 (liii ocr-impl) 上移至此；goldfish 编排层经 inserter
;; 参数回调本模块导出的插入函数。
;;
;; 调用方 (ocr liii-ocr) 的 :use 在其自身 *load-path* 设置之前加载本模块，
;; 故本模块自理 goldfish 库路径（幂等）。
(let ((sys-path (string-append (getenv "TEXMACS_PATH") "/plugins/ocr/goldfish")))
  (unless (member sys-path *load-path*)
    (set! *load-path* (cons sys-path *load-path*))
  ) ;unless
) ;let

(import (liii json))
(import (liii string))
(import (liii either))
(import (liii ocr-layout))
(import (liii ocr-title))
(import (liii ocr-latex))
(import (liii ocr-render))
(import (liii ocr-impl))

(define (ocr-insert-latex-by-cursor latex-code)
  (let* ((parsed-latex (parse-latex (ocr-latex-preprocess latex-code)))
         (texmacs-latex (latex->texmacs parsed-latex))
        ) ;
    (insert texmacs-latex)
  ) ;let*
) ;define

(define (ocr-insert-markdown-by-cursor markdown-code)
  (let* ((tree (markdown-snippet->tree (preprocess-math-in-markdown markdown-code))))
    (insert tree)
  ) ;let*
) ;define

(define (ocr-insert-html-by-cursor html-code . maybe-table-note)
  (insert (apply html-block->texmacs-tree html-code maybe-table-note))
  (kbd-return)
) ;define

(define (ocr-insert-text-by-cursor text)
  (ocr-insert-markdown-by-cursor text)
  (kbd-return)
) ;define

(define (ocr-insert-title-by-cursor title)
  (let* ((parsed (parse-title-prefix title))
         (kind (car parsed))
         (level (cadr parsed))
         (text (caddr parsed))
         (level->tag (lambda (l)
                       (cond ((= l 1) "section")
                             ((= l 2) "subsection")
                             (else "subsubsection")
                       ) ;cond
                     ) ;lambda
         ) ;level->tag
        ) ;
    (cond ((eq? kind 'plain)
           (ocr-insert-latex-by-cursor (string-append "\\section*{" text "}"))
          ) ;
          ((eq? kind 'numbered)
           (ocr-insert-latex-by-cursor (string-append "\\" (level->tag level) "{" text "}")
           ) ;ocr-insert-latex-by-cursor
          ) ;
          ((eq? kind 'appendix)
           (if (= level 1)
             (let* ((parsed-latex (parse-latex (ocr-latex-preprocess text)))
                    (texmacs-latex (latex->texmacs parsed-latex))
                   ) ;
               (insert `(appendix ,texmacs-latex))
             ) ;let*
             (ocr-insert-latex-by-cursor (string-append "\\" (level->tag level) "{" text "}")
             ) ;ocr-insert-latex-by-cursor
           ) ;if
          ) ;
    ) ;cond
  ) ;let*
  (kbd-return)
) ;define

(define (ocr-render-layout-node-by-cursor node)
  (let ((tag (layout-node-tag node))
        (attrs (layout-node-attrs node))
        (children (layout-node-children node))
       ) ;
    (cond ((or (eq? tag 'paragraph) (eq? tag 'section-heading))
           (ocr-insert-text-by-cursor (if (pair? children) (car children) ""))
          ) ;
          ((eq? tag 'title)
           (ocr-insert-title-by-cursor (if (pair? children) (car children) ""))
          ) ;
          ((eq? tag 'equation)
           (insert (equation->texmacs-tree (if (pair? children) (car children) "")))
           (kbd-return)
          ) ;
          ((eq? tag 'figure)
           (insert (image-content->texmacs-tree (layout-attr-ref attrs 'content "")
                     (layout-attr-ref attrs 'width #f)
                     (layout-attr-ref attrs 'height #f)
                     (layout-attr-ref attrs 'caption "")
                   ) ;image-content->texmacs-tree
           ) ;insert
           (kbd-return)
          ) ;
          ((eq? tag 'table)
           (insert (html-block->texmacs-tree (layout-attr-ref attrs 'html "")
                     (layout-attr-ref attrs 'note "")
                   ) ;html-block->texmacs-tree
           ) ;insert
           (kbd-return)
          ) ;
          ((eq? tag 'ordered-group)
           (insert (layout-group->texmacs-tree node))
           (kbd-return)
          ) ;
          (else #f)
    ) ;cond
  ) ;let
) ;define

(define (ocr-render-layout-document-by-cursor layout)
  (for-each ocr-render-layout-node-by-cursor (layout-node-children layout))
) ;define

(define (ocr-insert-list-by-cursor array-json)
  (if (vector? array-json)
    (ocr-render-layout-document-by-cursor (ocr-blocks->layout array-json))
    (notify-user (from-left "识别异常，请联系客服！"))
  ) ;if
) ;define

(tm-define (ocr-insert-by-cursor j)
  (let ((format (json-ref-string j "format" "latex")) (text (json-ref j "text")))
    (cond ((string=? format "list")
           (if (vector? text)
             (ocr-insert-list-by-cursor text)
             (notify-user (from-left "识别异常，请联系客服！"))
           ) ;if
          ) ;
          ((string=? format "latex")
           (if (string? text)
             (if (string-null? text)
               (notify-user (from-left "识别结果为空，请联系客服！"))
               (ocr-insert-latex-by-cursor text)
             ) ;if
             (notify-user (from-left "识别异常，请联系客服！"))
           ) ;if
          ) ;
          ((string=? format "markdown")
           (if (string? text)
             (if (string-null? text)
               (notify-user (from-left "识别结果为空，请联系客服！"))
               (ocr-insert-markdown-by-cursor text)
             ) ;if
             (notify-user (from-left "识别异常，请联系客服！"))
           ) ;if
          ) ;
          ((string=? format "html")
           (if (string? text)
             (if (string-null? text)
               (notify-user (from-left "识别结果为空，请联系客服！"))
               (ocr-insert-html-by-cursor text)
             ) ;if
             (notify-user (from-left "识别异常，请联系客服！"))
           ) ;if
          ) ;
          (else #t)
    ) ;cond
  ) ;let
) ;tm-define

(tm-define (ocr-cursor-mode)
  (let ((mode (get-env "mode")))
    (if (string=? mode "math") "math" "text")
  ) ;let
) ;tm-define

(tm-define (ocr-insert-by-image t j)
  (tree-go-to t :end)
  (kbd-return)
  (ocr-insert-by-cursor j)
) ;tm-define
