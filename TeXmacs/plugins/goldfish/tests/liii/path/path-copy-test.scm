(import (liii check) (liii path) (liii os))

(check-set-mode! 'report-failed)

;; path-copy
;; 将文件复制到目标路径。
;;
;; 语法
;; ----
;; (path-copy source target)
;;
;; 参数
;; ----
;; source : path | string
;; 源文件路径。
;; target : path | string
;; 目标文件路径。
;;
;; 返回值
;; ------
;; boolean
;; 复制成功返回 #t，失败返回 #f。

(let ((src "tests/liii/path/path-copy-src.txt")
      (dst "tests/liii/path/path-copy-dst.txt")
     ) ;
  (path-write-text src "hello path-copy")
  (check-true (path-copy src dst))
  (check (path-read-text dst) => "hello path-copy")
  (path-unlink src)
  (path-unlink dst)
) ;let

(check-report)
