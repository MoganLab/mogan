;; MODULE      : 1199.scm
;; DESCRIPTION : 回归测试：kbd-map 支持 UTF-8 键名（kbd-binding 注册时
;;             : UTF-8→cork 归一），中文输入法 Shift+4 提交的全角 ￥
;;             : （cork <#FFE5>）由此可在 kbd-map 生效。
;;
;;   断言（keyboard-press 直调，驱动真实 key_press 路径）：
;;     1. 编码契约：(string-convert "￥" "UTF-8" "Cork") => "<#FFE5>"。
;;     2. 注册契约：(kbd-find-key-binding "<#FFE5>") 有绑定
;;        （scheme 侧 ("￥" ...) 键名经归一后被 cork 查找命中）。
;;     3. 文本模式 keyboard-press "<#FFE5>" → (get-env "mode") 变 "math"。
;;     4. 数学模式再次 keyboard-press "<#FFE5>" → mode 回到 "text"。
;;     5. 变体：文本模式 ￥ 后按 tab → 撤销数学模式、插入 ￥ 字符本身。
;;     6. 文本模式 keyboard-press "<#3001>"（、）→ 进入 hybrid（同 \）。
;;     7. 、 后按 tab → 撤销 hybrid、插入 、 字符本身。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1199      # 真实 GUI，跑断言链
;;
;; 注意：断言在异步链里，必须 MOGAN_TEST_GUI=1 才执行——headless 模式
;; （xmake r 1199）启动即 (quit-TeXmacs)，异步链来不及调度，断言不跑
;; （仅冒烟进程不崩）。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.

(texmacs-module (texmacs tests 1199))

(import (liii check))
(check-set-mode! 'report-failed)

(define step-delay-ms 500)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1199-step] ")
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

(tm-define (test_1199)
  (run-chain (list (cons "encoding contract: ￥ -> cork <#FFE5>"
                     (lambda () (check (string-convert "￥" "UTF-8" "Cork") => "<#FFE5>"))
                   ) ;cons
               (cons "new document in text mode"
                 (lambda () (new-document) (check (get-env "mode") => "text"))
               ) ;cons
               (cons "registration: utf8 key ￥ reachable via cork lookup"
                 (lambda () (check-true (pair? (kbd-find-key-binding "<#FFE5>"))))
               ) ;cons
               (cons "<#FFE5> enters math mode"
                 (lambda () (keyboard-press "<#FFE5>" 0) (check (get-env "mode") => "math"))
               ) ;cons
               (cons "<#FFE5> again leaves math mode"
                 (lambda () (keyboard-press "<#FFE5>" 0) (check (get-env "mode") => "text"))
               ) ;cons
               (cons "variant: ￥ then tab inserts ￥ itself"
                 (lambda ()
                   (keyboard-press "<#FFE5>" 0)
                   (check (get-env "mode") => "math")
                   (keyboard-press "tab" 0)
                   (check (get-env "mode") => "text")
                   (check-true (string-contains? (tree->string (buffer-tree)) "￥"))
                 ) ;lambda
               ) ;cons
               (cons "、 behaves like backslash (hybrid) in text mode"
                 (lambda () (keyboard-press "<#3001>" 0) (check-true (inside? 'hybrid)))
               ) ;cons
               (cons "、 then tab inserts 、 itself"
                 (lambda ()
                   (keyboard-press "tab" 0)
                   (check-false (inside? 'hybrid))
                   (check-true (string-contains? (tree->string (buffer-tree)) "、"))
                 ) ;lambda
               ) ;cons
               (cons "report + quit" (lambda () (check-report) (quit-TeXmacs)))
             ) ;list
  ) ;run-chain
  (noop)
) ;tm-define
