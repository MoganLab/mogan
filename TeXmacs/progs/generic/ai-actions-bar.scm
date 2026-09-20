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
  ) ;:use
) ;texmacs-module

;; image 标签视为叶子；with 的 key/val 属性对不算内容（居中图片的选区树是
;; (with "par-mode" "center" (image ...))，1603），只递归末尾 body；其余复合
;; 节点逐子节点递归；字符串叶子须全空白。

(define (ai-only-images-or-blank? t)
  (cond
   ((tree-atomic? t) (string-null? (tm-string-trim-both (tree->string t))))
   ((tree-is? t 'image) #t)
   ((and (tree-is? t 'with) (> (tree-arity t) 0))
    (ai-only-images-or-blank? (tree-ref t :last))
   ) ;
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
;; 释义上下文（引文1）：ghost 字符预算窗口
;; =============================================================================
;; 展平与切分照 ghost 上下文（liii ghost-context.scm 的 ghost-split-text-context，
;; mogan2 不依赖该插件，此处自实现）：表格/图片换成 [TABLE]/[IMAGE] 标记，with
;; 只取末尾 body（前面是 key/val 属性对），document 子节点换行连接；按路径把
;; 正文切成（选区前文本 . 选区后文本），再按字符预算截断。
;;
;; 公式不走文本标记（0996 追加：引文块里公式要渲染成真实公式，与翻译/对话的
;; 引用块一致）：展平时公式子树登记进 ai-formula-store 并在文本里留下
;; <#F<k>G> 哨兵（k 为登记表下标），切分/截断完成后由 ai-context->document
;; 把哨兵原位换回子树，组装成 document 树。哨兵含非十六进制字符 G，不会与
;; 真实 herk <#XXXX> 序列冲突；形如 <#...> 的闭合片段，herk 截断保护同样
;; 不会把它切半。

;; 字符预算同 ghost：选区前 1500 字节 / 选区后 500 字节（herk 编码下 CJK
;; 一个字占 7 字节的 <#XXXX> 序列，预算按字节计、截断不切半序列）

(define ai-context-before-limit 1500)

(define ai-context-after-limit 500)

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

;; 展平遇到的公式子树登记表（ai-selection-context 入口重置）：哨兵 <#F<k>G>
;; 的 k 即本列表下标，重组时原位换回子树

(define ai-formula-store '())

(define (ai-formula-store-reset!)
  (set! ai-formula-store '())
) ;define

(define (ai-formula-sentinel st)
  (let ((k (length ai-formula-store)))
    (set! ai-formula-store (append ai-formula-store (list st)))
    (string-append "<#F" (number->string k) "G>")
  ) ;let
) ;define

;; 不透明节点（公式/表格/图片，含 inline 数学 with）谓词，纯函数；
;; ai-marker 有登记副作用，cond 测试位只能用本谓词

(define (ai-opaque-node? st)
  (or (ai-inline-math? st)
    (memq (car st) ai-formula-labels)
    (memq (car st) ai-table-labels)
    (memq (car st) ai-image-labels)
  ) ;or
) ;define

;; 不透明节点的替换标记，透明则 #f；公式登记子树后留哨兵，表格/图片留文本标记

(define (ai-marker st)
  (cond
   ((or (ai-inline-math? st) (memq (car st) ai-formula-labels))
    (ai-formula-sentinel st)
   ) ;
   ((memq (car st) ai-table-labels) "[TABLE]")
   ((memq (car st) ai-image-labels) "[IMAGE]")
   (else #f)
  ) ;cond
) ;define

(define (ai-flatten-text st)
  (cond ((string? st) st)
        ((not (pair? st)) "")
        ;; with：inline 数学整体替换；其余只取末尾 body（前面是 key/val 属性对）
        ((eq? (car st) 'with) (or (ai-marker st) (ai-flatten-text (last st))))
        ((eq? (car st) 'document) (string-join (map ai-flatten-text (cdr st)) "\n"))
        (else
          (or (ai-marker st) (string-concatenate (map ai-flatten-text (cdr st))))
        ) ;else
  ) ;cond
) ;define

;; ===== 按路径切分：path 末位是字符串叶子的字符偏移 =====

(define (ai-safe-substring s from to)
  (let* ((len (string-length s)) (f (max 0 (min from len))) (t (max f (min to len))))
    (substring s f t)
  ) ;let*
) ;define

(define (ai-split-node st path)
  (cond ((null? path) (cons "" (ai-flatten-text st)))
        ((string? st)
         (let ((idx (car path)))
           (cons (ai-safe-substring st 0 idx)
             (ai-safe-substring st idx (string-length st))
           ) ;cons
         ) ;let
        ) ;
        ((not (pair? st)) (cons "" ""))
        (else (ai-split-compound st path))
  ) ;cond
) ;define

;; compound 切分：idx 为路径所指的子节点（相对 children），rest 下钻。
;; 数学/表格/图片整体不透明（光标入内时全归 after 的标记）；with 的属性对
;; 不进上下文，只对末尾 body 下钻

(define (ai-split-compound st path)
  (let* ((idx (car path)) (rest (cdr path)) (lab (car st)))
    (cond ((ai-opaque-node? st) (cons "" (ai-flatten-text st)))
          ((eq? lab 'with) (ai-split-node (last st) rest))
          ((or (< idx 0) (>= (+ 1 idx) (length st))) (cons "" (ai-flatten-text st)))
          (else
            (let* ((children (cdr st))
                   (pivot-pair (ai-split-node (list-ref children idx) rest))
                   ;; document 子节点换行连接：把切点两侧的文本片段当作首尾
                   ;; 子节点拼回 document 再展平，分隔符自然落在切口两侧
                   (before (append (list-take children idx) (list (car pivot-pair))))
                   (after (cons (cdr pivot-pair) (list-tail children (+ idx 1))))
                  ) ;
              (if (eq? lab 'document)
                (cons (ai-flatten-text (cons 'document before))
                  (ai-flatten-text (cons 'document after))
                ) ;cons
                (cons (string-concatenate (map ai-flatten-text before))
                  (string-concatenate (map ai-flatten-text after))
                ) ;cons
              ) ;if
            ) ;let*
          ) ;else
    ) ;cond
  ) ;let*
) ;define

;; ===== 截断：不切半 herk 的 <#XXXX> 序列 =====

;; 保留前 limit 字节；切点落入 <#...> 序列内（序列起点在窗内且 '>' 未到）
;; 时退到序列起点

(define (ai-herk-head s limit)
  (let ((n (string-length s)))
    (if (<= n limit)
      s
      (let* ((cand (substring s 0 limit))
             (open (string-search-forwards "<#" (max 0 (- limit 8)) cand))
            ) ;
        (if (and (>= open 0) (< (string-search-forwards ">" open cand) 0))
          (substring cand 0 open)
          cand
        ) ;if
      ) ;let*
    ) ;if
  ) ;let
) ;define

;; 保留后 limit 字节；切点左侧有未闭合的 <# 时，从序列 '>' 之后开始

(define (ai-herk-tail s limit)
  (let ((n (string-length s)))
    (if (<= n limit)
      s
      (let* ((start (- n limit)) (open (string-search-forwards "<#" (max 0 (- start 8)) s)))
        (if (and (>= open 0) (< open start))
          (let ((close (string-search-forwards ">" open s)))
            (if (and (>= close 0) (>= close start))
              (substring s (+ close 1) n)
              (substring s start n)
            ) ;if
          ) ;let
          (substring s start n)
        ) ;if
      ) ;let*
    ) ;if
  ) ;let
) ;define

;; ===== 重组：哨兵换回公式子树，文本组装成 document stree =====

;; 一行文本按哨兵 <#F<k>G> 拆成节点列表（字符串与公式子树交错），空串剔除

(define (ai-sentinel-split s store)
  (let ((open (string-search-forwards "<#F" 0 s)))
    (if (< open 0)
      (if (> (string-length s) 0) (list s) '())
      (let ((close (string-search-forwards "G>" (+ open 2) s)))
        (if (< close 0)
          ;; 截断保护不切半哨兵，不会走到；兜底丢弃残片，哨兵不外泄
          (list (substring s 0 open))
          (let* ((k (string->number (substring s (+ open 3) close)))
                 (head (substring s 0 open))
                 (tail (substring s (+ close 2) (string-length s)))
                ) ;
            (append (if (> (string-length head) 0) (list head) '())
              (list (list-ref store k))
              (ai-sentinel-split tail store)
            ) ;append
          ) ;let*
        ) ;if
      ) ;let
    ) ;if
  ) ;let
) ;define

;; 上下文文本（含哨兵）→ document stree：按行拆段（空行跳过），段内多个
;; 行内节点用 concat 连接，单节点段落直接用该节点

(define (ai-context->document text store)
  (let* ((lines (string-split text #\newline))
         (paras
           (filter (lambda (p) (not (null? p)))
             (map (lambda (ln) (ai-sentinel-split ln store)) lines)
           ) ;filter
         ) ;paras
        ) ;
    (if (null? paras)
      '(document "")
      (cons 'document
        (map
          (lambda (p) (if (null? (cdr p)) (car p) (cons 'concat p)))
          paras
        ) ;map
      ) ;cons
    ) ;if
  ) ;let*
) ;define

(tm-define (ai-selection-context)
  (:synopsis "选区上下文（AI 释义的引文1）的 document 树，公式保留为子树"
  ) ;:synopsis
  ;; ghost 式字符预算窗口：选区前最多 ai-context-before-limit 字节 + 选区
  ;; 本身 + 选区后最多 ai-context-after-limit 字节。跨段落自然延伸、可在
  ;; 段落中间截断；选区嵌在引文1 中段，「引文2是引文1的一部分」成立。
  ;; 路径剥掉 buffer 前缀后对正文 stree 切分（同 ghost-relative-path）；
  ;; 展平文本中的公式哨兵在重组时换回子树（引用块里渲染成真实公式）
  (ai-formula-store-reset!)
  (let* ((body (tm->stree (buffer-get-body (current-buffer))))
         (bp (buffer-path))
         (rel
           (lambda (p) (if (>= (length p) (length bp)) (list-tail p (length bp)) p))
         ) ;rel
         (at-start (ai-split-node body (rel (selection-get-start))))
         (at-end (ai-split-node body (rel (selection-get-end))))
         (before (ai-herk-tail (car at-start) ai-context-before-limit))
         (after (ai-herk-head (cdr at-end) ai-context-after-limit))
         (middle (ai-flatten-text (tm->stree (selection-tree))))
        ) ;
    (stree->tree (ai-context->document (tm-string-trim-both (string-append before middle after))
                   ai-formula-store
                 ) ;ai-context->document
    ) ;stree->tree
  ) ;let*
) ;tm-define
