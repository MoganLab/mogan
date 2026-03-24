;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Simple Message Module - 主模块文件
;;
;; 功能：加载所有简单消息组件
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (simple-message simple-message))

(use-modules (simple-message message-widgets))
(use-modules (simple-message message-kbd))
(use-modules (simple-message message-utils))
(use-modules (simple-message message-menu))