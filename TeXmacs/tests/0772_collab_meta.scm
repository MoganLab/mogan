;; IMPORTANT:
;; 先 `node tools/loro-server/server.js` 启动服务端，再跑本测试：
;;   MOGAN_TEST_GUI=1 xmake r 0772_collab_meta
;;
;; 验证 body 之外的文档部分（style / initial / attachments）经 CRDT 同步：
;; A 在协作会话中修改这些 meta section（触发 setter 钩子 → 镜像 → 广播），
;; A 断开后 B 加入，应从远端 snapshot 读到一致的 page-type / attachment。
(texmacs-module (texmacs tests 0772_collab_meta))

(tm-define (test_0772_collab_meta)
  (display "Running Collab meta-section integration test...")
  (newline)
  (if (loro-enabled?)
    (begin
      (debug-set "loro" #t)
      (new-buffer)

      (display "Creating collab session (Client A)...")
      (newline)
      (loro-collab-create "ws://127.0.0.1:8765")

      (let loopA
        ((retries 100))
        (loro-collab-poll)
        (if (loro-collab-active?)
          (begin
            (display "Client A active. Doc ID: ")
            (let ((doc-id (loro-collab-doc-id)))
              (display doc-id)
              (newline)

              (insert "Client A body")
              ;; A 修改 body 之外的文档部分（触发 setter 钩子 → 镜像 → 广播）
              (init-env "page-type" "a5")
              (set-attachment "collab-meta-test" "hello-meta")
              (set-style-tree '(tuple "generic"))
              ;; Wait a bit for the broadcast to happen
              (let wait-send
                ((w 30))
                (loro-collab-poll)
                (if (> w 0)
                  (exec-delayed-at (lambda () (wait-send (- w 1))) (+ (texmacs-time) 50))
                  (begin
                    (display "Disconnecting Client A...")
                    (newline)
                    (loro-collab-disconnect)

                    (display "Creating new buffer for Client B...")
                    (newline)
                    (new-buffer)

                    (display "Joining collab session (Client B)...")
                    (newline)
                    (loro-collab-join "ws://127.0.0.1:8765" doc-id)

                    (let loopB
                      ((retriesB 100))
                      (loro-collab-poll)
                      (if (loro-collab-active?)
                        (begin
                          (display "Client B active.")
                          (newline)
                          ;; Wait a bit for the remote operations to be applied
                          (let wait-apply
                            ((w2 30))
                            (loro-collab-poll)
                            (if (> w2 0)
                              (exec-delayed-at (lambda () (wait-apply (- w2 1))) (+ (texmacs-time) 50))
                              (begin
                                ;; B 读到的 meta 应与 A 一致
                                (let ((b-page (get-init "page-type")) (b-att (get-attachment "collab-meta-test")))
                                  (display "B got: page-type=")
                                  (display b-page)
                                  (display ", attachment=")
                                  (display b-att)
                                  (newline)
                                  (display (if (and (== b-page "a5") (== b-att "hello-meta"))
                                             "META SYNC: PASS"
                                             "META SYNC: FAIL"
                                           ) ;if
                                  ) ;display
                                  (newline)
                                ) ;let

                                (quit-TeXmacs)
                              ) ;begin
                            ) ;if
                          ) ;let
                        ) ;begin
                        (if (> retriesB 0)
                          (exec-delayed-at (lambda () (loopB (- retriesB 1))) (+ (texmacs-time) 100))
                          (begin
                            (display "Timeout Client B\n")
                            (quit-TeXmacs)
                          ) ;begin
                        ) ;if
                      ) ;if
                    ) ;let
                  ) ;begin
                ) ;if
              ) ;let
            ) ;let
          ) ;begin
          (if (> retries 0)
            (exec-delayed-at (lambda () (loopA (- retries 1))) (+ (texmacs-time) 100))
            (begin
              (display "Timeout Client A\n")
              (quit-TeXmacs)
            ) ;begin
          ) ;if
        ) ;if
      ) ;let
    ) ;begin
  ) ;if
  (noop)
) ;tm-define
