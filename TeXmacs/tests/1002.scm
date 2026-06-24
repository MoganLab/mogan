;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1002.scm
;; DESCRIPTION : Integration tests for startup tab recent documents API
;; COPYRIGHT   : (C) 2026  Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (save-recent-docs)
  (startup-tab-get-recent-docs)
) ;define

(define (restore-recent-docs docs)
  (startup-tab-clear-all-recent)
  (for-each startup-tab-add-recent-doc docs)
) ;define

;; 比较路径时忽略平台差异（url->system 可能在 Windows 上转换斜杠）。
;; 对于裸文件名输入，url->system 往返转换后返回不带目录前缀的文件名，
;; 因此允许文件名直接出现在路径末尾（start 为 0 时不要求分隔符）。

(define (path-has-filename? path name)
  (let ((len-path (string-length path)) (len-name (string-length name)))
    (and (>= len-path len-name)
      (let ((start (- len-path len-name)))
        (and (or (== start 0)
               (let ((ch (string-ref path (- start 1))))
                 (or (== ch #\/) (== ch #\\))
               ) ;let
             ) ;or
          (== (substring path start len-path) name)
        ) ;and
      ) ;let
    ) ;and
  ) ;let
) ;define

;; 连续快速添加时，recent-files 的 last_open 时间戳精度为秒，
;; 同秒内添加的文档顺序不确定，因此用顺序无关的集合包含断言。

(define (docs-contain? docs name)
  (list-any (lambda (p) (path-has-filename? p name)) docs)
) ;define

(define (list-any pred lst)
  (cond ((null? lst) #f)
        ((pred (car lst)) #t)
        (else (list-any pred (cdr lst)))
  ) ;cond
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-get-recent-docs-returns-list)
  (let ((docs (startup-tab-get-recent-docs)))
    (check (list? docs) => #t)
  ) ;let
) ;define

(define (test-add-recent-doc)
  (let ((original (save-recent-docs)))
    (startup-tab-clear-all-recent)
    (check (length (startup-tab-get-recent-docs)) => 0)

    ;; 使用简单文件名避免 url->system 的平台差异
    (startup-tab-add-recent-doc "test-doc-1.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      (check (length docs) => 1)
      (check (path-has-filename? (car docs) "test-doc-1.tmu") => #t)
    ) ;let

    ;; 添加第二个文档（同秒内添加，顺序不确定，故只断言内容存在）
    (startup-tab-add-recent-doc "test-doc-2.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      (check (length docs) => 2)
      (check (docs-contain? docs "test-doc-1.tmu") => #t)
      (check (docs-contain? docs "test-doc-2.tmu") => #t)
    ) ;let

    ;; 重新添加已有文档不应产生重复
    (startup-tab-add-recent-doc "test-doc-1.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      (check (length docs) => 2)
      (check (docs-contain? docs "test-doc-1.tmu") => #t)
      (check (docs-contain? docs "test-doc-2.tmu") => #t)
    ) ;let

    (restore-recent-docs original)
  ) ;let
) ;define

(define (test-clear-recent-doc)
  (let ((original (save-recent-docs)))
    (startup-tab-clear-all-recent)
    (startup-tab-add-recent-doc "test-doc-a.tmu")
    (startup-tab-add-recent-doc "test-doc-b.tmu")
    (startup-tab-add-recent-doc "test-doc-c.tmu")

    (startup-tab-clear-recent-doc "test-doc-b.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      ;; 同秒内添加，顺序不确定；只断言 b 已删除、a 与 c 仍在
      (check (length docs) => 2)
      (check (docs-contain? docs "test-doc-a.tmu") => #t)
      (check (docs-contain? docs "test-doc-c.tmu") => #t)
      (check (docs-contain? docs "test-doc-b.tmu") => #f)
    ) ;let

    ;; 清除不存在的文档不应崩溃
    (startup-tab-clear-recent-doc "non-existent.tmu")
    (check (length (startup-tab-get-recent-docs)) => 2)

    (restore-recent-docs original)
  ) ;let
) ;define

(define (test-clear-all-recent)
  (let ((original (save-recent-docs)))
    (startup-tab-add-recent-doc "test-doc-x.tmu")
    (startup-tab-clear-all-recent)
    (check (length (startup-tab-get-recent-docs)) => 0)
    (restore-recent-docs original)
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test_1002)
  (test-get-recent-docs-returns-list)
  (test-add-recent-doc)
  (test-clear-recent-doc)
  (test-clear-all-recent)
  (check-report)
) ;tm-define
