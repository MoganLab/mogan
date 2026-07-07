
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-markup.scm
;; DESCRIPTION : extra graphical macros
;; COPYRIGHT   : (C) 2012  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (graphics graphics-markup)
  (:use (graphics graphics-drd) (graphics graphics-utils))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Definition of graphical macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (ca*r x)
  (if (pair? x) (ca*r (car x)) x)
) ;define

(tm-define-macro (define-graphics head . l)
  (receive (opts body)
    (list-break l not-define-option?)
    `(begin
       (set! gr-tags-user (cons (quote ,(ca*r head)) gr-tags-user))
       (tm-define ,head ,@opts (:secure #t) ,@body))
  ) ;receive
) ;tm-define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Useful subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tm-point? p) (tm-func? p 'point 2))
(tm-define (tm-x p) (tm-ref p 0))
(tm-define (tm-y p) (tm-ref p 1))

(tm-define (tm->number t) (if (tm-atomic? t) (string->number (tm->string t)) 0))

(tm-define (number->tm x) (number->string x))

(tm-define (point->complex p)
  (make-rectangular (tm->number (tm-x p)) (tm->number (tm-y p)))
) ;tm-define

(tm-define (complex->point z)
  `(point ,(number->tm (real-part z)) ,(number->tm (imag-part z)))
) ;tm-define

(tm-define (graphics-transform fun g)
  (cond ((tm-point? g) (fun g))
        ((tm-atomic? g) g)
        (else (cons (tm-car g) (map (cut graphics-transform fun <>) (tm-children g))))
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-graphics (rectangle P1 P2)
  (let* ((p1 (if (tm-point? P1) P1 '(point "0" "0"))) (p2 (if (tm-point? P2) P2 p1)))
    `(cline ,p1 (point ,(tm-x p2) ,(tm-y p1)) ,p2 (point ,(tm-x p1) ,(tm-y p2)))
  ) ;let*
) ;define-graphics

(define-graphics (circle C P)
  (let* ((c (if (tm-point? C) C '(point "0" "0")))
         (p (if (tm-point? P) P c))
         (cx (tm-x c))
         (cy (tm-y c))
         (px (tm-x p))
         (py (tm-y p))
         (dx `(minus ,px ,cx))
         (dy `(minus ,py ,cy))
         (q1 `(point (minus ,cx ,dx) (minus ,cy ,dy)))
         (q2 `(point (minus ,cx ,dy) (plus ,cy ,dx)))
        ) ;
    `(carc ,p ,q1 ,q2)
  ) ;let*
) ;define-graphics


;; C is the center of the circle
;; P is a point on the circle, and P is also the starting point of the arc.
;; The point where Q and C intersect the circle is called X. X is the end point of the arc.
;; Then we will find a point M, M is in the middle of arc P to X.
;; r is the radius
;; Enter three points. The first is the center of the circle and the second is the starting point of the arc. Draw a straight line through the third point and the center of the circle. The intersection of the straight line and the circle is the end point of the arc. Use clockwise method.
;; when clockwise param is >, we got the clockwise arc. If it is <, we got the counterclockwise arc.

(define (std-arc-helper C P Q clockwise)
  (let* ((c (if (tm-point? C) (tree->stree C) '(point "0" "0")))
         (p (if (tm-point? P) (tree->stree P) c))
         (q (if (tm-point? Q) (tree->stree Q) p))
         (r (points-distance c p))
         (r1 (points-distance c q))
         (x (if (zero? r)
              c
              (if (zero? r1)
                p
                (points-add (point-times (point-get-unit (points-sub q c)) r) c)
              ) ;if
            ) ;if
         ) ;x
         (mid-p-x (points-mid p x))
         (vec-c-p (points-sub p c))
         (vec-c-q (points-sub q c))
         (m (if (or (zero? r) (zero? r1))
              x
              (if (clockwise (points-cross-product-k vec-c-p vec-c-q) 0)
                (points-add (point-times (point-get-unit (points-sub mid-p-x c)) (- r)) c)
                (if (= (points-cross-product-k vec-c-p vec-c-q) 0)
                  ;; If cross product == 0, then the angle between vec-c-p and vec-c-q is 0 or 180.
                  ;; We should find out whether it's 0 or 180.
                  ;; And we should determine whether it's clockwise or counterclockwise.
                  (if (equal? (point-get-unit vec-c-p) (point-get-unit vec-c-q))
                    x
                    (if (eq? clockwise >)
                      (points-add (point-rotate-90 (point-rotate-90 (point-rotate-90 vec-c-p))) c)
                      (points-add (point-rotate-90 vec-c-p) c)
                    ) ;if
                  ) ;if
                  (points-add (point-times (point-get-unit (points-sub mid-p-x c)) r) c)
                ) ;if
              ) ;if
            ) ;if
         ) ;m
        ) ;
    `(arc ,p ,m ,x)
  ) ;let*
) ;define


(define-graphics (std-arc C P Q) (std-arc-helper C P Q >))


(define-graphics (std-arc-counterclockwise C P Q) (std-arc-helper C P Q <))


(define-graphics (sector-helper C P Q clockwise)
  (let* ((c (if (tm-point? C) (tree->stree C) '(point "0" "0")))
         (p (if (tm-point? P) (tree->stree P) c))
         (q (if (tm-point? Q) (tree->stree Q) p))
         (r (points-distance c p))
         (x (if (equal? r 0.0)
              c
              (points-add (point-times (point-get-unit (points-sub q c)) r) c)
            ) ;if
         ) ;x
         (mid-p-x (points-mid p x))
         (vec-c-p (points-sub p c))
         (vec-c-q (points-sub q c))
         (m (if (equal? r 0.0)
              c
              (if (clockwise (points-cross-product-k vec-c-p vec-c-q) 0)
                (points-add (point-times (point-get-unit (points-sub mid-p-x c)) (- r)) c)
                (if (= (points-cross-product-k vec-c-p vec-c-q) 0)
                  x
                  (points-add (point-times (point-get-unit (points-sub mid-p-x c)) r) c)
                ) ;if
              ) ;if
            ) ;if
         ) ;m
        ) ;
    (if (clockwise (points-cross-product-k vec-c-p vec-c-q) 0)
      (if (eq? clockwise >)
        `(superpose (std-arc ,c ,p ,m)
           (std-arc ,c ,m ,x)
           (with ,"color" ,"none" (line ,p ,c ,x ,m ,p)))
        `(superpose (std-arc-counterclockwise ,c ,p ,m)
           (std-arc-counterclockwise ,c ,m ,x)
           (with ,"color" ,"none" (line ,p ,c ,x ,m ,p)))
      ) ;if
      `(superpose (with ,"color" ,"none" (line ,p ,c ,x)) (arc ,p ,m ,x))
    ) ;if
  ) ;let*
) ;define-graphics

(define-graphics (sector C P Q) (sector-helper C P Q >))

(define-graphics (sector-counterclockwise C P Q) (sector-helper C P Q <))

(define-graphics (three-points-circle P1 P2 P3)
  (let* ((p1 (if (tm-point? P1) P1 '(point "0" "0")))
         (p2 (if (tm-point? P2) P2 p1))
         (p3 (if (tm-point? P3) P3 p2))
        ) ;
    `(carc ,p1 ,p2 ,p3)
  ) ;let*
) ;define-graphics

(define-graphics (three-points-arc P1 P2 P3)
  (let* ((p1 (if (tm-point? P1) P1 '(point "0" "0")))
         (p2 (if (tm-point? P2) P2 p1))
         (p3 (if (tm-point? P3) P3 p2))
        ) ;
    `(arc ,p1 ,p2 ,p3)
  ) ;let*
) ;define-graphics

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Electrical diagrams
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define ((rescale z0 dz) p)
  (complex->point (+ z0 (* dz (point->complex p))))
) ;define

(tm-define (electrical im scale p1 p2 p3)
  (let* ((z1 (if (tm-point? p1) (point->complex p1) 0))
         (z2 (if (tm-point? p2) (point->complex p2) z1))
         (z3 (if (tm-point? p3) (point->complex p3) z2))
         (dz (- z2 z1))
         (l (magnitude dz))
         (d1 (if (= dz 0) 0 (abs (* l (imag-part (/ (- z3 z1) dz))))))
         (d2 (/ (min l (/ d1 scale)) 2))
         (u (if (= dz 0) 0 (* d2 (/ dz l))))
         (vm (/ (+ z1 z2) 2))
         (v1 (- vm u))
         (v2 (+ vm u))
         (rescaler (rescale v1 (- v2 v1)))
        ) ;
    `(superpose (line ,p1 ,(complex->point v1))
       ,(graphics-transform rescaler im)
       (line ,(complex->point v2) ,p2)
       (with ,"point-style" ,"none" ,p3))
  ) ;let*
) ;tm-define

(define (std-condensator)
  '(superpose (line (point "0" "-2") (point "0" "2"))
     (line (point "1" "-2") (point "1" "2")))
) ;define

(define-graphics (condensator p1 p2 p3)
  (electrical (std-condensator) 2 p1 p2 p3)
) ;define-graphics

(define (std-diode)
  '(superpose (cline (point "0" "-0.5") (point "1" "0") (point "0" "0.5"))
     (line (point "1" "-0.5") (point "1" "0.5")))
) ;define

(define-graphics (diode p1 p2 p3) (electrical (std-diode) 0.5 p1 p2 p3))

(define (std-battery)
  '(superpose (line (point "0" "-2") (point "0" "2"))
     (line (point "0.333" "-1") (point "0.333" "1"))
     (line (point "0.666" "-2") (point "0.666" "2"))
     (line (point "1" "-1") (point "1" "1")))
) ;define

(define-graphics (battery p1 p2 p3) (electrical (std-battery) 1.5 p1 p2 p3))

(define (std-resistor)
  '(line (point "0" "0.0")
     (point "0.10" "0.0")
     (point "0.17" "0.13")
     (point "0.302" "-0.13")
     (point "0.434" "0.13")
     (point "0.566" "-0.13")
     (point "0.698" "0.13")
     (point "0.83" "-0.13")
     (point "0.90" "0.0")
     (point "1" "0.0"))
) ;define

(define-graphics (resistor p1 p2 p3) (electrical (std-resistor) 0.2 p1 p2 p3))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Triangle with text inside
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-group graphical-contains-curve-tag arrow-with-text arrow-with-text*)

(define-group graphical-contains-text-tag triangle-with-text)

(tm-define (graphics-incomplete? obj)
  (:require (tm-is? obj 'triangle-with-text))
  ;; (display* "incomplete? " obj " -> " (< (tm-arity obj) 3) "\n")
  (< (tm-arity obj) 3)
) ;tm-define

(tm-define (graphics-complete? obj)
  (:require (tm-is? obj 'triangle-with-text))
  ;; (display* "complete? " obj " -> " (>= (tm-arity obj) 3) "\n")
  (>= (tm-arity obj) 3)
) ;tm-define

(tm-define (graphics-complete obj)
  (:require (tm-is? obj 'triangle-with-text))
  (if (> (tm-arity obj) 3)
    (list obj #f)
    (list (append obj (list '(text-at "X"))) (list 3 2 0))
  ) ;if
) ;tm-define

(define-graphics (triangle-with-text P1 P2 P3 T)
  ;; (display* "twt " P1 ", " P2 ", " P3 ", " T "\n")
  (let* ((p1 (if (tm-point? P1) P1 '(point "0" "0")))
         (p2 (if (tm-point? P2) P2 p1))
         (p3 (if (tm-point? P3) P3 p2))
         (t (if (tm-is? T 'text-at) T '(text-at "X")))
         (z1 (point->complex p1))
         (z2 (point->complex p2))
         (z3 (point->complex p3))
         (p (complex->point (/ (+ z1 z2 z3) 3)))
        ) ;
    `(superpose (cline ,p1 ,p2 ,p3)
       (with ,"text-at-halign"
         ,"center"
         ,"text-at-valign"
         ,"center"
         (text-at ,(tm-ref t 0) ,p)))
  ) ;let*
) ;define-graphics

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Arrow or line with text
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-graphics (arrow-with-text P1 P2 T) (arrow-with-text-sub P1 P2 T 1.0))

(define-graphics (arrow-with-text* P1 P2 T) (arrow-with-text-sub P1 P2 T -1.0))

(define-group graphical-contains-curve-tag arrow-with-text arrow-with-text*)

(define-group graphical-contains-text-tag arrow-with-text arrow-with-text*)

(define-group variant-tag (arrow-with-text-tag))

(define-group arrow-with-text-tag arrow-with-text arrow-with-text*)

(tm-define (graphics-incomplete? obj)
  (:require (tm-in? obj '(arrow-with-text arrow-with-text*)))
  (< (tm-arity obj) 2)
) ;tm-define

(tm-define (graphics-complete? obj)
  (:require (tm-in? obj '(arrow-with-text arrow-with-text*)))
  (>= (tm-arity obj) 2)
) ;tm-define

(tm-define (graphics-complete obj)
  (:require (tm-in? obj '(arrow-with-text arrow-with-text*)))
  (if (> (tm-arity obj) 2)
    (list obj #f)
    (list (append obj (list '(math-at "x"))) (list 2 2 0))
  ) ;if
) ;tm-define

(define (directional-halign u)
  (cond ((> (real-part u) (* 0.333 (abs (imag-part u)))) "left")
        ((< (real-part u) (* -0.333 (abs (imag-part u)))) "right")
        (else "center")
  ) ;cond
) ;define

(define (directional-valign u)
  (cond ((> (imag-part u) (* 0.666 (abs (real-part u)))) "bottom")
        ((< (imag-part u) (* -0.666 (abs (real-part u)))) "top")
        (else "center")
  ) ;cond
) ;define

(define (arrow-with-text-sub P1 P2 T dir)
  ;; (display* "awt " P1 ", " P2 ", " T "\n")
  (let* ((p1 (if (tm-point? P1) P1 '(point "0" "0")))
         (p2 (if (tm-point? P2) P2 p1))
         (t (if (tm-is? T 'math-at) T '(math-at "x")))
         (z1 (point->complex p1))
         (z2 (point->complex p2))
         (m (/ (+ z1 z2) 2))
         (a (magnitude (- z2 z1)))
         (u (if (= a 0) a (/ (- z2 z1) a)))
         (n (* u (make-rectangular 0.0 (* 0.1 dir))))
         (c (+ m n))
         (ha (directional-halign n))
         (va (directional-valign n))
        ) ;
    `(superpose (line ,p1 ,p2)
       (with ,"text-at-halign"
         ,ha
         ,"text-at-valign"
         ,va
         (math-at (small ,(tm-ref t 0)) ,(complex->point c))))
  ) ;let*
) ;define

(tm-define (kbd-remove t forwards?)
  (:require (and (tree-in? t '(arrow-with-text arrow-with-text*))
              (tree-down t)
              (== (tree-index (tree-down t)) 2)
              (tree-func? (tree-down t) 'math-at 1)
              (tree-empty? (tree-ref t 2 0))
            ) ;and
  ) ;:require
  (tree-set t `(line ,(tree-ref t 0) ,(tree-ref t 1)))
) ;tm-define
