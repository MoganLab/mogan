;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2023.scm
;; DESCRIPTION : GUI 验证 page-setup 的 QML form 弹窗逻辑契约。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [2023] 验证「文件 → 页面设置」迁移到 QML form 引擎后：
;;     - OK：cpp-form-dialog 返回 ((key value) ...)，scm 写回 4 个 preference
;;     - Cancel：返回空，不写回
;;     - scm 侧 tree->stree 解构正确（mogan tree 非 scheme pair）
;;
;;   通过环境变量绕过模态 QML 弹窗：
;;     - MOGAN_TEST_FORM_DIALOG=ok    模拟点 OK（回传字段表当前值）
;;     - MOGAN_TEST_FORM_DIALOG=cancel 模拟 Cancel（返回空）
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2023
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/texmacs/menus/print-widgets.scm")

(check-set-mode! 'report-failed)

(define step-delay-ms 3000)

(define (pref-get key) (get-pretty-preference key))

(define (pref-set! key val) (set-pretty-preference key val))

(define (preset-ok!) (system-setenv "MOGAN_TEST_FORM_DIALOG" "ok"))
(define (preset-cancel!) (system-setenv "MOGAN_TEST_FORM_DIALOG" "cancel"))
(define (clear-hook!) (system-setenv "MOGAN_TEST_FORM_DIALOG" ""))

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。
(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2023-step] ")
                           (display label)
                           (newline)
                           (act)
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms)))
          t)))))

(tm-define (test_2023)
  (run-chain (append
               ;; 0) 先建普通文档（默认启动页上 preference 读写异常，需普通 buffer）
               (list (cons "new document" (lambda () (new-document))))

               ;; 0.5) 诊断：OK 返回的 tree->stree 结构（确认 cadr/caddr 解构）
               (list (cons "diag stree structure (ok)"
                       (lambda ()
                         (preset-ok!)
                         (let* ((r (cpp-form-dialog (stree->tree (page-setup-form-tree))))
                                (s (tree->stree r)))
                           (display "  raw tree->stree: ")(display s)(newline)))))

               ;; 1) Cancel：弹窗 Cancel 应不写回（值保持改动前）
               (list (cons "preset=cancel"
                       (lambda ()
                         (preset-cancel!)
                         (let ((before (pref-get "paper type")))
                           (open-page-setup)
                           (check-true (equal? (pref-get "paper type") before))))))

               ;; 2) OK：改值后弹窗，OK 应写回字段表当前值
               (list (cons "preset=ok + change + open (expect writeback)"
                       (lambda ()
                         (preset-ok!)
                         (pref-set! "paper type" "A3")
                         (open-page-setup)
                         ;; page-setup-form-tree 在 open-page-setup 内构造，
                         ;; 取当前 preference（A3），OK 写回 A3。
                         (check-true (equal? (pref-get "paper type") "A3")))))

               ;; 收尾
               (list (cons "check-report + quit"
                       (lambda ()
                         (clear-hook!)
                         (check-report)
                         (quit-TeXmacs)))))))
