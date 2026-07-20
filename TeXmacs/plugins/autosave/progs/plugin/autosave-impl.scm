;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : autosave-impl.scm
;; DESCRIPTION : autosave and auto-backup implementation for the autosave plugin
;; COPYRIGHT   : (C) 2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (plugin autosave-impl) (:use (utils library cursor)))

(import (liii uuid))
(import (liii json))
(import (liii path))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Autosave
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define autosave-fixed-interval-ms 120000)

(tm-define (autosave-enabled?) (!= (get-preference "autosave") "0"))

(tm-define (auto-backup-enabled?) (!= (get-preference "autobackup") "off"))

(tm-define (liiistem-version) (xmacs-version))

;; auto-backup-texmacs-path-buffer?
;; 判断 buffer 是否位于 get-texmacs-path 返回的目录或其子目录中。
;;
;; 语法
;; ----
;; (auto-backup-texmacs-path-buffer? name)
;;
;; 参数
;; ----
;; name : url
;; 待检查的 buffer 名称。
;;
;; 返回值
;; ----
;; boolean
;; #t 表示 buffer 对应路径位于 get-texmacs-path 下。
;;
;; 逻辑
;; ----
;; 将 buffer url 转成系统路径，再使用 (liii path) 的 path-parent 逐级
;; 向上检查是否能到达 get-texmacs-path。
;;
;; 注意
;; ----
;; TeXmacs 安装路径下的文件被视为只读内置资源，不进入自动备份。
(tm-define (auto-backup-texmacs-path-buffer? name)
  (url-descends? name (get-texmacs-path))
) ;tm-define

(define (auto-backup-path->url p)
  (system->url (path->string p))
) ;define

(define (auto-backup-format name)
  (if (url-scratch? name) "texmacs" (url-format name))
) ;define

