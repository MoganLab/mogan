
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-protocol.scm
;; DESCRIPTION : Protocol session layer and bridge functions
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (llm chat-protocol)
  (:use (llm chat-tree-ops)
    (utils library tree)
    (utils library cursor)
    (utils plugins plugin-eval)
    (utils edit variants)
    (dynamic session-edit)
    (kernel texmacs tm-plugins)
    (texmacs texmacs tm-files)
    (texmacs texmacs tm-server)
    (data latex)
  ) ;:use
) ;texmacs-module

(import (liii njson))

;;; ---------- 全局常量 ----------

(define chat-tab-session-name "llm")

;;; ---------- Buffer URL 推导函数 ----------

(tm-define (chat-tab-session->message-buffer session-id)
  (string->url (string-append "tmfs://chat/" session-id "/message"))
) ;tm-define

(tm-define (chat-tab-session->input-buffer session-id)
  (string->url (string-append "tmfs://chat/" session-id "/input"))
) ;tm-define

;;; ---------- Buffer 类型检测 ----------

(tm-define (chat-message-buffer? buf)
  (with s
    (url->system buf)
    (and (string-starts? s "tmfs://chat/") (string-contains? s "/message"))
  ) ;with
) ;tm-define

(tm-define (chat-input-buffer? buf)
  (with s
    (url->system buf)
    (and (string-starts? s "tmfs://chat/") (string-contains? s "/input"))
  ) ;with
) ;tm-define

