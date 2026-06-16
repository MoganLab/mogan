(import (liii check) (liii path) (liii os))

(check-set-mode! 'report-failed)

;; path-clone
;; 复制路径值。
;;
;; 语法
;; ----
;; (path-clone path-value)
;;
;; 参数
;; ----
;; path-value : path-value
;; 要复制的路径值。
;;
;; 返回值
;; ------
;; path-value
;; 返回一个新的路径值副本。

(when (not (os-windows?))
  (check (path->string (path-clone (path "tmp/demo.txt"))) => "tmp/demo.txt")
) ;when

(when (os-windows?)
  (check (path->string (path-clone (path "tmp/demo.txt"))) => "tmp\\demo.txt")
) ;when

(check-true (path=? (path "tmp/demo.txt") (path-clone (path "tmp/demo.txt"))))

(check-report)
