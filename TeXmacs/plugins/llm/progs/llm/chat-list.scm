;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-list.scm
;; DESCRIPTION : Chat session list persistence (manifest and entries)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (llm chat-list))

(import (liii njson))

;;; ---------- 路径工具 ----------

(tm-define (chat-persist-home-path) (url->system (get-texmacs-home-path)))

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

(tm-define (chat-persist-make-entry sid title model archived . rest)
  (let* ((created-at (if (and (pair? rest) (car rest)) (car rest) (number->string (current-time)))
         ) ;created-at
         (opts (if (pair? rest) (cdr rest) '()))
         (thinking (if (and (pair? opts) (car opts)) (car opts) "disabled"))
         (opts2 (if (pair? opts) (cdr opts) '()))
         (search (if (and (pair? opts2) (car opts2)) (car opts2) "disabled"))
         (updated-at (if (and (pair? opts2) (pair? (cdr opts2)) (cadr opts2)) (cadr opts2) #f)
         ) ;updated-at
         (entry (string->njson "{}"))
        ) ;
    (njson-set! entry "sessionId" sid)
    (njson-set! entry "title" title)
    (njson-set! entry "model" model)
    (njson-set! entry
      "archived"
      (if (or (not archived) (== archived "false")) "false" "true")
    ) ;njson-set!
    (njson-set! entry "createdAt" (or created-at ""))
    (njson-set! entry "defaultExpandCount" 5)
    (njson-set! entry "thinking" thinking)
    (njson-set! entry "search" search)
    ;; updateAt: 最近活跃时间戳，用于排序索引；缺失时回退到 createdAt
    (njson-set! entry "updateAt" (or updated-at created-at ""))
    entry
  ) ;let*
) ;tm-define

;;; ---------- 加载状态 ----------

(tm-define (chat-persist-load-all)
  (let ((manifest-path (chat-persist-manifest-path)))
    (if (not (file-exists? manifest-path))
      (noop)
      (let* ((manifest (file->njson manifest-path))
             (sessions-json (njson-ref manifest "sessions"))
             (entries (njson-array->list sessions-json))
            ) ;
        (for-each (lambda (entry)
                    ;; njson-array->list 返回 alist，用 assoc 访问字段
                    (let* ((sid (cdr (assoc "sessionId" entry)))
                           (title (cdr (assoc "title" entry)))
                           (model (cdr (assoc "model" entry)))
                           (archived-str (cdr (assoc "archived" entry)))
                           (created-at-pair (assoc "createdAt" entry))
                           (created-at (if created-at-pair (cdr created-at-pair) ""))
                           (updated-at-pair (assoc "updateAt" entry))
                           ;; updateAt 缺失时回退到 createdAt（兼容旧 manifest）
                           (updated-at (if updated-at-pair (cdr updated-at-pair) created-at))
                           (expand-count-pair (assoc "defaultExpandCount" entry))
                           (expand-count (if expand-count-pair (cdr expand-count-pair) 5))
                           (thinking-pair (assoc "thinking" entry))
                           (thinking (if thinking-pair (cdr thinking-pair) "disabled"))
                           (search-pair (assoc "search" entry))
                           (search (if search-pair (cdr search-pair) "disabled"))
                          ) ;
                      ;; 只传元数据给 C++，不加载 buffer 内容
                      (qt-chat-tab-restore-session sid
                        title
                        model
                        archived-str
                        created-at
                        updated-at
                        expand-count
                        thinking
                        search
                      ) ;qt-chat-tab-restore-session
                    ) ;let*
                  ) ;lambda
          entries
        ) ;for-each
        (njson-free manifest)
      ) ;let*
    ) ;if
  ) ;let
) ;tm-define

;;; ---------- 增量保存 ----------

(tm-define (chat-persist-update-manifest session-id title model archived . rest)
  (let* ((created-at (if (and (pair? rest) (car rest)) (car rest) (number->string (current-time)))
         ) ;created-at
         (opts (if (pair? rest) (cdr rest) '()))
         (thinking (if (and (pair? opts) (car opts)) (car opts) "disabled"))
         (opts2 (if (pair? opts) (cdr opts) '()))
         (search (if (and (pair? opts2) (car opts2)) (car opts2) "disabled"))
         (updated-at (if (and (pair? opts2) (pair? (cdr opts2)) (cadr opts2)) (cadr opts2) #f)
         ) ;updated-at
         (manifest-path (chat-persist-manifest-path))
         (entry (chat-persist-make-entry session-id
                  title
                  model
                  archived
                  created-at
                  thinking
                  search
                  updated-at
                ) ;chat-persist-make-entry
         ) ;entry
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
        (let ((new-arr (string->njson "[]")) (found #f))
          (for-each (lambda (e)
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
  ) ;let*
) ;tm-define

;;; ---------- 删除持久化会话 ----------

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
          (for-each (lambda (e)
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
