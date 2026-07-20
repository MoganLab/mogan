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

(tm-define (graphics-get-all-previous-points)
  (:state graphics-state)
  (if (and sticky-point (integer? current-point-no) (> current-point-no 0))
    (let* ((obj (stree-radical (car (sketch-get1)))) (pts (cdr obj)))
      (let loop
        ((i 0) (res '()))
        (if (< i current-point-no)
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
        (map (lambda (e) (list (list (cadr e) (caddr e)) (cadddr e)))
          (cdr l)))
      (set! ghost-lines '()))
    (if (nnull? ghost-lines) (graphics-decorations-update)))
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
               (x_start (number->string (- x1 (* 12.0 dx))))
               (y_start (number->string (- y1 (* 12.0 dy))))
               (x_end (number->string (+ x1 (* 12.0 dx))))
               (y_end (number->string (+ y1 (* 12.0 dy))))
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
