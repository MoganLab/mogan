;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0986.scm
;; DESCRIPTION : 集成测试：AI翻译悬浮按钮依赖的选区几何 glue（0986 第一步）。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; PURPOSE
;;   [0986] 选中文字后在其最末文字右方弹出「AI翻译」悬浮按钮，按钮尺寸随选区
;;   最小文字字号缩放。C++ 侧的定位/尺寸依赖两个选区几何函数：
;;     - get_selection_last_rect  ：选区最末（屏幕最下方、同行最右）矩形
;;     - get_selection_min_height ：选区内最小文字渲染高度
;;   二者经 glue 暴露为 (selection-last-rect) / (selection-min-height)，
;;   本测试钉死其语义（相对比较，不依赖绝对坐标）：
;;     1. 只选第二段 与 跨两段选区 的最末矩形相同（都在第二段末）。
;;     2. 跨两段选区的最末矩形比只选第一段的更靠屏幕下方（逻辑 y2 更小）。
;;     3. 混排字号选区的最小高度等于其中小字单选的高度，且小于大字单选。
;;     4. 无选区时最小高度为 0。
;;
;; USAGE
;;   xmake b stem && xmake r 0986
;;
;; 断言全部同步执行（不串 exec-delayed 链），headless 模式下直接生效。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

;; 选中从 start 到 end 的路径区间，返回后由调用方读取几何 glue。

(define (select-range start end)
  (selection-cancel)
  (go-to start)
  (selection-set-start)
  (go-to end)
  (selection-set-end)
) ;define

(tm-define (test_0986)
  (buffer-set-body (current-buffer) '(document "AAAA" "BBBB"))

  ;; 1 最末矩形由选区末尾决定：只选第二段 == 跨两段（末尾都在第二段末）。
  (select-range (path 1 0) (path 1 4))
  (let ((rect-para2 (selection-last-rect)))
    (check-true (== (length rect-para2) 4))
    (select-range (path 0 0) (path 1 4))
    (check (selection-last-rect) => rect-para2)
  ) ;let

  ;; 2 更靠屏幕下方 => 逻辑 y2 更小（逻辑坐标 y 向上）。
  (select-range (path 1 0) (path 1 4))
  (let ((rect-para2 (selection-last-rect)))
    (select-range (path 0 0) (path 0 4))
    (let ((rect-para1 (selection-last-rect)))
      (check-true (< (list-ref rect-para2 3) (list-ref rect-para1 3)))
    ) ;let
  ) ;let

  ;; 3 最小高度取选区内最小字号：混排 == 小字单选，且 < 大字单选。
  (buffer-set-body (current-buffer)
    '(document (with "font-size" "2" "Big") "small"))
  (select-range (path 0 1 0) (path 0 1 5))
  (let ((h-small (selection-min-height)))
    (check-true (> h-small 0))
    (select-range (path 0 0 0) (path 0 1 5))
    (check (selection-min-height) => h-small)
    (select-range (path 0 0 0) (path 0 0 3))
    (check-true (> (selection-min-height) h-small))
  ) ;let

  ;; 4 无选区 => 最小高度 0。
  (selection-cancel)
  (check (selection-min-height) => 0)

  (check-report)
) ;tm-define
