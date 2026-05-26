;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-session-persist.scm
;; DESCRIPTION : Chat session persistence across restarts
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic chat-session-persist)
  (:use (dynamic chat-tab-session)
    (texmacs texmacs tm-files)
    (utils library cursor)
  ) ;:use
) ;texmacs-module

(import (liii njson)
  (liii os)
)

;;; ---------- 路径工具 ----------

(tm-define (chat-persist-home-path)
  (url->system (get-texmacs-home-path))
) ;tm-define

(tm-define (chat-persist-base-dir)
  (string-append (chat-persist-home-path) "/system/ai-chat-sessions")
) ;tm-define

(tm-define (chat-persist-manifest-path)
  (string-append (chat-persist-base-dir) "/manifest.json")
) ;tm-define

(tm-define (chat-persist-message-path session-id)
  (string-append (chat-persist-base-dir) "/" session-id "/message.tmu")
) ;tm-define

;;; ---------- 目录管理 ----------

(tm-define (chat-persist-parent-dir dir)
  (url->system (url-head (system->url dir)))
) ;tm-define

(tm-define (chat-persist-ensure-dir! dir)
  (if (not (file-exists? dir))
    (begin
      (chat-persist-ensure-dir! (chat-persist-parent-dir dir))
      (mkdir dir)
    ) ;begin
  ) ;if
) ;tm-define

;;; ---------- JSON 条目 ----------

(tm-define (chat-persist-make-entry sid title model archived created-at)
  (let ((entry (string->njson "{}")))
    (njson-set! entry "sessionId" sid)
    (njson-set! entry "title" title)
    (njson-set! entry "model" model)
    (njson-set! entry "archived" (if (or (not archived) (== archived "false")) "false" "true"))
    (njson-set! entry "createdAt" (or created-at ""))
    entry
  ) ;let
) ;tm-define

;;; ---------- 标题提取 ----------

;; chat-persist-extract-title
;; 从文档树中提取纯文本标题。
;;
;; 语法
;; ----
;; (chat-persist-extract-title session-id max-len)
;;
;; 参数
;; ----
;; session-id : string
;;   会话 UUID，用于推导输入 buffer URL。
;; max-len : int
;;   标题最大字符数。
;;
;; 返回值
;; ----
;; string
;;   提取的标题字符串。

(tm-define (chat-persist-extract-title session-id)
  (let* ((in-buf (chat-tab-session->input-buffer session-id))
         (body (buffer-get-body in-buf))
        ) ;
    (chat-persist-extract-title-from-tree body)
  ) ;let*
) ;tm-define

;; chat-persist-extract-title-from-tree
;; 从文档树 body 中提取纯文本标题（拼接所有原子文本）。

(tm-define (chat-persist-extract-title-from-tree body)
  (chat-persist-extract-title-loop body 0 "")
) ;tm-define

;; 内部递归：遍历 DOCUMENT 子节点，拼接所有原子文本。

(tm-define (chat-persist-extract-title-loop body idx result)
  (if (or (not (tree-children body))
          (>= idx (length (tree-children body))))
    result
    (let ((child (tree-ref body idx)))
      (if (tree-atomic? child)
        (chat-persist-extract-title-loop body (+ idx 1)
          (string-append result (tree->string child)))
        (chat-persist-extract-title-loop body (+ idx 1) result)
      ) ;if
    ) ;let
  ) ;if
) ;tm-define

;;; ---------- 加载状态 ----------

;; chat-persist-load-all
;; 启动时仅加载元数据（sessionId, title, model, archived），
;; 不加载消息内容到 buffer，实现延迟加载。
;;
;; 语法
;; ----
;; (chat-persist-load-all)
;;
;; 说明
;; ----
;; 消息内容由 chat-persist-load-session-content 在用户点击会话时按需加载。

