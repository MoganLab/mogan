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
    (dynamic session-edit)
    (kernel texmacs tm-plugins)
    (texmacs texmacs tm-files)
  ) ;:use
) ;texmacs-module

(define chat-tab-session-name "llm")

(define chat-tab-current-model "default")

(define chat-tab-session-states (make-ahash-table))

;;; ---------- Buffer URL 推导函数 ----------

(tm-define (chat-tab-session->message-buffer session-id)
  (string->url (string-append "tmfs://chat-message-" session-id))
) ;tm-define

(define (chat-tab-session->input-buffer session-id)
  (string->url (string-append "tmfs://chat-input-" session-id))
) ;define

;;; ---------- State 构造器和访问器 ----------

(tm-define (chat-tab-state model)
  (list model)
) ;tm-define

(define (chat-tab-state-model st)
  (list-ref st 0)
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

(define (chat-tab-message-document message-buffer)
  (with-buffer message-buffer
    (let ((doc (buffer-get-body message-buffer)))
      (cond ((tree-is? doc 'document) doc)
            ((tree-is? doc 'session)
             (with d (tree-ref doc 2) (if (tree-is? d 'document) d doc))
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
        (set-user-active #f)
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

(define (chat-tab-append-round! message-buffer body)
  (with-buffer message-buffer
    (let* ((doc (chat-tab-message-document message-buffer))
           (model chat-tab-current-model)
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
      (set-user-active #f)
      (buffer-pretend-saved message-buffer)
      (let ((last-node (tree-ref doc :last)))
        (and (tree-is? last-node 'unfolded-io-text) (tree-ref last-node 2))
      ) ;let
    ) ;let*
  ) ;with-buffer
) ;define

;;; ---------- 会话管理 ----------

(define (chat-tab-ensure-session! session-id)
  (let ((st (chat-tab-get-state session-id)))
    (if st
      st
      (let* ((model (or chat-tab-current-model "default"))
             (plugin-ses (string-append model ":chat-tab:" session-id))
             (new (chat-tab-state model))
            ) ;
        (session-enable-text-input chat-tab-session-name plugin-ses)
        (chat-tab-set-state! session-id new)
        new
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
          (plugin-write lan ses input :session)
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
        (let ((msg-buf (chat-tab-session->message-buffer session-id))
              (in-buf (chat-tab-session->input-buffer session-id)))
          (cond ((== ch "output")
                 (with-buffer msg-buf
                   (chat-tab-output out t)
                   (buffer-pretend-saved msg-buf)
                 ) ;with-buffer
                ) ;
                ((== ch "error")
                 (with-buffer msg-buf
                   (chat-tab-errput out t)
                   (buffer-pretend-saved msg-buf)
                 ) ;with-buffer
                ) ;
                ((== ch "prompt") (noop))
                ((and (== ch "input") (null? (cdr l)))
                 (chat-tab-set-input-body! in-buf t)
                ) ;
          ) ;cond
        ) ;let
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
        (let ((msg-buf (chat-tab-session->message-buffer session-id)))
          (with-buffer msg-buf
            (when (and (tm-func? out 'document)
                    (> (tree-arity out) 0)
                    (tm-func? (tree-ref out :last) 'script-busy)
                  ) ;and
              (tree-assign (tree-ref out :last)
                (if dead? '(script-dead) '(script-interrupted))
              ) ;tree-assign
            ) ;when
            (buffer-pretend-saved msg-buf)
          ) ;with-buffer
        ) ;let
        (chat-tab-session-detach (car l))
        ;; 通知 C++ 生成结束
        (exec-delayed (lambda () (qt-chat-tab-set-state session-id "idle")))
      ) ;with
    ) ;when
  ) ;with
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
  (set! input (plugin-preprocess lan ses input opts))
  (with-buffer (chat-tab-session->message-buffer session-id)
    (tree-assign! out '(document (script-busy)))
  ) ;with-buffer
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
;; 4. 在消息缓冲区追加一轮对话
;; 5. 清空输入缓冲区
;; 6. 通过 plugin 机制发送消息

(tm-define (chat-tab-session-send session-id)
  (:synopsis "Send user message through chat tab session")
  (:argument session-id "Session UUID")
  (let* ((in-buf (chat-tab-session->input-buffer session-id))
         (body (buffer-get-body in-buf)))
    (if (chat-tab-empty-body? body)
      #f
      (let* ((input (chat-tab-normalize-document body))
             (msg-buf (chat-tab-session->message-buffer session-id))
             (st (chat-tab-ensure-session! session-id))
             (plugin-ses (chat-tab-state->plugin-session-id st session-id))
             (out (chat-tab-append-round! msg-buf input))
            ) ;
        (if (not out)
          #f
          (begin
            (chat-tab-clear-input! in-buf)
            (if (not (connection-defined? chat-tab-session-name))
              (begin
                (with-buffer msg-buf
                  (chat-tab-output out input)
                  (buffer-pretend-saved msg-buf)
                ) ;with-buffer
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
  (exec-delayed
    (lambda () (qt-chat-tab-set-state session-id state)))
) ;tm-define
