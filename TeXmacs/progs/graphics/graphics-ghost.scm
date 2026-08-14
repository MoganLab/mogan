;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-ghost.scm
;; DESCRIPTION : ghost lines and angle guides for graphics drawing
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (graphics graphics-ghost)
  (:use (graphics graphics-env) (graphics graphics-utils))
) ;texmacs-module

(define ghost-lines '())

(define midpoints '())

(tm-define (graphics-get-all-previous-points)
  (:state graphics-state)
  (if (and sticky-point (integer? current-point-no) (> current-point-no 0))
    (let* ((obj (stree-radical (car (sketch-get1)))) (pts (cdr obj)))
      (let loop
        ((i 0) (res '()))
        (if (< i (min current-point-no (length pts)))
          (let ((p (list-ref pts i)))
            (if (and (pair? p) (== (car p) 'point))
              (loop (+ i 1) (cons (cdr p) res))
              (loop (+ i 1) res)
            ) ;if
          ) ;let
          (tm->tree (cons 'tuple (map (lambda (x) (cons 'tuple x)) (reverse res))))
        ) ;if
      ) ;let
    ) ;let*
    (tm->tree '(tuple))
  ) ;if
) ;tm-define

;; 折线（line/cline）绘制中，已落固定点连成的各线段中点。绘制中的
;; 对象尚未进入文档树，C++ graphical_select 选不到，由这里按文档坐标
;; 系提供；spline 等曲线的控制点连线不是线段，不在其列
(tm-define (graphics-get-previous-midpoints)
  (:state graphics-state)
  (if (and sticky-point (integer? current-point-no) (> current-point-no 1))
    (let* ((obj (stree-radical (car (sketch-get1))))
           (pts (list-head (cdr obj) (min current-point-no (length (cdr obj)))))
          ) ;
      (if (and (pair? obj) (in? (car obj) '(line cline)))
        (let loop
          ((l pts) (res '()))
          (if (or (null? l) (null? (cdr l)))
            (tm->tree (cons 'tuple (reverse res)))
            (let* ((p1 (car l)) (p2 (cadr l)))
              (if (and (tm-func? p1 'point 2) (tm-func? p2 'point 2))
                (let* ((x1 (s2f (cadr p1)))
                       (y1 (s2f (caddr p1)))
                       (x2 (s2f (cadr p2)))
                       (y2 (s2f (caddr p2)))
                       (m (cons 'tuple (list (f2s (/ (+ x1 x2) 2)) (f2s (/ (+ y1 y2) 2)))))
                      ) ;
                  (loop (cdr l) (cons m res))
                ) ;let*
                (loop (cdr l) res)
              ) ;if
            ) ;let*
          ) ;if
        ) ;let
        (tm->tree '(tuple))
      ) ;if
    ) ;let*
    (tm->tree '(tuple))
  ) ;if
) ;tm-define

(tm-define (graphics-clear-ghost-lines)
  (:state graphics-state)
  (set! ghost-lines '())
) ;tm-define

;; 参数 lines: (tuple (x y theta) ...)
(tm-define (graphics-set-ghost-lines lines)
  (:state graphics-state)
  (let ((l (if (tree? lines) (tree->stree lines) lines)))
    (if (and (pair? l) (== (car l) 'tuple))
      (set! ghost-lines
        (map (lambda (e) (list (list (cadr e) (caddr e)) (cadddr e))) (cdr l))
      ) ;set!
      (set! ghost-lines '())
    ) ;if
    (if (nnull? ghost-lines) (graphics-decorations-update))
  ) ;let
) ;tm-define

(tm-define (graphics-get-decorations-ghost-line)
  (if (nnull? ghost-lines)
    (let loop
      ((lines ghost-lines) (res '()))
      (if (null? lines)
        res
        (let* ((line-item (car lines))
               (p1 (car line-item))
               (theta (cadr line-item))
               (x1 (string->number (car p1)))
               (y1 (string->number (cadr p1)))
               (t (string->number theta))
               (dx (cos t))
               (dy (sin t))
               (x_start (number->string (- x1 (* 50.0 dx))))
               (y_start (number->string (- y1 (* 50.0 dy))))
               (x_end (number->string (+ x1 (* 50.0 dx))))
               (y_end (number->string (+ y1 (* 50.0 dy))))
              ) ;
          (loop (cdr lines)
            (cons `(with ,"color"
                     ,"green"
                     (line (point ,x_start ,y_start) (point ,x_end ,y_end)))
              res
            ) ;cons
          ) ;loop
        ) ;let*
      ) ;if
    ) ;let
    '()
  ) ;if
) ;tm-define

;; 把中点列表 (("x1" "y1") ...) 转成绿色圆点装饰，坐标为字符串

(define (midpoint-decorations pts)
  (if (nnull? pts)
    (map (lambda (p)
           `(with ,"color"
              ,"green"
              ,"point-style"
              ,"disk"
              (point ,(car p) ,(cadr p)))
         ) ;lambda
      pts
    ) ;map
    '()
  ) ;if
) ;define

(tm-define (graphics-clear-midpoints)
  (:state graphics-state)
  (set! midpoints '())
) ;tm-define

;; 参数 points: (tuple (x y) ...)；C++ 每次鼠标移动都会调用，
;; 这里做变更检测，仅当中点集合变化时才刷新装饰
(tm-define (graphics-set-midpoints points)
  (:state graphics-state)
  (let* ((l (if (tree? points) (tree->stree points) points))
         (new (if (and (pair? l) (== (car l) 'tuple))
                (map (lambda (e) (list (cadr e) (caddr e))) (cdr l))
                '()
              ) ;if
         ) ;new
        ) ;
    (if (not (equal? new midpoints))
      (begin
        (set! midpoints new)
        (graphics-decorations-update)
      ) ;begin
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (graphics-get-decorations-midpoint) (midpoint-decorations midpoints))

(tm-define (graphics-midpoints-active?)
  (:state graphics-state)
  (nnull? midpoints)
) ;tm-define

;; 非绘制态（未落第一个点）悬停线段时，装饰层会被整体重建；此时仅
;; 保留中点绿点，避免绿点预览被清空
(tm-define (graphics-render-midpoints)
  (:state graphics-state)
  (graphical-object! `(concat ,@(midpoint-decorations midpoints)))
) ;tm-define
