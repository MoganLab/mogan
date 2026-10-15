;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-protocol-test.scm
;; DESCRIPTION : 纯逻辑单元测试：%chat 协议 v2（attachments 字段）、假附件
;;               插件 ingest/ready、假 llm 插件 sha256 注入回显
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r chat-protocol-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii json))
(import (liii path))
(import (texmacs attachment-cache))

(check-set-mode! 'report-failed)

;;; ---------- connection-defined? 间谍 ----------

;; 测试环境装有 llm 插件（连接已定义），会走真实 feed 路径并拉起 goldfish
;; 子进程。必须在模块加载前定义间谍，使 chat-tab-send 走「连接未定义」
;; 回显分支（s7 对 glue 函数的引用在模块加载时静态绑定，见 chat-list-test）

(tm-define (connection-defined? name) #f)

;; 复用 chat-tree-ops 的 stree 拍平（仿 chat-tree-ops-test 直接加载模块文件）
(load "./TeXmacs/plugins/llm/progs/llm/chat-tree-ops.scm")
(load "./TeXmacs/plugins/llm/progs/llm/chat-protocol.scm")

;;; ---------- chat-input->json：attachments 编码 ----------

(define (make-ctx input attachments)
  (make-chat-input input "sid-json" "model-x" "" "disabled" "disabled" "medium"
    attachments
  ) ;make-chat-input
) ;define

;; 空 attachments：字段恒在，输出空数组

(define (test-json-empty-attachments)
  (let* ((ctx (make-ctx '(document "hello") '()))
         (parsed (string->json (json->string (chat-input->json ctx))))
         (attachments (json-ref parsed "attachments"))
        ) ;
    (check (json-ref-string parsed "sessionId" #f) => "sid-json")
    (check (vector? attachments) => #t)
    (check (vector-length attachments) => 0)
  ) ;let*
) ;define

;; 非空 attachments：每项 sha256/filename 两键（llm 插件只接收 sha 与
;; 文件名），不含本地路径与大小

(define (test-json-nonempty-attachments)
  (let* ((attachments-in (list (list (cons "sha256" "deadbeef") (cons "filename" "b.pdf")))
         ) ;attachments-in
         (ctx (make-ctx '(document "hi") attachments-in))
         (parsed (string->json (json->string (chat-input->json ctx))))
         (attachments (json-ref parsed "attachments"))
        ) ;
    (check (vector-length attachments) => 1)
    (let ((att0 (vector-ref attachments 0)))
      (check (json-ref-string att0 "sha256" #f) => "deadbeef")
      (check (json-ref-string att0 "filename" #f) => "b.pdf")
      (check (json-ref-string att0 "size" #f) => #f)
      (check (json-ref-string att0 "path" #f) => #f)
    ) ;let
  ) ;let*
) ;define

;;; ---------- chat-tab-send（连接未定义分支回显） ----------

;; 7 参入口携带 attachments：connection-defined? 为假时走回显分支
;; （追加一轮并回显输入、返回 #t）。输入区清空依赖 buffer 视图，headless
;; 无视图属既有行为，不在本测试断言

(define (test-chat-tab-send-with-attachments)
  (let* ((sid "test-send-v2")
         (in-buf (chat-tab-session->input-buffer sid))
         (msg-buf (chat-tab-session->message-buffer sid))
        ) ;
    (buffer-set-body in-buf '(document "hello v2"))
    (check
      (chat-tab-send sid
        "model-x"
        ""
        "disabled"
        "disabled"
        "medium"
        (list (list (cons "sha256" "deadbeef") (cons "filename" "b.pdf")))
      ) ;chat-tab-send
      =>
      #t
    ) ;check
    (check
      (string-contains? (chat-tab-flatten-stree (tm->stree (buffer-get-body msg-buf)))
        "hello v2"
      ) ;string-contains?
      =>
      #t
    ) ;check
  ) ;let*
) ;define

;;; ---------- 假插件加载（预定义测试模式跳过 REPL） ----------

(define *tm-attachment-test-mode* #t)
(load "./TeXmacs/plugins/attachment/goldfish/tm-attachment.scm")

(define *tm-llm-test-mode* #t)
(load "./TeXmacs/plugins/llm/goldfish/tm-llm.scm")

;;; ---------- 假附件插件：ingest → ready + 抽取缓存 ----------

;; 协议行构造 helper：%attachment ingest 请求（count/bytes 为本轮已选
;; 累计，插件侧预检用）与 %chat 消息（后者只差 attachments 实参）

(define (make-ingest-line task-id path count bytes)
  (string-append "%attachment "
    (json->string (list (cons "taskId" task-id)
                    (cons "path" path)
                    (cons "count" count)
                    (cons "bytes" bytes)
                  ) ;list
    ) ;json->string
  ) ;string-append
) ;define

(define (make-chat-line attachments)
  (string-append "%chat "
    (json->string
      (list (cons "sessionId" "s1")
        (cons "content" "hi")
        (cons "params" (list (cons "model" "m")))
        (cons "attachments" attachments)
      ) ;list
    ) ;json->string
  ) ;string-append
) ;define

(define (test-fake-attachment-sha256-deterministic)
  (let ((a (fake-attachment-sha256 "/tmp/chat-protocol-test-a.pdf"))
        (b (fake-attachment-sha256 "/tmp/chat-protocol-test-a.pdf"))
        (c (fake-attachment-sha256 "/tmp/chat-protocol-test-b.pdf"))
       ) ;
    (check a => b)
    (check (string=? a c) => #f)
    (check (string-length a) => 64)
  ) ;let
) ;define

(define (test-fake-attachment-ingest-ready)
  (let* ((task-id "test-task-1")
         (path "/tmp/chat-protocol-test-fake.pdf")
         (content "fake pdf bytes")
         (ignored (path-write-text path content))
         (line (make-ingest-line task-id path 0 0))
         (j (string->json (fake-attachment-ingest line)))
         (sha256 (json-ref-string j "sha256" #f))
        ) ;
    (check (json-ref-string j "event" #f) => "ready")
    (check (json-ref-string j "taskId" #f) => task-id)
    (check sha256 => (fake-attachment-sha256 path))
    (check (json-ref-string j "filename" #f) => "chat-protocol-test-fake.pdf")
    (check (json-ref-integer j "size" -1) => (string-length content))
    (check (> (json-ref-integer j "extractedChars" 0) 0) => #t)
    ;; 抽取缓存已按约定落盘（假 llm 按 sha256 能读到假提取文本）
    (let ((cache (attachment-cache-payload-content sha256)))
      (check (string? cache) => #t)
      (check (string-contains? cache "[fake] extracted text of") => #t)
    ) ;let
  ) ;let*
) ;define

(define (test-fake-attachment-ingest-error)
  (let ((j (string->json (fake-attachment-ingest "%attachment not-json"))))
    (check (json-ref-string j "event" #f) => "error")
    (check (string-contains? (json-ref-string j "reason" "") "invalid") => #t)
  ) ;let
) ;define

;;; ---------- 插件侧预检：扩展名白名单 / 数量 / 总大小 ----------

;; 预检在附件插件内（mogan UI 不再本地预检）：count/bytes 为本轮已选
;; 累计；通过返回 #f，违规返回原因字符串

(define (test-fake-attachment-precheck)
  ;; 扩展名白名单：文档 + 源码，大小写不敏感；无扩展名与隐藏文件拒绝
  (check (fake-attachment-precheck "a.PDF" 1 0 0) => #f)
  (check (fake-attachment-precheck "a.Md" 1 0 0) => #f)
  (check (fake-attachment-precheck "a.py" 1 0 0) => #f)
  (check (fake-attachment-precheck "a.rs" 1 0 0) => #f)
  (check (string? (fake-attachment-precheck "noext" 1 0 0)) => #t)
  (check (string? (fake-attachment-precheck ".hidden" 1 0 0)) => #t)
  (let ((err (fake-attachment-precheck "a.exe" 1 0 0)))
    (check (string? err) => #t)
    (check (string-contains? err "a.exe") => #t)
  ) ;let
  ;; 数量上限（含已选累计）：已选 9 可加第 10，已选 10 拒绝
  (check (fake-attachment-precheck "a.pdf" 1 9 0) => #f)
  (check (string? (fake-attachment-precheck "a.pdf" 1 10 0)) => #t)
  ;; 总大小上限 80 MiB（含已选累计）：恰好通过，+1 拒绝
  (check (fake-attachment-precheck "a.pdf" 100 0 (- (* 80 1024 1024) 100)) => #f)
  (check
    (string? (fake-attachment-precheck "a.pdf" 100 0 (- (* 80 1024 1024) 99)))
    =>
    #t
  ) ;check
) ;define

;; 预检违规经 ingest 回 error 事件（mogan UI 原样 toast reason）

(define (test-fake-attachment-ingest-precheck)
  (let* ((bad-path "/tmp/chat-protocol-test-precheck.exe")
         (ignored (path-write-text bad-path "x"))
         (bad-j (string->json (fake-attachment-ingest (make-ingest-line "t-bad" bad-path 0 0)))
         ) ;bad-j
         (ok-path "/tmp/chat-protocol-test-precheck.pdf")
         (ignored2 (path-write-text ok-path "x"))
         (cnt-j (string->json (fake-attachment-ingest (make-ingest-line "t-cnt" ok-path 10 0)))
         ) ;cnt-j
         (bytes-j
           (string->json
             (fake-attachment-ingest (make-ingest-line "t-bytes" ok-path 0 (+ (* 80 1024 1024) 1))
             ) ;fake-attachment-ingest
           ) ;string->json
         ) ;bytes-j
        ) ;
    (check (json-ref-string bad-j "event" #f) => "error")
    (check (json-ref-string bad-j "taskId" #f) => "t-bad")
    (check (string-contains? (json-ref-string bad-j "reason" "") "precheck.exe")
      =>
      #t
    ) ;check
    (check (json-ref-string cnt-j "event" #f) => "error")
    (check (json-ref-string bytes-j "event" #f) => "error")
  ) ;let*
) ;define

;;; ---------- 抽取缓存：月度清理 + 命中刷新（同真实插件约定） ----------

(define (test-attachment-cache-sweep-and-refresh)
  (let* ((root (attachment-extract-root))
         (stale (path-join (path->string root) "202001" "deadbeef"))
         (path "/tmp/chat-protocol-test-sweep.pdf")
         (sha256 (fake-attachment-sha256 path))
         ;; 造过期月目录：ingest 入口的月度清理应删除（保留当月与上月）
         (ignored
           (begin
             (path-mkdir stale #t #t)
             (path-write-text (path->string (path-join stale "result.json")) "[]")
           ) ;begin
         ) ;ignored
         (ignored2 (fake-attachment-ingest (make-ingest-line "t-sweep" path 0 0)))
        ) ;
    (check (path-dir? (path-join (path->string root) "202001")) => #f)
    ;; 命中刷新：预置缓存原文后再次 ingest，原文被保留（仅刷新 ts 重写
    ;; 至当月），不被假抽取文本覆盖
    (let* ((custom "[fake] custom cached content")
           (dir (attachment-extract-cache-dir sha256))
           (payload
             (json->string (vector (list (cons "id" sha256) (cons "ts" "1"))
                             (list (cons "content" custom) (cons "filename" "x.pdf"))
                           ) ;vector
             ) ;json->string
           ) ;payload
           (ignored3
             (begin
               (path-mkdir dir #t #t)
               (path-write-text (path->string (path-join dir "result.json")) payload)
             ) ;begin
           ) ;ignored3
           (ignored4 (fake-attachment-ingest (make-ingest-line "t-refresh" path 0 0)))
          ) ;
      (check (attachment-cache-payload-content sha256) => custom)
    ) ;let*
  ) ;let*
) ;define

;;; ---------- 假 llm 插件：按 sha256 读缓存注入回显 ----------

;; attachment-cache-payload-content 即「按 sha256 查缓存」（上面的
;; ready 测试已借它验证缓存落盘）；这里验证 %chat 回复的注入闭环

(define (test-fake-llm-attachment-injection)
  (let* ((path "/tmp/chat-protocol-test-llm.pdf")
         (sha256 (fake-attachment-sha256 path))
         (ignored (fake-attachment-ingest (make-ingest-line "t-llm" path 0 0)))
         (chat-line
           (make-chat-line (vector (list (cons "sha256" sha256) (cons "filename" "f.pdf")))
           ) ;make-chat-line
         ) ;chat-line
         (reply (fake-llm-chat-reply chat-line))
        ) ;
    (check (string? reply) => #t)
    (check (string-contains? reply "[fake-llm] 我收到了你的消息：hi")
      =>
      #t
    ) ;check
    (check (string-contains? reply (string-append "[fake] attachment " sha256 " injected"))
      =>
      #t
    ) ;check
  ) ;let*
) ;define

;; 无附件（空数组）：无注入行，行为与 v1 一致

(define (test-fake-llm-no-attachments)
  (let ((reply (fake-llm-chat-reply (make-chat-line (vector)))))
    (check (string-contains? reply "[fake] attachment") => #f)
  ) ;let
) ;define

;;; ---------- 测试套件入口 ----------

(tm-define (regtest-chat-protocol)
  (test-json-empty-attachments)
  (test-json-nonempty-attachments)
  (test-chat-tab-send-with-attachments)
  (test-fake-attachment-sha256-deterministic)
  (test-fake-attachment-ingest-ready)
  (test-fake-attachment-ingest-error)
  (test-fake-attachment-precheck)
  (test-fake-attachment-ingest-precheck)
  (test-attachment-cache-sweep-and-refresh)
  (test-fake-llm-attachment-injection)
  (test-fake-llm-no-attachments)
  (check-report)
) ;tm-define
