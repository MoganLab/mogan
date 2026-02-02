
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : format-geometry-edit.scm
;; DESCRIPTION : routines for resizing and repositioning
;; COPYRIGHT   : (C) 2010  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic format-geometry-edit)
  (:use (utils edit selections)
        (generic embedded-edit)
        (generic format-drd)
        (kernel gui kbd-handlers)
        (utils library length)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Customizable step changes for length modifications
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-preferences
  ("w increase" "0.05" noop)
  ("h increase" "0.05" noop)
  ("em increase" "0.1" noop)
  ("ex increase" "0.1" noop)
  ("spc increase" "0.2" noop)
  ("fn increase" "0.5" noop)
  ("mm increase" "0.5" noop)
  ("cm increase" "0.1" noop)
  ("inch increase" "0.05" noop)
  ("pt increase" "10" noop)
  ("msec increase" "50" noop)
  ("sec increase" "1" noop)
  ("min increase" "0.1" noop)
  ("% increase" "5" noop)
  ("default unit" "ex" noop))

(define step-table (make-ahash-table))
(define step-list
  '(0.005 0.01 0.02 0.05 0.1 0.2 0.5 1 2 5 10 20 50 100 200 500))
(define unit-list '("spc" "cm" "in" "em" "ex" "pt"))

(define (get-step unit)
  (when (not (ahash-ref step-table unit))
    (with pref (get-preference (string-append unit " increase"))
      (if pref (set! pref (string->number pref)))
      (with step (or pref 0.1)
	(ahash-set! step-table unit step))))
  (ahash-ref step-table unit))

(define (set-step unit step)
  (ahash-set! step-table unit step)
  (when (get-preference (string-append unit " increase"))
    (set-preference (string-append unit " increase") (number->string step))))

(define (change-step unit plus)
  (let* ((step (get-step unit))
	 (i (list-find-index step-list (lambda (x) (== x step))))
	 (j (and i (max 0 (min (+ i plus) (- (length step-list) 1)))))
	 (next (if j (list-ref step-list j) 0.1)))
    (set-step unit next)
    (set-message `(concat "Current step-size: " ,(number->string next) ,unit)
		 "Change step-size")))

(define (length-increase-step len inc)
  (with t (length-rightmost len)
    (when (tm-length? t)
      (change-step (tm-length-unit t) inc))))

(define (get-unit) (get-preference "default unit"))
(define (get-zero-unit) (string-append "0" (get-preference "default unit")))
(define (set-unit unit) (set-preference "default unit" unit))

(define (circulate-unit plus)
  (let* ((unit (get-unit))
	 (i (list-find-index unit-list (lambda (x) (== x unit))))
	 (j (and i (modulo (+ i plus) (length unit-list))))
	 (next (if j (list-ref unit-list j) "spc")))
    (set-unit next)
    (set-message `(concat "Default unit: " ,next)
		 "Change default unit")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Extra conversion routines for lengths
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tm-rich-length? t)
  (cond ((tm-atomic? t) #t)
	((and (tm-is? t 'minus) (not (tm-atomic? (tm-ref t :last)))) #f)
	((tm-in? t '(plus minus))
	 (list-and (map tm-rich-length? (tm-children t))))
	(else #f)))

(tm-define (tm->rich-length t)
  (cond ((tm-atomic? t) (tm->string t))
	((tm-is? t 'plus)
	 (with s (string-recompose (map tm->rich-length (tm-children t)) "+")
	   (string-replace s "+-" "-")))
	((tm-func? t 'minus 1)
	 (with s (string-append "-" (tm->rich-length (tm-ref t 0)))
	   (if (string-starts? s "--")
	       (substring s 2 (string-length s))
	       s)))
	((tm-is? t 'minus)
	 (with r `(plus ,@(cDr (tm-children t)) (minus ,(cAr (tm-children t))))
	   (tm->rich-length r)))
	(else "")))

(tm-define (rich-length->tm s)
  (with r (string-replace s "-" "+-")
    (with l (string-decompose r "+")
      (when (and (nnull? l) (== (car l) ""))
        (set! l (cdr l)))
      (if (<= (length l) 1) s
	  `(plus ,@l)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Useful subroutines for length manipulations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (length-increase t by)
  (cond ((tree-in? t '(plus minimum maximum))
         (length-increase (tree-ref t :last) by))
        ((tree-in? t '(minus))
         (length-increase (tree-ref t :last) (- by)))
        ((tm-length? t)
         (let* ((l (tree->string t))
                (v (tm-length-value l))
                (u (tm-length-unit l))
                (a (get-step u))
                (new-v-raw (+ v (* by a)))
                (new-v (/ (round (* new-v-raw 1000000)) 1000000.0))
                (new-l (tm-make-length new-v u)))
           (tree-set t new-l)))))

(tm-define (length-scale t sc step-mult)
  (cond ((tree-in? t '(plus minus minimum maximum))
         (for-each (cut length-scale <> sc) (tree-children)))
	((tm-length? t)
	 (let* ((l (tree->string t))
		(v (tm-length-value l))
		(u (tm-length-unit l))
		(a (* (get-step u) step-mult))
		(new-v (* (round (/ (* sc v) a)) a))
		(new-l (tm-make-length new-v u)))
           (when (> (* v new-v) 0.0)
             (tree-set t new-l))))))

(define (length-rightmost t)
  (cond ((tree-in? t '(plus minus minimum maximum))
	 (length-rightmost (tree-ref t :last)))
	(else t)))

(define (lengths-consistent? len1 len2)
  (let* ((t1 (length-rightmost len1))
	 (t2 (length-rightmost len2)))
    (and (tm-length? t1)
	 (tm-length? t2)
	 (== (tm-length-unit t1) (tm-length-unit t2)))))

(tm-define (replace-empty t i by)
  (when (tree-empty? (tree-ref t i))
    (tree-assign (tree-ref t i) by)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Rigid horizontal spaces
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (space-context? t)
  (tree-is? t 'space))

(tm-define (var-space-context? t)
  (or (tree-is? t 'space)
      (tree-func? t 'separating-space 1)
      (tree-func? t 'application-space 1)))

(define (space-make-ternary t)
  (cond ((== (tm-arity t) 1) (tree-insert t 1 '("0ex" "1ex")))
	((== (tm-arity t) 2) (tree-insert t 1 '("1ex")))))

(define (space-consistent? t)
  (and (== (tm-arity t) 3)
       (lengths-consistent? (tree-ref t 1) (tree-ref t 2))))

(tm-define (geometry-speed t inc?)
  (:require (var-space-context? t))
  (with inc (if inc? 1 -1)
    (with-focus-after t
      (length-increase-step (tree-ref t 0) inc))))

(tm-define (geometry-horizontal t forward?)
  (:require (var-space-context? t))
  (with inc (if forward? 1 -1)
    (with-focus-after t
      (length-increase (tree-ref t 0) inc))))

(tm-define (geometry-vertical t down?)
  (:require (space-context? t))
  (with inc (if down? -1 1)
    (with-focus-after t
      (space-make-ternary t)
      (length-increase (tree-ref t 2) inc))))

(tm-define (geometry-incremental t down?)
  (:require (space-context? t))
  (with inc (if down? -1 1)
    (with-focus-after t
      (space-make-ternary t)
      (when (space-consistent? t)
	(length-increase (tree-ref t 1) inc)
	(length-increase (tree-ref t 2) inc)))))

(tm-define (geometry-scale t scale*)
  (:require (var-space-context? t))
  (when pinch-modified? (undo 0))
  (let* ((old (tree->stree t))
         (scale (sqrt (+ (abs scale*) 0.000001)))
         (mult (if (== (tree-arity t) 1) 1.0 0.001)))
    (for-each (cut length-scale <> scale mult) (tree-children t))
    (set! pinch-modified? (!= (tree->stree t) old))
    (tree-go-to t :end)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Rubber horizontal spaces
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (hspace-context? t)
  (tree-is? t 'hspace))

(define (rubber-space-consistent? t)
  (or (== (tm-arity t) 1)
      (and (== (tm-arity t) 3)
	   (lengths-consistent? (tree-ref t 0) (tree-ref t 1))
	   (lengths-consistent? (tree-ref t 1) (tree-ref t 2)))))

(define (rubber-space-increase t by)
  (when (rubber-space-consistent? t)
    (length-increase (tree-ref t 0) by)
    (when (== (tm-arity t) 3)
      (length-increase (tree-ref t 1) by)
      (length-increase (tree-ref t 2) by))))

(tm-define (geometry-speed t inc?)
  (:require (hspace-context? t))
  (with inc (if inc? 1 -1)
    (with-focus-after t
      (length-increase-step (tree-ref t 0) inc))))

(tm-define (geometry-horizontal t forward?)
  (:require (hspace-context? t))
  (with inc (if forward? 1 -1)
    (with-focus-after t
      (rubber-space-increase t inc))))

(tm-define (geometry-scale t scale*)
  (:require (hspace-context? t))
  (when pinch-modified? (undo 0))
  (let* ((old (tree->stree t))
         (scale (sqrt (+ (abs scale*) 0.000001)))
         (mult (if (== (tree-arity t) 1) 1.0 0.001)))
    (for-each (cut length-scale <> scale mult) (tree-children t))
    (set! pinch-modified? (!= (tree->stree t) old))
    (tree-go-to t :end)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Vertical spaces
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (vspace-context? t)
  (tree-in? t '(vspace vspace*)))

(tm-define (geometry-speed t inc?)
  (:require (vspace-context? t))
  (with inc (if inc? 1 -1)
    (with-focus-after t
      (length-increase-step (tree-ref t 0) inc))))

(tm-define (geometry-vertical t down?)
  (:require (vspace-context? t))
  (with inc (if down? 1 -1)
    (with-focus-after t
      (rubber-space-increase t inc))))

(tm-define (geometry-scale t scale*)
  (:require (vspace-context? t))
  (when pinch-modified? (undo 0))
  (let* ((old (tree->stree t))
         (scale (sqrt (+ (abs scale*) 0.000001))))
    (for-each (cut length-scale <> scale 0.2) (tree-children t))
    (set! pinch-modified? (!= (tree->stree t) old))
    (tree-go-to t :end)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Vertical adjustments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (vadjust-context? t)
  (tree-in? t (reduce-by-tag-list)))

(tm-define (geometry-speed t inc?)
  (:require (vadjust-context? t))
  (with inc (if inc? 1 -1)
    (length-increase-step (tree-ref t 1) inc)))

(tm-define (geometry-vertical t down?)
  (:require (vadjust-context? t))
  (with inc (if down? 1 -1)
    (length-increase (tree-ref t 1) inc)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Move and shift
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (move-context? t)
  (tree-in? t (move-tag-list)))

(define (set-adjust-message s c)
  (let* ((l (kbd-find-inv-system-binding '(geometry-left)))
	 (r (kbd-find-inv-system-binding '(geometry-right))))
    (if (and l r)
	(set-message (string-append s " using " l ", " r ", etc. or "
				    "via the fields in the focus bar") c)
	(set-message (string-append s " using the keyboard or "
				    "via the fields in the focus bar") c))))

(tm-define (make-move hor ver)
  (:argument hor "Horizontal")
  (:argument ver "Vertical")
  (wrap-selection-small
    (insert-go-to `(move "" ,hor ,ver) '(0 0))
    (set-adjust-message "Adjust position" "move")))

(tm-define (make-shift hor ver)
  (:argument hor "Horizontal")
  (:argument ver "Vertical")
  (wrap-selection-small
    (insert-go-to `(shift "" ,hor ,ver) '(0 0))
    (set-adjust-message "Adjust position" "shift")))

(tm-define (geometry-speed t inc?)
  (:require (move-context? t))
  (with inc (if inc? 1 -1)
    (with-focus-after t
      (length-increase-step (tree-ref t 1) inc)
      (when (not (lengths-consistent? (tree-ref t 1) (tree-ref t 2)))
        (length-increase-step (tree-ref t 2) inc)))))

(tm-define (geometry-variant t forward?)
  (:require (move-context? t))
  (circulate-unit (if forward? 1 -1)))

(tm-define (geometry-horizontal t forward?)
  (:require (move-context? t))
  (with inc (if forward? 1 -1)
    (with-focus-after t
      (replace-empty t 1 (get-zero-unit))
      (length-increase (tree-ref t 1) inc))))

(tm-define (geometry-vertical t down?)
  (:require (move-context? t))
  (with inc (if down? -1 1)
    (with-focus-after t
      (replace-empty t 2 (get-zero-unit))
      (length-increase (tree-ref t 2) inc))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Resize and clipped
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (resize-context? t)
  (tree-in? t (resize-tag-list)))

(tm-define (make-resize l b r t)
  (:argument l "Left")
  (:argument b "Bottom")
  (:argument r "Right")
  (:argument t "Top")
  (wrap-selection-small
    (insert-go-to `(resize "" ,l ,b ,r ,t) '(0 0))
    (set-adjust-message "Adjust extents" "resize")))

(tm-define (make-extend l b r t)
  (:argument l "Left")
  (:argument b "Bottom")
  (:argument r "Right")
  (:argument t "Top")
  (wrap-selection-small
    (insert-go-to `(extend "" ,l ,b ,r ,t) '(0 0))
    (set-adjust-message "Adjust extension" "extend")))

(tm-define (make-clipped l b r t)
  (:argument l "Left")
  (:argument b "Bottom")
  (:argument r "Right")
  (:argument t "Top")
  (wrap-selection-small
    (insert-go-to `(clipped "" ,l ,b ,r ,t) '(0 0))
    (set-adjust-message "Adjust clipping" "clipped")))

(tm-define (make-reduce-by by)
  (:argument by "Reduce by")
  (wrap-selection-small
    (insert-go-to `(reduce-by "" ,by) '(0 0))
    (set-adjust-message "Reduce vertical size" "reduce-by")))

(define (replace-empty-horizontal t)
  (replace-empty t 1 `(plus "1l" ,(get-zero-unit)))
  (replace-empty t 3 `(plus "1r" ,(get-zero-unit))))

(define (replace-empty-vertical t)
  (replace-empty t 2 `(plus "1b" ,(get-zero-unit)))
  (replace-empty t 4 `(plus "1t" ,(get-zero-unit))))

(define (resize-consistent-horizontal? t)
  (replace-empty-horizontal t)
  (lengths-consistent? (tree-ref t 1) (tree-ref t 3)))

(define (resize-consistent-vertical? t)
  (replace-empty-vertical t)
  (lengths-consistent? (tree-ref t 2) (tree-ref t 4)))

(tm-define (geometry-speed t inc?)
  (:require (resize-context? t))
  (with inc (if inc? -1 1)
    (with-focus-after t
      (length-increase-step (tree-ref t 3) inc)
      (when (not (lengths-consistent? (tree-ref t 3) (tree-ref t 4)))
        (length-increase-step (tree-ref t 3) inc)))))

(tm-define (geometry-variant t forward?)
  (:require (resize-context? t))
  (circulate-unit (if forward? 1 -1)))

(tm-define (geometry-horizontal t forward?)
  (:require (resize-context? t))
  (with inc (if forward? 1 -1)
    (with-focus-after t
      (replace-empty-horizontal t)
      (length-increase (tree-ref t 3) inc))))

(tm-define (geometry-vertical t down?)
  (:require (resize-context? t))
  (with inc (if down? -1 1)
    (with-focus-after t
      (replace-empty-vertical t)
      (length-increase (tree-ref t 4) inc))))

(tm-define (geometry-extremal t forward?)
  (:require (resize-context? t))
  (with inc (if forward? 1 -1)
    (with-focus-after t
      (when (resize-consistent-horizontal? t)
        (length-increase (tree-ref t 1) inc)
        (length-increase (tree-ref t 3) inc)))))

(tm-define (geometry-incremental t down?)
  (:require (resize-context? t))
  (with inc (if down? -1 1)
    (with-focus-after t
      (when (resize-consistent-vertical? t)
        (length-increase (tree-ref t 2) inc)
        (length-increase (tree-ref t 4) inc)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Images
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (geometry-speed t inc?)
  (:require (image-context? t))
  (with inc (if inc? 1 -1)
    (with-focus-after t
      (length-increase-step (tree-ref t 0) inc))))

(tm-define (geometry-horizontal t forward?)
  (:require (image-context? t))
  (with inc (if forward? 1 -1)
    (with-focus-after t
      (replace-empty t 1 "1w")
      (length-increase (tree-ref t 1) inc))))

(tm-define (geometry-vertical t down?)
  (:require (image-context? t))
  (with inc (if down? -1 1)
    (with-focus-after t
      (replace-empty t 2 "1h")
      (length-increase (tree-ref t 2) inc))))

(tm-define (geometry-incremental t down?)
  (:require (image-context? t))
  (with inc (if down? -1 1)
    (with-focus-after t
      (replace-empty t 4 "0h")
      (length-increase (tree-ref t 4) inc))))

(tm-define (geometry-scale t scale*)
  (:require (image-context? t))
  (when pinch-modified? (undo 0))
  (let* ((old (tree->stree t))
         (scale (sqrt (+ (abs scale*) 0.000001)))
         (e1? (tree-empty? (tree-ref t 1)))
         (e2? (tree-empty? (tree-ref t 2)))
         (mult (if (or e1? e2?) 0.1 0.001)))
    (length-scale (tree-ref t 1) scale mult)
    (length-scale (tree-ref t 2) scale mult)
    (set! pinch-modified? (!= (tree->stree t) old))
    (tree-go-to t :end)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Image mouse dragging for resizing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define image-resize-handle #f)
(define image-resize-start-x #f)
(define image-resize-start-y #f)
(define image-resize-orig-w #f)
(define image-resize-orig-h #f)
(define image-resize-mark #f)

(define (image-get-bbox t)
  ;; 获取图片节点的边界框坐标列表 (x1 y1 x2 y2)
  (and-with rect (tree-bounding-rectangle t)
    (and (== (length rect) 4) rect)))

(define (image-point-on-handle? t)
  ;; 检测鼠标是否点在图片的8个handles上
  (and-with bbox (image-get-bbox t)
    (let* ((mpos (get-mouse-position))
           (mx (car mpos)) (my (cadr mpos))
           (x1 (car bbox)) (y1 (cadr bbox))
           (x2 (caddr bbox)) (y2 (cadddr bbox))
           (midx (/ (+ x1 x2) 2)) (midy (/ (+ y1 y2) 2))
           (hs (px->tmpt 10))
           (near? (lambda (a b) (< (abs (- a b)) hs)))
           (handles `((nw ,x1 ,y2)
                      (n  ,midx ,y2)
                      (ne ,x2 ,y2)
                      (e  ,x2 ,midy)
                      (se ,x2 ,y1)
                      (s  ,midx ,y1)
                      (sw ,x1 ,y1)
                      (w  ,x1 ,midy))))
      (let loop ((hspec handles))
        (if (null? hspec) #f
            (let* ((h (car hspec)) (hx (cadr h)) (hy (caddr h)))
              (if (and (near? mx hx) (near? my hy)) (car h) (loop (cdr hspec)))))))))

(define (image-get-dimensions t)
  ;; 获取图片现在的宽高属性，返回值为 (width height) 列表，单位为 tmpt
  (let* ((w-str (tm->string (tree-ref t 1)))
         (h-str (tm->string (tree-ref t 2)))
         (w (and w-str (not (string-null? w-str)) (length-decode w-str)))
         (h (and h-str (not (string-null? h-str)) (length-decode h-str))))
    (or (and w h (list w h))
        (and-with bbox (image-get-bbox t)
          (list (- (caddr bbox) (car bbox)) (- (cadddr bbox) (cadr bbox)))))))

(define (px->tmpt v) (* v 256.0))
(define (cm->tmpt v) (* v 60472.0))
(define (tmpt->cm v) (/ v 60472.0))
(define (cm->str v) (string-append (number->string v) "cm"))

(define (image-apply-resize t handle dx dy)
  ;; 拖拽 handles 时偏移值计算的重要函数
  ;; uniform-scale 用于处理四个角 handles 拖拽时的等比例缩放
  ;; 四条边 handles 拖拽时，非等比例缩放
  (when (and image-resize-orig-w image-resize-orig-h)
    (let* ((ow (tmpt->cm image-resize-orig-w))
           (oh (tmpt->cm image-resize-orig-h))
           (sx (tmpt->cm dx)) (sy (tmpt->cm dy))
           (nw (- ow sx)) (nh (- oh sy))
           (uniform-scale
            (lambda (scale-x scale-y)
              (let* ((scale (if (or (> scale-x 1) (> scale-y 1))
                                (max scale-x scale-y)
                                (min scale-x scale-y))))
                  (let* ((nw (* ow scale))
                         (nh (* oh scale)))
                    (when (> nw 0.1) (tree-set! t 1 (cm->str nw)))
                    (when (> nh 0.1) (tree-set! t 2 (cm->str nh)))
                    (refresh-window))))))
      (case handle
        ((se) (uniform-scale (/ (+ ow sx) ow) (/ (- oh sy) oh)))
        ((sw) (uniform-scale (/ nw ow) (/ (- oh sy) oh)))
        ((ne) (uniform-scale (/ (+ ow sx) ow) (/ nh oh)))
        ((nw) (uniform-scale (/ nw ow) (/ nh oh)))
        ((e)  (when (> (+ ow sx) 0.1) (tree-set! t 1 (cm->str (+ ow sx))) (refresh-window)))
        ((w)  (when (> nw 0.1) (tree-set! t 1 (cm->str nw)) (refresh-window)))
        ((n)  (when (> nh 0.1) (tree-set! t 2 (cm->str nh)) (refresh-window)))
        ((s)  (when (> (- oh sy) 0.1) (tree-set! t 2 (cm->str (- oh sy))) (refresh-window)))))))

(define (image-reset-drag-state!)
  ;; 重置拖拽变量和编辑会话标记的值，以便下一次重新使用
  (when image-resize-mark
    (mark-cancel image-resize-mark)
    (set! image-resize-mark #f))
  (set! image-resize-handle #f)
  (set! image-resize-start-x #f)
  (set! image-resize-start-y #f)
  (set! image-resize-orig-w #f)
  (set! image-resize-orig-h #f))

(tm-define (mouse-event key x y mods time data)
  ;; 重载 mouse-event 鼠标事件触发
  ;; 仅在鼠标拖拽启动/结束时触发图片大小调整
  (:require (and (tree-innermost image-context? #t)
                 (in? key '("start-drag-left" "dragging-left" "end-drag-left"))))
  (and-with t (tree-innermost image-context? #t)
    (cond
      ((== key "start-drag-left")
       (image-reset-drag-state!)
       (let ((handle (image-point-on-handle? t)))
         (when handle
           (let ((dims (image-get-dimensions t)))
             (set! image-resize-handle handle)
             (set! image-resize-start-x x)
             (set! image-resize-start-y y)
             (set! image-resize-orig-w (if dims (car dims) (cm->tmpt 1)))
             (set! image-resize-orig-h (if dims (cadr dims) (cm->tmpt 1)))
             (set! image-resize-mark (mark-new))
             (mark-start image-resize-mark))))
       (former key x y mods time data))
      ((== key "dragging-left")
       (if image-resize-handle
           (when (and image-resize-start-x image-resize-start-y)
             (image-apply-resize t image-resize-handle
                                  (- x image-resize-start-x) (- y image-resize-start-y)))
           (former key x y mods time data)))
      ((== key "end-drag-left")
       (when image-resize-mark
         (mark-end image-resize-mark)
         (set! image-resize-mark #f))
       (image-reset-drag-state!)
       (former key x y mods time data)))))
