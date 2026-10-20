;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-attachment.scm
;; DESCRIPTION : Initialize fake attachment plugin (%attachment ingest/ready)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; MIT License
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (utils plugins plugin-eval) (binary goldfish))

(import (liii path) (liii json) (liii string))

;; 宿主侧终态预检：真插件与假插件共同遵从同一规则与文案；显式
;; TEXMACS_PATH，宿主所有，不认用户目录覆盖（与 init-research.scm:121
;; load 惯例一致）
(load (url-concretize "$TEXMACS_PATH/plugins/attachment/progs/attachment-policy.scm")
) ;load

;; 与 llm 假插件同款自包含写法（内置 goldfish 与共享设施不改，插件各自
;; 持有 launcher/serializer）：% 开头的单字符串文档（%attachment 协议行）
;; 须原样透传，object->string 会加引号/转义并包 (document ...) 外壳

(define (attachment-serialize lan t)
  (if (and (pair? t)
        (>= (length t) 2)
        (eq? (car t) 'document)
        (string? (cadr t))
        (string-starts? (cadr t) "%")
      ) ;and
    (string-append (cadr t) "\n<EOF>\n")
    (string-append (object->string t) "\n<EOF>\n")
  ) ;if
) ;define

;; 用户目录的入口脚本覆盖系统目录（商业版整目录覆盖 goldfish/）

(define (attachment-launcher)
  (let* ((home (path-from-env "TEXMACS_HOME_PATH"))
         (sys (path-from-env "TEXMACS_PATH"))
         (user (path-join home "plugins" "attachment" "goldfish" "tm-attachment.scm"))
         (sys-path (path-join sys "plugins" "attachment" "goldfish" "tm-attachment.scm"))
         (entry (if (url-exists? (path->string user))
                  (path->string user)
                  (path->string sys-path)
                ) ;if
         ) ;entry
        ) ;
    (string-append (string-quote (url->system (find-binary-goldfish)))
      " load "
      (string-quote (url->system entry))
    ) ;string-append
  ) ;let*
) ;define

;;; ---------- %attachment 会话回调 ----------
;; 每个 ingest 任务独占一个 pending 条目：(cb input task-id)；假插件单包
;; 回传 ready/error 事件（一个 flush-verbatim 包即任务完成）

(define (attachment-session-do lan ses)
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (plugin-write lan ses (second (car l)) :session)
    ) ;when
  ) ;with
) ;define

(define (attachment-output->text t)
  ;; utf8: 通道输出树的全部字符串内容拼接
  (let loop
    ((s (tm->stree t)))
    (cond ((string? s) s)
          ((pair? s) (string-append (loop (car s)) (loop (cdr s))))
          (else "")
    ) ;cond
  ) ;let
) ;define

(define attachment-finalized-tasks (make-ahash-table))

(define (attachment-task-finalized? task-id)
  (ahash-ref attachment-finalized-tasks task-id)
) ;define

(define (attachment-mark-task-finalized! task-id)
  (ahash-set! attachment-finalized-tasks task-id #t)
) ;define

(define (attachment-clear-task-finalized! task-id)
  (ahash-remove! attachment-finalized-tasks task-id)
) ;define

(define (attachment-event-dispatch j)
  ;; ready/error 事件经 glue 投递回 C++ 面板（exec-delayed 避免在
  ;; connection-notify 上下文中重入 UI）
  (let ((event (json-ref-string j "event" ""))
        (task-id (json-ref-string j "taskId" ""))
       ) ;
    (cond
     ((== event "ready")
      ;; chip 展示用的 filename/size 在选择时已填入条目，事件只需 taskId
      ;; 定位与 sha256 回填
      (when (and (string? task-id)
              (not (string-null? task-id))
              (not (attachment-task-finalized? task-id))
            ) ;and
        (attachment-mark-task-finalized! task-id)
        (exec-delayed (lambda () (qt-chat-attachment-ready task-id (json-ref-string j "sha256" "")))
        ) ;exec-delayed
      ) ;when
     ) ;
     ((== event "error")
      (when (and (string? task-id)
              (not (string-null? task-id))
              (not (attachment-task-finalized? task-id))
            ) ;and
        (attachment-mark-task-finalized! task-id)
        (exec-delayed (lambda () (qt-chat-attachment-error task-id (json-ref-string j "reason" "")))
        ) ;exec-delayed
      ) ;when
     ) ;
    ) ;cond
  ) ;let
) ;define

(define (attachment-dispatch-error task-id reason)
  ;; 终态派发：C++ 侧在 glue 调用返回后才注册 attachTaskSession_，
  ;; 宿主预检或兜底报错必须 exec-delayed 派发，避免在 UI 注册前被丢弃；
  ;; 通过 attachment-mark-task-finalized! 防止多通道重复报错
  (when (and (string? task-id)
          (not (string-null? task-id))
          (not (attachment-task-finalized? task-id))
        ) ;and
    (attachment-mark-task-finalized! task-id)
    (exec-delayed (lambda () (qt-chat-attachment-error task-id reason)))
  ) ;when
) ;define

(define (attachment-session-fallback-error lan ses reason)
  ;; 插件通道异常或损坏协议输出时的事件兜底：使用当前 pending 任务的
  ;; taskId 投递 error 事件，避免 C++ 面板永久等待处理中动画与发送栅栏
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with task-id (third (car l)) (attachment-dispatch-error task-id reason))
    ) ;when
  ) ;with
) ;define

