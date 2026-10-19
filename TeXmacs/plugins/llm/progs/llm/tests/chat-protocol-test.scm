;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-protocol-test.scm
;; DESCRIPTION : 纯逻辑单元测试：%chat 协议 v2（attachments 字段）、假附件
;;               插件 ingest/ready、假 llm 插件 sha256 注入回显、线格式
;;               编码（herk 内部形态 → 插件侧 UTF-8）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r chat-protocol-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii json))
(import (liii os))
(import (liii path))

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
    (let ((cache (fake-llm-attachment-content sha256)))
      (check (string? cache) => #t)
      (check (string-contains? cache "[fake] extracted text of") => #t)
    ) ;let
    (path-unlink path #t)
  ) ;let*
) ;define

(define (test-fake-attachment-ingest-error)
  (let ((j (string->json (fake-attachment-ingest "%attachment not-json"))))
    (check (json-ref-string j "event" #f) => "error")
    (check (string-contains? (json-ref-string j "reason" "") "invalid") => #t)
  ) ;let
) ;define

;;; ---------- 插件侧预检：可访问性 / 扩展名白名单 / 数量 / 总大小 ----------

;; 预检在附件插件内（mogan UI 不再本地预检）：可访问性（存在且为普通
;; 文件、可读）+ count/bytes 为本轮已选累计；通过返回 #f，违规返回
;; 原因字符串

(define (test-fake-attachment-precheck)
  ;; 通过用例的 path 基准：存在且可读的真实文件（filename 与 path 解耦，
  ;; 逐规则独立断言）
  (let ((ok "/tmp/chat-protocol-test-ok.pdf")
        (missing "/tmp/chat-protocol-test-missing.pdf")
       ) ;
    (path-write-text ok "x")
    ;; 可访问性：不存在 / 目录拒绝，存在且可读通过
    (check (string? (fake-attachment-precheck missing "a.pdf" 1 0 0)) => #t)
    (check (string? (fake-attachment-precheck (path-temp-dir) "a.pdf" 1 0 0)) => #t)
    (check (fake-attachment-precheck ok "a.PDF" 1 0 0) => #f)
    ;; 扩展名白名单：文档 + 源码，大小写不敏感；无扩展名与隐藏文件拒绝
    (check (fake-attachment-precheck ok "a.Md" 1 0 0) => #f)
    (check (fake-attachment-precheck ok "a.py" 1 0 0) => #f)
    (check (fake-attachment-precheck ok "a.rs" 1 0 0) => #f)
    ;; Mogan 文档格式
    (check (fake-attachment-precheck ok "a.stem" 1 0 0) => #f)
    (check (fake-attachment-precheck ok "a.tmu" 1 0 0) => #f)
    (check (string? (fake-attachment-precheck ok "noext" 1 0 0)) => #t)
    (check (string? (fake-attachment-precheck ok ".hidden" 1 0 0)) => #t)
    (let ((err (fake-attachment-precheck ok "a.exe" 1 0 0)))
      (check (string? err) => #t)
      (check (string-contains? err "a.exe") => #t)
    ) ;let
    ;; 数量上限（含已选累计）：已选 9 可加第 10，已选 10 拒绝
    (check (fake-attachment-precheck ok "a.pdf" 1 9 0) => #f)
    (check (string? (fake-attachment-precheck ok "a.pdf" 1 10 0)) => #t)
    ;; 总大小上限 80 MiB（含已选累计）：恰好通过，+1 拒绝
    (check (fake-attachment-precheck ok "a.pdf" 100 0 (- (* 80 1024 1024) 100))
      =>
      #f
    ) ;check
    (check
      (string? (fake-attachment-precheck ok "a.pdf" 100 0 (- (* 80 1024 1024) 99)))
      =>
      #t
    ) ;check
    (path-unlink ok #t)
  ) ;let
) ;define

;; 不可读分支：chmod 000 构造；探测仍可读（root 或无 chmod 的 Windows）
;; 则跳过断言，不断言环境不可达的分支

(define (test-fake-attachment-precheck-unreadable)
  ;; 变量名避开 path：(liii path) 的 path 构造函数会被同名局部变量遮蔽
  (let ((noperm "/tmp/chat-protocol-test-noperm.pdf"))
    (path-write-text noperm "x")
    (if (not (os-windows?)) (os-call (string-append "chmod 000 " noperm)))
    (if (not (access noperm 'R_OK))
      (let* ((err (fake-attachment-precheck noperm "a.pdf" 1 0 0))
             ;; 回归：ingest 对不可读文件回 error 事件而非抛
             ;; permission-error（size 求值经 file-exists?）
             (j (string->json (fake-attachment-ingest (make-ingest-line "t-noperm" noperm 0 0)))
             ) ;j
            ) ;
        (check (string? err) => #t)
        (check (string-contains? err "a.pdf") => #t)
        (check (string-contains? err "不可读取") => #t)
        (check (json-ref-string j "event" #f) => "error")
        (check (json-ref-string j "taskId" #f) => "t-noperm")
        (check (string-contains? (json-ref-string j "reason" "") "不可读取") => #t)
      ) ;let*
    ) ;if
    ;; 先恢复权限再清理：file-exists? 对存在但不可读的文件抛
    ;; permission-error，path-unlink 内部经 file-exists?
    (if (not (os-windows?)) (os-call (string-append "chmod 644 " noperm)))
    (path-unlink noperm #t)
  ) ;let
) ;define

;; 预检违规经 ingest 回 error 事件（mogan UI 原样 toast reason）

(define (test-fake-attachment-ingest-precheck)
  (let* ((bad-path "/tmp/chat-protocol-test-precheck.exe")
         (ignored (path-write-text bad-path "x"))
         (bad-j (string->json (fake-attachment-ingest (make-ingest-line "t-bad" bad-path 0 0)))
         ) ;bad-j
         ;; 可访问性：不存在的路径同样经 ingest 回 error 事件
         (gone-j (string->json (fake-attachment-ingest (make-ingest-line "t-gone" "/tmp/chat-protocol-test-gone.pdf" 0 0)
                               ) ;fake-attachment-ingest
                 ) ;string->json
         ) ;gone-j
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
    (check (json-ref-string gone-j "event" #f) => "error")
    (check (json-ref-string gone-j "taskId" #f) => "t-gone")
    (check (string-contains? (json-ref-string gone-j "reason" "")
             "chat-protocol-test-gone.pdf"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (json-ref-string cnt-j "event" #f) => "error")
    (check (json-ref-string bytes-j "event" #f) => "error")
    (path-unlink bad-path #t)
    (path-unlink ok-path #t)
  ) ;let*
) ;define

;;; ---------- 抽取缓存：月度清理 + 命中刷新（同真实插件约定） ----------

(define (test-attachment-cache-sweep-and-refresh)
  (let* ((root (attachment-extract-root))
         (stale (path-join (path->string root) "202001" "deadbeef"))
         (path "/tmp/chat-protocol-test-sweep.pdf")
         (sha256 (fake-attachment-sha256 path))
         ;; 造过期月目录：ingest 入口的月度清理应删除（保留当月与上月）。
         ;; 被 ingest 的文件须真实存在（可访问性预检拒绝不存在路径），用
         ;; 后清理，避免 /tmp 残留掩盖后续同类遗漏
         (ignored
           (begin
             (path-write-text path "x")
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
      (check (fake-llm-attachment-content sha256) => custom)
    ) ;let*
    (path-unlink path #t)
  ) ;let*
) ;define

;;; ---------- 假 llm 插件：按 sha256 读缓存注入回显 ----------

;; tm-llm 的 fake-llm-attachment-content 即「按 sha256 查缓存」（上面的
;; ready 测试已借它验证缓存落盘）；这里验证 %chat 回复的注入闭环

(define (test-fake-llm-attachment-injection)
  (let* ((path "/tmp/chat-protocol-test-llm.pdf")
         ;; 被 ingest 的文件须真实存在（可访问性预检拒绝不存在路径），
         ;; 用后清理，避免 /tmp 残留掩盖后续同类遗漏
         (ignored0 (path-write-text path "x"))
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
    (path-unlink path #t)
  ) ;let*
) ;define

;; 无附件（空数组）：无注入行，行为与 v1 一致

(define (test-fake-llm-no-attachments)
  (let ((reply (fake-llm-chat-reply (make-chat-line (vector)))))
    (check (string-contains? reply "[fake] attachment") => #f)
  ) ;let
) ;define

;;; ---------- 线格式编码：herk 内部形态 → 插件侧 UTF-8 ----------

;; C++ 侧经 from_qstring 传入 herk 内部形态（裸 UTF-8 会被
;; connection_write 的 herk-tree->utf8-tree 逐字节二次映射打乱，中文
;; 路径/文件名到插件侧成乱码）。此处验证 scheme 线格式链：herk 形态的
;; path/filename 经 json->string 组行 + herk-tree->utf8-tree
;; （connection_write 的转换）后，插件收到原始 UTF-8

(define (wire-convert line)
  ;; 模拟 connection_write：树先过 herk-tree->utf8-tree 再交序列化器
  (tm->stree
    (herk-tree->utf8-tree
      (stree->tree `(document ,line))
    ) ;herk-tree->utf8-tree
  ) ;tm->stree
) ;define

(define (test-chat-wire-encoding)
  (let* ((raw "/tmp/北大(pkuthss é).pdf")
         (herk (utf8->herk raw))
         ;; %attachment ingest 行（attachment-ingest 的组行方式）
         (ingest
           (string-append "%attachment "
             (json->string (list (cons "taskId" "t-enc")
                             (cons "path" herk)
                             (cons "count" 0)
                             (cons "bytes" 0)
                           ) ;list
             ) ;json->string
           ) ;string-append
         ) ;ingest
         (ingest-wire (cadr (wire-convert ingest)))
         (ingest-j (string->json (string-drop ingest-wire (string-length "%attachment ")))
         ) ;ingest-j
         ;; %chat 行 attachments 的 filename（attachment_items_to_scheme 下发形态）
         (chat-line
           (json->string
             (chat-input->json
               (make-ctx '(document "hi")
                 (list (list (cons "sha256" "deadbeef") (cons "filename" herk)))
               ) ;make-ctx
             ) ;chat-input->json
           ) ;json->string
         ) ;chat-line
         (chat-wire (cadr (wire-convert (string-append "%chat " chat-line))))
         (chat-j (string->json (string-drop chat-wire (string-length "%chat "))))
        ) ;
    (check (json-ref-string ingest-j "path" "") => raw)
    (check (json-ref-string (vector-ref (json-ref chat-j "attachments") 0) "filename" "")
      =>
      raw
    ) ;check
  ) ;let*
) ;define

;; 故障注入：缓存清理或 ready 缓存写入抛错，仍返回带原 taskId 的 error 事件而非向外抛异常

(define (test-fake-attachment-ingest-fault-injection)
  (let* ((path "/tmp/chat-protocol-test-fault.pdf")
         (ignored (path-write-text path "x"))
         (orig-sweep attachment-cache-sweep!)
         (orig-put attachment-cache-put!)
        ) ;
    ;; 1. 故障注入：缓存清理抛错
    (set! attachment-cache-sweep! (lambda () (error "simulated sweep failure")))
    (let ((j (string->json (fake-attachment-ingest (make-ingest-line "t-fault-sweep" path 0 0))
             ) ;string->json
          ) ;j
         ) ;
      (check (json-ref-string j "event" #f) => "error")
      (check (json-ref-string j "taskId" #f) => "t-fault-sweep")
      (check (string? (json-ref-string j "reason" #f)) => #t)
    ) ;let
    (set! attachment-cache-sweep! orig-sweep)

    ;; 2. 故障注入：ready 缓存写入抛错
    (set! attachment-cache-put!
      (lambda (sha filename content) (error "simulated put failure"))
    ) ;set!
    (let ((j (string->json (fake-attachment-ingest (make-ingest-line "t-fault-put" path 0 0))
             ) ;string->json
          ) ;j
         ) ;
      (check (json-ref-string j "event" #f) => "error")
      (check (json-ref-string j "taskId" #f) => "t-fault-put")
      (check (string? (json-ref-string j "reason" #f)) => #t)
    ) ;let
    (set! attachment-cache-put! orig-put)

    ;; 3. 非法 JSON：无法取得 taskId 时回退 taskId 为 ""
    (let ((j (string->json (fake-attachment-ingest "%attachment {bad-json"))))
      (check (json-ref-string j "event" #f) => "error")
      (check (json-ref-string j "taskId" #f) => "")
      (check (json-ref-string j "reason" #f) => "invalid ingest request")
    ) ;let

    (path-unlink path #t)
  ) ;let*
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
  (test-fake-attachment-precheck-unreadable)
  (test-fake-attachment-ingest-fault-injection)
  (test-fake-attachment-ingest-precheck)
  (test-attachment-cache-sweep-and-refresh)
  (test-fake-llm-attachment-injection)
  (test-fake-llm-no-attachments)
  (test-chat-wire-encoding)
  (check-report)
) ;tm-define
