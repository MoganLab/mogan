;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : updater-test.scm
;; DESCRIPTION : 纯逻辑单元测试：更新器 idle 默认值 + 状态名映射。不触发任何
;;               网络检查，跨平台安全。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r updater-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/utils/misc/updater.scm")

;; 状态名映射：纯 scheme 函数（updater-state-name），无 glue 依赖，全平台可测。

(define (test-updater-state-name)
  (check (updater-state-name 0) => "idle")
  (check (updater-state-name 1) => "checking")
  (check (updater-state-name 2) => "available")
  (check (updater-state-name 3) => "downloading")
  (check (updater-state-name 4) => "ready")
  (check (updater-state-name 5) => "applying")
  (check (updater-state-name 6) => "failed")
  (check (updater-state-name 99) => "unknown")
) ;define

;; idle 默认值：state=0 (UPDATER_IDLE)，版本/错误码为空，进度 0，
;; download/apply 均返回 #f（未到可用/就绪状态）。
;; glue 仅在 Windows(USE_PLUGIN_VELOPACK) 注册；其他平台 (use-plugin-updater?)
;; 为 #f，跳过 glue 断言。

(define (test-updater-idle)
  (when (use-plugin-updater?)
    (check (updater-state) => 0)
    (check (updater-available-version) => "")
    (check (updater-progress) => 0)
    (check (updater-error-code) => "")
    (check (updater-download) => #f)
    (check (updater-apply) => #f)
    (check (updater-download-update) => #f)
    (check (updater-apply-update) => #f)
  ) ;when
) ;define

(tm-define (regtest-updater)
  (test-updater-state-name)
  (test-updater-idle)
  (check-report)
) ;tm-define
