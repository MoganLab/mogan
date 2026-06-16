(import (liii check) (liii path) (liii os))

(check-set-mode! 'report-failed)

;; path-copy-into
;; 将文件复制到目标目录，保留原文件名。
;;
;; 语法
;; ----
;; (path-copy-into source target-dir)
;;
;; 参数
;; ----
;; source : path | string
;; 源文件路径。
;; target-dir : path | string
;; 目标目录路径。
;;
;; 返回值
;; ------
;; boolean
;; 复制成功返回 #t，失败返回 #f。

(let ((src "tests/liii/path/path-copy-into-src.txt")
      (dir "tests/liii/path/copy-into-dir")
     ) ;
  (path-write-text src "hello path-copy-into")
  (when (not (path-exists? dir))
    (g_mkdir dir)
  ) ;when
  (check-true (path-copy-into src dir))
  (check (path-read-text (path-join dir (path "path-copy-into-src.txt")))
    =>
    "hello path-copy-into"
  ) ;check
  (path-unlink src)
  (path-unlink (path-join dir (path "path-copy-into-src.txt")))
  (path-rmdir dir)
) ;let

(check-report)
