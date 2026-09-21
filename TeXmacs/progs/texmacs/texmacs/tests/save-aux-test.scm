;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : save-aux-test.scm
;; DESCRIPTION : 纯逻辑单元测试：STEM 格式文档【保存辅助数据】默认关闭验证（Issue 6002）
;;               - STEM 格式文档 save-aux 默认应为 "false"，save-aux-enabled? 为 #f
;;               - 非 STEM 格式（如 TMU）save-aux 默认保持为 "true"，save-aux-enabled? 为 #t
;;               - toggle-save-aux 可在开启和关闭之间正确切换
;;               - STEM 文档默认导出时不保存 auxiliary / references 辅助数据
;;               - STEM 文档显式开启 save-aux 后导出时保留辅助数据
;;               不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r save-aux-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/texmacs/tm-tools.scm")
(load "./TeXmacs/progs/texmacs/texmacs/tm-files.scm")

;; 1. STEM 格式文档默认 save-aux 为 "false"，save-aux-enabled? 为 #f
(define (test-stem-default-save-aux-off)
  (let* ((stem-path "/tmp/test_stem_default_6002.stem")
         (stem-url (system->url stem-path)))
    (string-save "(document (TeXmacs \"2.1.4\") (body (document \"Hello\")))" stem-url)
    (load-buffer stem-url)
    (check (get-env "save-aux") => "false")
    (check (save-aux-enabled?) => #f)
    (system-remove stem-url)
  ) ;let*
) ;define

;; 2. TMU 格式文档默认 save-aux 为 "true"，save-aux-enabled? 为 #t
(define (test-tmu-default-save-aux-on)
  (let* ((tmu-path "/tmp/test_tmu_default_6002.tmu")
         (tmu-url (system->url tmu-path)))
    (string-save "(document (TeXmacs \"2.1.4\") (body (document \"Hello\")))" tmu-url)
    (load-buffer tmu-url)
    (check (get-env "save-aux") => "true")
    (check (save-aux-enabled?) => #t)
    (system-remove tmu-url)
  ) ;let*
) ;define

;; 3. STEM 文档下 toggle-save-aux 切换
(define (test-stem-toggle-save-aux)
  (let* ((stem-path "/tmp/test_stem_toggle_6002.stem")
         (stem-url (system->url stem-path)))
    (string-save "(document (TeXmacs \"2.1.4\") (body (document \"Hello\")))" stem-url)
    (load-buffer stem-url)
    (check (save-aux-enabled?) => #f)
    (toggle-save-aux)
    (check (save-aux-enabled?) => #t)
    (check (get-env "save-aux") => "true")
    (toggle-save-aux)
    (check (save-aux-enabled?) => #f)
    (check (get-env "save-aux") => "false")
    (system-remove stem-url)
  ) ;let*
) ;define

;; 4. STEM 格式导出：默认不保存辅助数据，手动开启后保存辅助数据
(define (test-stem-export-aux-behavior)
  (let* ((orig-path "/tmp/test_orig_6002.stem")
         (orig-url (system->url orig-path))
         (dest-default-path "/tmp/test_dest_default_6002.stem")
         (dest-default-url (system->url dest-default-path))
         (dest-enabled-path "/tmp/test_dest_enabled_6002.stem")
         (dest-enabled-url (system->url dest-enabled-path)))
    (string-save
      "(document (TeXmacs \"2.1.4\") (body (document \"Hello\")) (references (collection (associate \"k\" \"v\"))))"
      orig-url
    ) ;string-save
    (load-buffer orig-url)
    ;; 默认导出，辅助数据应被丢弃
    (buffer-export orig-url dest-default-url "stem")
    (let ((text-default (string-load dest-default-url)))
      (check (string-contains? text-default "references") => #f)
      (check (string-contains? text-default "auxiliary") => #f)
    ) ;let
    ;; 显式开启辅助数据保存后导出，辅助数据应被保留
    (toggle-save-aux)
    (buffer-export orig-url dest-enabled-url "stem")
    (let ((text-enabled (string-load dest-enabled-url)))
      (check (string-contains? text-enabled "references") => #t)
      (check (string-contains? text-enabled "save-aux") => #t)
    ) ;let
    (system-remove orig-url)
    (system-remove dest-default-url)
    (system-remove dest-enabled-url)
  ) ;let*
) ;define

;; 5. TMU 格式导出：默认保留辅助数据
(define (test-tmu-export-aux-behavior)
  (let* ((orig-path "/tmp/test_tmu_orig_6002.tmu")
         (orig-url (system->url orig-path))
         (dest-path "/tmp/test_tmu_dest_6002.tmu")
         (dest-url (system->url dest-path)))
    (string-save
      "<TMU|<tuple|1.1.0|2026.3.0>>\n\nHello\n\n<\\references>\n  <\\collection>\n    <associate|k|v>\n  </collection>\n</references>\n"
      orig-url
    ) ;string-save
    (load-buffer orig-url)
    (buffer-export orig-url dest-url "tmu")
    (let ((text (string-load dest-url)))
      (check (string-contains? text "references") => #t)
    ) ;let
    (system-remove orig-url)
    (system-remove dest-url)
  ) ;let*
) ;define

(tm-define (regtest-save-aux)
  (test-stem-default-save-aux-off)
  (test-tmu-default-save-aux-on)
  (test-stem-toggle-save-aux)
  (test-stem-export-aux-behavior)
  (test-tmu-export-aux-behavior)
  (check-report)
) ;tm-define
