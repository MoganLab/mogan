;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-collab.scm
;; DESCRIPTION : 云文档协作（Mogan × Loro CRDT）的 scheme 编排：
;;               新建/加入协作文档——创建空 buffer → 切到其 view → 驱动
;;               C++ 协作会话层连接服务端（CREATE/JOIN），随后由会话层在
;;               收到服务端 snapshot/updates 时把内容同步进 buffer。
;; COPYRIGHT   : (C) 2026  Jim Zhou
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs texmacs tm-collab)
  (:use (texmacs texmacs tm-server) (texmacs texmacs tm-files))
) ;texmacs-module

;; 协作服务端地址优先级：用户经 Collaborative 菜单设置的偏好（collab:server-url）
;; > C++ loro-collab-server-url（native 读 OS env MOGAN_LORO_SERVER；WASM 读
;; window.MOGAN_LORO_SERVER / ?loro_server= 查询参数；都未设回落 ws://127.0.0.1:8765）。
;; 偏好是终端用户上线的主路径，env 保留给开发/CI。

(define collab-server-url-key "collab:server-url")

(tm-define (collab-server-url)
  (with configured
    (get-preference collab-server-url-key)
    (if (!= configured "") configured (loro-collab-server-url))
  ) ;with
) ;tm-define

;; 是否已显式配置服务端（Collaborative 菜单据此切「仅设置项 / 完整菜单」两形态）。
(tm-define (collab-server-configured?)
  (!= (get-preference collab-server-url-key) "")
) ;tm-define

;; === 服务端地址：地址+端口两框 ↔ 完整 URL（纯函数，供单测） ===
;; 地址框默认填纯 host，端口单独一框，拼成 ws://host:port；但地址框亦接受完整
;; ws(s):// URL（隐藏的高级回退，保留 wss/TLS、IPv6、路径等能力）。

;; 判前缀（mogan scheme 无 string-prefix?，手写）。

(define (collab-string-prefix? p s)
  (and (>= (string-length s) (string-length p))
    (string=? (substring s 0 (string-length p)) p)
  ) ;and
) ;define

;; 已存 URL → (address . port) 回填两框。仅处理常见 ws(s)://host:port；
;; 含路径 / 多冒号（IPv6 等）/ 非 ws(s) scheme → 整串塞进 address（端口空），
;; 即「地址框支持完整 URL」的回退。逐字符扫描，避开本模块未导入的 string-index/
;; string-contains（只用 string->list / char=?，与 collab-valid-doc-name? 同套）。

