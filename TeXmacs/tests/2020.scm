;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 2020.scm
;; DESCRIPTION : GUI 验证：双击 .tmu 应在已有窗口里以新标签页打开，而非新窗口
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT NO WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; PURPOSE
;;   需求：在 OS 文件管理器里双击 .tmu 文档，应在【已有的窗口】里新开一个
;;   标签页，而不是弹一个新窗口（除非当前没有任何窗口）。
;;
;;   C++ 侧的修复在 QTMGuiHelper::eventFilter：原本用 static new_window_flag
;;   让"第一个 FileOpen 事件之后"一律走 :new-window；改为按 has_current_window()
;;   判断——有当前窗口就传 :current-window（在当前窗口开新标签页），没有才
;;   :new-window。FileOpen 事件本身由 OS 派发，Scheme 里无法真实注入，因此
;;   本测试覆盖的是修复所依赖的【可观察契约】：load-buffer 的默认（当前窗口）
;;   路径会把后续文档开成新标签页且不增加窗口数；而显式 :new-window 才会增加
;;   窗口数。两条路径的窗口数差异，正是修复后双击行为的保证。
;;
;;   覆盖断言：
;;     1. 启动后只有 1 个窗口。
;;     2. 默认 load-buffer 载入 a：窗口数仍为 1，tabpage 数 +1。
;;     3. 默认 load-buffer 载入 b：窗口数仍为 1，tabpage 数再 +1（新标签页）。
;;     4. 显式 :new-window 载入 b：窗口数变为 2（确认 :new-window 路径仍生效，
;;        且与默认路径形成对照）。
;;
;;   夹具从 $TEXMACS_PATH/tests/tmu 复制到 /tmp，避免 save/编辑污染检入副本。
;;   用 exec-delayed-at 串异步链，不阻塞 Qt 事件循环；链尾自己 quit。
;;
;;   异步链 + (liii check)：每步用 (check got => expected) 累计断言，
;;   链尾调用 (check-report) 打印汇总行（包含失败计数）并在有失败时
;;   exit 1，CI 可直接据此判定。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 2020
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(define (tmp-name name)
  (string->url (string-append "/tmp/" name))
) ;define

(define (refresh-fixture name)
  (let ((src (string->url (string-append "$TEXMACS_PATH/tests/tmu/" name))))
    (when (url-exists? src)
      (system-copy src (tmp-name name))
    ) ;when
  ) ;let
) ;define

;; 步骤间隔：给 Qt 事件循环 + typeset 足够时间，避免异步事件叠加。

(define step-delay-ms 4000)

;; 窗口数与 tabpage 数，用于把日志和动作对齐、做断言。

(define (nr-windows)
  (length (window-list))
) ;define

(define (nr-tabpages)
  (length (tabpage-list #t))
) ;define

(define (log-state label)
  (display "[2020-step] ")
  (display label)
  (display "  windows=")
  (display (nr-windows))
  (display "  tabs=")
  (display (nr-tabpages))
  (newline)
) ;define

;; 串异步链：每步在 exec-delayed-at 触发，步间隔 step-delay-ms。
;; 每个元素是 (label . action-thunk)。

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[2020-step] ")
                           (display label)
                           (newline)
                           (act)
                           (log-state (string-append "after " label))
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_2020)
  (refresh-fixture "0101.tmu")
  (refresh-fixture "0103.tmu")
  (let* ((path-a (tmp-name "0101.tmu")) (path-b (tmp-name "0103.tmu")))
    ;; 动作链：
    ;;  - 先记下初始窗口/tab 数（启动后应为 1 窗口、1 tab：欢迎页）。
    ;;  - 默认 load-buffer 载入 a：应 1 窗口、tabs+1（新标签页，非新窗口）。
    ;;  - 默认 load-buffer 载入 b：应仍 1 窗口、tabs 再 +1（又一个新标签页）。
    ;;  - 显式 :new-window 载入 b：窗口数 +1（对照：只有显式 :new-window 才开窗）。
    ;;  - 结束：check-report 打印汇总并在有失败时 exit 1，然后 quit-TeXmacs。
    ;;
    ;; 取每步【执行后】的窗口/tab 数做断言；windows 预期全程为 1，直到最后一步。
    (let ((steps (append (list (cons "snapshot initial" (lambda () (check (nr-windows) => 1))))
                   ;; 1) 默认路径载入 a —— 双击文件后修复会走的就是这条路径
                   (list (cons "load a (default => current window)" (lambda () (load-buffer path-a)))
                     (cons "check after a" (lambda () (check (nr-windows) => 1)))
                   ) ;list
                   ;; 2) 默认路径载入 b —— 再双击第二个文件，仍应在新标签页打开
                   (list (cons "load b (default => current window)" (lambda () (load-buffer path-b)))
                     (cons "check after b"
                       (lambda ()
                         (check (nr-windows) => 1)
                         ;; b 应是相对 a 的又一个标签页：tab 数应 > 1
                         (check (> (nr-tabpages) 1) => #t)
                       ) ;lambda
                     ) ;cons
                   ) ;list
                   ;; 3) 显式 :new-window —— 对照组，确认该路径确实会开新窗口
                   (list (cons "load b :new-window (control)"
                           (lambda () (load-buffer path-b :new-window))
                         ) ;cons
                     (cons "check after :new-window" (lambda () (check (nr-windows) => 2)))
                   ) ;list
                   ;; 结束：汇总 + 退出。check-report 会在有失败时 exit 1。
                   (list (cons "done; reporting and quitting" (lambda () (check-report) (quit-TeXmacs)))
                   ) ;list
                 ) ;append
          ) ;steps
         ) ;
      (display "[2020-step] starting delayed chain\n")
      (run-chain steps)
    ) ;let
  ) ;let*
) ;tm-define
