;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 201_63_text_toolbar_test.scm
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helper functions for testing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 模拟缓存时间检查
(define (cache-still-valid? last-check current-time)
  "检查缓存是否在100ms有效期内"
  (< (- current-time last-check) 100))

;; 模拟缓存失效判断
(define (should-invalidate-cache? last-check current-time)
  "检查是否应该使缓存失效"
  (>= (- current-time last-check) 100))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Cache mechanism tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check "缓存有效期内应该返回true"
  (cache-still-valid? 1000 1099) => #t)

(check "缓存过期边界（正好100ms）应该返回false"
  (cache-still-valid? 1000 1100) => #f)

(check "缓存过期后应该返回false"
  (cache-still-valid? 1000 1101) => #f)

(check "缓存失效判断：有效期内"
  (should-invalidate-cache? 1000 1099) => #f)

(check "缓存失效判断：正好过期"
  (should-invalidate-cache? 1000 1100) => #t)

(check "缓存失效判断：已过期"
  (should-invalidate-cache? 1000 1200) => #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Rectangle validity tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (rectangle-valid? x1 y1 x2 y2)
  "检查矩形是否有效（非零正面积）"
  (and (< x1 x2) (< y1 y2)))

(check "有效矩形检测"
  (rectangle-valid? 100 200 300 400) => #t)

(check "零宽度矩形应该无效"
  (rectangle-valid? 100 200 100 400) => #f)

(check "零高度矩形应该无效"
  (rectangle-valid? 100 200 300 200) => #f)

(check "负宽度矩形应该无效"
  (rectangle-valid? 300 200 100 400) => #f)

(check "负高度矩形应该无效"
  (rectangle-valid? 100 400 300 200) => #f)

(check "最小有效矩形（1x1）"
  (rectangle-valid? 0 0 1 1) => #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Coordinate conversion tests (simulating SI to pixel conversion)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define INV_UNIT (/ 1.0 256))

(define (si->pixel si-coord)
  "将SI坐标转换为像素坐标"
  (inexact->exact (round (* si-coord INV_UNIT))))

(check "坐标转换：2560 -> 10"
  (si->pixel 2560) => 10)

(check "坐标转换：5120 -> 20"
  (si->pixel 5120) => 20)

(check "坐标转换：0 -> 0"
  (si->pixel 0) => 0)

(check "坐标转换：255 -> 1（四舍五入）"
  (si->pixel 255) => 1)

(check "坐标转换：256 -> 1（正好1单位）"
  (si->pixel 256) => 1)

(check "坐标转换：257 -> 1（略大于1）"
  (si->pixel 257) => 1)

(check "坐标转换：大数值1000000"
  (si->pixel 1000000) => 3906)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Mode checking simulation tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (should-show-toolbar? in-math? in-prog? in-code? in-verbatim? 
                               has-selection? selection-empty?)
  "模拟should_show_text_toolbar的逻辑"
  (and (not in-math?)
       (not in-prog?)
       (not in-code?)
       (not in-verbatim?)
       has-selection?
       (not selection-empty?)))

(check "普通文本选区：应该显示"
  (should-show-toolbar? #f #f #f #f #t #f) => #t)

(check "数学模式中：不应该显示"
  (should-show-toolbar? #t #f #f #f #t #f) => #f)

(check "编程模式中：不应该显示"
  (should-show-toolbar? #f #t #f #f #t #f) => #f)

(check "代码模式中：不应该显示"
  (should-show-toolbar? #f #f #t #f #t #f) => #f)

(check "原文模式中：不应该显示"
  (should-show-toolbar? #f #f #f #t #t #f) => #f)

(check "无选区时：不应该显示"
  (should-show-toolbar? #f #f #f #f #f #f) => #f)

(check "空选区时：不应该显示"
  (should-show-toolbar? #f #f #f #f #t #t) => #f)

(check "数学模式 + 无选区：不应该显示"
  (should-show-toolbar? #t #f #f #f #f #f) => #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Viewport intersection tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (selection-in-view? sel-x1 sel-y1 sel-x2 sel-y2
                           view-x1 view-y1 view-x2 view-y2)
  "检查选区是否在视图范围内"
  (not (or (< sel-x2 view-x1)    ; 选区在视图左侧
           (> sel-x1 view-x2)    ; 选区在视图右侧
           (< sel-y2 view-y1)    ; 选区在视图上方
           (> sel-y1 view-y2)))) ; 选区在视图下方

(check "选区完全在视图内"
  (selection-in-view? 100 100 200 200 0 0 500 500) => #t)

(check "选区部分在视图内（左侧）"
  (selection-in-view? -50 100 50 200 0 0 500 500) => #t)

(check "选区部分在视图内（右侧）"
  (selection-in-view? 450 100 550 200 0 0 500 500) => #t)

(check "选区完全在视图左侧"
  (selection-in-view? -100 100 -50 200 0 0 500 500) => #f)

(check "选区完全在视图右侧"
  (selection-in-view? 550 100 600 200 0 0 500 500) => #f)

(check "选区完全在视图上方"
  (selection-in-view? 100 -100 200 -50 0 0 500 500) => #f)

(check "选区完全在视图下方"
  (selection-in-view? 100 550 200 600 0 0 500 500) => #f)

(check "选区正好接触视图边界（应该算在视图内）"
  (selection-in-view? 0 0 100 100 0 0 500 500) => #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Run all tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check-report)
