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

(check-set-mode! 'report-failed)

;;; ---------- connection-defined? 间谍 ----------

;; 测试环境装有 llm 插件（连接已定义），会走真实 feed 路径并拉起 goldfish
;; 子进程。必须在模块加载前定义间谍，使 chat-tab-send 走「连接未定义」
;; 回显分支（s7 对 glue 函数的引用在模块加载时静态绑定，见 chat-list-test）

(tm-define (connection-defined? name) #f)

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

;; 非空 attachments：每项 sha256/filename/size 三键，不含本地路径

(define (test-json-nonempty-attachments)
  (let* ((attachments-in (list (list (cons "sha256" "deadbeef") (cons "filename" "b.pdf") (cons "size" 1024))
                         ) ;list
         ) ;attachments-in
         (ctx (make-ctx '(document "hi") attachments-in))
         (parsed (string->json (json->string (chat-input->json ctx))))
         (attachments (json-ref parsed "attachments"))
        ) ;
    (check (vector-length attachments) => 1)
    (let ((att0 (vector-ref attachments 0)))
      (check (json-ref-string att0 "sha256" #f) => "deadbeef")
      (check (json-ref-string att0 "filename" #f) => "b.pdf")
      (check (json-ref-integer att0 "size" 0) => 1024)
      (check (json-ref-string att0 "path" #f) => #f)
    ) ;let
  ) ;let*
) ;define

;;; ---------- chat-tab-send（连接未定义分支回显） ----------

;; 递归查 stree 中是否出现指定文本（chat-tab-flatten-stree 为模块私有）

(define (stree-contains-text? t sub)
  (cond ((string? t) (string-contains? t sub))
        ((pair? t)
         (let loop
           ((rest (cdr t)))
           (if (null? rest)
             #f
             (or (stree-contains-text? (car rest) sub) (loop (cdr rest)))
           ) ;if
         ) ;let
        ) ;
        (else #f)
  ) ;cond
) ;define

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
        (list (list (cons "sha256" "deadbeef") (cons "filename" "b.pdf") (cons "size" 1024))
        ) ;list
      ) ;chat-tab-send
      =>
      #t
    ) ;check
    (check (stree-contains-text? (tm->stree (buffer-get-body msg-buf)) "hello v2")
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
         (line
           (string-append "%attachment "
             (json->string (list (cons "taskId" task-id) (cons "path" path)))
           ) ;string-append
         ) ;line
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
  ) ;let*
) ;define

(define (test-fake-attachment-ingest-error)
  (let ((j (string->json (fake-attachment-ingest "%attachment not-json"))))
    (check (json-ref-string j "event" #f) => "error")
    (check (string-contains? (json-ref-string j "reason" "") "invalid") => #t)
  ) ;let
) ;define

;;; ---------- 假 llm 插件：按 sha256 读缓存注入回显 ----------

;; tm-llm 的 fake-llm-attachment-content 即「按 sha256 查缓存」（上面的
;; ready 测试已借它验证缓存落盘）；这里验证 %chat 回复的注入闭环

(define (test-fake-llm-attachment-injection)
  (let* ((path "/tmp/chat-protocol-test-llm.pdf")
         (sha256 (fake-attachment-sha256 path))
         (ignored
           (fake-attachment-ingest
             (string-append "%attachment "
               (json->string (list (cons "taskId" "t-llm") (cons "path" path)))
             ) ;string-append
           ) ;fake-attachment-ingest
         ) ;ignored
         (chat-line
           (string-append "%chat "
             (json->string
               (list (cons "sessionId" "s1")
                 (cons "content" "hi")
                 (cons "params" (list (cons "model" "m")))
                 (cons "attachments"
                   (vector (list (cons "sha256" sha256) (cons "filename" "f.pdf") (cons "size" 1)))
                 ) ;cons
               ) ;list
             ) ;json->string
           ) ;string-append
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
  (let ((reply
          (fake-llm-chat-reply
            (string-append "%chat "
              (json->string
                (list (cons "sessionId" "s1")
                  (cons "content" "hi")
                  (cons "params" (list (cons "model" "m")))
                  (cons "attachments" (vector))
                ) ;list
              ) ;json->string
            ) ;string-append
          ) ;fake-llm-chat-reply
        ) ;reply
       ) ;
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
  (test-fake-llm-attachment-injection)
  (test-fake-llm-no-attachments)
  (check-report)
) ;tm-define
