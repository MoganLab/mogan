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

(define (attachment-dispatch-error task-id reason)
  ;; 终态派发：C++ 侧在 glue 调用返回后才注册 attachTaskSession_，
  ;; 宿主预检或兜底报错必须 exec-delayed 派发，避免在 UI 注册前被丢弃；
  ;; 重复派发由 attachTaskSession_ 的 take 幂等吸收，宿主无需防重表。
  ;; reason 到达此处应为 cork/herk 实体形态（中文宿主字面量须先经
  ;; utf8->cork，通道文本已是实体，勿二次转换）
  (when (and (string? task-id) (not (string-null? task-id)))
    (exec-delayed (lambda () (qt-chat-attachment-error task-id reason)))
  ) ;when
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
      ;; 定位与 sha256 回填；extractedChars 供宿主做提取后累计上限判定
      (when (and (string? task-id) (not (string-null? task-id)))
        (exec-delayed (lambda ()
                        (qt-chat-attachment-ready task-id
                          (json-ref-string j "sha256" "")
                          (json-ref-number j "extractedChars" 0)
                        ) ;qt-chat-attachment-ready
                      ) ;lambda
        ) ;exec-delayed
      ) ;when
     ) ;
     ((== event "error")
      (attachment-dispatch-error task-id (json-ref-string j "reason" ""))
     ) ;
    ) ;cond
  ) ;let
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
                   (string-append (utf8->cork "附件协议格式错误： ") text)
                 ) ;attachment-session-fallback-error
               ) ;if
             ) ;with
            ) ;
            (else (attachment-session-fallback-error lan
                    ses
                    (string-append (utf8->cork "附件插件异常输出： ") text)
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
          (utf8->cork "附件插件异常")
          (string-append (utf8->cork "附件插件异常： ") text)
        ) ;if
      ) ;attachment-session-fallback-error
    ) ;with
   ) ;
  ) ;cond
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
        (attachment-dispatch-error task-id "attachment plugin stopped")
      ) ;with
    ) ;when
  ) ;with
) ;define

;; 形参不得命名为 path：会遮蔽 (liii path) 的 path 构造器，s7 中字符串
;; 可直接被调用（等价 string-ref），(path path) 会按字符串应用抛错并被
;; C++ call 降级为假值，误报「附件插件不可用」
(tm-define (attachment-ingest task-id file-path count bytes source-path)
  (:synopsis "Ingest an attachment file through the attachment plugin")
  (:argument task-id "Ingest task id")
  (:argument file-path
    "Local absolute path (only flows to the attachment plugin)"
  ) ;:argument
  (:argument count "Already-selected attachment count this round")
  (:argument bytes "Already-selected total bytes this round")
  (:argument source-path
    "Original document path for host-converted attachments, '' otherwise"
  ) ;:argument
  ;; 宿主侧终态预检：违规经 attachment-dispatch-error 以 exec-delayed
  ;; qt-chat-attachment-error 派发，条目录常入面板，由既有 error 清理链移除
  ;; 并 toast。count/bytes 仍作为宿主预检入参；请求 JSON 瘦身只带
  ;; {taskId, path}，插件侧不再二次预检。宿主做过格式转换（文档转临时
  ;; PDF）时另带 sourcePath：插件据原路径计算 sha256 与抽取缓存键，
  ;; 实际读取仍指向 path（临时 PDF）；常规附件不带该键，契约不变
  (if (not (connection-defined? "attachment"))
    #f
    (let* ((fs-path (cork->utf8 file-path))
           ;; file-path 自 glue 传入为 herk 实体形态（6217 的 from_qstring
           ;; 契约，wire 侧 connection_write 才做 tree_herk_to_utf8）；
           ;; 宿主预检不经 wire，文件系统调用必须先解回真实 UTF-8，
           ;; 否则中文路径 path-file? 恒假、误报「文件不存在」
           (p (path fs-path))
           ;; 文案文件名取 herk 形态：toast 侧 cork_to_utf8 会解码实体；
           ;; 扩展名判定只看最后一个点后的 ASCII，实体无点不受影响
           (filename (path->string (path-name (path file-path))))
           ;; size 在可访问时求值：path-getsize 内的 file-exists? 对存在
           ;; 但不可读的文件抛 permission-error（boot.scm）
           (size (if (and (path-file? p) (access fs-path 'R_OK)) (path-getsize p) 0))
           (reason (attachment-host-precheck fs-path filename size count bytes))
          ) ;
      (if reason
        (begin
          ;; 宿主字面量为裸 UTF-8，过 glue 前转 <#XXXX> 实体（toast 侧
          ;; cork_to_utf8 只认实体形态）
          (attachment-dispatch-error task-id (utf8->cork reason))
          #t
        ) ;begin
        (let ((input
                (stree->tree
                  `(document ,(string-append "%attachment "
                                (attachment-ingest-request-json task-id
                                  file-path source-path)))
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
