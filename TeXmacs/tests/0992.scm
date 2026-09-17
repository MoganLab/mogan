;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0992.scm
;; DESCRIPTION : 回归测试：AI 操作栏（AiActionsBar）只在文档正文出现。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [0992] 审查 should-show-translate-popup?（操作栏显隐总闸门）的场景门控：
;;     1. 正文选区：企业版且「ai:actions bar」开关开启时允许弹出；社区版或
;;        开关关闭时一律不弹（社区版无 AI Chat 接收方，见 0987）。
;;     2. preamble 编辑区（show-preamble 展开后）选中文字：一律不弹。
;;     3. src 源码模式 buffer（.ts 样式文件整篇以 src 模式打开）选中文字：
;;        一律不弹。
;;     4. 选区只有图片：一律不弹（翻译/润色/对话都以文字为对象）；树形状
;;        判定谓词 ai-selection-only-images? 的纯逻辑覆盖见
;;        TeXmacs/progs/generic/tests/ai-actions-bar-test.scm。
;;   AI 聊天场景（tmfs://chat/... 输入框与消息视图）与页眉页脚编辑
;;   （tmfs://aux/...）由 buffer 名 tmfs:// 前缀门控覆盖，本测试不重复布景。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 0992                     # headless：冒烟（断言在异步链里不执行）
;;   MOGAN_TEST_GUI=1 xmake r 0992    # 真实 GUI：跑断言链
;;
;; 注意：断言依赖真实视图/当前 buffer 切换（headless 下当前 buffer 恒为
;; tmfs://startup-tab，无法布景非 tmfs 场景），故断言串在 exec-delayed-at
;; 链里，必须 MOGAN_TEST_GUI=1 才真正执行。正文用例期望值随构建 flavor 与
;; 首选项计算；两个非正文用例在任何环境下都必须为 #f。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(check-set-mode! 'report-failed)

(define step-delay-ms 300)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[0992-step] ")
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

;; 光标移入 tree 的指定叶子并选中其全部内容
(define (select-leaf-content t . indices)
  (apply tree-go-to (append (list t) indices (list :start)))
  (selection-set-start)
  (apply tree-go-to (append (list t) indices (list :end)))
  (selection-set-end)
) ;define

(tm-define (test_0992)
  (run-chain (list
    (cons "new document with preamble"
      (lambda ()
        (new-document)
        (insert "BODYTEXT")
        ;; 插入 preamble 并展开为可编辑的 show-preamble（布景同
        ;; macro-widgets.scm）
        (let ((body (buffer-get-body (current-buffer))))
          (tree-insert! body 0 '((hide-preamble (document "PREAMBLETEXT"))))
          (tree-assign-node (tree-ref body 0) 'show-preamble)
        ) ;let
      ) ;lambda
    ) ;cons
    ;; 1) 正文选区：期望值随 flavor 与首选项（pref key 与 C++ 侧字面量一致，
    ;; pref-keys.scm 的 accessor 在测试默认模块作用域不可见）。
    ;; body 树：(document (show-preamble (document "PREAMBLETEXT")) "BODYTEXT")
    (cons "body selection"
      (lambda ()
        (let ((body-expected
                (and (not (community-stem?))
                     (== (get-preference "ai:actions bar") "on"))))
          (select-leaf-content (buffer-get-body (current-buffer)) 1)
          (check (selection-active-any?) => #t)
          (check (inside? 'show-preamble) => #f)
          (check (should-show-translate-popup?) => body-expected)
        ) ;let
      ) ;lambda
    ) ;cons
    ;; 2) preamble 编辑区：一律不弹
    (cons "preamble selection"
      (lambda ()
        (select-leaf-content (tree-ref (buffer-get-body (current-buffer)) 0 0) 0)
        (check (selection-active-any?) => #t)
        (check (inside? 'show-preamble) => #t)
        (check (should-show-translate-popup?) => #f)
      ) ;lambda
    ) ;cons
    ;; 3) src 源码模式 buffer（样式文件整篇以 src 模式打开）：一律不弹
    (cons "src-mode buffer selection"
      (lambda ()
        (load-buffer "$TEXMACS_PATH/styles/article.ts")
      ) ;lambda
    ) ;cons
    (cons "src-mode buffer assert"
      (lambda ()
        (select-all)
        (check (selection-active-any?) => #t)
        (check (get-init "mode") => "src")
        (check (should-show-translate-popup?) => #f)
      ) ;lambda
    ) ;cons
    ;; 4) 选区只有图片：一律不弹（翻译/润色/对话都以文字为对象）；
    ;; 整篇只含一张图的文档 + select-all 是最稳的图片选区布景
    (cons "image-only document"
      (lambda ()
        (new-document)
        (insert '(image "$TEXMACS_PATH/misc/images/new-mogan-128.png"
                  "77pt" "77pt" "" ""))
      ) ;lambda
    ) ;cons
    (cons "image-only selection assert"
      (lambda ()
        (select-all)
        (check (selection-active-any?) => #t)
        (check (should-show-translate-popup?) => #f)
      ) ;lambda
    ) ;cons
    (cons "report + quit"
      (lambda () (check-report) (quit-TeXmacs))
    ) ;cons
  )) ;run-chain, list
) ;tm-define
