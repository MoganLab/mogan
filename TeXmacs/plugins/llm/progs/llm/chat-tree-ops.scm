;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-tree-ops.scm
;; DESCRIPTION : Stateless tree operations for chat tab
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (llm chat-tree-ops)
  (:use (utils library tree)
    (data latex)
    (utils edit variants)
    (texmacs texmacs tm-files)
    (utils library cursor)
  ) ;:use
) ;texmacs-module

;;; ---------- 文档处理工具 ----------

(tm-define (chat-tab-normalize-document body)
  (cond ((tree? body)
         (if (tree-is? body 'document)
           body
           (stree->tree `(document ,(tree->stree body)))
         ) ;if
        ) ;
        ((and (pair? body) (eq? (car body) 'document)) (stree->tree body))
        (else (stree->tree `(document ,body)))
  ) ;cond
) ;tm-define

(define (chat-tab-flatten-stree x)
  (cond ((string? x) x)
        ((pair? x) (apply string-append (map chat-tab-flatten-stree (cdr x))))
        (else "")
  ) ;cond
) ;define

(tm-define (chat-tab-empty-body? body)
  (== (string-trim-spaces (chat-tab-flatten-stree (tree->stree (chat-tab-normalize-document body)))
      ) ;string-trim-spaces
    ""
  ) ;==
) ;tm-define

(define (chat-tab-body-children body)
  (map tree-copy (tree-children (chat-tab-normalize-document body)))
) ;define

(tm-define (chat-tab-buffer-empty? body)
  (or (not body)
    (and (tree-is? body 'document) (== (tree-arity body) 0))
    (and (tree-is? body 'document)
      (== (tree-arity body) 1)
      (tree-empty? (tree-ref body 0))
    ) ;and
    (and (tree-is? body 'session)
      (let ((d (tree-ref body 2)))
        (or (not (tree-is? d 'document)) (== (tree-arity d) 0))
      ) ;let
    ) ;and
  ) ;or
) ;tm-define

(define (chat-tab-model-prompt model)
  (with parts
    (string-tokenize-by-char model #\-)
    (with part
      (list-find parts (lambda (p) (string-occurs? "0123456789" p)))
      (string-append (if part part (cAr parts)) "> ")
    ) ;with
  ) ;with
) ;define

(define (var-tree-children t)
  (with r (tree-children t) (if (and (nnull? r) (tree-empty? (cAr r))) (cDr r) r))
) ;define

;;; ---------- 文本转换 ----------

(tm-define (chat-tab-tree->plain-text t)
  (serialize-latex (texmacs->latex (tm->stree t) '()))
) ;tm-define

(tm-define (chat-tab-tree-has-image? t)
  (let ((s (if (tree? t) (tree->stree t) t)))
    (cond ((string? s) #f)
          ((not (pair? s)) #f)
          ((eq? (car s) 'image) #t)
          (else (let loop
                  ((rest (cdr s)))
                  (if (null? rest)
                    #f
                    (or (chat-tab-tree-has-image? (car rest)) (loop (cdr rest)))
                  ) ;if
                ) ;let
          ) ;else
    ) ;cond
  ) ;let
) ;tm-define

;;; ---------- 推理渲染 ----------

(tm-define (tree-contains-label? t label)
  (cond ((not (tree? t)) #f)
        ((eq? (tree-label t) label) #t)
        (else (let loop
                ((i 0) (n (tree-arity t)))
                (if (>= i n)
                  #f
                  (or (tree-contains-label? (tree-ref t i) label) (loop (+ i 1) n))
                ) ;if
              ) ;let
        ) ;else
  ) ;cond
) ;tm-define

(tm-define (tree-remove-label-from-children! t label)
  (when (tm-func? t 'document)
    (let loop
      ((i (- (tree-arity t) 1)))
      (when (>= i 0)
        (let ((child (tree-ref t i)))
          (cond ((eq? (tree-label child) label) (tree-remove! t i 1))
                ((tm-func? child 'concat)
                 (let sub-loop
                   ((j (- (tree-arity child) 1)))
                   (when (>= j 0)
                     (when (eq? (tree-label (tree-ref child j)) label)
                       (tree-remove! child j 1)
                     ) ;when
                     (sub-loop (- j 1))
                   ) ;when
                 ) ;let
                ) ;
                (else (noop))
          ) ;cond
        ) ;let
        (loop (- i 1))
      ) ;when
    ) ;let
  ) ;when
) ;tm-define

(tm-define (tree-extract-reasoning-delta! t)
  ;; 递归收集所有 reasoning-delta 的文本
  (define (collect node)
    (cond ((not (tree? node)) "")
          ((eq? (tree-label node) 'reasoning-delta)
           (if (> (tree-arity node) 0) (or (tree->stree (tree-ref node 0)) "") "")
          ) ;
          (else (let loop
                  ((i 0) (n (tree-arity node)) (acc '()))
                  (if (>= i n)
                    (apply string-append (reverse acc))
                    (loop (+ i 1) n (cons (collect (tree-ref node i)) acc))
                  ) ;if
                ) ;let
          ) ;else
    ) ;cond
  ) ;define

  (let ((text (collect t)))
    (tree-remove-label-from-children! t 'reasoning-delta)
    text
  ) ;let
) ;tm-define

(define (chat-tab-find-last-unfolded-explain out i)
  (let loop
    ((k (- i 1)))
    (if (< k 0)
      #f
      (let ((child (tree-ref out k)))
        (cond ((tm-func? child 'unfolded-explain) child)
              ((tm-func? child 'concat)
               (let sub-loop
                 ((j 0) (n (tree-arity child)))
                 (if (>= j n)
                   (loop (- k 1))
                   (if (tm-func? (tree-ref child j) 'unfolded-explain)
                     (tree-ref child j)
                     (sub-loop (+ j 1) n)
                   ) ;if
                 ) ;if
               ) ;let
              ) ;
              (else (loop (- k 1)))
        ) ;cond
      ) ;let
    ) ;if
  ) ;let
) ;define

(tm-define (chat-tab-append-reasoning! out text)
  (when (tm-func? out 'document)
    (with i
      (tree-arity out)
      ;; 跳过 script-busy
      (if (and (> i 0) (tm-func? (tree-ref out (- i 1)) 'script-busy))
        (set! i (- i 1))
      ) ;if
      ;; 找到 unfolded-explain（直接子节点或在 concat 内）
      (with ue
        (chat-tab-find-last-unfolded-explain out i)
        (when ue
          (with body
            (tree-ref ue 1)
            ;; 在 body(with) 的子节点中搜索 document
            (let doc-loop
              ((j 0))
              (when (< j (tree-arity body))
                (if (tm-func? (tree-ref body j) 'document)
                  (let* ((doc (tree-ref body j)) (content (if (tree? text) (tree->stree text) text)))
                    ;; 按 \n 拆分：首段追加到 doc 末尾，后续段落新增
                    (when (and (string? content) (not (string-null? content)))
                      (let* ((cork-parts (string-split (cork->utf8 content) #\newline))
                             (parts (map utf8->cork cork-parts))
                            ) ;
                        (when (nnull? parts)
                          ;; 追加第一段到 doc 最后一个子节点
                          (let ((last-idx (- (tree-arity doc) 1)))
                            (when (>= last-idx 0)
                              (tree-set doc
                                last-idx
                                (string-append (or (tree->stree (tree-ref doc last-idx)) "") (car parts))
                              ) ;tree-set
                            ) ;when
                          ) ;let
                          ;; 后续段落作为新子节点插入
                          (when (> (length parts) 1)
                            (tree-insert! doc (tree-arity doc) (cdr parts))
                          ) ;when
                        ) ;when
                      ) ;let*
                    ) ;when
                  ) ;let*
                  (doc-loop (+ j 1))
                ) ;if
              ) ;when
            ) ;let
          ) ;with
        ) ;when
      ) ;with
    ) ;with
  ) ;when
) ;tm-define

(tm-define (chat-tab-fold-last-explain! out)
  (when (tm-func? out 'document)
    (with i
      (tree-arity out)
      ;; 跳过 script-busy
      (if (and (> i 0) (tm-func? (tree-ref out (- i 1)) 'script-busy))
        (set! i (- i 1))
      ) ;if
      ;; 找到并折叠 unfolded-explain（直接子节点或在 concat 内）
      (with ue
        (chat-tab-find-last-unfolded-explain out i)
        (when ue
          (variant-set ue 'folded-explain)
        ) ;when
      ) ;with
    ) ;with
  ) ;when
) ;tm-define

;;; ---------- 输出渲染 ----------

(tm-define (chat-tab-output t u)
  (when (tm-func? t 'document)
    (with i
      (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy)) (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput)) (set! i (- i 1)))
      (when (tm-func? u 'document)
        (tree-insert! t i (var-tree-children u))
        (tree-go-to t :end)
      ) ;when
    ) ;with
  ) ;when
) ;tm-define

(tm-define (chat-tab-errput t u)
  (when (tm-func? t 'document)
    (with i
      (tree-arity t)
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'script-busy)) (set! i (- i 1)))
      (if (and (> i 0) (tm-func? (tree-ref t (- i 1)) 'errput))
        (set! i (- i 1))
        (tree-insert! t i '((errput (document))))
      ) ;if
      (chat-tab-output (tree-ref t i 0) u)
    ) ;with
  ) ;when
) ;tm-define

;;; ---------- 辅助函数 ----------

(define (chat-tab-join-nonempty strs sep)
  (let loop
    ((rest strs) (acc '()) (first? #t))
    (if (null? rest)
      (apply string-append (reverse acc))
      (let ((s (car rest)))
        (if (or (string-null? s) (string=? s "\n"))
          (loop (cdr rest) acc first?)
          (loop (cdr rest) (if first? (list s) (cons s (cons sep acc))) #f)
        ) ;if
      ) ;let
    ) ;if
  ) ;let
) ;define

(define (chat-tab-find-session body)
  (if (tree-is? body 'session)
    body
    (if (tree-is? body 'document)
      (let loop
        ((i 0))
        (if (>= i (tree-arity body))
          #f
          (if (tree-is? (tree-ref body i) 'session) (tree-ref body i) (loop (+ i 1)))
        ) ;if
      ) ;let
      #f
    ) ;if
  ) ;if
) ;define

(tm-define (chat-tab-message-document message-buffer)
  (with-buffer message-buffer
    (let ((doc (buffer-get-body message-buffer)))
      (cond ((tree-is? doc 'session)
             (with d (tree-ref doc 2) (if (tree-is? d 'document) d doc))
            ) ;
            ((tree-is? doc 'document)
             ;; body 为 document 时，查找其中的 session 节点
             (let ((sess (chat-tab-find-session doc)))
               (if sess (let ((d (tree-ref sess 2))) (if (tree-is? d 'document) d doc)) doc)
             ) ;let
            ) ;
            (else (buffer-set-body message-buffer '(document ""))
              (buffer-pretend-saved message-buffer)
              (buffer-get-body message-buffer)
            ) ;else
      ) ;cond
    ) ;let
  ) ;with-buffer
) ;tm-define

;;; ---------- 输入操作 ----------

(tm-define (chat-tab-clear-input! input-buffer)
  (with-buffer input-buffer
    (buffer-set-body input-buffer '(document ""))
    (buffer-pretend-saved input-buffer)
  ) ;with-buffer
) ;tm-define

(tm-define (chat-tab-set-input-body! input-buffer body)
  (with-buffer input-buffer
    (buffer-set-body input-buffer (chat-tab-normalize-document body))
    (buffer-pretend-saved input-buffer)
  ) ;with-buffer
) ;tm-define

;;; ---------- 追加对话轮次 ----------

(tm-define (chat-tab-append-round! message-buffer body model)
  (with-buffer message-buffer
    (let* ((doc (chat-tab-message-document message-buffer))
           (prompt (chat-tab-model-prompt model))
           (input-children (chat-tab-body-children body))
           (input-stree (map tree->stree input-children))
           (io-node (stree->tree `(unfolded-io-text (document ,prompt)
                                    (document ,@input-stree)
                                    (document ""))
                    ) ;stree->tree
           ) ;io-node
          ) ;
      (tree-insert! doc (tree-arity doc) (list io-node))
      (tree-go-to doc :end)
      (buffer-pretend-saved message-buffer)
      (let ((last-node (tree-ref doc :last)))
        (and (tree-is? last-node 'unfolded-io-text) (tree-ref last-node 2))
      ) ;let
    ) ;let*
  ) ;with-buffer
) ;tm-define

;;; ---------- 样式包管理 ----------

(tm-define (chat-tab-add-default-style-packages! session-name)
  ;; 偏好驱动，参考 buffer-set-default-style（tm-files.scm:130-146）
  (add-style-package "number-europe")
  (add-style-package "preview-ref")
  (with lan
    (get-preference "language")
    (when (!= lan "english")
      (set-document-language lan)
      ;; 中文等 CJK 语言自动加载对应样式包
      (when (== lan "chinese")
        (add-style-package "chinese")
        (add-style-package "table-captions-above")
      ) ;when
    ) ;when
  ) ;with
  ;; 插件样式包：动态检测，参考 session-edit 的 make-session
  (when (url-exists? (url-unix "$TEXMACS_STYLE_PATH" (string-append session-name ".ts"))
        ) ;url-exists?
    (add-style-package session-name)
  ) ;when
) ;tm-define
