;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Simple Message Widget - 极简消息展示组件
;;
;; 功能：提供一个基础的 texmacs-input 组件用于消息输入和显示
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (simple-message message-widgets)
  (:use (simple-message message-utils)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 基础消息窗口实现
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((simple-message-widget aux) quit)
  (padded
    (resize "400px" "300px"
      (texmacs-input `(document (paragraph "请输入内容..."))
                     `(style (tuple "generic")) aux))
    ===
    (explicit-buttons
      ("关闭" (quit))
      >>>
      ("发送" (simple-message-send aux)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 消息发送处理
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (simple-message-send aux)
  (:interactive #t)
  (let* ((content (buffer-get-body aux))
         (text (if (tree? content) (tree->string content) ""))
         (reply (string-append "收到消息：" text)))
    (buffer-set-body aux `(document (paragraph ,reply)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 窗口打开函数
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (simple-message)
  (:interactive #t)
  (change-auxiliary-widget-focus)
  (let* ((aux (buffer-new)))
    (buffer-set-master aux (current-buffer))
    (auxiliary-widget (simple-message-widget aux)
                      (lambda ()
                        (buffer-close aux))
                      (translate "Simple Message") aux)))

(tm-define (open-simple-message-window)
  (:interactive #t)
  (simple-message))