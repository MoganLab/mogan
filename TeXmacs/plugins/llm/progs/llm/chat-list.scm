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

(import (liii json))

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
         (archived-str (if (or (not archived) (== archived "false")) "false" "true"))
         (actual-created-at (or created-at ""))
         (actual-updated-at (or updated-at created-at ""))
        ) ;
    `((,"sessionId" . ,sid)
      (,"title" . ,title)
      (,"model" . ,model)
      (,"archived" . ,archived-str)
      (,"createdAt" . ,actual-created-at)
      (,"defaultExpandCount" . ,5)
      (,"thinking" . ,thinking)
      (,"search" . ,search)
      (,"updateAt" . ,actual-updated-at))
  ) ;let*
) ;tm-define

;;; ---------- 加载状态 ----------

(tm-define (chat-persist-load-all)
  (let ((manifest-path (chat-persist-manifest-path)))
    (when (file-exists? manifest-path)
      (let* ((manifest (catch #t
                         (lambda () (string->json (string-load (system->url manifest-path))))
                         (lambda args #f)
                       ) ;catch
             ) ;manifest
             (sessions-vec (and (json-object? manifest) (json-ref manifest "sessions")))
             (entries (if (vector? sessions-vec) (vector->list sessions-vec) '()))
            ) ;
        (for-each (lambda (entry)
                    (let* ((sid (json-ref-string entry "sessionId" ""))
                           (title (json-ref-string entry "title" ""))
                           (model (json-ref-string entry "model" ""))
                           (archived-str (json-ref-string entry "archived" "false"))
                           (created-at (json-ref-string entry "createdAt" ""))
                           ;; updateAt 缺失时回退到 createdAt（兼容旧 manifest）
                           (updated-at (json-ref-string entry "updateAt" created-at))
                           (expand-count (json-ref-integer entry "defaultExpandCount" 5))
                           (thinking (json-ref-string entry "thinking" "disabled"))
                           (search (json-ref-string entry "search" "disabled"))
                          ) ;
                      ;; 只传元数据给 C++，不加载 buffer 内容
                      (qt-chat-tab-restore-session sid title model archived-str
                        created-at updated-at expand-count thinking search
                      ) ;qt-chat-tab-restore-session
                    ) ;let*
                  ) ;lambda
          entries
        ) ;for-each
      ) ;let*
    ) ;when
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
         (entry (chat-persist-make-entry session-id title model archived
                  created-at thinking search updated-at
                ) ;chat-persist-make-entry
         ) ;entry
        ) ;
    (chat-persist-ensure-dir! (chat-persist-base-dir))
    (let* ((manifest (if (file-exists? manifest-path)
                       (catch #t
                         (lambda () (string->json (string-load (system->url manifest-path))))
                         (lambda args #f)
                       ) ;catch
                       #f
                     ) ;if
           ) ;manifest
           (version (if (json-object? manifest) (json-ref-integer manifest "version" 1) 1))
           (sessions-vec (and (json-object? manifest) (json-ref manifest "sessions")))
           (entries (if (vector? sessions-vec) (vector->list sessions-vec) '()))
           (found #f)
           (updated-entries (map (lambda (e)
                                   (if (equal? (json-ref-string e "sessionId" "") session-id)
                                     (begin
                                       (set! found #t)
                                       entry
                                     ) ;begin
                                     e
                                   ) ;if
                                 ) ;lambda
                              entries
                            ) ;map
           ) ;updated-entries
           (final-entries (if found updated-entries (append updated-entries (list entry))))
           (new-manifest `((,"version" . ,version)
                           (,"sessions" . ,(list->vector final-entries)))
           ) ;new-manifest
          ) ;
      (string-save (json->string new-manifest) (system->url manifest-path))
    ) ;let*
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
      (let* ((manifest (catch #t
                         (lambda () (string->json (string-load (system->url manifest-path))))
                         (lambda args #f)
                       ) ;catch
             ) ;manifest
             (version (if (json-object? manifest) (json-ref-integer manifest "version" 1) 1))
             (sessions-vec (and (json-object? manifest) (json-ref manifest "sessions")))
             (entries (if (vector? sessions-vec) (vector->list sessions-vec) '()))
             (remaining (filter (lambda (e) (not (equal? (json-ref-string e "sessionId" "") session-id)))
                          entries
                        ) ;filter
             ) ;remaining
             (new-manifest `((,"version" . ,version)
                             (,"sessions" . ,(list->vector remaining)))
             ) ;new-manifest
            ) ;
        (string-save (json->string new-manifest) (system->url manifest-path))
      ) ;let*
    ) ;when
  ) ;let
) ;tm-define
