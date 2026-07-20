;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2040.scm
;; DESCRIPTION : Test magic-paste-shortcut live rebind (notify 重绑 std v/V)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; 验证改 magic-paste-shortcut 偏好后，std v / std V 绑定实时互换
;; （kbd-apply-magic-paste-shortcut 由 notify 触发）。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic generic-kbd))
(import (liii check))

(tm-define (test_2040)
  ;; 记录原始值，测试结束恢复（避免污染全局 preference）
  (let ((orig (get-preference "magic-paste-shortcut")))
    ;; magic-paste 命令当前绑定的 key（std v 或 std V）
    (define (magic-binding) (kbd-find-rev-binding "kbd-magic-paste"))
    (define (paste-binding) (kbd-find-rev-binding "kbd-paste"))

    ;; 默认 ctrl+shift+v：std v = magic-paste，std V = paste
    (set-preference "magic-paste-shortcut" "ctrl+shift+v")
    (check (magic-binding) => "std v")
    (check (paste-binding) => "std V")

    ;; 切到 ctrl+v：std v = paste，std V = magic-paste（互换）
    (set-preference "magic-paste-shortcut" "ctrl+v")
    (check (magic-binding) => "std V")
    (check (paste-binding) => "std v")

    ;; 恢复
    (set-preference "magic-paste-shortcut" orig)))