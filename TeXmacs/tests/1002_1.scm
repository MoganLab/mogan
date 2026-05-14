;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1002_1.scm
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
  (startup-tab-get-recent-docs))

(define (restore-recent-docs docs)
  (startup-tab-clear-all-recent)
  (for-each startup-tab-add-recent-doc docs))

;; 比较路径时忽略平台差异（url->system 可能在 Windows 上转换斜杠）
(define (path-has-filename? path name)
  (let ((len-path (string-length path))
        (len-name (string-length name)))
    (and (>= len-path len-name)
         (let ((start (- len-path len-name)))
           (and (or (== start 0)
                    (let ((ch (string-ref path (- start 1))))
                      (or (== ch #\/) (== ch #\\))))
                (== (substring path start len-path) name))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-get-recent-docs-returns-list)
  (let ((docs (startup-tab-get-recent-docs)))
    (check (list? docs) => #t)))

(define (test-add-recent-doc)
  (let ((original (save-recent-docs)))
    (startup-tab-clear-all-recent)
    (check (length (startup-tab-get-recent-docs)) => 0)

    ;; 使用简单文件名避免 url->system 的平台差异
    (startup-tab-add-recent-doc "test-doc-1.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      (check (length docs) => 1)
      (check (path-has-filename? (car docs) "test-doc-1.tmu") => #t))

    ;; 添加第二个文档
    (startup-tab-add-recent-doc "test-doc-2.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      (check (length docs) => 2)
      ;; 最近添加的应在最前面
      (check (path-has-filename? (car docs) "test-doc-2.tmu") => #t))

    ;; 重新添加已有文档应将其移到最前面
    (startup-tab-add-recent-doc "test-doc-1.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      (check (length docs) => 2)
      (check (path-has-filename? (car docs) "test-doc-1.tmu") => #t))

    (restore-recent-docs original)))

(define (test-clear-recent-doc)
  (let ((original (save-recent-docs)))
    (startup-tab-clear-all-recent)
    (startup-tab-add-recent-doc "test-doc-a.tmu")
    (startup-tab-add-recent-doc "test-doc-b.tmu")
    (startup-tab-add-recent-doc "test-doc-c.tmu")

    (startup-tab-clear-recent-doc "test-doc-b.tmu")
    (let ((docs (startup-tab-get-recent-docs)))
      (check (length docs) => 2)
      (check (path-has-filename? (car docs) "test-doc-c.tmu") => #t)
      (check (path-has-filename? (cadr docs) "test-doc-a.tmu") => #t))

    ;; 清除不存在的文档不应崩溃
    (startup-tab-clear-recent-doc "non-existent.tmu")
    (check (length (startup-tab-get-recent-docs)) => 2)

    (restore-recent-docs original)))

(define (test-clear-all-recent)
  (let ((original (save-recent-docs)))
    (startup-tab-add-recent-doc "test-doc-x.tmu")
    (startup-tab-clear-all-recent)
    (check (length (startup-tab-get-recent-docs)) => 0)
    (restore-recent-docs original)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test_1002_1)
  (test-get-recent-docs-returns-list)
  (test-add-recent-doc)
  (test-clear-recent-doc)
  (test-clear-all-recent)
  (check-report))
