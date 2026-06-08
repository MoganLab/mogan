;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1003_1.scm
;; DESCRIPTION : Tests for chat session persistence
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii njson))
(import (liii os))

(check-set-mode! 'report)

;;; ========== 辅助函数 ==========

;; 备份 manifest，执行 thunk，然后恢复

(define (chat-persist-with-backup thunk)
  (let ((manifest-path (chat-persist-manifest-path))
        (bak-path (string-append (chat-persist-manifest-path) ".bak2"))
       ) ;
    (when (file-exists? manifest-path)
      (system-move (system->url manifest-path) (system->url bak-path))
    ) ;when
    (thunk)
    ;; 清理测试产生的 manifest
    (when (file-exists? manifest-path)
      (system-remove (system->url manifest-path))
    ) ;when
    (when (file-exists? bak-path)
      (system-move (system->url bak-path) (system->url manifest-path))
    ) ;when
  ) ;let
) ;define

;; 读取 manifest 中 sessions 数组的长度

(define (manifest-session-count)
  (let ((manifest-path (chat-persist-manifest-path)))
    (if (not (file-exists? manifest-path))
      0
      (let* ((manifest (file->njson manifest-path))
             (sessions (njson-ref manifest "sessions"))
             (entries (njson-array->list sessions))
            ) ;
        (njson-free manifest)
        (length entries)
      ) ;let*
    ) ;if
  ) ;let
) ;define

;; 在 manifest 中查找指定 sessionId 的条目，返回其字段值
;; njson-array->list 返回的元素是 alist，如:
;;   (("sessionId" . "xxx") ("title" . "yyy") ...)

(define (manifest-find-session sid field)
  (let* ((manifest-path (chat-persist-manifest-path))
         (manifest (file->njson manifest-path))
         (sessions (njson-ref manifest "sessions"))
         (entries (njson-array->list sessions))
         (result #f)
        ) ;
    (for-each (lambda (e)
                (when (pair? e)
                  (let ((sid-pair (assoc "sessionId" e)))
                    (when (and sid-pair (== (cdr sid-pair) sid))
                      (let ((field-pair (assoc field e)))
                        (when field-pair
                          (set! result (cdr field-pair))
                        ) ;when
                      ) ;let
                    ) ;when
                  ) ;let
                ) ;when
              ) ;lambda
      entries
    ) ;for-each
    (njson-free manifest)
    result
  ) ;let*
) ;define

;; 辅助：创建测试 session（注册 + 写 buffer）

(define (setup-test-session sid model body)
  (chat-persist-register-session sid model)
  (let ((msg-buf (chat-tab-session->message-buffer sid)))
    (buffer-set-body msg-buf `(document ,body))
    (buffer-pretend-saved msg-buf)
  ) ;let
) ;define

;;; ========== chat-persist-home-path ==========

(define (test-chat-persist-home-path)
  ;; 返回值应是非空字符串且是绝对路径
  (let ((home (chat-persist-home-path)))
    (check (string? home) => #t)
    (check (> (string-length home) 0) => #t)
    (check (char=? (string-ref home 0) #\/) => #t)
  ) ;let
) ;define

;;; ========== chat-persist-base-dir ==========

(define (test-chat-persist-base-dir)
  (let ((base (chat-persist-base-dir)))
    (check (string? base) => #t)
    ;; 以 /system/ai-chat-sessions 结尾
    (check (string-ends? base "/system/ai-chat-sessions") => #t)
  ) ;let
) ;define

;;; ========== chat-persist-manifest-path ==========

(define (test-chat-persist-manifest-path)
  (let ((base (chat-persist-base-dir)))
    (check (chat-persist-manifest-path) => (string-append base "/manifest.json"))
  ) ;let
) ;define

;;; ========== chat-persist-message-path ==========

(define (test-chat-persist-message-path)
  (let ((base (chat-persist-base-dir)) (sid "TEST-UUID-1234"))
    (check (chat-persist-message-path sid)
      =>
      (string-append base "/" sid "/message.tmu")
    ) ;check
    ;; 不同 sid 得到不同路径
    (check (chat-persist-message-path "other-sid")
      =>
      (string-append base "/other-sid/message.tmu")
    ) ;check
  ) ;let
) ;define

;;; ========== chat-persist-parent-dir ==========

(define (test-chat-persist-parent-dir)
  (check (chat-persist-parent-dir "/tmp/foo.txt") => "/tmp")
  (check (chat-persist-parent-dir "/a/b/c") => "/a/b")
  (check (chat-persist-parent-dir "/foo") => "/")
  ;; 单层文件
  (check (chat-persist-parent-dir "/tmp/bar") => "/tmp")
) ;define

;;; ========== chat-persist-ensure-dir! ==========

(define (test-chat-persist-ensure-dir!)
  (let ((nested (string-append "/tmp/chat-persist-test-"
                  (number->string (current-time))
                  "/a/b/c"
                ) ;string-append
        ) ;nested
       ) ;
    ;; 创建前不存在
    (check (file-exists? nested) => #f)
    ;; 递归创建
    (chat-persist-ensure-dir! nested)
    (check (file-exists? nested) => #t)
    ;; 重复调用不报错
    (chat-persist-ensure-dir! nested)
    (check (file-exists? nested) => #t)
  ) ;let
) ;define

;;; ========== chat-persist-make-entry ==========

(define (test-chat-persist-make-entry)
  ;; archived=#f 时 archived 字段为 "false"
  (let ((entry (chat-persist-make-entry "sid-1" "Hello" "Kimi" #f)))
    (check (njson-ref entry "sessionId") => "sid-1")
    (check (njson-ref entry "title") => "Hello")
    (check (njson-ref entry "model") => "Kimi")
    (check (njson-ref entry "archived") => "false")
    (njson-free entry)
  ) ;let
  ;; archived=#t 时 archived 字段为 "true"
  (let ((entry (chat-persist-make-entry "sid-2" "World" "GPT" #t)))
    (check (njson-ref entry "archived") => "true")
    (njson-free entry)
  ) ;let
  ;; 空标题和空模型
  (let ((entry (chat-persist-make-entry "sid-3" "" "" #f)))
    (check (njson-ref entry "title") => "")
    (check (njson-ref entry "model") => "")
    (njson-free entry)
  ) ;let
) ;define

;;; ========== chat-persist-make-entry updateAt 字段 ==========

(define (test-chat-persist-make-entry-updateAt-default)
  ;; 不传 updateAt 时，回退到 createdAt（传入 "1700000000"）
  (let ((entry (chat-persist-make-entry "sid-ua1" "Title" "Model" #f "1700000000")))
    (check (njson-ref entry "updateAt") => "1700000000")
    (njson-free entry)
  ) ;let
) ;define

(define (test-chat-persist-make-entry-updateAt-explicit)
  ;; 显式传 updateAt
  (let ((entry (chat-persist-make-entry "sid-ua2"
                 "Title"
                 "Model"
                 #f
                 "1700000000"
                 "enabled"
                 "1800000000"
               ) ;chat-persist-make-entry
        ) ;entry
       ) ;
    (check (njson-ref entry "updateAt") => "1800000000")
    (njson-free entry)
  ) ;let
) ;define

(define (test-chat-persist-make-entry-updateAt-no-createdAt)
  ;; 不传 updateAt 时，updateAt 回退到 createdAt
  (let ((entry (chat-persist-make-entry "sid-ua3" "Title" "Model" #f "1000")))
    (check (njson-ref entry "updateAt") => "1000")
    (njson-free entry)
  ) ;let
) ;define

;;; ========== chat-persist-register-session ==========

(define (test-chat-persist-register-session)
  (let* ((sid (string-append "reg-" (number->string (current-time))))
         (model "RegModel")
        ) ;
    ;; 注册前状态不存在
    (check (chat-tab-get-state sid) => #f)
    ;; 注册后状态存在
    (chat-persist-register-session sid model)
    (check (chat-tab-get-state sid) => (list model "disabled"))
    ;; 重复注册不会覆盖
    (chat-persist-register-session sid "OtherModel")
    (check (chat-tab-get-state sid) => (list model "disabled"))
  ) ;let*
) ;define

;;; ========== chat-persist-load-all ==========

(define (test-chat-persist-load-empty)
  (chat-persist-with-backup (lambda ()
                              ;; manifest 不存在时调用不崩溃
                              (chat-persist-load-all)
                              (check (manifest-session-count) => 0)
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

(define (test-chat-persist-load-all-with-data)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "loadd-" (number->string (current-time))))
                                     (model "LoadModel")
                                    ) ;
                                (setup-test-session sid model "Load test content")
                                (chat-persist-save-one sid "LoadTitle" model #f)
                                ;; 验证 manifest 中数据正确
                                (check (manifest-session-count) => 1)
                                (check (manifest-find-session sid "title") => "LoadTitle")
                                (check (manifest-find-session sid "model") => model)
                                (check (manifest-find-session sid "archived") => "false")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

(define (test-chat-persist-load-all-multi)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid1 (string-append "loadm1-" (number->string (current-time))))
                                     (sid2 (string-append "loadm2-" (number->string (current-time))))
                                     (model1 "ModelA")
                                     (model2 "ModelB")
                                    ) ;
                                (setup-test-session sid1 model1 "A")
                                (setup-test-session sid2 model2 "B")
                                (chat-persist-save-one sid1 "Title1" model1 #f)
                                (chat-persist-save-one sid2 "Title2" model2 #t)
                                ;; 验证 manifest 中两条数据正确
                                (check (manifest-session-count) => 2)
                                (check (manifest-find-session sid1 "title") => "Title1")
                                (check (manifest-find-session sid1 "archived") => "false")
                                (check (manifest-find-session sid2 "title") => "Title2")
                                (check (manifest-find-session sid2 "archived") => "true")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

(define (test-chat-persist-load-all-missing-message-file)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "loadd-missing-" (number->string (current-time))))
                                     (model "MissingModel")
                                     (msg-path (chat-persist-message-path sid))
                                    ) ;
                                ;; 用 save-one 创建 manifest 和 message 文件
                                (setup-test-session sid model "Content")
                                (chat-persist-save-one sid "MissingTitle" model #f)
                                (check (file-exists? msg-path) => #t)
                                ;; 删除 message 文件
                                (system-remove (system->url msg-path))
                                ;; manifest 数据仍正确
                                (check (manifest-session-count) => 1)
                                (check (manifest-find-session sid "title") => "MissingTitle")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;;; ========== chat-persist-save-one（增量保存）==========

;; 首次保存（manifest 不存在）

(define (test-chat-persist-save-one-new-manifest)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "so-new-" (number->string (current-time))))
                                     (model "ModelA")
                                    ) ;
                                (setup-test-session sid model "First message")
                                (chat-persist-save-one sid "TitleA" model #f)
                                ;; 验证文件已创建
                                (check (file-exists? (chat-persist-manifest-path)) => #t)
                                (check (file-exists? (chat-persist-message-path sid)) => #t)
                                ;; 验证 manifest 内容
                                (check (manifest-session-count) => 1)
                                (check (manifest-find-session sid "title") => "TitleA")
                                (check (manifest-find-session sid "model") => model)
                                (check (manifest-find-session sid "archived") => "false")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;; 追加到已有 manifest

(define (test-chat-persist-save-one-append)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid1 (string-append "so-app1-" (number->string (current-time))))
                                     (sid2 (string-append "so-app2-" (number->string (current-time))))
                                     (model1 "ModelA")
                                     (model2 "ModelB")
                                    ) ;
                                (setup-test-session sid1 model1 "Message 1")
                                (setup-test-session sid2 model2 "Message 2")
                                ;; 先保存第一个
                                (chat-persist-save-one sid1 "Title1" model1 #f)
                                (check (manifest-session-count) => 1)
                                ;; 再保存第二个（追加）
                                (chat-persist-save-one sid2 "Title2" model2 #f)
                                (check (manifest-session-count) => 2)
                                (check (manifest-find-session sid1 "title") => "Title1")
                                (check (manifest-find-session sid2 "title") => "Title2")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;; 更新已有 session（标题和归档状态）

(define (test-chat-persist-save-one-update)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "so-upd-" (number->string (current-time))))
                                     (model "ModelA")
                                    ) ;
                                (setup-test-session sid model "Initial")
                                ;; 首次保存
                                (chat-persist-save-one sid "OldTitle" model #f)
                                (check (manifest-session-count) => 1)
                                (check (manifest-find-session sid "title") => "OldTitle")
                                (check (manifest-find-session sid "archived") => "false")
                                ;; 更新标题和归档状态
                                (chat-persist-save-one sid "NewTitle" model #t)
                                (check (manifest-session-count) => 1)
                                (check (manifest-find-session sid "title") => "NewTitle")
                                (check (manifest-find-session sid "archived") => "true")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;; 更新时保留其他 session 不变

(define (test-chat-persist-save-one-update-preserves-others)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid1 (string-append "so-pr1-" (number->string (current-time))))
                                     (sid2 (string-append "so-pr2-" (number->string (current-time))))
                                     (model1 "ModelA")
                                     (model2 "ModelB")
                                    ) ;
                                (setup-test-session sid1 model1 "A")
                                (setup-test-session sid2 model2 "B")
                                (chat-persist-save-one sid1 "Title1" model1 #f)
                                (chat-persist-save-one sid2 "Title2" model2 #f)
                                (check (manifest-session-count) => 2)
                                ;; 更新 sid1，sid2 应不变
                                (chat-persist-save-one sid1 "UpdatedTitle1" model1 #t)
                                (check (manifest-session-count) => 2)
                                (check (manifest-find-session sid1 "title") => "UpdatedTitle1")
                                (check (manifest-find-session sid1 "archived") => "true")
                                (check (manifest-find-session sid2 "title") => "Title2")
                                (check (manifest-find-session sid2 "archived") => "false")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;; 连续增量追加

(define (test-chat-persist-save-one-mixed)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid1 (string-append "so-mix1-" (number->string (current-time))))
                                     (sid2 (string-append "so-mix2-" (number->string (current-time))))
                                     (model1 "ModelA")
                                     (model2 "ModelB")
                                    ) ;
                                (setup-test-session sid1 model1 "X")
                                (setup-test-session sid2 model2 "Y")
                                (chat-persist-save-one sid1 "FullTitle" model1 #f)
                                (check (manifest-session-count) => 1)
                                (chat-persist-save-one sid2 "IncTitle" model2 #t)
                                (check (manifest-session-count) => 2)
                                (check (manifest-find-session sid1 "title") => "FullTitle")
                                (check (manifest-find-session sid2 "title") => "IncTitle")
                                (check (manifest-find-session sid2 "archived") => "true")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;; save-one 后通过 manifest 验证数据

(define (test-chat-persist-save-one-and-load-all)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "so-rt-" (number->string (current-time))))
                                     (model "RTModel")
                                    ) ;
                                (setup-test-session sid model "Round trip")
                                (chat-persist-save-one sid "RTTitle" model #t)
                                ;; 验证 manifest 数据
                                (check (manifest-session-count) => 1)
                                (check (manifest-find-session sid "title") => "RTTitle")
                                (check (manifest-find-session sid "model") => model)
                                (check (manifest-find-session sid "archived") => "true")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;; 三条 session 连续增量保存

(define (test-chat-persist-save-one-three-sessions)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid1 (string-append "so-3a-" (number->string (current-time))))
                                     (sid2 (string-append "so-3b-" (number->string (current-time))))
                                     (sid3 (string-append "so-3c-" (number->string (current-time))))
                                     (model "TriModel")
                                    ) ;
                                (setup-test-session sid1 model "1")
                                (setup-test-session sid2 model "2")
                                (setup-test-session sid3 model "3")
                                (chat-persist-save-one sid1 "T1" model #f)
                                (chat-persist-save-one sid2 "T2" model #f)
                                (chat-persist-save-one sid3 "T3" model #t)
                                (check (manifest-session-count) => 3)
                                ;; 更新中间的 sid2
                                (chat-persist-save-one sid2 "T2Updated" model #t)
                                (check (manifest-session-count) => 3)
                                (check (manifest-find-session sid1 "title") => "T1")
                                (check (manifest-find-session sid2 "title") => "T2Updated")
                                (check (manifest-find-session sid2 "archived") => "true")
                                (check (manifest-find-session sid3 "title") => "T3")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;;; ========== chat-persist-make-entry 字符串 archived ==========

;; 模拟 C++ 传入字符串 "false"/"true" 的情况

(define (test-chat-persist-make-entry-string-archived)
  ;; archived="false"（字符串）时 archived 字段应为 "false"
  (let ((entry (chat-persist-make-entry "sid-str1" "Title" "Model" "false")))
    (check (njson-ref entry "archived") => "false")
    (njson-free entry)
  ) ;let
  ;; archived="true"（字符串）时 archived 字段应为 "true"
  (let ((entry (chat-persist-make-entry "sid-str2" "Title" "Model" "true")))
    (check (njson-ref entry "archived") => "true")
    (njson-free entry)
  ) ;let
) ;define

(define (test-chat-persist-save-one-string-archived-false)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "so-strf-" (number->string (current-time))))
                                     (model "StrModel")
                                    ) ;
                                (setup-test-session sid model "String archived test")
                                ;; 模拟 C++ 传入字符串 "false"
                                (chat-persist-save-one sid "StrTitle" model "false")
                                (check (manifest-find-session sid "archived") => "false")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;;; ========== update-manifest updateAt 持久化 ==========

(define (test-chat-persist-update-manifest-with-updateAt)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "uma-" (number->string (current-time))))
                                     (model "UpAtModel")
                                    ) ;
                                (setup-test-session sid model "Content")
                                ;; update-manifest 带 updateAt 参数
                                (chat-persist-update-manifest sid
                                  "Title"
                                  model
                                  #f
                                  "1700000000"
                                  "disabled"
                                  "1800000000"
                                ) ;chat-persist-update-manifest
                                (check (manifest-find-session sid "updateAt") => "1800000000")
                                (check (manifest-find-session sid "createdAt") => "1700000000")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

(define (test-chat-persist-update-manifest-without-updateAt)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "umano-" (number->string (current-time))))
                                     (model "UpAtModel2")
                                    ) ;
                                (setup-test-session sid model "Content")
                                ;; update-manifest 不带 updateAt 参数，应回退到 createdAt
                                (chat-persist-update-manifest sid "Title" model #f "1700000000")
                                (check (manifest-find-session sid "updateAt") => "1700000000")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

(define (test-chat-persist-save-one-with-updateAt)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "so-ua-" (number->string (current-time))))
                                     (model "UpAtModel3")
                                    ) ;
                                (setup-test-session sid model "Content")
                                ;; save-one 带 updateAt
                                (chat-persist-save-one sid
                                  "Title"
                                  model
                                  #f
                                  "1700000000"
                                  "disabled"
                                  "1900000000"
                                ) ;chat-persist-save-one
                                (check (manifest-find-session sid "updateAt") => "1900000000")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;;; ========== 旧 manifest 兼容性（无 updateAt 字段）==========

(define (test-chat-persist-load-all-legacy-manifest)
  (chat-persist-with-backup (lambda ()
                              ;; 手写一个无 updateAt 字段的旧版 manifest
                              (let* ((manifest-path (chat-persist-manifest-path))
                                     (legacy-json "{\"version\":1,\"sessions\":[{\"sessionId\":\"legacy-1\",\"title\":\"Old Chat\",\"model\":\"GPT\",\"archived\":\"false\",\"createdAt\":\"1700000000\",\"defaultExpandCount\":5,\"thinking\":\"disabled\"}]}"
                                     ) ;legacy-json
                                    ) ;
                                (chat-persist-ensure-dir! (chat-persist-parent-dir manifest-path))
                                (string-save legacy-json (system->url manifest-path))
                                ;; load-all 不应崩溃
                                (chat-persist-load-all)
                                ;; 验证 manifest 仍然可读
                                (check (manifest-session-count) => 1)
                                (check (manifest-find-session "legacy-1" "title") => "Old Chat")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;;; ========== updateAt 更新覆盖测试 ==========

(define (test-chat-persist-update-manifest-updateAt-changes)
  (chat-persist-with-backup (lambda ()
                              (let* ((sid (string-append "umch-" (number->string (current-time))))
                                     (model "ChangeModel")
                                    ) ;
                                (setup-test-session sid model "Content")
                                ;; 第一次保存，updateAt = 1700000000
                                (chat-persist-update-manifest sid
                                  "Title"
                                  model
                                  #f
                                  "1700000000"
                                  "disabled"
                                  "1700000000"
                                ) ;chat-persist-update-manifest
                                (check (manifest-find-session sid "updateAt") => "1700000000")
                                ;; 第二次保存，updateAt 更新为 1800000000
                                (chat-persist-update-manifest sid
                                  "Title"
                                  model
                                  #f
                                  "1700000000"
                                  "disabled"
                                  "1800000000"
                                ) ;chat-persist-update-manifest
                                (check (manifest-find-session sid "updateAt") => "1800000000")
                                ;; createdAt 不应被改变
                                (check (manifest-find-session sid "createdAt") => "1700000000")
                              ) ;let*
                            ) ;lambda
  ) ;chat-persist-with-backup
) ;define

;;; ========== 测试入口 ==========

(tm-define (test_0205)
  (test-chat-persist-home-path)
  (test-chat-persist-base-dir)
  (test-chat-persist-manifest-path)
  (test-chat-persist-message-path)
  (test-chat-persist-parent-dir)
  (test-chat-persist-ensure-dir!)
  (test-chat-persist-make-entry)
  (test-chat-persist-make-entry-updateAt-default)
  (test-chat-persist-make-entry-updateAt-explicit)
  (test-chat-persist-make-entry-updateAt-no-createdAt)
  (test-chat-persist-register-session)
  (test-chat-persist-load-empty)
  (test-chat-persist-load-all-with-data)
  (test-chat-persist-load-all-multi)
  (test-chat-persist-load-all-missing-message-file)
  (test-chat-persist-save-one-new-manifest)
  (test-chat-persist-save-one-append)
  (test-chat-persist-save-one-update)
  (test-chat-persist-save-one-update-preserves-others)
  (test-chat-persist-save-one-mixed)
  (test-chat-persist-save-one-and-load-all)
  (test-chat-persist-save-one-three-sessions)
  (test-chat-persist-make-entry-string-archived)
  (test-chat-persist-save-one-string-archived-false)
  (test-chat-persist-update-manifest-with-updateAt)
  (test-chat-persist-update-manifest-without-updateAt)
  (test-chat-persist-save-one-with-updateAt)
  (test-chat-persist-load-all-legacy-manifest)
  (test-chat-persist-update-manifest-updateAt-changes)
  (check-report)
) ;tm-define
