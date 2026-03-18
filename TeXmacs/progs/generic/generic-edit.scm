
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-edit.scm
;; DESCRIPTION : Generic editing routines
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic generic-edit)
  (:use (utils library tree)
	(utils library cursor)
	(utils edit variants)
        (utils misc tooltip)
        (bibtex bib-complete)
	(source macro-search)))

(tm-define (generic-context? t) #t) ;; overridden in, e.g., graphics mode

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic cursor movements via the keyboard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-horizontal t forwards?)
  (and-with p (tree-outer t)
    (kbd-horizontal p forwards?)))

(tm-define (kbd-vertical t downwards?)
  (and-with p (tree-outer t)
    (kbd-vertical p downwards?)))

(tm-define (kbd-extremal t forwards?)
  (and-with p (tree-outer t)
    (kbd-extremal p forwards?)))

(tm-define (kbd-incremental t downwards?)
  (and-with p (tree-outer t)
    (kbd-incremental p downwards?)))

(tm-define (kbd-horizontal t forwards?)
  (:require (tree-is-buffer? t))
  (with move (lambda () (if forwards? (go-right) (go-left)))
    (go-to-next-such-that move generic-context?)))

(tm-define (kbd-vertical t downwards?)
  (:require (tree-is-buffer? t))
  (with move (lambda () (if downwards? (go-down) (go-up)))
    (go-to-next-such-that move generic-context?)))

(tm-define (kbd-extremal t forwards?)
  (:require (tree-is-buffer? t))
  (with move (lambda () (if forwards? (go-end-line) (go-start-line)))
    (go-to-next-such-that move generic-context?)))

(tm-define (kbd-incremental t downwards?)
  (:require (tree-is-buffer? t))
  (with move (lambda () (if downwards? (go-page-down) (go-page-up)))
    (go-to-next-such-that move generic-context?)))

