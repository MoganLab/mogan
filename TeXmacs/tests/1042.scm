;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1042.scm
;; DESCRIPTION : Unit tests for floating search on-input dedup logic
;; COPYRIGHT   : (C) 2026  Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/generic/search-widgets.scm")

(check-set-mode! 'report-failed)

(define (test-floating-search-on-input-dedup)
  (display "Testing floating-search-on-input dedup logic...\n")

  ;; 创建临时 buffer，使用原子字符串作为 body（tree->string 才能返回值）
  (let* ((buf (buffer-new)) (initial-content "hello"))

    ;; 设置初始内容
    (buffer-set-body buf initial-content)

    ;; 初始化 floating search 状态
    (set! floating-search-active? #t)
    (set! floating-search-aux buf)
    ;; 同步 last-content 为当前 buffer 内容
    (set! floating-search-last-content (tree->string (buffer-get-body buf)))

    ;; Test 1: 内容未变 → floating-search-last-content 保持不变
    (let ((before floating-search-last-content))
      (floating-search-on-input)
      (check floating-search-last-content => before)
    ) ;let

    ;; Test 2: 内容改变 → floating-search-last-content 应更新为新内容
    (buffer-set-body buf "hello world")
    (floating-search-on-input)
    (check floating-search-last-content => "hello world")

    ;; Test 3: 再次用相同内容调用 → 不变
    (let ((before floating-search-last-content))
      (floating-search-on-input)
      (check floating-search-last-content => before)
    ) ;let

    ;; Test 4: 清空内容 → last-content 应更新为空串
    (buffer-set-body buf "")
    (floating-search-on-input)
    (check floating-search-last-content => "")

    ;; Test 5: floating-search-active? 为 #f 时 → last-content 不变
    (set! floating-search-active? #f)
    (buffer-set-body buf "should be ignored")
    (let ((before floating-search-last-content))
      (floating-search-on-input)
      (check floating-search-last-content => before)
    ) ;let

    ;; 恢复
    (set! floating-search-active? #t)

    (display "floating-search-on-input dedup tests passed!\n")
  ) ;let*
) ;define

(define (test-floating-search-toggle-mode)
  (display "Testing floating-search-toggle-mode...\n")

  ;; 记录原始 mode
  (let ((orig-mode floating-search-mode))
    ;; 确保从已知状态开始
    (set! floating-search-mode "text")
    (check floating-search-mode => "text")

    ;; 切换到 math
    (set! floating-search-mode "math")
    (check floating-search-mode => "math")

    ;; 切回 text
    (set! floating-search-mode "text")
    (check floating-search-mode => "text")

    ;; 恢复
    (set! floating-search-mode orig-mode)
    (display "floating-search-toggle-mode tests passed!\n")
  ) ;let
) ;define

(tm-define (test_1042)
  (display "Running test_1042...\n")
  (test-floating-search-on-input-dedup)
  (test-floating-search-toggle-mode)
  (check-report)
) ;tm-define
