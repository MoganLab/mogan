;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0905.scm
;; DESCRIPTION : 回归测试：双字母 tab 循环最右端应能切到单字母斜体形态
;;
;; 数学模式下连按两个相同字母（如 a a）进入符号变体 tab 循环，
;; 循环到最末档后应能继续切到「只按一个字母」的斜体形态（如 a），
;; 构成完整闭环。本测试覆盖普通字母 a、大写字母 A、以及基础绑定
;; 特殊的小写 m（其变体链整体后移一个 var 层级）。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 0905))

(import (liii check))
(check-set-mode! 'report-failed)

;; 规避 main 上与本任务无关的遗留问题：list-tag-list 被多处引用但未定义，
;; 数学模式下按键后的菜单/页脚谓词求值会抛错并打断按键处理，此处打桩
(when (not (defined? 'list-tag-list))
  (define (list-tag-list)
    '(enumerate itemize description)
  ) ;define
) ;when

(define (body-verbatim)
  (texmacs->verbatim (buffer-get-body (current-buffer)))
) ;define

(define (press key)
  (catch #t
    (lambda () (keyboard-press key 0))
    (lambda args (display* "0905 press error: " args "\n"))
  ) ;catch
) ;define

(define (press-times key n)
  (when (> n 0)
    (press key)
    (press-times key (- n 1))
  ) ;when
) ;define

(define step-delay-ms 300)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (catch #t
                             (lambda () (act))
                             (lambda args (display* "0905 step error: " args "\n"))
                           ) ;catch
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

;; delayed-kbd-map 分片注册与 math-kbd 懒加载需要事件循环时间，
;; 轮询等待本任务新增的「单字母斜体」末档绑定就绪后再开始按键
;; （该绑定是判断整个字母变体链注册完成的最可靠信号）

(define (wait-kbd-ready next tries)
  (exec-delayed-at (lambda ()
                     (if (pair? (catch #t
                                  (lambda () (kbd-find-key-binding "a a var var var var var"))
                                  (lambda args #f)
                                ) ;catch
                         ) ;pair?
                       (next)
                       (if (> tries 0) (wait-kbd-ready next (- tries 1)) (next))
                     ) ;if
                   ) ;lambda
    (+ (texmacs-time) 500)
  ) ;exec-delayed-at
) ;define

;; 对字母 letter 验证：连按两次进入循环，tab 走到末档 <b-up-letter> 后再多按一次 tab，
;; 底文应恰好是该字母的斜体形态（与单按一次该字母一致）。
;;   single-tab-count: 从双字母基础态按多少次 tab 切到单字母斜体
;;              普通字母 5 次（基础 bbb，tab 切 frak/b/up/b-up/single）
;;              小写 m   6 次（基础无绑定，从 m m tab 的 bbb-m 起，多一档）

(define (check-cycle-to-single letter single-tab-count)
  (run-chain (list (cons "new document"
                     (lambda () (new-document) (check (get-env "mode") => "text"))
                   ) ;cons
               (cons "enter math" (lambda () (press "$") (check (get-env "mode") => "math")))
               (cons (string-append "type " letter letter)
                 (lambda () (press letter) (press letter))
               ) ;cons
               (cons "tab through variants to single letter"
                 (lambda ()
                   (press-times "tab" single-tab-count)
                   ;; 单字母斜体形态：底文恰好是该字母（数学模式渲染为斜体），
                   ;; 不再含任何 <...> 符号标记
                   (check (body-verbatim) => letter)
                 ) ;lambda
               ) ;cons
               (cons "report + quit" (lambda () (check-report) (quit-TeXmacs)))
             ) ;list
  ) ;run-chain
) ;define

(define (run-steps)
  (check-cycle-to-single "a" 5)
) ;define

(tm-define (test_0905)
  ;; 插件经 :idle 延迟任务初始化，测试事件循环下未必触发，这里强制初始化。
  ;; math-kbd 依赖 math-edit（math-tabcycle-menu-needed? 等定义于此），
  ;; 显式加载避免按键处理时该谓词 unbound 打断循环
  (plugin-initialize 'math-kbd)
  (module-provide '(math math-edit))
  (wait-kbd-ready run-steps 30)
) ;tm-define
