;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2036.scm
;; DESCRIPTION : 回归测试：ParagraphFormat 重置即时性 + dialog-value-table 缓存正确性。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [2036] 原缺陷：「格式→段落」「文档→段落」对话框点重置，第一次不生效、要点两次。
;;   根因：reset 路径破例跨 eval 重读 get-env/get-init，读取滞后于树写入。修复给 facade
;;   配了本地真相表（utils/library/dialog-value-table），reset/set/cancel 同步更新表，
;;   meta 读表即时命中、不碰树。
;;
;;   本测试钉死修复契约（任一条回退都会红）：
;;     1. set 后 meta 即时反映新值（不走树读、无滞后）。
;;     2. reset 后 meta **一次**即返回目标值——段落级=打开时快照值、文档级=全局默认。
;;        （这是原 bug 的核心回归点：改前 meta 此刻返回 set 后的旧值。）
;;     3. cancel 关窗语义：restore-snapshot + cleanup 不抛、cleanup 注销 specs。
;;        （cancel 后 meta 不再有意义——cleanup 清缓存，关窗后 QML 不读，故不断言 meta 值。）
;;     4. dialog-value-table 的读写语义：set! 命中、缺项/remove!/clean 走 fallback。
;;        （底层 s7 把「存 #f」当删除，不可能缓存 falsy 值，故不测该态；调用方契约本
;;        就只缓存非空 string，此前提在此钉死。）
;;
;;   与 2029 互补：2029 钉 facade 全链数据契约（meta 形状 / live 写树 / 快照回滚 / 句柄
;;   复用 / cpp 钩子）；2036 专钉「缓存即时性 + falsy」这条 2036 引入的新机制。
;;
;;   断言用 meta 的 value（读本地真相表），文档级另用 get-init 交叉验证树真相。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2036      # 真实 GUI，跑断言链（12 条 check）
;;
;; 注意：断言在异步链尾部，必须 MOGAN_TEST_GUI=1 才执行——headless 模式（xmake r 2036）
;; 启动即 (quit-TeXmacs)，异步链来不及调度，断言不跑、形同零覆盖（仅冒烟进程不崩）。
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
;; 重新 load 两个被测模块：texmacs-module 记录模块、load 重跑 body，顺带把模块内状态
;; （specs 注册表 / value-table）重置成干净态，保证本测试与其它测试互不污染。
(load "./TeXmacs/progs/generic/paragraph-format-widgets.scm")
(load "./TeXmacs/progs/utils/library/dialog-value-table.scm")

