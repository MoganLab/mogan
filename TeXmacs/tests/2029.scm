;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2029.scm
;; DESCRIPTION : GUI 验证 paragraph-format 的 QML 迁移全链数据契约。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [2029] 验证「格式 → 段落」迁移到 QML（ParagraphFormat.qml + ParagraphFormatBridge
;;   + paragraph-format-* facade + cpp-paragraph-format-dialog glue）后：
;;     - basic/advanced meta 形状（label/options/var/value/editable）
;;     - ui-labels（按钮文案 + sepPresetLabel + sepPresets 4 项）
;;     - specsKey 句柄 register -> lookup 往返，cleanup 后清除（无句柄泄漏）
;;     - live 写回：set 经 make-multi-line-with 把 par-mode 写入文档树
;;     - 快照撤销：revert / cancel 把文档树回滚到 set 之前的值
;;     - cpp-paragraph-format-dialog：Cancel 钩子返回空 tree，OK 钩子走 commit
;;
;;   通过环境变量绕过模态 QML 弹窗：
;;     - MOGAN_TEST_PARAGRAPH_FORMAT=ok     模拟 OK（走 commit）
;;     - MOGAN_TEST_PARAGRAPH_FORMAT=cancel 模拟 Cancel（返回空 tree）
;;
;;   文档树断言用 buffer->tree（set 前抓的 buf）+ stree-with-ref 提取 with 值，而非
;;   get-env：make-multi-line-with 末尾 tree-select 会挪动焦点，get-env 读数不稳；
;;   buffer 树是真相源、不受光标影响。
;;
;;   QML 真实交互（点选/双击编辑/预设按钮/Esc）无法在 scheme 集成测试里触及，靠手动：
;;     MOGAN_TEST_GUI=1 xmake r 2029
;;
;; USAGE
;;   xmake b stem
;;   xmake r 2029                       # headless：数据契约（事件循环未跑，仅冒烟）
;;   MOGAN_TEST_GUI=1 xmake r 2029      # 真实 GUI：跑全部断言 + 手动验证交互
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

;; 提取 stree 中 var 当前的 with 值。with 的 stree 形如
;; (with "par-mode" "center" [...更多 var/val 对...] body)，参数是 label 之后扁平排列
;; 的 (var val ...) 对。从外向内找第一个声明 var 的 with 节点，返回其值；找不到返回 #f。
;; 用途：精确断言段落参数的文档树值（set 写入 / revert 回滚后 par-mode 实际是啥），
;; 比递归查找字符串（文档别处出现同名串会假阳性）更可靠。

(define (stree-with-ref stree var)
  (cond ((and (pair? stree) (== (car stree) 'with)) (with-ref-pairs (cdr stree) var))
        ((pair? stree) (children-with-ref stree var))
        (else #f)
  ) ;cond
) ;define

;; 子节点里逐个递归找；with 嵌套时外层优先（顺序遍历先命中外层）。

(define (children-with-ref children var)
  (if (null? children)
    #f
    (or (stree-with-ref (car children) var) (children-with-ref (cdr children) var))
  ) ;if
) ;define

;; with 子节点 (var val ... body)：在参数对里找 var 取其后继，找不到下钻 body。

(define (with-ref-pairs children var)
  (cond ((or (null? children) (null? (cdr children))) #f)
        ((== (car children) var) (cadr children))
        (else (with-ref-pairs (cddr children) var))
  ) ;cond
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

               ;; 2) live 写回：set 改文档树。set 经 make-multi-line-with 写焦点 buffer，
               ;;    末尾 tree-select 会令 GUI 焦点/current-buffer 与 set 作用的 buffer 错位，
               ;;    故 set 前抓住 buffer 路径、set 后用 buffer->tree 读该 buffer 的树
               ;;    （不依赖 current-buffer）。
               (list (cons "set: writes par-mode to buffer tree"
                       (lambda ()
                         (clear-hook!)
                         (new-document)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (buf (current-buffer))
                                  (after (paragraph-format-set key "par-mode" "center"))
                                 ) ;
                             (check-true (equal? after "center"))
                             (check-true (equal? (stree-with-ref (tree->stree (buffer->tree buf)) "par-mode") "center")
                             ) ;check-true
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2b) 快照撤销：set 后 revert，文档树回到 set 之前的值。snap-before 在
               ;;     set 之前抓（此时无 set 干扰、get-env 稳定），revert 后断言根 with 的
               ;;     par-mode 值等于 snap-before。全程用 buffer->tree buf 读该 buffer 的
               ;;     树——set 末尾 tree-select 会挪动焦点，current-buffer 读数不稳，但
               ;;     buffer 树是真相源、不受光标影响。
               (list (cons "revert: rollback to snapshot"
                       (lambda ()
                         (clear-hook!)
                         (new-document)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (buf (current-buffer))
                                  (snap-before (get-env "par-mode"))
                                 ) ;
                             (paragraph-format-set key "par-mode" "center")
                             (check-true (equal? (stree-with-ref (tree->stree (buffer->tree buf)) "par-mode") "center")
                             ) ;check-true
                             (paragraph-format-revert key)
                             (check-true (equal? (stree-with-ref (tree->stree (buffer->tree buf)) "par-mode")
                                           snap-before
                                         ) ;equal?
                             ) ;check-true
                             (paragraph-format-cancel key)
                           ) ;let*
                         ) ;with
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2c) cancel 真回滚：set 后 cancel（revert + cleanup），文档树回到 set 前。
               (list (cons "cancel: document rollback"
                       (lambda ()
                         (clear-hook!)
                         (new-document)
                         (with specs
                           (paragraph-specs)
                           (let* ((key (paragraph-format-register-specs specs))
                                  (buf (current-buffer))
                                  (snap-before (get-env "par-mode"))
                                 ) ;
                             (paragraph-format-set key "par-mode" "center")
                             (check-true (equal? (stree-with-ref (tree->stree (buffer->tree buf)) "par-mode") "center")
                             ) ;check-true
                             (paragraph-format-cancel key)
                             (check-true (equal? (stree-with-ref (tree->stree (buffer->tree buf)) "par-mode")
                                           snap-before
                                         ) ;equal?
                             ) ;check-true
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
