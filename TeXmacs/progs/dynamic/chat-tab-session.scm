;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-tab-session.scm
;; DESCRIPTION : Session engine for chat tab LLM integration
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic chat-tab-session)
  (:use (utils library tree)
    (utils library cursor)
    (utils plugins plugin-eval)
    (utils edit variants)
    (dynamic session-edit)
    (kernel texmacs tm-plugins)
    (texmacs texmacs tm-files)
  ) ;:use
) ;texmacs-module

(import (liii njson))

(define chat-tab-session-name "llm")

(define chat-tab-current-model "Kimi-VLM")

(define chat-tab-session-states (make-ahash-table))

(define chat-tab-style-loaded (make-ahash-table))

(define (chat-tab-style-loaded? session-id)
  (ahash-ref chat-tab-style-loaded session-id)
) ;define

(define (chat-tab-set-style-loaded! session-id)
  (ahash-set! chat-tab-style-loaded session-id #t)
) ;define

;;; ---------- Buffer URL 推导函数 ----------

(tm-define (chat-tab-session->message-buffer session-id)
  (string->url (string-append "tmfs://chat-message-" session-id))
) ;tm-define

(tm-define (chat-tab-session->input-buffer session-id)
  (string->url (string-append "tmfs://chat-input-" session-id))
) ;tm-define

;;; ---------- State 构造器和访问器 ----------

(tm-define (chat-tab-state model . opts)
  (let ((thinking (if (and (pair? opts) (car opts)) (car opts) "disabled")))
    (list model thinking)
  ) ;let
) ;tm-define

(define (chat-tab-state-model st)
  (list-ref st 0)
) ;define

(define (chat-tab-state-thinking st)
  (if (>= (length st) 2) (list-ref st 1) "disabled")
) ;define

(define (chat-tab-state->plugin-session-id st session-id)
  (string-append (chat-tab-state-model st) ":chat-tab:" session-id)
) ;define

(tm-define (chat-tab-set-state! session-id st)
  (ahash-set! chat-tab-session-states session-id st)
) ;tm-define

(tm-define (chat-tab-get-state session-id)
  (ahash-ref chat-tab-session-states session-id)
) ;tm-define

;; chat-tab-session-set-thinking
;; 设置会话的推理模式开关，并持久化到 manifest。
;;
;; 语法
;; ----
;; (chat-tab-session-set-thinking session-id thinking)
;;
;; 参数
;; ----
;; session-id : string
;;   会话 UUID。
;; thinking : string
;;   "enabled" 或 "disabled"。

(tm-define (chat-tab-session-set-thinking session-id thinking)
  (:synopsis "Set thinking mode for a chat session")
  (:argument session-id "Session UUID")
  (:argument thinking "Thinking mode: enabled or disabled")
  (let ((st (chat-tab-get-state session-id)))
    (when st
      (chat-tab-set-state! session-id
        (chat-tab-state (chat-tab-state-model st) thinking)
      ) ;chat-tab-set-state!
    ) ;when
  ) ;let
) ;tm-define

;;; ---------- 文档处理工具 ----------

(define (chat-tab-normalize-document body)
  (cond ((tree? body)
         (if (tree-is? body 'document)
           body
           (stree->tree `(document ,(tree->stree body)))
         ) ;if
        ) ;
        ((and (pair? body) (eq? (car body) 'document)) (stree->tree body))
        (else (stree->tree `(document ,body)))
  ) ;cond
) ;define

(define (chat-tab-flatten-stree x)
  (cond ((string? x) x)
        ((pair? x) (apply string-append (map chat-tab-flatten-stree (cdr x))))
        (else "")
  ) ;cond
) ;define

(define (chat-tab-empty-body? body)
  (== (string-trim-spaces (chat-tab-flatten-stree (tree->stree (chat-tab-normalize-document body)))
      ) ;string-trim-spaces
    ""
  ) ;==
) ;define

(define (chat-tab-body-children body)
  (map tree-copy (tree-children (chat-tab-normalize-document body)))
) ;define

;; chat-tab-buffer-empty?
;; 检查 buffer body 是否为空（无对话历史）。

(define (chat-tab-buffer-empty? body)
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
) ;define

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

;; chat-tab-tree->plain-text
;; 将 TeXmacs 树转为纯文本，直接提取字符串（不经过 LaTeX 管线，保留中文等 Unicode）。
;;
;; 语法
;; ----
;; (chat-tab-tree->plain-text t)
;;
;; 参数
;; ----
;; t : tree 或 stree
;; 待转换的 TeXmacs 树。
;;
;; 返回值
;; ----
;; string
;; 纯文本表示。

(define (chat-tab-tree->plain-text t)
  (let ((s (if (tree? t) (tree->stree t) t)))
    (cond ((string? s) (cork->utf8 s))
          ((null? s) "")
          ((number? s) (number->string s))
          ((symbol? s) "")
          ((not (pair? s)) "")
          ((eq? (car s) 'document)
           (chat-tab-join-nonempty (map chat-tab-tree->plain-text (cdr s)) "\n")
          ) ;
          ((eq? (car s) 'concat)
           (apply string-append (map chat-tab-tree->plain-text (cdr s)))
          ) ;
          ((eq? (car s) 'new-line) "\n")
          ((eq? (car s) 'folded-explain) "")
          ((eq? (car s) 'unfolded-explain) "")
          ((eq? (car s) 'image) "filtered image")
          ((eq? (car s) 'with)
           ;; (with var1 val1 ... body) → 只取 body
           (if (null? (cdr s)) "" (chat-tab-tree->plain-text (car (reverse s))))
          ) ;
          (else (apply string-append (map chat-tab-tree->plain-text (cdr s))))
    ) ;cond
  ) ;let
) ;define

;; chat-tab-tree-has-image?
;; 递归检测 tree/stree 中是否包含 (image ...) 元素。
;;
;; 语法
;; ----
;; (chat-tab-tree-has-image? t)
;;
;; 参数
;; ----
;; t : tree 或 stree
;; 待检测的 TeXmacs 树。
;;
;; 返回值
;; ----
;; boolean
;; #t 表示包含图片元素。

(define (chat-tab-tree-has-image? t)
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
) ;define

;; tree-contains-label?
;; 递归检查 tree 中是否包含指定 label 的节点

(define (tree-contains-label? t label)
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
) ;define

;; tree-remove-label-from-children!
;; 从 document 的直接子节点和 concat 子节点中移除指定 label 的节点

(define (tree-remove-label-from-children! t label)
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
) ;define

;; tree-extract-reasoning-delta!
;; 从 tree 中递归提取所有 reasoning-delta 节点的文本，并清除这些节点
;; 返回提取的文本字符串

(define (tree-extract-reasoning-delta! t)
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
) ;define

;; chat-tab-join-nonempty
;; 用分隔符连接非空字符串列表。
;;
;; 语法
;; ----
;; (chat-tab-join-nonempty strs sep)
;;
;; 参数
;; ----
;; strs : list of string
;; sep : string
;;
;; 返回值
;; ----
;; string

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

;; chat-tab-find-session
;; 在 body 树中查找 session 节点（处理 TMU 加载后 body 为 document 包裹的情况）。

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

(define (chat-tab-message-document message-buffer)
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
) ;define

(define (chat-tab-output t u)
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
) ;define

(define (chat-tab-errput t u)
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
) ;define

;; chat-tab-find-last-unfolded-explain
;; 从 out 的位置 i-1 向前搜索 unfolded-explain
;; 支持直接子节点和 concat 内包裹的情况

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

(define (chat-tab-append-reasoning! out text)
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
) ;define

(define (chat-tab-fold-last-explain! out)
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
) ;define

(define (chat-tab-clear-input! input-buffer)
  (with-buffer input-buffer
    (buffer-set-body input-buffer '(document ""))
    (buffer-pretend-saved input-buffer)
  ) ;with-buffer
) ;define

(define (chat-tab-set-input-body! input-buffer body)
  (with-buffer input-buffer
    (buffer-set-body input-buffer (chat-tab-normalize-document body))
    (buffer-pretend-saved input-buffer)
  ) ;with-buffer
) ;define

(define (chat-tab-append-round! message-buffer body session-id)
  (with-buffer message-buffer
    (let* ((doc (chat-tab-message-document message-buffer))
           (st (chat-tab-ensure-session! session-id))
           (model (chat-tab-state-model st))
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
) ;define

;;; ---------- 会话管理 ----------

(tm-define (chat-tab-add-default-style-packages!)
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
  (when (url-exists? (url-unix "$TEXMACS_STYLE_PATH" (string-append chat-tab-session-name ".ts"))
        ) ;url-exists?
    (add-style-package chat-tab-session-name)
  ) ;when
) ;tm-define

(tm-define (chat-tab-sync-dark-style! session-id)
  ;; 在 llm 样式包之后加载暗色主题，确保 dark 中的覆盖值生效
  (when (== (get-preference "gui theme") "liii-night")
    (let ((msg-buf (chat-tab-session->message-buffer session-id))
          (in-buf (chat-tab-session->input-buffer session-id))
         ) ;
      (with-buffer msg-buf
        (when (not (has-style-package? "dark"))
          (add-style-package "dark")
        ) ;when
      ) ;with-buffer
      (with-buffer in-buf
        (when (not (has-style-package? "dark"))
          (add-style-package "dark")
        ) ;when
      ) ;with-buffer
    ) ;let
  ) ;when
) ;tm-define

(define (chat-tab-ensure-session! session-id)
  ;; Step 1: 确保 state 存在
  (let ((st (chat-tab-get-state session-id)))
    (when (not st)
      (let* ((model (or chat-tab-current-model "Kimi-VLM"))
             (plugin-ses (string-append model ":chat-tab:" session-id))
             (new (chat-tab-state model))
            ) ;
        (session-enable-text-input chat-tab-session-name plugin-ses)
        (chat-tab-set-state! session-id new)
      ) ;let*
    ) ;when
  ) ;let
  ;; Step 2: 确保初始化完成（幂等保护）
  (let ((st (chat-tab-get-state session-id)))
    (if (chat-tab-style-loaded? session-id)
      st
      (let* ((plugin-ses (chat-tab-state->plugin-session-id st session-id))
             (msg-buf (chat-tab-session->message-buffer session-id))
             (in-buf (chat-tab-session->input-buffer session-id))
            ) ;
        (with-buffer msg-buf
          (let ((body (buffer-get-body msg-buf)))
            (when (chat-tab-buffer-empty? body)
              (buffer-set-body msg-buf
                `(session ,chat-tab-session-name ,plugin-ses (document))
              ) ;buffer-set-body
              (buffer-pretend-saved msg-buf)
            ) ;when
          ) ;let
          (chat-tab-add-default-style-packages!)
        ) ;with-buffer
        (with-buffer in-buf (chat-tab-add-default-style-packages!))
        ;; dark 必须在 llm 之后加载，确保覆盖值生效
        (chat-tab-sync-dark-style! session-id)
        (chat-tab-set-style-loaded! session-id)
        st
      ) ;let*
    ) ;if
  ) ;let
) ;define

;;; ---------- 编码/解码 ----------

;; chat-tab-session-encode
;; 将聊天标签会话的上下文编码为一个列表，用于传递给 plugin-feed。
;;
;; 语法
;; ----
;; (chat-tab-session-encode input session-id out opts)
;;
;; 参数
;; ----
;; input : tree
;; 规范化后的输入消息树。
;;
;; session-id : string
;; 会话 UUID，从中可推导出 message-buffer 和 input-buffer。
;;
;; out : tree
;; 输出节点树。
;;
;; opts : list
;; 附加选项列表。
;;
;; 返回值
;; ----
;; list
;; 编码后的列表，结构为：
;;   ((do notify next cancel) input session-id tree-pointer opts)
;; 其中第一个元素是四个回调函数的列表，out 被转换为 tree-pointer 以避免
;; 在异步执行期间被垃圾回收。

(define (chat-tab-session-encode input session-id out opts)
  (list (list chat-tab-session-do
          chat-tab-session-notify
          chat-tab-session-next
          chat-tab-session-cancel
        ) ;list
    input
    session-id
    (tree->tree-pointer out)
    opts
  ) ;list
) ;define

;; chat-tab-session-decode
;; 将 chat-tab-session-encode 编码的列表解码回会话上下文。
;;
;; 语法
;; ----
;; (chat-tab-session-decode l)
;;
;; 参数
;; ----
;; l : list
;; 由 chat-tab-session-encode 编码的列表。
;;
;; 返回值
;; ----
;; list
;; 解码后的上下文列表：
;;   (input session-id out opts)
;; 其中 out 由 tree-pointer 还原为 tree。

(define (chat-tab-session-decode l)
  (list (second l) (third l) (tree-pointer->tree (fourth l)) (fifth l))
) ;define

(define (chat-tab-session-detach l)
  (tree-pointer-detach (fourth l))
) ;define

;;; ---------- 回调函数 ----------

;; chat-tab-session-do
;; 聊天标签会话的任务开始回调。
;; 解码 pending 队列首元素，并将输入消息写入插件会话。
;;
;; 语法
;; ----
;; (chat-tab-session-do lan ses)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。

(define (chat-tab-session-do lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input session-id out opts)
        (chat-tab-session-decode (car l))
        (if (tree-empty? input)
          (plugin-next lan ses)
          (begin
            (plugin-write lan ses input :session)
            (with p (plugin-prompt lan ses) (when (tree? p) (tree-set out :up 0 p)))
          ) ;begin
        ) ;if
      ) ;with
    ) ;when
  ) ;with
) ;define

;; chat-tab-session-next
;; 聊天标签会话的任务完成回调。
;; 清理输出区域的 script-busy 标记，分离当前任务的 tree-pointer，
;; 并通知 C++ 生成结束。
;;
;; 语法
;; ----
;; (chat-tab-session-next lan ses)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。

(define (chat-tab-session-next lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input session-id out opts)
        (chat-tab-session-decode (car l))
        (when (chat-tab-get-state session-id)
          (let ((msg-buf (chat-tab-session->message-buffer session-id)))
            (with-buffer msg-buf
              (when (and (tm-func? out 'document)
                      (> (tree-arity out) 0)
                      (tm-func? (tree-ref out :last) 'script-busy)
                    ) ;and
                (tree-remove! out (- (tree-arity out) 1) 1)
              ) ;when
              (buffer-pretend-saved msg-buf)
            ) ;with-buffer
          ) ;let
        ) ;when
        (chat-tab-session-detach (car l))
        ;; 通知 C++ 生成结束
        (exec-delayed (lambda () (qt-chat-tab-set-state session-id "idle")))
      ) ;with
    ) ;when
  ) ;with
) ;define

;; chat-tab-session-notify
;; 聊天标签会话的插件通知回调。
;; 根据通道类型处理插件返回的输出、错误、提示或输入数据。
;;
;; 语法
;; ----
;; (chat-tab-session-notify lan ses ch t)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。
;;
;; ch : string
;; 通知通道名称：
;; - "output" : 正常输出数据
;; - "error"  : 错误输出数据
;; - "prompt" : 提示信息
;; - "input"  : 输入数据
;;
;; t : tree
;; 通知携带的数据树。

(define (chat-tab-session-notify lan ses ch t)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input session-id out opts)
        (chat-tab-session-decode (car l))
        (when (chat-tab-get-state session-id)
          (let ((msg-buf (chat-tab-session->message-buffer session-id))
                (in-buf (chat-tab-session->input-buffer session-id))
               ) ;
            (cond ((== ch "output")
                   (cond
                     ;; t 包含 reasoning-delta → 提取并追加到 unfolded-explain
                     ;; 注意：t 可能同时包含 fold-explain-reasoning，需要一并处理
                     ((tree-contains-label? t 'reasoning-delta)
                      (with-buffer msg-buf
                        (let* ((text (tree-extract-reasoning-delta! t))
                               (has-fold? (tree-contains-label? t 'fold-explain-reasoning))
                              ) ;
                          (when has-fold?
                            (tree-remove-label-from-children! t 'fold-explain-reasoning)
                          ) ;when
                          ;; 输出 t 中剩余的非 reasoning 内容（如 unfolded-explain）
                          (when (> (tree-arity t) 0)
                            (chat-tab-output out t)
                          ) ;when
                          ;; 追加 reasoning 文本到 out 中的 unfolded-explain
                          (chat-tab-append-reasoning! out text)
                          ;; 如果同时有 fold 命令，折叠
                          (when has-fold?
                            (chat-tab-fold-last-explain! out)
                          ) ;when
                        ) ;let*
                        (buffer-pretend-saved msg-buf)
                      ) ;with-buffer
                     ) ;
                     ;; t 仅包含 fold-explain-reasoning → 直接折叠
                     ((tree-contains-label? t 'fold-explain-reasoning)
                      (with-buffer msg-buf
                        (chat-tab-fold-last-explain! out)
                        (buffer-pretend-saved msg-buf)
                      ) ;with-buffer
                     ) ;
                     ;; 正常输出
                     (else (with-buffer msg-buf (chat-tab-output out t) (buffer-pretend-saved msg-buf))
                     ) ;else
                   ) ;cond
                  ) ;
                  ((== ch "error")
                   (with-buffer msg-buf (chat-tab-errput out t) (buffer-pretend-saved msg-buf))
                  ) ;
                  ((== ch "prompt")
                   (with-buffer msg-buf
                     (tree-set out :up 0 (tree-copy t))
                     (buffer-pretend-saved msg-buf)
                   ) ;with-buffer
                  ) ;
                  ((and (== ch "input") (null? (cdr l))) (chat-tab-set-input-body! in-buf t))
            ) ;cond
          ) ;let
        ) ;when
      ) ;with
    ) ;when
  ) ;with
) ;define

;; chat-tab-session-cancel
;; 聊天标签会话的任务取消回调。
;; 将输出区域的 script-busy 标记替换为 script-dead 或 script-interrupted，
;; 分离当前任务的 tree-pointer，并通知 C++ 生成结束。
;;
;; 语法
;; ----
;; (chat-tab-session-cancel lan ses dead?)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。
;;
;; dead? : boolean
;; 取消原因标志：
;; - #t : 插件进程已死亡，替换为 script-dead。
;; - #f : 任务被用户中断，替换为 script-interrupted。

(define (chat-tab-session-cancel lan ses dead?)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input session-id out opts)
        (chat-tab-session-decode (car l))
        (when (chat-tab-get-state session-id)
          (let ((msg-buf (chat-tab-session->message-buffer session-id)))
            (with-buffer msg-buf
              (when (and (tm-func? out 'document)
                      (> (tree-arity out) 0)
                      (tm-func? (tree-ref out :last) 'script-busy)
                    ) ;and
                (tree-assign (tree-ref out :last) '(script-interrupted))
              ) ;when
              (buffer-pretend-saved msg-buf)
            ) ;with-buffer
          ) ;let
        ) ;when
        (chat-tab-session-detach (car l))
        ;; 通知 C++ 生成结束
        (exec-delayed (lambda () (qt-chat-tab-set-state session-id "idle")))
      ) ;with
    ) ;when
  ) ;with
) ;define

;;; ---------- 上下文构建 ----------

;; chat-tab-extract-rounds
;; 从 message buffer 提取所有已完成对话轮次的 (role . text) 列表。
;;
;; 语法
;; ----
;; (chat-tab-extract-rounds message-buffer)
;;
;; 参数
;; ----
;; message-buffer : url
;; 消息缓冲区 URL。
;;
;; 返回值
;; ----
;; list
;; (role . text) 对列表，按时间顺序排列。

(define (chat-tab-extract-rounds message-buffer)
  (with-buffer message-buffer
    (let ((doc (chat-tab-message-document message-buffer)))
      (let loop
        ((children (tree-children doc)) (rounds '()))
        (if (null? children)
          (chat-tab-merge-consecutive-rounds (reverse rounds))
          (let ((child (car children)))
            (if (tm-func? child 'unfolded-io-text 3)
              (let* ((user-text (chat-tab-tree->plain-text (tree-ref child 1)))
                     (asst-doc (tree-ref child 2))
                     (asst-text (chat-tab-tree->plain-text asst-doc))
                    ) ;
                (if (or (chat-tab-empty-body? asst-doc)
                      (string-null? (string-trim-spaces asst-text))
                    ) ;or
                  (loop (cdr children) (cons (cons "user" user-text) rounds))
                  (loop (cdr children)
                    (cons (cons "assistant" asst-text) (cons (cons "user" user-text) rounds))
                  ) ;loop
                ) ;if
              ) ;let*
              (loop (cdr children) rounds)
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;let
  ) ;with-buffer
) ;define

;; chat-tab-merge-consecutive-rounds
;; 合并连续相同 role 的消息。
;;
;; 语法
;; ----
;; (chat-tab-merge-consecutive-rounds rounds)
;;
;; 参数
;; ----
;; rounds : list of (role . text)
;; 按时间顺序排列的对话轮次列表。
;;
;; 返回值
;; ----
;; list of (role . text)
;; 合并后的对话轮次列表，不包含连续相同 role 的消息。

(define (chat-tab-merge-consecutive-rounds rounds)
  (let loop
    ((items rounds) (acc '()))
    (if (null? items)
      (reverse acc)
      (let ((curr (car items)))
        (if (null? acc)
          (loop (cdr items) (cons curr acc))
          (let ((prev (car acc)))
            (if (string=? (car prev) (car curr))
              (loop (cdr items)
                (cons (cons (car curr) (string-append (cdr prev) "\n" (cdr curr))) (cdr acc))
              ) ;loop
              (loop (cdr items) (cons curr acc))
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;if
  ) ;let
) ;define

;; chat-tab-rounds->json
;; 将 (role . text) 列表转为 JSON 字符串。
;;
;; 语法
;; ----
;; (chat-tab-rounds->json rounds)
;;
;; 参数
;; ----
;; rounds : list of (role . text)
;; 对话轮次列表。
;;
;; 返回值
;; ----
;; string
;; JSON 字符串，格式为 {"messages":[...]}。

(define (chat-tab-rounds->json rounds thinking)
  (let ((arr (string->njson "[]")))
    (for-each (lambda (pair)
                (let ((entry (string->njson "{}")))
                  (njson-set! entry "role" (car pair))
                  (njson-set! entry "content" (cdr pair))
                  (njson-append! arr entry)
                  (njson-free entry)
                ) ;let
              ) ;lambda
      rounds
    ) ;for-each
    (let ((obj (string->njson "{}")))
      (njson-set! obj "messages" arr)
      (when (and thinking (not (string=? thinking "default")) (not (string=? thinking "")))
        (njson-set! obj "thinking" thinking)
      ) ;when
      (let ((result (njson->string obj)))
        (njson-free arr)
        (njson-free obj)
        result
      ) ;let
    ) ;let
  ) ;let
) ;define

;; chat-tab-build-context-input
;; 构建 %context 命令树，包含完整对话历史。
;;
;; 语法
;; ----
;; (chat-tab-build-context-input session-id input)
;;
;; 参数
;; ----
;; session-id : string
;; 会话 UUID。
;;
;; input : tree
;; 当前用户输入树。
;;
;; 返回值
;; ----
;; tree
;; 包含 %context 命令的文档树。

(define (chat-tab-build-context-input session-id input)
  (let* ((msg-buf (chat-tab-session->message-buffer session-id))
         (rounds (chat-tab-extract-rounds msg-buf))
         (st (chat-tab-get-state session-id))
         (thinking (if st (chat-tab-state-thinking st) "disabled"))
         (json-str (chat-tab-rounds->json rounds thinking))
         (cmd (string-append "%context " (utf8->cork json-str)))
        ) ;
    (stree->tree `(document ,cmd))
  ) ;let*
) ;define

;;; ---------- Feed ----------

;; chat-tab-session-feed
;; 将用户输入投递到插件会话的待处理队列，并标记输出区域为忙等状态。
;;
;; 语法
;; ----
;; (chat-tab-session-feed lan ses input session-id out opts)
;;
;; 参数
;; ----
;; lan : string
;; 插件语言名称。
;;
;; ses : string
;; 会话标识字符串。
;;
;; input : tree
;; 规范化后的输入消息树。
;;
;; session-id : string
;; 会话 UUID。
;;
;; out : tree
;; 输出节点树，将被重置为 (document (script-busy))。
;;
;; opts : list
;; 附加选项列表。

(define (chat-tab-session-feed lan ses input session-id out opts)
  ;; 用完整上下文替换原始输入
  (set! input (chat-tab-build-context-input session-id input))
  (set! input (plugin-preprocess lan ses input opts))
  (with-buffer (chat-tab-session->message-buffer session-id)
    (tree-assign! out '(document (script-busy)))
  ) ;with-buffer
  ;; 通知 C++ 进入 Generating 状态，切换按钮为 Stop
  (chat-tab-notify-state session-id "generating")
  (with x
    (chat-tab-session-encode input session-id out opts)
    (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x)))
  ) ;with
) ;define

;;; ---------- 模型选择 ----------

;; chat-tab-session-select-model
;; 选择用于新聊天标签会话的模型。
;;
;; 语法
;; ----
;; (chat-tab-session-select-model model)
;;
;; 参数
;; ----
;; model : string
;; 模型名称字符串。非空时更新当前模型；为空时仅返回当前模型。
;;
;; 返回值
;; ----
;; string
;; 当前选中的模型名称。

(tm-define (chat-tab-session-select-model model)
  (:synopsis "Select the model used for new chat tab sessions")
  (:argument model "Model")
  (when (and model (!= model ""))
    (set! chat-tab-current-model model)
  ) ;when
  chat-tab-current-model
) ;tm-define

;;; ---------- 发送 ----------

;; chat-tab-session-send
;; 通过聊天标签会话发送用户消息。
;;
;; 语法
;; ----
;; (chat-tab-session-send session-id)
;;
;; 参数
;; ----
;; session-id : string
;; 会话 UUID，C++ 侧传入。
;;
;; 返回值
;; ----
;; boolean
;; - #t : 消息发送成功
;; - #f : 消息为空，未发送
;;
;; 逻辑
;; ----
;; 1. 从 input buffer 读取 body
;; 2. 检查 body 是否为空
;; 3. 确保会话存在（chat-tab-ensure-session!）
;; 4. 检测是否包含图片等非文本内容
;;    - 包含：过滤图片后追加到 message buffer，输出提示，不发给插件
;;    - 不包含：继续后续流程
;; 5. 在消息缓冲区追加一轮对话
;; 6. 清空输入缓冲区
;; 7. 通过 plugin 机制发送消息

(tm-define (chat-tab-session-send session-id)
  (:synopsis "Send user message through chat tab session")
  (:argument session-id "Session UUID")
  (let* ((in-buf (chat-tab-session->input-buffer session-id))
         (body (buffer-get-body in-buf))
        ) ;
    (if (chat-tab-empty-body? body)
      #f
      (let* ((input (chat-tab-normalize-document body))
             (msg-buf (chat-tab-session->message-buffer session-id))
             (st (chat-tab-ensure-session! session-id))
             (plugin-ses (chat-tab-state->plugin-session-id st session-id))
            ) ;
        (if (chat-tab-tree-has-image? input)
          ;; 包含图片等非文本内容：过滤图片，输出提示，不发给插件
          (begin
            (chat-tab-clear-input! in-buf)
            (let* ((plain (chat-tab-tree->plain-text input))
                   (filtered (stree->tree `(document ,plain)))
                   (out (chat-tab-append-round! msg-buf filtered session-id))
                  ) ;
              (if (not out)
                #f
                (begin
                  (with-buffer msg-buf
                    (chat-tab-output out
                      (stree->tree `(document (with ,"color"
                                                ,"dark grey"
                                                ,"font-shape"
                                                ,"italic"
                                                ,(utf8->cork "AI 聊天暂不支持图片等非文本内容，相关内容已过滤。")))
                      ) ;stree->tree
                    ) ;chat-tab-output
                    (buffer-pretend-saved msg-buf)
                  ) ;with-buffer
                  #t
                ) ;begin
              ) ;if
            ) ;let*
          ) ;begin
          ;; 纯文本内容：正常发送流程
          (let* ((out (chat-tab-append-round! msg-buf input session-id)))
            (if (not out)
              #f
              (begin
                (chat-tab-clear-input! in-buf)
                (if (not (connection-defined? chat-tab-session-name))
                  (begin
                    (with-buffer msg-buf (chat-tab-output out input) (buffer-pretend-saved msg-buf))
                    #t
                  ) ;begin
                  (begin
                    (chat-tab-session-feed chat-tab-session-name
                      plugin-ses
                      input
                      session-id
                      out
                      '()
                    ) ;chat-tab-session-feed
                    #t
                  ) ;begin
                ) ;if
              ) ;begin
            ) ;if
          ) ;let*
        ) ;if
      ) ;let*
    ) ;if
  ) ;let*
) ;tm-define

;;; ---------- 通知 C++ ----------

;; chat-tab-notify-state
;; 通知 C++ 侧会话生成状态变更。
;;
;; 语法
;; ----
;; (chat-tab-notify-state session-id state)
;;
;; 参数
;; ----
;; session-id : string
;; 会话 UUID。
;;
;; state : string
;; 新状态："idle" 或 "generating"。

(tm-define (chat-tab-notify-state session-id state)
  (:synopsis "Notify C++ that session generation state changed")
  (:argument session-id "Session UUID")
  (:argument state "New state: idle or generating")
  (exec-delayed (lambda () (qt-chat-tab-set-state session-id state)))
) ;tm-define

;;; ---------- 会话销毁 ----------

;; chat-tab-session-destroy
;; 销毁聊天标签会话并清理 Scheme 层状态。
;;
;; 语法
;; ----
;; (chat-tab-session-destroy session-id)
;;
;; 参数
;; ----
;; session-id : string
;;   会话 UUID。

(tm-define (chat-tab-session-destroy session-id)
  (:synopsis "Destroy a chat tab session and clean up state")
  (:argument session-id "Session UUID")
  (ahash-remove! chat-tab-session-states session-id)
) ;tm-define
