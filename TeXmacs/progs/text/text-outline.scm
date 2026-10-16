
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : text-outline.scm
;; DESCRIPTION : document outline (section tree) sidebar for the editor
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This module provides a document outline sidebar that shows the section
;; structure of the current buffer. Clicking an entry navigates to it.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (text text-outline)
  (:use (text text-drd) (text text-structure))
) ;texmacs-module

;; ---------------------------------------------------------------------------
;; 辅助函数：路径转冒号分隔字符串
;; ---------------------------------------------------------------------------

(define (section-path->string p)
  (if (null? p) "" (string-join (map number->string p) ":"))
) ;define

;; ---------------------------------------------------------------------------
;; 章节级别映射（0 为最高级 Part，依次递减）
;; ---------------------------------------------------------------------------

(define (section-level t)
  (with lbl
    (tree-label t)
    (cond ((member lbl '(part part*)) 0)
          ((member lbl '(chapter chapter* appendix appendix*)) 1)
          ((member lbl '(section section*)) 2)
          ((member lbl '(subsection subsection*)) 3)
          ((member lbl '(subsubsection subsubsection*)) 4)
          ((member lbl '(paragraph paragraph*)) 5)
          ((member lbl '(subparagraph subparagraph*)) 6)
          (else 7)
    ) ;cond
  ) ;with
) ;define

;; ---------------------------------------------------------------------------
;; 纯净标题提取（不含前置伪缩进空格）
;; ---------------------------------------------------------------------------

(define (outline-clean-title s)
  (with raw-title
    (tm/section-get-title-string s #f)
    (if (and (string? raw-title) (!= raw-title "no title") (!= raw-title ""))
      raw-title
      (if (> (tree-arity s) 0)
        (with t-str
          (texmacs->title-string (tree-ref s 0))
          (if (!= t-str "") t-str "Untitled")
        ) ;with
        "Untitled"
      ) ;if
    ) ;if
  ) ;with
) ;define

;; ---------------------------------------------------------------------------
;; 将扁平的 (level title path-str) 列表构建为具有父子关系的嵌套树
;; 输出格式: ((title path-str (child ...)) ...)
;; ---------------------------------------------------------------------------

(define (build-outline-tree items)
  (if (null? items)
    '()
    (letrec ((min-lvl (apply min (map car items)))
             (gather
               (lambda (rem current-lvl)
                 (let loop
                   ((cur-rem rem) (acc '()))
                   (cond ((null? cur-rem) (cons (reverse acc) '()))
                         ((< (caar cur-rem) current-lvl) (cons (reverse acc) cur-rem))
                         (else
                           (let* ((item (car cur-rem)) (lvl (car item)) (title (cadr item)) (p-str (caddr item)))
                             (let* ((sub-res (gather (cdr cur-rem) (+ lvl 1)))
                                    (children (car sub-res))
                                    (after (cdr sub-res))
                                    (node (list title p-str children))
                                   ) ;
                               (loop after (cons node acc))
                             ) ;let*
                           ) ;let*
                         ) ;else
                   ) ;cond
                 ) ;let
               ) ;lambda
             ) ;gather
            ) ;
      (car (gather items min-lvl))
    ) ;letrec
  ) ;if
) ;define

;; ---------------------------------------------------------------------------
;; 大纲默认展开状态计算
;; 规则 1：单非叶子节点链条自动递归穿透展开
;; 规则 2：基于 1.5 屏（30 条）阈值的逐层贪心评估展开
;; ---------------------------------------------------------------------------

(define (outline-node-leaf? node)
  (null? (caddr node))
) ;define

(define (outline-node-non-leaf? node)
  (pair? (caddr node))
) ;define

(define (outline-node-path node)
  (cadr node)
) ;define

(define (outline-node-title node)
  (car node)
) ;define

(define (outline-node-children node)
  (caddr node)
) ;define

(tm-define (outline-default-expanded-state tree . rest)
  (:synopsis "Compute default expanded nodes and visible count for an outline tree"
  ) ;:synopsis
  (let ((threshold (if (pair? rest) (car rest) 30)))
    (if (null? tree)
      (cons '() 0)
      ;; 阶段一：规则 1 单支递归穿透
      (let loop-p1
        ((curr tree) (expanded '()))
        (let ((leaves (list-filter curr outline-node-leaf?))
              (non-leaves (list-filter curr outline-node-non-leaf?))
             ) ;
          (if (and (null? leaves) (== (length non-leaves) 1))
            (let ((sole (car non-leaves)))
              (loop-p1 (outline-node-children sole) (cons sole expanded))
            ) ;let
            ;; 阶段二：规则 2 逐层评估展开（阈值 threshold）
            (let loop-p2
              ((layer curr) (vis-count (+ (length expanded) (length curr))) (exp expanded))
              (let ((candidates (list-filter layer outline-node-non-leaf?)))
                (if (null? candidates)
                  (cons (reverse exp) vis-count)
                  (let* ((new-children (apply append (map outline-node-children candidates)))
                         (new-count (length new-children))
                         (total (+ vis-count new-count))
                        ) ;
                    (if (<= total threshold)
                      (loop-p2 new-children total (append (reverse candidates) exp))
                      (cons (reverse exp) vis-count)
                    ) ;if
                  ) ;let*
                ) ;if
              ) ;let
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;if
  ) ;let
) ;tm-define

(tm-define (outline-default-expanded-paths tree . rest)
  (:synopsis "Return list of paths for default expanded nodes in outline tree")
  (let ((state (apply outline-default-expanded-state (cons tree rest))))
    (map outline-node-path (car state))
  ) ;let
) ;tm-define

(tm-define (outline-default-expanded-titles tree . rest)
  (:synopsis "Return list of titles for default expanded nodes in outline tree")
  (let ((state (apply outline-default-expanded-state (cons tree rest))))
    (map outline-node-title (car state))
  ) ;let
) ;tm-define

;; ---------------------------------------------------------------------------
;; 获取当前缓冲区的文档大纲嵌套树
;; ---------------------------------------------------------------------------

(tm-define (document-outline)
  (:synopsis "Return the hierarchical document outline tree for current buffer")
  (with raw-sections
    (tree-search-sections (buffer-tree))
    ;; 单遍遍历：每个 section 只计算一次 tree->path，无路径的节点直接跳过
    (let loop
      ((ss raw-sections) (acc '()))
      (if (null? ss)
        (build-outline-tree (reverse acc))
        (let* ((s (car ss))
               (p
                 (and (not (equal? (tree-label s) 'subparagraph)) (tree->path s))
               ) ;p
              ) ;
          (loop (cdr ss)
            (if p
              (cons (list (section-level s) (outline-clean-title s) (section-path->string p))
                acc
              ) ;cons
              acc
            ) ;if
          ) ;loop
        ) ;let*
      ) ;if
    ) ;let
  ) ;with
) ;tm-define

;; ---------------------------------------------------------------------------
;; 编辑器跳转到指定大纲路径
;; ---------------------------------------------------------------------------

(define (outline-chapter-path? p)
  ;; 章节路径约定形如 (1 0 n ...)
  (and (>= (length p) 3) (== (car p) 1) (== (cadr p) 0))
) ;define

(define (outline-get-chap-idx p)
  (cond ((outline-chapter-path? p) (caddr p))
        ((>= (length p) 2) (cadr p))
        (else #f)
  ) ;cond
) ;define

(define (outline-fallback-path p last-child)
  (cond ((outline-chapter-path? p) (list 1 0 last-child 0))
        (else (list (car p) last-child 0))
  ) ;cond
) ;define

(tm-define (outline-go-to path-str)
  (:synopsis "Navigate editor cursor and view to section outline path")
  (if (and (string? path-str) (not (string-null? path-str)))
    (let* ((parts (string-split path-str #\:)) (p (map string->number parts)))
      (when (and (pair? p) (not (memq #f p)))
        (let* ((pending? (and (defined? 'typeset-in-progress?) (typeset-in-progress?)))
               (cur-child (if pending? (typeset-frontier) #f))
               (chap-idx (outline-get-chap-idx p))
               (unready? (and pending? cur-child chap-idx (>= chap-idx cur-child)))
               (target-p
                 (if unready? (outline-fallback-path p (max 0 (- cur-child 1))) p)
               ) ;target-p
              ) ;
          (with t
            (path->tree target-p)
            (if (and t (> (tree-arity t) 0)) (tree-go-to t 0 :start) (go-to-path target-p))
            (with u
              (current-view)
              (when u
                (make-cursor-visible u)
                (delayed (:idle 1) (make-cursor-visible u))
              ) ;when
            ) ;with
          ) ;with
          (when unready?
            (set-message (translate "Document is still typesetting in background, jumped to available page..."
                         ) ;translate
              (translate "typesetting progress")
            ) ;set-message
          ) ;when
        ) ;let*
      ) ;when
    ) ;let*
  ) ;if
) ;tm-define
