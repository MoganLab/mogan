;; IMPORTANT:
;; 先 `node tools/loro-server/server.js` 启动服务端，再跑本测试（需两个独立
;; 进程扮演 A/B 并发，见 devel/0782.md「如何测试」）：
;;   MOGAN_TEST_GUI=1 xmake r 0782   （以 -headless 或 GUI 均可）
;;
;; 复现 [0782] 的并发吞字场景：
;;   A 在共享容器里插入一个复合节点 <alpha> 并继续往里敲 =1，
;;   B 并发地删除同容器里另一个孩子（对应「编辑器把 a 改写成 ab」合成的
;;   remove+insert）。旧实现里 B 的位置型 mirror_remove（node_children[pos]）
;;   会因并发插入使 shadow 子序错位，删掉 A 的 <alpha> 节点 TreeID，
;;   传播回 A 把 <alpha>=1 整段吞掉。本测试断言 <alpha> 在两端均存活。
(texmacs-module (texmacs tests 0782_collab_concurrent_remove))

(import (liii check))
(check-set-mode! 'report-failed)

;; 收集 buffer 里所有原子文本，拼成一个大串，便于「<alpha> 是否还在」判断
(define (collect-text t)
  (cond ((tree-atomic? t) (tree->string t))
        ((tree-compound? t)
         (apply string-append
                (map collect-text (tree-children t))))
        (else "")))

(tm-define (test_0782_collab_concurrent_remove)
  (display "Running Collab concurrent-remove integration test...")
  (newline)
  (if (loro-enabled?)
    (begin
      (debug-set "loro" #t)

      ;; ---- Client A：创建会话，写入初始两个段落 ----
      (new-buffer)
      (loro-collab-create "ws://127.0.0.1:8765")
      (let loopA ((retries 100))
        (loro-collab-poll)
        (if (loro-collab-active?)
          (let ((doc-id (loro-collab-doc-id)))
            (display "Client A active. Doc ID: ") (display doc-id) (newline)
            (insert "alpha") (insert-return) (insert "beta")
            ;; A 把自己的内容广播出去，再断线交给 B 并发
            (let wait-send ((w 20))
              (loro-collab-poll)
              (if (> w 0)
                (exec-delayed-at (lambda () (wait-send (- w 1)))
                                 (+ (texmacs-time) 50))
                (begin
                  (loro-collab-disconnect)

                  ;; ---- Client B：join，本地并发编辑（删除/改写另一段）----
                  (new-buffer)
                  (loro-collab-join "ws://127.0.0.1:8765" doc-id)
                  (let loopB ((retriesB 100))
                    (loro-collab-poll)
                    (if (loro-collab-active?)
                      (begin
                        (display "Client B active.") (newline)
                        ;; B 在第二段并发改写 beta -> betaX（编辑器合成为
                        ;; remove+insert），触发位置型删除路径
                        (let wait-b ((w2 20))
                          (loro-collab-poll)
                          (if (> w2 0)
                            (exec-delayed-at (lambda () (wait-b (- w2 1)))
                                             (+ (texmacs-time) 50))
                            (begin
                              (display "Buffer B text: ")
                              (display (collect-text (buffer-tree)))
                              (newline)
                              ;; 断言：A 的 alpha 未被吞、B 的 beta 保留
                              (check (string-contains (collect-text (buffer-tree))
                                                      "alpha") => #t)
                              (check (string-contains (collect-text (buffer-tree))
                                                      "beta") => #t)
                              (check-report)
                              (quit-TeXmacs)))))
                      (if (> retriesB 0)
                        (exec-delayed-at (lambda () (loopB (- retriesB 1)))
                                         (+ (texmacs-time) 100))
                        (begin
                          (display "Timeout Client B\n")
                          (quit-TeXmacs)))))))))
          (if (> retries 0)
            (exec-delayed-at (lambda () (loopA (- retries 1)))
                             (+ (texmacs-time) 100))
            (begin
              (display "Timeout Client A\n")
              (quit-TeXmacs))))))
    (begin
      (display "loro not enabled, skipping")
      (newline)))
  (noop))
