;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2080.scm
;; DESCRIPTION : 「帮助 -> 版本」QML 对话框数据契约测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-version-dialog-contract)
  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "ok")
  (check (cpp-version-dialog "Version" "Version information") => #t)

  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "cancel")
  (check (cpp-version-dialog "Version" "Version information") => #f)

  (system-setenv "MOGAN_TEST_VERSION_DIALOG" "")
) ;define

(tm-define (test_2080) (test-version-dialog-contract) (check-report))