(tm-define (chat-persist-load-all)
  (let ((manifest-path (chat-persist-manifest-path)))
    (if (not (file-exists? manifest-path))
      (display "[chat-persist] load-all: manifest not found\n")
      (let* ((manifest (file->njson manifest-path))
             (sessions-json (njson-ref manifest "sessions"))
             (entries (njson-array->list sessions-json))
            ) ;
        (display "[chat-persist] load-all: found ")
        (display (length entries))
        (display " sessions in manifest\n")
        (for-each (lambda (entry)
                    ;; njson-array->list 返回 alist，用 assoc 访问字段
                    (let* ((sid (cdr (assoc "sessionId" entry)))
                           (title (cdr (assoc "title" entry)))
                           (model (cdr (assoc "model" entry)))
                           (archived-str (cdr (assoc "archived" entry)))
                           (created-at-pair (assoc "createdAt" entry))
                           (created-at (if created-at-pair (cdr created-at-pair) ""))
                          ) ;
                      (display "[chat-persist]   restoring meta: sid=")
                      (display sid)
                      (newline)
                      ;; 只传元数据给 C++，不加载 buffer 内容
                      (qt-chat-tab-restore-session sid title model archived-str created-at)
                    ) ;let*
                  ) ;lambda
          entries
        ) ;for-each
        (njson-free manifest)
      ) ;let*
    ) ;if
  ) ;let
) ;tm-define

;; chat-persist-load-session-content
;; 按需加载指定会话的消息内容到 buffer。
;; 由 ChatController::loadSessionContent 在用户点击会话时调用。
;;
;; 语法
;; ----
;; (chat-persist-load-session-content session-id)
;;
;; 参数
;; ----
;; session-id : string
;;   会话 UUID。

