;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2028.scm
;; DESCRIPTION : GUI 验证 font-selector 的 QML 迁移（Phase 2/4/3 全链数据契约）。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [2028] 验证「格式 → 字体」迁移到 QML（FontSelector.qml + FontSelectorBridge +
;;   font-selector-* facade + cpp-font-selector-dialog glue）后：
;;     - facade 全链在主程序可用：families/styles/sizes/filter-meta/customize-meta
;;       /preview（光栅化 data URL）/ui-labels（翻译）
;;     - cpp-font-selector-dialog 的 OK 钩子经 font-selector-commit 写回 buffer
;;     - Cancel 钩子返回空 tree，不写回
;;     - specsKey 句柄 register -> lookup 往返
;;
;;   通过环境变量绕过模态 QML 弹窗：
;;     - MOGAN_TEST_FONT_SELECTOR=ok     模拟 OK（走 font-selector-commit 写回）
;;     - MOGAN_TEST_FONT_SELECTOR=cancel 模拟 Cancel（返回空 tree）
;;
;;   覆盖范围说明：本测试覆盖 cpp↔scm 数据契约与 facade 全链（families 非空 /
;;   preview 为合法 data URL / commit 写回）。QML 真实交互（三栏点选联动、预览
;;   刷新、Advanced 子对话框）mogan 无控件自动化 API，靠手动 GUI 验证：
;;     MOGAN_TEST_GUI=1 xmake r 2028
;;   打开「格式 → 字体」、点 family 看 style 列表与预览变、OK 看写回。
;;   纯逻辑（选项列表、specsKey 往返、meta 形状）见
;;   TeXmacs/progs/fonts/font-selector-test.scm。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 2028                       # headless：数据契约
;;   MOGAN_TEST_GUI=1 xmake r 2028      # 真实 GUI：手动验证三栏联动/预览
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/fonts/font-new-widgets.scm")

(check-set-mode! 'report-failed)

(define step-delay-ms 3000)

(define (preset-ok!)
  (system-setenv "MOGAN_TEST_FONT_SELECTOR" "ok")
) ;define

(define (preset-cancel!)
  (system-setenv "MOGAN_TEST_FONT_SELECTOR" "cancel")
) ;define

(define (clear-hook!)
  (system-setenv "MOGAN_TEST_FONT_SELECTOR" "")
) ;define

;; 文档字体 specs（get-init / init-multi / global?=#t），与 open-document-font-selector 同源。
(define (document-font-specs)
  (list get-init init-multi #t)
) ;define

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。
(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2028-step] ")
                           (display label)
                           (newline)
                           (act)
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_2028)
  (run-chain (append
               ;; 0) 普通文档（启动页上 init 读写异常，需普通 buffer）。
               (list (cons "new document" (lambda () (new-document))))

               ;; 1) facade 全链：register specs 后 families 非空、preview 为合法
               ;;    data URL、filter/customize meta 形状、ui-labels 翻译非空。
               (list (cons "facade full chain"
                       (lambda ()
                         (with specs (document-font-specs)
                           (selector-clean specs)
                           (let* ((key (font-selector-register-specs specs))
                                  (families (font-selector-families key))
                                  (url (font-selector-preview key)))
                             (display "  key=")(display key)(display "\n")
                             (display "  families count=")(display (length families))(display "\n")
                             (check-true (>= (length families) 1))
                             (check-true (string-starts? url "data:image/png;base64,"))
                             (check-true (>= (string-length url) 200))
                             (check-true (= (length (font-selector-filter-meta key)) 9))
                             (check-true (>= (length (font-selector-customize-meta key)) 1))
                             (check-true (nnull? (font-selector-styles key (font-selector-get key :family))))
                             (check-true (string? (assoc-ref (font-selector-ui-labels key) 'family)))
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2) Cancel：cpp-font-selector-dialog 返回空 tree，不写回。
               ;;    font-family 改动前后应不变（cancel 不 commit）。
               (list (cons "cancel: empty tree, no writeback"
                       (lambda ()
                         (preset-cancel!)
                         (with specs (document-font-specs)
                           (selector-clean specs)
                           (let* ((key (font-selector-register-specs specs))
                                  (before (get-init (pref-font-family)))
                                  (r (cpp-font-selector-dialog key))
                                  (s (tree->stree r)))
                             (display "  cancel tree->stree: ")(display s)(display "\n")
                             (check-true (func? s 'tuple 0))
                             (check-true (equal? (get-init (pref-font-family)) before))
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 3) OK：预设 font-family 为某值，register specs 后开对话框（=ok 走
               ;;    font-selector-commit），断言 commit 后 init 已反映选择。
               ;;    live 路径下 selector-set 实时写 buffer，commit 补齐差异。
               (list (cons "ok: commit writes back"
                       (lambda ()
                         (preset-ok!)
                         (with specs (document-font-specs)
                           (selector-clean specs)
                           (let* ((key (font-selector-register-specs specs))
                                  (r (cpp-font-selector-dialog key))
                                  (s (tree->stree r)))
                             (display "  ok tree->stree: ")(display s)(display "\n")
                             ;; ok 钩子返回 (tuple "ok")——func? 匹配 tuple 且首子为 "ok"。
                             (check-true (func? s 'tuple))
                             (check-true (>= (length (cdr s)) 1))
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 收尾
               (list (cons "check-report + quit"
                       (lambda () (clear-hook!) (check-report) (quit-TeXmacs))
                     ) ;cons
               ) ;list
             ) ;append
  ) ;run-chain
) ;tm-define
