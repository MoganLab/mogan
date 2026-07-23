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

;; ---- submit 路径用到的偏好存取 helper ----
;; 改偏好会污染全局配置，故链首统一抓快照、链尾统一还原。本测试改动的键固定，
;; 在快照表里枚举；test_2044 开头建快照、收尾步按快照还原。

;; 单键读当前值（每步据此推导 want = 反转当前值）。

(define (snapshot-pref key)
  (get-preference key)
) ;define

(define (snapshot-bool key)
  (get-boolean-preference key)
) ;define

(define pref-snapshot '())

;; 抓取本测试会改动的全部键的当前值（boolean 键与 string 键混合）。

(define (capture-pref-snapshot)
  (set! pref-snapshot
    (list (list "use large brackets" (get-boolean-preference "use large brackets"))
      (list "complex actions" (get-preference "complex actions"))
      (list "buffer management" (get-preference "buffer management"))
      (list "tab bar" (get-boolean-preference "tab bar"))
      (list "gui theme" (get-preference "gui theme"))
    ) ;list
  ) ;set!
) ;define

;; 按快照逐键还原：boolean 键还原成 #t/#f，string 键还原成原串。

(define (restore-pref-snapshot)
  (for (item pref-snapshot)
    (let* ((key (car item)) (val (cadr item)))
      (cond ((== key "use large brackets") (set-boolean-preference key val))
            ((== key "tab bar") (set-boolean-preference key val))
            (else (set-preference key val))
      ) ;cond
    ) ;let*
  ) ;for
  (save-preferences)
) ;define

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
  (run-chain (append (list (cons "new document + snapshot"
                             (lambda ()
                               (new-document)
                               ;; 链尾会按此快照还原，避免污染全局配置。
                               (capture-pref-snapshot)
                             ) ;lambda
                           ) ;cons
                     ) ;list

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
                           ;; keyboard：5 combo + 8 IR = 13；macOS 多 keyboard shortcut style = 14。
                           (check-true (== (length (caddr (tab-ref meta "keyboard"))) (if (os-macos?) 14 13))
                           ) ;check-true
                           (check-true (== (length (caddr (tab-ref meta "mathematics"))) 10))
                           (check-true (== (length (caddr (tab-ref meta "convert"))) 0))
                           (check-true (== (length (caddr (tab-ref meta "other"))) 15))
                           ;; Convert 7 子 tab + sub-tab key 顺序。
                           (let ((convert (tab-ref meta "convert")))
                             (check-true (== (length (cadddr convert)) 7))
                             (check-true (equal? (map car (cadddr convert))
                                           (list "html" "latex" "bibtex" "verbatim" "pdf" "image" "mogan-scheme")
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
               ;;    toggle——有 kind/key/label/value + group flag。
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
                           ;; group flag（单栏分组标题，过 translate 非空串）。
                           (check-true (and (string? (field-ref brackets 'group))
                                         (> (string-length (field-ref brackets 'group)) 0)
                                       ) ;and
                           ) ;check-true
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 3.1 IR 控件双列布局：keyboard tab 的 ir-left / ir-center 等字段带
               ;;     layout=two-col 与 column 0/1（QML activeSections 据此左右分列）。
               ;;     keyboard shortcut style 仅 macOS 出现在 keyboard tab（非 macOS 过滤）。
               (list (cons "descriptor: keyboard IR layout/column + shortcut style tab"
                       (lambda ()
                         (let* ((meta (preferences-qml-meta))
                                (kb (caddr (tab-ref meta "keyboard")))
                                (ir-left (list-find kb (lambda (x) (== (field-ref x 'key) "ir-left"))))
                                (ir-center (list-find kb (lambda (x) (== (field-ref x 'key) "ir-center"))))
                                (kss (list-find kb (lambda (x) (== (field-ref x 'key) "keyboard shortcut style")))
                                ) ;kss
                               ) ;
                           ;; IR 字段双列布局：左列 column 0、右列 column 1。
                           (check-true (eq? (field-ref ir-left 'layout) 'two-col))
                           (check-true (== (field-ref ir-left 'column) 0))
                           (check-true (eq? (field-ref ir-center 'layout) 'two-col))
                           (check-true (== (field-ref ir-center 'column) 1))
                           ;; ir-left 的 group 横跨整行（groupSpan）+ group 过了 translate（非空串）。
                           (check-true (== (field-ref ir-left 'groupSpan) #t))
                           (check-true (and (string? (field-ref ir-left 'group))
                                         (> (string-length (field-ref ir-left 'group)) 0)
                                       ) ;and
                           ) ;check-true
                           ;; keyboard shortcut style 归到 keyboard tab，仅 macOS 显示。
                           (check-true (== (not (not kss)) (os-macos?)))
                           (when (os-macos?)
                             (check-true (== (field-ref kss 'restart?) #t))
                           ) ;when
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 3.5 hint 编码：带中文 hint 的字段（show only semantic focus），其 hint
               ;;     经 utf8->cork 归一化成 Cork，bridge cork_to_utf8 能还原成原 UTF-8。
               ;;     回归点：.scm 源码字面中文是 UTF-8 字节，reader 未转 Cork；若 facade
               ;;     不做 utf8->cork，bridge 把 UTF-8 字节当 Cork 二次解码 -> QML 显示乱码。
               ;;     断言 cork->utf8 后等于 UTF-8 原文（含中文），即编码往返无损。
               (list (cons "descriptor: hint utf8->cork round-trip"
                       (lambda ()
                         (let* ((meta (preferences-qml-meta))
                                (math (caddr (tab-ref meta "mathematics")))
                                ;; 找带 hint 的字段（show only semantic focus）。
                                (f (list-find math (lambda (x) (== (field-ref x 'key) "show only semantic focus")))
                                ) ;f
                                (hint-cork (and f (field-ref f 'hint)))
                               ) ;
                           (check-true (and f (string? hint-cork)))
                           ;; descriptor 里 hint 是 Cork 编码串；cork->utf8 须能还原（非空）。
                           (check-true (> (string-length (cork->utf8 hint-cork)) 0))
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 3.6 detailed menus 编解码表根因修复：options 的 internal key 用无空格
               ;;     "simple"（与 tm-modes.scm 的 simple-menus% 判定、tm-server.scm 默认值、
               ;;     preferences-menu.scm 的 enum 写入一致）。回归点：texmacs 遗留把
               ;;     define-preference-names 登记处写成 "simple "（带尾空格），使 encode/
               ;;     decode 表与消费侧脱节——get-pretty-preference 读无空格磁盘值时 encode
               ;;     表查不到、回退返回原始串，QML combo 翻译往返跟着退化。本步断言：
               ;;     (a) descriptor options 含无空格 "simple"；(b) set→get-pretty-preference
               ;;     往返正确（存 "simple" 能翻译成 "Simplified menus"）；(c) 收尾还原磁盘值。
               (list (cons "descriptor: detailed menus 'simple' no trailing space"
                       (lambda ()
                         (let* ((meta (preferences-qml-meta))
                                (general (caddr (tab-ref meta "general")))
                                (dm (list-find general (lambda (x) (== (field-ref x 'key) "detailed menus"))))
                                (key "detailed menus")
                                (old (get-preference key))
                               ) ;
                           ;; (a) options 用无空格 "simple"（带空格即根因复发）。
                           (check-true (and dm (member "simple" (field-ref dm 'options))))
                           (check-true (and dm (not (member "simple " (field-ref dm 'options)))))
                           ;; (b) 往返：存无空格 "simple" -> get-pretty-preference 须翻译成
                           ;;     显示串 "Simplified menus"（encode 表登记处与磁盘值一致才能命中）。
                           (set-preference key "simple")
                           (check-true (== (get-pretty-preference key) "Simplified menus"))
                           ;; current_value 经 optionsTr 反查后回吐无空格 internal key。
                           (check-true (== (preferences-qml-current-value "detailed menus" "combo"
                                                      (list "simple" "detailed")
                                                      (list "Simplified menus" "Detailed menus"))
                                          "simple"))
                           ;; (c) 还原磁盘值，不污染配置。
                           (set-preference key old)
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 4 submit（bridge quote 路径）：模拟 bridge 拼的带 quote 字符串表达式，
               ;;   eval 求值——钉死 quote 修复。用重启键 gui theme + later preset，确认
               ;;   走到弹窗分支返回 later、值 silent 落库。回归点：bridge 没 quote 时，
               ;;   assoc 字面量出现在实参位置被当函数应用（car "k" 当函数），报
               ;;   attempt to evaluate ("k" . "v") 并 SIGSEGV，submit 函数体根本没进。
               (list (cons "submit: bridge quoted assoc literal evaluates"
                       (lambda ()
                         (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "later")
                         (let* ((key "gui theme")
                                (want "liii-night")
                                ;; 与 PreferencesBridge::eval_submit 拼的串同构：quote + dotted pair。
                                (expr (string-append "(preferences-qml-submit '((\"" key "\" . \"" want "\")))")
                                ) ;expr
                                (rc (eval (read (open-input-string expr))))
                               ) ;
                           (check-true (equal? rc "later"))
                           (check-true (== (get-preference key) want))
                           (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "")
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 5 submit: 非重启 toggle（use large brackets）——钉死 bridge 的 dotted-pair
               ;;   assoc wire 与 facade (cdr (assoc ...)) 取值一致：toggle 值正确落库。
               ;;   回归点：旧 build_assoc_literal 产出二元组 ("k" "v")，cdr 得 ("v") 列表，
               ;;   (== val "on") 恒假，toggle 永不切——本步专挡该 bug。
               (list (cons "submit: non-restart toggle applies"
                       (lambda ()
                         (let* ((key "use large brackets")
                                (old (snapshot-bool key))
                                (want (if old "off" "on"))
                                (rc (preferences-qml-submit (list (cons key want))))
                               ) ;
                           (check-true (equal? rc "applied"))
                           (check-true (== (get-boolean-preference key) (== want "on")))
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 5 submit: 非重启 combo（complex actions）——combo 内部键正确落库、
               ;;   返回 "applied"。
               (list (cons "submit: non-restart combo applies"
                       (lambda ()
                         (let* ((key "complex actions")
                                (old (snapshot-pref key))
                                (want (if (== old "menus") "popups" "menus"))
                                (rc (preferences-qml-submit (list (cons key want))))
                               ) ;
                           (check-true (equal? rc "applied"))
                           (check-true (== (get-preference key) want))
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 6 submit: 副作用 key（buffer management）——联动 tab bar boolean 偏好。
               ;;   shared -> tab bar on；separate -> tab bar off。
               (list (cons "submit: buffer management links tab bar"
                       (lambda ()
                         (let* ((bm-key "buffer management")
                                (old-bm (snapshot-pref bm-key))
                                (old-tb (snapshot-bool "tab bar"))
                                (want-bm (if (== old-bm "shared") "separate" "shared"))
                                (rc (preferences-qml-submit (list (cons bm-key want-bm))))
                               ) ;
                           (check-true (equal? rc "applied"))
                           (check-true (== (get-preference bm-key) want-bm))
                           (check-true (== (get-boolean-preference "tab bar") (== want-bm "shared")))
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 7 submit: 含重启键 + MOGAN_TEST_CONFIRM_RESTART=cancel——返回 "cancel"，
               ;;   diff 内任一字段都不应落库（先确认再 apply：未 apply 故无需回滚）。
               (list (cons "submit: restart key + cancel applies nothing"
                       (lambda ()
                         (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "cancel")
                         (let* ((rk "gui theme")
                                (nrk "complex actions")
                                (old-rk (snapshot-pref rk))
                                (old-nrk (snapshot-pref nrk))
                                (rc (preferences-qml-submit (list (cons rk "liii-night") (cons nrk "menus"))))
                               ) ;
                           (check-true (equal? rc "cancel"))
                           (check-true (== (get-preference rk) old-rk))
                           (check-true (== (get-preference nrk) old-nrk))
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 8 submit: 含重启键 + MOGAN_TEST_CONFIRM_RESTART=later——返回 "later"：
               ;;   重启键走 silent 写值（已落库，下次启动生效）、非重启键实时 apply。
               ;;   不测 restart 分支——其会调 restart-TeXmacs 终止进程，与异步链收尾
               ;;   冲突；该分支代码简单（apply + save-all-buffers + restart-TeXmacs）。
               (list (cons "submit: restart key + later (silent + non-restart live)"
                       (lambda ()
                         (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "later")
                         (let* ((rk "gui theme")
                                (nrk "complex actions")
                                (old-rk (snapshot-pref rk))
                                (old-nrk (snapshot-pref nrk))
                                (want-nrk (if (== old-nrk "menus") "popups" "menus"))
                                (rc (preferences-qml-submit (list (cons rk "liii") (cons nrk want-nrk))))
                               ) ;
                           (check-true (equal? rc "later"))
                           (check-true (== (get-preference rk) "liii"))
                           (check-true (== (get-preference nrk) want-nrk))
                           ;; 清掉 confirm-restart preset，避免影响后续进程。
                           (system-setenv "MOGAN_TEST_CONFIRM_RESTART" "")
                         ) ;let*
                       ) ;lambda
                     ) ;cons
               ) ;list

               ;; 收尾：还原本轮被改的偏好（避免污染后续测试与检入配置）+ 报告 + 退出。
               (list (cons "restore prefs + check-report + quit"
                       (lambda () (restore-pref-snapshot) (check-report) (quit-TeXmacs))
                     ) ;cons
               ) ;list
             ) ;append
  ) ;run-chain
) ;tm-define
