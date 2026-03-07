;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : claw-ai.scm
;; DESCRIPTION : Claw AI integration for Mogan
;; COPYRIGHT   : (C) 2026 Liii Network
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (claw-ai)
  (:use (generic generic-edit)
        (utils library cursor)
        (kernel gui menu-widget)
        (claw-ai-http)))

;; ========== 状态管理 ==========

(define claw-ai-buffer #f)
(define claw-ai-widget-visible? #f)
(define claw-ai-session-id #f)
(define claw-ai-message-history '())

;; ========== 缓冲区管理 ==========

(tm-define (claw-ai-buffer-get)
  (:synopsis "获取或创建 Claw AI 缓冲区")
  (if (and claw-ai-buffer (buffer-exists? claw-ai-buffer))
      claw-ai-buffer
      (begin
        (set! claw-ai-buffer 
              (string->url "tmfs://aux/claw-ai"))
        (buffer-create claw-ai-buffer)
        claw-ai-buffer)))

;; ========== 窗口控制 ==========

(tm-define (claw-ai-show)
  (:synopsis "显示 Claw AI 面板")
  (:interactive #t)
  (change-auxiliary-widget-focus)
  (let* ((u (current-buffer))
         (aux (claw-ai-buffer-get)))
    (buffer-set-master aux u)
    (set! claw-ai-widget-visible? #t)
    ;; 调用 C++ 层显示窗口
    (claw-ai-widget-show)
    ;; 设置窗口标题
    (set-auxiliary-widget-title "Claw AI")
    (buffer-focus aux #t)))

(tm-define (claw-ai-hide)
  (:synopsis "隐藏 Claw AI 面板")
  (set! claw-ai-widget-visible? #f)
  (claw-ai-widget-hide)
  (when (nnull? (window-list))
    (buffer-focus (current-buffer) #t)))

(tm-define (claw-ai-toggle)
  (:synopsis "切换 Claw AI 面板显示/隐藏")
  (:interactive #t)
  (if claw-ai-widget-visible?
      (claw-ai-hide)
      (claw-ai-show)))

;; ========== 消息处理 ==========

(tm-define (claw-ai-send message)
  (:synopsis "发送消息到 Claw AI")
  (:argument message "Message to send")
  (when (and message (> (string-length message) 0))
    ;; 1. 显示用户消息
    (claw-ai-widget-append "user" message)
    ;; 2. 添加到历史
    (set! claw-ai-message-history
          (append claw-ai-message-history
                  (list (list "user" message))))
    ;; 3. 获取文档上下文
    (let ((context (claw-ai-get-context)))
      ;; 4. 调用 OpenClaw API（异步）
      (claw-ai-call-api message context))))

(tm-define (claw-ai-receive response)
  (:synopsis "接收 Claw AI 响应")
  ;; 1. 显示助手消息
  (claw-ai-widget-append "assistant" response)
  ;; 2. 添加到历史
  (set! claw-ai-message-history
        (append claw-ai-message-history
                (list (list "assistant" response)))))

(tm-define (claw-ai-stream chunk)
  (:synopsis "接收流式输出块")
  ;; 更新最后一条消息（流式输出）
  (claw-ai-widget-update-last chunk))

;; ========== 文档上下文 ==========

(tm-define (claw-ai-get-context)
  (:synopsis "获取当前文档上下文")
  (with-buffer (current-buffer)
    (let* ((selection (if (selection-active?)
                          (selection-tree)
                          ""))
           (cursor-context (tree-around-cursor))
           (doc-type (get-env "mode")))
      (list 
       (cons "selection" selection)
       (cons "cursor-context" cursor-context)
       (cons "doc-type" doc-type)
       (cons "buffer" (url->string (current-buffer)))))))

(tm-define (tree-around-cursor)
  (:synopsis "获取光标周围的树结构")
  (let ((path (cursor-path)))
    (if (null? path)
        ""
        (with t (path->tree (cDr path))
          (if t (tree->stree t) "")))))

;; ========== 历史管理 ==========

(tm-define (claw-ai-clear-history)
  (:synopsis "清空消息历史")
  (set! claw-ai-message-history '())
  (claw-ai-widget-clear))

(tm-define (claw-ai-get-history)
  (:synopsis "获取消息历史")
  claw-ai-message-history)

;; ========== OpenClaw API 调用 ==========

(tm-define (claw-ai-call-api message context)
  (:synopsis "调用 OpenClaw API")
  ;; 使用 claw-ai-http 模块
  (let ((result (claw-ai-api-chat message context)))
    (if (eq? (car result) 'success)
        (claw-ai-receive (cadr result))
        (claw-ai-handle-error result))))

;; ========== C++ 层接口（Glue 函数） ==========

;; 这些函数在 C++ 层实现，通过 glue 绑定到 Scheme
;; 参见 src/Plugins/Qt/claw_ai_glue.cpp

;; (claw-ai-widget-show) - 显示窗口
;; (claw-ai-widget-hide) - 隐藏窗口
;; (claw-ai-widget-append role content) - 添加消息
;; (claw-ai-widget-update-last content) - 更新最后一条消息
;; (claw-ai-widget-clear) - 清空消息
;; (claw-ai-widget-set-streaming streaming?) - 设置流式输出状态
;; (claw-ai-widget-message-count) - 获取消息数量

;; 包装函数，提供更友好的接口
(tm-define (claw-ai-ui-show)
  (:synopsis "显示 Claw AI UI")
  (claw-ai-widget-show))

(tm-define (claw-ai-ui-hide)
  (:synopsis "隐藏 Claw AI UI")
  (claw-ai-widget-hide))

(tm-define (claw-ai-ui-append role content)
  (:synopsis "添加消息到 UI")
  (claw-ai-widget-append role content))

(tm-define (claw-ai-ui-update content)
  (:synopsis "更新最后一条消息")
  (claw-ai-widget-update-last content))

(tm-define (claw-ai-ui-clear)
  (:synopsis "清空 UI 消息")
  (claw-ai-widget-clear))

;; 消息处理回调（由 C++ 层调用）
(tm-define (claw-ai-handle-message message)
  (:synopsis "处理用户发送的消息（C++ 回调）")
  (claw-ai-send message))

;; ========== 键盘快捷键 ==========

(kbd-map
  ("C-`" (claw-ai-toggle))           ; Ctrl+` 切换面板
  ("A-`" (claw-ai-toggle)))          ; Alt+` 切换面板

;; ========== 菜单集成 ==========

(menu-bind claw-ai-menu
  ("Show Claw AI" (claw-ai-show))
  ("Hide Claw AI" (claw-ai-hide))
  ---
  ("Clear History" (claw-ai-clear-history)))

;; 添加到 Tools 菜单
(menu-bind tools-menu
  ("Claw AI" (link claw-ai-menu)))
