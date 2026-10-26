;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : go-menu-test.scm
;; DESCRIPTION : Go 菜单数据契约与 Scheme 接口纯逻辑单元测试
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r go-menu-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/menus/file-menu.scm")

(define (test-go-menu-meta-shape)
  (let ((meta (go-menu-meta)))
    (check (list? meta) => #t)
    (check (pair? (assoc "label_recent" meta)) => #t)
    (check (pair? (assoc "label_open" meta)) => #t)
    (check (pair? (assoc "recent" meta)) => #t)
    (check (list? (cdr (assoc "recent" meta))) => #t)
  ) ;let
) ;define

;; recent-entry-remove-if-missing 的失效清理契约：
;; 缺失本地条目移除、存在条目与云文档保留。只添加临时条目并在末尾清理，
;; 不触碰用户真实最近列表。

(define (test-recent-entry-remove-if-missing)
  (let ((kept "/tmp/mogan_6231_kept.tm")
        (missing "/tmp/mogan_6231_missing.tm")
        (cloud "tmfs://collab/doc_6231")
       ) ;
    (string-save "6231" (string->url kept))
    (when (url-exists? missing)
      (system-remove (string->url missing))
    ) ;when
    (when (recent-files-get-name missing)
      (recent-files-remove-by-path missing)
    ) ;when
    (when (recent-files-get-name cloud)
      (recent-files-remove-by-path cloud)
    ) ;when
    (recent-files-learn kept "kept.tm")
    (recent-files-learn missing "missing.tm")
    ;; 存在的本地文件保留
    (recent-entry-remove-if-missing (system->url kept))
    (check (recent-files-get-name kept) => "kept.tm")
    ;; 缺失的本地文件从最近列表移除
    (recent-entry-remove-if-missing (system->url missing))
    (check (recent-files-get-name missing) => #f)
    ;; 云文档非磁盘路径，不受失效清理影响
    (recent-files-learn cloud "cloud.tm")
    (recent-entry-remove-if-missing (string->url cloud))
    (check (recent-files-get-name cloud) => "cloud.tm")
    ;; 还原现场
    (recent-files-remove-by-path kept)
    (recent-files-remove-by-path cloud)
    (system-remove (string->url kept))
  ) ;let
) ;define

(tm-define (regtest-go-menu)
  (test-go-menu-meta-shape)
  (test-recent-entry-remove-if-missing)
  (check-report)
) ;tm-define
