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
    (kernel library list)
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

;; stree 原子与 symbol 判等：属性对可能为字符串或 symbol

(define (ai-atom-eq? x sym)
  (or (eq? x sym) (and (string? x) (string=? x (symbol->string sym))))
) ;define

;; inline 数学：(with "mode" "math" ...)

(define (ai-inline-math? st)
  (and (pair? st)
    (eq? (car st) 'with)
    (>= (length st) 3)
    (let ((a1 (list-ref st 1)))
      (and (pair? a1) (ai-atom-eq? (car a1) 'mode) (ai-atom-eq? (cadr a1) 'math))
    ) ;let
  ) ;and
) ;define

(define (ai-flatten-text st)
  (cond ((string? st) st)
        ((null? st) "")
        ((not (pair? st)) "")
        ((eq? (car st) 'with)
         (if (ai-inline-math? st) "[FORMULA]" (ai-flatten-text (last st)))
        ) ;
        ((memq (car st) ai-formula-labels) "[FORMULA]")
        ((memq (car st) ai-table-labels) "[TABLE]")
        ((memq (car st) ai-image-labels) "[IMAGE]")
        ((eq? (car st) 'document) (string-join (map ai-flatten-text (cdr st)) "\n"))
        (else (string-concatenate (map ai-flatten-text (cdr st))))
  ) ;cond
) ;define

;; document stree 中第 i1 到 i2 个子节点（含两端），保持 document 标签

(define (ai-span-stree st i1 i2)
  (cons (car st) (list-take (list-tail (cdr st) i1) (+ (- i2 i1) 1)))
) ;define

(tm-define (ai-selection-context)
  (:synopsis "选区上下文（AI 释义的引文1）的纯文本")
  ;; 同段选区取所在段落（para 标签节点，否则公共前缀节点本身——顶层段落
  ;; 内容），是「引文2是引文1的一部分」成立的最小上下文；跨段选区的公共
  ;; 祖先是 document 时，只拼接覆盖选区两端的直接子节点（选中的若干段），
  ;; 不把整个公共祖先（可能远超选区）喂给模型
  (let* ((p1 (selection-get-start))
         (p2 (selection-get-end))
         (common (list-common p1 p2))
         (base (path->tree common))
         (para (and base (tree-search-upwards base 'para)))
        ) ;
    (cond
      (para (tm-string-trim-both (ai-flatten-text (tm->stree para))))
      ((and base (tree-is? base 'document))
       (let* ((r1 (list-tail p1 (length common)))
              (r2 (list-tail p2 (length common)))
              (st (tm->stree base))
              (i1 (if (null? r1) 0 (car r1)))
              (i2 (if (null? r2) (- (length st) 2) (car r2)))
             ) ;
         (tm-string-trim-both (ai-flatten-text (ai-span-stree st i1 i2)))
       ) ;let*
      ) ;
      (base (tm-string-trim-both (ai-flatten-text (tm->stree base))))
      (else "")
    ) ;cond
  ) ;let*
) ;tm-define
