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
  ;; 验证 2026.3.2-rc.1 绝不比 2026.3.5 新 (failed? is-latest? primary-enabled?)
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

(define (test-version-dialog-smoke)
  ;; 测试钩子模拟确认
  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "cancel")
  (check-for-updates)
  (mogan-version)
  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "")
) ;define

(tm-define (test_0528)
  (test-update-urls)
  (test-semver-update-logic)
  (test-version-dialog-smoke)
  (check-report)
) ;tm-define