(define (attachment-session-notify lan ses ch t)
  (cond
   ((== ch "output")
    (with text
      (string-trim (attachment-output->text t))
      (cond ((string-null? text) (noop))
            ((== text "Attachment Plugin") (noop))
            ((string-starts? text "%attachment ")
             (with j
               (catch #t
                 (lambda ()
                   (string->json (string-trim (string-drop text (string-length "%attachment "))))
                 ) ;lambda
                 (lambda args #f)
               ) ;catch
               (if (json-object? j)
                 (attachment-event-dispatch j)
                 (attachment-session-fallback-error lan
                   ses
                   (string-append "附件协议格式错误： " text)
                 ) ;attachment-session-fallback-error
               ) ;if
             ) ;with
            ) ;
            (else (attachment-session-fallback-error lan
                    ses
                    (string-append "附件插件异常输出： " text)
                  ) ;attachment-session-fallback-error
            ) ;else
      ) ;cond
    ) ;with
   ) ;
   ((== ch "error")
    (with text
      (string-trim (attachment-output->text t))
      (attachment-session-fallback-error lan
        ses
        (if (string-null? text)
          "附件插件异常"
          (string-append "附件插件异常： " text)
        ) ;if
      ) ;attachment-session-fallback-error
    ) ;with
   ) ;
  ) ;cond
) ;define

(define (attachment-session-next lan ses)
  ;; 出队与推进由 plugin-next 统一完成，清理已完成任务的终态标记
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with task-id
        (third (car l))
        (when (string? task-id)
          (attachment-clear-task-finalized! task-id)
        ) ;when
      ) ;with
    ) ;when
  ) ;with
) ;define

(define (attachment-session-cancel lan ses dead?)
  ;; 插件进程终止：处理中的条目以 error 事件语义通知 UI 移除
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with task-id
        (third (car l))
        (attachment-dispatch-error task-id "attachment plugin stopped")
      ) ;with
    ) ;when
  ) ;with
) ;define

(tm-define (attachment-ingest task-id path count bytes)
  (:synopsis "Ingest an attachment file through the attachment plugin")
  (:argument task-id "Ingest task id")
  (:argument path "Local absolute path (only flows to the attachment plugin)")
  (:argument count "Already-selected attachment count this round")
  (:argument bytes "Already-selected total bytes this round")
  ;; 宿主侧终态预检：违规走 attachment-mark-task-finalized! + exec-delayed
  ;; qt-chat-attachment-error 派发，条目录常入面板，由既有 error 清理链移除
  ;; 并 toast。count/bytes 仍作为宿主预检入参；请求 JSON 瘦身只带
  ;; {taskId, path}，插件侧不再二次预检
  (if (not (connection-defined? "attachment"))
    #f
    (let* ((p (path path))
           (filename (path->string (path-name p)))
           ;; size 在可访问时求值：path-getsize 内的 file-exists? 对存在
           ;; 但不可读的文件抛 permission-error（boot.scm）
           (size (if (and (path-file? p) (access path 'R_OK)) (path-getsize p) 0))
           (reason (attachment-host-precheck path filename size count bytes))
          ) ;
      (if reason
        (begin
          (attachment-dispatch-error task-id reason)
          #t
        ) ;begin
        (let ((input
                (stree->tree
                  `(document ,(string-append "%attachment "
                                (json->string (list (cons "taskId" task-id)
                                                (cons "path" path)))))
                ) ;stree->tree
              ) ;input
             ) ;
          (plugin-feed "attachment"
            "default"
            attachment-session-do
            attachment-session-notify
            attachment-session-next
            attachment-session-cancel
            (list input task-id)
          ) ;plugin-feed
          #t
        ) ;let
      ) ;if
    ) ;let*
  ) ;if
) ;tm-define

(define (init-attachment)
  (plugin-configure attachment
    (:require (has-binary-goldfish?))
    (:launch ,(attachment-launcher))
    (:serializer ,attachment-serialize)
  ) ;plugin-configure
) ;define

(init-attachment)
