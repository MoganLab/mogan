;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 4157.scm
;; DESCRIPTION : 「编辑 → 搜索最近打开的文档」QML 迁移的数据契约测试。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [4157] 验证编辑菜单「搜索最近打开的文档」改用专用 QML 弹窗后：
;;     - recent-grep-what 从 cpp-search-recent-dialog 返回 tree 提取搜索词
;;     - 测试钩子 cancel 不搜索；ok 返回空 what（scheme 侧跳过空串）
;;     - 语义保持：OK 非空词仍走 docgrep-in-recent（内容 grep）
;;
;; USAGE
;;   xmake b stem
;;   xmake r 4157
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/doc/docgrep.scm")

(check-set-mode! 'report-failed)

(define (check-what-ok)
  (check (recent-grep-what (stree->tree '(tuple (tuple "what" "parsing"))))
    =>
    "parsing")
) ;define

(define (check-what-cancel)
  (check (recent-grep-what (stree->tree '(tuple))) => #f)
) ;define

(define (check-hook)
  (system-setenv "MOGAN_TEST_SEARCH_RECENT" "cancel")
  (check (recent-grep-what (cpp-search-recent-dialog)) => #f)
  (system-setenv "MOGAN_TEST_SEARCH_RECENT" "ok")
  (check (recent-grep-what (cpp-search-recent-dialog)) => "")
  (system-setenv "MOGAN_TEST_SEARCH_RECENT" "")
) ;define

(tm-define (test_4157)
  (check-what-ok)
  (check-what-cancel)
  (check-hook)
  (check-report)
) ;tm-define
