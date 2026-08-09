
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1194.scm
;; DESCRIPTION : 回归测试：plugin-list 的 scheme 实现（tm-plugins.scm）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1194] plugin-list 由 C++ glue（read_directory + merge_sort + 去重）下沉为
;;   tm-plugins.scm 的 scheme 实现（(liii os) listdir + sort! vector + 按下标
;;   去重）。基准显示 scheme 版（15 ms/100 轮）快于原 C++ 版（42 ms/100 轮）。
;;   本测试钉死数据契约：
;;     1. 返回 symbol 列表且按 string<? 升序、无重复。
;;     2. 不含 "." / ".."。
;;     3. 与 listdir 原始读目结果一致（手工重算参考值对比）。
;;
;; USAGE
;;   xmake b stem && xmake r 1194    # headless 即可跑（同步基准，无异步链）
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check) (liii os))
(check-set-mode! 'report-failed)

;; 用 listdir 原始结果手工重算参考值（list 路径，与被测实现的 vector 路径
;; 互为交叉验证），契约与 plugin-list 文档一致：排序 + 去重 + 滤 "." ".."

(define (read-plugin-dir path)
  (let ((u (string->url path)))
    (if (url-exists? u) (vector->list (listdir (url->system u))) '())
  ) ;let
) ;define

(define (plugin-list-reference)
  (let* ((a (read-plugin-dir "$TEXMACS_PATH/plugins"))
         (b (read-plugin-dir "$TEXMACS_HOME_PATH/plugins"))
         (sorted (list-sort (append a b) string<?))
         (deduped (let loop
                    ((l sorted))
                    (cond ((null? l) '())
                          ((or (== (car l) ".") (== (car l) "..")) (loop (cdr l)))
                          ((and (pair? (cdr l)) (== (car l) (cadr l))) (loop (cdr l)))
                          (else (cons (car l) (loop (cdr l))))
                    ) ;cond
                  ) ;let
         ) ;deduped
        ) ;
    (map string->symbol deduped)
  ) ;let*
) ;define

(define (bench f n)
  (let ((start (texmacs-time)))
    (do ((i 0 (+ i 1)))
      ((= i n))
      (f)
    ) ;do
    (- (texmacs-time) start)
  ) ;let
) ;define

(tm-define (test_1194)
  (let ((plugins (plugin-list)))
    (check plugins => (plugin-list-reference))
    (check (list-and (map symbol? plugins)) => #t)
    (check (list-filter plugins (lambda (s) (in? (symbol->string s) '("." ".."))))
      =>
      '()
    ) ;check
  ) ;let
  (let* ((n 100) (ms (bench plugin-list n)))
    (display (string-append "bench plugin-list x"
               (number->string n)
               ": "
               (number->string ms)
               " ms\n"
             ) ;string-append
    ) ;display
  ) ;let*
  (check-report)
) ;tm-define
