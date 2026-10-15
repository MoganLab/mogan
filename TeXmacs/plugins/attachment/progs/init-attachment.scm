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
      (exec-delayed (lambda () (qt-chat-attachment-ready task-id (json-ref-string j "sha256" "")))
      ) ;exec-delayed
     ) ;
     ((== event "error")
      (exec-delayed (lambda () (qt-chat-attachment-error task-id (json-ref-string j "reason" "")))
      ) ;exec-delayed
     ) ;
    ) ;cond
  ) ;let
) ;define

(define (attachment-session-notify lan ses ch t)
  (when (== ch "output")
    (with text
      (string-trim (attachment-output->text t))
      (when (string-starts? text "%attachment ")
        (with j
          (catch #t
            (lambda ()
              (string->json (string-trim (string-drop text (string-length "%attachment "))))
            ) ;lambda
            (lambda args #f)
          ) ;catch
          (when (json-object? j)
            (attachment-event-dispatch j)
          ) ;when
        ) ;with
      ) ;when
    ) ;with
  ) ;when
) ;define

(define (attachment-session-next lan ses)
  ;; 出队与推进由 plugin-next 统一完成，附件任务无额外收尾
  (noop)
) ;define

(define (attachment-session-cancel lan ses dead?)
  ;; 插件进程终止：处理中的条目以 error 事件语义通知 UI 移除
  (with l
    (pending-ref lan ses)
    (when (nnull? l)
      (with task-id
        (third (car l))
        (exec-delayed (lambda () (qt-chat-attachment-error task-id "attachment plugin stopped"))
        ) ;exec-delayed
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
  ;; count/bytes 为本轮已选累计，供插件侧预检（扩展名白名单/数量/总大小
  ;; 上限均在插件内判定，违规回 error 事件）
  (if (not (connection-defined? "attachment"))
    #f
    (let ((input
            (stree->tree
              `(document ,(string-append "%attachment "
                            (json->string (list (cons "taskId" task-id)
                                            (cons "path" path)
                                            (cons "count" count)
                                            (cons "bytes" bytes)))))
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
) ;tm-define

(define (init-attachment)
  (plugin-configure attachment
    (:require (has-binary-goldfish?))
    (:launch ,(attachment-launcher))
    (:serializer ,attachment-serialize)
  ) ;plugin-configure
) ;define

(init-attachment)
