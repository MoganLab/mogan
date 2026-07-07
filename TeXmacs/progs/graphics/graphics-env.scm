
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : graphics-env.scm
;; DESCRIPTION : routines for managing the graphical context
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;               (C) 2004-2007  Joris van der Hoeven and Henri Lesourd
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (graphics graphics-env) (:use (graphics graphics-utils)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; State variables & history for the current graphics context
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; NOTE: This section is OK
;; State variables
(define-state graphics-state
  (slots ((graphics-action #f)
          (the-current-cursor-style "normal")
          (current-graphical-object #f)
          (choosing #f)
          (sticky-point #f)
          (dragging-create? #f)
          (dragging-busy? #f)
          (leftclick-waiting #f)
          (current-point-no #f)
          (current-edge-sel? #f)
          (current-selection #f)
          (previous-selection #f)
          (subsel-no #f)
          (graphics-undo-enabled #t)
          (remove-undo-mark? #f)
          (the-sketch '())
          (selecting-x0 #f)
          (selecting-y0 #f)
          (multiselecting #f)
          (preselected #f)
          (layer-of-last-removed-object #f)
         ) ;
  ) ;slots
  (props ((current-x (f2s (get-graphical-x)))
          (current-y (f2s (get-graphical-y)))
          (sel (if sticky-point #f (select-choose (s2f current-x) (s2f current-y))))
          (pxy (if sel (car sel) '()))
          (current-path (if sticky-point (cDr (cursor-path)) (graphics-path pxy)))
          (current-obj (with gm
                         (graphics-group-mode? (graphics-mode))
                         (if gm
                           '(point)
                           (if sticky-point
                             (with o (graphical-object #t) (if (and (pair? o) (nnull? (cdr o))) (cadr o) #f))
                             (graphics-object pxy)
                           ) ;if
                         ) ;if
                       ) ;with
          ) ;current-obj
          (other-inits (if (not sticky-point)
                         (begin
                           (set! current-point-no (if sel (cAr (car sel)) #f))
                           (set! current-edge-sel? (and sel (== (length sel) 2)))
                         ) ;begin
                       ) ;if
          ) ;other-inits
         ) ;
  ) ;props
) ;define-state

;; State stack (1)
(tm-define state-slots `(quote ,(state-names graphics-state)))

(define-public-macro (state-len) `(length ,state-slots))

(tm-define (new-state) (make-vector (state-len)))

(define-public-macro (state-slot-ref i) `(- (length (memq ,i ,state-slots)) ,1))

(define-public-macro (state-ref st var) `(vector-ref ,st (state-slot-ref ,var)))

(define-public-macro (state-set! st var val)
  `(vector-set! ,st (state-slot-ref ,var) ,val)
) ;define-public-macro

(tm-define (graphics-state-get)
  (with st
    (new-state)
    (state-set! st 'graphics-action #f)
    (state-set! st 'current-graphical-object (get-graphical-object))
    (state-set! st 'choosing choosing)
    (state-set! st 'sticky-point sticky-point)
    (state-set! st 'dragging-create? dragging-create?)
    (state-set! st 'dragging-busy? dragging-busy?)
    (state-set! st 'leftclick-waiting leftclick-waiting)
    (state-set! st 'current-obj current-obj)
    (state-set! st 'current-point-no current-point-no)
    (state-set! st 'current-edge-sel? current-edge-sel?)
    (state-set! st 'current-selection current-selection)
    (state-set! st 'previous-selection previous-selection)
    (state-set! st 'subsel-no subsel-no)
    (state-set! st 'current-x current-x)
    (state-set! st 'current-y current-y)
    (state-set! st 'graphics-undo-enabled graphics-undo-enabled)
    (state-set! st 'remove-undo-mark? remove-undo-mark?)
    (state-set! st 'the-sketch (tm-copy the-sketch))
    (state-set! st 'selecting-x0 selecting-x0)
    (state-set! st 'selecting-y0 selecting-y0)
    (state-set! st 'multiselecting multiselecting)
    (state-set! st 'preselected preselected)
    (state-set! st 'current-path current-path)
    st
  ) ;with
) ;tm-define

(tm-define (graphics-state-set st)
  (with o
    (state-ref st 'current-graphical-object)
    (if (pair? (tree->stree o))
      (set-graphical-object o)
      (graphics-decorations-reset)
    ) ;if
  ) ;with
  (set! choosing (state-ref st 'choosing))
  (set! sticky-point (state-ref st 'sticky-point))
  (set! dragging-create? (state-ref st 'dragging-create?))
  (set! dragging-busy? (state-ref st 'dragging-busy?))
  (set! leftclick-waiting (state-ref st 'leftclick-waiting))
  (set! current-obj (state-ref st 'current-obj))
  (set! current-point-no (state-ref st 'current-point-no))
  (set! current-edge-sel? (state-ref st 'current-edge-sel?))
  (set! current-selection (state-ref st 'current-selection))
  (set! previous-selection (state-ref st 'previous-selection))
  (set! subsel-no (state-ref st 'subsel-no))
  (set! current-x (state-ref st 'current-x))
  (set! current-y (state-ref st 'current-y))
  (set! graphics-undo-enabled (state-ref st 'graphics-undo-enabled))
  (set! remove-undo-mark? (state-ref st 'remove-undo-mark?))
  (set! the-sketch (state-ref st 'the-sketch))
  (set! selecting-x0 (state-ref st 'selecting-x0))
  (set! selecting-y0 (state-ref st 'selecting-y0))
  (set! multiselecting (state-ref st 'multiselecting))
  (set! preselected (state-ref st 'preselected))
  (set! current-path (state-ref st 'current-path))
) ;tm-define

;; State stack (2)
(tm-define graphics-states '())

(tm-define (graphics-states-void?) (null? graphics-states))

(tm-define (graphics-state-first?)
  (and (not (graphics-states-void?)) (null? (cdr graphics-states)))
) ;tm-define

(tm-define (graphics-push-state st)
  (set! graphics-states (cons st graphics-states))
) ;tm-define

(tm-define (graphics-pop-state)
  (if (graphics-states-void?)
    (display* "(graphics-pop-state)::void(graphics-states)\n")
    (with st (car graphics-states) (set! graphics-states (cdr graphics-states)) st)
  ) ;if
) ;tm-define

;; User interface
(tm-define graphics-first-state #f)

(tm-define (graphics-store-first action)
  (if graphics-first-state
    (display* "(graphics-store-first)::!void(graphics-store-first)\n")
  ) ;if
  (set! graphics-first-state (graphics-state-get))
  (state-set! graphics-first-state 'graphics-action action)
) ;tm-define

(tm-define (graphics-back-first)
  (graphics-state-set graphics-first-state)
  (set! graphics-first-state #f)
) ;tm-define

(tm-define (graphics-store-state first?)
  (if first?
    (graphics-store-first first?)
    (graphics-push-state (graphics-state-get))
  ) ;if
) ;tm-define

(tm-define (graphics-back-state first?)
  (if first?
    (graphics-back-first)
    (if (graphics-state-first?)
      (undo 0)
      (with st
        (graphics-pop-state)
        (graphics-state-set st)
        (if (graphics-states-void?) (graphics-push-state st))
      ) ;with
    ) ;if
  ) ;if
) ;tm-define

(tm-define (graphics-set-default-state-except-sticky-point)
  (graphics-decorations-reset)
  (set! choosing #f)
  (set! dragging-create? #f)
  (set! dragging-busy? #f)
  (set! leftclick-waiting #f)
  (set! current-obj #f)
  (set! current-point-no #f)
  (set! current-edge-sel? #f)
  (set! current-selection #f)
  (set! previous-selection #f)
  (set! subsel-no #f)
  (set! current-x #f)
  (set! current-y #f)
  (set! graphics-undo-enabled #t)
  (set! remove-undo-mark? #f)
  (set! the-sketch '())
  (set! selecting-x0 #f)
  (set! selecting-y0 #f)
  (set! multiselecting #f)
  (set! preselected #f)
  (set! current-path #f)
  (set! layer-of-last-removed-object #f)
) ;tm-define

(tm-define (graphics-init-state)
  (graphics-set-default-state-except-sticky-point)
  (set! sticky-point #f)
) ;tm-define

(tm-define (graphics-reset-state)
  (graphics-set-default-state-except-sticky-point)
  (set-sticky-point-false)
) ;tm-define

(tm-define (graphics-forget-states)
  (set! graphics-first-state #f)
  (set! graphics-states '())
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for using and maintaining the current graphics context
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Graphical select
;; NOTE: This subsection is OK, except the hack for custom objects ;-(...

(define (filter-graphical-select l x y)
  (define (filter-path p)
    (define (cut-after-first-negative p)
      ;; NOTE: Belongs to the hack
      (with ref
        #f
        (foreach-cons (x p) (if (and (not ref) (< (car x) 0)) (set! ref x)))
        (if ref (set-cdr! ref '()))
      ) ;with
      p
    ) ;define
    (with p2
      (map string->number (cdr p))
      (set! p2 (cut-after-first-negative p2))
      ;; NOTE: Hack
      (if (graphics-path p2) p2 #f)
    ) ;with
  ) ;define
  (define (filter-sel e)
    (with e2
      (map filter-path (cdr e))
      ;; FIXME: Hack to workaround the bad results of (graphical-select) for
      ;;   custom objects (sends back paths with negative elements).
      (if (and (>= (length e2) 1) (pair? (car e2)) (< (cAr (car e2)) 0))
        (set-car! e2 (rcons (cDr (car e2)) 0))
      ) ;if
      (if (and (>= (length e2) 2) (pair? (cadr e2)) (< (cAr (cadr e2)) 0))
        (set-car! (cdr e2) (rcons (cDr (cadr e2)) 0))
      ) ;if
      (if (and (>= (length e2) 2) (== (car e2) (cadr e2))) (set-cdr! e2 '()))
      (if (car e2)
        (let* ((p (graphics-path (car e2))) (o (if p (path->tree p) #f)))
          (if (and (compound-tree? o)
                (eq? (tree-label o) 'with)
                (eq? (tree-label (tree-ref o (- (tree-arity o) 1))) 'graphics)
              ) ;and
            (begin
              (set! o #f)
              (set-car! e2 #f)
            ) ;begin
          ) ;if
          (if (and o (not (in? (tree-label o) gr-tags-all)))
            ;; Custom markup
            (set-car! e2
              (rcons p (if x (object-closest-point-pos (tree->stree o) (f2s x) (f2s y)) 0))
            ) ;set-car!
          ) ;if
        ) ;let*
      ) ;if
      ;; FIXME: End hack
      (if (== (car e2) #f) #f e2)
    ) ;with
  ) ;define
  (define (remove-filtered-elts l)
    (if (pair? l)
      (if (pair? (cdr l))
        (if (== (cadr l) #f)
          (begin
            (set-cdr! l (cddr l))
            (remove-filtered-elts l)
          ) ;begin
          (remove-filtered-elts (cdr l))
        ) ;if
        l
      ) ;if
      #f
    ) ;if
  ) ;define
  (with l2
    (cons 'tuple (map filter-sel (cdr l)))
    (remove-filtered-elts l2)
    (cdr l2)
  ) ;with
) ;define

(tm-define (graphics-select x y d)
  (with res
    (tree->stree (graphical-select x y))
    (filter-graphical-select res x y)
  ) ;with
) ;tm-define

(tm-define (graphics-select-area x1 y1 x2 y2)
  (define l '())
  (with res
    (tree->stree (graphical-select-area x1 y1 x2 y2))
    (set! res (filter-graphical-select res #f #f))
    (for (e res) (set! l (cons (graphics-path (car e)) l)))
    (reverse (list-remove-duplicates l))
  ) ;with
) ;tm-define

(tm-define (select-first x y)
  (with sel (graphics-select x y 15) (if (pair? sel) (car sel) #f))
) ;tm-define

(tm-define (select-choose x y)
  (with sel
    (graphics-select x y 15)
    (set! previous-selection current-selection)
    (set! current-selection sel)
    (if (or (null? sel) (!= current-selection previous-selection))
      (set! subsel-no 0)
    ) ;if
    (if (pair? sel) (car (list-tail sel subsel-no)) #f)
  ) ;with
) ;tm-define

(tm-define (select-next inc)
  (if (and current-selection subsel-no)
    (begin
      (set! subsel-no (+ subsel-no inc))
      (if (>= subsel-no (length current-selection)) (set! subsel-no 0))
      (if (< subsel-no 0) (set! subsel-no (- (length current-selection) 1)))
    ) ;begin
  ) ;if
) ;tm-define

;; Graphics X cursor
;; NOTE: This subsection is OK
(tm-define graphics-texmacs-pointer #f)
(tm-define (set-texmacs-pointer curs . cache)
  (define (set-pointer name)
    (if (symbol? name) (set! name (symbol->string name)))
    (set! graphics-texmacs-pointer name)
  ) ;define
  (if (null? cache) (set! graphics-texmacs-pointer #f))
  (if (not (string-symbol=? graphics-texmacs-pointer curs))
    (cond ((== curs 'none)
           (set-pointer 'none)
           (set-mouse-pointer
             ;; FIXME: This function is horribly slow, due to the
             ;;   non-correct (?) caching of the xmp files, or to
             ;;   the building of too much X11 datastructures.
             (tm_xpm "tm_cursor_none.xpm")
             (tm_xpm "tm_mask_none.xpm")
           ) ;set-mouse-pointer
          ) ;
          ((== curs 'text-arrow)
           (set-pointer 'none)
           (set-predef-mouse-pointer "XC_top_left_arrow")
          ) ;
          ((== curs 'graphics-cross)
           (set-texmacs-pointer 'none)
           (set-pointer 'graphics-cross)
          ) ;
          ((== curs 'graphics-cross-arrows)
           (set-texmacs-pointer 'none)
           (set-pointer 'graphics-cross-arrows)
          ) ;
          (else #t)
    ) ;cond
  ) ;if
) ;tm-define

;; Graphics context
;; NOTE: This subsection is not yet OK. We still need to :
;;  (1) remove (tmdefine) ;
;;  (2) review & simplify (graphics-reset-context) ;
(define-public-macro (with-graphics-state x y path obj no edge . body)
  `(with-state-by-name graphics-state
     (let* ((,path current-path)
            (,obj current-obj)
            (,no current-point-no)
            (,edge current-edge-sel?))
       unquote
       body))
) ;define-public-macro
;; Deprecated

(define (define-option-state opt decl)
  (with (fun head . body)
    decl
    `(,fun ,head (with-state-by-name ,(car opt) ,@body))
  ) ;with
) ;define

(ahash-set! define-option-table :state define-option-state)

(define (define-option-state-slots opt decl)
  (with (fun head . body)
    decl
    `(,fun ,head (with-state-slots-by-name ,(car opt) ,@body))
  ) ;with
) ;define

(ahash-set! define-option-table :state-slots define-option-state-slots)


;; Graphics context [reset]

(define current-cursor #f)

(define (tm_xpm name)
  (string-append (url->system (get-texmacs-path))
    "/misc/pixmaps/traditional/--x17/"
    name
  ) ;string-append
) ;define

(tm-define (graphics-reset-context cmd)
  ;; FIXME: Should be called only once, when we move out of a <graphics>.
  ;;  Should not be called all the time because of the case 'text-cursor.
  ;;  Find a way to test this more precisely inside the C++.
  (:state-slots graphics-state)
  ;; cmd in { begin, exit, undo }
  ;; (display* "Graphics] Reset-context " cmd "\n")
  (if (in? cmd '(begin exit)) (set! current-path #f))
  (cond ((== cmd 'text-cursor)
         (if (not (== current-cursor 'text-cursor))
           (begin
             (set! current-cursor 'text-cursor)
             (set-texmacs-pointer 'text-arrow)
           ) ;begin
         ) ;if
        ) ;
        ((== cmd 'graphics-cursor)
         (if (not (== current-cursor 'graphics-cursor))
           (begin
             (set! current-cursor 'graphics-cursor)
             (set-texmacs-pointer 'graphics-cross)
           ) ;begin
         ) ;if
        ) ;
        ((and (in? cmd '(begin exit)) (or (== cmd 'begin) (not sticky-point)))
         (graphics-init-state)
         (graphics-forget-states)
         (with p
           (graphics-active-path)
           (if p
             (begin
               (set! current-path p)
               (set! current-obj (graphics-active-object))
               (set! current-point-no #f)
               (graphics-decorations-update)
             ) ;begin
           ) ;if
         ) ;with
        ) ;
        ((and (== cmd 'exit) sticky-point)
         (set! graphics-undo-enabled #t)
         (set! remove-undo-mark? #f)
         (if graphics-first-state
           (begin
             (if (== (state-ref graphics-first-state 'graphics-action) 'start-move)
               (with p
                 (cursor-path)
                 (unredoable-undo)
                 ;; FIXME: Should rely on remove-undo-mark?
                 (go-to p)
               ) ;with
             ) ;if
           ) ;begin
         ) ;if
         (graphics-init-state)
         (graphics-forget-states)
        ) ;
        ((== cmd 'undo)
         (if (and sticky-point
               (not graphics-undo-enabled)
               (in? (state-ref graphics-first-state 'graphics-action)
                 '(start-move start-operation)
               ) ;in?
             ) ;and
           (begin
             (set! graphics-undo-enabled #t)
             (set! remove-undo-mark? #f)
             (unredoable-undo)
           ) ;begin
           (begin
             (set! the-sketch '())
             (invalidate-graphical-object)
             (if (and graphics-undo-enabled (not sticky-point))
               (with p
                 (graphics-active-path)
                 (if p
                   (begin
                     (set! current-path p)
                     (set! current-obj (graphics-active-object))
                     (set! current-point-no #f)
                     (graphics-decorations-update)
                   ) ;begin
                   (graphics-decorations-reset)
                 ) ;if
               ) ;with
             ) ;if
             (if (and (not graphics-undo-enabled) sticky-point) (graphics-decorations-reset))
             (set! choosing #f)
             (set-sticky-point-false)
             (set! dragging-create? #f)
             (set! dragging-busy? #f)
             (set! current-point-no #f)
             (set! graphics-undo-enabled #t)
             (set! remove-undo-mark? #f)
             (set! multiselecting #f)
             (set! preselected #f)
             (if graphics-first-state (graphics-back-first))
             (graphics-forget-states)
             (invalidate-graphical-object)
           ) ;begin
         ) ;if
         (graphics-group-start)
        ) ;
        (else (display* "Uncaptured reset-context " cmd "\n"))
  ) ;cond
) ;tm-define


;; helper functions
(tm-define (set-cursor-style style)
  (set-cursor-style-origin style)
  (set! the-current-cursor-style style)
) ;tm-define

;; modes in which the cursor should become hand-style, see graphics-main.scm too.

(define hand-modes
  '((tuple "group-edit" "move")
    (tuple "group-edit" "rotate")
    (tuple "group-edit" "zoom"))
) ;define

(tm-define (set-sticky-point-true)
  (set-cursor-style-if-in-hand-modes "closehand")
  (set! sticky-point #t)
) ;tm-define

(tm-define (set-sticky-point-false)
  (set-cursor-style-if-in-hand-modes "openhand")
  (set! sticky-point #f)
) ;tm-define

(tm-define (set-cursor-style-if-in-hand-modes style)
  (if (not (eq? (member (graphics-get-property "gr-mode") hand-modes) #f))
    (set-cursor-style style)
  ) ;if
) ;tm-define

(tm-define (set-cursor-style-now) (set-cursor-style the-current-cursor-style))