;; auto-backup-buffer-eligible?
;; 判断指定 buffer 是否允许进入自动备份。
;;
;; 语法
;; ----
;; (auto-backup-buffer-eligible? name)
;;
;; 参数
;; ----
;; name : url
;; 待检查的 buffer 名称。
;;
;; 返回值
;; ----
;; boolean
;; #t 表示允许自动备份，#f 表示跳过。
;;
;; 逻辑
;; ----
;; 只允许本地、非 tmfs、非 web 且格式为 texmacs/stm/tmu 的文档备份；
;; 位于 get-texmacs-path 目录或子目录下的内置只读文件直接跳过。
;;
;; 注意
;; ----
;; 这个判断也会影响 doc id 绑定，跳过的只读资源不会被写入 stem-doc-id。
(tm-define (auto-backup-buffer-eligible? name)
  (and (url? name)
    (buffer-exists? name)
    (not (url-rooted-web? name))
    (not (url-rooted-tmfs? name))
    (not (auto-backup-texmacs-path-buffer? name))
    (in? (auto-backup-format name) '("texmacs" "stm" "tmu" "stem"))
  ) ;and
) ;tm-define

(define (auto-backup-valid-doc-id? doc-id)
  (and (string? doc-id) (!= doc-id ""))
) ;define

(tm-define (auto-backup-buffer-doc-id name)
  (catch #t
    (lambda ()
      ;; First try to get from init-env (memory), then from document tree (file)
      (with-buffer name
        (let* ((from-env (get-init-env "stem-doc-id"))
               (doc-id (if (and (string? from-env) (!= from-env ""))
                         from-env
                         (let* ((doc (buffer-get name)) (initial (tmfile-extract doc 'initial)))
                           (and initial (collection-ref initial "stem-doc-id"))
                         ) ;let*
                       ) ;if
               ) ;doc-id
              ) ;
          doc-id
        ) ;let*
      ) ;with-buffer
    ) ;lambda
    (lambda args #f)
  ) ;catch
) ;tm-define

(tm-define (auto-backup-buffer-needs-doc-id? name)
  (and (auto-backup-buffer-eligible? name)
    (not (auto-backup-valid-doc-id? (auto-backup-buffer-doc-id name)))
  ) ;and
) ;tm-define

;; auto-backup-ensure-buffer-doc-id!
;; 确保可备份 buffer 已经绑定 stem-doc-id。
;;
;; 语法
;; ----
;; (auto-backup-ensure-buffer-doc-id! name)
;;
;; 参数
;; ----
;; name : url
;; 待检查和绑定的 buffer 名称。
;;
;; 返回值
;; ----
;; string or #f
;; 返回已有或新生成的 doc id；不可备份或失败时返回 #f。
;;
;; 逻辑
;; ----
;; 先读取 buffer 当前 init-env 或 initial collection 中的 stem-doc-id；
;; 若没有，则生成新的 uuid4 并写入 init-env。
;;
;; 注意
;; ----
;; 这里只写入 init-env，避免触发文档重新解析；doc id 是否持久化到文件由
;; 用户后续保存动作决定。
(tm-define (auto-backup-ensure-buffer-doc-id! name)
  (catch #t
    (lambda ()
      (and (auto-backup-buffer-eligible? name)
        (with-buffer name
          (let ((old-doc-id (auto-backup-buffer-doc-id name)))
            (if (auto-backup-valid-doc-id? old-doc-id)
              old-doc-id
              (let ((doc-id (uuid4)))
                ;; 写入 init-env 即可绑定到当前会话，避免 buffer-set 触发
                ;; 文档重新解析。
                (init-env "stem-doc-id" doc-id)
                doc-id
              ) ;let
            ) ;if
          ) ;let
        ) ;with-buffer
      ) ;and
    ) ;lambda
    (lambda args #f)
  ) ;catch
) ;tm-define

(tm-define (auto-backup-trig-payload name kind)
  (let* ((path (url->system name))
         (doc-id (auto-backup-ensure-buffer-doc-id! name))
         (session-id (uuid4))
         (payload (string->json "{}"))
        ) ;
    (set! payload (json-push payload "path" path))
    (set! payload (json-push payload "type" kind))
    (set! payload (json-push payload "id" doc-id))
    (set! payload (json-push payload "session-id" session-id))
    ;; 云备份请求头所需的 4 个静态字段：autosave 子进程通过 payload 拿到这些值，
    ;; 构造 Authorization / User-Agent / X-Device-Id 头和 upload URL。
    ;; 账号模块或 glue 函数在未登录/未加载时会抛异常，逐个 catch 回退空串，
    ;; 避免 payload 构造失败导致整个 copy 流程中断。
    (set! payload
      (json-push payload
        "site"
        (catch #t (lambda () (current-stem-site)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (set! payload
      (json-push payload
        "token"
        (catch #t (lambda () (account-load-token)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (set! payload
      (json-push payload
        "user-agent"
        (catch #t (lambda () (stem-user-agent)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (set! payload
      (json-push payload
        "device-id"
        (catch #t (lambda () (stem-device-id)) (lambda args ""))
      ) ;json-push
    ) ;set!
    (values (json->string payload) session-id)
  ) ;let*
) ;tm-define

;; auto-backup-trig
;; 自动备份触发入口，当前仅用于调试输出触发参数。
;;
;; 语法
;; ----
;; (auto-backup-trig u kind)
;;
;; 参数
;; ----
;; u : url
;; 需要备份的 buffer url。
;;
;; kind : string
;; 备份类型，例如 "save"、"save-as"、"export-pdf"、"on-open"、"auto"、"manual-open"。

(tm-define (auto-backup-trig u kind)
  (when (and (auto-backup-enabled?) (auto-backup-buffer-eligible? u))
    (receive (s session-id)
      (auto-backup-trig-payload u kind)
      (silent-feed* "autosave"
        session-id
        `(document ,(utf8->cork s))
        (lambda (r) (noop))
        '()
      ) ;silent-feed*
    ) ;receive
  ) ;when
) ;tm-define

;; auto-backup-opened-buffer!
;; 文件打开后的自动备份准备流程。
;;
;; 语法
;; ----
;; (auto-backup-opened-buffer! name)
;;
;; 参数
;; ----
;; name : url
;; 已经打开并切换完成的 buffer 名称。
;;
;; 逻辑
;; ----
;; 打开文件时只在当前会话中绑定缺失的 stem-doc-id，避免静默改写源文件；
;; 随后延迟触发一次 on-open 备份，由 md5 去重避免重复版本。

(tm-define (auto-backup-opened-buffer! name)
  (auto-backup-ensure-buffer-doc-id! name)
  (delayed (:pause 100) (auto-backup-trig name "on-open"))
) ;tm-define

(tm-define (auto-backup-official-url)
  (if (== (get-output-language) "chinese")
    "https://liiistem.cn/personal-center/backup.html?utm_source=auto_backup_button"
    "https://liiistem.com/?utm_source=auto_backup_button"
  ) ;if
) ;tm-define

(tm-define (auto-backup-button-label)
  (if (community-stem?) "View help" "Cloud backup")
) ;tm-define

(tm-define (open-auto-backup-location)
  (if (community-stem?)
    (open-url "https://liiistem.cn/docs/guide-auto-backup")
    (open-url (auto-backup-official-url))
  ) ;if
  (auto-backup-trig (current-buffer-url) "visit-cloud-backup")
) ;tm-define

(tm-define (autosave-all)
  (for-each (lambda (name)
              (when (and (buffer-modified? name) (not (url-scratch? name)))
                (save-buffer-save name (list) "auto")
              ) ;when
            ) ;lambda
    (buffer-list)
  ) ;for-each
) ;tm-define

(tm-define (autosave-now)
  (when (autosave-enabled?)
    (let ((name (current-buffer)))
      (when (and (buffer-modified? name) (not (url-scratch? name)))
        (save-buffer-save name (list) "auto")
      ) ;when
    ) ;let
    (autosave-delayed)
  ) ;when
) ;tm-define

(tm-define (save-all-buffers)
  (for-each (lambda (buf)
              (when (buffer-modified? buf)
                (auto-backup-ensure-buffer-doc-id! buf)
                (buffer-save buf)
              ) ;when
            ) ;lambda
    (buffer-list)
  ) ;for-each
) ;tm-define

(tm-define (autosave-delayed)
  (when (autosave-enabled?)
    (delayed (:pause autosave-fixed-interval-ms) (autosave-now))
  ) ;when
) ;tm-define

(define (notify-autosave var val)
  (if (current-view) (begin (autosave-delayed)))
) ;define

(define-preferences ("autosave" "120" notify-autosave))
