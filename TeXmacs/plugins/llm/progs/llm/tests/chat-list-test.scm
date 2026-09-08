;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-list-test.scm
;; DESCRIPTION : 纯逻辑单元测试：Chat 会话列表持久化（条目构造、增量保存、会话加载与删除）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r chat-list-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii json))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/llm/progs/llm/chat-list.scm")

;;; ---------- 测试环境隔离（沙箱目录） ----------

(define test-sandbox-dir
  (url->system (url-append (url-temp) "test-ai-chat-sessions"))
) ;define

(tm-define (chat-persist-base-dir) test-sandbox-dir)

(define (clean-test-sandbox!)
  (let ((manifest-url (system->url (chat-persist-manifest-path))))
    (when (url-exists? manifest-url)
      (system-remove manifest-url)
    ) ;when
  ) ;let
  (let ((msg-url-2 (system->url (chat-persist-message-path "del-2"))))
    (when (url-exists? msg-url-2)
      (system-remove msg-url-2)
    ) ;when
  ) ;let
  (let ((session-dir-2 (system->url (string-append test-sandbox-dir "/del-2"))))
    (when (url-exists? session-dir-2)
      (system-rmdir session-dir-2)
    ) ;when
  ) ;let
) ;define

;;; ---------- 1. 条目构造测试 ----------

