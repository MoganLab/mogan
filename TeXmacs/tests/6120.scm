;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6120.scm
;; DESCRIPTION : 集成测试：修复插入参考文献时 bib 文件路径含中文导致插入失败
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)
(load "./TeXmacs/plugins/latex/progs/latex/bibtex-bib-widgets.scm")

(define test-dir
  (string-replace (url->string (url-append (url-temp-dir) "参考文献测试_6120"))
    "\\"
    "/"
  ) ;string-replace
) ;define

(define test-bib (string-append test-dir "/ref.bib"))

(define (setup-test-file)
  (system-mkdir (string->url test-dir))
  (string-save (string-append "@article{test2024,\n"
                 "  author = {Zhang, San},\n" "  title = {A Test Article},\n"
                 "  journal = {Test Journal},\n" "  year = {2024}\n" "}\n"
               ) ;string-append
    (string->url test-bib)
  ) ;string-save
) ;define

(define (cleanup-test-file)
  (system-remove (string->url test-bib))
  (system-rmdir (string->url test-dir))
) ;define

;; 验收断言：修复成功时，中文路径必须能正常解析出有效的预览树

(define (verify-chinese-path-accepted)
  ;; 1. 物理文件必须真实存在
  (check (url-exists? (string->url test-bib)) => #t)

  ;; 2. 核心验收断言：解析中文路径的 bib 文件
  (let ((res (bib-to-tree test-bib "tm-plain")))
    ;; 状态判定：必须是 "valid"；若为 "not_found" 说明修复未成功
    (check (car res) => "valid")
    ;; 错误文案判定：必须为空 ""；若包含 "文件不存在" 说明路径未正确解析
    (check (cadr res) => "")
    ;; 内容判定：必须成功解析生成有效的文档树
    (check (tree? (caddr res)) => #t)
  ) ;let
) ;define

;; 对比断言：明确展示 Bug 的触发特征（供排查与对比）

(define (verify-bug-reproduction-pattern)
  ;; C++ qt_scheme_quote 将 UTF-8 误转为 Cork 编码 <less>#XXXX<gtr> 逃逸串
  (let* ((corked-path (utf8->cork test-bib)) (res (bib-to-tree corked-path "tm-plain")))
    ;; 逃逸路径在文件系统不存在
    (check (url-exists? (string->url corked-path)) => #f)
    ;; 触发 Bug 时表现为 not_found 与 文件不存在
    (check (car res) => "not_found")
    (check (cork->utf8 (cadr res)) => "文件不存在")
  ) ;let*
) ;define

(tm-define (test_6120)
  (setup-test-file)
  (verify-chinese-path-accepted)
  (verify-bug-reproduction-pattern)
  (cleanup-test-file)
  (check-report)
) ;tm-define
