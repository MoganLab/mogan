;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ai-actions-bar.scm
;; DESCRIPTION : AI 操作栏（AiActionsBar）的选区内容判定
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
