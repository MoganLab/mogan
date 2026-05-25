;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-adapter.scm
;; DESCRIPTION : Adapter layer between chat tab container and session engine
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (dynamic chat-adapter)
  (:use (dynamic chat-tab-session)
    (dynamic chat-session-persist)
    (texmacs texmacs tm-files)
    (texmacs texmacs tm-server)
  ) ;:use
) ;texmacs-module

(define chat-tab-url (string->url "tmfs://chat-tab"))

;; open-llm-chat-tab
;; 打开或切换到 LLM 聊天标签页。
;;
;; 语法
;; ----
;; (open-llm-chat-tab . model-opt)
;;
;; 参数
;; ----
;; model-opt : list
;; 可选参数列表。若提供，第一个元素为要选择的模型名称。
;;
;; 返回值
;; ----
;; #<unspecified>
;; 无显式返回值。

(tm-define (open-llm-chat-tab . model-opt)
  (:synopsis "Open or switch to the LLM Chat tab")
  (when (nnull? model-opt)
    (chat-tab-session-select-model (car model-opt))
  ) ;when
  (if (buffer-exists? chat-tab-url)
    (switch-to-buffer chat-tab-url)
    (begin
      (buffer-set chat-tab-url '(document ""))
      (buffer-set-title chat-tab-url "Chat")
      (switch-to-buffer chat-tab-url)
      (buffer-pretend-saved chat-tab-url)
    ) ;begin
  ) ;if
) ;tm-define

;; chat-tab-send
;; 聊天标签的适配器发送入口。
;;
;; 语法
;; ----
;; (chat-tab-send session-id)
;;
;; 参数
;; ----
;; session-id : string
;; 会话 UUID。
;;
;; 返回值
;; ----
;; boolean
;; 委托 chat-tab-session-send 的返回值：
;; - #t : 消息发送成功
;; - #f : 消息为空，未发送

(tm-define (chat-tab-send session-id)
  (:synopsis "Adapter send entry for a chat tab")
  (:argument session-id "Session UUID")
  (chat-tab-session-send session-id)
) ;tm-define

;; chat-tab-cancel
;; 聊天标签的适配器取消入口。
;;
;; 语法
;; ----
;; (chat-tab-cancel session-id)
;;
;; 参数
;; ----
;; session-id : string
;; 会话 UUID。

(tm-define (chat-tab-cancel session-id)
  (:synopsis "Adapter cancel entry for a chat tab")
  (:argument session-id "Session UUID")
  (let* ((st (chat-tab-get-state session-id))
         (model (if st (car st) "Kimi-VLM"))
         (plugin-ses (string-append model ":chat-tab:" session-id))
        ) ;
    (if (!= (connection-status "llm" plugin-ses) 0)
      (begin
        (connection-stop "llm" plugin-ses)
        (plugin-cancel "llm" plugin-ses #t)
        ;; kill 子进程后 plugin 完成回调不会再触发，手动通知 C++ 恢复 Idle
        (chat-tab-notify-state session-id "idle")
      ) ;begin
    ) ;if
  ) ;let*
) ;tm-define
