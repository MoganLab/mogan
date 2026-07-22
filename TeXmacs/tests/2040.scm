;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2040.scm
;; DESCRIPTION : Test language ConfirmRestart 三按钮（重启/稍后/取消）语义
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; 用 MOGAN_TEST_CONFIRM_RESTART 钩子驱动 cpp-confirm-restart 返回值，覆盖：
;;   later  -> 只写值不实时切（get-preference 已是新值，get-output-language 不变）
;;   cancel -> 回滚旧值（get-preference 回到 old）
;; （restart 会触发重启，不在自动化测试覆盖范围）
;;
;; 覆盖缺口：later 的落盘未断言（scheme 侧无 preference 文件路径入口），仅验内存值。
;; 误删 later-proc 的 save-preferences 此测试不会失败；后续可补 glue 读文件验证。
;;
;; 运行：MOGAN_TEST_CONFIRM_RESTART=later xmake r 2040
;;      （钩子在 run_qml_dialog 之前 return，headless 可跑，无需 MOGAN_TEST_GUI）
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (texmacs menus preferences-menu))
(import (liii check))

(tm-define (test_2040)
  (let ((orig (get-preference "language")) (orig-out (get-output-language)))
    ;; 统一回到 english 作为起点，避免受历史 preference 影响
    (cpp-set-preference "language" "english")
    (set-output-language "english")

    ;; --- later：写值不实时切 ---
    (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "later")
    (set-language-and-notify "chinese")
    ;; preference 已写入新值
    (check (get-preference "language") => "chinese")
    ;; 但输出语言未实时切换（later 用 cpp-set-preference-silent 跳过 notify-language）
    (check (get-output-language) => "english")

    ;; --- cancel：回滚旧值 ---
    (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "cancel")
    (set-language-and-notify "english")
    ;; cancel 不落定，回滚；此时旧值是 chinese（上一步 later 写入的）
    (check (get-preference "language") => "chinese")

    ;; 还原（空串不命中钩子的 restart/later/cancel，等同取消）
    (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "")
    (cpp-set-preference "language" orig)
    (set-output-language orig-out)
  ) ;let
) ;tm-define
