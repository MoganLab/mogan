;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1184.scm
;; DESCRIPTION : chat-tab-add-default-style-packages! 优化后的等价性与耗时验证
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report)

;;; 旧实现（逐包 add-style-package），作为等价性基准
(define (ref-add-default-style-packages! session-name)
  (add-style-package "number-europe")
  (add-style-package "preview-ref")
  (with lan
    (get-preference "language")
    (when (!= lan "english")
      (set-document-language lan)
      (when (== lan "chinese")
        (add-style-package "chinese")
        (add-style-package "table-captions-above")
      ) ;when
    ) ;when
  ) ;with
  (when (url-exists? (url-unix "$TEXMACS_STYLE_PATH" (string-append session-name ".ts"))
        ) ;url-exists?
    (add-style-package session-name)
  ) ;when
) ;define

(define (with-bench-buffer tag thunk)
  ;; buffer-new 无参返回新 buffer，rename 到 tmfs 测试 url；
  ;; 测试加载环境无 with-buffer 宏，用 buffer-focus 显式切换
  (let ((buf (buffer-new))
        (u   (string->url (string-append "tmfs://bench-1184-" tag))
        )) ;
    (buffer-rename buf u)
    (let ((old (current-buffer)) (res #f))
      (buffer-focus u #f)
      (set! res (thunk))
      (buffer-focus old #f)
      (buffer-close u)
      res
    ) ;let
  ) ;let
) ;define

(define (bench-style-impl f n)
  (let ((start (texmacs-time)))
    (do ((i 0 (+ i 1))) ((= i n))
      (with-bench-buffer (number->string i) (lambda () (f "llm")))
    ) ;do
    (- (texmacs-time) start)
  ) ;let
) ;define

(tm-define (test_1184)
  (use-modules (llm chat-loader))

  ;; 等价性：同一初始条件下，新旧实现最终 style list 必须一致
  (let ((old-list '()) (new-list '()))
    (with-bench-buffer "equiv-old"
      (lambda ()
        (ref-add-default-style-packages! "llm")
        (set! old-list (get-style-list))
      ) ;lambda
    ) ;with-bench-buffer
    (with-bench-buffer "equiv-new"
      (lambda ()
        (chat-tab-add-default-style-packages! "llm")
        (set! new-list (get-style-list))
      ) ;lambda
    ) ;with-bench-buffer
    (check new-list => old-list)
  ) ;let

  ;; 耗时对比（输出到日志，肉眼确认优化幅度）
  (let ((t-old (bench-style-impl ref-add-default-style-packages! 10))
        (t-new (bench-style-impl chat-tab-add-default-style-packages! 10))
       ) ;
    (display* "[1184] ref(old) x10: " t-old " ms\n")
    (display* "[1184] new      x10: " t-new " ms\n")
  ) ;let

  (check-report)
  (quit-TeXmacs)
) ;tm-define