(define (test-make-entry-defaults)
  (let ((entry (chat-persist-make-entry "sid-001" "Title 1" "gpt-4o" #f)))
    (check (json-object? entry) => #t)
    (check (json-ref-string entry "sessionId" #f) => "sid-001")
    (check (json-ref-string entry "title" #f) => "Title 1")
    (check (json-ref-string entry "model" #f) => "gpt-4o")
    (check (json-ref-string entry "archived" #f) => "false")
    (check (string? (json-ref-string entry "createdAt" #f)) => #t)
    (check (json-ref-integer entry "defaultExpandCount" 0) => 5)
    (check (json-ref-string entry "thinking" #f) => "disabled")
    (check (json-ref-string entry "search" #f) => "disabled")
    ;; updateAt 缺省时回退到 createdAt
    (check (json-ref-string entry "updateAt" #f)
      =>
      (json-ref-string entry "createdAt" #f)
    ) ;check
  ) ;let
) ;define

(define (test-make-entry-archived-variants)
  (let ((e1 (chat-persist-make-entry "sid-1" "T1" "M1" #t))
        (e2 (chat-persist-make-entry "sid-2" "T2" "M2" "true"))
        (e3 (chat-persist-make-entry "sid-3" "T3" "M3" "false"))
        (e4 (chat-persist-make-entry "sid-4" "T4" "M4" #f))
       ) ;
    (check (json-ref-string e1 "archived" #f) => "true")
    (check (json-ref-string e2 "archived" #f) => "true")
    (check (json-ref-string e3 "archived" #f) => "false")
    (check (json-ref-string e4 "archived" #f) => "false")
  ) ;let
) ;define

(define (test-make-entry-with-options)
  ;; 传入 rest 参数: created-at thinking search updated-at
  (let ((entry (chat-persist-make-entry "sid-opt" "Deep Thinking" "deepseek-r1"
                 #f "1700000000" "enabled" "enabled" "1800000000"
               ) ;chat-persist-make-entry
        ) ;entry
       ) ;
    (check (json-object? entry) => #t)
    (check (json-ref-string entry "sessionId" #f) => "sid-opt")
    (check (json-ref-string entry "createdAt" #f) => "1700000000")
    (check (json-ref-string entry "thinking" #f) => "enabled")
    (check (json-ref-string entry "search" #f) => "enabled")
    (check (json-ref-string entry "updateAt" #f) => "1800000000")
  ) ;let
) ;define

(define (test-make-entry-json-serialization)
  (let* ((entry (chat-persist-make-entry "sid-json" "Serial Test" "model-x" #f
                  "1700000000" "disabled" "disabled" "1700000000"
                ) ;chat-persist-make-entry
         ) ;entry
         (json-str (json->string entry))
         (parsed (string->json json-str))
        ) ;
    (check (json-object? parsed) => #t)
    (check (json-ref-string parsed "sessionId" #f) => "sid-json")
    (check (json-ref-string parsed "title" #f) => "Serial Test")
    (check (json-ref-string parsed "model" #f) => "model-x")
    (check (json-ref-integer parsed "defaultExpandCount" 0) => 5)
  ) ;let*
) ;define

;;; ---------- 2. 增量保存与更新测试 ----------

(define (test-manifest-create-and-update)
  (clean-test-sandbox!)
  (let ((manifest-path (chat-persist-manifest-path)))
    ;; 1. 初次写入：manifest 不存在，应自动创建新文件
    (chat-persist-update-manifest "sid-1" "Title 1" "model-a" #f "100"
      "disabled" "disabled" "100"
    ) ;chat-persist-update-manifest
    (check (file-exists? manifest-path) => #t)

    (let* ((parsed (string->json (string-load (system->url manifest-path))))
           (sessions (and (json-object? parsed) (json-ref parsed "sessions")))
          ) ;
      (check (json-ref-integer parsed "version" 0) => 1)
      (check (vector? sessions) => #t)
      (check (vector-length sessions) => 1)
      (let ((s0 (vector-ref sessions 0)))
        (check (json-ref-string s0 "sessionId" #f) => "sid-1")
        (check (json-ref-string s0 "title" #f) => "Title 1")
      ) ;let
    ) ;let*

    ;; 2. 追加第二条记录：sessions 长度应变为 2
    (chat-persist-update-manifest "sid-2" "Title 2" "model-b" #t "200" "enabled"
      "disabled" "200"
    ) ;chat-persist-update-manifest
    (let* ((parsed (string->json (string-load (system->url manifest-path))))
           (sessions (json-ref parsed "sessions"))
          ) ;
      (check (vector-length sessions) => 2)
      (let ((s0 (vector-ref sessions 0)) (s1 (vector-ref sessions 1)))
        (check (json-ref-string s0 "sessionId" #f) => "sid-1")
        (check (json-ref-string s1 "sessionId" #f) => "sid-2")
        (check (json-ref-string s1 "archived" #f) => "true")
        (check (json-ref-string s1 "thinking" #f) => "enabled")
      ) ;let
    ) ;let*

    ;; 3. 更新第一条记录：同 sessionId，原地更新，总数保持 2
    (chat-persist-update-manifest "sid-1" "Title 1 Updated" "model-a" #f "100"
      "disabled" "disabled" "300"
    ) ;chat-persist-update-manifest
    (let* ((parsed (string->json (string-load (system->url manifest-path))))
           (sessions (json-ref parsed "sessions"))
          ) ;
      (check (vector-length sessions) => 2)
      (let ((s0 (vector-ref sessions 0)) (s1 (vector-ref sessions 1)))
        (check (json-ref-string s0 "sessionId" #f) => "sid-1")
        (check (json-ref-string s0 "title" #f) => "Title 1 Updated")
        (check (json-ref-string s0 "updateAt" #f) => "300")
        (check (json-ref-string s1 "sessionId" #f) => "sid-2")
      ) ;let
    ) ;let*
  ) ;let
  (clean-test-sandbox!)
) ;define

;;; ---------- 3. 会话恢复与读取测试 ----------

(define restored-sessions '())

(tm-define (qt-chat-tab-restore-session sid title model archived createdAt
             updateAt expandCount thinking search
           ) ;qt-chat-tab-restore-session
  (set! restored-sessions
    (cons (list sid title model archived createdAt updateAt expandCount thinking search)
      restored-sessions
    ) ;cons
  ) ;set!
) ;tm-define

(define (test-load-all-normal)
  (clean-test-sandbox!)
  (set! restored-sessions '())
  ;; 保存两条会话
  (chat-persist-update-manifest "load-sid-1" "LTitle 1" "gpt" #f "1000"
    "disabled" "disabled" "1000"
  ) ;chat-persist-update-manifest
  (chat-persist-update-manifest "load-sid-2" "LTitle 2" "claude" #t "2000"
    "enabled" "enabled" "3000"
  ) ;chat-persist-update-manifest

  (chat-persist-load-all)

  (check (length restored-sessions) => 2)
  ;; sessions 在 load-all 中按原序遍历调用
  (let ((s2 (assoc "load-sid-2" restored-sessions))
        (s1 (assoc "load-sid-1" restored-sessions))
       ) ;
    (check (pair? s1) => #t)
    (check (pair? s2) => #t)
    (check (list-ref s1 1) => "LTitle 1")
    (check (list-ref s1 2) => "gpt")
    (check (list-ref s1 3) => "false")
    (check (list-ref s1 4) => "1000")
    (check (list-ref s1 5) => "1000")
    (check (list-ref s1 6) => 5)
    (check (list-ref s1 7) => "disabled")
    (check (list-ref s1 8) => "disabled")

    (check (list-ref s2 1) => "LTitle 2")
    (check (list-ref s2 2) => "claude")
    (check (list-ref s2 3) => "true")
    (check (list-ref s2 4) => "2000")
    (check (list-ref s2 5) => "3000")
    (check (list-ref s2 7) => "enabled")
    (check (list-ref s2 8) => "enabled")
  ) ;let
  (clean-test-sandbox!)
) ;define

(define (test-load-all-legacy-compatibility)
  ;; 验证对缺少 updateAt / defaultExpandCount / thinking / search 的历史 manifest 的兼容回退
  (clean-test-sandbox!)
  (chat-persist-ensure-dir! test-sandbox-dir)
  (set! restored-sessions '())

  (let ((legacy-manifest-str "{\"version\":1,\"sessions\":[{\"sessionId\":\"legacy-1\",\"title\":\"Old Session\",\"model\":\"gpt-3.5\",\"archived\":\"false\",\"createdAt\":\"500\"}]}"
        ) ;legacy-manifest-str
       ) ;
    (string-save legacy-manifest-str (system->url (chat-persist-manifest-path)))
  ) ;let

  (chat-persist-load-all)

  (check (length restored-sessions) => 1)
  (let ((s (car restored-sessions)))
    (check (list-ref s 0) => "legacy-1")
    (check (list-ref s 1) => "Old Session")
    ;; updateAt 缺失时应回退到 createdAt
    (check (list-ref s 5) => "500")
    ;; defaultExpandCount 缺失时缺省为 5
    (check (list-ref s 6) => 5)
    ;; thinking / search 缺失时缺省为 "disabled"
    (check (list-ref s 7) => "disabled")
    (check (list-ref s 8) => "disabled")
  ) ;let
  (clean-test-sandbox!)
) ;define

(define (test-load-all-missing-and-corrupted)
  (clean-test-sandbox!)
  (set! restored-sessions '())
  ;; 1. manifest 文件不存在时，安全返回且不崩溃
  (chat-persist-load-all)
  (check (length restored-sessions) => 0)

  ;; 2. manifest 内容损坏时，安全捕获且不崩溃
  (chat-persist-ensure-dir! test-sandbox-dir)
  (string-save "{invalid-json-content..."
    (system->url (chat-persist-manifest-path))
  ) ;string-save
  (chat-persist-load-all)
  (check (length restored-sessions) => 0)
  (clean-test-sandbox!)
) ;define

;;; ---------- 4. 会话删除测试 ----------

(define (test-delete-one)
  (clean-test-sandbox!)
  (let ((manifest-path (chat-persist-manifest-path)))
    (chat-persist-update-manifest "del-1" "To Delete" "m1" #f)
    (chat-persist-update-manifest "del-2" "To Keep" "m2" #f)

    ;; 模拟创建会话目录及 message.tmu 文件
    (let ((msg-path-1 (chat-persist-message-path "del-1"))
          (msg-path-2 (chat-persist-message-path "del-2"))
         ) ;
      (chat-persist-ensure-dir! (chat-persist-parent-dir msg-path-1))
      (chat-persist-ensure-dir! (chat-persist-parent-dir msg-path-2))
      (string-save "test message 1" (system->url msg-path-1))
      (string-save "test message 2" (system->url msg-path-2))

      (check (file-exists? msg-path-1) => #t)
      (check (file-exists? msg-path-2) => #t)

      ;; 执行删除 del-1
      (chat-persist-delete-one "del-1")

      ;; 验证 del-1 的文件被删除，del-2 依然存在
      (check (file-exists? msg-path-1) => #f)
      (check (file-exists? msg-path-2) => #t)

      ;; 验证 manifest 中 del-1 被移除，del-2 保留
      (let* ((parsed (string->json (string-load (system->url manifest-path))))
             (sessions (json-ref parsed "sessions"))
            ) ;
        (check (vector-length sessions) => 1)
        (let ((s0 (vector-ref sessions 0)))
          (check (json-ref-string s0 "sessionId" #f) => "del-2")
        ) ;let
      ) ;let*

      ;; 幂等性：重复删除已删除的 session 不报错
      (chat-persist-delete-one "del-1")
      (chat-persist-delete-one "non-existent-sid")
    ) ;let
  ) ;let
  (clean-test-sandbox!)
) ;define

;;; ---------- 测试套件入口 ----------

(tm-define (regtest-chat-list)
  (test-make-entry-defaults)
  (test-make-entry-archived-variants)
  (test-make-entry-with-options)
  (test-make-entry-json-serialization)
  (test-manifest-create-and-update)
  (test-load-all-normal)
  (test-load-all-legacy-compatibility)
  (test-load-all-missing-and-corrupted)
  (test-delete-one)
  (check-report)
) ;tm-define
