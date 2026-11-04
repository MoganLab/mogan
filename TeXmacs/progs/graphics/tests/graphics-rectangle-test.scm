;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-rectangle-test.scm
;; DESCRIPTION : 纯逻辑单元测试：矩形（rectangle）的图形属性与闭合曲线判定，
;;               验证矩形在绘制模式下能够正常拥有 dash-style、line-width
;;               等曲线属性。无 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r graphics-rectangle-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/graphics/graphics-drd.scm")
(load "./TeXmacs/progs/graphics/graphics-utils.scm")

;; 矩形属于闭合曲线族，具备与多边形（cline）、圆（circle）一致的曲线属性

(define (test-rectangle-curve-tag)
  (check (graphical-closed-curve-tag? 'rectangle) => #t)
  (check (graphical-curve-tag? 'rectangle) => #t)
  (check (graphical-contains-curve-tag? 'rectangle) => #t)
) ;define

;; 矩形拥有边框虚线、线宽、填充色等曲线属性

(define (test-rectangle-attributes)
  (check (graphics-attribute? 'rectangle "dash-style") => #t)
  (check (graphics-attribute? 'rectangle "dash-style-unit") => #t)
  (check (graphics-attribute? 'rectangle "line-width") => #t)
  (check (graphics-attribute? 'rectangle "fill-color") => #t)
) ;define

;; (edit rectangle) 编辑模式包含 dash-style 与 line-width 属性

(define (test-rectangle-mode-attributes)
  (check (graphics-mode-attribute? '(edit rectangle) "dash-style") => #t)
  (check (graphics-mode-attribute? '(edit rectangle) "dash-style-unit") => #t)
  (check (graphics-mode-attribute? '(edit rectangle) "line-width") => #t)
  (check (graphics-mode-attribute? '(edit rectangle) "fill-color") => #t)
) ;define

;; graphics-enrich-filter 不过滤矩形的虚线样式属性

(define (test-rectangle-enrich-filter)
  (check (graphics-enrich-filter 'rectangle '(("dash-style" "11100")))
    =>
    '("dash-style" "11100")
  ) ;check
) ;define

(tm-define (regtest-graphics-rectangle)
  (test-rectangle-curve-tag)
  (test-rectangle-attributes)
  (test-rectangle-mode-attributes)
  (test-rectangle-enrich-filter)
  (check-report)
) ;tm-define