(define (collab-url->fields url)
  (let ((strip (lambda (p)
                 (and (collab-string-prefix? p url)
                   (substring url (string-length p) (string-length url))
                 ) ;and
               ) ;lambda
        ) ;strip
       ) ;
    (let ((rest (or (strip "ws://") (strip "wss://"))))
      (if (not rest)
        (cons url "")
        (let loop
          ((cs (string->list rest)) (i 0) (colon #f))
          (cond ((null? cs)
                 (if (not colon)
                   (cons rest "")
                   (cons (substring rest 0 colon)
                     (substring rest (+ colon 1) (string-length rest))
                   ) ;cons
                 ) ;if
                ) ;
                ((char=? (car cs) #\/) (cons url ""))
                ((and (char=? (car cs) #\:) colon) (cons url ""))
                ((char=? (car cs) #\:) (loop (cdr cs) (+ i 1) i))
                (else (loop (cdr cs) (+ i 1) colon))
          ) ;cond
        ) ;let
      ) ;if
    ) ;let
  ) ;let
) ;define

;; 两框 → URL。地址为完整 URL（ws/wss 开头）→ 原样；地址空 → 清除；
;; 否则按 host[:port] 拼 ws://。

(define (collab-fields->url addr port)
  (cond ((== addr "") "")
        ((or (collab-string-prefix? "ws://" addr) (collab-string-prefix? "wss://" addr))
         addr
        ) ;
        ((== port "") (string-append "ws://" addr))
        (else (string-append "ws://" addr ":" port))
  ) ;cond
) ;define

;; 弹框配置/修改协作服务端：地址 + 端口两框（预填当前生效值）；地址框亦接受完整
;; ws(s):// URL。空地址 = 清除偏好（回到 env/默认）。设置后下次展开菜单自动切完整形态。
;; 会话进行中禁止改地址：连接 URL 在连接时固定，改了也不迁移当前会话，故要求先 Leave。
(tm-define (collab-configure-server)
  (:interactive #t)
  (if (loro-collab-active?)
    (set-message "Leave the current session before changing the server address"
      "Collaborative"
    ) ;set-message
    (with cur
      (collab-url->fields (collab-server-url))
      (interactive (lambda (addr port)
                     (with url
                       (collab-fields->url addr port)
                       (if (== url "")
                         (reset-preference collab-server-url-key)
                         (set-preference collab-server-url-key url)
                       ) ;if
                     ) ;with
                   ) ;lambda
        (list "Server address" "string" (car cur))
        (list "Server port" "string" (cdr cur))
      ) ;interactive
    ) ;with
  ) ;if
) ;tm-define

;; === collab 缓冲（云端文档）的标识 ===
;; 多会话：每个协作 buffer 用专用 tmfs URL（tmfs://collab/<doc_id>）标识，
;; collab-buffer? 据 URL 协议判定，无需单值变量。下游 save-buffer 覆盖用它
;; 门控（不可保存）。tmfs buffer 自动当 scratch，关闭弹「另存」契合云文档。

;; collab buffer URL：tmfs://collab/<doc_id>。
(tm-define (collab-buffer-url->tmfs doc-id)
  (unix->url (string-append "tmfs://collab/" doc-id))
) ;tm-define

;; create 时 doc_id 尚未从服务端返回，用唯一占位 id（也是 collab 协议，
;; 被 collab-buffer? 识别）；become_ready 收到 DOC 后改名成真实 doc_id。

(define collab-placeholder-counter 0)

(define (collab-placeholder-doc-id)
  (set! collab-placeholder-counter (+ collab-placeholder-counter 1))
  (string-append "pending-"
    (number->string (texmacs-time))
    "-"
    (number->string collab-placeholder-counter)
  ) ;string-append
) ;define

;; 纯 URL 谓词：支持多会话，占位（pending-…）与最终 doc_id URL 都识别。
(tm-define (collab-buffer? u) (url-rooted-tmfs-protocol? u "collab"))

;; === 文档显示名校验与列表工具（纯函数，供测试覆盖） ===
;; 规则与服务端 tools/loro-server/validate.js 保持一致：trim 后长度 1–64
;; （按字符计，用 utf8-string-length——string-length 计字节数），
;; 禁止 \ / : * ? " < > | 及控制字符。Scheme 侧仅做预校验（即时反馈），
;; 服务端仍是权威校验方。

(define collab-doc-name-forbidden-chars '(#\\ #\/ #\: #\* #\? #\" #\< #\> #\|))

(define (collab-valid-doc-name? name)
  (and (string? name)
    (>= (utf8-string-length name) 1)
    (<= (utf8-string-length name) 64)
    (not (list-find (string->list name)
           (lambda (c)
             (or (in? c collab-doc-name-forbidden-chars)
               (< (char->integer c) 32)
               (== (char->integer c) 127)
             ) ;or
           ) ;lambda
         ) ;list-find
    ) ;not
  ) ;and
) ;define

(define (collab-docs-pairs flat)
  (if (or (null? flat) (null? (cdr flat)))
    '()
    (cons (cons (car flat) (cadr flat)) (collab-docs-pairs (cddr flat)))
  ) ;if
) ;define

(define (collab-doc-label uuid name dup?)
  (if (and (string? name) (> (string-length name) 0))
    (if dup?
      `(concat (verbatim ,name)
         ," "
         (with ,"color"
           ,"dark grey"
           (verbatim ,(string-append "("
                        (substring uuid 0 (min 4 (string-length uuid)))
                        ")"))))
      `(verbatim ,name)
    ) ;if
    uuid
  ) ;if
) ;define

(define (collab-doc-name-duplicates pairs)
  (let ((counts '()))
    (for (p pairs)
      (with name
        (cdr p)
        (when (and (string? name) (> (string-length name) 0))
          (let ((cell (assoc name counts)))
            (if cell
              (set-cdr! cell (+ (cdr cell) 1))
              (set! counts (cons (cons name 1) counts))
            ) ;if
          ) ;let
        ) ;when
      ) ;with
    ) ;for
    counts
  ) ;let
) ;define
(tm-define (collab-new-document)
  (:interactive #t)
  (interactive (lambda (name) (collab-new-document-named name)) "Document name")
) ;tm-define
(tm-define (collab-new-document-named name)
  (let ((uname (cork->utf8 name)))
    (cond ((and (string? uname)
             (> (string-length uname) 0)
             (not (collab-valid-doc-name? uname))
           ) ;and
           (set-message "Invalid name: 1-64 chars, no \\ / : * ? \" < > | or control chars"
             "Collaborative"
           ) ;set-message
          ) ;
          (else (with-default-view (if (window-per-buffer?) (open-window) (new-buffer))
                  ;; 新建 buffer 改名为 tmfs 占位 URL（成为可识别的 collab buffer）；
                  ;; C++ become_ready 收到 DOC 后再改名为 tmfs://collab/<真实 doc_id>。
                  ;; 标题先设为用户输入的显示名（无名文档保持 No Name），再 rename：
                  ;; rename 的 propose_title 据 old_title 决定，old_title=doc_name 时
                  ;; keep_old 保留（而非 No name→tmfs URL），避免第一次 tab 重建显示 URL。
                  (when (> (string-length name) 0)
                    (buffer-set-title (current-buffer) name)
                  ) ;when
                  (buffer-rename (current-buffer)
                    (collab-buffer-url->tmfs (collab-placeholder-doc-id))
                  ) ;buffer-rename
                  (loro-collab-create (collab-server-url) uname)
                  (set-message (string-append "Creating collaborative document (Server "
                                 (collab-server-url)
                                 ")"
                               ) ;string-append
                    "Collaborative"
                  ) ;set-message
                ) ;with-default-view
          ) ;else
    ) ;cond
  ) ;let
) ;tm-define

;; 由文件 url 推导共享文档默认显示名：取文件名（去目录与后缀）。

(define (collab-file->doc-name u)
  (let* ((tail (url->system (url-tail u))) (suffix (url-suffix u)))
    (if (and (string? suffix) (> (string-length suffix) 0))
      (substring tail 0 (- (string-length tail) (+ 1 (string-length suffix))))
      tail
    ) ;if
  ) ;let*
) ;define

;; 选定文件后弹显示名输入框（预填文件名），确认走 collab-new-document-from-file-named。

(define (collab-share-file-prompt-name u)
  (interactive (lambda (name) (collab-new-document-from-file-named u name))
    (list "Document name" "string" (collab-file->doc-name u))
  ) ;interactive
) ;define

;; 打开（上传）本地 .tmu/.tm 文件为共享文档：文件对话框选文件 → 输入显示名
;; （预填文件名）→ 加载文件到 buffer → 标记 collab → CREATE。会话就绪时 C++ 端
;; eager-seed（见 loro_collab.cpp become_ready）把文件内容作为初始全量推到服务端，
;; 无需等待首次编辑。
(tm-define (collab-new-document-from-file)
  (:interactive #t)
  (choose-file collab-share-file-prompt-name "Load file to share" "action_open")
) ;tm-define

(tm-define (collab-new-document-from-file-named u name)
  ;; name 来自 Qt 输入框为 Cork 编码，协作链下游期望 UTF-8（见 collab-new-document-named）。
  (let ((uname (cork->utf8 name)))
    (cond ((and (string? uname)
             (> (string-length uname) 0)
             (not (collab-valid-doc-name? uname))
           ) ;and
           (set-message "Invalid name: 1-64 chars, no \\ / : * ? \" < > | or control chars"
             "Collaborative"
           ) ;set-message
          ) ;
          (else
            ;; 加载文件到 buffer（window-per-buffer 开新窗口，否则新标签页），
            ;; current-buffer 随即切到该文件 buffer。
            (if (window-per-buffer?) (load-buffer-in-new-window u) (load-buffer u))
            ;; 文件内容已随 buffer 载入；改名成 tmfs 占位 URL（内容不变），
            ;; become_ready eager-seed 时仍能把文件内容推到服务端。
            (buffer-rename (current-buffer)
              (collab-buffer-url->tmfs (collab-placeholder-doc-id))
            ) ;buffer-rename
            (loro-collab-create (collab-server-url) uname)
            (set-message (string-append "Uploading file as collaborative document (Server "
                           (collab-server-url)
                           ")"
                         ) ;string-append
              "Collaborative"
            ) ;set-message
          ) ;else
    ) ;cond
  ) ;let
) ;tm-define

;; 加入指定 UUID 的协作文档（非交互：UUID 由 Join 子菜单选中项传入，
;; opt-name 为菜单已知的显示名预填，最终以服务端 DOC 帧内 name 为准）。
;; 建空 buffer 并切到它 → 会话层 JOIN。服务端回 DOC 后补发 snapshot/updates，
;; 首帧到达时把内容构建进 buffer。
(tm-define (collab-join-document doc-id . opt-name)
  (let ((name (if (and (nnull? opt-name) (string? (car opt-name))) (car opt-name) ""))
        (buf-url (collab-buffer-url->tmfs doc-id))
       ) ;
    (when (and (string? doc-id) (> (string-length doc-id) 0))
      (if (in? (url->unix buf-url) (map url->unix (buffer-list)))
        ;; 同进程同 doc_id 已打开：直接跳转，不新建 buffer（同一文档在本进程
        ;; 唯一 buffer，与打开同一文件的行为一致；避免 tmfs URL 冲突）。用
        ;; url->unix 串比较：buffer-exists? 经 url->url，对 tmfs URL 易失配。
        (switch-to-buffer buf-url)
        (with-default-view (if (window-per-buffer?) (open-window) (new-buffer))
          ;; 标题先设为显示名（无名回退 UUID），再 rename：rename 的 propose_title
          ;; 据 old_title 决定，old_title=doc_name 时 keep_old 保留（而非 No
          ;; name→tmfs URL），避免第一次 tab 重建显示 URL。
          (buffer-set-title (current-buffer) (if (> (string-length name) 0) name doc-id))
          ;; join 的 doc_id 已知，直接改名为最终 tmfs URL（无需 become_ready 再改）。
          (buffer-rename (current-buffer) buf-url)
          (loro-collab-join (collab-server-url) doc-id name)
          (set-message (string-append "Joining collaborative document "
                         (if (> (string-length name) 0) name doc-id)
                       ) ;string-append
            "Collaborative"
          ) ;set-message
        ) ;with-default-view
      ) ;if
    ) ;when
  ) ;let
) ;tm-define

;; 触发后台拉取服务端可用文档 UUID（异步、幂等：loading 中为 no-op）。
(tm-define (collab-refresh-docs) (loro-collab-fetch-docs (collab-server-url)))

;; Join 子菜单：展开时触发后台拉取（不阻塞 GUI），按状态显示
;;   loading → "(loading...)"，error → "(unreachable)"，
;;   ready+空 → "(no documents)"，ready+非空 → 各文档项（点击即加入；
;;   菜单文字为显示名，无名回退 UUID，见 collab-doc-label）。
;; Refresh 强制重新拉取。状态经 loro-collab-docs-status 轮询，ImGui 每帧重建
;; 菜单时自动刷新到最新结果，无需缓存。
(tm-menu (collab-docs-menu)
  (with status
    (begin
      ;; 首次展开（idle）触发后台拉取；fetch 立即把状态置 loading，故只触发一次，
      ;; 之后每帧轮询到 loading/ready/error 都不再自动重拉（重拉仅靠 Refresh）
      (when (== (loro-collab-docs-status) "idle")
        (collab-refresh-docs)
      ) ;when
      (loro-collab-docs-status)
    ) ;begin
    (cond ((== status "loading") ("(loading...)" (collab-refresh-docs)))
          ((== status "error") ("(server unreachable)" (collab-refresh-docs)))
          ((and (== status "ready") (null? (loro-collab-docs)))
           ("(no documents)" (collab-refresh-docs))
          ) ;
          (else (with pairs
                  (collab-docs-pairs (loro-collab-docs))
                  (with dups
                    (collab-doc-name-duplicates pairs)
                    (for (p pairs)
                      (with uuid
                        (car p)
                        (with name
                          (cdr p)
                          (with dup?
                            (let ((cell (assoc name dups)))
                              (and cell (> (cdr cell) 1))
                            ) ;let
                            ((eval (collab-doc-label uuid name dup?)) (collab-join-document uuid name))
                          ) ;with
                        ) ;with
                      ) ;with
                    ) ;for
                  ) ;with
                ) ;with
          ) ;else
    ) ;cond
    ---
    ("Refresh" (collab-refresh-docs))
  ) ;with
) ;tm-menu

;; 退出当前协作会话（不断开 buffer，仅关闭上行/下行通道）。
(tm-define (collab-leave)
  (loro-collab-disconnect)
  (set-message "Collaboration ended" "Collaborative")
) ;tm-define

;; === collab 缓冲特殊语义的覆盖 ===
;; collab 文档（云端）：
;;   - 永不"已修改" → 无星号、关闭不弹保存提示、不进自动备份：
;;     已下沉到 C++ editor::need_save（据 tm_buffer_rep::cloud 标志，由会话层
;;     collab_session_manager::get_or_create 在每次挂接时置位——复用残留会话时
;;     构造函数不跑，故必须在 get_or_create 里显式标记）。need_save 是标题星号
;;     / 关闭提示 / tab 星号 / 自动保存的公共收敛点，故不再在 Scheme 侧覆盖
;;     buffer-modified?。
;;   - Save（Cmd+S）→ 静默写一份本地备份（$TEXMACS_HOME_PATH/system/collab-backups/
;;     <doc_id>.tmu），为磁盘类下游（如 autosave 云备份插件需要从磁盘读文档）提供
;;     落点。云端文档本体仍在云端，不 buffer-rename、不切 buffer。占位 URL 跳过。
;;   - Save as 在协作模式下菜单显示为 Download，功能也变为 Download：用 buffer-export
;;     仅写一份用户选定路径的本地副本，不 buffer-rename、不切换 buffer（云端会话不中断）。
;; 用「模块加载时捕获原始绑定」覆盖 save-buffer / save-buffer-as，避免对纯
;; glue 函数用 former（tm-define 对未注册到 tm-defined-table 的函数走 else
;; 分支，former 退化为 noop，会全局破坏）。

(define %original-save-buffer save-buffer)

(tm-define (save-buffer . l)
  (if (collab-buffer? (current-buffer))
    (collab-silent-backup (current-buffer))
    (apply %original-save-buffer l)
  ) ;if
) ;tm-define

;; 协作（云端）文档：把当前 buffer 导出一份本地副本（Download 语义）。
;; 不 buffer-rename、不 buffer-pretend-modified、不写回云端源 buffer——只写 dest。
;; 写盘用 buffer-export（buffer_export，dest≠源，无回写副作用），区别于
;; save-buffer-as 的 rename+save 链（会把云端 buffer 改名成本地文件，破坏协作
;; 会话）。覆盖确认只用 url-test?/user-confirm（均为 glue/原语，全局可见）；
;; tm-files 的 cannot-write? 是模块私有 define，跨模块不可见，且其只读对话框
;; 的"Save as"按钮会递归回到本覆盖，故不复用——写盘失败由 buffer-export 返回
;; 值兜底报"Could not save"。

(define (collab-do-export new-name)
  (with vname
    `(verbatim ,(url->system new-name))
    (with fm
      (url-format new-name)
      ;; 未知后缀兜底 texmacs，避免退化成 verbatim 丢失格式。
      (when (== fm "generic")
        (set! fm "texmacs")
      ) ;when
      (if (buffer-export (current-buffer) new-name fm)
        (set-message `(concat ,"Could not save " ,vname) "Save file")
        (set-message `(concat ,"Saved " ,vname) "Save file")
      ) ;if
    ) ;with
  ) ;with
) ;define

(define (collab-download-copy new-name)
  (if (url-test? new-name "f")
    (user-confirm "File already exists. Really overwrite?"
      #f
      (lambda (answ) (when answ (collab-do-export new-name)))
    ) ;user-confirm
    (collab-do-export new-name)
  ) ;if
) ;define

;; === 协作文档 → 本地不可见备份（Cmd+S / save-buffer 触发）===
;; 云端文档无磁盘落点，磁盘类下游（如 autosave 云备份插件需从磁盘读文档）无法处理。
;; 这里把 tmfs://collab/<doc_id> 映射到 $TEXMACS_HOME_PATH/system/collab-backups/
;; <doc_id>.tmu，由 Cmd+S 同步写一份。collab-silent-backup 是单一同步入口——
;; buffer_export→save_string 同步，返回时字节已落盘，将来 autosave 在它返回后读盘
;; 即可保证时序。占位 URL（pending-…，服务端尚未回真实 doc_id）跳过。

(define (collab-url->doc-id u)
  (let ((s (url->unix u)))
    (substring s (string-length "tmfs://collab/") (string-length s))
  ) ;let
) ;define

(define (collab-backups-dir)
  (url-append (get-texmacs-home-path) (system->url "system/collab-backups"))
) ;define

(define (collab-backup-url doc-id)
  (url-append (get-texmacs-home-path)
    (system->url (string-append "system/collab-backups/" doc-id ".tmu"))
  ) ;url-append
) ;define

;; 写本地备份，返回 #t 成功 / #f 跳过或失败。静默（用户不可见）；仅失败时给一行
;; 提示，不暴露内部路径。不重命名、不进最近文件、不弹窗、不写回云端源 buffer。
;; 单一同步入口：buffer_export→save_string 同步，返回时字节已落盘——将来 autosave
;; 在它返回后读盘即可保证时序。

(define (collab-silent-backup . opt-buf)
  (let ((buf (if (pair? opt-buf) (car opt-buf) (current-buffer))))
    (and (collab-buffer? buf) (collab-do-silent-backup buf))
  ) ;let
) ;define

;; 占位 URL（pending-…）/ 空 doc_id 跳过：服务端尚未回真实 doc_id。

(define (collab-do-silent-backup buf)
  (let ((doc-id (collab-url->doc-id buf)))
    (cond ((string=? doc-id "") #f)
          ((collab-string-prefix? "pending-" doc-id) #f)
          (else (collab-write-backup buf doc-id))
    ) ;cond
  ) ;let
) ;define

;; system/ 启动时已存在，只需建一层 collab-backups。buffer-export 失败时给一行提示。

(define (collab-write-backup buf doc-id)
  (when (not (url-exists? (collab-backups-dir)))
    (system-mkdir (collab-backups-dir))
  ) ;when
  (if (buffer-export buf (collab-backup-url doc-id) "tmu")
    (begin
      (set-message "Backup failed" "Save")
      #f
    ) ;begin
    #t
  ) ;if
) ;define

(define %original-save-buffer-as save-buffer-as)

;; 协作模式下 Save as → Download：仅导出本地副本，不切 buffer。
;; 非协作 buffer 原样委派给原始 save-buffer-as（含其 :argument/:default 元数据、
;; 文件名消毒、权限/覆盖/格式检查与 rename+save 链），行为零变化。
(tm-define (save-buffer-as new-name . args)
  (:argument new-name texmacs-file "Save as")
  (:default new-name (propose-name-buffer))
  (if (collab-buffer? (current-buffer))
    (with-default-view (when (string? new-name)
                         (set! new-name (string-replace new-name ":" "-"))
                         (set! new-name (string-replace new-name ";" "-"))
                       ) ;when
      (collab-download-copy new-name)
    ) ;with-default-view
    (apply %original-save-buffer-as (cons new-name args))
  ) ;if
) ;tm-define
