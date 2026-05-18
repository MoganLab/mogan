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
;;
;; 逻辑
;; ----
;; 1. 检查是否提供了 model-opt
;;    - 若提供，调用 chat-tab-session-select-model 设置模型
;; 2. 检查聊天标签页缓冲区是否已存在
;;    - 若存在，直接切换到该缓冲区
;;    - 若不存在，创建新缓冲区并初始化：
;;      a. 设置缓冲区内容为空文档
;;      b. 设置缓冲区标题为 "Chat"
;;      c. 切换到该缓冲区
;;      d. 标记缓冲区为已保存状态
;;
;; 注意
;; ----
;; 此函数是 LLM 聊天功能的入口，负责聊天标签页的创建和切换。
;; 聊天标签页使用固定的 tmfs://chat-tab URL。
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
;; (chat-tab-send message-buffer input-buffer body)
;;
;; 参数
;; ----
;; message-buffer : url
;; 消息缓冲区名称（URL），用于存储对话历史。
;;
;; input-buffer : url
;; 输入缓冲区名称（URL），用户在此输入消息。
;;
;; body : tree
;; 输入消息树，即用户要发送的内容。
;;
;; 返回值
;; ----
;; boolean
;; 委托 chat-tab-session-send 的返回值：
;; - #t : 消息发送成功
;; - #f : 消息为空，未发送
;;
;; 逻辑
;; ----
;; 1. 直接调用 chat-tab-session-send，将参数原样传递
;;
;; 注意
;; ----
;; 此函数是 chat-tab-session.scm 的薄包装层，用于解耦适配层与会话引擎。
;; 所有实际发送逻辑由 chat-tab-session-send 处理。
(tm-define (chat-tab-send message-buffer input-buffer body)
  (:synopsis "Adapter send entry for a chat tab")
  (:argument message-buffer "Message buffer name")
  (:argument input-buffer "Input buffer name")
  (:argument body "Input message tree")
  (chat-tab-session-send message-buffer input-buffer body)
) ;tm-define
