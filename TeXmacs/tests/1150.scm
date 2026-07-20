;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1150.scm
;; DESCRIPTION : GUI 基准：在新建标签页的表格中连续插入新行的性能
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   测量"在表格里连续向下插入 N 行"的总耗时，配合 C++ 侧 bench 埋点
;;   （table_insert_row / table_insert / table_correct_block_content /
;;   table_resize_notify）定位热点。
;;
;;   实现要点（参考 1145.scm 的 GUI 异步链模式）：
;;     - `(new-document)` 在界面里新建一个标签页（遵循 window-per-buffer? 偏好）。
;;       紧接 `(set-main-style "generic")` 切到 generic 样式——new-document
;;       默认开无样式文档，与用户实际场景不一致。
;;     - `(make 'tabular)` 新建 1×1 表格并定位光标到首个 cell，
;;       与「插入 → 表格」菜单同源。
;;     - `(table-insert-row #t)` 是 kbd-enter / 菜单"Row below"走的路径。
;;     - 用 `exec-delayed-pause` + `run-chain` 串异步链，每步间隔 step-delay-ms
;;       让事件循环真正驱动 GUI（typeset + idle update_menus 才会跑）。
;;       必须真实 GUI 跑；exec-delayed-at 单次调度事件循环不真正驱动，
;;       本质还是 headless。
;;     - 每步插 rows-per-step 行（默认 1 行），步间让事件循环空转 +
;;       `(update-menus)` 同步触发 C++ 侧刷新。
;;     - 在 checkpoint-rows / total-rows 处各打印一次 bench dump + 累积
;;       插入耗时，一次运行直接对比两个规模的增长曲线。
;;
;;   bench 输出：链尾调用 `(bench-print-all)` dump C++ 侧所有累积 task。
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1150
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1150))

;; 步骤间隔：给 typeset + idle update_menus 足够时间（idle >= 1/60s 才触发）。

(define step-delay-ms 1500)

;; 先用小规模探明热点：跑到 checkpoint-rows 打印一次 bench dump + 总耗时，
;; 再跑到 total-rows 打印第二次。一次运行直接对比两个规模的增长曲线。

(define checkpoint-rows 50)

(define total-rows 100)

(define rows-per-step 1)

;; 每行的列数。建表后先扩到 num-cols 列再开始插行——真实表格很少是单列，
;; 多列场景下 table_correct_block_content 的 O(N×M) 开销更显著。

(define num-cols 10)

;; 记录每步开始的时间，链尾累加得到总插入耗时（不含步间 sleep）。

(define step-start-time 0)

(define total-insert-ms 0)

(define (log-step label)
  (display "[1150-step] ")
  (display label)
  (newline)
) ;define

;; run-chain 复刻 1145.scm 的 exec-delayed-pause 模式：每步 lambda 返回剩余
;; 毫秒表示继续等待，返回 #t 表示完成。

(define (run-chain steps on-done)
  (if (null? steps)
    (on-done)
    (exec-delayed-pause (let ((start (texmacs-time)))
                          (lambda ()
                            (let ((left (- (+ start step-delay-ms) (texmacs-time))))
                              (if (> left 0)
                                left
                                (begin
                                  (log-step (caar steps))
                                  ((cdar steps))
                                  ;; 同步触发 C++ update_menus，不依赖 idle/焦点
                                  (update-menus)
                                  (run-chain (cdr steps) on-done)
                                  #t
                                ) ;begin
                              ) ;if
                            ) ;let
                          ) ;lambda
                        ) ;let
    ) ;exec-delayed-pause
  ) ;if
) ;define

;; 当前已插入行数（跨步累积）

(define rows-so-far 0)

;; 单步：同步插 rows-per-step 行。开头记 start、结尾累加 elapsed 到 total。

(define (make-insert-step from-rows target-rows)
  (cons (string-append "insert row "
          (number->string (+ from-rows 1))
          " → "
          (number->string target-rows)
        ) ;string-append
    (lambda ()
      (set! step-start-time (texmacs-time))
      (let loop
        ((k 0))
        (when (< k rows-per-step)
          (table-insert-row #t)
          (set! rows-so-far (+ rows-so-far 1))
          (loop (+ k 1))
        ) ;when
      ) ;let
      (set! total-insert-ms (+ total-insert-ms (- (texmacs-time) step-start-time)))
    ) ;lambda
  ) ;cons
) ;define

;; 在指定行数处插入一次 bench dump 步骤（不 reset，累积继续）

(define (make-checkpoint-step n)
  (cons (string-append "checkpoint @ " (number->string n) " rows")
    (lambda ()
      (display "[1150] @ ")
      (display n)
      (display " rows: cumulative insert=")
      (display total-insert-ms)
      (display " ms")
      (newline)
      (bench-print-all)
    ) ;lambda
  ) ;cons
) ;define

;; 生成 [from..n] 行的插入步序列（不依赖全局 rows-so-far——它在 step 执行后才更新）

(define (insert-steps-up-to from n)
  (let loop
    ((i from) (acc '()))
    (if (>= i n)
      (reverse acc)
      (loop (+ i rows-per-step) (cons (make-insert-step i (+ i rows-per-step)) acc))
    ) ;if
  ) ;let
) ;define

(tm-define (test_1150)
  ;; 重置累积
  (set! total-insert-ms 0)
  (set! rows-so-far 0)
  ;; 链头四步：新建标签页 + 切 generic 样式 + 建表 + 扩到 num-cols 列；
  ;; 之后插到 checkpoint-rows → 打印 checkpoint → 插到 total-rows → checkpoint → quit。
  (let* ((head (list (cons "new-document" (lambda () (new-document)))
                 (cons "set style generic" (lambda () (set-main-style "generic")))
                 (cons "make tabular" (lambda () (make 'tabular)))
                 (cons (string-append "expand to " (number->string num-cols) " cols")
                   (lambda ()
                     (let loop
                       ((c 1))
                       (when (< c num-cols)
                         (table-insert-column #t)
                         (loop (+ c 1))
                       ) ;when
                     ) ;let
                   ) ;lambda
                 ) ;cons
               ) ;list
         ) ;head
         (steps (append head
                  (insert-steps-up-to 0 checkpoint-rows)
                  (list (make-checkpoint-step checkpoint-rows))
                  (insert-steps-up-to checkpoint-rows total-rows)
                  (list (make-checkpoint-step total-rows))
                ) ;append
         ) ;steps
         (on-done (lambda () (display "[1150-step] done, quit") (newline) (quit-TeXmacs))
         ) ;on-done
        ) ;
    (run-chain steps on-done)
  ) ;let*
) ;tm-define
