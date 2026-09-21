;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : theme-menu-test.scm
;; DESCRIPTION : 菜单与工具栏主题名称国际化（多语言）逻辑单元测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r theme-menu-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/various/theme-menu.scm")

(define (test-theme-names-english)
  (let ((old-lang (get-output-language)))
    (set-output-language "english")
    ;; 英文界面：菜单项应该显示为 Light 和 Dark
    (check (basic-theme-menu-name "plain") => "Light")
    (check (basic-theme-menu-name "dark") => "Dark")
    ;; 英文界面：工具栏按钮名
    (check (basic-theme-button-name "plain") => "Theme")
    (check (basic-theme-button-name "dark") => "Dark")
    ;; 其他主题名首字母大写
    (check (basic-theme-menu-name "blackboard") => "Blackboard")
    (set-output-language old-lang)
  ) ;let
) ;define

(define (test-theme-names-chinese)
  (let ((old-lang (get-output-language)))
    (set-output-language "chinese")
    ;; 中文界面：菜单项应该显示为 浅色 和 深色
    (check (cork->utf8 (basic-theme-menu-name "plain")) => "浅色")
    (check (cork->utf8 (basic-theme-menu-name "dark")) => "深色")
    ;; 中文界面：工具栏按钮名
    (check (cork->utf8 (basic-theme-button-name "plain")) => "主题")
    (check (cork->utf8 (basic-theme-button-name "dark")) => "深色")
    (set-output-language old-lang)
  ) ;let
) ;define

(tm-define (regtest-theme-menu)
  (test-theme-names-english)
  (test-theme-names-chinese)
  (check-report)
) ;tm-define
