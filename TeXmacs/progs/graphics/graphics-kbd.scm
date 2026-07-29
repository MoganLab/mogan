
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-kbd.scm
;; DESCRIPTION : keyboard handling for graphics mode
;; COPYRIGHT   : (C) 2007  Joris van der Hoeven and Henri Lesourd
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (graphics graphics-kbd)
  (:use (generic generic-kbd)
    (utils library cursor)
    (graphics graphics-env)
    (graphics graphics-main)
    (graphics graphics-edit)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Various contexts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (in-active-graphics?)
  (and (in-graphics?) (== (get-env "preamble") "false"))
) ;define

(define (in-beamer-graphics?)
  (and (in-active-graphics?) (in-screens?))
) ;define

(define (graphics-context? t)
  (tree-is? t 'graphics)
) ;define

(define (inside-graphics-context? t)
  (tree-search-upwards t graphics-context?)
) ;define


(tm-define (generic-context? t) (:require (inside-graphics-context? t)) #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Extra abbreviations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define graphics-zoom-factor 1.189207115002721)
(tm-define (graphics-zoom-in) (graphics-zoom graphics-zoom-factor))
(tm-define (graphics-zoom-out) (graphics-zoom (/ 1.0 graphics-zoom-factor)))

(tm-define (graphics-move-origin-left) (graphics-move-origin "+0.01gw" "0gh"))
(tm-define (graphics-move-origin-right) (graphics-move-origin "-0.01gw" "0gh"))
(tm-define (graphics-move-origin-down) (graphics-move-origin "0gw" "+0.01gh"))
(tm-define (graphics-move-origin-up) (graphics-move-origin "0gw" "-0.01gh"))
(tm-define (graphics-move-origin-left-fast)
  (graphics-move-origin "+0.1gw" "0gh")
) ;tm-define
(tm-define (graphics-move-origin-right-fast)
  (graphics-move-origin "-0.1gw" "0gh")
) ;tm-define
(tm-define (graphics-move-origin-down-fast)
  (graphics-move-origin "0gw" "+0.1gh")
) ;tm-define
(tm-define (graphics-move-origin-up-fast) (graphics-move-origin "0gw" "-0.1gh"))

;; 方向键在 group-edit 且有选中对象时微移选中对象
;; 否则回退到平移整个坐标系
(tm-define (graphics-arrow-left)
  (or (graphics-should-nudge -0.1 0) (graphics-move-origin-left))
) ;tm-define
(tm-define (graphics-arrow-right)
  (or (graphics-should-nudge 0.1 0) (graphics-move-origin-right))
) ;tm-define
(tm-define (graphics-arrow-down)
  (or (graphics-should-nudge 0 -0.1) (graphics-move-origin-down))
) ;tm-define
(tm-define (graphics-arrow-up)
  (or (graphics-should-nudge 0 0.1) (graphics-move-origin-up))
) ;tm-define
(tm-define (graphics-arrow-left-fast)
  (or (graphics-should-nudge -1.0 0) (graphics-move-origin-left-fast))
) ;tm-define
(tm-define (graphics-arrow-right-fast)
  (or (graphics-should-nudge 1.0 0) (graphics-move-origin-right-fast))
) ;tm-define
(tm-define (graphics-arrow-down-fast)
  (or (graphics-should-nudge 0 -1.0) (graphics-move-origin-down-fast))
) ;tm-define
(tm-define (graphics-arrow-up-fast)
  (or (graphics-should-nudge 0 1.0) (graphics-move-origin-up-fast))
) ;tm-define

(tm-define (graphics-decrease-hsize) (graphics-change-extents "-0.1cm" "0cm"))
(tm-define (graphics-increase-hsize) (graphics-change-extents "+0.1cm" "0cm"))
(tm-define (graphics-decrease-vsize) (graphics-change-extents "0cm" "-0.1cm"))
(tm-define (graphics-increase-vsize) (graphics-change-extents "0cm" "+0.1cm"))
(tm-define (graphics-decrease-hsize-fast)
  (graphics-change-extents "-1cm" "0cm")
) ;tm-define
(tm-define (graphics-increase-hsize-fast)
  (graphics-change-extents "+1cm" "0cm")
) ;tm-define
(tm-define (graphics-decrease-vsize-fast)
  (graphics-change-extents "0cm" "-1cm")
) ;tm-define
(tm-define (graphics-increase-vsize-fast)
  (graphics-change-extents "0cm" "+1cm")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map (:mode in-active-graphics?)
 ("+" (graphics-zoom-in))
 ("=" (graphics-zoom-in))
 ("-" (graphics-zoom-out))
 ("A-1" (graphics-set-zoom 1))
 ("A-2" (graphics-set-zoom 0.5))
 ("A-3" (graphics-set-zoom 0.333333333333))
 ("A-4" (graphics-set-zoom 0.25))
 ("A-5" (graphics-set-zoom 0.2))
 ("A-6" (graphics-set-zoom 0.166666666666))
 ("A-7" (graphics-set-zoom 0.142857142857))
 ("A-8" (graphics-set-zoom 0.125))
 ("A-9" (graphics-set-zoom 0.111111111111))
 ("1" (graphics-set-mode '(edit point)))
 ("2" (graphics-set-mode '(edit line)))
 ("3" (graphics-set-mode '(edit cline)))
 ("4" (graphics-set-mode '(edit spline)))
 ("5" (graphics-set-mode '(edit cspline)))
 ("6" (graphics-set-mode '(edit std-arc-counterclockwise)))
 ("7" (graphics-set-mode '(edit circle)))
 ("c" (graphics-set-origin "0.5gw" "0.5gh"))
 ("t" (graphics-set-origin "0gw" "1gh"))
 ("l" (graphics-set-origin "0gw" "0.5gh"))
 ("b" (graphics-set-origin "0gw" "0gh"))
 ("e" (graphics-resume-last-insert))
 ("#" (graphics-toggle-grid))
 ("!" (open-plots-editor "scheme" "default" ""))
 ("left" (graphics-arrow-left))
 ("right" (graphics-arrow-right))
 ("down" (graphics-arrow-down))
 ("up" (graphics-arrow-up))
 ("S-left" (graphics-arrow-left-fast))
 ("S-right" (graphics-arrow-right-fast))
 ("S-down" (graphics-arrow-down-fast))
 ("S-up" (graphics-arrow-up-fast))
 ("home" (graphics-zmove 'foreground))
 ("end" (graphics-zmove 'background))
 ("pageup" (graphics-zmove 'closer))
 ("pagedown" (graphics-zmove 'farther))
 ("return" (graphics-apply-props-at-mouse))
 ("S-return" (graphics-get-props-at-mouse))
 ("A-left" (graphics-decrease-hsize))
 ("A-right" (graphics-increase-hsize))
 ("A-down" (graphics-increase-vsize))
 ("A-up" (graphics-decrease-vsize))
 ("A-S-left" (graphics-decrease-hsize-fast))
 ("A-S-right" (graphics-increase-hsize-fast))
 ("A-S-down" (graphics-increase-vsize-fast))
 ("A-S-up" (graphics-decrease-vsize-fast))
 ("backspace" (graphics-kbd-remove #f))
 ("delete" (graphics-kbd-remove #t))
 ("C-2" (graphics-set-grid-aspect 'detailed 2 #t))
 ("C-3" (graphics-set-grid-aspect 'detailed 3 #t))
 ("C-4" (graphics-set-grid-aspect 'detailed 4 #t))
 ("C-5" (graphics-set-grid-aspect 'detailed 5 #t))
 ("C-6" (graphics-set-grid-aspect 'detailed 6 #t))
 ("C-7" (graphics-set-grid-aspect 'detailed 7 #t))
 ("C-8" (graphics-set-grid-aspect 'detailed 8 #t))
 ("C-9" (graphics-set-grid-aspect 'detailed 9 #t))
 ("C-0" (graphics-set-grid-aspect 'detailed 10 #t))
 ("C-left" (graphics-rotate-xz -0.1))
 ("C-right" (graphics-rotate-xz 0.1))
 ("C-up" (graphics-rotate-yz 0.1))
 ("C-down" (graphics-rotate-yz -0.1))
 ("C-home" (graphics-zmove 'foreground))
 ("C-end" (graphics-zmove 'background))
 ("C-pageup" (graphics-zmove 'closer))
 ("C-pagedown" (graphics-zmove 'farther))
) ;kbd-map

(kbd-map (:mode in-beamer-graphics?)
 ("pageup" (screens-switch-to :previous))
 ("pagedown" (screens-switch-to :next))
) ;kbd-map

(define graphics-keys
  '("+"
    "="
    "-"
    "1"
    "2"
    "3"
    "4"
    "5"
    "6"
    "7"
    "8"
    "9"
    "0"
    "#"
    "!"
    "c"
    "l"
    "b"
    "t"
    "e"
    "left"
    "right"
    "down"
    "up"
    "home"
    "end"
    "pageup"
    "pagedown"
    "return"
    "backspace"
    "delete"
    "tab"
    "F1"
    "F2"
    "F3"
    "F4"
    "F9"
    "F10"
    "F11"
    "F12")
) ;define

(tm-define (keyboard-press key time)
  (:mode in-active-graphics?)
  (cond ((string-occurs? "-" key) (key-press key))
        ((in? key graphics-keys) (key-press key))
  ) ;cond
) ;tm-define

;; 按下 e：从更改属性模式切回上一次插入对象的插入模式
(tm-define (graphics-resume-last-insert)
  (:mode in-active-graphics?)
  (and-with last
    (graphics-last-inserted-type)
    (when (equal? (graphics-mode) '(group-edit edit-props))
      (graphics-set-mode last)
    ) ;when
  ) ;and-with
) ;tm-define

(tm-define (mouse-drop-event x y obj)
  (:mode in-active-graphics?)
  (set! the-graphics-drop-object (tm->stree obj))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Gestures in graphics mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define pinch-graphics-scale #f)

(tm-define (pinch-start)
  (:mode in-active-graphics?)
  (set! pinch-graphics-scale (graphics-get-zoom))
) ;tm-define

(tm-define (pinch-end)
  (:mode in-active-graphics?)
  (set! pinch-graphics-scale #f)
) ;tm-define

(tm-define (pinch-scale scale*)
  (:mode in-active-graphics?)
  (when (not pinch-graphics-scale)
    (pinch-start)
  ) ;when
  (let* ((old (graphics-get-zoom))
         (lg (/ (log scale*) (log 2.0)))
         (lg* (/ (round (* 24.0 lg)) 24.0))
         (scale (exp (* (log 2.0) lg*)))
        ) ;
    (graphics-set-zoom (* scale pinch-graphics-scale))
  ) ;let*
) ;tm-define

(tm-define (wheel-capture?) (:mode in-active-graphics?) #t)

(tm-define (graphics-wheel dx* dy*)
  (let* ((dx (/ (round (* (string->number dx*) 100.0)) 100.0))
         (dy (/ (round (* (string->number dy*) 100.0)) 100.0))
        ) ;
    (graphics-move-origin (string-append (number->string dx) "gw")
      (string-append (number->string dy) "gh")
    ) ;graphics-move-origin
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Overriding standard structured editing commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-variant t forwards?)
  (:require (in-active-graphics?))
  (graphics-choose-point (if forwards? 1 -1))
) ;tm-define

(tm-define (graphics-kbd-remove forward?)
  (cond ((and (with-active-selection?)
           (with-cursor (rcons (selection-path) 0) (not (in-graphics?)))
         ) ;and
         (go-to (rcons (selection-path) 0))
         (clipboard-cut "primary")
        ) ;
        ((inside-graphical-text?) (if forward? (kbd-delete) (kbd-backspace)))
        ((graphics-selection-active?) (remove-selected-objects))
        (else (edit_delete))
  ) ;cond
) ;tm-define

(tm-define (geometry-vertical t down?)
  (:require (in-active-graphics?))
  (graphics-change-geo-valign down?)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Text at
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-horizontal t forwards?)
  (:require (graphical-text-context? t))
  (with-define (move)
   ((if forwards? go-right go-left))
   (go-to-next-inside move
     (lambda (t2) (tree-search-upwards t2 (lambda (u) (equal? u t))))
   ) ;go-to-next-inside
  ) ;with-define
) ;tm-define

(tm-define (kbd-vertical t downwards?)
  (:require (graphical-text-context? t))
  (with-define (move)
   ((if downwards? go-down go-up))
   (go-to-next-inside move
     (lambda (t2) (tree-search-upwards t2 (lambda (u) (equal? u t))))
   ) ;go-to-next-inside
  ) ;with-define
) ;tm-define

(tm-define (kbd-extremal t forwards?)
  (:require (graphical-text-context? t))
  (and-with c (tree-down t) (tree-go-to c (if forwards? :end :start)))
) ;tm-define

(tm-define (geometry-horizontal t forwards?)
  (:require (graphical-text-context? t))
  (let* ((old (graphical-get-attribute t "text-at-halign"))
         (new (if forwards?
                (cond ((== old "right") "center")
                      (else "left")
                ) ;cond
                (cond ((== old "left") "center")
                      (else "right")
                ) ;cond
              ) ;if
         ) ;new
        ) ;
    (graphical-set-attribute t "text-at-halign" new)
  ) ;let*
) ;tm-define

(tm-define (geometry-vertical t down?)
  (:require (graphical-text-context? t))
  (let* ((valign-var (graphics-valign-var t))
         (old (graphical-get-attribute t valign-var))
         (new (if down?
                (cond ((== old "bottom") "base")
                      ((== old "base") "axis")
                      ((== old "axis") "center")
                      (else "top")
                ) ;cond
                (cond ((== old "top") "center")
                      ((== old "center") "axis")
                      ((== old "axis") "base")
                      (else "bottom")
                ) ;cond
              ) ;if
         ) ;new
        ) ;
    (graphical-set-attribute t valign-var new)
  ) ;let*
) ;tm-define

(tm-define (geometry-extremal t forwards?)
  (:require (graphical-text-context? t))
  (graphical-set-attribute t "text-at-halign" (if forwards? "left" "right"))
) ;tm-define

(tm-define (geometry-incremental t down?)
  (:require (graphical-text-context? t))
  (graphical-set-attribute t (graphics-valign-var t) (if down? "top" "bottom"))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Draw over / draw under
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map (:mode inside-graphical-over-under?)
 ("C-*" (graphics-toggle-over-under))
) ;kbd-map
