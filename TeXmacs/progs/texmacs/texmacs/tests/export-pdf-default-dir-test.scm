;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : export-pdf-default-dir-test.scm
;; DESCRIPTION : 纯逻辑单元测试：导出 PDF 缺省目录策略（Issue #1268）——
;;               tmu 位于 texmacs_path / texmacs_home_path 之外时用 tmu 所在
;;               目录，其余（内置/帮助文档、scratch 草稿、tmfs 云文档）落
;;               Documents/LiiiSTEM。不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r export-pdf-default-dir-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/texmacs/tm-print.scm")

;; 本地 tmu 在 texmacs 路径之外：默认导出到 tmu 所在目录。

(define (test-local-tmu-uses-own-dir)
  (check (url->system (export-pdf-default-dir (system->url "/tmp/1268/demo.tmu")))
    =>
    "/tmp/1268"
  ) ;check
) ;define

;; 内置/帮助文档（texmacs_path 下）：不写安装目录，落 Documents/LiiiSTEM。

(define (test-below-texmacs-path-goes-to-documents)
  (let ((master (url-append (get-texmacs-path) "doc/tmdoc/manual.tmu"))
        (doc-dir (url->system (url-append (get-documents-path) "LiiiSTEM")))
       ) ;
    (check (url->system (export-pdf-default-dir master)) => doc-dir)
  ) ;let
) ;define

;; 用户配置目录（texmacs_home_path）下的文档同样落 Documents/LiiiSTEM。

(define (test-below-texmacs-home-path-goes-to-documents)
  (let ((master (url-append (get-texmacs-home-path) "system/no_name/demo.tmu"))
        (doc-dir (url->system (url-append (get-documents-path) "LiiiSTEM")))
       ) ;
    (check (url->system (export-pdf-default-dir master)) => doc-dir)
  ) ;let
) ;define

;; scratch 草稿无本地 tmu 位置，不落 no_name 暂存目录，落 Documents/LiiiSTEM。

(define (test-scratch-goes-to-documents)
  (let ((master (url-append (get-documents-path) "LiiiSTEM/no_name/draft_20260905_120000.tmu"))
        (doc-dir (url->system (url-append (get-documents-path) "LiiiSTEM")))
       ) ;
    (check (url-scratch? master) => #t)
    (check (url->system (export-pdf-default-dir master)) => doc-dir)
  ) ;let
) ;define

;; tmfs（云/帮助文档）master 非本地路径，落 Documents/LiiiSTEM。

(define (test-tmfs-goes-to-documents)
  (let ((master (system->url "tmfs://help/index.tmu"))
        (doc-dir (url->system (url-append (get-documents-path) "LiiiSTEM")))
       ) ;
    (check (url->system (export-pdf-default-dir master)) => doc-dir)
  ) ;let
) ;define

(tm-define (regtest-export-pdf-default-dir)
  (test-local-tmu-uses-own-dir)
  (test-below-texmacs-path-goes-to-documents)
  (test-below-texmacs-home-path-goes-to-documents)
  (test-scratch-goes-to-documents)
  (test-tmfs-goes-to-documents)
  (check-report)
) ;tm-define
