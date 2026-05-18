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

(define chat-tab-session-serial 0)

(define chat-tab-session-states (make-ahash-table))

(define (chat-tab-state input-buffer model session-id)
  (list input-buffer model session-id)
) ;define

(define (chat-tab-state-input-buffer st)
  (list-ref st 0)
) ;define

(define (chat-tab-state-model st)
  (list-ref st 1)
) ;define

(define (chat-tab-state-session-id st)
  (list-ref st 2)
) ;define

(define (chat-tab-set-state! message-buffer st)
  (ahash-set! chat-tab-session-states message-buffer st)
) ;define

(define (chat-tab-get-state message-buffer)
  (ahash-ref chat-tab-session-states message-buffer)
) ;define

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

(define (var-tree-children t)
  (with r (tree-children t) (if (and (nnull? r) (tree-empty? (cAr r))) (cDr r) r))
) ;define

(define (chat-tab-message-document message-buffer)
  (with-buffer message-buffer
    (let ((doc (buffer-get-body message-buffer)))
      (if (tree-is? doc 'document)
        doc
        (begin
          (buffer-set-body message-buffer '(document ""))
          (buffer-pretend-saved message-buffer)
          (buffer-get-body message-buffer)
        ) ;begin
      ) ;if
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
           (prefix (if (> (tree-arity doc) 0) (list "") '()))
           (payload (append prefix
                      (list '(with "font-series" "bold" "User:"))
                      (chat-tab-body-children body)
                      (list "" '(with "font-series" "bold" "Assistant:") '(output (document "")))
                    ) ;append
           ) ;payload
          ) ;
      (tree-insert! doc (tree-arity doc) payload)
      (set-user-active #f)
      (buffer-pretend-saved message-buffer)
      (let ((out-node (tree-ref doc :last)))
        (and (tree-is? out-node 'output) (tree-ref out-node 0))
      ) ;let
    ) ;let*
  ) ;with-buffer
) ;define

(define (chat-tab-next-session-id model)
  (set! chat-tab-session-serial (+ chat-tab-session-serial 1))
  (string-append model
    ":chat-tab:"
    (number->string (texmacs-time))
    "-"
    (number->string chat-tab-session-serial)
  ) ;string-append
) ;define

(define (chat-tab-ensure-session! message-buffer input-buffer)
  (let ((st (chat-tab-get-state message-buffer)))
    (if st
      (if (== (chat-tab-state-input-buffer st) input-buffer)
        st
        (let ((updated (chat-tab-state input-buffer
                         (chat-tab-state-model st)
                         (chat-tab-state-session-id st)
                       ) ;chat-tab-state
              ) ;updated
             ) ;
          (chat-tab-set-state! message-buffer updated)
          updated
        ) ;let
      ) ;if
      (let* ((model (or chat-tab-current-model "default"))
             (ses (chat-tab-next-session-id model))
             (new (chat-tab-state input-buffer model ses))
            ) ;
        (session-enable-text-input chat-tab-session-name ses)
        (chat-tab-set-state! message-buffer new)
        new
      ) ;let*
    ) ;if
  ) ;let
) ;define

;; chat-tab-session-encode
;; 将聊天标签会话的上下文编码为一个列表，用于传递给 plugin-feed。
;;
;; 语法
;; ----
;; (chat-tab-session-encode input message-buffer input-buffer out opts)
;;
;; 参数
;; ----
;; input : tree
;; 规范化后的输入消息树。
;;
;; message-buffer : url
;; 消息缓冲区名称。
;;
;; input-buffer : url
;; 输入缓冲区名称。
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
;;   ((do notify next cancel) input message-buffer input-buffer tree-pointer opts)
;; 其中第一个元素是四个回调函数的列表，out 被转换为 tree-pointer 以避免
;; 在异步执行期间被垃圾回收。
;;
;; 注意
;; ----
;; 此函数与 chat-tab-session-decode 成对使用，负责在任务进入 pending
;; 队列前将上下文打包。

(define (chat-tab-session-encode input message-buffer input-buffer out opts)
  (list (list chat-tab-session-do
          chat-tab-session-notify
          chat-tab-session-next
          chat-tab-session-cancel
        ) ;list
    input
    message-buffer
    input-buffer
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
;;   (input message-buffer input-buffer out opts)
;; 其中 out 由 tree-pointer 还原为 tree。
;;
;; 注意
;; ----
;; 返回值通常通过 with 绑定解构为多个变量使用。
;; 此函数与 chat-tab-session-encode 成对使用。

(define (chat-tab-session-decode l)
  (list (second l) (third l) (fourth l) (tree-pointer->tree (fifth l)) (sixth l))
) ;define

(define (chat-tab-session-detach l)
  (tree-pointer-detach (fifth l))
) ;define

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
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 逻辑
;; ----
;; 1. 获取当前 (lan ses) 的待处理队列 l。
;; 2. 若队列非空：
;;    a. 解码首元素，获取 input、message-buffer 等上下文。
;;    b. 若 input 为空树，调用 plugin-next 跳过当前任务。
;;    c. 否则，调用 plugin-write 将 input 以 :session 模式写入插件。
;;
;; 注意
;; ----
;; 此函数作为 do 回调传递给 plugin-feed，由 plugin-do 在任务开始执行时调用。
;; 当输入为空时直接跳过，避免向插件发送无意义请求。

(define (chat-tab-session-do lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input message-buffer input-buffer out opts)
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
;; 清理输出区域的 script-busy 标记，并分离当前任务的 tree-pointer。
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
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 逻辑
;; ----
;; 1. 获取当前 (lan ses) 的待处理队列 l。
;; 2. 若队列非空：
;;    a. 解码首元素，获取 message-buffer、out 等上下文。
;;    b. 在 message-buffer 中检查 out 的最后一个节点是否为 script-busy，
;;       若是则移除该节点。
;;    c. 标记 message-buffer 为已保存。
;;    d. 分离当前任务的 tree-pointer。
;;
;; 注意
;; ----
;; 此函数作为 next 回调传递给 plugin-feed，由 plugin-next 在任务完成后调用。
;; 主要职责是结束输出区域的忙等状态并释放 tree-pointer 资源。

(define (chat-tab-session-next lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input message-buffer input-buffer out opts)
        (chat-tab-session-decode (car l))
        (with-buffer message-buffer
          (when (and (tm-func? out 'document)
                  (> (tree-arity out) 0)
                  (tm-func? (tree-ref out :last) 'script-busy)
                ) ;and
            (tree-remove! out (- (tree-arity out) 1) 1)
          ) ;when
          (buffer-pretend-saved message-buffer)
        ) ;with-buffer
        (chat-tab-session-detach (car l))
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
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 逻辑
;; ----
;; 1. 获取当前 (lan ses) 的待处理队列 l。
;; 2. 若队列非空，解码首元素获取上下文。
;; 3. 根据通道 ch 分派处理：
;;    - "output" : 调用 chat-tab-output 将 t 追加到输出区域。
;;    - "error"  : 调用 chat-tab-errput 将 t 追加到错误输出区域。
;;    - "prompt" : 无操作。
;;    - "input"  : 若队列中仅剩当前任务，将 t 设置到输入缓冲区。
;; 4. 每次写入后标记 message-buffer 为已保存。
;;
;; 注意
;; ----
;; 此函数作为 notify 回调传递给 plugin-feed，由插件连接层异步调用。
;; "input" 通道仅在队列为单任务时更新输入缓冲区，避免多任务竞争。

(define (chat-tab-session-notify lan ses ch t)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input message-buffer input-buffer out opts)
        (chat-tab-session-decode (car l))
        (cond ((== ch "output")
               (with-buffer message-buffer
                 (chat-tab-output out t)
                 (buffer-pretend-saved message-buffer)
               ) ;with-buffer
              ) ;
              ((== ch "error")
               (with-buffer message-buffer
                 (chat-tab-errput out t)
                 (buffer-pretend-saved message-buffer)
               ) ;with-buffer
              ) ;
              ((== ch "prompt") (noop))
              ((and (== ch "input") (null? (cdr l)))
               (chat-tab-set-input-body! input-buffer t)
              ) ;
        ) ;cond
      ) ;with
    ) ;when
  ) ;with
) ;define

;; chat-tab-session-cancel
;; 聊天标签会话的任务取消回调。
;; 将输出区域的 script-busy 标记替换为 script-dead 或 script-interrupted，
;; 并分离当前任务的 tree-pointer。
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
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 逻辑
;; ----
;; 1. 获取当前 (lan ses) 的待处理队列 l。
;; 2. 若队列非空：
;;    a. 解码首元素，获取 message-buffer、out 等上下文。
;;    b. 在 message-buffer 中检查 out 的最后一个节点是否为 script-busy，
;;       若是则根据 dead? 替换为 script-dead 或 script-interrupted。
;;    c. 标记 message-buffer 为已保存。
;;    d. 分离当前任务的 tree-pointer。
;;
;; 注意
;; ----
;; 此函数作为 cancel 回调传递给 plugin-feed，由 plugin-cancel 在任务被取消
;; 或插件进程异常终止时调用。

(define (chat-tab-session-cancel lan ses dead?)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input message-buffer input-buffer out opts)
        (chat-tab-session-decode (car l))
        (with-buffer message-buffer
          (when (and (tm-func? out 'document)
                  (> (tree-arity out) 0)
                  (tm-func? (tree-ref out :last) 'script-busy)
                ) ;and
            (tree-assign (tree-ref out :last)
              (if dead? '(script-dead) '(script-interrupted))
            ) ;tree-assign
          ) ;when
          (buffer-pretend-saved message-buffer)
        ) ;with-buffer
        (chat-tab-session-detach (car l))
      ) ;with
    ) ;when
  ) ;with
) ;define

;; chat-tab-session-feed
;; 将用户输入投递到插件会话的待处理队列，并标记输出区域为忙等状态。
;;
;; 语法
;; ----
;; (chat-tab-session-feed lan ses input message-buffer input-buffer out opts)
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
;; message-buffer : url
;; 消息缓冲区名称。
;;
;; input-buffer : url
;; 输入缓冲区名称。
;;
;; out : tree
;; 输出节点树，将被重置为 (document (script-busy))。
;;
;; opts : list
;; 附加选项列表。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。
;;
;; 逻辑
;; ----
;; 1. 调用 plugin-preprocess 对 input 进行预处理。
;; 2. 在 message-buffer 中将 out 重置为 (document (script-busy))，表示开始忙等。
;; 3. 调用 chat-tab-session-encode 编码上下文为 x。
;;    - (car x) 为回调列表 (do notify next cancel)。
;;    - (cdr x) 为附加参数列表 (input message-buffer input-buffer tree-pointer opts)。
;; 4. 通过 apply 调用 plugin-feed，将任务加入 pending 队列。
;;    若队列为空则立即开始执行。
;;
;; 注意
;; ----
;; 此函数是聊天标签会话向底层插件引擎提交任务的唯一入口。
;; out 被重置为 script-busy 后，用户界面会显示忙等指示，直到任务完成。

(define (chat-tab-session-feed lan ses input message-buffer input-buffer out opts)
  (set! input (plugin-preprocess lan ses input opts))
  (with-buffer message-buffer (tree-assign! out '(document (script-busy))))
  (with x
    (chat-tab-session-encode input message-buffer input-buffer out opts)
    (apply plugin-feed `(,lan ,ses ,@(car x) ,(cdr x)))
  ) ;with
) ;define

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
;; 当前选中的模型名称（全局变量 chat-tab-current-model）。
;;
;; 逻辑
;; ----
;; 1. 检查 model 非空
;;    - 若非空，将 chat-tab-current-model 设置为该模型
;; 2. 返回 chat-tab-current-model
;;
;; 注意
;; ----
;; 此函数仅影响后续新建的会话，不会更改已有会话的模型。
(tm-define (chat-tab-session-select-model model)
  (:synopsis "Select the model used for new chat tab sessions")
  (:argument model "Model")
  (when (and model (!= model ""))
    (set! chat-tab-current-model model)
  ) ;when
  chat-tab-current-model
) ;tm-define

;; chat-tab-session-send
;; 通过聊天标签会话发送用户消息。
;;
;; 语法
;; ----
;; (chat-tab-session-send message-buffer input-buffer body)
;;
;; 参数
;; ----
;; message-buffer : url
;; 消息缓冲区名称（URL），用于存储对话历史。
;;
;; input-buffer : url
;; 输入缓冲区名称（URL），用户在此输入消息。
;;
;; body : tree
;; 输入消息树，即用户要发送的内容。
;;
;; 返回值
;; ----
;; boolean
;; - #t : 消息发送成功
;; - #f : 消息为空，未发送
;;
;; 逻辑
;; ----
;; 1. 检查 body 是否为空
;;    - 若为空，返回 #f
;; 2. 规范化输入文档
;; 3. 确保会话存在（chat-tab-ensure-session!）
;; 4. 在消息缓冲区追加一轮对话（chat-tab-append-round!）
;;    - 若追加失败，返回 #f
;; 5. 清空输入缓冲区
;; 6. 检查是否已定义 chat-tab-session-name 连接
;;    - 若未定义，直接将输入回显到输出区域
;;    - 若已定义，通过 plugin 机制发送消息（chat-tab-session-feed）
;; 7. 返回 #t
;;
;; 注意
;; ----
;; 此函数是聊天标签会话的核心发送入口，负责消息格式化、会话管理和插件交互。
(tm-define (chat-tab-session-send message-buffer input-buffer body)
  (:synopsis "Send user message through chat tab session")
  (:argument message-buffer "Message buffer name")
  (:argument input-buffer "Input buffer name")
  (:argument body "Input message tree")
  (if (chat-tab-empty-body? body)
    #f
    (let* ((input (chat-tab-normalize-document body))
           (st (chat-tab-ensure-session! message-buffer input-buffer))
           (ses (chat-tab-state-session-id st))
           (out (chat-tab-append-round! message-buffer input))
          ) ;
      (if (not out)
        #f
        (begin
          (chat-tab-clear-input! input-buffer)
          (if (not (connection-defined? chat-tab-session-name))
            (begin
              (with-buffer message-buffer
                (chat-tab-output out input)
                (buffer-pretend-saved message-buffer)
              ) ;with-buffer
              #t
            ) ;begin
            (begin
              (chat-tab-session-feed chat-tab-session-name
                ses
                input
                message-buffer
                input-buffer
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
) ;tm-define
