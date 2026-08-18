;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : pdf-last-page-test.scm
;; DESCRIPTION : 纯逻辑单元测试：PDF 阅读位置记忆（pdf-last-page-get/set/
;;               to-restore）的存取、去重、LRU 上限、序列化 roundtrip 与
;;               "pdf:restore-last-page" 开关。不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r pdf-last-page-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/texmacs/tm-files.scm")

;; 测试直接读写真实 preference key，先快照原值，结束后恢复，
;; 避免污染测试环境的用户偏好。

(define saved-last-pages (get-preference "pdf:last-pages"))

(define saved-restore-switch (get-preference "pdf:restore-last-page"))

(define (restore-preferences!)
  (set-preference "pdf:last-pages" saved-last-pages)
  (set-preference "pdf:restore-last-page" saved-restore-switch)
) ;define

;; 基本 roundtrip：写入后读回；未记录的路径返回 #f。

(define (test-set-get-roundtrip)
  (set-preference "pdf:last-pages" "default")
  (pdf-last-page-set "/tmp/a.pdf" 3)
  (check (pdf-last-page-get "/tmp/a.pdf") => 3)
  (check (pdf-last-page-get "/tmp/never-seen.pdf") => #f)
) ;define

;; 路径含分号/引号/反斜杠等特殊字符、或超过 s7 print-length 的长路径，
;; 序列化均不丢数据。

(define (test-special-chars-roundtrip)
  (set-preference "pdf:last-pages" "default")
  (let ((weird "/tmp/a;b\"c\\d e.pdf")
        (long (string-append "/" (make-string 100 #\x) ".pdf"))
       ) ;
    (pdf-last-page-set weird 42)
    (pdf-last-page-set long 8)
    (pdf-last-page-set "/tmp/other.pdf" 7)
    (check (pdf-last-page-get weird) => 42)
    (check (pdf-last-page-get long) => 8)
    (check (pdf-last-page-get "/tmp/other.pdf") => 7)
  ) ;let
) ;define

;; 同一路径重复记录：去重且新值置于表头（MRU）。

(define (test-overwrite-moves-to-front)
  (set-preference "pdf:last-pages" "default")
  (pdf-last-page-set "/tmp/a.pdf" 1)
  (pdf-last-page-set "/tmp/b.pdf" 2)
  (pdf-last-page-set "/tmp/a.pdf" 9)
  (check (pdf-last-page-get "/tmp/a.pdf") => 9)
  (check (car (car (pdf-last-pages-read))) => "/tmp/a.pdf")
  (check (length (pdf-last-pages-read)) => 2)
) ;define

;; LRU 上限：写入超过 100 条后截断表尾（最旧条目被淘汰）。

(define (test-lru-cap)
  (set-preference "pdf:last-pages" "default")
  (let loop
    ((i 1))
    (when (<= i 105)
      (pdf-last-page-set (string-append "/tmp/f" (number->string i) ".pdf") i)
      (loop (+ i 1))
    ) ;when
  ) ;let
  (let ((lst (pdf-last-pages-read)))
    (check (length lst) => 100)
    ;; 最新写入的在表头，最旧的 5 条被淘汰
    (check (pdf-last-page-get "/tmp/f105.pdf") => 105)
    (check (pdf-last-page-get "/tmp/f1.pdf") => #f)
    (check (pdf-last-page-get "/tmp/f6.pdf") => 6)
  ) ;let
) ;define

;; 非法输入不落盘：page<=0、非整数、非字符串路径。

(define (test-invalid-input-ignored)
  (set-preference "pdf:last-pages" "default")
  (pdf-last-page-set "/tmp/a.pdf" 0)
  (pdf-last-page-set "/tmp/a.pdf" -1)
  (pdf-last-page-set "/tmp/a.pdf" "3")
  (pdf-last-page-set 123 3)
  (check (pdf-last-pages-read) => '())
) ;define

;; 损坏行（无法反序列化/形状非法）被跳过，不拖垮其余记录。

(define (test-corrupt-preference-tolerated)
  (set-preference "pdf:last-pages"
    "(\"good.pdf\" . 4)\n((broken\n(\"also.pdf\" . 2)"
  ) ;set-preference
  (check (pdf-last-pages-read) => '(("good.pdf" . 4) ("also.pdf" . 2)))
  ;; 哨兵 "default"（未设置）同样按无记录处理
  (set-preference "pdf:last-pages" "default")
  (check (pdf-last-pages-read) => '())
) ;define

;; 开关：缺省（哨兵 "default"）恢复，显式 "off" 时不恢复。

(define (test-restore-switch)
  (set-preference "pdf:last-pages" "default")
  (pdf-last-page-set "/tmp/a.pdf" 5)
  (set-preference "pdf:restore-last-page" "default")
  (check (pdf-last-page-to-restore "/tmp/a.pdf") => 5)
  (set-preference "pdf:restore-last-page" "on")
  (check (pdf-last-page-to-restore "/tmp/a.pdf") => 5)
  (set-preference "pdf:restore-last-page" "off")
  (check (pdf-last-page-to-restore "/tmp/a.pdf") => #f)
) ;define

(tm-define (regtest-pdf-last-page)
  (test-set-get-roundtrip)
  (test-special-chars-roundtrip)
  (test-overwrite-moves-to-front)
  (test-lru-cap)
  (test-invalid-input-ignored)
  (test-corrupt-preference-tolerated)
  (test-restore-switch)
  (restore-preferences!)
  (check-report)
) ;tm-define
