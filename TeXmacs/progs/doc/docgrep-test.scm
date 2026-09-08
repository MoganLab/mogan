;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : docgrep-test.scm
;; DESCRIPTION : 纯逻辑单元测试：搜索结果页链接文本的编码。
;;               文档无标题节点时，链接文本回退为文件路径（UTF-8），
;;               必须经 utf8->cork 转换后再进入内部文档树，
;;               否则中文文件名在结果页显示为 cork 乱码（见 devel/0522.md）。
;;               不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r docgrep-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/doc/docgrep.scm")

;; 递归判断 stree 中是否存在满足 pred 的字符串叶子

(define (stree-any-string pred x)
  (cond ((string? x) (pred x))
        ((pair? x) (or (stree-any-string pred (car x)) (stree-any-string pred (cdr x))))
        (else #f)
  ) ;cond
) ;define

;; 中文文件名、无标题节点的文档：链接文本应为 cork 转义串，不含原始 UTF-8 中文。

(define (test-link-text-cork-encoded)
  (let* ((cn-name "测试文档0522.tm")
         (u (url-append (url-temp-dir) (system->url cn-name)))
         (path (url->system u))
        ) ;
    (string-save "<\\TeXmacs|2.1>\n\n<style|generic>\n\n<\\body>\n  hello\n</body>"
      u
    ) ;string-save
    (with st
      (tm->stree (build-doc-search-results "hello" (list (cons path 1))))
      (check-true (stree-any-string (cut string-contains? <> (utf8->cork cn-name)) st)
      ) ;check-true
      (check-false (stree-any-string (cut string-contains? <> cn-name) st))
    ) ;with
  ) ;let*
) ;define

(tm-define (regtest-docgrep) (test-link-text-cork-encoded) (check-report))
