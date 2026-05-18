
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : format-widgets.scm
;; DESCRIPTION : Widgets for text, paragraph and page properties
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; See menu-define.scm for the grammar of menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic pattern-selector) (:use (generic format-widgets)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Name conversions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (encode-pattern-name u)
  (let* ((name (if (string? u) u (url->unix u)))
         (t (url->unix (url-tail u)))
         (p (url->unix "$TEXMACS_PATH/misc/patterns"))
         (a (url->unix "$TEXMACS_PATH/misc"))
         (p* (url-append (unix->url p) "dummy"))
         (a* (url-append (unix->url a) "dummy"))
        ) ;
    (cond ((string-starts? name p) (url->unix (url-delta p* (unix->url name))))
          (else u)
    ) ;cond
  ) ;let*
) ;define

(define (decode-pattern-name s)
  (let* ((name (unix->url s))
         (base1 "$TEXMACS_PATH/misc/patterns/neutral-pattern.png")
         (base2 "$TEXMACS_PATH/misc/pictures/gradients/vertical-white-black.png")
         (base (if global-gradient? base2 base1))
         (artw "$TEXMACS_PATH/misc/dummy")
        ) ;
    (cond ((not (url-rooted? name)) (url-relative base name))
          (else name)
    ) ;cond
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Pattern selector / accessors
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define global-picture? #f)

(define global-gradient? #f)

(define global-pattern-color '(pattern "neutral-pattern.png" "1cm" "100@"))

(define (set-color col)
  (set! global-pattern-color col)
  (refresh-now "pattern-sample")
  (refresh-now "pattern-options")
) ;define

(define (get-color)
  global-pattern-color
) ;define

(define (set-name name)
  (with enc
    (encode-pattern-name name)
    (with col (get-color) (set-color `(pattern ,enc ,@(cddr col))))
  ) ;with
) ;define

(define (get-name)
  (cadr (get-color))
) ;define

(define (set-width w)
  (with col (get-color) (set-color `(pattern ,(cadr col) ,w ,@(cdddr col))))
) ;define

(define (get-width)
  (caddr (get-color))
) ;define

(define (set-height h)
  (with col
    (get-color)
    (set-color `(pattern ,(cadr col) ,(caddr col) ,h ,@(cddddr col)))
  ) ;with
) ;define

(define (get-height)
  (cadddr (get-color))
) ;define

(define (set-size s)
  (cond ((== s "Fit") (set-width "100%") (set-height "100%"))
        ((== s "Fit to width") (set-width "100%") (set-height "100@"))
        ((== s "Fit to height") (set-width "100@") (set-height "100%"))
  ) ;cond
) ;define

(define (get-size)
  (cond ((and (== (get-width) "100%") (== (get-height) "100%")) "Fit")
        ((== (get-width) "100%") "Fit to width")
        ((== (get-height) "100%") "Fit to height")
        (else "Fit")
  ) ;cond
) ;define

(define (reset-effect eff kind)
  (cond ((or (npair? eff) (npair? (cdr eff))) eff)
        ((== (car eff) kind) (reset-effect (cadr eff) kind))
        (else (cons* (car eff) (reset-effect (cadr eff) kind) (cddr eff)))
  ) ;cond
) ;define

(define (assign-effect eff kind args)
  (cons* kind eff args)
) ;define

(define (set-effect* tail apply? kind args)
  (if (null? tail) (set! tail (list "0")))
  (with res
    (reset-effect (car tail) kind)
    (with new
      (if apply? (assign-effect res kind args) res)
      (if (== new "0") (list) (list new))
    ) ;with
  ) ;with
) ;define

(define (set-effect kind apply? . args)
  (with col
    (get-color)
    (set-color `(pattern ,(cadr col)
                  ,(caddr col)
                  ,(cadddr col)
                  ,@(set-effect* (cddddr col) apply? kind args))
    ) ;set-color
  ) ;with
) ;define

(define (get-effect* eff kind)
  (cond ((or (npair? eff) (npair? (cdr eff))) #f)
        ((== (car eff) kind) (cddr eff))
        (else (get-effect* (cadr eff) kind))
  ) ;cond
) ;define

(define (get-effect kind)
  (with col
    (get-color)
    (with eff (and (nnull? (cddddr col)) (car (cddddr col))) (get-effect* eff kind))
  ) ;with
) ;define

(define (set-recolor recol)
  (set-effect 'eff-recolor (nnot recol) recol)
) ;define

(define (get-recolor)
  (and-with opts (get-effect 'eff-recolor) (car opts))
) ;define

(define (set-skin skin)
  (when (string? skin)
    (with (r g b a)
      (named-color->rgba skin)
      (when (== a 255)
        (set! a 64)
      ) ;when
      (set! skin (rgba->named-color (list r g b a)))
    ) ;with
  ) ;when
  (set-effect 'eff-skin (nnot skin) skin)
) ;define

(define (inc-skin)
  (and-with skin
    (get-skin)
    (with (r g b a)
      (named-color->rgba skin)
      (set! a (min 215 (inexact->exact (round (* (sqrt (sqrt 2)) a)))))
      (set-skin (rgba->named-color (list r g b a)))
    ) ;with
  ) ;and-with
) ;define

(define (dec-skin)
  (and-with skin
    (get-skin)
    (with (r g b a)
      (named-color->rgba skin)
      (set! a (max 4 (inexact->exact (round (* (sqrt (sqrt 0.5)) a)))))
      (set-skin (rgba->named-color (list r g b a)))
    ) ;with
  ) ;and-with
) ;define

(define (get-skin)
  (and-with opts (get-effect 'eff-skin) (car opts))
) ;define

(define (set-blur r)
  (set-effect 'eff-blur (nnot r) `(eff-gaussian ,r))
) ;define

(define (get-blur)
  (and-with opts
    (get-effect 'eff-blur)
    (and (tm-func? (car opts) 'eff-gaussian) (tm-ref (car opts) 0))
  ) ;and-with
) ;define

(define (set-gradient-foreground fg)
  (with opts
    (or (get-effect 'eff-gradient) (list "black" "white"))
    (set-effect 'eff-gradient #t fg (cadr opts))
  ) ;with
) ;define

(define (get-gradient-foreground)
  (and-with opts (get-effect 'eff-gradient) (car opts))
) ;define

(define (set-gradient-background bg)
  (with opts
    (or (get-effect 'eff-gradient) (list "black" "white"))
    (set-effect 'eff-gradient #t (car opts) bg)
  ) ;with
) ;define

(define (get-gradient-background)
  (and-with opts (get-effect 'eff-gradient) (cadr opts))
) ;define

(define (normalize-color col)
  (if (tm-func? col 'pattern) (apply tm-pattern (cdr col)) col)
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Pattern selector
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (pattern-name-selector)
  (let* ((name (unix->url (get-name)))
         (curr (decode-pattern-name (get-name)))
         (setter (lambda (c)
                   (when (and (pair? c) (url? (car c)))
                     (set-name (url->unix (car c)))
                   ) ;when
                 ) ;lambda
         ) ;setter
        ) ;
    (hlist (enum (set-name (url->unix answer))
             (list `(verbatim ,(url->system name)) "")
             `(verbatim ,(url->system name))
             "15em"
           ) ;enum
      //
      //
      //
      ((icon "tm_find.xpm")
       (cond ((and (not global-picture?) (not global-gradient?))
              (choose-file setter "Background pattern" "image" "" curr)
             ) ;
             (global-gradient? (choose-file setter "Background gradient" "image" "" curr))
             ((url-rooted? (unix->url (get-name)))
              (choose-file setter "Background picture" "image" "" curr)
             ) ;
             (else (choose-file setter "Background picture" "image"))
       ) ;cond
      ) ;
      >>
    ) ;hlist
  ) ;let*
) ;tm-widget

(tm-widget (pattern-recolor-options)
  (with recol
    (get-recolor)
    (hlist (when recol
             (enum (set-recolor answer)
               (list (or recol "")
                 "black"
                 "white"
                 "grey"
                 "red"
                 "green"
                 "blue"
                 "yellow"
                 "cyan"
                 "magenta"
                 "orange"
                 "brown"
                 ""
               ) ;list
               (or recol "")
               "15em"
             ) ;enum
           ) ;when
      //
      //
      //
      (toggle (set-recolor (and answer "black")) (nnot (get-recolor)))
      //
      //
      //
      ((icon "tm_color.xpm") (interactive-color set-recolor (list (or recol ""))))
      >>
    ) ;hlist
  ) ;with
) ;tm-widget

(tm-widget (pattern-skin-options)
  (with skin
    (get-skin)
    (hlist (when skin
             (enum (set-skin answer)
               (list (or skin "")
                 "black"
                 "white"
                 "grey"
                 "red"
                 "green"
                 "blue"
                 "yellow"
                 "cyan"
                 "magenta"
                 "orange"
                 "brown"
                 ""
               ) ;list
               (or skin "")
               "15em"
             ) ;enum
           ) ;when
      //
      //
      //
      (toggle (set-skin (and answer "black")) (nnot (get-skin)))
      //
      //
      //
      ((icon "tm_color.xpm") (interactive-color set-skin (list (or skin ""))))
      //
      //
      (when skin
        ((icon "tm_remove.xpm") (dec-skin))
        ((icon "tm_add.xpm") (inc-skin))
      ) ;when
      >>
    ) ;hlist
  ) ;with
) ;tm-widget

(tm-widget (pattern-blur-options)
  (with blur
    (get-blur)
    (hlist (when blur
             (enum (set-blur answer)
               (list (or blur "") "0.2pt" "0.5pt" "1pt" "2pt" "5px" "")
               (or blur "")
               "15em"
             ) ;enum
           ) ;when
      //
      //
      //
      (toggle (set-blur (and answer "1pt")) (nnot (get-blur)))
      >>
    ) ;hlist
  ) ;with
) ;tm-widget

(tm-widget (pattern-background-options)
  (with bg
    (get-gradient-background)
    (hlist (enum (set-gradient-background answer)
             (list (or bg "")
               "black"
               "white"
               "grey"
               "red"
               "green"
               "blue"
               "yellow"
               "cyan"
               "magenta"
               "orange"
               "brown"
               ""
             ) ;list
             (or bg "white")
             "15em"
           ) ;enum
      //
      //
      //
      ((icon "tm_color.xpm")
       (interactive-color set-gradient-background (list (or bg "white")))
      ) ;
      >>
    ) ;hlist
  ) ;with
) ;tm-widget

(tm-widget (pattern-foreground-options)
  (with fg
    (get-gradient-foreground)
    (hlist (enum (set-gradient-foreground answer)
             (list (or fg "")
               "black"
               "white"
               "grey"
               "red"
               "green"
               "blue"
               "yellow"
               "cyan"
               "magenta"
               "orange"
               "brown"
               ""
             ) ;list
             (or fg "black")
             "15em"
           ) ;enum
      //
      //
      //
      ((icon "tm_color.xpm")
       (interactive-color set-gradient-foreground (list (or fg "black")))
      ) ;
      >>
    ) ;hlist
  ) ;with
) ;tm-widget

(tm-widget ((pattern-selector u) cmd)
  (padded (hlist (vlist (refreshable "pattern-sample"
                          (resize "600px"
                            "450px"
                            (texmacs-output `(document (block (tformat (cwith "1"
                                                                         "1"
                                                                         "1"
                                                                         "1"
                                                                         "cell-width"
                                                                         "596guipx")
                                                                (cwith "1"
                                                                  "1"
                                                                  "1"
                                                                  "1"
                                                                  "cell-height"
                                                                  "446guipx")
                                                                (cwith "1"
                                                                  "1"
                                                                  "1"
                                                                  "1"
                                                                  "cell-vmode"
                                                                  "exact")
                                                                (cwith ,"1"
                                                                  ,"1"
                                                                  ,"1"
                                                                  ,"1"
                                                                  ,"cell-background"
                                                                  ,(get-color))
                                                                (table (row (cell ""))))))
                              '(style (tuple "generic"))
                            ) ;texmacs-output
                          ) ;resize
                        ) ;refreshable
                 ) ;vlist
            //
            //
            //
            (explicit-buttons (vlist (refreshable "pattern-options"
                                       (assuming (and (not global-picture?) (not global-gradient?))
                                         (aligned (item (text "Name:") (link pattern-name-selector))
                                           (item (text "Width:")
                                             (hlist (enum (set-width answer)
                                                      (list (get-width) "100%" "100@" "1cm" "")
                                                      (get-width)
                                                      "15em"
                                                    ) ;enum
                                               >>
                                             ) ;hlist
                                           ) ;item
                                           (item (text "Height:")
                                             (hlist (enum (set-height answer)
                                                      (list (get-height) "100%" "100@" "1cm" "")
                                                      (get-height)
                                                      "15em"
                                                    ) ;enum
                                               >>
                                             ) ;hlist
                                           ) ;item
                                           (item (text "Recolor:") (link pattern-recolor-options))
                                           (item (text "Skin:") (link pattern-skin-options))
                                           ;; TODO: the blur effect changes the image size;
                                           ;; one needs a blur that wraps around torically
                                           ;; (item (text "Blur:")
                                           ;;  (link pattern-blur-options))
                                         ) ;aligned
                                       ) ;assuming
                                       (assuming global-gradient?
                                         (aligned (item (text "Name:") (link pattern-name-selector))
                                           (item (text "Width:")
                                             (hlist (enum (set-width answer)
                                                      (list (get-width) "100%" "100@" "1cm" "")
                                                      (get-width)
                                                      "15em"
                                                    ) ;enum
                                               >>
                                             ) ;hlist
                                           ) ;item
                                           (item (text "Height:")
                                             (hlist (enum (set-height answer)
                                                      (list (get-height) "100%" "100@" "1cm" "")
                                                      (get-height)
                                                      "15em"
                                                    ) ;enum
                                               >>
                                             ) ;hlist
                                           ) ;item
                                           (item (text "Foreground:") (link pattern-foreground-options))
                                           (item (text "Background:") (link pattern-background-options))
                                         ) ;aligned
                                       ) ;assuming
                                       (assuming global-picture?
                                         (aligned (item (text "Name:") (link pattern-name-selector))
                                           (item (text "Size:")
                                             (hlist (enum (set-size answer)
                                                      (list "Fit" "Fit to width" "Fit to height")
                                                      (get-size)
                                                      "15em"
                                                    ) ;enum
                                               >>
                                             ) ;hlist
                                           ) ;item
                                           (item (text "Recolor:") (link pattern-recolor-options))
                                           (item (text "Skin:") (link pattern-skin-options))
                                         ) ;aligned
                                       ) ;assuming
                                     ) ;refreshable
                                ======
                                (glue #f #t 0 0)
                              ) ;vlist
            ) ;explicit-buttons
          ) ;hlist
    ======
    (explicit-buttons (hlist >>>
                       ("Ok"
                         (with col
                           (normalize-color (get-color))
                           (if global-picture?
                             (insert-preferred-list "my pictures" col 16)
                             (insert-preferred-list "my patterns" col 16)
                           ) ;if
                           (cmd col)
                         ) ;with
                       ) ;
                      ) ;hlist
    ) ;explicit-buttons
  ) ;padded
) ;tm-widget

(tm-define (open-pattern-selector cmd w)
  (:interactive #t)
  (when (or global-picture? global-gradient? (== (get-name) "neutral-pattern.png"))
    (set! global-picture? #f)
    (set! global-gradient? #f)
    (set! global-pattern-color `(pattern ,"neutral-pattern.png" ,w ,"100@"))
  ) ;when
  (with u
    (current-buffer)
    (dialogue-window (pattern-selector u) cmd "Pattern selector")
  ) ;with
) ;tm-define

(tm-define (open-gradient-selector cmd . opt-old)
  (:interactive #t)
  (when (or global-picture?
          (not global-gradient?)
          (== (get-name) "neutral-pattern.png")
        ) ;or
    (set! global-picture? #f)
    (set! global-gradient? #t)
    (set! global-pattern-color '(pattern "vertical-white-black.png"
                                  "100%"
                                  "100%"))
  ) ;when
  (when (nnull? opt-old)
    (set! global-pattern-color (car opt-old))
  ) ;when
  (with u
    (current-buffer)
    (dialogue-window (pattern-selector u) cmd "Gradient selector")
  ) ;with
) ;tm-define

(tm-define (open-background-picture-selector cmd . opt-old)
  (:interactive #t)
  (when (or (not global-picture?)
          global-gradient?
          (== (get-name) "neutral-pattern.png")
        ) ;or
    (set! global-picture? #t)
    (set! global-gradient? #f)
    (set! global-pattern-color '(pattern "neutral-pattern.png" "100%" "100%"))
  ) ;when
  (when (nnull? opt-old)
    (set! global-pattern-color (car opt-old))
  ) ;when
  (with u
    (current-buffer)
    (dialogue-window (pattern-selector u) cmd "Background picture selector")
  ) ;with
) ;tm-define
