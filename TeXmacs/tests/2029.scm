;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2029.scm
;; DESCRIPTION : GUI 验证 paragraph-format 的 QML 迁移全链数据契约。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [2029] 验证「格式 → 段落」迁移到 QML（ParagraphFormat.qml + ParagraphFormatBridge
;;   + paragraph-format-* facade + cpp-paragraph-format-dialog glue）后：
;;     - facade 全链在主程序可用：basic/advanced meta 形状（label/options/var/value
;;       /editable）、ui-labels（含 sepPresets 行间距预设表）
;;     - cpp-paragraph-format-dialog 的 OK 钩子经 paragraph-format-commit 注销 specs
;;     - Cancel 钩子返回空 tree
;;     - specsKey 句柄 register -> lookup 往返，cleanup 后清除（无句柄泄漏）
;;     - live 写回：paragraph-format-set 经 make-multi-line-with 改 buffer，返回传入值
;;     - 快照撤销：revert 把改动恢复到打开时快照；多参数改动一次写回（cancel 真回滚）
;;
;;   通过环境变量绕过模态 QML 弹窗：
;;     - MOGAN_TEST_PARAGRAPH_FORMAT=ok     模拟 OK（走 commit）
;;     - MOGAN_TEST_PARAGRAPH_FORMAT=cancel 模拟 Cancel（返回空 tree）
;;
;;   QML 真实交互（点选/双击编辑/预设按钮/Cancel 回滚）靠手动 GUI 验证：
;;     MOGAN_TEST_GUI=1 xmake r 2029
;;
;; USAGE
;;   xmake b stem
;;   xmake r 2029                       # headless：数据契约
;;   MOGAN_TEST_GUI=1 xmake r 2029      # 真实 GUI：手动验证
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/generic/paragraph-format-widgets.scm")

(check-set-mode! 'report-failed)

(define step-delay-ms 3000)

(define (preset-ok!)
  (system-setenv "MOGAN_TEST_PARAGRAPH_FORMAT" "ok")
) ;define

(define (preset-cancel!)
  (system-setenv "MOGAN_TEST_PARAGRAPH_FORMAT" "cancel")
) ;define

(define (clear-hook!)
  (system-setenv "MOGAN_TEST_PARAGRAPH_FORMAT" "")
) ;define

;; 段落 specs（get-env / make-multi-line-with），与 open-paragraph-format-window 同源。

(define (paragraph-specs)
  (list get-env make-multi-line-with)
) ;define

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2029-step] ")
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

