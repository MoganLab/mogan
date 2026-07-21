;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0848.scm
;; DESCRIPTION : GUI 复现：在 beamer 幻灯片里插入/删除换行的卡顿
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   复现 chapter-3.tmu 这类 beamer 文档在编辑时的明显卡顿：先用
;;   `go-to-label` 跳转到文档中的 testing 标签（落在某张幻灯片的 shown
;;   节点里），连续做「插入一个换行」和「删除该换行」，分别计时。
;;   目测单次动作 >500ms（整张幻灯片重排版）。
;;
;;   要点：
;;     - 复制 fixture 到 /tmp 再加载，避免编辑脏写回原文件；全程不保存。
;;     - `insert-return` 即 kbd-enter 走的插入换行路径；`kbd-backspace`
;;       即退格删除刚插入的换行。两步分别用 texmacs-time 计时。
;;     - scheme 侧编辑调用本身只做树修改，真正的排版在事件循环 idle 时
;;       增量执行（可中断、跨多帧）。为了让计时覆盖「用户看到排版结束」
;;       的完整延迟，每个计时动作在编辑后调用 `(update-forced)`
;;       （C++ typeset_forced）同步跑完整个排版循环，再取停止时间。
;;     - 用 exec-delayed-at 串异步链，不阻塞 Qt 事件循环；链尾自己 quit。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 0848
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 0848))

;; 复现文件名（放在 ~/tests/ 下，非检入夹具，故先复制到 /tmp 再加载）。

(define fixture-src "~/tests/chapter-3.tmu")
(define fixture-name "0848_chapter-3.tmu")

;; 跳转目标：文档中预置的 testing 标签（位于某张幻灯片的 shown 节点内）。

(define target-label "testing")

;; 步骤间隔：给 Qt 事件循环 + typeset 足够时间，避免异步事件叠加。

(define step-delay-ms 3000)

(define (tmp-name name)
  (string->url (string-append "/tmp/" name))
) ;define

(define (refresh-fixture)
  (let ((src (string->url fixture-src)))
    (when (url-exists? src)
      (system-copy src (tmp-name fixture-name))
    ) ;when
  ) ;let
) ;define

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (act)
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

;; 单步计时动作：执行 act，打印 label 与耗时 (ms)。

(define (timed label act)
  (let ((start (texmacs-time)))
    (act)
    (let ((elapsed (- (texmacs-time) start)))
      (display "[0848] ")
      (display label)
      (display ": ")
      (display elapsed)
      (display " ms\n")
      elapsed
    ) ;let
  ) ;let
) ;define

(tm-define (test_0848)
  (refresh-fixture)
  (let ((path (tmp-name fixture-name)))
    (if (not (url-exists? path))
      (begin
        (display "[0848] ABORT: fixture not found at ")
        (display fixture-src)
        (display "\n")
        (quit-TeXmacs)
      ) ;begin
      (begin
        ;; 载入幻灯片，等待排版稳定后开始驱动编辑链。
        (load-buffer path)
        (let ((steps
                (list
                  ;; 1. 跳转到 testing 标签（位于某张幻灯片的 shown 节点内）
                  (cons "goto label"
                    (lambda ()
                      (go-to-label target-label)
                      (display "[0848] at label: cursor-path=")
                      (display (cursor-path))
                      (newline)
                    ) ;lambda
                  ) ;cons
                  ;; 2. 插入一个换行 + 同步排版 —— 计时（用户感知延迟）
                  (cons "insert return"
                    (lambda ()
                      (timed "insert-return+typeset"
                        (lambda () (insert-return) (update-forced)))
                      (display "[0848] after insert: cursor-path=")
                      (display (cursor-path))
                      (newline)
                    ) ;lambda
                  ) ;cons
                  ;; 3. 删除刚插入的换行 + 同步排版 —— 计时，光标应回到插入前
                  (cons "backspace (delete return)"
                    (lambda ()
                      (timed "kbd-backspace+typeset"
                        (lambda () (kbd-backspace) (update-forced)))
                      (display "[0848] after delete: cursor-path=")
                      (display (cursor-path))
                      (newline)
                    ) ;lambda
                  ) ;cons
                  ;; 结束退出
                  (cons "done; quitting" (lambda () (quit-TeXmacs)))
                ) ;list
              )) ;let bindings
          (display "[0848] starting delayed chain\n")
          (run-chain steps)
        ) ;let
      ) ;begin
    ) ;if
  ) ;let
) ;tm-define