(tm-define (chat-persist-load-session-content session-id)
  (let ((msg-path (chat-persist-message-path session-id))
        (msg-buf (chat-tab-session->message-buffer session-id))
       ) ;
    (when (file-exists? msg-path)
      (display "[chat-persist] load-session-content: sid=")
      (display session-id)
      (newline)
      ;; 用 tree-import 读取文件内容，不经过 buffer 系统
      ;; 避免 buffer-load 创建临时文件 buffer 导致多余 tab
      ;; 避免 buffer-set-body 对已有嵌入式 editor 触发 assign 导致 crash
      (let* ((doc (tree-import (system->url msg-path) "generic"))
             (body (tmfile-extract doc 'body)))
        (when body
          (buffer-set-body msg-buf body)
          ;; 恢复宏包：加载时 buffer 只有默认 generic 样式，
          ;; 需要重新添加 number-europe、language 等宏包以正确渲染
          (with-buffer msg-buf
            (chat-tab-add-default-style-packages!))
          (buffer-pretend-saved msg-buf)))
    ) ;when
  ) ;let
) ;tm-define

;;; ---------- 增量保存 ----------

;; chat-persist-export-buffer
;; 只导出 message buffer 到磁盘。
;;
;; 语法
;; ----
;; (chat-persist-export-buffer session-id)

(tm-define (chat-persist-export-buffer session-id)
  (let ((msg-path (chat-persist-message-path session-id))
        (msg-buf (chat-tab-session->message-buffer session-id))
       ) ;
    (chat-persist-ensure-dir! (chat-persist-parent-dir msg-path))
    (buffer-export msg-buf (system->url msg-path) "tmu")
  ) ;let
) ;tm-define

;; chat-persist-update-manifest
;; 只更新 manifest 中的条目（不导出 buffer）。
;;
;; 语法
;; ----
;; (chat-persist-update-manifest session-id title model archived created-at)

(tm-define (chat-persist-update-manifest session-id title model archived created-at)
  (let ((manifest-path (chat-persist-manifest-path))
        (entry (chat-persist-make-entry session-id title model archived created-at))
       ) ;
    (chat-persist-ensure-dir! (chat-persist-base-dir))
    (if (not (file-exists? manifest-path))
      ;; manifest 不存在：创建新的，直接构建包含 entry 的数组
      (let* ((manifest (string->njson "{\"version\":1,\"sessions\":[]}"))
             (new-arr (string->njson "[]"))
            ) ;
        (njson-append! new-arr entry)
        (njson-set! manifest "sessions" new-arr)
        (njson->file manifest-path manifest)
        (njson-free new-arr)
        (njson-free manifest)
      ) ;let*
      ;; manifest 存在：读取，查找并更新或追加
      (let* ((manifest (file->njson manifest-path))
             (sessions-arr (njson-ref manifest "sessions"))
             (entries (njson-array->list sessions-arr))
            ) ;
        (let ((new-arr (string->njson "[]"))
              (found #f)
             ) ;
          (for-each
            (lambda (e)
              (let ((sid-pair (assoc "sessionId" e)))
                (if (and sid-pair (== (cdr sid-pair) session-id))
                  (begin
                    (njson-append! new-arr entry)
                    (set! found #t)
                  ) ;begin
                  (njson-append! new-arr (json->njson e))
                ) ;if
              ) ;let
            ) ;lambda
            entries
          ) ;for-each
          (when (not found)
            (njson-append! new-arr entry)
          ) ;when
          (njson-drop! manifest "sessions")
          (njson-set! manifest "sessions" new-arr)
          (njson->file manifest-path manifest)
          (njson-free new-arr)
          (njson-free manifest)
        ) ;let
      ) ;let*
    ) ;if
    (njson-free entry)
  ) ;let
) ;tm-define

;; chat-persist-save-one
;; 导出 buffer 并更新 manifest（组合调用）。

(tm-define (chat-persist-save-one session-id title model archived created-at)
  (chat-persist-export-buffer session-id)
  (chat-persist-update-manifest session-id title model archived created-at)
) ;tm-define

;;; ---------- 删除持久化会话 ----------

;; chat-persist-delete-one
;; 从磁盘和 manifest 中删除指定会话的持久化数据。
;;
;; 语法
;; ----
;; (chat-persist-delete-one session-id)
;;
;; 参数
;; ----
;; session-id : string
;;   会话 UUID。

(tm-define (chat-persist-delete-one session-id)
  (:synopsis "Delete a chat session from persistent storage")
  (:argument session-id "Session UUID")
  ;; 1. 删除会话目录及消息文件
  (let ((session-dir (string-append (chat-persist-base-dir) "/" session-id)))
    (when (file-exists? session-dir)
      (let ((msg-path (chat-persist-message-path session-id)))
        (when (file-exists? msg-path)
          (system-remove (system->url msg-path))
        ) ;when
      ) ;let
      (system-rmdir (system->url session-dir))
    ) ;when
  ) ;let
  ;; 2. 从 manifest 中移除条目
  (let ((manifest-path (chat-persist-manifest-path)))
    (when (file-exists? manifest-path)
      (let* ((manifest (file->njson manifest-path))
             (sessions-arr (njson-ref manifest "sessions"))
             (entries (njson-array->list sessions-arr))
            ) ;
        (let ((new-arr (string->njson "[]")))
          (for-each
            (lambda (e)
              (let ((sid-pair (assoc "sessionId" e)))
                (when (not (and sid-pair (== (cdr sid-pair) session-id)))
                  (njson-append! new-arr (json->njson e))
                ) ;when
              ) ;let
            ) ;lambda
            entries
          ) ;for-each
          (njson-drop! manifest "sessions")
          (njson-set! manifest "sessions" new-arr)
          (njson->file manifest-path manifest)
          (njson-free new-arr)
          (njson-free manifest)
        ) ;let
      ) ;let*
    ) ;when
  ) ;let
) ;tm-define

;;; ---------- 注册恢复后的会话 ----------

(tm-define (chat-persist-register-session session-id model)
  (let ((st (chat-tab-get-state session-id)))
    (when (not st)
      (chat-tab-set-state! session-id (chat-tab-state model))
    ) ;when
  ) ;let
) ;tm-define
