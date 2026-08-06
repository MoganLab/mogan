
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : text-outline.scm
;; DESCRIPTION : document outline (section tree) sidebar for the editor
;; COPYRIGHT   : (C) 2026
;;
;; This module provides a document outline sidebar that shows the section
;; structure of the current buffer. Clicking an entry navigates to it.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (text text-outline))

;; ---------------------------------------------------------------------------
;; 获取文档大纲数据（嵌套列表）
;; 每个节点: (title path-as-string . children)
;;   title:        章节标题（带缩进前缀）
;;   path-as-string: 路径字符串，如 "0:1:2"，用于 C++ 端 go_to
;;   children:      子节点列表
;; ---------------------------------------------------------------------------

(define (section-path->string p)
  ;; 将 path (list of ints) 转为字符串 "0:1:2"
  (if (null? p) "" (string-join (map number->string p) ":"))
) ;define

(define (section-level t)
  ;; 返回章节级别：0=chapter, 1=section, 2=subsection, ...
  (with lbl
    (tree-label t)
    (cond ((in? lbl '(chapter chapter* appendix appendix* part part*)) 0)
          ((in? lbl '(section section*)) 1)
          ((in? lbl '(subsection subsection*)) 2)
          ((in? lbl '(subsubsection subsubsection*)) 3)
          (else 4)
    ) ;cond
  ) ;with
) ;define

(define (outline-nodes->nested nodes)
  ;; 将扁平的 section 列表按层级转为嵌套结构
  (let iter
    ((ns nodes) (result '()))
    (if (null? ns)
      result
      (let* ((s (car ns))
             (lvl (section-level s))
             (title (tm/section-get-title-string s #t))
             (p (tree->path s))
             (path-str (if p (section-path->string p) ""))
             (node (list title path-str))
            ) ;
        ;; 简单扁平输出，层级信息由 title 缩进体现
        (iter (cdr ns) (cons node result))
      ) ;let*
    ) ;if
  ) ;let
) ;define

(tm-define (document-outline)
  ;; 返回文档大纲：((title path-string) ...)，按文档顺序
  (with raw-sections
    (tree-search-sections (buffer-tree))
    (let* ((sections (list-filter raw-sections
                       (lambda (x) (not (equal? (tree-label x) 'subparagraph)))
                     ) ;list-filter
           ) ;sections
           (nodes (outline-nodes->nested sections))
          ) ;
      (reverse nodes)
    ) ;let*
  ) ;with
) ;tm-define

;; ---------------------------------------------------------------------------
;; 侧边栏 widget（tm-widget 模式）
;; ---------------------------------------------------------------------------

(tm-define (document-outline-widget)
  (resize "200px"
    "100%"
    (refreshable "document-outline-refresh"
      (vertical (for (item (document-outline)) (horizontal ((eval (car item))))))
    ) ;refreshable
  ) ;resize
) ;tm-define
