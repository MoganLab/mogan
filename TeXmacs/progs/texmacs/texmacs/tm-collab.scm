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

;; 协作服务端地址：经 C++ loro-collab-server-url 取（native 读 OS env
;; MOGAN_LORO_SERVER；WASM 读 window.MOGAN_LORO_SERVER / ?loro_server= 查询参数），
;; 未设置回落 ws://127.0.0.1:8765。运行期可配，无需重编译。
(tm-define (collab-server-url) (loro-collab-server-url))

;; === collab 缓冲（云端文档）的标识 ===
;; 单会话：同时只有一个 collab 缓冲。new/join 时把新建 buffer 的 url 记入
;; collab-buffer-url，collab-buffer? 据此判定。下游 buffer-modified?/save-buffer
;; 覆盖用它门控特殊语义（不标修改/不可保存/关闭不提示）。

(define collab-buffer-url #f)

(tm-define (collab-buffer? u) (and collab-buffer-url (== u collab-buffer-url)))

;; 把当前 buffer 标记为 collab 缓冲（在 with-default-view 建 new-buffer 后调用，
;; 此时 (current-buffer) 即新建的协作 buffer）。
(tm-define (collab-mark-current-buffer)
  (set! collab-buffer-url (current-buffer))
) ;tm-define

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
  (cond ((and (string? name)
           (> (string-length name) 0)
           (not (collab-valid-doc-name? name))
         ) ;and
         (set-message "Invalid name: 1-64 chars, no \\ / : * ? \" < > | or control chars"
           "Collaborative"
         ) ;set-message
        ) ;
        (else (with-default-view (if (window-per-buffer?) (open-window) (new-buffer))
                (collab-mark-current-buffer)
                (loro-collab-create (collab-server-url) name)
                (set-message (string-append "Creating collaborative document (Server "
                               (collab-server-url)
                               ")"
                             ) ;string-append
                  "Collaborative"
                ) ;set-message
              ) ;with-default-view
        ) ;else
  ) ;cond
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
  (cond ((and (string? name)
           (> (string-length name) 0)
           (not (collab-valid-doc-name? name))
         ) ;and
         (set-message "Invalid name: 1-64 chars, no \\ / : * ? \" < > | or control chars"
           "Collaborative"
         ) ;set-message
        ) ;
        (else
          ;; 加载文件到 buffer（window-per-buffer 开新窗口，否则新标签页），
          ;; current-buffer 随即切到该文件 buffer。
          (if (window-per-buffer?) (load-buffer-in-new-window u) (load-buffer u))
          (collab-mark-current-buffer)
          (loro-collab-create (collab-server-url) name)
          (set-message (string-append "Uploading file as collaborative document (Server "
                         (collab-server-url)
                         ")"
                       ) ;string-append
            "Collaborative"
          ) ;set-message
        ) ;else
  ) ;cond
) ;tm-define

;; 加入指定 UUID 的协作文档（非交互：UUID 由 Join 子菜单选中项传入，
;; opt-name 为菜单已知的显示名预填，最终以服务端 DOC 帧内 name 为准）。
;; 建空 buffer 并切到它 → 会话层 JOIN。服务端回 DOC 后补发 snapshot/updates，
;; 首帧到达时把内容构建进 buffer。
(tm-define (collab-join-document doc-id . opt-name)
  (let ((name (if (and (nnull? opt-name) (string? (car opt-name))) (car opt-name) "")))
    (when (and (string? doc-id) (> (string-length doc-id) 0))
      (with-default-view (if (window-per-buffer?) (open-window) (new-buffer))
        (collab-mark-current-buffer)
        ;; 标题立即设为显示名（无名回退 UUID）；DOC 帧到达后 C++ become_ready
        ;; 会以服务端 name 重设标题（最终一致）
        (buffer-set-title (current-buffer) (if (> (string-length name) 0) name doc-id))
        (loro-collab-join (collab-server-url) doc-id name)
        (set-message (string-append "Joining collaborative document "
                       (if (> (string-length name) 0) name doc-id)
                     ) ;string-append
          "Collaborative"
        ) ;set-message
      ) ;with-default-view
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
;;   - 永不"已修改" → 无星号、关闭不弹保存提示、不进自动备份（filter buffer-modified?）
;;   - Save 无效（文档在云端）→ 提示用 Save as 导出本地副本；Save as 正常
;; 用「模块加载时捕获原始绑定」覆盖，避免对纯 glue 函数用 former（tm-define 对
;; 未注册到 tm-defined-table 的函数走 else 分支，former 退化为 noop，会全局破坏）。

(define %original-buffer-modified? buffer-modified?)

(define %original-save-buffer save-buffer)

(tm-define (buffer-modified? name)
  (if (collab-buffer? name) #f (%original-buffer-modified? name))
) ;tm-define

(tm-define (save-buffer . l)
  (if (collab-buffer? (current-buffer))
    (set-message "Cloud document: use Save as to export a local copy" "Save")
    (apply %original-save-buffer l)
  ) ;if
) ;tm-define
