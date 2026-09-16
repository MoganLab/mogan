;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0528.scm
;; DESCRIPTION : 「帮助 -> 检查更新」单元测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/doc/help-funcs.scm")

(check-set-mode! 'report-failed)

(define (test-update-urls)
  ;; 验证默认 profile
  (check (get-update-base-url) => "https://liiistem.cn")
  (check (get-latest-version-url #t)
    =>
    "https://liiistem.cn/api/v1/public/update/win-x64/latest"
  ) ;check
  (check (get-latest-version-url #f)
    =>
    "https://liiistem.cn/api/v1/public/commercial/update/win-x64/latest"
  ) ;check
) ;define

(define (test-semver-update-logic)
  ;; 验证 2026.3.2-rc.1 绝不比 2026.3.5 新 (failed? is-latest? has-new?)
  (check (calc-update-status "2026.3.5" "2026.3.2-rc.1") => '(#f #t #f))
  (check (calc-update-status "2026.3.2-rc.1" "2026.3.5") => '(#f #f #t))
  (check (calc-update-status "2026.3.5" "2026.3.5") => '(#f #t #f))
  (check (calc-update-status "2026.3.5" "v2026.3.5") => '(#f #t #f))
  (check (calc-update-status "2026.3.5" "2026.3.6") => '(#f #f #t))
  (check (calc-update-status "2026.3.2-rc.1" "2026.3.2-rc.2") => '(#f #f #t))
  (check (calc-update-status "2026.3.2-rc.2" "2026.3.2-rc.1") => '(#f #t #f))
  (check (calc-update-status "2026.3.5" "") => '(#t #f #f))
  (check (calc-update-status "2026.3.5" "bad-ver") => '(#t #f #f))
) ;define

(define (test-updates-disabled-predicate)
  ;; disabled 通道 ⇒ 更新器平台判定为「已禁用」；非更新器平台（Linux）无此语义，
  ;; 恒 #f。前者依赖 use-plugin-updater? 短路，避免取未加载模块的绑定。
  (with original
    (get-preference "update-channel")
    (set-preference "update-channel" "disabled")
    (check (updates-disabled?) => (use-plugin-updater?))
    (set-preference "update-channel" "stable")
    (check (updates-disabled?) => #f)
    (set-preference "update-channel" "beta")
    (check (updates-disabled?) => #f)
    (set-preference "update-channel" original)
  ) ;with
) ;define

(define (test-primary-enabled-gate)
  ;; 硬约定：disabled 通道下主按钮恒不可用——即使版本上确有更新（has-new? 为 #t），
  ;; 弹窗也只作信息展示。
  (check (calc-primary-enabled #t #t) => #f)
  (check (calc-primary-enabled #f #t) => #f)
  (check (calc-primary-enabled #t #f) => #t)
  (check (calc-primary-enabled #f #f) => #f)
) ;define

(define (test-version-dialog-smoke)
  ;; 测试钩子模拟确认
  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "cancel")
  (check-for-updates)
  (mogan-version)
  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "")
) ;define

(define (test-disabled-channel-smoke)
  ;; 禁用通道下走一遍 check-for-updates：updates-disabled? 会在更新器平台取
  ;; updater-current-channel，此用例保证该新增调用路径不报错（钩子 cancel ⇒
  ;; 不触发任何动作，也不会打开 Qt 弹窗）。
  (with original
    (get-preference "update-channel")
    (set-preference "update-channel" "disabled")
    (system-setenv "MOGAN_TEST_VERSION_DIALOG" "cancel")
    (check-for-updates)
    (set-preference "update-channel" original)
  ) ;with
  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "")
) ;define

(tm-define (test_0528)
  (test-update-urls)
  (test-semver-update-logic)
  (test-updates-disabled-predicate)
  (test-primary-enabled-gate)
  (test-version-dialog-smoke)
  (test-disabled-channel-smoke)
  (check-report)
) ;tm-define
