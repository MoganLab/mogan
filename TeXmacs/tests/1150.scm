;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1150.scm
;; DESCRIPTION : 基准测试：在表格中连续插入新行的性能
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   测量"在表格里连续向下插入 N 行"的总耗时与单行平均耗时，
;;   作为后续表格插入路径优化的对比基线。
;;
;;   实现要点：
;;     - `(make 'tabular)` 新建 1×1 表格并定位光标到首个 cell，
;;       与「插入 → 表格」菜单同源。
;;     - `(table-insert-row #t)` 是 kbd-enter / 菜单"Row below"走的路径。
;;     - 每行插入之间不插入 kbd 等待或排版抖动；insert 内部已驱动必要的
;;       排版增量更新，测量的是真实用户体感的"按一下回车"链路耗时。
;;
;; USAGE
;;   xmake b stem
;;   xmake r 1150
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1150))

(define insert-count 1000)

(define (bench-insert-rows n)
  (make 'tabular)
  (let ((start (texmacs-time)))
    (let loop ((i 0))
      (when (< i n)
        (table-insert-row #t)
        (loop (+ i 1))
      ) ;when
    ) ;let
    (let* ((elapsed (- (texmacs-time) start))
           (per-row (if (zero? n) 0 (/ elapsed n)))
          ) ;
      (display "[1150] insert ")
      (display n)
      (display " rows: total=")
      (display elapsed)
      (display " ms, per-row=")
      (display per-row)
      (display " ms")
      (newline)
    ) ;let*
  ) ;let
) ;define

(tm-define (test_1150)
  (bench-insert-rows insert-count)
) ;tm-define
