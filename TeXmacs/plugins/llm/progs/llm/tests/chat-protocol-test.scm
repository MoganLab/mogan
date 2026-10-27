;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-protocol-test.scm
;; DESCRIPTION : 纯逻辑单元测试：%chat 协议 v2（attachments 字段）、假附件
;;               插件 ingest/ready 与缓存 supplier 落盘确认、假 llm 插件
;;               sha256 注入回显、线格式编码（herk 内部形态 → 插件侧 UTF-8）
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
(import (texmacs protocol))

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
(load "./TeXmacs/plugins/attachment/progs/attachment-policy.scm")
(load "./TeXmacs/plugins/attachment/goldfish/tm-attachment.scm")

(define *tm-llm-test-mode* #t)
(load "./TeXmacs/plugins/llm/goldfish/tm-llm.scm")

;;; ---------- 假附件插件：ingest → ready + 抽取缓存 ----------

;; 协议行构造 helper：%attachment ingest 请求（只含 taskId/path，可选
;; sourcePath，预检已上移宿主）与 %chat 消息（后者只差 attachments 实参）

(define (make-ingest-line task-id path . opt)
  (string-append "%attachment "
    (attachment-ingest-request-json task-id path (if (null? opt) "" (car opt)))
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

(define (test-clear-extract-cache! sha256)
  ;; 测试自净：清掉该 sha 当月与上月的缓存（机器上残留的旧版缓存会让
  ;; 新缓存断言误走命中刷新分支），测试结束时也调用以免影响其他用例
  (for-each
    (lambda (m)
      (let ((d (path-join (path->string (attachment-extract-root)) m sha256)))
        (when (path-dir? d)
          (attachment-remove-tree d)
        ) ;when
      ) ;let
    ) ;lambda
    (attachment-keep-months)
  ) ;for-each
) ;define

(define (test-fake-attachment-ingest-ready)
  ;; 6234：新缓存在 [1] 标注 supplier=fake，直接读当月 result.json 验证
  ;; （不只经假 llm content reader——它只读 content 且逐月查找）
  (let* ((task-id "test-task-1")
         (path "/tmp/chat-protocol-test-fake.pdf")
         (content "fake pdf bytes")
         (sha256 (fake-attachment-sha256 path))
         (ignored-clean (test-clear-extract-cache! sha256))
         (ignored (path-write-text path content))
         (line (make-ingest-line task-id path))
         (j (string->json (fake-attachment-ingest line)))
        ) ;
    (check (json-ref-string j "event" #f) => "ready")
    (check (json-ref-string j "taskId" #f) => task-id)
    (check (json-ref-string j "sha256" #f) => sha256)
    (check (json-ref-string j "filename" #f) => "chat-protocol-test-fake.pdf")
    (check (json-ref-integer j "size" -1) => (string-length content))
    (check (> (json-ref-integer j "extractedChars" 0) 0) => #t)
    ;; 当月 result.json：顶层二元 vector、[0].id 正确、[1] 含 supplier
    ;; 与假提取文本
    (let* ((f (path-join (attachment-extract-cache-dir sha256) "result.json"))
           (cache (string->json (path-read-text (path->string f))))
          ) ;
      (check (vector? cache) => #t)
      (check (vector-length cache) => 2)
      (check (json-ref-string (vector-ref cache 0) "id" #f) => sha256)
      (check (json-ref-string (vector-ref cache 1) "supplier" #f) => "fake")
      (check (json-ref-string (vector-ref cache 1) "content" #f)
        =>
        "[fake] extracted text of chat-protocol-test-fake.pdf"
      ) ;check
    ) ;let*
    ;; 回归：假 llm 仍能按 sha256 读到假提取文本
    (let ((cache (fake-llm-attachment-content sha256)))
      (check (string? cache) => #t)
      (check (string-contains? cache "[fake] extracted text of") => #t)
    ) ;let
    (path-unlink path #t)
    (test-clear-extract-cache! sha256)
  ) ;let*
) ;define

(define (test-fake-attachment-ingest-error)
  (let ((j (string->json (fake-attachment-ingest "%attachment not-json"))))
    (check (json-ref-string j "event" #f) => "error")
    (check (string-contains? (json-ref-string j "reason" "") "invalid") => #t)
  ) ;let
) ;define

;;; ---------- 6229：sourcePath（宿主格式转换后 sha256 以原文件为准） ----------

(define (test-attachment-ingest-request-json-shape)
  ;; 宿主请求构造（attachment-policy.scm）：常规附件不带 sourcePath 键，
  ;; 与旧插件的请求契约保持兼容；转换附件带该键且 path 仍指向读取目标
  (let ((plain (string->json (attachment-ingest-request-json "t1" "/tmp/a.pdf" "")))
        (converted (string->json (attachment-ingest-request-json "t2" "/tmp/x/a.pdf" "/home/u/报告.tmu")
                   ) ;string->json
        ) ;converted
       ) ;
    (check (json-ref-string plain "taskId" #f) => "t1")
    (check (json-ref-string plain "path" #f) => "/tmp/a.pdf")
    (check (json-ref-string plain "sourcePath" #f) => #f)
    (check (json-ref-string converted "sourcePath" #f) => "/home/u/报告.tmu")
    (check (json-ref-string converted "path" #f) => "/tmp/x/a.pdf")
  ) ;let
) ;define

(define (test-fake-attachment-sha256-key)
  ;; 键选择纯函数：source-path 为空退回读取路径，非空以原文件为准
  (check (fake-attachment-sha256-key "/a/b.pdf" "") => "/a/b.pdf")
  (check (fake-attachment-sha256-key "/a/b.pdf" "/a/b.tmu") => "/a/b.tmu")
) ;define

(define (test-fake-attachment-ingest-source-sha256)
  ;; 宿主把 .tmu 转成临时 PDF 后派发：sha256 与抽取缓存键须随原文档——
  ;; 同一文档重复添加（临时 PDF 路径每次随机）命中同一缓存；
  ;; filename/size 仍取实际读取的临时 PDF
  (let* ((task-id "test-task-src")
         (pdf-path "/tmp/chat-protocol-test-src.pdf")
         (source-path "/tmp/chat-protocol-test-src.tmu")
         (ignored (path-write-text pdf-path "fake pdf bytes"))
         (j (string->json (fake-attachment-ingest (make-ingest-line task-id pdf-path source-path))
            ) ;string->json
         ) ;j
         (sha256 (json-ref-string j "sha256" #f))
        ) ;
    (check (json-ref-string j "event" #f) => "ready")
    (check sha256 => (fake-attachment-sha256 source-path))
    (check (string=? sha256 (fake-attachment-sha256 pdf-path)) => #f)
    (check (json-ref-string j "filename" #f) => "chat-protocol-test-src.pdf")
    (path-unlink pdf-path #t)
  ) ;let*
) ;define

(define (test-fake-attachment-ingest-empty-source-falls-back)
  ;; 显式空 sourcePath 与缺键等价：退回对读取路径取散列
  (let* ((task-id "test-task-src-empty")
         (pdf-path "/tmp/chat-protocol-test-src-empty.pdf")
         (ignored (path-write-text pdf-path "fake pdf bytes"))
         (j (string->json (fake-attachment-ingest (make-ingest-line task-id pdf-path "")))
         ) ;j
        ) ;
    (check (json-ref-string j "event" #f) => "ready")
    (check (json-ref-string j "sha256" #f) => (fake-attachment-sha256 pdf-path))
    (path-unlink pdf-path #t)
  ) ;let*
) ;define

;;; ---------- 宿主侧预检：可访问性 / 扩展名白名单 / 数量 / 总大小 ----------

;; 预检策略上移宿主（attachment-policy.scm，真假插件共同遵从同一约定）：
;; 可访问性（存在且为普通文件、可读）+ count/bytes 为本轮已选累计；
;; 通过返回 #f，违规返回原因字符串

(define (test-attachment-host-precheck)
  ;; 通过用例的 path 基准：存在且可读的真实文件（filename 与 path 解耦，
  ;; 逐规则独立断言）
  (let ((ok "/tmp/chat-protocol-test-ok.pdf")
        (missing "/tmp/chat-protocol-test-missing.pdf")
       ) ;
    (path-write-text ok "x")
    ;; 可访问性：不存在 / 目录拒绝，存在且可读通过
    (check (string? (attachment-host-precheck missing "a.pdf" 1 0 0)) => #t)
    (check (string? (attachment-host-precheck (path-temp-dir) "a.pdf" 1 0 0)) => #t)
    (check (attachment-host-precheck ok "a.PDF" 1 0 0) => #f)
    ;; 扩展名白名单：文档 + 源码，大小写不敏感；无扩展名与隐藏文件拒绝
    (check (attachment-host-precheck ok "a.Md" 1 0 0) => #f)
    (check (attachment-host-precheck ok "a.py" 1 0 0) => #f)
    (check (attachment-host-precheck ok "a.rs" 1 0 0) => #f)
    ;; Mogan 文档格式
    (check (attachment-host-precheck ok "a.stem" 1 0 0) => #f)
    (check (attachment-host-precheck ok "a.tmu" 1 0 0) => #f)
    (check (string? (attachment-host-precheck ok "noext" 1 0 0)) => #t)
    (check (string? (attachment-host-precheck ok ".hidden" 1 0 0)) => #t)
    (let ((err (attachment-host-precheck ok "a.exe" 1 0 0)))
      (check (string? err) => #t)
      (check (string-contains? err "a.exe") => #t)
    ) ;let
    ;; 数量上限（含已选累计）：已选 9 可加第 10，已选 10 拒绝
    (check (attachment-host-precheck ok "a.pdf" 1 9 0) => #f)
    (check (string? (attachment-host-precheck ok "a.pdf" 1 10 0)) => #t)
    ;; 总大小上限 80 MiB（含已选累计）：恰好通过，+1 拒绝
    (check (attachment-host-precheck ok "a.pdf" 100 0 (- (* 80 1024 1024) 100))
      =>
      #f
    ) ;check
    (check
      (string? (attachment-host-precheck ok "a.pdf" 100 0 (- (* 80 1024 1024) 99)))
      =>
      #t
    ) ;check
    (path-unlink ok #t)
  ) ;let
) ;define

;; 不可读分支：chmod 000 构造；探测仍可读（root 或无 chmod 的 Windows）
;; 则跳过断言，不断言环境不可达的分支

(define (test-attachment-host-precheck-unreadable)
  ;; 变量名避开 path：(liii path) 的 path 构造函数会被同名局部变量遮蔽
  (let ((noperm "/tmp/chat-protocol-test-noperm.pdf"))
    (path-write-text noperm "x")
    (if (not (os-windows?)) (os-call (string-append "chmod 000 " noperm)))
    (if (not (access noperm 'R_OK))
      (let ((err (attachment-host-precheck noperm "a.pdf" 1 0 0)))
        (check (string? err) => #t)
        (check (string-contains? err "a.pdf") => #t)
        (check (string-contains? err "不可读取") => #t)
      ) ;let
    ) ;if
    ;; 先恢复权限再清理：file-exists? 对存在但不可读的文件抛
    ;; permission-error，path-unlink 内部经 file-exists?
    (if (not (os-windows?)) (os-call (string-append "chmod 644 " noperm)))
    (path-unlink noperm #t)
  ) ;let
) ;define

;; 非 ASCII 路径：宿主预检直接做文件系统调用（不经 wire，无
;; connection_write 的 tree_herk_to_utf8），故须验证 herk 形态路径
;; 先解回真实 UTF-8 后才查得到文件。attachment-ingest 依赖 tm-server
;; 的 connection/plugin 设施，headless 单测不加载；此处直接验证解码
;; 后的等价条件与 policy 行为。

(define (test-attachment-host-precheck-nonascii-path)
  (let ((dir "/tmp/chat-protocol-test-论文")
        (file "/tmp/chat-protocol-test-论文/ok.pdf")
       ) ;
    (path-mkdir dir)
    (path-write-text file "x")
    ;; herk 形态（C++ from_qstring 契约下 attachment-ingest 的入参形态）
    ;; 直接做 path-file? 必为假；cork->utf8 解码后为真
    (let ((herk (utf8->cork file)))
      (check (path-file? herk) => #f)
      (check (path-file? (cork->utf8 herk)) => #t)
      ;; 解码后走 policy：存在且可读 → #f
      (check
        (attachment-host-precheck (cork->utf8 herk)
          (path->string (path-name (path herk)))
          1
          0
          0
        ) ;attachment-host-precheck
        =>
        #f
      ) ;check
    ) ;let
    (path-unlink file #t)
    (path-rmdir dir)
  ) ;let
) ;define

;; 异常兜底测试：通过故障注入让缓存清理或 ready 缓存写入抛错，
;; 仍返回带原 taskId 的 error 事件而不是向外抛出异常。dynamic-wind
;; 保证断言中断时也恢复全局函数并清理样本

(define (test-fake-attachment-ingest-exception-fallback)
  (let* ((ok-path "/tmp/chat-protocol-test-fallback.pdf")
         (ignored (path-write-text ok-path "x"))
         (orig-sweep attachment-cache-sweep!)
         (orig-put attachment-cache-put!)
         (j-pair
           (dynamic-wind (lambda () #t)
             (lambda ()
               (let* (
                       ;; 1. 注入缓存清理异常
                       (ignored-sweep (set! attachment-cache-sweep!
                                        (lambda () (error 'io-error "disk failure during sweep"))
                                      ) ;set!
                       ) ;ignored-sweep
                       (sweep-j (string->json (fake-attachment-ingest (make-ingest-line "t-sweep-err" ok-path)))
                       ) ;sweep-j
                       (ignored-restore-sweep (set! attachment-cache-sweep! orig-sweep))
                       ;; 2. 注入 ready 缓存写入异常
                       (ignored-put (set! attachment-cache-put!
                                      (lambda (sha name content) (error 'disk-full "cannot put cache"))
                                    ) ;set!
                       ) ;ignored-put
                       (put-j (string->json (fake-attachment-ingest (make-ingest-line "t-put-err" ok-path)))
                       ) ;put-j
                     ) ;
                 (list sweep-j put-j)
               ) ;let*
             ) ;lambda
             (lambda ()
               ;; 断言中途失败也恢复全局函数、清理样本
               (set! attachment-cache-sweep! orig-sweep)
               (set! attachment-cache-put! orig-put)
               (path-unlink ok-path #t)
             ) ;lambda
           ) ;dynamic-wind
         ) ;j-pair
        ) ;
    (let ((sweep-j (car j-pair)) (put-j (cadr j-pair)))
      (check (json-ref-string sweep-j "event" #f) => "error")
      (check (string=? (json-ref-string sweep-j "event" #f) "ready") => #f)
      (check (json-ref-string sweep-j "taskId" #f) => "t-sweep-err")
      (check (string-contains? (json-ref-string sweep-j "reason" "") "附件处理异常")
        =>
        #t
      ) ;check
      (check (string-contains? (json-ref-string sweep-j "reason" "")
               "disk failure during sweep"
             ) ;string-contains?
        =>
        #t
      ) ;check
      (check (json-ref-string put-j "event" #f) => "error")
      (check (string=? (json-ref-string put-j "event" #f) "ready") => #f)
      (check (json-ref-string put-j "taskId" #f) => "t-put-err")
      (check (string-contains? (json-ref-string put-j "reason" "") "附件处理异常")
        =>
        #t
      ) ;check
      (check (string-contains? (json-ref-string put-j "reason" "") "cannot put cache")
        =>
        #t
      ) ;check
    ) ;let
  ) ;let*
) ;define

;;; ---------- 6234：当月落盘确认助手 attachment-cache-payload-stored? ----------

(define (test-attachment-cache-payload-stored)
  ;; 直接构造当月/上月缓存文件逐场景验证：只有当月文件存在且 id 与完整
  ;; [1] 一致才返回 #t；文件不存在、非法 JSON、非二元 vector、id/payload
  ;; 不符、只有上月文件都不得确认成功
  (let* ((sha "6234000000000000000000000000000000000000000000000000000000006234")
         (entry (list (cons "content" "c-6234")
                  (cons "filename" "a.pdf")
                  (cons "supplier" "fake")
                ) ;list
         ) ;entry
         (months (attachment-keep-months))
         (cur (attachment-extract-cache-dir sha))
         (prev (path-join (path->string (attachment-extract-root)) (cadr months) sha))
         (write-cur!
           (lambda (text)
             (path-mkdir cur #t #t)
             (path-write-text (path->string (path-join cur "result.json")) text)
           ) ;lambda
         ) ;write-cur!
         (ok-text
           (json->string (vector (list (cons "id" sha) (cons "ts" "1")) entry))
         ) ;ok-text
        ) ;
    (dynamic-wind (lambda () #t)
      (lambda ()
        ;; 文件不存在
        (check (attachment-cache-payload-stored? sha entry) => #f)
        ;; 当月 id 与完整 payload 一致
        (write-cur! ok-text)
        (check (attachment-cache-payload-stored? sha entry) => #t)
        ;; 非法 JSON
        (write-cur! "not json {")
        (check (attachment-cache-payload-stored? sha entry) => #f)
        ;; 非二元 vector（顶层一元）
        (write-cur!
          (json->string (vector (list (cons "id" sha) (cons "ts" "1"))))
        ) ;write-cur!
        (check (attachment-cache-payload-stored? sha entry) => #f)
        ;; id 不符
        (write-cur!
          (json->string (vector (list (cons "id" "deadbeef") (cons "ts" "1")) entry))
        ) ;write-cur!
        (check (attachment-cache-payload-stored? sha entry) => #f)
        ;; content 不符
        (write-cur!
          (json->string (vector (list (cons "id" sha) (cons "ts" "1"))
                          (list (cons "content" "wrong")
                            (cons "filename" "a.pdf")
                            (cons "supplier" "fake")
                          ) ;list
                        ) ;vector
          ) ;json->string
        ) ;write-cur!
        (check (attachment-cache-payload-stored? sha entry) => #f)
        ;; supplier 缺失
        (write-cur!
          (json->string (vector (list (cons "id" sha) (cons "ts" "1"))
                          (list (cons "content" "c-6234") (cons "filename" "a.pdf"))
                        ) ;vector
          ) ;json->string
        ) ;write-cur!
        (check (attachment-cache-payload-stored? sha entry) => #f)
        ;; supplier 取值不同
        (write-cur!
          (json->string (vector (list (cons "id" sha) (cons "ts" "1"))
                          (list (cons "content" "c-6234")
                            (cons "filename" "a.pdf")
                            (cons "supplier" "local")
                          ) ;list
                        ) ;vector
          ) ;json->string
        ) ;write-cur!
        (check (attachment-cache-payload-stored? sha entry) => #f)
        ;; 只有上月文件、当月无文件：不得因上月文件存在而确认成功
        (attachment-remove-tree cur)
        (path-mkdir prev #t #t)
        (path-write-text (path->string (path-join prev "result.json")) ok-text)
        (check (attachment-cache-payload-stored? sha entry) => #f)
      ) ;lambda
      (lambda ()
        ;; 清理当月与上月测试缓存，避免影响其他用例
        (when (path-dir? cur)
          (attachment-remove-tree cur)
        ) ;when
        (when (path-dir? prev)
          (attachment-remove-tree prev)
        ) ;when
      ) ;lambda
    ) ;dynamic-wind
  ) ;let*
) ;define

;;; ---------- 6234：写入 no-op / 内容不一致的端到端故障用例 ----------

;; 最关键的新故障用例：attachment-cache-put! 正常返回但完全不写文件——
;; 证明实现不是只检查 put 的返回值，而是独立回读当月 result.json

(define (test-fake-attachment-ingest-put-noop)
  (let* ((path "/tmp/chat-protocol-test-noop.pdf")
         (ignored0 (path-write-text path "x"))
         (sha256 (fake-attachment-sha256 path))
         ;; 全新 sha：先清掉可能残留的旧缓存，确保当月没有可误用的文件
         (ignored-clean (test-clear-extract-cache! sha256))
         (orig-put attachment-cache-put!)
         (j
           (dynamic-wind
             (lambda () (set! attachment-cache-put! (lambda (sha name content) #t)))
             (lambda ()
               (string->json (fake-attachment-ingest (make-ingest-line "t-noop-put" path)))
             ) ;lambda
             (lambda () (set! attachment-cache-put! orig-put))
           ) ;dynamic-wind
         ) ;j
        ) ;
    (check (json-ref-string j "event" #f) => "error")
    (check (json-ref-string j "taskId" #f) => "t-noop-put")
    (check (string-contains? (json-ref-string j "reason" "") "附件缓存写入失败")
      =>
      #t
    ) ;check
    ;; 不得 ready：error 事件无 sha256 键
    (check (json-ref-string j "sha256" #f) => #f)
    (path-unlink path #t)
    (test-clear-extract-cache! sha256)
  ) ;let*
) ;define

;; 文件存在却内容不对（非法 JSON / 错误 id / 与 expected-payload 不同的
;; content）：端到端必须 error、不得 ready——防止实现只做存在性检查

(define (test-fake-attachment-ingest-inconsistent-cache)
  (let* ((path "/tmp/chat-protocol-test-badcache.pdf")
         (ignored0 (path-write-text path "x"))
         (sha256 (fake-attachment-sha256 path))
         (ignored-clean (test-clear-extract-cache! sha256))
         (dir (attachment-extract-cache-dir sha256))
         (filename "chat-protocol-test-badcache.pdf")
         (entry-good (list (cons "content" (fake-attachment-extracted-text filename))
                       (cons "filename" filename)
                       (cons "supplier" "fake")
                     ) ;list
         ) ;entry-good
         (write-cur!
           (lambda (text)
             (path-mkdir dir #t #t)
             (path-write-text (path->string (path-join dir "result.json")) text)
           ) ;lambda
         ) ;write-cur!
         (orig-put attachment-cache-put!)
         (ingest-put
           ;; 换上给定写入实现后跑一次完整 ingest（dynamic-wind 保证恢复）
           (lambda (put)
             (dynamic-wind (lambda () (set! attachment-cache-put! put))
               (lambda ()
                 (string->json (fake-attachment-ingest (make-ingest-line "t-bad-cache" path)))
               ) ;lambda
               (lambda () (set! attachment-cache-put! orig-put))
             ) ;dynamic-wind
           ) ;lambda
         ) ;ingest-put
         ;; 1. 创建目录但写入非法 JSON（put 仍返回正确的 expected-payload）
         (j1 (ingest-put (lambda (sha name content) (write-cur! "not json {") entry-good))
         ) ;j1
         ;; 2. 写入错误 id
         (j2
           (ingest-put
             (lambda (sha name content)
               (write-cur!
                 (json->string (vector (list (cons "id" "deadbeef") (cons "ts" "1")) entry-good))
               ) ;write-cur!
               entry-good
             ) ;lambda
           ) ;ingest-put
         ) ;j2
         ;; 3. 写入与 expected-payload 不同的 content（supplier 正确也不行）
         (j3
           (ingest-put
             (lambda (sha name content)
               (write-cur!
                 (json->string (vector (list (cons "id" sha) (cons "ts" "1"))
                                 (list (cons "content" "tampered")
                                   (cons "filename" filename)
                                   (cons "supplier" "fake")
                                 ) ;list
                               ) ;vector
                 ) ;json->string
               ) ;write-cur!
               entry-good
             ) ;lambda
           ) ;ingest-put
         ) ;j3
        ) ;
    (for-each
      (lambda (j)
        (check (json-ref-string j "event" #f) => "error")
        (check (json-ref-string j "taskId" #f) => "t-bad-cache")
        (check (string-contains? (json-ref-string j "reason" "") "附件缓存写入失败")
          =>
          #t
        ) ;check
        (check (json-ref-string j "sha256" #f) => #f)
      ) ;lambda
      (list j1 j2 j3)
    ) ;for-each
    (path-unlink path #t)
    (test-clear-extract-cache! sha256)
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
         (ignored2 (fake-attachment-ingest (make-ingest-line "t-sweep" path)))
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
           (ignored4 (fake-attachment-ingest (make-ingest-line "t-refresh" path)))
          ) ;
      (check (fake-llm-attachment-content sha256) => custom)
    ) ;let*
    (path-unlink path #t)
  ) ;let*
) ;define

;;; ---------- 6234：命中刷新也必须当月回读一致后才 ready ----------

(define (test-attachment-hit-refresh-confirmation)
  ;; 上月有效缓存：命中刷新为 no-op（写当月失败）时，即使上月文件还在
  ;; 也必须 error；正常命中刷新则当月文件 [1] 保留 content/supplier、
  ;; ts 刷新，回读一致后才 ready
  (let* ((path "/tmp/chat-protocol-test-refresh.pdf")
         (ignored0 (path-write-text path "x"))
         (sha256 (fake-attachment-sha256 path))
         (ignored-clean (test-clear-extract-cache! sha256))
         (months (attachment-keep-months))
         (prev (path-join (path->string (attachment-extract-root)) (cadr months) sha256))
         (cur (attachment-extract-cache-dir sha256))
         (custom "[fake] cached last month")
         (entry (list (cons "content" custom)
                  (cons "filename" "r.pdf")
                  (cons "supplier" "fake")
                ) ;list
         ) ;entry
         (prev-text
           (json->string (vector (list (cons "id" sha256) (cons "ts" "1")) entry))
         ) ;prev-text
         ;; 预置上月有效缓存，当月无文件
         (ignored1
           (begin
             (path-mkdir prev #t #t)
             (path-write-text (path->string (path-join prev "result.json")) prev-text)
           ) ;begin
         ) ;ignored1
         (orig-put attachment-cache-put!)
         (j-noop
           (dynamic-wind
             (lambda () (set! attachment-cache-put! (lambda (sha name content) #t)))
             (lambda ()
               (string->json (fake-attachment-ingest (make-ingest-line "t-refresh-noop" path)))
             ) ;lambda
             (lambda () (set! attachment-cache-put! orig-put))
           ) ;dynamic-wind
         ) ;j-noop
         ;; no-op 后仍只有上月文件：查找阶段能命中上月缓存
         (found-prev (attachment-extract-cache-find sha256))
         (j-ok (string->json (fake-attachment-ingest (make-ingest-line "t-refresh-ok" path)))
         ) ;j-ok
         (cur-j
           (string->json (path-read-text (path->string (path-join cur "result.json"))))
         ) ;cur-j
        ) ;
    (check found-prev => prev-text)
    ;; no-op 刷新：上月文件在也不得 ready
    (check (json-ref-string j-noop "event" #f) => "error")
    (check (json-ref-string j-noop "taskId" #f) => "t-refresh-noop")
    (check (string-contains? (json-ref-string j-noop "reason" "")
             "附件缓存写入失败"
           ) ;string-contains?
      =>
      #t
    ) ;check
    (check (json-ref-string j-noop "sha256" #f) => #f)
    ;; 正常刷新：ready，当月文件 [0].id 正确、[1] 保留 content/supplier、
    ;; ts 已刷新（不再是预置的 "1"）
    (check (json-ref-string j-ok "event" #f) => "ready")
    (check (json-ref-string (vector-ref cur-j 0) "id" #f) => sha256)
    (check (json-ref-string (vector-ref cur-j 1) "content" #f) => custom)
    (check (json-ref-string (vector-ref cur-j 1) "supplier" #f) => "fake")
    (check (string=? (json-ref-string (vector-ref cur-j 0) "ts" "") "1") => #f)
    (test-clear-extract-cache! sha256)
    (path-unlink path #t)
  ) ;let*
) ;define

;;; ---------- 假 llm 插件：按 sha256 读缓存注入回显 ----------

;; tm-llm 的 fake-llm-attachment-content 即「按 sha256 查缓存」（上面的
;; ready 测试已借它验证缓存落盘）；这里验证 %chat 回复的注入闭环

(define (test-fake-llm-attachment-injection)
  (let* ((path "/tmp/chat-protocol-test-llm.pdf")
         ;; 被 ingest 的文件须真实存在（可访问性预检拒绝不存在路径）
         (ignored0 (path-write-text path "x"))
         (sha256 (fake-attachment-sha256 path))
         (ignored (fake-attachment-ingest (make-ingest-line "t-llm" path)))
         ;; 6233：假插件已写假缓存并 ready，宿主随即删除转换 PDF；
         ;; 假 LLM 只能依靠 extract/result.json 完成后续注入
         (ignored-delete (path-unlink path #t))
         (chat-line
           (make-chat-line (vector (list (cons "sha256" sha256) (cons "filename" "f.pdf")))
           ) ;make-chat-line
         ) ;chat-line
         (reply (fake-llm-chat-reply chat-line))
        ) ;
    (check (path-file? path) => #f)
    (check (string? reply) => #t)
    (check (string-contains? reply "[fake-llm] 我收到了你的消息：hi")
      =>
      #t
    ) ;check
    (check (string-contains? reply (string-append "[fake] attachment " sha256 " injected"))
      =>
      #t
    ) ;check
    ;; 6234：新 supplier 字段不影响 content 注入，且不出现在回显正文
    (check (string-contains? reply "supplier") => #f)
  ) ;let*
) ;define

;; 无附件（空数组）：无注入行，行为与 v1 一致；未知 sha（缓存读取失败）
;; 同样无注入行，既有行为不变

(define (test-fake-llm-no-attachments)
  (check
    (string-contains? (fake-llm-chat-reply (make-chat-line (vector)))
      "[fake] attachment"
    ) ;string-contains?
    =>
    #f
  ) ;check
  (check
    (string-contains?
      (fake-llm-chat-reply
        (make-chat-line (vector (list (cons "sha256" "6234-unknown-sha") (cons "filename" "u.pdf")))
        ) ;make-chat-line
      ) ;fake-llm-chat-reply
      "[fake] attachment"
    ) ;string-contains?
    =>
    #f
  ) ;check
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
             (json->string (list (cons "taskId" "t-enc") (cons "path" herk)))
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

;;; ---------- read-paragraph-by-visible-eof：R7RS read-line 兼容 ----------

(define (test-read-paragraph-eof-marker)
  ;; 18.11.40：read-line 不带行尾换行，helper 需自行补回；
  ;; 18.11.39：行内已带换行，helper 原样拼接。两种语义期望结果相同。
  (with-input-from-string "line1\nline2\n<EOF>\n"
    (lambda () (check (read-paragraph-by-visible-eof) => "line1\nline2\n"))
  ) ;with-input-from-string
  ;; 空段落
  (with-input-from-string "<EOF>\n"
    (lambda () (check (read-paragraph-by-visible-eof) => ""))
  ) ;with-input-from-string
  ;; 空行也保留
  (with-input-from-string "a\n\nb\n<EOF>\n"
    (lambda () (check (read-paragraph-by-visible-eof) => "a\n\nb\n"))
  ) ;with-input-from-string
) ;define

;;; ---------- 测试套件入口 ----------

(tm-define (regtest-chat-protocol)
  (test-json-empty-attachments)
  (test-json-nonempty-attachments)
  (test-chat-tab-send-with-attachments)
  (test-fake-attachment-sha256-deterministic)
  (test-fake-attachment-ingest-ready)
  (test-fake-attachment-ingest-error)
  (test-attachment-host-precheck)
  (test-attachment-host-precheck-unreadable)
  (test-attachment-host-precheck-nonascii-path)
  (test-fake-attachment-ingest-exception-fallback)
  (test-attachment-cache-payload-stored)
  (test-fake-attachment-ingest-put-noop)
  (test-fake-attachment-ingest-inconsistent-cache)
  (test-attachment-ingest-request-json-shape)
  (test-fake-attachment-sha256-key)
  (test-fake-attachment-ingest-source-sha256)
  (test-fake-attachment-ingest-empty-source-falls-back)
  (test-attachment-cache-sweep-and-refresh)
  (test-attachment-hit-refresh-confirmation)
  (test-fake-llm-attachment-injection)
  (test-fake-llm-no-attachments)
  (test-chat-wire-encoding)
  (test-read-paragraph-eof-marker)
  (check-report)
) ;tm-define