(tm-define (test_2029)
  (run-chain (append
               ;; 0) 普通文档（get-env 需 buffer）。
               (list (cons "new document" (lambda () (new-document))))

               ;; 1) facade 全链：register specs 后 basic/advanced meta 形状正确、
               ;;    ui-labels 含按钮文案 + sepPresetLabel + sepPresets 4 项。
               ;;    cleanup 后 specs 句柄清除（无泄漏）。
               (list (cons "facade full chain + cleanup"
                       (lambda ()
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (basic (paragraph-format-meta key "basic"))
                                  (adv (paragraph-format-meta key "advanced"))
                                  (labels (paragraph-format-ui-labels))
                                 ) ;
                             (display "  key=")
                             (display key)
                             (display "\n")
                             ;; 基础 8 项、高级 11 项。
                             (check-true (= (length basic) 8))
                             (check-true (= (length adv) 11))
                             ;; 每项含 label/options/var/value/editable 五个 key（editable
                             ;; 可为 #f，故用 assoc 查 key 是否存在，而非值的真假）。
                             (check-true (null? (list-filter basic
                                                  (lambda (item)
                                                    (not (and (assoc 'label item)
                                                           (assoc 'options item)
                                                           (assoc 'var item)
                                                           (assoc 'value item)
                                                           (assoc 'editable item)
                                                         ) ;and
                                                    ) ;not
                                                  ) ;lambda
                                                ) ;list-filter
                                         ) ;null?
                             ) ;check-true
                             ;; 高级 tab 同样字段齐全。
                             (check-true (null? (list-filter adv
                                                  (lambda (item)
                                                    (not (and (assoc 'label item)
                                                           (assoc 'options item)
                                                           (assoc 'var item)
                                                           (assoc 'value item)
                                                           (assoc 'editable item)
                                                         ) ;and
                                                    ) ;not
                                                  ) ;lambda
                                                ) ;list-filter
                                         ) ;null?
                             ) ;check-true
                             ;; editable 为 boolean（#t 或 #f），par-mode 应为 #f（不可编辑）。
                             (check-true (boolean? (assoc-ref (car basic) 'editable)))
                             (check-true (not (assoc-ref (car basic) 'editable)))
                             ;; par-left（基础第 2 项）应可编辑（editable=#t）。
                             (check-true (assoc-ref (list-ref basic 1) 'editable))
                             ;; par-sep 选项含 0.25fn（1.25x 预设对应值）。
                             (check-true (in? "0.25fn" (assoc-ref (list-ref basic 4) 'options)))
                             ;; meta 的 value 来自 get-env（打开时读一次）。
                             (check-true (equal? (assoc-ref (car basic) 'value) (get-env "par-mode")))
                             ;; ui-labels：按钮文案 + sepPresetLabel + sepPresets 4 项（1.0x→2.0x）。
                             (check-true (string? (assoc-ref labels 'basic)))
                             (check-true (string? (assoc-ref labels 'ok)))
                             (check-true (string? (assoc-ref labels 'reset)))
                             (check-true (string? (assoc-ref labels 'sepPresetLabel)))
                             (check-true (= (length (assoc-ref labels 'sepPresets)) 4))
                             ;; specsKey 往返。
                             (check-true (== (paragraph-format-lookup-specs key) specs))
                             ;; cleanup 后句柄清除（无泄漏）。
                             (paragraph-format-cleanup key)
                             (check-true (not (paragraph-format-lookup-specs key)))
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2) live 写回：paragraph-format-set 改 par-mode，返回传入值 + get-env 读到新值。
               (list (cons "set: live write-back returns val"
                       (lambda ()
                         (clear-hook!)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (after (paragraph-format-set key "par-mode" "center"))
                                 ) ;
                             (display "  par-mode after set: ")
                             (display after)
                             (display "\n")
                             ;; set 直接返回传入值（不重读 get-env，避免滞后）。
                             (check-true (equal? after "center"))
                             (check-true (equal? (get-env "par-mode") "center"))
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2b) 快照撤销：改一个参数后 revert，get-env 回到打开时快照值。
               (list (cons "revert: single param rollback"
                       (lambda ()
                         (clear-hook!)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs)) (snap-mode (get-env "par-mode")))
                             (paragraph-format-set key "par-mode" "right")
                             (check-true (equal? (get-env "par-mode") "right"))
                             (paragraph-format-revert key)
                             (check-true (equal? (get-env "par-mode") snap-mode))
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2c) 多参数撤销：改两个参数后 revert，两个都恢复（一次 make-multi-line-with
               ;;     写所有差异，避免多次嵌套 with 吞选区）。
               (list (cons "revert: multi param rollback"
                       (lambda ()
                         (clear-hook!)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (snap-mode (get-env "par-mode"))
                                  (snap-sep (get-env "par-sep"))
                                 ) ;
                             (paragraph-format-set key "par-mode" "justify")
                             (paragraph-format-set key "par-sep" "1fn")
                             (check-true (equal? (get-env "par-mode") "justify"))
                             (check-true (equal? (get-env "par-sep") "1fn"))
                             (paragraph-format-revert key)
                             (check-true (equal? (get-env "par-mode") snap-mode))
                             (check-true (equal? (get-env "par-sep") snap-sep))
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2d) cancel 真回滚：改参数后调 cancel（revert + cleanup），get-env 恢复。
               (list (cons "cancel: document rollback"
                       (lambda ()
                         (clear-hook!)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs)) (snap-mode (get-env "par-mode")))
                             (paragraph-format-set key "par-mode" "right")
                             (paragraph-format-cancel key)
                             (check-true (equal? (get-env "par-mode") snap-mode))
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 3) Cancel：cpp-paragraph-format-dialog 返回空 tree。
               (list (cons "cancel: empty tree"
                       (lambda ()
                         (preset-cancel!)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (r (cpp-paragraph-format-dialog key))
                                  (s (tree->stree r))
                                 ) ;
                             (display "  cancel tree->stree: ")
                             (display s)
                             (display "\n")
                             (check-true (func? s 'tuple 0))
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 4) OK：钩子走 paragraph-format-commit，返回 (tuple "ok")。
               (list (cons "ok: commit marker"
                       (lambda ()
                         (preset-ok!)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (r (cpp-paragraph-format-dialog key))
                                  (s (tree->stree r))
                                 ) ;
                             (display "  ok tree->stree: ")
                             (display s)
                             (display "\n")
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