(tm-define (chat-buffer-session-id buf)
  (with s
    (url->system buf)
    (if (not (string-starts? s "tmfs://chat/"))
      #f
      (let* ((rest (substring s (string-length "tmfs://chat/")))
             (i (string-index rest #\/))
            ) ;
        (if i (substring rest 0 i) #f)
      ) ;let*
    ) ;if
  ) ;with
) ;tm-define

;;; ---------- 会话初始化 ----------

(tm-define (chat-tab-init-session! session-id model)
  (:synopsis "Initialize chat tab session buffers and style packages")
  (:argument session-id "Session UUID")
  (:argument model "Model name")
  ;; model/thinking 状态由 C++ 管理，此处只做 buffer 初始化
  ;; plugin session ID 不再包含 model 前缀
  (let ((plugin-ses (string-append "chat-tab:" session-id)))
    ;; Step 1: 注册 text-input 模式
    (session-enable-text-input chat-tab-session-name plugin-ses)
    ;; Step 2: 初始化 buffer
    (let* ((msg-buf (chat-tab-session->message-buffer session-id))
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
        (chat-tab-add-default-style-packages! chat-tab-session-name)
      ) ;with-buffer
      (with-buffer in-buf
        (chat-tab-add-default-style-packages! chat-tab-session-name)
      ) ;with-buffer
    ) ;let*
  ) ;let
) ;tm-define

(tm-define (chat-tab-load-input-styles! session-id)
  (:synopsis "Load style packages for input buffer only (new conversation)")
  (:argument session-id "Session UUID")
  (let ((in-buf (chat-tab-session->input-buffer session-id)))
    (with-buffer in-buf
      (chat-tab-add-default-style-packages! chat-tab-session-name)
    ) ;with-buffer
  ) ;let
) ;tm-define

(tm-define (chat-tab-sync-dark-style! session-id)
  ;; C++ 侧创建 panel 后调用，同步暗色样式包
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

;;; ---------- 编码/解码 ----------

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

(define (chat-tab-session-decode l)
  (list (second l) (third l) (tree-pointer->tree (fourth l)) (fifth l))
) ;define

(define (chat-tab-session-detach l)
  (tree-pointer-detach (fourth l))
) ;define

;;; ---------- 回调函数 ----------

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

(define (chat-tab-session-notify lan ses ch t)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with (input session-id out opts)
        (chat-tab-session-decode (car l))
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
      ) ;with
    ) ;when
  ) ;with
) ;define

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
              (tree-assign (tree-ref out :last) '(script-interrupted))
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

;;; ---------- 上下文构建 ----------

(define (chat-tab-suffix->mime suffix)
  (cond ((== suffix "png") "image/png")
        ((or (== suffix "jpg") (== suffix "jpeg")) "image/jpeg")
        ((== suffix "gif") "image/gif")
        ((== suffix "webp") "image/webp")
        (else #f)
  ) ;cond
) ;define

(define (chat-tab-image-node->pair img-stree)
  ;; img-stree = (image <name> ...)
  ;; Returns (mime . base64-data) or #f
  (if (< (length img-stree) 2)
    #f
    (let ((name (cadr img-stree)))
      (cond
        ;; Embedded: (tuple (raw-data <base64>) <filename>)
        ((and (pair? name) (eq? (car name) 'tuple) (>= (length name) 3))
         (let ((data-node (cadr name)) (suffix-str (caddr name)))
           (let ((suffix (url-suffix suffix-str))
                 (mime (chat-tab-suffix->mime (url-suffix suffix-str)))
                ) ;
             (if (not mime)
               #f
               (cond
                 ;; raw-data format: data already base64
                 ((and (pair? data-node)
                    (>= (length data-node) 2)
                    (eq? (car data-node) 'raw-data)
                  ) ;and
                  (cons mime (cadr data-node))
                 ) ;
                 (else #f)
               ) ;cond
             ) ;if
           ) ;let
         ) ;let
        ) ;
        ;; Linked: string path — 需要读文件并 base64 编码
        ((string? name)
         ;; TODO: 需要加载 (liii base64) 后支持链接图片的 base64 编码
         #f
        ) ;
        (else #f)
      ) ;cond
    ) ;let
  ) ;if
) ;define

(define (chat-tab-collect-images s acc)
  (cond ((string? s) acc)
        ((not (pair? s)) acc)
        ((eq? (car s) 'image)
         (let ((img (chat-tab-image-node->pair s)))
           (if img (cons img acc) acc)
         ) ;let
        ) ;
        (else (let loop
                ((rest (cdr s)) (a acc))
                (if (null? rest) a (loop (cdr rest) (chat-tab-collect-images (car rest) a)))
              ) ;let
        ) ;else
  ) ;cond
) ;define

(define (chat-tab-build-context-input input session-id model thinking search)
  ;; 单轮：只编码当前用户输入 + per-round 参数
  ;; 线格式：%chat <json>\n<EOF>\n
  (let* ((content (chat-tab-tree->plain-text input))
         (obj (string->njson "{}"))
         (params (string->njson "{}"))
         (stree-input (if (tree? input) (tree->stree input) input))
         (images (chat-tab-collect-images stree-input '()))
        ) ;
    (njson-set! obj "sessionId" session-id)
    (njson-set! params "model" model)
    (njson-set! params "thinking" thinking)
    (njson-set! params "search" search)
    (njson-set! obj "params" params)
    (njson-set! obj "content" content)
    ;; 可选：有图片时加入 images 数组
    (when (pair? images)
      (let ((img-arr (string->njson "[]")))
        (for-each (lambda (img-pair)
                    (let ((img-obj (string->njson "{}")))
                      (njson-set! img-obj "mime" (car img-pair))
                      (njson-set! img-obj "data" (cdr img-pair))
                      (njson-append! img-arr img-obj)
                      (njson-free img-obj)
                    ) ;let
                  ) ;lambda
          images
        ) ;for-each
        (njson-set! obj "images" img-arr)
        (njson-free img-arr)
      ) ;let
    ) ;when
    (let ((json-str (njson->string obj)))
      (njson-free params)
      (njson-free obj)
      (let ((cork-json (utf8->cork json-str)))
        (stree->tree `(document ,(string-append "%chat " cork-json)))
      ) ;let
    ) ;let
  ) ;let*
) ;define

;;; ---------- Feed ----------

(define (chat-tab-session-feed lan ses input session-id out opts model thinking search)
  ;; 用单轮输入替换原始输入
  (set! input
    (chat-tab-build-context-input input session-id model thinking search)
  ) ;set!
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

;;; ---------- 发送 ----------

(tm-define (chat-tab-session-send session-id model thinking search)
  (:synopsis "Send user message through chat tab session")
  (:argument session-id "Session UUID")
  (:argument model "Model name")
  (:argument thinking "Thinking mode: enabled or disabled")
  (:argument search "Search mode: enabled or disabled")
  (let* ((in-buf (chat-tab-session->input-buffer session-id))
         (body (buffer-get-body in-buf))
        ) ;
    (if (chat-tab-empty-body? body)
      #f
      (let* ((input (chat-tab-normalize-document body))
             (msg-buf (chat-tab-session->message-buffer session-id))
            ) ;
        ;; 延迟初始化：首次发送时设置 session body 并加载样式包
        (let ((plugin-ses (string-append "chat-tab:" session-id)))
          (with-buffer msg-buf
            (let ((msg-body (buffer-get-body msg-buf)))
              (when (chat-tab-buffer-empty? msg-body)
                (session-enable-text-input chat-tab-session-name plugin-ses)
                (buffer-set-body msg-buf
                  `(session ,chat-tab-session-name ,plugin-ses (document))
                ) ;buffer-set-body
                (buffer-pretend-saved msg-buf)
                (chat-tab-add-default-style-packages! chat-tab-session-name)
              ) ;when
            ) ;let
          ) ;with-buffer
          (let* ((out (chat-tab-append-round! msg-buf input model)))
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
                      model
                      thinking
                      search
                    ) ;chat-tab-session-feed
                    #t
                  ) ;begin
                ) ;if
              ) ;begin
            ) ;if
          ) ;let*
        ) ;let
      ) ;let*
    ) ;if
  ) ;let*
) ;tm-define

;;; ---------- 通知 C++ ----------

(tm-define (chat-tab-notify-state session-id state)
  (:synopsis "Notify C++ that session generation state changed")
  (:argument session-id "Session UUID")
  (:argument state "New state: idle or generating")
  (exec-delayed (lambda () (qt-chat-tab-set-state session-id state)))
) ;tm-define

;;; ---------- 桥接函数（原 chat-adapter.scm） ----------

(define chat-tab-url (string->url "tmfs://chat-tab"))

(tm-define (open-llm-chat-tab . model-opt)
  (:synopsis "Open or switch to the LLM Chat tab")
  (if (buffer-exists? chat-tab-url)
    (switch-to-buffer chat-tab-url)
    (begin
      (buffer-set chat-tab-url '(document ""))
      (buffer-set-title chat-tab-url "Chat")
      (switch-to-buffer chat-tab-url)
      (buffer-pretend-saved chat-tab-url)
    ) ;begin
  ) ;if
) ;tm-define

(tm-define (chat-tab-send session-id model thinking search)
  (:synopsis "Adapter send entry for a chat tab")
  (:argument session-id "Session UUID")
  (:argument model "Model name")
  (:argument thinking "Thinking mode")
  (:argument search "Search mode")
  (chat-tab-session-send session-id model thinking search)
) ;tm-define

(tm-define (chat-tab-cancel session-id)
  (:synopsis "Adapter cancel entry for a chat tab")
  (:argument session-id "Session UUID")
  ;; plugin session ID 不再包含 model 前缀
  (let ((plugin-ses (string-append "chat-tab:" session-id)))
    (if (!= (connection-status "llm" plugin-ses) 0)
      (begin
        (connection-stop "llm" plugin-ses)
        (plugin-cancel "llm" plugin-ses #t)
        ;; kill 子进程后 plugin 完成回调不会再触发，手动通知 C++ 恢复 Idle
        (chat-tab-notify-state session-id "idle")
      ) ;begin
    ) ;if
  ) ;let
) ;tm-define
