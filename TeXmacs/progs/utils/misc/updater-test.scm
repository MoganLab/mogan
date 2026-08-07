;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : updater-test.scm
;; DESCRIPTION : 纯逻辑单元测试：更新器 idle 默认值。不触发任何网络检查，
;;               跨平台安全——非 Windows 上基础 tm_updater 的默认实现同样
;;               满足这些断言。
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

;; idle 状态下：state=0 (UPDATER_IDLE)，版本/错误码为空，进度 0，
;; download/apply 均返回 #f（未到可用/就绪状态）。

(define (test-updater-idle)
  (check (updater-state) => 0)
  (check (updater-available-version) => "")
  (check (updater-progress) => 0)
  (check (updater-error-code) => "")
  (check (updater-download) => #f)
  (check (updater-apply) => #f)
) ;define

(tm-define (regtest-updater) (test-updater-idle) (check-report))
