;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2044.scm
;; DESCRIPTION : 回归测试：Preferences.qml scheme facade 数据契约。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [2044] 把首选项对话框重写为 QML 弹窗（Preferences.qml），scheme facade
;;   preferences-qml-meta 返回 tab 树供 C++ bridge 消费。本测试钉死 facade 的
;;   数据契约（任一条回退都会红）：
;;     1. meta 返回 5 主 tab（general / keyboard / mathematics / convert / other）。
;;     2. 各 tab 字段数符合设计稿（general 9 / keyboard 13 / mathematics 10 /
;;        convert 0 / other 15）。
;;     3. Convert tab 无直接 fields、字段在 6 个子 tab 内（html/latex/bibtex/
;;        verbatim/pdf/image）——这是 bridge parse_meta_tree 区分 fields vs
;;        sub-tabs 的契约。
;;     4. 字段描述符结构：combo 有 kind/key/label/options/optionsTr/editable/
;;        value + 可选 restart?；toggle 有 kind/key/label/value + 可选 group/
;;        column/hint 等。assoc-list of dotted pairs 格式（bridge assoc_to_
;;        variantmap 遍历消费）。
;;
;;   断言走 preferences-qml-meta（facade 直调、绕开 bridge 与 QML），挡 scheme
;;   语义 bug（supported-languages 变量非函数 / string-empty? / with 宏解构 /
;;   list-tail 越界等历史回归点）。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2044      # 真实 GUI，跑断言链
;;
;; 注意：断言在异步链里，必须 MOGAN_TEST_GUI=1 才执行——headless 模式（xmake r 2044）
;; 启动即 (quit-TeXmacs)，异步链来不及调度，断言不跑（仅冒烟进程不崩）。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
;; 重新 load 被测模块：texmacs-module 记录模块、load 重跑 body、重置模块状态成
;; 干净态，保证本测试与其它测试互不污染。
(load "./TeXmacs/progs/texmacs/menus/preferences-widgets.scm")

(check-set-mode! 'report-failed)

;; meta 里取某 tab key 对应的 tab 对象：(key label fields [sub-tabs])。

(define (tab-ref meta tab-key)
  (list-find meta (lambda (t) (== (car t) tab-key)))
) ;define

;; field-descriptor 里取某 symbol key 对应的 value（assoc 查找）。
;; field-descriptor 是 assoc-list：((kind . "combo") (key . "...") ...)。

(define (field-ref field sym)
  (let ((pair (assoc sym field)))
    (and pair (cdr pair))
  ) ;let
) ;define

;; 异步步长：本测试只调 facade（无排版/动画等待），500ms 足够 Qt 事件循环调度。

(define step-delay-ms 500)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2044-step] ")
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

(tm-define (test_2044)
  (run-chain (append (list (cons "new document" (lambda () (new-document))))

               ;; 1 meta 返回 5 主 tab + 各 tab 字段数 + Convert 子 tab 数。
               (list (cons "meta: 5 tabs + field counts + convert sub-tabs"
                       (lambda ()
                         (let* ((meta (preferences-qml-meta)))
                           ;; 5 主 tab + tab key 顺序。
                           (check-true (== (length meta) 5))
                           (check-true (equal? (map car meta)
                                         (list "general" "keyboard" "mathematics" "convert" "other")
                                       ) ;equal?
                           ) ;check-true
                           ;; 各 tab 字段数（Convert 为 0、字段在子 tab 内）。
                           (check-true (== (length (caddr (tab-ref meta "general"))) 9))
                           (check-true (== (length (caddr (tab-ref meta "keyboard"))) 13))
                           (check-true (== (length (caddr (tab-ref meta "mathematics"))) 10))
                           (check-true (== (length (caddr (tab-ref meta "convert"))) 0))
                           (check-true (== (length (caddr (tab-ref meta "other"))) 15))
                           ;; Convert 6 子 tab + sub-tab key 顺序。
                           (let ((convert (tab-ref meta "convert")))
                             (check-true (== (length (cadddr convert)) 6))
                             (check-true (equal? (map car (cadddr convert))
                                           (list "html" "latex" "bibtex" "verbatim" "pdf" "image")
                                         ) ;equal?
                             ) ;check-true
                           ) ;let
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 2 字段描述符结构：general 首字段（look and feel）是 combo——有
               ;;    kind/key/label/options/optionsTr/editable/value + restart? flag。
               (list (cons "descriptor: combo structure (look and feel)"
                       (lambda ()
                         (let* ((meta (preferences-qml-meta)) (laf (car (caddr (tab-ref meta "general")))))
                           (check-true (equal? (field-ref laf 'kind) "combo"))
                           (check-true (equal? (field-ref laf 'key) "look and feel"))
                           (check-true (== (field-ref laf 'restart?) #t))
                           ;; combo 必有 options / optionsTr（list、非空）。
                           (check-true (and (list? (field-ref laf 'options)) (pair? (field-ref laf 'options)))
                           ) ;check-true
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 3 字段描述符结构：mathematics 首字段（use large brackets）是
               ;;    toggle——有 kind/key/label/value + group/column flag。
               (list (cons "descriptor: toggle structure (use large brackets)"
                       (lambda ()
                         (let* ((meta (preferences-qml-meta))
                                (brackets (car (caddr (tab-ref meta "mathematics"))))
                               ) ;
                           (check-true (equal? (field-ref brackets 'kind) "toggle"))
                           (check-true (equal? (field-ref brackets 'key) "use large brackets"))
                           ;; toggle value 为 "on"/"off" 串（wire 格式统一字符串）。
                           (check-true (or (equal? (field-ref brackets 'value) "on")
                                         (equal? (field-ref brackets 'value) "off")
                                       ) ;or
                           ) ;check-true
                           ;; column flag（双栏布局列号）。
                           (check-true (== (field-ref brackets 'column) 0))
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 收尾
               (list (cons "check-report + quit" (lambda () (check-report) (quit-TeXmacs))))
             ) ;append
  ) ;run-chain
) ;tm-define
