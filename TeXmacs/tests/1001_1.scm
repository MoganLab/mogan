;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1001_1.scm
;; DESCRIPTION : Tests for startup tab preservation when opening files
;; COPYRIGHT   : (C) 2026  Yuki
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 检查当前标签页列表中是否包含 tmfs://startup-tab
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (startup-tab-in-list?)
  (let ((views (tabpage-list #t))
        (target "tmfs://startup-tab"))
    (let loop ((rest views))
      (cond ((null? rest) #f)
            ((== (utf8->cork (url->system (view->buffer (car rest)))) target) #t)
            (else (loop (cdr rest)))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 初始状态应只有 1 个标签页（启动页）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-initial-state)
  (check (length (tabpage-list #t)) => 1)
  (check (startup-tab-in-list?) => #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 加载文件后启动页应保留
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-load-buffer-preserves-startup)
  (load-buffer "$TEXMACS_PATH/tests/tmu/201_5.tmu")
  (check (length (tabpage-list #t)) => 2)
  (check (startup-tab-in-list?) => #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 关闭启动页应无效（受保护）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-close-startup-tab-protected)
  (let ((startup "tmfs://startup-tab")
        (tab-count-before (length (tabpage-list #t))))
    (cpp-buffer-close startup)
    (check (length (tabpage-list #t)) => tab-count-before)
    (check (startup-tab-in-list?) => #t)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 加载多个文件后启动页仍应保留
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-multiple-buffers-startup-preserved)
  (load-buffer "$TEXMACS_PATH/tests/tmu/201_5_in_same_window.tmu")
  (check (length (tabpage-list #t)) => 3)
  (check (startup-tab-in-list?) => #t))

(tm-define (test_1001_1)
  (test-initial-state)
  (test-load-buffer-preserves-startup)
  (test-close-startup-tab-protected)
  (test-multiple-buffers-startup-preserved)
  (check-report))
