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

(define (attachment-serialize lan t)
  ;; 与 llm-serialize 同理：% 开头的单字符串文档（%attachment 协议行）
  ;; 须原样透传，object->string 会加引号/转义并包 (document ...) 外壳
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
      (exec-delayed (lambda ()
                      (qt-chat-attachment-ready task-id
                        (json-ref-string j "sha256" "")
                        (json-ref-string j "filename" "")
                        (json-ref-number j "size" 0)
                        (json-ref-number j "extractedChars" 0)
                      ) ;qt-chat-attachment-ready
                    ) ;lambda
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

(tm-define (attachment-ingest task-id path)
  (:synopsis "Ingest an attachment file through the attachment plugin")
  (:argument task-id "Ingest task id")
  (:argument path "Local absolute path (only flows to the attachment plugin)")
  (if (not (connection-defined? "attachment"))
    #f
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
) ;tm-define

(define (init-attachment)
  (plugin-configure attachment
    (:require (has-binary-goldfish?))
    (:launch ,(attachment-launcher))
    (:serializer ,attachment-serialize)
  ) ;plugin-configure
) ;define

(init-attachment)
