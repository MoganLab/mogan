
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE     : startup-tab-file.scm
;; DESCRIPTION: Scheme bindings for startup tab file operations
;; COPYRIGHT  : (C) 2026 Yuki Lu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (startup-tab startup-tab-file)
  (:use (texmacs texmacs tm-server))
  (:use (texmacs texmacs tm-files))
  (:use (texmacs menus file-menu))
  (:use (kernel texmacs tm-dialogue))
  (:use (utils library cursor))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document creation with specific style
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (new-document-with-style style-id)
  ;; Create a new document with the specified style
  ;; style-id: "generic", "beamer", "book", "exam", "letter", "article"
  ;; Use with-buffer to ensure we're working in the correct buffer context
  (with-default-view (let ((buf (if (window-per-buffer?) (open-window) (new-buffer))))
                       ;; Schedule style initialization after buffer is fully set up
                       (delayed (:idle 100) (with-buffer buf (init-style style-id)))
                     ) ;let
  ) ;with-default-view
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; File operations wrappers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (startup-tab-file-open)
  ;; Open file dialog wrapper
  (open-document)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Recent documents management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (startup-tab-get-recent-docs)
  ;; Get recent document paths with the same filtering and ordering
  ;; as File -> Recent used
  (let* ((raw (string->number (get-preference "startup-tab:max-recent")))
         (nr (if (number? raw) raw 10))
         (nr (max 1 nr))
        ) ;
    (map url->system (recent-file-list nr))
  ) ;let*
) ;tm-define

;; C++ 启动页点击最近文档的分派入口：云文档（tmfs://collab/<doc_id>）按 doc_id
;; 重新 join，本地文档走 load-document。与 File→Recent 的 file-list-menu 同语义。
;; 云文档须带上存储里的 title，否则 collab-join-document 名字缺省 → 标题退化为 UUID。
;; collab 分派叠 (loro-enabled?) 门控：loro=no 构建下 loro-collab-join 等 glue
;; 未注册，残留云条目点击改走 load-document 优雅失败，而非撞未注册 glue 崩溃。
(tm-define (startup-tab-open-recent path)
  (let ((u (system->url path)))
    (if (and (collab-buffer? u) (loro-enabled?))
      (collab-join-document (collab-url->doc-id u)
        (or (recent-files-get-name (url->system u)) "")
      ) ;collab-join-document
      (load-document u)
    ) ;if
  ) ;let
) ;tm-define

(tm-define (startup-tab-add-recent-doc path)
  ;; Add or refresh a document in global recent-file state
  (learn-interactive 'recent-buffer (list (cons "0" path)))
) ;tm-define

(tm-define (startup-tab-clear-recent-doc path)
  ;; Remove a specific document from global recent-file state
  (recent-files-remove-by-path path)
) ;tm-define

(tm-define (startup-tab-clear-all-recent)
  ;; Clear all recent documents
  (forget-interactive "recent-buffer")
) ;tm-define
