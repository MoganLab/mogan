;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1314.scm
;; DESCRIPTION : 验证幻灯片/多屏幕文档导出为 PDF 时正确展开为多页
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1314))

(import (liii check))
(check-set-mode! 'report-failed)

(define in-path (string->url "$TEXMACS_PATH/tests/tmu/1314.tmu"))

(define out-path "/tmp/test_1314_export.pdf")

(tm-define (test_1314)
  (when (url-test? out-path "f")
    (url-remove out-path)
  ) ;when
  (load-buffer in-path :strict)
  (export-buffer-to-pdf out-path)
  (check (url-test? out-path "f") => #t)
  (check (string-contains? (string-load out-path) "/Count 2") => #t)
  (check-report)
) ;tm-define
