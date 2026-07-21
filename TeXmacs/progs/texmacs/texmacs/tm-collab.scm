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

;; 协作服务端地址：优先 OS 环境变量 MOGAN_LORO_SERVER，否则本地默认。
;; 注意用 system-getenv（OS 环境），而非 get-env（编辑器/buffer 环境）。
(tm-define (collab-server-url)
  (let ((u (system-getenv "MOGAN_LORO_SERVER")))
    (if (or (not u) (== (string-length u) 0)) "ws://127.0.0.1:8765" u)
  ) ;let
) ;tm-define

;; 新建协作文档：先建空 buffer 并切到它（成为当前编辑器），再让会话层
;; 连服务端 CREATE。服务端分配 UUID 后回 DOC，会话层据此置位协作开关；
;; 用户随后首次编辑会 seed shadow 并把初始全量上行。
(tm-define (collab-new-document)
  (with-default-view (if (window-per-buffer?) (open-window) (new-buffer))
    (loro-collab-create (collab-server-url))
    (set-message (string-append "Creating collaborative document (Server "
                   (collab-server-url)
                   ")"
                 ) ;string-append
      "Collaborative"
    ) ;set-message
  ) ;with-default-view
) ;tm-define

;; 加入指定 UUID 的协作文档（非交互：UUID 由 Join 子菜单选中项传入）。
;; 建空 buffer 并切到它 → 会话层 JOIN。服务端回 DOC 后补发 snapshot/updates，
;; 首帧到达时把内容构建进 buffer。
(tm-define (collab-join-document doc-id)
  (when (and (string? doc-id) (> (string-length doc-id) 0))
    (with-default-view (if (window-per-buffer?) (open-window) (new-buffer))
      (loro-collab-join (collab-server-url) doc-id)
      (set-message (string-append "Joining collaborative document " doc-id)
        "Collaborative"
      ) ;set-message
    ) ;with-default-view
  ) ;when
) ;tm-define

;; 触发后台拉取服务端可用文档 UUID（异步、幂等：loading 中为 no-op）。
(tm-define (collab-refresh-docs) (loro-collab-fetch-docs (collab-server-url)))

;; Join 子菜单：展开时触发后台拉取（不阻塞 GUI），按状态显示
;;   loading → "(loading...)"，error → "(unreachable)"，
;;   ready+空 → "(no documents)"，ready+非空 → 各 UUID 项（点击即加入）。
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
          (else (for (id (loro-collab-docs)) ((eval `(verbatim ,id)) (collab-join-document id)))
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
