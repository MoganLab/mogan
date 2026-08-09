
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1194.scm
;; DESCRIPTION : 性能对比：scheme 实现的 scheme-plugin-list vs C++ glue plugin-list
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [1194] plugin-list（C++，read_directory + merge_sort + 去重）是启动期
;;   plugins 阶段的第一步。本测试在 scheme 侧用 url-read-directory 等价实现
;;   scheme-plugin-list，钉死两者结果一致，并各跑 N 轮对比耗时，评估该逻辑
;;   下沉 scheme 的可行性。
;;
;; USAGE
;;   xmake b stem && xmake r 1194    # headless 即可跑（同步基准，无异步链）
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)

;; 与 C++ plugin_list 同构：读两个 plugins 目录 -> 合并排序 -> 去连续重复 ->
;; 滤 "." ".."。C++ 端 read_directory 返回条目名，scheme 侧 url-read-directory
;; 返回完整 url，需 url-tail 取末段再转字符串；元素最终转 symbol 与 glue 返回对齐
;; （plugin-list 的元素是 symbol，见 help-menu.scm 的 symbol->string 用法）。

(define (read-plugin-dir path)
  (map (lambda (u) (url->string (url-tail u))) (url-read-directory path "*"))
) ;define

(define (scheme-plugin-list)
  (let* ((a (read-plugin-dir "$TEXMACS_PATH/plugins"))
         (b (read-plugin-dir "$TEXMACS_HOME_PATH/plugins"))
         (sorted (list-sort (append a b) string<?))
         (deduped
           (let loop ((l sorted))
             (cond ((null? l) '())
                   ((or (== (car l) ".") (== (car l) "..")) (loop (cdr l)))
                   ((and (pair? (cdr l)) (== (car l) (cadr l))) (loop (cdr l)))
                   (else (cons (car l) (loop (cdr l)))))
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
  (check (scheme-plugin-list) => (plugin-list))
  (let* ((n 100)
         (cpp-ms (bench plugin-list n))
         (scm-ms (bench scheme-plugin-list n))
        ) ;
    (display (string-append "bench plugin-list (C++) x"
               (number->string n)
               ": "
               (number->string cpp-ms)
               " ms\n"
             ) ;string-append
    ) ;display
    (display (string-append "bench scheme-plugin-list x"
               (number->string n)
               ": "
               (number->string scm-ms)
               " ms\n"
             ) ;string-append
    ) ;display
  ) ;let
  (check-report)
) ;tm-define
