;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-ghost-test.scm
;; DESCRIPTION : 纯逻辑单元测试：线段中点绿色吸附点装饰树（midpoint-
;;               decorations）的构造。不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r graphics-ghost-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/graphics/graphics-ghost.scm")

;; midpoint-decorations：空集中点不产生装饰

(define (test-midpoint-empty)
  (check (midpoint-decorations '()) => '())
) ;define

;; 单个中点：绿色 disk 圆点，坐标字符串原样进入 point

(define (test-midpoint-single)
  (check (midpoint-decorations '(("1" "2")))
    =>
    '((with "color" "green" "point-style" "disk" (point "1" "2")))
  ) ;check
) ;define

;; 多个中点：与输入顺序一致，各自独立成装饰

(define (test-midpoint-multiple)
  (check (midpoint-decorations '(("0.5" "-0.5") ("3" "4")))
    =>
    '((with "color" "green" "point-style" "disk" (point "0.5" "-0.5"))
      (with "color" "green" "point-style" "disk" (point "3" "4")))
  ) ;check
) ;define

(tm-define (regtest-graphics-ghost)
  (test-midpoint-empty)
  (test-midpoint-single)
  (test-midpoint-multiple)
  (check-report)
) ;tm-define
