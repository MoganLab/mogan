;; IMPORTANT:
;; 先 `node tools/loro-server/server.js` 启动服务端，再跑本测试：
;;   MOGAN_TEST_GUI=1 xmake r 7001_collab_url
;;
;; [7001] 验证协作 buffer 的 URL 改造（端到端）：经 scheme 入口
;; collab-new-document-named 创建的协作 buffer，become_ready 后其 URL 必须是
;; tmfs://collab/<doc_id>（占位 URL 改名而来），且 collab-buffer? 谓词返回 #t。
;;   必须走 scheme 入口——裸 new-buffer + loro-collab-create 不经 buffer-rename，
;; URL 仍是 scratch 路径，测不到本改造。断言在异步轮询链里，须 MOGAN_TEST_GUI=1
;; 才执行（headless 启动即 quit，链来不及调度）。
(texmacs-module (texmacs tests 7001_collab_url))

(import (liii check))
(check-set-mode! 'report-failed)

(tm-define (test_7001)
  (display "Running Collab buffer URL integration test ([7001])...")
  (newline)
  (if (loro-enabled?)
    (begin
      (debug-set "loro" #t)
      ;; 经 scheme 入口 create：new-buffer → buffer-rename 占位 → loro-collab-create。
      (collab-new-document-named "url-test")

      (let loopA
        ((retries 100))
        (loro-collab-poll)
        (if (loro-collab-active?)
          (begin
            (let ((doc-id (loro-collab-doc-id)) (buf (current-buffer)))
              (display "doc-id: ")
              (display doc-id)
              (newline)
              (display "buffer: ")
              (display (url->unix buf))
              (newline)
              ;; [7001] create 经 become_ready 改名后，URL = tmfs://collab/<doc_id>
              (check-true (collab-buffer? buf))
              (check-true (string-starts? (url->unix buf) "tmfs://collab/"))
              (check-true (== (url->unix buf) (string-append "tmfs://collab/" doc-id)))
              ;; [7001] join 同一 doc_id：buffer 已存在 → 跳转，不新建 buffer
              ;; （同一文档在本进程唯一 buffer，避免 tmfs URL 冲突）
              (let ((n (length (buffer-list))))
                (collab-join-document doc-id "url-test-dup")
                (check-true (== (length (buffer-list)) n))
                (check-true (== (url->unix (current-buffer)) (url->unix buf)))
              ) ;let
              (check-report)
              (quit-TeXmacs)
            ) ;let
          ) ;begin
          (if (> retries 0)
            (exec-delayed-at (lambda () (loopA (- retries 1))) (+ (texmacs-time) 100))
            (begin
              (display "Timeout waiting for session ready\n")
              (check-report)
              (quit-TeXmacs)
            ) ;begin
          ) ;if
        ) ;if
      ) ;let
    ) ;begin
    (begin
      (display "Loro not enabled; skipping\n")
      (check-report)
      (quit-TeXmacs)
    ) ;begin
  ) ;if
  (noop)
) ;tm-define
