;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-print-test.scm
;; DESCRIPTION : 纯逻辑单元测试：导出覆盖确认（6207）的备份文件名计算与
;;               headless 下的覆盖改名行为。不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r tm-print-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/texmacs/tm-print.scm")

;; 备份名：基名_时间戳.后缀，路径目录部分剔除

(define (test-backup-name-pdf)
  (check (pdf-export-backup-name "/tmp/exports/foo.pdf" "20260928_153000" 0)
    =>
    "foo_20260928_153000.pdf"
  ) ;check
) ;define

;; 嵌入附件的双后缀（.tmu.pdf）：时间戳插在最后一个后缀之前，保持 .pdf 后缀完整

(define (test-backup-name-tmu-pdf)
  (check (pdf-export-backup-name "/tmp/foo.tmu.pdf" "20260928_153000" 0)
    =>
    "foo.tmu_20260928_153000.pdf"
  ) ;check
) ;define

;; 同名备份冲突时追加 -N（对齐 scratch 草稿的唯一化规则），0 不带序号

(define (test-backup-name-collision-index)
  (check (pdf-export-backup-name "foo.pdf" "20260928_153000" 2)
    =>
    "foo_20260928_153000-2.pdf"
  ) ;check
) ;define

;; 无后缀防御：直接追加时间戳，不吞基名末字符

(define (test-backup-name-no-suffix)
  (check (pdf-export-backup-name "foo" "20260928_153000" 0)
    =>
    "foo_20260928_153000"
  ) ;check
) ;define

;; 备份目的地：保留原目录（覆盖含空格路径），无冲突时不加序号

(define (test-backup-dest-keeps-dir)
  (check (url->system (pdf-export-backup-dest "/tmp/a b/foo.pdf" "99991231_235959"))
    =>
    "/tmp/a b/foo_99991231_235959.pdf"
  ) ;check
) ;define

;; headless 覆盖语义（6207）：无弹窗按「是」处理——旧文件改名为
;; 「基名_时间戳.pdf」留在原目录（内容不丢），原路径让位给新导出

(define (test-export-confirm-overwrite-headless)
  (with t
    (url-temp)
    (with u
      (url-glue t ".pdf")
      (string-save "old content" u)
      (check (url-test? u "f") => #t)
      (check (export-confirm-overwrite u) => #t)
      (check (url-test? u "f") => #f)
      (with backups
        (url-read-directory (url-head u)
          (string-append (url->string (url-tail t)) "_*")
        ) ;url-read-directory
        (check (length backups) => 1)
        (check (string-load (car backups)) => "old content")
        (for-each url-remove backups)
      ) ;with
    ) ;with
  ) ;with
) ;define

(tm-define (regtest-tm-print)
  (test-backup-name-pdf)
  (test-backup-name-tmu-pdf)
  (test-backup-name-collision-index)
  (test-backup-name-no-suffix)
  (test-backup-dest-keeps-dir)
  (test-export-confirm-overwrite-headless)
  (check-report)
) ;tm-define
