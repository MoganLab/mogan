;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : claw-ai-http.scm
;; DESCRIPTION : HTTP client for Claw AI OpenClaw API
;; COPYRIGHT   : (C) 2026 Liii Network
;;
;; This module provides HTTP client functionality for communicating with
;; the OpenClaw API. It uses the HTTP functions provided by Goldfish.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (claw-ai-http)
  (:use (liii hash-table)))

;; ========== OpenClaw API 配置 ==========

(define claw-ai-api-base-url "http://127.0.0.1:18790")
(define claw-ai-api-timeout 30000)  ; 30 seconds

;; ========== HTTP 请求封装 ==========

(tm-define (claw-ai-http-get path . params)
  (:synopsis "发送 HTTP GET 请求到 OpenClaw API")
  (:argument path "API 路径，如 '/v1/chat'")
  (:argument params "可选的查询参数，如 '((\"key\" . \"value\"))'")
  (let* ((url (string-append claw-ai-api-base-url path))
         (full-url (if (null? params)
                       url
                       (string-append url "?" (claw-ai-params-to-query params))))
         (response (g_http-get full-url)))
    (claw-ai-parse-response response)))

(tm-define (claw-ai-http-post path body . headers)
  (:synopsis "发送 HTTP POST 请求到 OpenClaw API")
  (:argument path "API 路径")
  (:argument body "请求体（字符串或 hash-table）")
  (:argument headers "可选的请求头")
  (let* ((url (string-append claw-ai-api-base-url path))
         (body-str (if (hash-table? body)
                      (claw-ai-body-to-json body)
                      body))
         (header-list (if (null? headers)
                         '(("Content-Type" . "application/json"))
                         (car headers)))
         (response (g_http-post url body-str header-list)))
    (claw-ai-parse-response response)))

;; ========== 辅助函数 ==========

(tm-define (claw-ai-params-to-query params)
  (:synopsis "将参数列表转换为 URL 查询字符串")
  (if (null? params)
      ""
      (string-join
        (map (lambda (p)
               (string-append (car p) "=" (cdr p)))
             params)
        "&")))

(tm-define (claw-ai-body-to-json body)
  (:synopsis "将 hash-table 转换为 JSON 字符串")
  ;; 简化实现，实际使用时应使用 json 库
  (if (hash-table? body)
      (let ((result "{"))
        (hash-table-for-each
          body
          (lambda (key value)
            (set! result
                  (string-append result
                                 "\"" key "\":"
                                 (if (string? value)
                                     (string-append "\"" value "\"")
                                     (number->string value))
                                 ","))))
        ;; 移除最后一个逗号并添加 }
        (if (> (string-length result) 1)
            (set! result (substring result 0 (- (string-length result) 1))))
        (string-append result "}"))
      "{}"))

(tm-define (claw-ai-parse-response response)
  (:synopsis "解析 HTTP 响应")
  (if (hash-table? response)
      (let ((status-code (hash-table-ref response "status-code"))
            (body (hash-table-ref response "text")))
        (if (= status-code 200)
            (list 'success body)
            (list 'error status-code body)))
      (list 'error -1 "Invalid response")))

;; ========== OpenClaw API 封装 ==========

(tm-define (claw-ai-api-chat message context)
  (:synopsis "调用 OpenClaw 聊天 API")
  (:argument message "用户消息")
  (:argument context "文档上下文信息")
  (let ((body (make-hash-table)))
    (hash-table-set! body "message" message)
    (hash-table-set! body "context" context)
    (hash-table-set! body "session_id" (claw-ai-get-session-id))
    (claw-ai-http-post "/v1/chat" body)))

(tm-define (claw-ai-api-stream message context callback)
  (:synopsis "调用 OpenClaw 流式聊天 API")
  (:argument message "用户消息")
  (:argument context "文档上下文信息")
  (:argument callback "回调函数，接收每个数据块")
  ;; 流式输出需要特殊处理
  ;; 这里简化实现，实际应该使用 SSE (Server-Sent Events)
  (let ((body (make-hash-table)))
    (hash-table-set! body "message" message)
    (hash-table-set! body "context" context)
    (hash-table-set! body "session_id" (claw-ai-get-session-id))
    (hash-table-set! body "stream" #t)
    (claw-ai-http-post "/v1/chat/stream" body)))

(tm-define (claw-ai-api-health)
  (:synopsis "检查 OpenClaw API 健康状态")
  (claw-ai-http-get "/health"))

;; ========== 会话管理 ==========

(define claw-ai-session-id #f)

(tm-define (claw-ai-get-session-id)
  (:synopsis "获取或创建会话 ID")
  (if (not claw-ai-session-id)
      (set! claw-ai-session-id (claw-ai-generate-session-id)))
  claw-ai-session-id)

(tm-define (claw-ai-generate-session-id)
  (:synopsis "生成新的会话 ID")
  ;; 简化实现：使用时间戳和随机数
  (string-append "session-"
                 (number->string (current-time))
                 "-"
                 (number->string (random 10000))))

(tm-define (claw-ai-reset-session)
  (:synopsis "重置会话 ID")
  (set! claw-ai-session-id #f))

;; ========== 错误处理 ==========

(tm-define (claw-ai-handle-error error)
  (:synopsis "处理 API 错误")
  (let ((error-type (car error))
        (error-code (cadr error))
        (error-message (caddr error)))
    (display* "Claw AI Error [" error-code "]: " error-message "\n")
    ;; 显示错误消息到 UI
    (claw-ai-widget-append "assistant" 
                           (string-append "Error: " error-message))))
