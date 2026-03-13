;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 201_63.scm
;; DESCRIPTION : Unit tests for text toolbar functionality
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(tm-define (test_201_63)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helper functions for testing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 模拟缓存时间检查
(define (cache-still-valid? last-check current-time)
  (< (- current-time last-check) 100))

;; 模拟缓存失效判断
(define (should-invalidate-cache? last-check current-time)
  (>= (- current-time last-check) 100))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Cache mechanism tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 缓存有效期内应该返回true
(check (cache-still-valid? 1000 1099) => #t)

;; 缓存过期边界（正好100ms）应该返回false
(check (cache-still-valid? 1000 1100) => #f)

;; 缓存过期后应该返回false
(check (cache-still-valid? 1000 1101) => #f)

;; 缓存失效判断：有效期内
(check (should-invalidate-cache? 1000 1099) => #f)

;; 缓存失效判断：正好过期
(check (should-invalidate-cache? 1000 1100) => #t)

;; 缓存失效判断：已过期
(check (should-invalidate-cache? 1000 1200) => #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Rectangle validity tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (rectangle-valid? x1 y1 x2 y2)
  (and (< x1 x2) (< y1 y2)))

;; 有效矩形检测
(check (rectangle-valid? 100 200 300 400) => #t)

;; 零宽度矩形应该无效
(check (rectangle-valid? 100 200 100 400) => #f)

;; 零高度矩形应该无效
(check (rectangle-valid? 100 200 300 200) => #f)

;; 负宽度矩形应该无效
(check (rectangle-valid? 300 200 100 400) => #f)

;; 负高度矩形应该无效
(check (rectangle-valid? 100 400 300 200) => #f)

;; 最小有效矩形（1x1）
(check (rectangle-valid? 0 0 1 1) => #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Coordinate conversion tests (simulating SI to pixel conversion)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define INV_UNIT (/ 1.0 256))

(define (si->pixel si-coord)
  (inexact->exact (round (* si-coord INV_UNIT))))

;; 坐标转换：2560 -> 10
(check (si->pixel 2560) => 10)

;; 坐标转换：5120 -> 20
(check (si->pixel 5120) => 20)

;; 坐标转换：0 -> 0
(check (si->pixel 0) => 0)

;; 坐标转换：255 -> 1（四舍五入）
(check (si->pixel 255) => 1)

;; 坐标转换：256 -> 1（正好1单位）
(check (si->pixel 256) => 1)

;; 坐标转换：257 -> 1（略大于1）
(check (si->pixel 257) => 1)

;; 坐标转换：大数值1000000
(check (si->pixel 1000000) => 3906)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Mode checking simulation tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (should-show-toolbar? in-math? in-prog? in-code? in-verbatim? 
                               has-selection? selection-empty?)
  (and (not in-math?)
       (not in-prog?)
       (not in-code?)
       (not in-verbatim?)
       has-selection?
       (not selection-empty?)))

;; 普通文本选区：应该显示
(check (should-show-toolbar? #f #f #f #f #t #f) => #t)

;; 数学模式中：不应该显示
(check (should-show-toolbar? #t #f #f #f #t #f) => #f)

;; 编程模式中：不应该显示
(check (should-show-toolbar? #f #t #f #f #t #f) => #f)

;; 代码模式中：不应该显示
(check (should-show-toolbar? #f #f #t #f #t #f) => #f)

;; 原文模式中：不应该显示
(check (should-show-toolbar? #f #f #f #t #t #f) => #f)

;; 无选区时：不应该显示
(check (should-show-toolbar? #f #f #f #f #f #f) => #f)

;; 空选区时：不应该显示
(check (should-show-toolbar? #f #f #f #f #t #t) => #f)

;; 数学模式 + 无选区：不应该显示
(check (should-show-toolbar? #t #f #f #f #f #f) => #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Viewport intersection tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (selection-in-view? sel-x1 sel-y1 sel-x2 sel-y2
                           view-x1 view-y1 view-x2 view-y2)
  (not (or (< sel-x2 view-x1)
           (> sel-x1 view-x2)
           (< sel-y2 view-y1)
           (> sel-y1 view-y2))))

;; 选区完全在视图内
(check (selection-in-view? 100 100 200 200 0 0 500 500) => #t)

;; 选区部分在视图内（左侧）
(check (selection-in-view? -50 100 50 200 0 0 500 500) => #t)

;; 选区部分在视图内（右侧）
(check (selection-in-view? 450 100 550 200 0 0 500 500) => #t)

;; 选区完全在视图左侧
(check (selection-in-view? -100 100 -50 200 0 0 500 500) => #f)

;; 选区完全在视图右侧
(check (selection-in-view? 550 100 600 200 0 0 500 500) => #f)

;; 选区完全在视图上方
(check (selection-in-view? 100 -100 200 -50 0 0 500 500) => #f)

;; 选区完全在视图下方
(check (selection-in-view? 100 550 200 600 0 0 500 500) => #f)

;; 选区正好接触视图边界（应该算在视图内）
(check (selection-in-view? 0 0 100 100 0 0 500 500) => #t)

  (check-report))
