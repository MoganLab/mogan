# AI 接口调用文档

## 1. 设计目标
- **统一抽象**：Scheme 侧只关心“发送消息”与“接收流式回复”，不依赖具体 AI 服务
- **可插拔**：支持 OpenAI、Claude、本地模型等多种后端
- **错误处理**：网络、鉴权、内容过滤等异常统一封装
- **可取消**：用户可随时中断正在进行的流式请求

## 2. 核心接口（Scheme）
```scheme
;; 发送消息并启动流式回复
(chat-send-message! role content [callback])
;; 取消当前流式请求
(chat-cancel-request!)
;; 设置 AI 服务配置
(chat-set-config! provider api-key model [params])
```

## 3. 数据模型
```scheme
;; 消息结构
(define-record-type message
  (make-message role content timestamp status)
  message?
  (role        message-role)    ; 'user | 'assistant | 'system
  (content     message-content) ; string
  (timestamp   message-timestamp) ; number
  (status      message-status)) ; 'pending | 'streaming | 'done | 'error

;; 配置结构
(define-record-type ai-config
  (make-ai-config provider api-key model params)
  ai-config?
  (provider    config-provider) ; 'openai | 'claude | 'local
  (api-key     config-api-key)  ; string
  (model       config-model)    ; string
  (params      config-params))  ; alist
```

## 4. 调用流程
```
用户输入 → chat-send-message! → HTTP 流式请求 → 逐 token 回调 → 更新 UI
```

### 4.1 流式回调协议
```scheme
;; 回调函数签名：callback token-type data
;; token-type: 'delta | 'done | 'error
;; data: string | #f | error-object

;; 示例回调
(lambda (type data)
  (case type
    ((delta) (chat-update-last-message! data))
    ((done)  (chat-finish-message!))
    ((error) (chat-show-error! data))))
```

## 5. 后端适配器
### 5.1 OpenAI 适配器
```scheme
(define (openai-stream-request messages callback)
  (http-post-stream
   "https://api.openai.com/v1/chat/completions"
   `((model . "gpt-4o-mini")
     (messages . ,(messages->json messages))
     (stream . #t))
   callback))
```

### 5.2 Claude 适配器
```scheme
(define (claude-stream-request messages callback)
  (http-post-stream
   "https://api.anthropic.com/v1/messages"
   `((model . "claude-3-5-sonnet-20241022")
     (max_tokens . 1000)
     (messages . ,(messages->json messages))
     (stream . #t))
   callback))
```

### 5.3 本地模型适配器
```scheme
(define (local-stream-request messages callback)
  (http-post-stream
   "http://localhost:11434/api/generate"
   `((model . "llama3.1")
     (prompt . ,(messages->prompt messages))
     (stream . #t))
   callback))
```

## 6. 错误处理
### 6.1 错误类型
| 错误码 | 场景 | 用户提示 |
|---|---|---|
| 401 | API Key 无效 | "API Key 无效，请检查配置" |
| 429 | 频率限制 | "请求过于频繁，请稍后再试" |
| 500 | 服务器错误 | "AI 服务暂时不可用" |
| timeout | 网络超时 | "网络超时，请检查连接" |

### 6.2 重试机制
```scheme
(define (chat-send-with-retry message retry-count)
  (let loop ((n retry-count))
    (guard (ex
            ((http-error? ex)
             (if (> n 0)
                 (begin (sleep 1) (loop (- n 1)))
                 (chat-show-error! ex))))
      (chat-send-message! message))))
```

## 7. 配置管理
### 7.1 配置文件格式
```scheme
;; ~/.config/texmacs/chat-config.scm
((provider . openai)
 (api-key . "sk-...")
 (model . "gpt-4o-mini")
 (params ((temperature . 0.7)
          (max_tokens . 1000))))
```

### 7.2 运行时配置
```scheme
;; 设置全局配置
(chat-set-config! 'openai "sk-..." "gpt-4o-mini"
                  '((temperature . 0.7)))

;; 临时覆盖参数
(chat-send-message! "user" "hello"
  #:params '((temperature . 1.0)))
```

## 8. 取消机制
```scheme
;; 保存当前请求句柄
(define *current-request* #f)

(define (chat-send-message! role content)
  (set! *current-request*
        (http-post-stream ...))
  ...)

(define (chat-cancel-request!)
  (when *current-request*
    (http-cancel *current-request*)
    (set! *current-request* #f)
    (chat-show-message! "已取消")))
```

## 9. 内容安全
### 9.1 输入过滤
```scheme
(define (sanitize-input text)
  (string-replace
   text
   (regexp "[<>]")
   (lambda (m) (case (string-ref m 0) ((#\<) "&lt;") ((#\>) "&gt;")))))
```

### 9.2 输出过滤
```scheme
(define (filter-response text)
  (cond
   ((string-contains? text "<script>") "[内容被过滤]")
   ((string-contains? text "```") (highlight-code text))
   (else text)))
```

## 10. 调试与日志
### 10.1 日志级别
```scheme
(chat-set-log-level! 'debug) ; 'debug 'info 'warn 'error
```

### 10.2 调试命令
```scheme
;; 查看最近 10 次请求
(chat-get-recent-requests)

;; 重放指定请求
(chat-replay-request request-id)

;; 导出对话历史
(chat-export-history file-path)
```

## 11. 性能监控
### 11.1 指标收集
- 请求延迟（首 token 时间、总时间）
- token 数量（输入/输出）
- 错误率（按错误类型分类）

### 11.2 监控接口
```scheme
;; 获取统计信息
(chat-get-stats)
;; => ((total-requests . 42)
;;     (average-latency . 1.2)
;;     (error-rate . 0.05))
```

## 12. 扩展接口
### 12.1 自定义后端
```scheme
;; 注册新后端
(chat-register-backend! 'my-backend
  (lambda (messages callback)
    (my-http-request messages callback)))
```

### 12.2 中间件支持
```scheme
;; 添加请求中间件
(chat-add-middleware! 'log-request
  (lambda (messages next)
    (log "Request: " messages)
    (next messages)))
```