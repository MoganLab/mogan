;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-style.scm
;; DESCRIPTION : Chat session buffer addressing and style packages
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (llm chat-style)
  (:use (utils library cursor) (generic document-style))
) ;texmacs-module

;;; ---------- 全局常量 ----------

(define-public chat-tab-session-name "llm")

;;; ---------- Buffer URL 推导函数 ----------

(tm-define (chat-tab-session->message-buffer session-id)
  (string->url (string-append "tmfs://chat/" session-id "/message"))
) ;tm-define

(tm-define (chat-tab-session->input-buffer session-id)
  (string->url (string-append "tmfs://chat/" session-id "/input"))
) ;tm-define

;;; ---------- 样式包管理 ----------

(tm-define (chat-tab-add-default-style-packages! session-name)
  ;; 偏好驱动，参考 buffer-set-default-style（tm-files.scm:130-146）
  ;; 一次性 set-style-list：逐包 add 会每包触发一次样式树重建，耗时成倍
  (let ((packs (list "number-europe")) (lan (get-preference "language")))
    (when (and (!= lan "english") (in? lan supported-languages))
      (set! packs (append packs (list lan)))
      ;; 中文等 CJK 语言自动加载对应样式包
      (when (== lan "chinese")
        (set! packs (append packs (list "table-captions-above")))
      ) ;when
    ) ;when
    ;; 插件样式包：动态检测，参考 session-edit 的 make-session
    (when (url-exists? (url-unix "$TEXMACS_STYLE_PATH" (string-append session-name ".ts"))
          ) ;url-exists?
      (set! packs (append packs (list session-name)))
    ) ;when
    (set-style-list (append (get-style-list) packs))
  ) ;let
) ;tm-define

(tm-define (chat-tab-load-input-styles! session-id)
  (:synopsis "Load style packages for input buffer only (new conversation)")
  (:argument session-id "Session UUID")
  (let ((in-buf (chat-tab-session->input-buffer session-id)))
    (with-buffer in-buf
      (chat-tab-add-default-style-packages! chat-tab-session-name)
    ) ;with-buffer
  ) ;let
) ;tm-define

(tm-define (chat-tab-sync-dark-style! session-id)
  ;; C++ 侧创建 panel 后调用，同步暗色样式包
  (when (== (get-preference "gui theme") "liii-night")
    (let ((msg-buf (chat-tab-session->message-buffer session-id))
          (in-buf (chat-tab-session->input-buffer session-id))
         ) ;
      (with-buffer msg-buf
        (when (not (has-style-package? "dark"))
          (add-style-package "dark")
        ) ;when
      ) ;with-buffer
      (with-buffer in-buf
        (when (not (has-style-package? "dark"))
          (add-style-package "dark")
        ) ;when
      ) ;with-buffer
    ) ;let
  ) ;when
) ;tm-define
