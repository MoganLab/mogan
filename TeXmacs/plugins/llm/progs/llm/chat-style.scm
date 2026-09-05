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
  (:use (utils library cursor) (generic document-style) (llm chat-tree-ops))
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
    ;; 插件样式包：动态检测
    (when (url-exists? (url-append (get-texmacs-path)
                         (string-append "plugins/" session-name
                           "/packages/session/" session-name ".stem"
                         ) ;string-append
                       ) ;url-append
          ) ;url-exists?
      (set! packs (append packs (list session-name)))
    ) ;when
    ;; 深色主题下自动带上 dark 样式包，新建对话的输入/消息区跟随主题
    (when (== (get-preference "gui theme") "liii-night")
      (set! packs (append packs (list "dark")))
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

(tm-define (chat-tab-sync-session-styles! session-id)
  ;; C++ 侧在消息嵌入编辑器建立（含 set_buffer_tree 覆盖）后调用。
  ;; 无视图阶段 with-buffer 会静默跳过样式操作，而 texmacs_input_widget
  ;; 对已有 buffer 会整体覆盖样式，故须在视图就绪后按当前 buffer 补齐
  ;; 默认样式包，点击历史会话恢复后 llm 等插件包才不缺失。
  ;; set-style-list 归一化去重，包已齐时重复调用不触发样式树重建。
  (let ((msg-buf (chat-tab-session->message-buffer session-id))
        (in-buf (chat-tab-session->input-buffer session-id))
       ) ;
    (chat-tab-with-buffer msg-buf
      (when chat-tab-focus-ok?
        (chat-tab-add-default-style-packages! chat-tab-session-name)
      ) ;when
    ) ;chat-tab-with-buffer
    (with-buffer in-buf
      (chat-tab-add-default-style-packages! chat-tab-session-name)
    ) ;with-buffer
  ) ;let
) ;tm-define
