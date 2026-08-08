;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-persist.scm
;; DESCRIPTION : Chat session persistence across restarts
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (llm chat-persist)
  (:use (llm chat-list)
    (llm chat-style)
    (llm chat-protocol)
    (dynamic session-edit)
    (texmacs texmacs tm-files)
    (utils library cursor)
  ) ;:use
) ;texmacs-module

;;; ---------- 标题提取 ----------

(tm-define (chat-persist-extract-title session-id)
  (let* ((in-buf (chat-tab-session->input-buffer session-id))
         (body (buffer-get-body in-buf))
         (verbatim-text (texmacs->verbatim body))
         (title (string-replace verbatim-text "\n" " "))
        ) ;
    title
  ) ;let*
) ;tm-define

;;; ---------- 加载会话内容 ----------

(tm-define (chat-persist-load-session-content session-id n)
  (let ((msg-path (chat-persist-message-path session-id))
        (msg-buf (chat-tab-session->message-buffer session-id))
       ) ;
    (when (file-exists? msg-path)
      ;; 用 tree-import 读取文件内容，不经过 buffer 系统
      ;; 避免 buffer-load 创建临时文件 buffer 导致多余 tab
      ;; 避免 buffer-set-body 对已有嵌入式 editor 触发 assign 导致 crash
      (let* ((doc (tree-import (system->url msg-path) "generic"))
             (body (tmfile-extract doc 'body))
            ) ;
        (when body
          (buffer-set-body msg-buf body)
          (with-buffer msg-buf
            (session-unfold-last-n n)
            (chat-tab-add-default-style-packages! "llm")
            (go-end)
          ) ;with-buffer
          (buffer-pretend-saved msg-buf)
        ) ;when
      ) ;let*
    ) ;when
  ) ;let
) ;tm-define

(tm-define (chat-scroll-message-to-end session-id)
  (let ((msg-buf (chat-tab-session->message-buffer session-id)))
    (when msg-buf
      (with-buffer msg-buf (go-end))
    ) ;when
  ) ;let
) ;tm-define

;;; ---------- 增量保存 ----------

(tm-define (chat-persist-export-buffer session-id)
  (let ((msg-path (chat-persist-message-path session-id))
        (msg-buf (chat-tab-session->message-buffer session-id))
       ) ;
    (chat-persist-ensure-dir! (chat-persist-parent-dir msg-path))
    (buffer-export msg-buf (system->url msg-path) "tmu")
  ) ;let
) ;tm-define

;;; ---------- 导出会话到指定路径 ----------

(tm-define (chat-persist-export-session-to session-id target-path)
  (let ((msg-buf (chat-tab-session->message-buffer session-id))
        (msg-path (chat-persist-message-path session-id))
       ) ;
    ;; 确保 buffer 内容已写入磁盘
    (chat-persist-ensure-dir! (chat-persist-parent-dir msg-path))
    (buffer-export msg-buf (system->url msg-path) "tmu")
    ;; 复制到用户指定路径
    (when (file-exists? msg-path)
      (chat-persist-ensure-dir! (chat-persist-parent-dir target-path))
      (system-copy (system->url msg-path) (system->url target-path))
      ;; 加入最近文档列表
      (startup-tab-add-recent-doc target-path)
    ) ;when
  ) ;let
) ;tm-define
