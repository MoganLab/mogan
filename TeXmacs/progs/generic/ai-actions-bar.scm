;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-actions-bar.scm
;; DESCRIPTION : AI 操作栏（AiActionsBar）的选区内容判定与释义上下文
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic ai-actions-bar)
  (:use (kernel texmacs tm-define)
    (kernel library base)
    (kernel library content)
    (kernel library tree)
    (texmacs texmacs tm-tools)
    (utils library tree)
  ) ;:use
) ;texmacs-module

;; image 标签视为叶子；其余复合节点递归；字符串叶子须全空白。
;; 保守策略：with 等含非空白参数串的复合节点一律判 #f（照常弹出）

(define (ai-only-images-or-blank? t)
  (cond
   ((tree-atomic? t) (string-null? (tm-string-trim-both (tree->string t))))
   ((tree-is? t 'image) #t)
   (else
     (let loop
       ((i 0))
       (cond ((>= i (tree-arity t)) #t)
             ((ai-only-images-or-blank? (tree-ref t i)) (loop (+ i 1)))
             (else #f)
       ) ;cond
     ) ;let
   ) ;else
  ) ;cond
) ;define

(tm-define (ai-selection-only-images? t)
  (:synopsis "选区树是否只含图片（除空白外无其它内容，且至少一张图片）"
  ) ;:synopsis
  ;; tm-find-tag 命中即返回子树而非 #t，归一化为布尔供 C++ as_bool 消费
  (if (and (tm-find-tag t 'image) (ai-only-images-or-blank? t)) #t #f)
) ;tm-define

;; 翻译按钮可见性（0995）：过短或纯公式的选区不适合翻译，操作栏上隐藏翻译
;; 按钮（润色/对话仍显示）。字数口径与 Document statistics 一致（verbatim
;; 文本、去换行），纯函数便于单测。
(tm-define (ai-translate-eligible? sel)
  (:synopsis "选区树是否适合翻译（非纯公式，且字符数 >= 10）")
  (and (not (tree-in? sel '(math equation equation* eqnarray eqnarray*)))
    (>= (count-characters sel) 10)
  ) ;and
) ;tm-define

;; =============================================================================
;; 释义上下文（引文1）
;; =============================================================================
;; 展平约定照 ghost 上下文：表格/图片/公式换成标记，with 只取末尾 body（前面
;; 是 key/val 属性对），document 子节点换行连接。

(define ai-formula-labels
  '(equation equation* eqnarray eqnarray* align align* math)
) ;define

(define ai-table-labels '(table table*))

(define ai-image-labels '(image postscript graphics draw-over draw-under))

(define (ai-list-last l)
  (if (null? (cdr l)) (car l) (ai-list-last (cdr l)))
) ;define

;; inline 数学：(with "mode" "math" ...)，属性对可能为字符串或 symbol

(define (ai-inline-math? st)
  (and (pair? st)
    (eq? (car st) 'with)
    (>= (length st) 3)
    (let ((a1 (list-ref st 1)))
      (and (pair? a1)
        (or (and (string? (car a1)) (string=? (car a1) "mode")) (eq? (car a1) 'mode))
        (or (and (string? (cadr a1)) (string=? (cadr a1) "math")) (eq? (cadr a1) 'math))
      ) ;and
    ) ;let
  ) ;and
) ;define

(define (ai-join-lines lst)
  (cond ((null? lst) "")
        ((null? (cdr lst)) (car lst))
        (else (string-append (car lst) "\n" (ai-join-lines (cdr lst))))
  ) ;cond
) ;define

(define (ai-flatten-text st)
  (cond ((string? st) st)
        ((null? st) "")
        ((not (pair? st)) "")
        ((eq? (car st) 'with)
         (if (ai-inline-math? st) "[FORMULA]" (ai-flatten-text (ai-list-last st)))
        ) ;
        ((memq (car st) ai-formula-labels) "[FORMULA]")
        ((memq (car st) ai-table-labels) "[TABLE]")
        ((memq (car st) ai-image-labels) "[IMAGE]")
        ((eq? (car st) 'document) (ai-join-lines (map ai-flatten-text (cdr st))))
        (else (apply string-append (map ai-flatten-text (cdr st))))
  ) ;cond
) ;define

;; 最长公共前缀：完整包含选区两端的最深节点

(define (ai-common-prefix p1 p2)
  (cond ((null? p1) '())
        ((null? p2) '())
        ((== (car p1) (car p2)) (cons (car p1) (ai-common-prefix (cdr p1) (cdr p2))))
        (else '())
  ) ;cond
) ;define

(tm-define (ai-selection-context)
  (:synopsis "选区所在段落（AI 释义的引文1）的纯文本")
  ;; 段落是「引文2是引文1的一部分」成立的最小上下文；跨段选区无公共段落，
  ;; 退回共同祖先（仍完整包含选区）
  (let* ((common (ai-common-prefix (selection-get-start) (selection-get-end)))
         (base (and (pair? common) (path->tree common)))
         (ctx (and base (or (tree-search-upwards base 'para) base)))
        ) ;
    (if ctx (tm-string-trim-both (ai-flatten-text (tm->stree ctx))) "")
  ) ;let*
) ;tm-define
