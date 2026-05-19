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
  ) ;:use
) ;texmacs-module

(import (liii njson)
  (liii os)
  (liii string)
)

;;; ---------- 路径工具 ----------

(define (chat-persist-home-path)
  (url->system (get-texmacs-home-path))
) ;define

(define (chat-persist-base-dir)
  (string-append (chat-persist-home-path) "/system/ai-chat-sessions")
) ;define

(define (chat-persist-manifest-path)
  (string-append (chat-persist-base-dir) "/manifest.json")
) ;define

(define (chat-persist-message-path session-id)
  (string-append (chat-persist-base-dir) "/" session-id "/message.tm")
) ;define

;;; ---------- 目录管理 ----------

(tm-define (chat-persist-parent-dir dir)
  (url->system (url-head (system->url dir)))
) ;tm-define

(define (chat-persist-ensure-dir! dir)
  (if (not (file-exists? dir))
    (begin
      (chat-persist-ensure-dir! (chat-persist-parent-dir dir))
      (mkdir dir)
    ) ;begin
  ) ;if
) ;define

;;; ---------- 保存状态 ----------

(define chat-persist-save-sessions '())

(tm-define (chat-persist-save-begin) (set! chat-persist-save-sessions '()))

(tm-define (chat-persist-save-session session-id title model archived)
  (let ((msg-path (chat-persist-message-path session-id))
        (msg-buf (chat-tab-session->message-buffer session-id))
       ) ;
    (chat-persist-ensure-dir! (chat-persist-parent-dir msg-path))
    (buffer-export msg-buf (system->url msg-path) "tmu")
    (set! chat-persist-save-sessions
      (cons (list session-id title model archived) chat-persist-save-sessions)
    ) ;set!
  ) ;let
) ;tm-define

(tm-define (chat-persist-make-entry sid title model archived)
  (let ((entry (string->njson "{}")))
    (njson-set! entry "sessionId" sid)
    (njson-set! entry "title" title)
    (njson-set! entry "model" model)
    (njson-set! entry "archived" (if archived "true" "false"))
    entry
  ) ;let
) ;tm-define

(tm-define (chat-persist-save-end)
  (let* ((manifest-path (chat-persist-manifest-path))
         (entries (map (lambda (entry)
                         (chat-persist-make-entry (first entry)
                           (second entry)
                           (third entry)
                           (fourth entry)
                         ) ;chat-persist-make-entry
                       ) ;lambda
                    chat-persist-save-sessions
                  ) ;map
         ) ;entries
         (manifest (string->njson "{\"version\":1,\"sessions\":[]}"))
         (sessions-arr (njson-ref manifest "sessions"))
        ) ;
    (for-each (lambda (e) (njson-append! sessions-arr e)) entries)
    (chat-persist-ensure-dir! (chat-persist-base-dir))
    (njson->file manifest-path manifest)
    (njson-free manifest)
    (for-each njson-free entries)
    (set! chat-persist-save-sessions '())
  ) ;let*
) ;tm-define

;;; ---------- 加载状态 ----------

(tm-define (chat-persist-load-all)
  (let ((manifest-path (chat-persist-manifest-path)))
    (if (not (file-exists? manifest-path))
      '()
      (let* ((manifest (file->njson manifest-path))
             (sessions-json (njson-ref manifest "sessions"))
             (entries (njson-array->list sessions-json))
             (result '())
            ) ;
        (for-each (lambda (entry)
                    (let* ((sid (njson-ref entry "sessionId"))
                           (title (njson-ref entry "title"))
                           (model (njson-ref entry "model"))
                           (archived-str (njson-ref entry "archived"))
                           (archived (== archived-str "true"))
                           (msg-path (chat-persist-message-path sid))
                           (msg-buf (chat-tab-session->message-buffer sid))
                          ) ;
                      (when (file-exists? msg-path)
                        (let ((file-url (system->url msg-path)))
                          (buffer-load file-url)
                          (buffer-set-body msg-buf (buffer-get-body file-url))
                          (buffer-pretend-saved msg-buf)
                        ) ;let
                      ) ;when
                      (set! result (cons (list sid title model archived) result))
                    ) ;let*
                  ) ;lambda
          entries
        ) ;for-each
        (njson-free manifest)
        (for-each njson-free entries)
        (reverse result)
      ) ;let*
    ) ;if
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
