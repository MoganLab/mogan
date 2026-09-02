;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 4157_grep.scm
;; DESCRIPTION : 验证「搜索最近打开的文档」是内容 grep（走 tmfs://grep/），不是按文件名打开。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   补充 GUI 无法覆盖（4160 抢占鼠标）的语义验证：docgrep-in-recent 应加载
;;   tmfs://grep/ 查询文档（txtgrep 最近文档内容 → 内容搜索结果页），而不是
;;   按文件名模糊打开某个 tm:// 文件。以此证明迁移到 QML 对话框后仍保留「内容 grep」。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 4157_grep
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/doc/docgrep.scm")

(check-set-mode! 'report-failed)

;; docgrep-in-recent 打开 tmfs://grep/?type=recent&what=<term> 查询文档。
;; 命中/未命中都会渲染成一个「内容搜索结果」伪文档（tmfs://grep/），而非文件名列表。

(define (check-content-grep-routing)
  (new-document)
  (docgrep-in-recent "somenonexistentword_zzz_4157")
  (let* ((u (current-buffer)) (s (if u (url->string u) "")))
    (display "  current buffer = ")
    (display s)
    (newline)
    (check-true (string-starts? s "tmfs://grep/"))
  ) ;let*
) ;define

;; query 结构：type=recent + what=<term>，交给 grep tmfs（内容搜索）。

(define (check-query-shape)
  (with q
    (list->query (list (cons "type" "recent") (cons "what" "alpha")))
    (check-true (string-contains? q "type=recent"))
    (check-true (string-contains? q "what=alpha"))
  ) ;with
) ;define

(tm-define (test_4157_grep)
  (check-query-shape)
  (check-content-grep-routing)
  (check-report)
) ;tm-define
