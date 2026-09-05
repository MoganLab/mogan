;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : export-pdf-default-dir-test.scm
;; DESCRIPTION : 纯逻辑单元测试：导出 PDF 缺省目录策略（Issue #1268）——
;;               tmu 位于 texmacs_path / texmacs_home_path 之外时用 tmu 所在
;;               目录，其余（内置/帮助文档、scratch 草稿、tmfs 云文档）落
;;               Documents/LiiiSTEM；目的地后缀兜底（Issue #1271）——
;;               Browse 选中的文件名不带 pdf 后缀时补 .pdf。
;;               不弹任何 GUI，headless 可跑。
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
  (let ((master (url-append (get-documents-path) "LiiiSTEM/no_name/draft_20260905_120000.tmu")
        ) ;master
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

;; 目的地后缀兜底（Issue #1271）：不带 pdf 后缀补 .pdf，已带则原样保留。

(define (test-ensure-suffix-appends-when-missing)
  (check (export-pdf-ensure-suffix "/tmp/1268/demo") => "/tmp/1268/demo.pdf")
) ;define

(define (test-ensure-suffix-keeps-pdf)
  (check (export-pdf-ensure-suffix "/tmp/1268/demo.pdf") => "/tmp/1268/demo.pdf")
) ;define

(define (test-ensure-suffix-appends-after-other-suffix)
  (check (export-pdf-ensure-suffix "/tmp/1268/demo.bak")
    =>
    "/tmp/1268/demo.bak.pdf"
  ) ;check
) ;define

(tm-define (regtest-export-pdf-default-dir)
  (test-local-tmu-uses-own-dir)
  (test-below-texmacs-path-goes-to-documents)
  (test-below-texmacs-home-path-goes-to-documents)
  (test-scratch-goes-to-documents)
  (test-tmfs-goes-to-documents)
  (test-ensure-suffix-appends-when-missing)
  (test-ensure-suffix-keeps-pdf)
  (test-ensure-suffix-appends-after-other-suffix)
  (check-report)
) ;tm-define
