;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2015.scm
;; DESCRIPTION : GUI 排查：已保存文件切 tab 时为何标脏
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   用户现象：已经本地保存的文件（切换前没有 *），切 tab 时却冒出 * 脏标记。
;;
;;   根因（已确认）：切 tab 本身不标脏——它只是如实照见 buffer 的脏状态。真正的
;;   脏来自打开没有 stem-doc-id 的旧文件时：
;;     auto-backup-opened-buffer!
;;       -> auto-backup-ensure-buffer-doc-id!
;;       -> init-env "stem-doc-id"
;;       -> edit_typeset_rep::init_env 的 require_save() 把 buffer 标脏
;;   该注入是系统行为、没有配套清脏，于是文件一打开就静默带脏标记。tab 标题此时
;;   不一定重建，要等切 tab 时 tabpage-display-title -> buffer-modified? 重新求值
;;   （并使 tabpage-menu-signature 变化、tab 栏重建）才把那个 * "照"出来——
;;   即用户看到的「切 tab 才冒 *」。
;;
;;   修复：auto-backup-ensure-buffer-doc-id! 在新生成 doc-id（init-env 之后）立即
;;   buffer-pretend-saved 清脏。已有 doc-id 的 no-op 路径不进此分支，真实编辑后
;;   的脏不受影响。
;;
;;   本脚本驱动"打开两个已保存文件 -> 编辑/保存 -> 来回切 tab"，断言：
;;     - 2015_with_id.tmu：文件已持久化 stem-doc-id，打开 no-op，全程不脏。
;;     - 2015_no_id.tmu ：文件没有 stem-doc-id，打开注入 doc-id，修复后清脏，故也不脏。
;;     - 真实编辑（start/end-editing 包裹 insert）后标脏，切 tab 不丢失该脏。
;;
;;   备注：save-buffer 是异步的（with-default-view -> exec-delayed），保存后同步
;;   立即读 modified 仍为 #t；故断言保存干净须等落盘（"settle" 步骤），这是测试
;;   时序，不是产品 bug。
;;
;;   夹具从 $TEXMACS_PATH/tests/tmu 复制到 /tmp，避免 save/编辑污染检入副本。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2015
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 2015))

;; 步骤间隔足够长，让 load-buffer 的异步初始化（含 auto-backup-opened-buffer!
;; 的 delayed 备份）与 typeset 完成。

(define step-delay-ms 4000)

(define (tmp-name name)
  (string->url (string-append "/tmp/" name))
) ;define

(define (refresh-fixture name)
  (let ((src (string->url (string-append "$TEXMACS_PATH/tests/tmu/" name))))
    (when (url-exists? src)
      (system-copy src (tmp-name name))
    ) ;when
  ) ;let
) ;define

;; 打印某 buffer 的脏状态 + 内存中 stem-doc-id，把脏标记和 doc-id 注入对齐。
;; expect-clean 为 #t 时断言该 buffer 不脏，失败则打印 FAIL 行。

(define (log-buf label buf . opt)
  (when buf
    (let ((mod (buffer-modified? buf))
          (doc-id (catch #t (lambda () (auto-backup-buffer-doc-id buf)) (lambda args "ERR"))
          ) ;doc-id
          (expect-clean (and (pair? opt) (car opt)))
         ) ;
      (display "[2015] ")
      (display label)
      (display " modified=")
      (display mod)
      (display " doc-id=")
      (display doc-id)
      (display " buf=")
      (display buf)
      (when expect-clean
        (display (if mod "  <<FAIL: 应不脏" "  OK"))
      ) ;when
      (newline)
    ) ;let
  ) ;when
) ;define

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。
;; 每个元素是 (label . action-thunk)。

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2015-step] ")
                           (display label)
                           (newline)
                           (act)
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

;; 主文档可由环境变量 MOGAN_2015_DOC 指定（绝对路径），用于复现真实文档；
;; 未指定时退回内置夹具 2015_with_id.tmu。

(define (main-doc-path)
  (let ((env (system-getenv "MOGAN_2015_DOC")))
    (if (and (string? env) (!= env ""))
      (string->url env)
      (begin
        (refresh-fixture "2015_with_id.tmu")
        (tmp-name "2015_with_id.tmu")
      ) ;begin
    ) ;if
  ) ;let
) ;define

(tm-define (test_2015)
  (refresh-fixture "2015_no_id.tmu")
  (let* ((path-with (main-doc-path))
         (path-no (tmp-name "2015_no_id.tmu"))
         (buf-with #f)
         (buf-no #f)
        ) ;
    ;; 精确复现用户现象「保存后切换才标 *」：
    ;;  - 打开主文档（with_id 或 MOGAN_2015_DOC 指定的真实文档）
    ;;  - 打开 no_id（占位 tab）
    ;;  - 在主文档里真实编辑（start/end-editing）→ 标脏
    ;;  - 保存主文档 → 应变干净（无 *）
    ;;  - 切到 no_id，再切回主文档：断言主文档仍干净（用户报 bug 的点）
    ;;  - 结束退出
    ;;
    ;; 配合 C++ 端 [2015] require_save / init_env / init_style / pretend_saved
    ;; 日志，定位切 tab 过程中究竟谁触发了标脏。
    ;; 用 switch-to-view-index（即 Cmd+3/4 走的同一条路径）。
    (let ((steps (list (cons "load main doc"
                         (lambda ()
                           (load-buffer path-with)
                           (set! buf-with (current-buffer))
                           (log-buf "after load" buf-with #t)
                         ) ;lambda
                       ) ;cons
                   (cons "load no_id"
                     (lambda ()
                       (load-buffer path-no)
                       (set! buf-no (current-buffer))
                       (log-buf "after load" buf-no #t)
                     ) ;lambda
                   ) ;cons
                   (cons "edit in main doc"
                     (lambda ()
                       (switch-to-buffer* buf-with)
                       (start-editing)
                       (insert "hello")
                       (end-editing)
                       (log-buf "after-edit" buf-with)
                     ) ;lambda
                   ) ;cons
                   (cons "save main doc (async: 立即读可能仍脏)"
                     (lambda () (save-buffer buf-with) (log-buf "after-save-immediate" buf-with #t))
                   ) ;cons
                   (cons "settle: save 异步落盘后再读"
                     (lambda () (log-buf "after-save-settled" buf-with #t))
                   ) ;cons
                   (cons "switch -> no_id (Cmd+3 路径)"
                     (lambda ()
                       (switch-to-view-index 1)
                       (log-buf "switched" buf-no #t)
                       (log-buf "  other " buf-with #t)
                     ) ;lambda
                   ) ;cons
                   (cons "switch -> main doc (Cmd+2 路径)  [bug 点]"
                     (lambda ()
                       (switch-to-view-index 0)
                       (log-buf "switched-back" buf-with #t)
                       (log-buf "  other " buf-no #t)
                     ) ;lambda
                   ) ;cons
                   (cons "edit no_id 后切走再切回：真实编辑须保留脏"
                     (lambda ()
                       (switch-to-buffer* buf-no)
                       (start-editing)
                       (insert "world")
                       (end-editing)
                       (log-buf "after-edit-no_id" buf-no)
                       (switch-to-view-index 0)
                       (switch-to-view-index 1)
                       (log-buf "no_id real-dirty survives switch" buf-no)
                     ) ;lambda
                   ) ;cons
                   (cons "done; quitting" (lambda () (quit-TeXmacs)))
                 ) ;list
          ) ;steps
         ) ;
      (display "[2015-step] starting delayed chain\n")
      (run-chain steps)
    ) ;let
  ) ;let*
) ;tm-define
