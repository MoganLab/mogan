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
  (:use (graphics graphics-env)
        (graphics graphics-utils)
  ) ;:use
) ;texmacs-module

(define ghost-lines '())

(tm-define (graphics-get-all-previous-points)
  (:state graphics-state)
  (if (and sticky-point (integer? current-point-no) (> current-point-no 0))
      (let* ((obj (stree-radical (car (sketch-get1))))
             (pts (cdr obj)))
        (let loop ((i 0) (res '()))
          (if (< i current-point-no)
              (let ((p (list-ref pts i)))
                (if (and (pair? p) (== (car p) 'point))
                    (loop (+ i 1) (cons (cdr p) res))
                    (loop (+ i 1) res)))
              (tm->tree (cons 'tuple (map (lambda (x) (cons 'tuple x)) (reverse res)))))))
      (tm->tree `(tuple))))

(tm-define (graphics-clear-ghost-lines)
  (:state graphics-state)
  (set! ghost-lines '())
)

(tm-define (graphics-add-ghost-line x y theta)
  (:state graphics-state)
  (set! ghost-lines (cons `((,x ,y) ,theta) ghost-lines))
  (graphics-decorations-update)
)

(tm-define (graphics-set-ghost-line x y theta)
  (:state graphics-state)
  (graphics-clear-ghost-lines)
)

(tm-define (graphics-get-decorations-ghost-line)
  (if (nnull? ghost-lines)
      (let loop ((lines ghost-lines) (res '()))
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
                   (y_end (number->string (+ y1 (* 50.0 dy)))))
              (loop (cdr lines)
                    (cons `(with "color" "green" (line (point ,x_start ,y_start) (point ,x_end ,y_end))) res)))))
      '()))