(check-set-mode! 'report-failed)

;; meta 里取某 var 的 value；找不到 var 时报错（比静默返回 #f 更易定位拼写错）。

(define (meta-value meta var)
  (let ((item (list-find meta (lambda (i) (== (assoc-ref i 'var) var)))))
    (when (not item)
      (error "meta-value: var not found" var)
    ) ;when
    (assoc-ref item 'value)
  ) ;let
) ;define

;; 异步步长：每步动作是同步 scheme 调用（register/set/revert/cancel），不涉排版或
;; 动画等待，500ms 足够 Qt 事件循环调度；过大会拖慢整条链。

(define step-delay-ms 500)

;; 与 open-paragraph-format-window / open-document-paragraph-format-window 同源 specs。

(define (paragraph-specs)
  (list 'paragraph get-env make-multi-line-with)
) ;define

(define (document-specs)
  (list 'document get-init init-multi)
) ;define

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2036-step] ")
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

(tm-define (test_2036)
  (run-chain (append (list (cons "new document" (lambda () (new-document))))

               ;; 1) set 后 meta 即时反映新值（命中本地真相表，不读树）。
               ;;    段落级 set "par-mode" "center"，meta 立刻读到 center。
               (list (cons "set: meta reflects new value immediately (paragraph)"
                       (lambda ()
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs)))
                             (paragraph-format-set key "par-mode" "center")
                             (check-true (equal? (meta-value (paragraph-format-meta key "basic") "par-mode") "center")
                             ) ;check-true
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2) 段落级 reset 一次即生效（核心回归点）：
               ;;    set 改 par-mode，reset 后 meta **立即**返回打开时快照值（= set 前 get-env）。
               ;;    改前：此刻 meta 返回 set 后的旧值（缓存滞后），要点第二次 reset。
               (list (cons "reset: meta returns snapshot immediately (paragraph)"
                       (lambda ()
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (snap-before (get-env "par-mode"))
                                 ) ;
                             (paragraph-format-set key "par-mode" "center")
                             (check-true (equal? (meta-value (paragraph-format-meta key "basic") "par-mode") "center")
                             ) ;check-true
                             (paragraph-format-revert key)
                             ;; reset 后 meta 即时拿到快照值——不靠二次 reset。
                             (check-true (equal? (meta-value (paragraph-format-meta key "basic") "par-mode") snap-before)
                             ) ;check-true
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 3) 文档级 reset 一次即生效：set 改 par-mode，reset 后 meta **立即**返回
               ;;    全局默认（init-default 后 get-init），且 init-has? 为 #f（init 被移除）。
               (list (cons "reset: meta returns default immediately (document)"
                       (lambda ()
                         (with specs
                           (document-specs)
                           ;; 打开时确保无显式 init（继承全局默认）。
                           (init-default "par-mode")
                           (let* ((key (paragraph-format-register-specs specs))
                                  (default-val (get-init "par-mode"))
                                 ) ;
                             (paragraph-format-set key "par-mode" "center")
                             (check-true (equal? (get-init "par-mode") "center"))
                             (paragraph-format-revert key)
                             ;; reset 后 init 被移除、值回全局默认。
                             (check-true (not (init-has? "par-mode")))
                             (check-true (equal? (get-init "par-mode") default-val))
                             ;; meta 即时返回默认值——不靠二次 reset（核心回归点）。
                             (check-true (equal? (meta-value (paragraph-format-meta key "basic") "par-mode") default-val)
                             ) ;check-true
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 4) cancel 关窗语义：cancel = restore-snapshot（写树 + 写缓存）+ cleanup
               ;;    （清缓存/注销）。cleanup 后 meta 不再有意义（关窗后 QML 不再读），故不
               ;;    断言 cancel 后 meta 值——只验 cancel 不抛（cleanup 正确清表、注销 specs）。
               (list (cons "cancel: cleanup runs without error"
                       (lambda ()
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs)))
                             (paragraph-format-set key "par-mode" "center")
                             (paragraph-format-cancel key)
                             ;; cancel 后 specs 已注销（cleanup 清注册表）。
                             (check-true (not (paragraph-format-lookup-specs key)))
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 5) dialog-value-table 读写语义（用独立 entry-key，不污染段落/字体表）：
               ;;    set! truthy 值 → ref 返回它（不走 fallback）；缺项 → ref 走 fallback；
               ;;    remove! → 回到 fallback；clean → 回到 fallback。
               ;;    注：底层 s7 hash-table 把「存 #f」等同删除，故不测「缓存 #f 取回 #f」
               ;;    （不可能态）；调用方契约本就只缓存非空 string，此前提在此钉死。
               (list (cons "value-table: ref/set/remove/clean semantics"
                       (lambda ()
                         (let* ((k1 (list 'test2036 'one)) (k2 (list 'test2036 'two)))
                           ;; set! 后命中。
                           (value-table-set! k1 "v1")
                           (check-true (equal? (value-table-ref k1 (lambda () 'fallback)) "v1"))
                           ;; 缺项走 fallback。
                           (check-true (equal? (value-table-ref k2 (lambda () 'fallback)) 'fallback))
                           ;; remove! 后回到 fallback。
                           (value-table-remove! k1)
                           (check-true (equal? (value-table-ref k1 (lambda () 'fallback)) 'fallback))
                           ;; clean 整组后回到 fallback。
                           (value-table-set! k2 "v2")
                           (value-table-clean (list k1 k2))
                           (check-true (equal? (value-table-ref k2 (lambda () 'fallback)) 'fallback))
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 收尾
               (list (cons "check-report + quit" (lambda () (check-report) (quit-TeXmacs))))
             ) ;append
  ) ;run-chain
) ;tm-define
