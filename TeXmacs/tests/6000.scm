;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6000.scm
;; DESCRIPTION : 验证有背景色的PDF导出右边没有细线
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 6000))

(import (liii check))
(check-set-mode! 'report-failed)

(define in-path (string->url "$TEXMACS_PATH/tests/tmu/6000.tmu"))

(define out-path "/tmp/test_6000_export.pdf")

(tm-define (test_6000)
  (when (url-test? out-path "f")
    (url-remove out-path)
  ) ;when
  (load-buffer in-path :strict)
  (export-buffer-to-pdf out-path #f #t)
  (check (url-test? out-path "f") => #t)
  (let* ((py-script (url->system (string->url "$TEXMACS_PATH/tests/python/6000.py")))
         (ok-flag "/tmp/test_6000_ok")
         (cmd (string-append "python3 " py-script " " out-path))
        ) ;
    (when (url-test? ok-flag "f")
      (url-remove ok-flag)
    ) ;when
    (system cmd)
    (check (url-test? ok-flag "f") => #t)
    (when (url-test? ok-flag "f")
      (url-remove ok-flag)
    ) ;when
  ) ;let*
  (check-report)
) ;tm-define
