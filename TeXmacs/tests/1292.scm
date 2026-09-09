;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1292.scm
;; DESCRIPTION : 「焦点/文档 → 增加宏包 → 其他宏包」QML 迁移的数据契约测试。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1292] 验证「其他宏包」改用专用 QML 弹窗后：
;;     - add-package-result 从 cpp-add-package-dialog 返回 tree 提取宏包名
;;     - 测试钩子 cancel 返回空 tuple；ok 返回空宏包名；指定包名返回相应包名
;;     - package-exists? 校验宏包是否存在
;;     - open-add-package-dialog 在宏包不存在时弹确认提示并不追加，存在时追加宏包
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1292
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/generic/document-style.scm")

(check-set-mode! 'report-failed)

(define (check-result-ok)
  (check (add-package-result (stree->tree '(tuple (tuple "package"
                                                    "preview-ref"))))
    =>
    "preview-ref"
  ) ;check
) ;define

(define (check-result-cancel)
  (check (add-package-result (stree->tree '(tuple))) => #f)
) ;define

(define (check-hook)
  (system-setenv "MOGAN_TEST_ADD_PACKAGE" "cancel")
  (check (add-package-result (cpp-add-package-dialog)) => #f)
  (system-setenv "MOGAN_TEST_ADD_PACKAGE" "ok")
  (check (add-package-result (cpp-add-package-dialog)) => "")
  (system-setenv "MOGAN_TEST_ADD_PACKAGE" "preview-ref")
  (check (add-package-result (cpp-add-package-dialog)) => "preview-ref")
  (system-setenv "MOGAN_TEST_ADD_PACKAGE" "")
) ;define

(define (check-package-exists)
  (check (package-exists? "chinese") => #t)
  (check (package-exists? "preview-ref") => #t)
  (check (package-exists? "non-existent-pkg-xyz-12345") => #f)
  (check (package-exists? "") => #f)
) ;define

(define (check-dialog-nonexistent-package)
  (let* ((orig-style (get-style-list)))
    (system-setenv "MOGAN_TEST_ADD_PACKAGE" "non-existent-pkg-xyz-12345")
    (system-setenv "MOGAN_TEST_CONFIRM_QUESTION" "0")
    (open-add-package-dialog)
    (check (get-style-list) => orig-style)
    (system-setenv "MOGAN_TEST_ADD_PACKAGE" "")
    (system-setenv "MOGAN_TEST_CONFIRM_QUESTION" "")
  ) ;let*
) ;define

(define (check-dialog-valid-package)
  (let* ((orig-style (get-style-list)))
    (system-setenv "MOGAN_TEST_ADD_PACKAGE" "preview-ref")
    (open-add-package-dialog)
    (check (has-style-package? "preview-ref") => #t)
    (system-setenv "MOGAN_TEST_ADD_PACKAGE" "")
    (set-style-list orig-style)
  ) ;let*
) ;define

(tm-define (test_1292)
  (check-result-ok)
  (check-result-cancel)
  (check-hook)
  (check-package-exists)
  (check-dialog-nonexistent-package)
  (check-dialog-valid-package)
  (check-report)
) ;tm-define