(tm-define (kbd-left)
  (kbd-horizontal (focus-tree) #f))
(tm-define (kbd-right)
  (kbd-horizontal (focus-tree) #t))
(tm-define (kbd-up)
  (kbd-vertical (focus-tree) #f))
(tm-define (kbd-down)
  (kbd-vertical (focus-tree) #t))
(tm-define (kbd-start-line)
  (kbd-extremal (focus-tree) #f))
(tm-define (kbd-end-line)
  (kbd-extremal (focus-tree) #t))
(tm-define (kbd-page-up)
  (kbd-incremental (focus-tree) #f))
(tm-define (kbd-page-down)
  (kbd-incremental (focus-tree) #t))

(tm-define (kbd-select r)
  (select-from-shift-keyboard)
  (r)
  (select-from-cursor))

(tm-define (kbd-select-if-active r)
  (r)
  (select-from-cursor-if-active))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic editing via the keyboard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (insert-return) (insert-raw-return))

(tm-define (kbd-space-bar t shift?)
  (and-with p (tree-outer t)
    (kbd-space-bar p shift?)))

(tm-define (kbd-enter t shift?)
  (and-with p (tree-outer t)
    (kbd-enter p shift?)))

(tm-define (kbd-control-enter t shift?)
  (and-with p (tree-outer t)
    (kbd-control-enter p shift?)))

(tm-define (kbd-alternate-enter t shift?)
  (and-with p (tree-outer t)
    (kbd-alternate-enter p shift?)))

(tm-define (kbd-remove t forwards?)
  (and-with p (tree-outer t)
    (kbd-remove p forwards?)))

(tm-define (kbd-variant t forwards?)
  (and-with p (tree-outer t)
    (kbd-variant p forwards?)))

(tm-define (kbd-space-bar t shift?)
  (:require (tree-is-buffer? t))
  (insert " "))

(tm-define (kbd-enter t shift?)
  (:require (tree-is-buffer? t))
  (insert-return))

(tm-define (kbd-control-enter t shift?)
  (:require (tree-is-buffer? t))
  (noop))

(tm-define (kbd-alternate-enter t shift?)
  (:require (tree-is-buffer? t))
  (noop))

(tm-define (kbd-remove t forwards?)
  (:require (tree-is-buffer? t))
  (remove-text forwards?))

(tm-define (kbd-remove t forwards?)
  (:require (and (in-source?)
                 (not (in-source-mode?))  ;; 不在源码编辑或者导言区编辑模式
                 (not (with-any-selection?))))
  (remove-text forwards?)
  (source-complete-try))

(tm-define (kbd-remove t forwards?)
  (:require (and (tree-is-buffer? t) (with-any-selection?)))
  (clipboard-cut "nowhere")
  (clipboard-clear "nowhere"))

(tm-define (kbd-remove t forwards?)
  (:mode complete-mode?)
  (remove-text forwards?)
  (kbd-variant (focus-tree) #t))

(tm-define (kbd-remove t forwards?)
  (:require (at-image-start?))
  (let ((image (any-image-context?)))
    (tree-cut image)))

(tm-define (kbd-variant t forwards?)
  (:require (tree-is-buffer? t))
  (if (and (not (complete-try?)) forwards?)
      (with sh (kbd-system-rewrite (kbd-find-inv-binding '(kbd-alternate-tab)))
        (set-message `(concat "Use " ,sh " in order to insert a tab")
                     "tab"))))

;; 辅助函数：定义 enumerate-tag-list
(define (enumerate-tag-list)
  '(enumerate enumerate-1 enumerate-2 enumerate-3 enumerate-4))

;; 辅助函数：定义 itemize-tag-list
(define (itemize-tag-list)
  '(itemize itemize-dot itemize-minus itemize-arrow))

;; 辅助函数：定义 description-tag-list
(define (description-tag-list)
  '(description description-compact description-aligned description-dash description-long description-paragraphs))

;; 辅助函数：检查是否在有序列表环境中
(define (in-enumerate-context?)
  (not (not (tree-search-upwards (focus-tree) (lambda (node) (tree-in? node (enumerate-tag-list)))))))

;; 辅助函数：检查是否在无序列表环境中
(define (in-itemize-context?)
  (not (not (tree-search-upwards (focus-tree) (lambda (node) (tree-in? node (itemize-tag-list)))))))

;; 辅助函数：检查是否在描述列表环境中
(define (in-description-context?)
  (not (not (tree-search-upwards (focus-tree) (lambda (node) (tree-in? node (description-tag-list)))))))

;; 辅助函数：获取当前列表的类型
(define (get-list-type)
  (cond
    ((in-description-context?) 'description)
    ((in-itemize-context?) 'itemize)
    ((in-enumerate-context?) 'enumerate)
    (else #f)))

;; 辅助函数：查找包含 item 的 concat 包装和真正的 item list
(define (find-item-wrapper-and-list item)
  (let ((wrapper #f)
        (item-list #f))
    (let loop ((current (tree-outer item)))
      (if (tree-is? current 'concat)
          (begin
            (set! wrapper current)
            (loop (tree-outer current)))
          (set! item-list current)))
    (values wrapper item-list)))

;; 辅助函数：提取 item 内容（处理 concat 包装）
(define (extract-item-content wrapper)
  (if (and wrapper (> (tree-arity wrapper) 1))
      (tree-copy (tree-ref wrapper 1))
      #f))

;; 辅助函数：在列表中移除 item（处理 concat 包装）
(define (remove-item-from-list item wrapper item-list)
  (if wrapper
      ;; 如果有 wrapper，移除整个 wrapper
      (let ((wrapper-index (tree-index wrapper)))
        (tree-remove! item-list wrapper-index 1))
      ;; 否则移除单个 item
      (let ((item-index (tree-index item)))
        (tree-remove! item-list item-index 1))))

;; 在有序和无序列表中实现缩进功能
(tm-define (kbd-variant t forwards?)
  (:require 
    (or
      ;; 有序列表或者无序列表
      (and (or in-enumerate-context? in-itemize-context?) (tree-is? (focus-tree) 'item))
      ;; 描述列表
      (and in-description-context? (tree-is? (focus-tree) 'item*))))

  (let ((item (focus-tree)))
    ;; 步骤 1: 查找包装和列表
    (call-with-values (lambda () (find-item-wrapper-and-list item))
      (lambda (wrapper item-list)
        (if (and item item-list)
            (let ((item-index (if wrapper (tree-index wrapper) (tree-index item))))
              (if (> item-index 0)
                  (let ((prev-item (tree-ref item-list (- item-index 1))))
                    ;; 步骤 2: 提取内容
                    (let ((item-content (extract-item-content wrapper)))
                      ;; 步骤 3: 创建子列表并移动内容
                      (tree-go-to prev-item :end)
                      (insert-return)
                      (let ((list-type (get-list-type)))
                        (if list-type
                            (make-tmlist list-type)
                            (make-tmlist 'enumerate))) ; 默认使用有序列表
                      ;; 步骤4：拷贝内容
                      (if item-content
                          (let ((new-item (focus-tree))
                                (list-type (get-list-type)))
                            
                            (let ((content-stree (tree->stree item-content)))
                              (if (eq? list-type 'description)
                                  (tree-set! new-item `(concat (item*), content-stree))
                                  (tree-set! new-item `(concat (item), content-stree)))
                              (tree-go-to new-item))))
                      ;; 步骤 5: 从原列表中移除
                      (remove-item-from-list item wrapper item-list))))))))))

(tm-define (kbd-variant t forwards?)
  (:require (and (tree-in? t '(label reference pageref eqref smart-ref))
                 (cursor-inside? t)))
  (if (complete-try?) (noop)))

(tm-define (bib-cite-context? t)
  (and (tree-in? t '(cite nocite cite-detail cite-TeXmacs))
       (cursor-inside? t)))

(tm-define (kbd-variant t forwards?)
  (:require (and (not (supports-db?)) (bib-cite-context? t)))
  (with u (current-bib-file #t)
    (with ttxt (tree-ref t (cADr (cursor-path)))
      (if (or (url-none? u) (not ttxt))
          (set-message "No completions" "You must add a bibliography file")
          (custom-complete (tm->tree (citekey-completions u ttxt)))))))

(tm-define (kbd-alternate-variant t forwards?)
  (and-with p (tree-outer t)
    (kbd-alternate-variant p forwards?)))

(tm-define (kbd-alternate-variant t forwards?)
  (:require (tree-is-buffer? t))
  (make-htab "5mm"))

(tm-define (kbd-space)
  (kbd-space-bar (focus-tree) #f))
(tm-define (kbd-shift-space)
  (kbd-space-bar (focus-tree) #t))
(tm-define (kbd-return)
  (kbd-enter (focus-tree) #f))
(tm-define (kbd-shift-return)
  (kbd-enter (focus-tree) #t))
(tm-define (kbd-control-return)
  (kbd-control-enter (focus-tree) #f))
(tm-define (kbd-shift-control-return)
  (kbd-control-enter (focus-tree) #t))
(tm-define (kbd-alternate-return)
  (kbd-alternate-enter (focus-tree) #f))
(tm-define (kbd-shift-alternate-return)
  (kbd-alternate-enter (focus-tree) #t))
(tm-define (kbd-backspace)
  (kbd-remove (focus-tree) #f))
(tm-define (kbd-delete)
  (kbd-remove (focus-tree) #t))
(tm-define (kbd-tab)
  (kbd-variant (focus-tree) #t))
(tm-define (kbd-shift-tab)
  (kbd-variant (focus-tree) #f))
(tm-define (kbd-alternate-tab)
  (kbd-alternate-variant (focus-tree) #t))
(tm-define (kbd-shift-alternate-tab)
  (kbd-alternate-variant (focus-tree) #f))
(tm-define (kbd-copy)
  (clipboard-copy "primary"))
(tm-define (kbd-cut)
  (clipboard-cut "primary"))
(tm-define (kbd-paste)
  (clipboard-paste "primary"))
(tm-define (kbd-paste-verbatim)
  (clipboard-paste-import "verbatim" "primary"))
(tm-define (kbd-cancel)
  (clipboard-clear "primary"))

#|
ocr-paste
剪贴板中的内容是图像时，OCR并插入已识别的内容到当前光标处

语法
----
(ocr-paste)
|#
(tm-define (ocr-paste)
  (when (not (defined? 'ocr-to-latex-by-cursor))
    (use-modules (liii ocr)))
  (with data 
    (parse-texmacs-snippet (tree->string (tree-ref (clipboard-get "primary") 1)))
    (when (tree-is? (tree-ref data 0) 'image)
          (ocr-to-latex-by-cursor data))))

#|
image-and-ocr-paste
剪贴板中的内容是图像时，OCR并插入图像和已识别的内容到当前光标处。图像和已识别的内容通过回车键分隔。

语法
----
(image-and-ocr-paste)
|#
(tm-define (image-and-ocr-paste)
  (with data 
    (parse-texmacs-snippet (tree->string (tree-ref (clipboard-get "primary") 1)))
    (when (tree-is? (tree-ref data 0) 'image)
          (kbd-paste)
          (kbd-return)
          (when (not (defined? 'ocr-to-latex-by-cursor))
            (use-modules (liii ocr)))
          (ocr-to-latex-by-cursor data))))

(tm-define (paste-as-html)
  (with source-format (qt-clipboard-format)
    (if (string=? source-format "html")
        (let* ((fm (format-determine (qt-clipboard-text) "verbatim")))
          (cond ((string=? fm "html") (clipboard-paste-import "html" "primary"))
                ((string=? fm "latex") (clipboard-paste-import "latex" "primary"))  
                ((string=? fm "verbatim") (kbd-paste))
                ((string=? fm "markdown") (paste-as-markdown))))
        (clipboard-paste-import "html" "primary"))))

(tm-define (paste-as-markdown)
  (if (community-stem?)
      (begin
        (clipboard-paste-import "verbatim" "primary")
        (kbd-return)
        (let* ((latex-code (string-load (unix->url "$TEXMACS_PATH/plugins/account/data/md.tex")))
               (parsed-latex (parse-latex latex-code))
               (texmacs-latex (latex->texmacs parsed-latex)))
          (insert texmacs-latex)))
      (clipboard-paste-import "markdown" "primary")))

#|
paste-as-texmacs
期望以texmacs格式粘贴

语法
(paste-as-texmacs)
|#

(tm-define (paste-as-texmacs)
  (when (not (defined? 'ocr-to-latex-by-cursor))
    (use-modules (liii ocr)))
  (with img-tree (tree-ref (clipboard-get "primary") 1)
    (cond ((tree-is? img-tree 'image) 
           (ocr-to-latex-by-cursor img-tree))
          ((and (tree-is? img-tree 'with) (not (null? (tree-ref img-tree 2))))
           (let* ((sub-img-tree (tree-ref img-tree 2)))
             (when (tree-is? sub-img-tree 'image)
               (ocr-to-latex-by-cursor img-tree)))))))

#|
smart-format-paste
智能格式粘贴，对剪贴板格式进行检测，按照对应格式进行粘贴

语法
(smart-format-paste)
|#

(tm-define (smart-format-paste)
  (with source-format (qt-clipboard-format)
    (cond ((or (string=? source-format "verbatim") (string=? source-format "html"))
           (let* ((fm (format-determine (qt-clipboard-text) "verbatim")))
             (cond ((string=? fm "html") (clipboard-paste-import "html" "primary"))
                   ((string=? fm "latex") (clipboard-paste-import "latex" "primary"))  
                   ((string=? fm "verbatim") (kbd-paste))
                   ((string=? fm "markdown") (paste-as-markdown)))))
          ((string=? source-format "texmacs-snippet") (paste-as-texmacs))
          (else (kbd-paste-verbatim)))))

#|
kbd-magic-paste
智能粘贴。通过`Ctrl+Shift+v`或者`编辑->智能粘贴`触发，能够根据粘贴内容和当前模式，切换粘贴的方式。

语法
----
(kbd-magic-paste)

逻辑
----
如果剪贴板中的内容是图像，那么调用`(ocr-paste)`，先OCR然后粘贴。否则：
1. 代码模式：将剪贴板中的内容粘贴为代码块
2. 数学模式：将剪贴板中的内容作为LaTeX格式粘贴
3. 文本模式：将剪贴板中的内容作为纯文本粘贴

TODO: 在文本模式中，可以自动识别剪贴板中的内容，并智能粘贴。比如，内容格式经过识别，发现是LaTeX格式，
那么应该粘贴为LaTeX格式
|#
(tm-define (kbd-magic-paste)
  (if (string-starts? (qt-clipboard-format) "image")
      (ocr-paste)
      (with mode (get-env "mode")
        (cond ((== mode "prog")
               (clipboard-paste-import "code" "primary"))
              ((== mode "math")
               (clipboard-paste-import "latex" "primary"))
              (else (smart-format-paste))))))

(tm-define (any-image-context?)
  (tree-innermost 
   (lambda (t) (tree-is? t 'image)) 
   #t))

(tm-define (at-image-start?)
  (with image (any-image-context?)
    (and image
         (let* ((p (cursor-path))
                (ip (tree->path image)))
           (or (== p ip)
               (and (== (cDr p) ip)
                    (<= (cAr p) 1)))))))

(tm-define (notify-activated t) (noop))
(tm-define (notify-disactivated t) (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic gestures
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (swipe-horizontal t forward?)
  (and-with p (tree-outer t)
    (swipe-horizontal p forward?)))

(tm-define (swipe-vertical t down?)
  (and-with p (tree-outer t)
    (swipe-vertical p down?)))

(tm-define (swipe-left)
  (swipe-horizontal (focus-tree) #f))

(tm-define (swipe-right)
  (swipe-horizontal (focus-tree) #t))

(tm-define (swipe-up)
  (swipe-vertical (focus-tree) #f))

(tm-define (swipe-down)
  (swipe-vertical (focus-tree) #t))

(tm-define pinch-modified? #f)
(tm-define pinch-current-scale 1.0)
(tm-define pinch-current-angle 0.0)
(tm-define pinch-initial-zoom 1.0)

(tm-define (pinch-clear)
  (set! pinch-modified? #f)
  (set! pinch-current-scale 1.0)
  (set! pinch-current-angle 0.0)
  (set! pinch-initial-zoom 1.0))

(tm-define (structured-maximize t)
  (and-with p (tree-outer t)
    (structured-maximize p)))

(tm-define (structured-minimize t)
  (and-with p (tree-outer t)
    (structured-minimize p)))

(tm-define (pinch-start)
  (pinch-clear)
  (set! pinch-initial-zoom (get-window-zoom-factor)))

(tm-define (pinch-end)
  (cond ((> pinch-current-scale 1.05)
         (structured-maximize (focus-tree)))
        ((< pinch-current-scale 0.95)
         (structured-minimize (focus-tree))))
  (pinch-clear))

(tm-define (pinch-scale scale)
  (set! pinch-current-scale scale)
  (let* ((lg (/ (log scale) (log 2.0)))
         (lg* (/ (round (* 24.0 lg)) 24.0))
         (normalized-scale (exp (* (log 2.0) lg*)))
         (new-zoom (* normalized-scale pinch-initial-zoom)))
    (change-zoom-factor new-zoom)))

(tm-define (pinch-rotate angle)
  (geometry-rotate (focus-tree) angle))

(tm-define (wheel-capture?) #f)
(tm-define (wheel-event x y) (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic predicates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (simple-tags)
  '(concat document tformat table row cell shown hidden))

(tm-define (complex-context? t)
  (and (nleaf? t)
       (nin? (tree-label t) (simple-tags))))

(tm-define (simple-context? t)
  (or (leaf? t)
      (and (tree-in? t (simple-tags))
           (simple-context? (tree-down t)))))

(tm-define (document-context? t)
  (tree-is? t 'document))

(tm-define (table-markup-context? t)
  (or (tree-in? t '(table tformat))
      (and (== (tree-arity t) 1)
           (or (tree-in? (tree-ref t 0) '(table tformat))
               (and (tm-func? (tree-ref t 0) 'document 1)
                    (tree-in? (tree-ref t 0 0) '(table tformat)))))))

(tm-define (structured-horizontal? t)
  (or (tree-is-dynamic? t)
      (table-markup-context? t)))

(tm-define (structured-vertical? t)
  (or (tree-in? t '(tree))
      (table-markup-context? t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus predicates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-has-variants? t)
  (> (length (focus-variants-of t)) 1))

(tm-define (focus-has-toggles? t)
  (or (numbered-context? t)
      (alternate-context? t)))

(tm-define (focus-can-move? t)
  #t)

(tm-define (focus-can-insert-remove? t)
  (and (or (structured-horizontal? t) (structured-vertical? t))
       (cursor-inside? t)))

(tm-define (focus-can-insert? t)
  (< (tree-arity t) (tree-maximal-arity t)))

(tm-define (focus-can-remove? t)
  (> (tree-arity t) (tree-minimal-arity t)))

(tm-define (focus-has-geometry? t)
  #f)

(tm-define (focus-has-preferences? t)
  (and (tree-compound? t) (tree-label-extension? (tree-label t))))

(tm-define (focus-has-preferences? t)
  (:require (tree-in? t '(reference pageref eqref smart-ref
                          hlink locus ornament)))
  #t)

(tm-define (focus-has-preferences? t)
  (:require (tree-in? t '(bibliography bibliography* thebibliography)))
  #t)

(tm-define (focus-has-parameters? t)
  (focus-has-preferences? t))

(tm-define (focus-can-search? t) #f)
(tm-define (focus-has-search-menu? t) #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tree traversal
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (traverse-horizontal t forwards?)
  (if forwards? (go-to-next-word) (go-to-previous-word)))

(tm-define (traverse-vertical t downwards?)
  (and-with p (tree-outer t)
    (traverse-vertical p downwards?)))

(tm-define (traverse-vertical t downwards?)
  (:require (document-context? t))
  (with move (if downwards? go-to-next-tag go-to-previous-tag)
    (move 'document)))

(define (find-similar-upwards t l)
  (cond ((in? (tree-label t) l) t)
        ((and (not (tree-is-buffer? t)) (tree-up t))
         (find-similar-upwards (tree-up t) l))
        (else #f)))

(define-macro (with-focus-in l . body)
  `(begin
     ,@body
     (selection-cancel)
     (and-with t (find-similar-upwards (focus-tree) ,l)
       (tree-focus t))))

(tm-define (traverse-incremental t forwards?)
  (let* ((l (similar-to (tree-label t)))
         (fun (if forwards? go-to-next-tag go-to-previous-tag)))
    (with-focus-in l (fun l))))

(tm-define (traverse-extremal t forwards?)
  (let* ((l (similar-to (tree-label t)))
         (fun (if forwards? go-to-next-tag go-to-previous-tag))
         (inc (lambda () (fun l))))
    (with-focus-in l
      (go-to-repeat inc)
      (structured-inner-extremal t forwards?))))

(tm-define (traverse-previous)
  (traverse-incremental (focus-tree) #f))
(tm-define (traverse-next)
  (traverse-incremental (focus-tree) #t))
(tm-define (traverse-first)
  (traverse-extremal (focus-tree) #f))
(tm-define (traverse-last)
  (traverse-extremal (focus-tree) #t))
(tm-define (traverse-left)
  (traverse-horizontal (focus-tree) #f))
(tm-define (traverse-right)
  (traverse-horizontal (focus-tree) #t))
(tm-define (traverse-up)
  (traverse-vertical (focus-tree) #f))
(tm-define (traverse-down)
  (traverse-vertical (focus-tree) #t))
(tm-define (traverse-previous-section-title)
  (go-to-previous-tag (similar-to 'section)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured insert and remove
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-insert-horizontal t forwards?)
  (and-with p (tree-outer t)
    (structured-insert-horizontal p forwards?)))

(tm-define (structured-insert-vertical t downwards?)
  (and-with p (tree-outer t)
    (structured-insert-vertical p downwards?)))

(tm-define (structured-remove-horizontal t forwards?)
  (and-with p (tree-outer t)
    (structured-remove-horizontal p forwards?)))

(tm-define (structured-remove-vertical t downwards?)
  (and-with p (tree-outer t)
    (structured-remove-vertical p downwards?)))

(tm-define (structured-insert-horizontal t forwards?)
  (:require (structured-horizontal? t))
  (when (tree->path t :down)
    (insert-argument-at (tree->path t :down) forwards?)))

(tm-define (structured-remove-horizontal t forwards?)
  (:require (structured-horizontal? t))
  (when (tree->path t :down)
    (remove-argument-at (tree->path t :down) forwards?)))

(tm-define (structured-insert-extremal t forwards?)
  (structured-extremal t forwards?)
  (structured-insert-horizontal t forwards?))

(tm-define (structured-insert-incremental t downwards?)
  (structured-incremental t downwards?)
  (structured-insert-vertical t downwards?))

(tm-define (structured-insert-left)
  (structured-insert-horizontal (focus-tree) #f))
(tm-define (structured-insert-right)
  (structured-insert-horizontal (focus-tree) #t))
(tm-define (structured-remove-left)
  (structured-remove-horizontal (focus-tree) #f))
(tm-define (structured-remove-right)
  (structured-remove-horizontal (focus-tree) #t))
(tm-define (structured-insert-up)
  (structured-insert-vertical (focus-tree) #f))
(tm-define (structured-insert-down)
  (structured-insert-vertical (focus-tree) #t))
(tm-define (structured-remove-up)
  (structured-remove-vertical (focus-tree) #f))
(tm-define (structured-remove-down)
  (structured-remove-vertical (focus-tree) #t))
(tm-define (structured-insert-start)
  (structured-insert-extremal (focus-tree) #f))
(tm-define (structured-insert-end)
  (structured-insert-extremal (focus-tree) #t))
(tm-define (structured-insert-top)
  (structured-insert-incremental (focus-tree) #f))
(tm-define (structured-insert-bottom)
  (structured-insert-incremental (focus-tree) #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Structured movements
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-horizontal t forwards?)
  (and-with p (tree-outer t)
    (structured-horizontal p forwards?)))

(tm-define (structured-horizontal t forwards?)
  (:require (structured-horizontal? t))
  (with-focus-after t
    (when (tree-down t)
      (with move (if forwards? path-next-argument path-previous-argument)
        (with p (move (root-tree) (tree->path (tree-down t)))
          (if (nnull? p) (go-to p)))))))

(tm-define (structured-vertical t downwards?)
  (and-with p (tree-outer t)
    (structured-vertical p downwards?)))

(tm-define (structured-inner-extremal t forwards?)
  (and-with p (tree-outer t)
    (structured-inner-extremal p forwards?)))

(tm-define (structured-inner-extremal t forwards?)
  (:require (structured-horizontal? t))
  (with-focus-after t
    (tree-go-to t :down (if forwards? :end :start))))

(tm-define (structured-extremal t forwards?)
  (go-to-repeat (lambda () (structured-horizontal t forwards?)))
  (structured-inner-extremal t forwards?))

(tm-define (structured-incremental t downwards?)
  (go-to-repeat (lambda () (structured-vertical t downwards?)))
  (structured-inner-extremal t downwards?))

(tm-define (structured-exit t forwards?)
  (when (complex-context? t)
    (tree-go-to t (if forwards? :end :start))))

(tm-define (structured-left)
  (structured-horizontal (focus-tree) #f))
(tm-define (structured-right)
  (structured-horizontal (focus-tree) #t))
(tm-define (structured-up)
  (structured-vertical (focus-tree) #f))
(tm-define (structured-down)
  (structured-vertical (focus-tree) #t))
(tm-define (structured-start)
  (structured-extremal (focus-tree) #f))
(tm-define (structured-end)
  (structured-extremal (focus-tree) #t))
(tm-define (structured-top)
  (structured-incremental (focus-tree) #f))
(tm-define (structured-bottom)
  (structured-incremental (focus-tree) #t))
(tm-define (structured-exit-left)
  (structured-exit (focus-tree) #f))
(tm-define (structured-exit-right)
  (structured-exit (focus-tree) #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Multi-purpose alignment
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (geometry-speed t down?)
  (and-with p (tree-outer t)
    (geometry-speed p down?)))

(tm-define (geometry-variant t forwards?)
  (and-with p (tree-outer t)
    (geometry-variant p forwards?)))

(tm-define (geometry-default t)
  (and-with p (tree-outer t)
    (geometry-default p)))

(tm-define (geometry-horizontal t forwards?)
  (and-with p (tree-outer t)
    (geometry-horizontal p forwards?)))

(tm-define (geometry-vertical t down?)
  (and-with p (tree-outer t)
    (geometry-vertical p down?)))

(tm-define (geometry-extremal t forwards?)
  (and-with p (tree-outer t)
    (geometry-extremal p forwards?)))

(tm-define (geometry-incremental t down?)
  (and-with p (tree-outer t)
    (geometry-incremental p down?)))

(tm-define (geometry-scale t scale)
  (with p (tree-outer t)
    (if p (geometry-scale p scale)
        (set! pinch-current-scale scale))))

(tm-define (geometry-rotate t angle)
  (with p (tree-outer t)
    (if p (geometry-rotate p angle)
        (set! pinch-current-angle angle))))

(tm-define (geometry-slower)
  (geometry-speed (focus-tree) #f))
(tm-define (geometry-faster)
  (geometry-speed (focus-tree) #t))
(tm-define (geometry-circulate forwards?)
  (geometry-variant (focus-tree) forwards?))
(tm-define (geometry-reset)
  (geometry-default (focus-tree)))
(tm-define (geometry-left)
  (geometry-horizontal (focus-tree) #f))
(tm-define (geometry-right)
  (geometry-horizontal (focus-tree) #t))
(tm-define (geometry-up)
  (geometry-vertical (focus-tree) #f))
(tm-define (geometry-down)
  (geometry-vertical (focus-tree) #t))
(tm-define (geometry-start)
  (geometry-extremal (focus-tree) #f))
(tm-define (geometry-end)
  (geometry-extremal (focus-tree) #t))
(tm-define (geometry-top)
  (geometry-incremental (focus-tree) #f))
(tm-define (geometry-bottom)
  (geometry-incremental (focus-tree) #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special structured editing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (special-navigate t direction)
  (and-with p (tree-outer t)
    (special-navigate p direction)))

(tm-define (special-horizontal t forwards?)
  (and-with p (tree-outer t)
    (special-horizontal p forwards?)))

(tm-define (special-vertical t down?)
  (and-with p (tree-outer t)
    (special-vertical p down?)))

(tm-define (special-extremal t forwards?)
  (and-with p (tree-outer t)
    (special-extremal p forwards?)))

(tm-define (special-incremental t down?)
  (and-with p (tree-outer t)
    (special-incremental p down?)))

(tm-define (special-back)
  (special-navigate (focus-tree) :previous))
(tm-define (special-forward)
  (special-navigate (focus-tree) :next))
(tm-define (special-return)
  (special-navigate (focus-tree) :first))
(tm-define (special-shift-return)
  (special-navigate (focus-tree) :last))
(tm-define (special-left)
  (special-horizontal (focus-tree) #f))
(tm-define (special-right)
  (special-horizontal (focus-tree) #t))
(tm-define (special-up)
  (special-vertical (focus-tree) #f))
(tm-define (special-down)
  (special-vertical (focus-tree) #t))
(tm-define (special-first)
  (special-extremal (focus-tree) #f))
(tm-define (special-last)
  (special-extremal (focus-tree) #t))
(tm-define (special-previous)
  (special-incremental (focus-tree) #f))
(tm-define (special-next)
  (special-incremental (focus-tree) #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tree editing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (structured-insert-horizontal t forwards?)
  (:require (tree-is? t 'tree))
  (if (== (tree-down-index t) 0) (set! t (tree-up t)))
  (if (== (tm-car t) 'tree)
      (with pos (tree-down-index t)
        (if forwards? (set! pos (1+ pos)))
        (tree-insert! t pos '(""))
        (tree-go-to t pos 0))))

(tm-define (structured-remove-horizontal t forwards?)
  (:require (tree-is? t 'tree))
  (if (== (tree-down-index t) 0) (set! t (tree-up t)))
  (if (== (tm-car t) 'tree)
      (with pos (tree-down-index t)
        (cond (forwards?
               (tree-remove! t pos 1)
               (if (== pos (tree-arity t))
                   (tree-go-to t :end)
                   (tree-go-to t pos :start)))
              ((== pos 1) (tree-go-to t 0 :end))
              (else (tree-remove! t (- pos 1) 1))))))

(tm-define (structured-insert-vertical t downwards?)
  (:require (tree-is? t 'tree))
  (if downwards?
      (if (== (tree-down-index t) 0)
          (with pos (tree-arity t)
            (tree-insert! t pos '(""))
            (tree-go-to t pos 0))
          (begin
            (set! t (tree-down t))
            (tree-set! t `(tree ,t ""))
            (tree-go-to t 1 0)))
      (begin
        (if (!= (tree-down-index t) 0) (set! t (tree-down t)))
        (tree-set! t `(tree "" ,t))
        (tree-go-to t 0 0))))

(define (branch-active t)
  (with i (tree-down-index t)
    (if (and (= i 0) (tree-is? t :up 'tree))
        (tree-up t)
        t)))

(define (branch-go-to . l)
  (apply tree-go-to l)
  (if (tree-is? (cursor-tree) 'tree)
      (with last (cAr l)
        (if (nin? last '(:start :end)) (set! last :start))
        (tree-go-to (cursor-tree) 0 last))))

(tm-define (structured-horizontal t* forwards?)
  (:require (tree-is? t* 'tree))
  (let* ((t (branch-active t*))
         (i (tree-down-index t)))
    (cond ((and (not forwards?) (> i 1))
           (branch-go-to t (- i 1) :end))
          ((and forwards? (!= i 0) (< i (- (tree-arity t) 1)))
           (branch-go-to t (+ i 1) :start)))))

(tm-define (structured-vertical t* downwards?)
  (:require (tree-is? t* 'tree))
  (let* ((t (branch-active t*))
         (i (tree-down-index t)))
    (cond ((and (not downwards?) (!= i 0))
           (tree-go-to t 0 :end))
          ((and downwards? (== (tree-down-index t*) 0))
           (branch-go-to t* (quotient (tree-arity t*) 2) :start)))))

(tm-define (structured-extremal t* forwards?)
  (:require (tree-is? t* 'tree))
  (let* ((t (branch-active t*))
         (i (tree-down-index t)))
    (cond ((not forwards?)
           (branch-go-to t 1 :start))
          (forwards?
           (branch-go-to t :last :end)))))
  
(tm-define (structured-incremental t downwards?)
  (:require (tree-is? t 'tree))
  (go-to-repeat (if downwards? structured-down structured-up)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Extra editing functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kill-paragraph)
  (selection-set-start)
  (go-end-paragraph)
  (selection-set-end)
  (clipboard-cut "primary"))

(tm-define (backward-kill-word)
  (kbd-select traverse-left)
  (kbd-delete))

(tm-define (kill-word)
  (kbd-select traverse-right)
  (kbd-delete))

(tm-define (yank-paragraph)
  (selection-set-start)
  (go-end-paragraph)
  (selection-set-end)
  (clipboard-copy "primary"))

(tm-define (select-all)
  (let ((t (tree-ref (buffer-tree) 0)))
    ;; fix-me: 目前只针对一层 'hide-preamble 作了特殊处理
    ;;         最好改进为更加 general 的处理方式
    (if (and (tree-is? t 'hide-preamble)
             (= (tree-arity t) 1)
             (tree-is? (tree-ref t 0) 'document))
        (select-all-correct 1)
        (tree-select (buffer-tree)))))

(tm-define (go-to-line n . opt-from)
  (if (nnull? opt-from) (cursor-history-add (car opt-from)))
  (with-innermost t 'document
    (tree-go-to t n 0)))

(tm-define (go-to-column c . opt-from)
  (if (nnull? opt-from) (cursor-history-add (car opt-from)))
  (with-innermost t 'document
    (with p (tree-cursor-path t)
      (tree-go-to t (cADr p) c))))

(tm-define (select-word w t col)
  (:synopsis "Selects word @w in tree @t, more or less around column @col")
  (let* ((st (tree->string t))
         (pos (- col (string-length w)))
         (beg (string-search-forwards w (max 0 pos) st))) ; returns index of w in st
    (if beg
        (with p (tree->path t)
          (go-to (rcons p beg))
          (selection-set-start)
          (go-to (rcons p (+ beg (string-length w))))
          (selection-set-end)))
    beg))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Standard environment parameters for primitives
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (standard-parameters l)
  (:require (== l "action"))
  (list "locus-color"))

(tm-define (standard-parameters l)
  (:require (== l "locus"))
  (list "locus-color" "visited-color"))

(tm-define (standard-parameters l)
  (:require (== l "ornament"))
  (list "ornament-shape" "ornament-title-style"
        "ornament-border" "ornament-corner"
	"ornament-hpadding" "ornament-vpadding"
	"ornament-color" "ornament-extra-color"
	"ornament-sunny-color" "ornament-shadow-color"))

(tm-define (standard-parameters l)
  (:require (in? l '("reference" "pageref" "eqref" "smart-ref" "label" "tag")))
  (list))

(tm-define (standard-parameters l)
  (:require (in? l '("bibliography" "bibliography*" "thebibliography")))
  (list "bib-no-translate"))

(tm-define (search-parameters l)
  (:require (in? (if (string? l) l (symbol->string l))
                 '("reference" "pageref" "eqref" "smart-ref" "hlink")))
  (standard-parameters "locus"))

(tm-define (parameter-choice-list l)
  (:require (== l "ornament-shape"))
  (list "classic" "rounded" "angular" "cartoon"
        ;;"ring"
        ))

(tm-define (parameter-choice-list l)
  (:require (== l "ornament-title-style"))
  (list "classic"
        "top left" "top center" "top right"
        "bottom left" "bottom center" "bottom right"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Inserting various kinds of content
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (label-insert t)
  (and-with p (tree-outer t)
    (label-insert p)))

(tm-define (label-insert t)
  (:require (tree-is-buffer? t))
  (make 'label))

(tm-define (make-label)
  (label-insert (focus-tree)))

(tm-define (make-specific s)
  (if (or (== s "texmacs") (in-source?))
      (insert-go-to `(specific ,s "") '(1 0))
      (insert-go-to `(inactive (specific ,s "")) '(0 1 0))))

(tm-define (make-include u)
  (let ((delta-unix (url->delta-unix u)))
    (if delta-unix
      (insert `(include ,(utf8->cork delta-unix)))
      (set-message `(concat ,(translate "Unable to include file from another drive: ")
                            (verbatim ,(url->string u)) "")
                   (translate "include file")))))

(tm-define (make-inline-image l)
  (apply make-image (cons* (url->system (car l)) #f (cdr l))))

(tm-define (make-link-image l)
  (let ((delta-unix (url->delta-unix (car l))))
    (if delta-unix
      (apply make-image (cons* delta-unix #t (cdr l)))
      (set-message `(concat ,(translate "Unable to link images from another drive: ")
                            (verbatim ,(url->string (car l))) "")
                   (translate "link image")))))

(tm-define (make-graphics-over-selection)
  (when (selection-active-any?)
    (with selection (selection-tree)
      (clipboard-cut "graphics background")
      (insert-go-to `(draw-over ,selection (graphics) "0cm") '(1 1)))))

(tm-define (make-graphics-over)
  (if (selection-active-any?)
      (with g `(with "gr-mode" (tuple "hand-edit" "penscript") (graphics))
        (with selection (selection-tree)
          (clipboard-cut "graphics background")
          (insert-go-to `(draw-over ,selection ,g "2cm") '(1 2 1))))
      (with g `(with
                "gr-mode" (tuple "hand-edit" "penscript")
                "gr-grid" (tuple "cartesian" (point "0" "0") "2")
                "gr-edit-grid-aspect" (tuple (tuple "axes" "none")
                                              (tuple "1" "none")
                                              (tuple "10" "none"))
                "gr-edit-grid" (tuple "cartesian" (point "0" "0") "1")
                (graphics))
        (insert-go-to `(draw-over "" ,g "2cm") '(1 2 1)))))

(tm-define (make-anim l)
  (with duration "1s"
    (if (selection-active?)
        (let* ((selection (selection-tree))
	       (p (path-end selection (list))))
	  (when (selection-active-large?)
	    (set! selection `(par-block ,selection))
	    (set! p (cons 0 p)))
          (clipboard-cut "graphics background")
          (insert-go-to `(,l ,selection ,duration) (cons 0 p)))
        (insert-go-to `(,l "" ,duration) (list 0 0)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Detached notes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (propose-note-id ref?)
  (let* ((buf (buffer-tree))
         (is-ref? (cut tree-in? <> '(note-ref note-ref*)))
         (is-text? (cut tree-in? <> '(note-inline note-inline*
                                      note-wide note-wide*
                                      note-footnote note-footnote*)))
         (ref-l (tree-search buf is-ref?))
         (text-l (tree-search buf is-text?))
         (ref-id (lambda (t) (tree->stree (tm-ref t 0))))
         (text-id (lambda (t) (tree->stree (tm-ref t 1))))
         (refs (map ref-id ref-l))
         (texts (map text-id text-l))
         (diff (if ref?
                   (list-difference texts refs)
                   (list-difference refs texts))))
    (if (null? diff)
        (create-unique-id)
        (cAr diff))))

(tm-define (make-note-ref)
  (insert `(note-ref ,(propose-note-id #t))))

(tm-define (make-note-inline)
  (insert-go-to `(note-inline "" ,(propose-note-id #f)) '(0 0)))

(tm-define (make-note-wide)
  (insert-go-to `(note-wide (document "") ,(propose-note-id #f)) '(0 0 0)))

(tm-define (make-note-footnote)
  (insert-go-to `(note-footnote (document "") ,(propose-note-id #f)) '(0 0 0)))
                                      
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Thumbnails facility
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (thumbnail-suffixes)
  (list->url
    (map url-wildcard
         '("*.gif" "*.jpg" "*.jpeg" "*.JPG" "*.JPEG" "*.png" "*.PNG"))))

(define (fill-row l nr)
  (cond ((= nr 0) '())
        ((nnull? l) (cons (car l) (fill-row (cdr l) (- nr 1))))
        (else (cons "" (fill-row l (- nr 1))))))

(define (make-rows l nr)
  (if (> (length l) nr)
      (cons (list-head l nr) (make-rows (list-tail l nr) nr))
      (list (fill-row l nr))))

(define (make-thumbnails-sub l nr)
  (let* ((w (string-append (number->string (- (/ 1.0 nr) 0.02)) "par"))
         (mapper (lambda (x)
                   (and-let* ((delta-unix (url->delta-unix x)))
                    `(image ,delta-unix ,w "" "" ""))))
         (l1 (map mapper l))
         (l2 (make-rows l1 nr))
         (l3 (map (lambda (r) `(row ,@(map (lambda (c) `(cell ,c)) r))) l2)))
    (if l1
      (insert `(tabular* (tformat (twith "table-width" "1par")
                                  (twith "table-hyphen" "yes")
                                  (table ,@l3))))
      (set-message `(concat ,(translate "Unable to make thumbnail from another drive: ")
                            (verbatim ,(url->string (car l))) "")
                   (translate "make thumbnail")))))

(tm-define (make-thumbnails nr)
  (:argument nr "Number of pictures per row")
  (if (string? nr) (set! nr (min (string->number nr) 32)))
  (user-url "Picture directory" "directory" 
   (lambda (dir) 
     (let* ((find (url-append dir (thumbnail-suffixes)))
                  (files (url->list (url-expand (url-complete find "r"))))
                  (base (buffer-master))
                  (rel-files (map (lambda (x) (url-delta base x)) files)))
           (if (nnull? rel-files) (make-thumbnails-sub rel-files nr))))))
   
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Routines for floats
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (mini-flow-context? t)
  (tree-in? t (mini-flow-tag-list)))

(tm-define (in-main-flow?)
  (:synopsis "Are we inside the main document flow?")
  ;; FIXME: this routine can be improved quite a lot
  ;; we might make this property part of the DRD
  (not (tree-innermost mini-flow-context?)))

(tm-define (make-marginal-note)
  (:synopsis "Insert a marginal note")
  (wrap-selection-small
    (insert-go-to `(inactive (marginal-note "normal" "c" "")) '(0 2 0))))

(tm-define (test-marginal-note-hpos? hp)
  (and-with t (tree-innermost 'marginal-note #t)
    (tm-equal? (tree-ref t 0) hp)))
(tm-define (set-marginal-note-hpos hp)
  (:synopsis "Set the horizontal position of the marginal note to @hp")
  (:check-mark "v" test-marginal-note-hpos?)
  (and-with t (tree-innermost 'marginal-note #t)
    (tree-set t 0 hp)))

(tm-define (test-marginal-note-valign? va)
  (and-with t (tree-innermost 'marginal-note #t)
    (tm-equal? (tree-ref t 1) va)))
(tm-define (set-marginal-note-valign va)
  (:synopsis "Set the vertical alignment of the marginal note to @va")
  (:check-mark "v" test-marginal-note-valign?)
  (and-with t (tree-innermost 'marginal-note #t)
    (tree-set t 1 va)))

(tm-define (make-insertion s)
  (:synopsis "Make an insertion of type @s")
  (:applicable (in-main-flow?))
  (with pos (if (== s "float") "tbh" "")
    (insert-go-to (list 'float s pos (list 'document ""))
                  (list 2 0 0))))

(define (any-float? t)
  (tree-in? t '(float wide-float phantom-float)))

(tm-define (insertion-positioning what flag)
  (:synopsis "Allow/disallow the position @what for innermost float")
  (and-with t (tree-innermost any-float? #t)
    (let ((op (if flag string-union string-minus))
          (st (tree-ref t 1)))
      (tree-set! st (op (tree->string st) what)))))

(define (test-insertion-positioning? what)
  (and-with t (tree-innermost any-float? #t)
    (with c (string-ref what 0)
      (char-in-string? c (tree->string (tree-ref t 1))))))

(define (not-test-insertion-positioning? s)
  (not (test-insertion-positioning? s)))

(tm-define (toggle-insertion-positioning what)
  (:check-mark "v" test-insertion-positioning?)
  (insertion-positioning what (not-test-insertion-positioning? what)))

(tm-define (toggle-insertion-positioning-not s)
  (:check-mark "v" not-test-insertion-positioning?)
  (toggle-insertion-positioning s))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Balloons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (balloon-context? t)
  (tree-in? t (balloon-tag-list)))

(define (integer-floor x)
  (inexact->exact (floor x)))

(tm-define (display-balloon body balloon halign valign type)
  (:secure #t)
  (let* ((kind (or (tm->string type) "default"))
         (ha (or (tm->string halign) (if (== kind "mouse") "right" "left")))
         (va (or (tm->string valign) "Bottom"))
         (p  (tree->path body))
         (st (tree->stree body))
         (id (or (list p st) st)))
    (show-tooltip id body balloon ha va kind 0.833333)))

(tm-define (make-balloon)
  (:synopsis "Insert a balloon")
  (wrap-selection-small
    (insert-go-to `(inactive (hover-balloon "" "" "left" "Bottom"))
                  '(0 0 0))))

(tm-define (test-balloon-halign? ha)
  (and-with t (tree-innermost balloon-context? #t)
    (tm-equal? (tree-ref t 2) ha)))
(tm-define (set-balloon-halign ha)
  (:synopsis "Set the horizontal alignment of the marginal note to @ha")
  (:check-mark "v" test-balloon-halign?)
  (and-with t (tree-innermost balloon-context? #t)
    (tree-set t 2 ha)))

(tm-define (test-balloon-valign? va)
  (and-with t (tree-innermost balloon-context? #t)
    (tm-equal? (tree-ref t 3) va)))
(tm-define (set-balloon-valign va)
  (:synopsis "Set the vertical alignment of the marginal note to @va")
  (:check-mark "v" test-balloon-valign?)
  (and-with t (tree-innermost balloon-context? #t)
    (tree-set t 3 va)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Sound and video
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (make-sound u)
  (let ((delta-unix (url->delta-unix u)))
    (cond
      ((url-none? u)
       (set-message `(concat ,(translate "Unable to make sound which url is none: ")
                            (verbatim ,(url->string u)) "")
                   (translate "make sound")))
      ((not delta-unix)
       (set-message `(concat ,(translate "Unable to make sound from another drive: ")
                            (verbatim ,(url->string u)) "")
                   (translate "make sound")))
      (else (insert `(sound ,delta-unix))))))

(tm-define (make-animation u)
  (let ((delta-unix (url->delta-unix u)))
    (if delta-unix
      (interactive
          (lambda (w h len rep)
            (if (== rep "no") (set! rep "false"))
            (insert `(video ,delta-unix ,w ,h ,len ,rep)))
        "Width" "Height" "Length" "Repeat?")
      (set-message `(concat ,(translate "Unable to make animation from another drive: ")
                            (verbatim ,(url->string u)) "")
                   (translate "make animation")))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Labels attached to markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-label t) #f)

(tm-define (focus-get-label t)
  (and-with l (focus-label t)
    (tm->string (tm-ref l 0))))

(tm-define (focus-set-label t val)
  (and-with l (focus-label t)
    (tree-set l 0 val)))

(tm-define (focus-list-search-label l)
  (and (nnull? l)
       (or (focus-search-label (car l))
           (focus-list-search-label (cdr l)))))

(tm-define (focus-search-label t)
  (cond ((tm-func? t 'label 1) t)
        ((tm-in? t '(document concat table row cell))
         (focus-list-search-label (tm-children t)))
        ((tm-in? t '(tformat with surround))
         (focus-search-label (cAr (tm-children t))))
        (else #f)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special keyboard behaviour when entering hybrid commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (hybrid-kbd-space)
  (activate-hybrid #f)
  (insert " "))

(tm-define (hybrid-kbd-curly-left)
  (with-innermost t 'hybrid
    (with cmd (tm->string (tm-ref t 0))
      (cond ((or (not cmd) (== cmd "begin"))
             (insert "{"))
            ((in? cmd '("left\\" "right\\"))
             (insert "{")
             (activate-hybrid #f))
            (else
             (activate-hybrid #f))))))

(tm-define (hybrid-kbd-curly-right)
  (with-innermost t 'hybrid
    (with cmd (tm->string (tm-ref t 0))
      (cond ((not cmd)
             (activate-hybrid #f))
            ((string-starts? (tm->string cmd) "begin{")
             (tree-remove (tm-ref t 0) 0 6)
             (activate-hybrid #f))
            ((in? cmd '("left\\" "right\\"))
             (insert "}")
             (activate-hybrid #f))
            (else
             (activate-hybrid #f))))))

(tm-define (hybrid-kbd-backslash)
  (with-innermost t 'hybrid
    (with cmd (tm->string (tm-ref t 0))
      (cond ((in? cmd '("left" "right"))
             (insert "\\"))
            (else
             (activate-hybrid #f)
             (make-hybrid))))))

(tm-define (hybrid-kbd-sub)
  (activate-hybrid #f)
  (make-script #f #t))

(tm-define (hybrid-kbd-sup)
  (activate-hybrid #f)
  (make-script #t #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Search, replace, spell and tab-completion
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (key-press-command key)
  ;; FIXME: this routine should do exactly the same as key-press,
  ;; without modification of the internal state and without executing
  ;; the actual shortcut. It should rather return a command which
  ;; does all this, or #f
  (and-with p (kbd-find-key-binding key)
    (car p)))

(tm-define (keyboard-press key time)
  (:mode search-mode?)
  (with cmd (key-press-command (string-append "search " key))
    (cond (cmd (cmd))
          ((key-press-search key) (noop))
          (else (key-press key)))))

(tm-define (search-next)
  (key-press-search "next"))

(tm-define (search-previous)
  (key-press-search "previous"))

(tm-define (keyboard-press key time)
  (:mode replace-mode?)
  (with cmd (key-press-command (string-append "replace " key))
    (cond (cmd (cmd))
          ((key-press-replace key) (noop))
          (else (key-press key)))))

(tm-define (keyboard-press key time)
  (:mode spell-mode?)
  (with cmd (key-press-command (string-append "spell " key))
    (cond (cmd (cmd))
          ((key-press-spell key) (noop))
          (else (key-press key)))))

(tm-define (keyboard-press key time)
  (:mode complete-mode?)
  (with cmd (key-press-command (string-append "complete " key))
    (cond (cmd (cmd))
          ((key-press-complete key) (noop))
          (else (key-press key)))))

(tm-define (keyboard-press key time)
  (:require (and (in-source?) (not (in-source-mode?))))
  (with cmd (key-press-command (string-append "complete " key))
    (cond (cmd (cmd))
          ((key-press-source-complete key) (noop))
          (else (key-press key)))))

(tm-define (keyboard-press key time)
  (:mode remote-control-mode?)
  ;;(display* "Press " key "\n")
  (if (ahash-ref remote-control-remap key)
      (begin
        ;;(display* "Remap " (ahash-ref remote-control-remap key) "\n")
        (key-press (ahash-ref remote-control-remap key)))
      (key-press key)))

(tm-define (focus-open-search-tool t)
  (:interactive #t)
  (noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Marked text utilities
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (get-marked-color)
  (let* ((default-color (if (has-style-package? "dark")
                           "#dc9f4f"    ; 深色主题默认
                           "#ffe47f"))  ; 浅色主题默认
         (color (get-preference "marked-color")))
    (if (or (== color "") (== color "default"))
        default-color
        color)))

(tm-define (pure-text? t)
  (and (in-text?)
       (cond ((tree-is? t 'string) #t)
             ((tree-is? t 'document)
              (let loop ((i 0))
                (cond ((>= i (tree-arity t)) #t)
                      ((not (tree-is? (tree-ref t i) 'string)) #f)
                      (else (loop (+ i 1))))))
             (else #f))))

(tm-define (mark-text)
  (if (selection-active-any?)
      (let ((stree (tree->stree (selection-tree))))
        (if (pure-text? (selection-tree))
            (make-with "text-bg-color" (get-marked-color))
            (begin
              (make 'marked)
              (when (not (== (get-marked-color) "#ffe47f"))
                (with-set (focus-tree) "marked-color" (get-marked-color))))))
      (make-with "text-bg-color" (get-marked-color))))
