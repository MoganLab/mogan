
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : menu-convert.scm
;; DESCRIPTION : routines for generating menus
;; COPYRIGHT   : (C) 2023  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; See menu-define.scm for the grammar of menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel gui menu-convert) (:use (kernel gui menu-widget)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helper routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (mini? style)
  (!= (logand style widget-style-mini) 0)
) ;define

(define (mono? style)
  (!= (logand style widget-style-monospaced) 0)
) ;define

(define (greyed? style)
  (!= (logand style widget-style-grey) 0)
) ;define

(define (pressed? style)
  (!= (logand style widget-style-pressed) 0)
) ;define

(define (active? style)
  (== (logand style widget-style-inert) 0)
) ;define

(define (inert? style)
  (!= (logand style widget-style-inert) 0)
) ;define

(define (button? style)
  (!= (logand style widget-style-button) 0)
) ;define

(define (center? style)
  (!= (logand style widget-style-centered) 0)
) ;define

(define (bold? style)
  (!= (logand style widget-style-bold) 0)
) ;define

(define (verb? style)
  (!= (logand style widget-style-verb) 0)
) ;define

(define global-command-table (make-ahash-table))

(define global-command-counter 0)

(define (command->mangle cmd)
  ;; FIXME: memory management
  (set! global-command-counter (+ global-command-counter 1))
  (ahash-set! global-command-table global-command-counter cmd)
  global-command-counter
) ;define

(define (mangle->command nr)
  (ahash-ref global-command-table nr)
) ;define

(define (nullary-mangled cmd)
  (with nr
    (command->mangle cmd)
    (string-append "(eval-nullary-mangled " (number->string nr) ")")
  ) ;with
) ;define

(tm-define (eval-nullary-mangled nr)
  (:secure #t)
  ;; FIXME: not that secure
  ;; (display* "Eval " nr ", " (mangle->command nr) "\n")
  (command-eval (mangle->command nr))
) ;tm-define

(define (unary-mangled cmd)
  (with nr
    (command->mangle cmd)
    (string-append "(eval-unary-mangled " (number->string nr) " answer)")
  ) ;with
) ;define

(tm-define (eval-unary-mangled nr answer)
  (:secure #t)
  ;; FIXME: not that secure
  ;; (display* "Apply " nr ", " (mangle->command nr) ", " answer "\n")
  (command-apply (mangle->command nr) (list answer))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Widgets through markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (markup-empty) "")

(tm-define (markup-text text style col transparent?)
  ;; (display* "Text " text ", " style ", " col ", " transparent? "\n")
  (when (mini? style)
    (set! text `(small ,text))
  ) ;when
  (when (or (greyed? style)
          ;; (inert? style)
          (== col (color "dark grey"))
        ) ;or
    (set! text `(greyed ,text))
  ) ;when
  (when (or (mono? style) (verb? style))
    (set! text `(verbatim ,text))
  ) ;when
  (when (bold? style)
    (set! text `(strong ,text))
  ) ;when
  (when (not transparent?)
    (set! text `(text-opaque ,text))
  ) ;when
  (when (center? style)
    (set! text `(text-center ,text))
  ) ;when
  `(inflate ,text)
) ;tm-define

(tm-define (markup-menu-group text style)
  (markup-text text style (color "black") #t)
) ;tm-define

(tm-define (markup-box font text col transparent? ink?)
  (markup-text text 0 col transparent?)
) ;tm-define

(tm-define (markup-balloon body balloon)
  `(help-balloon ,body ,balloon ,"right" ,"")
) ;tm-define

(tm-define (markup-xpm u)
  (let* ((s (url->string u))
         (r (if (string-ends? s ".xpm") (string-drop-right s 4) s))
         (i (string-append r "_x4.png"))
        ) ;
    `(icon ,i)
  ) ;let*
) ;tm-define

(tm-define (markup-glue hext? vext? w h)
  (let* ((ws (string-append (number->string (quotient w 256)) "px"))
         (hs (string-append (number->string (quotient h 256)) "px"))
        ) ;
    `(glue ,(if hext? "true" "false") ,(if vext? "true" "false") ,ws ,hs)
  ) ;let*
) ;tm-define

(tm-define (markup-color col hext? vext? w h)
  (let* ((ws (string-append (number->string w) "px"))
         (hs (string-append (number->string h) "px"))
        ) ;
    `(monochrome ,ws ,hs ,col)
  ) ;let*
) ;tm-define

(tm-define (markup-separator vertical?) "")

(define (listify tag l)
  (set! l (list-filter l (cut != <> "")))
  (set! l (append-map (lambda (x) (if (func? x tag) (cdr x) (list x))) l))
  (cond ((null? l) "")
        ((null? (cdr l)) (car l))
        (else `(,tag ,@l))
  ) ;cond
) ;define

(tm-define (markup-hlist l) (listify 'hlist l))

(tm-define (markup-vlist l) (listify 'vlist l))

(tm-define (markup-hsplit l r) `(hlist ,l ,r))

(tm-define (markup-vsplit t b) `(vlist ,t ,b))

(tm-define (markup-hmenu l) (listify 'hlist l))

(tm-define (markup-vmenu l) (listify 'vlist l))

(tm-define (markup-minibar-menu l)
  ;; TODO: minibar style?
  (listify 'hlist l)
) ;tm-define

(tm-define (markup-tmenu items columns)
  `(tiled ,(number->string columns) ,@items)
) ;tm-define

(define (alternate l r)
  (if (or (null? l) (null? r))
    (list)
    (cons* (car l) (car r) (alternate (cdr l) (cdr r)))
  ) ;if
) ;define

(tm-define (markup-aligned l-list r-list)
  `(align-tiled ,"2" ,@(alternate l-list r-list))
) ;tm-define

(tm-define (markup-menu-button text cmd check shortcut style)
  ;; TODO: handle 'shortcut'
  ;; TODO: handle inert style
  (let* ((body (markup-text text style (color "black") #t))
         (but `(menu-button ,body ,(nullary-mangled cmd)))
        ) ;
    (cond ((button? style) `(with-explicit-buttons ,but))
          ((pressed? style) `(with-pressed-buttons ,but))
          (else but)
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (markup-toggle cmd* on? style)
  ;; TODO: handle inert style
  (with cmd (unary-mangled cmd*) `(toggle-button ,(if on? "true" "false") ,cmd))
) ;tm-define

(tm-define (markup-tabs names bodies)
  `(tabs (tabs-bar (active-tab ,(car names))
           ,@(map (lambda (x) `(passive-tab ,x)) (cdr names)))
     (tabs-body (shown ,(car bodies))
       ,@(map (lambda (x) `(hidden ,x)) (cdr bodies))))
) ;tm-define

(define (attach-icon icon name)
  `(concat ,(markup-xpm icon) ," " ,name)
) ;define

(tm-define (markup-icon-tabs icons names bodies)
  (with combined (map attach-icon icons names) (markup-tabs combined bodies))
) ;tm-define

(tm-define (markup-input cmd* type props style width)
  ;; TODO: handle inert style
  (with cmd
    (unary-mangled cmd*)
    (cond ((null? props) `(input-field ,type ,cmd ,width ,""))
          ((null? (cdr props)) `(input-field ,type ,cmd ,width ,(car props)))
          (else `(input-popup ,type
                   ,cmd
                   ,width
                   ,(car props)
                   (choice-list ,cmd ,(car props) ,@props))
          ) ;else
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (markup-numeric-input cmd* width unit min max step def)
  (with cmd
    (unary-mangled cmd*)
    `(numeric-input ,cmd ,width ,unit ,min ,max ,step ,def)
  ) ;with
) ;tm-define

(tm-define (markup-enum cmd* props val style width)
  ;; TODO: handle inert style
  (with cmd
    (unary-mangled cmd*)
    (cond ((null? props) `(input-field ,"generic" ,cmd ,width ,val))
          ((== (cAr props) "")
           `(input-popup ,"generic"
              ,cmd
              ,width
              ,val
              (choice-list ,cmd ,val ,@(cDr props)))
          ) ;
          (else `(input-list ,"generic"
                   ,cmd
                   ,width
                   ,val
                   (choice-list ,cmd ,val ,@props)))
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (markup-choice cmd* proposals selected)
  (when (null? proposals)
    (set! proposals (list ""))
  ) ;when
  (with cmd (unary-mangled cmd*) `(choice-list ,cmd ,selected ,@proposals))
) ;tm-define

(tm-define (markup-choices cmd* proposals selected)
  (when (null? proposals)
    (set! proposals (list ""))
  ) ;when
  (with cmd
    (unary-mangled cmd*)
    `(check-list ,cmd (tuple ,@selected) ,@proposals)
  ) ;with
) ;tm-define

(tm-define (markup-filtered-choice cmd proposals selected filter)
  ;; TODO: do not ignore the filter
  (when (null? proposals)
    (set! proposals (list ""))
  ) ;when
  (markup-choice cmd proposals selected)
) ;tm-define

(tm-define (object->promise-markup obj) obj)

(tm-define (markup-pulldown-button button popup*)
  (with popup (popup*) `(popup-balloon ,button ,popup ,"left" ,"Bottom"))
) ;tm-define

(tm-define (markup-pullright-button button popup*)
  (with popup (popup*) `(popup-balloon ,button ,popup ,"Right" ,"top"))
) ;tm-define

(define (make-division tag body)
  (cond ((and (tm-in? body '(hlist vlist)) (in? tag '(plain-style discrete-style)))
         `(,(tm-car body) ,@(map (cut make-division tag <>) (tm-children body)))
        ) ;
        ((tm-is? body 'glue) body)
        (else `(,tag ,body))
  ) ;cond
) ;define

(tm-define (markup-division type body)
  (if (in? type
        (list "title" "section" "subsection" "section-tabs" "plain" "discrete")
      ) ;in?
    (make-division (string->symbol (string-append type "-style")) body)
    body
  ) ;if
) ;tm-define

(tm-define (markup-refresh wid kind)
  (lazy-initialize-force)
  (with x `(vertical (link ,(string->symbol wid))) (build-menu-widget x 0))
) ;tm-define

(tm-define (markup-refreshable body-promise kind) (body-promise))

(tm-define (markup-resize body style w1 h1 w2 h2 w3 h3 hpos vpos)
  (let* ((ww w2)
         (hh (if (string? h2) (string-append "-" h2) `(minus ,h2)))
         ;; (cv (lambda (x) `(minipar* ,x ,w2 ,h2)))
         (cv (lambda (x) `(canvas ,"0px" ,hh ,ww ,"0px" ,"0%" ,"0%" ,x)))
        ) ;
    (if (tm-func? body 'input-area 1) `(input-area ,(cv (tm-ref body 0))) (cv body))
  ) ;let*
) ;tm-define

(tm-define (markup-extend body items) body)

(tm-define (markup-scrollable body scroll-style) body)

(define (decode-packages style)
  (cond ((string? style) (list style))
        ((func? style 'style) (append-map decode-packages (cdr style)))
        ((func? style 'tuple) (append-map decode-packages (cdr style)))
        (else (list))
  ) ;cond
) ;define

(define (markup-with-packages packs body)
  ;; TODO: check absence of conflicts due to double loading
  (if (null? packs)
    body
    `(with-package ,(car packs) ,(markup-with-packages (cdr packs) body))
  ) ;if
) ;define

(tm-define (markup-texmacs-output body style)
  (markup-with-packages (decode-packages style) body)
) ;tm-define

(tm-define (markup-texmacs-input body style u)
  ;; TODO: url mirroring according to 'u'
  `(input-area ,(markup-with-packages (decode-packages style) body))
) ;tm-define

(tm-define (markup-color-picker cmd background? proposals) "Color picker")
(tm-define (markup-tree-view cmd data data-roles) "Tree view")
(tm-define (markup-ink cmd) "Ink widget")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu utilities
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-menu-error . args)
  (apply tm-display-error args)
  (markup-text "Error" 0 (color "black") #t)
) ;define

(define (build-menu-bad-format p style)
  (build-menu-error "menu has bad format in " (object->string p))
) ;define

(define (build-menu-empty)
  (markup-hmenu '())
) ;define

(define (delay-command cmd)
  (object->command (lambda () (exec-delayed cmd)))
) ;define

(define-macro (build-menu-command cmd)
  `(delay-command (lambda ,() (protected-call (lambda ,() ,cmd))))
) ;define-macro

(define (menu-protect cmd)
  (lambda x (exec-delayed (lambda () (protected-call (lambda () (apply cmd x))))))
) ;define

(define (kbd-system shortcut menu-flag?)
  (cond ((nstring? shortcut) "")
        ((and (qt-gui?) menu-flag?) shortcut)
        (else (translate (kbd-system-rewrite shortcut)))
  ) ;cond
) ;define

(define (kbd-find-shortcut what menu-flag?)
  (with r
    (kbd-find-inv-binding what)
    (when (string-contains? r "accent:")
      (set! r (string-replace r "accent:deadhat" "^"))
      (set! r (string-replace r "accent:tilde" "~"))
      (set! r (string-replace r "accent:acute" "'"))
      (set! r (string-replace r "accent:grave" "`"))
      (set! r (string-replace r "accent:umlaut" "\""))
      (set! r (string-replace r "accent:abovedot" "."))
      (set! r (string-replace r "accent:breve" "U"))
      (set! r (string-replace r "accent:invbreve" "A"))
      (set! r (string-replace r "accent:check" "C"))
    ) ;when
    ;; (when (!= r "")
    ;;  (display* what " -> " r " -> " (kbd-system r menu-flag?) "\n"))
    (kbd-system r menu-flag?)
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu labels
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (translatable? s)
  (or (string? s) (func? s 'concat) (func? s 'verbatim) (func? s 'replace))
) ;define

(define (recursive-occurs? w t)
  (cond ((string? t) (string-occurs? w t))
        ((list? t) (list-or (map (cut recursive-occurs? w <>) t)))
        (else #f)
  ) ;cond
) ;define

(define (recursive-replace t w b)
  (cond ((string? t) (string-replace t w b))
        ((list? t) (map (cut recursive-replace <> w b) t))
        (else t)
  ) ;cond
) ;define

(define (adjust-translation s t)
  (cond ((not (and (qt-gui?)
                (os-macos?)
                (in? (get-preference "language") (list "english" "british"))
              ) ;and
         ) ;not
         t
        ) ;
        ((recursive-occurs? "reference" s)
         (recursive-replace (recursive-replace t "c" "<#441>") "e" "<#435>")
        ) ;
        ((recursive-occurs? "onfigur" s) (recursive-replace t "o" "<#43E>"))
        ((in? s (list "Help" "Edit" "View::menu")) (recursive-replace t "e" "<#435>"))
        (else t)
  ) ;cond
) ;define

(define (build-menu-label p style . opt)
  "Make widget for menu label @p."
  ;; Possibilities for p:
  ;;   <label> :: (balloon <label> <string>)
  ;;     Label with a popup balloon. The <string> is the balloon text.
  ;;   <label> :: (text <font desc> <string>)
  ;;     Label <string> drawn in black text of an arbitrary font.
  ;;     <font desc> :: ([family [class [series [shape [size [dpi]]]]]])
  ;;     Example default values are: family="roman", class="mr",
  ;;     series="medium", shape="normal", size=10, dpi=600.
  ;;   <label> :: <string>
  ;;     Simple menu label, its display style is controlled by tt? and style
  ;;   <label> :: (icon <string>)
  ;;     Pixmap menu label, the <string> is the name of the pixmap.
  (let ((tt? (and (nnull? opt) (car opt)))
        (col (color (if (greyed? style) "dark grey" "black")))
       ) ;
    (cond ((translatable? p)
           (markup-text (adjust-translation p (translate p)) style col #t)
          ) ;
          ((tuple? p 'balloon 2) (build-menu-label (cadr p) style tt?))
          ((tuple? p 'extend)
           (with l
             (build-menu-items (cddr p) style tt?)
             (markup-extend (build-menu-label (cadr p) style tt?) l)
           ) ;with
          ) ;
          ((tuple? p 'style 2)
           (let* ((x (cadr p))
                  (new-style (if (> x 0) (logior style x) (logand style (lognot (- x)))))
                 ) ;
             (build-menu-label (caddr p) new-style tt?)
           ) ;let*
          ) ;
          ((tuple? p 'text 2) (markup-box (cadr p) (caddr p) col #t #t))
          ((tuple? p 'icon 1) (markup-xpm (cadr p)))
          ((tuple? p 'color 5)
           (markup-color (second p)
             (third p)
             (fourth p)
             (* (fifth p) 256)
             (* (sixth p) 256)
           ) ;markup-color
          ) ;
    ) ;cond
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Elementary menu items
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-menu-hsep)
  "Make @--- menu item."
  (markup-separator #f)
) ;define

(define (build-menu-vsep)
  "Make @| menu item."
  (markup-separator #t)
) ;define

(define (build-menu-glue hext? vext? minw minh)
  "Make @(glue :boolean? :boolean? :integer? :integer?) menu item."
  (markup-glue hext? vext? (* minw 256) (* minh 256))
) ;define

(define (build-menu-color col hext? vext? minw minh)
  "Make @(glue :1% :boolean? :boolean? :integer? :integer?) menu item."
  (markup-color col hext? vext? (* minw 256) (* minh 256))
) ;define

(define (build-menu-group s style)
  "Make @(group :string?) menu item."
  (markup-menu-group (adjust-translation s (translate s)) style)
) ;define

(define (build-menu-text s style)
  "Make @(text :string?) menu item."
  ;; (markup-text s style (color "black") #t)
  (markup-text (translate s) style (color "black") #f)
) ;define

(define (build-texmacs-output p style)
  "Make @(texmacs-output :%2) item."
  (with (tag t tmstyle) p (markup-texmacs-output (t) (tmstyle)))
) ;define

(define (build-texmacs-input p style)
  "Make @(texmacs-input :%3) item."
  (with (tag t tmstyle name)
    p
    (markup-texmacs-input (t) (tmstyle) (or (name) (url-none)))
  ) ;with
) ;define

(define (build-menu-input p style)
  "Make @(input :%1 :string? :%1 :string?) menu item."
  (with (tag cmd type props width)
    p
    (markup-input (object->command (menu-protect cmd)) type (props) style width)
  ) ;with
) ;define

(define (build-numeric-input p style)
  "Make @(numeric-input :%7) menu item."
  (with (tag cmd width unit min max step def)
    p
    (markup-numeric-input (object->command (menu-protect cmd))
      width
      unit
      min
      max
      step
      def
    ) ;markup-numeric-input
  ) ;with
) ;define

(define (build-enum p style)
  "Make @(enum :%3 :string?) item."
  (with (tag cmd vals val width)
    p
    (let* ((translate* (if (verb? style) identity translate))
           (xval (val))
           (xvals (vals))
           (nvals (if (and (nnull? xvals) (== (cAr xvals) ""))
                    `(,@(cDr xvals) ,xval ,"")
                    `(,@xvals ,xval)
                  ) ;if
           ) ;nvals
           (xvals* (list-remove-duplicates nvals))
           (tval (translate* xval))
           (tvals (map translate* xvals*))
           (dec (map (lambda (v) (cons (translate* v) v)) xvals*))
           (cmd* (lambda (r) (cmd (or (assoc-ref dec r) r))))
          ) ;
      (markup-enum (object->command (menu-protect cmd*)) tvals tval style width)
    ) ;let*
  ) ;with
) ;define

(define (build-choice p style)
  "Make @(choice :%3) item."
  (with (tag cmd vals val)
    p
    (markup-choice (object->command (menu-protect cmd)) (vals) (val))
  ) ;with
) ;define

(define (build-choices p style)
  "Make @(choices :%3) item."
  (with (tag cmd vals mc)
    p
    (markup-choices (object->command (menu-protect cmd)) (vals) (mc))
  ) ;with
) ;define

(define (build-filtered-choice p style)
  "Make @(filtered-choice :%4) item."
  (with (tag cmd vals val filterstr)
    p
    (markup-filtered-choice (object->command (menu-protect cmd))
      (vals)
      (val)
      (filterstr)
    ) ;markup-filtered-choice
  ) ;with
) ;define

(define (build-color-input p style)
  "Make @(color-input :%3) menu item."
  (with (tag cmd bg? props)
    p
    (markup-color-picker (object->command (menu-protect cmd)) bg? (props))
  ) ;with
) ;define

(define (build-tree-view p style)
  "Make @(tree-view :%3) item."
  (with (tag cmd data roles)
    p
    (markup-tree-view (object->command (menu-protect cmd)) (data) (roles))
  ) ;with
) ;define


(define (build-toggle p style)
  "Make @(toggle :%2) item."
  (with (tag cmd on) p (markup-toggle (object->command cmd) (on) style))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu entries
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (synopsis-substitute synopsis source)
  (if (string-occurs? "@" synopsis)
    #f
    ;; not yet implemented
    synopsis
  ) ;if
) ;define

(define (search-balloon-help action)
  (and-with source
    (promise-source action)
    (and (pair? source)
      (or (and-with prop
            (property (car source) :balloon)
            (with txt (apply (car prop) (cdr source)) (and (string? txt) txt))
          ) ;and-with
        (and-with prop
          (property (car source) :synopsis)
          (and (pair? prop)
            (string? (car prop))
            (with txt (synopsis-substitute (car prop) source) (and (string? txt) txt))
          ) ;and
        ) ;and-with
      ) ;or
    ) ;and
  ) ;and-with
) ;define

(define (add-menu-entry-balloon but style action)
  (with txt
    (search-balloon-help action)
    (if (not txt)
      but
      (with bal (markup-text txt style (color "black") #t) (markup-balloon but bal))
    ) ;if
  ) ;with
) ;define

(define (build-menu-entry-button style bar? bal? check label short action)
  (let* ((command (build-menu-command (if (active? style) (apply action '()))))
         (l (build-menu-label label style))
         (pressed? (and bar? (!= check "")))
         (new-style (logior style (if pressed? widget-style-pressed 0)))
        ) ;
    (with but
      (if bar?
        (markup-menu-button l command "" "" new-style)
        (markup-menu-button l command check short style)
      ) ;if
      (if bal? but (add-menu-entry-balloon but style action))
    ) ;with
  ) ;let*
) ;define

(define (build-menu-entry-shortcut label action opt-key)
  (cond (opt-key (kbd-system opt-key #t))
        ((pair? label) "")
        (else (with source
                (promise-source action)
                (if source (kbd-find-shortcut source #t) "")
              ) ;with
        ) ;else
  ) ;cond
) ;define

(define (build-menu-entry-check-sub result propose)
  (cond ((string? result) result)
        (result propose)
        (else "")
  ) ;cond
) ;define

(define (build-menu-entry-check opt-check action)
  (if opt-check
    (build-menu-entry-check-sub ((cadr opt-check)) (car opt-check))
    (with source
      (promise-source action)
      (cond ((not (and source (pair? source))) "")
            (else (with prop
                    (property (car source) :check-mark)
                    (build-menu-entry-check-sub (and prop (apply (cadr prop) (cdr source)))
                      (and prop (car prop))
                    ) ;build-menu-entry-check-sub
                  ) ;with
            ) ;else
      ) ;cond
    ) ;with
  ) ;if
) ;define

(define (menu-label-add-dots l)
  (cond ((match? l ':string?) (string-append l "..."))
        ((match? l '(concat :*)) `(,@(cDr l) ,(menu-label-add-dots (cAr l))))
        ((match? l '(verbatim :*)) `(,@(cDr l) ,(menu-label-add-dots (cAr l))))
        ((match? l '(text :tuple? :string?))
         `(text ,(cadr l) ,(string-append (caddr l) "..."))
        ) ;
        ((match? l '(icon :string?)) l)
        (else `(,(car l) ,(menu-label-add-dots (cadr l)) ,(caddr l)))
  ) ;cond
) ;define

(define (build-menu-entry-dots label action)
  (with source
    (promise-source action)
    (if (and source (pair? source) (property (car source) :interactive))
      (menu-label-add-dots label)
      label
    ) ;if
  ) ;with
) ;define

(define (build-menu-entry-style style action)
  (with source
    (promise-source action)
    (if (not (pair? source))
      style
      (with prop
        (property (car source) :applicable)
        (if (or (not prop) (apply (car prop) (list)))
          style
          (logior style (+ widget-style-inert widget-style-grey))
        ) ;if
      ) ;with
    ) ;if
  ) ;with
) ;define

(define (build-menu-entry-attrs label action opt-key opt-check)
  (cond ((match? label '(check :%1 :string? :%1))
         (build-menu-entry-attrs (cadr label) action opt-key (cddr label))
        ) ;
        ((match? label '(shortcut :%1 :string?))
         (build-menu-entry-attrs (cadr label) action (caddr label) opt-check)
        ) ;
        (else (values label action opt-key opt-check))
  ) ;cond
) ;define

(define (build-menu-entry-sub p style bar?)
  (receive (label action opt-key opt-check)
    (build-menu-entry-attrs (car p) (cAr p) #f #f)
    (build-menu-entry-button (build-menu-entry-style style action)
      bar?
      (tuple? (car p) 'balloon 2)
      (build-menu-entry-check opt-check action)
      (build-menu-entry-dots label action)
      (build-menu-entry-shortcut label action opt-key)
      action
    ) ;build-menu-entry-button
  ) ;receive
) ;define

(define (build-menu-entry p style bar?)
  "Make @:menu-wide-item menu item."
  (let ((but (build-menu-entry-sub p style bar?)) (label (car p)))
    (if (tuple? label 'balloon 2)
      (let* ((text (caddr label))
             (cmd (and (nnull? (cdr p)) (procedure? (cadr p)) (cadr p)))
             (src (and cmd (promise-source cmd)))
             (sh (and src (kbd-find-shortcut src #f)))
             (txt (if (or (not sh) (== sh ""))
                    text
                    (if (string? text) (string-append text " (" sh ")") text)
                  ) ;if
             ) ;txt
             (ftxt (translate txt))
             (twid (markup-text ftxt style (color "black") #t))
            ) ;
        (markup-balloon but twid)
      ) ;let*
      but
    ) ;if
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Symbol fields
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-menu-symbol-button style sym opt-cmd)
  (with col
    (color (if (greyed? style) "dark grey" "black"))
    (if opt-cmd
      (markup-menu-button (markup-box '() sym col #t #f)
        (build-menu-command (apply opt-cmd '()))
        ""
        ""
        style
      ) ;markup-menu-button
      (markup-menu-button (markup-box '() sym col #t #f)
        (build-menu-command (insert sym))
        ""
        ""
        style
      ) ;markup-menu-button
    ) ;if
  ) ;with
) ;define

(define (build-menu-symbol p style)
  "Make @(symbol :string? :*) menu item."
  ;; Possibilities for p:
  ;;   <menu-symbol> :: (symbol <symbol-string> [<cmd>])
  (with (tag symstring . opt)
    p
    (with opt-cmd
      (and (nnull? opt) (car opt))
      (if (and opt-cmd (not (procedure? opt-cmd)))
        (build-menu-error "invalid symbol command in " p)
        (let* ((source (and opt-cmd (promise-source opt-cmd)))
               (sh (kbd-find-shortcut (if source source symstring) #f))
              ) ;
          (if (== sh "")
            (build-menu-symbol-button style symstring opt-cmd)
            (markup-balloon (build-menu-symbol-button style symstring opt-cmd)
              (build-menu-label (string-append "Keyboard equivalent: " sh) style)
            ) ;markup-balloon
          ) ;if
        ) ;let*
      ) ;if
    ) ;with
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Composite menus and submenus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-menu-horizontal p style)
  "Make @(horizontal :menu-item-list) menu item."
  (markup-hmenu (build-menu-items (cdr p) style #t))
) ;define

(define (build-menu-vertical p style)
  "Make @(vertical :menu-item-list) menu item."
  (markup-vmenu (build-menu-items (cdr p) style #f))
) ;define

(define (build-menu-hlist p style)
  "Make @(hlist :menu-item-list) menu item."
  (markup-hlist (build-menu-items (cdr p) style #t))
) ;define

(define (build-menu-vlist p style)
  "Make @(vlist :menu-item-list) menu item."
  (markup-vlist (build-menu-items (cdr p) style #f))
) ;define

(define (build-menu-division p style)
  "Make @(division :%1 :menu-item-list) item."
  (with (tag name . items)
    p
    (with inner
      (build-menu-items (list (cons 'vertical items)) style #f)
      (markup-division (name) (car inner))
    ) ;with
  ) ;with
) ;define

(define (build-menu-class p style)
  "Make @(class :%1 :menu-item-list) item."
  (with (tag name . items)
    p
    (with inner
      (build-menu-items (list (cons 'horizontal items)) style #f)
      (markup-division (name) (car inner))
    ) ;with
  ) ;with
) ;define

(define (build-aligned p style)
  "Make @(aligned :menu-item-list) item."
  (markup-aligned (build-menu-items (map cadr (cdr p)) style #f)
    (build-menu-items (map caddr (cdr p)) style #f)
  ) ;markup-aligned
) ;define

(define (build-aligned-item p style)
  "Make @(aligned-item :%2) item."
  (display* "Error 'build-aligned-item', " p ", " style "\n")
  (list 'vlist)
) ;define

(define (tab-key x)
  (cadr x)
) ;define

(define (tab-value x)
  (list 'vlist (cddr x))
) ;define

(define (build-menu-tabs p style)
  "Make @(tabs :menu-item-list) menu item."
  (markup-tabs (build-menu-items (map tab-key (cdr p)) style #f)
    (build-menu-items (map tab-value (cdr p)) style #f)
  ) ;markup-tabs
) ;define

(define (build-menu-tab p style)
  "Make @(tab :menu-item-list) menu item."
  (display* "Error 'build-menu-tab', " p ", " style "\n")
  (list 'vlist)
) ;define

(define (icon-tab-icon x)
  (string->url (cadr x))
) ;define

(define (icon-tab-key x)
  (caddr x)
) ;define

(define (icon-tab-value x)
  (list 'vlist (cdddr x))
) ;define

(define (build-menu-icon-tabs p style)
  "Make @(icon-tabs :menu-item-list) menu item."
  (with style*
    (logior style widget-style-mini)
    (markup-icon-tabs (map icon-tab-icon (cdr p))
      (build-menu-items (map icon-tab-key (cdr p)) style* #f)
      (build-menu-items (map icon-tab-value (cdr p)) style #f)
    ) ;markup-icon-tabs
  ) ;with
) ;define

(define (build-menu-icon-tab p style)
  "Make @(icon-tab :menu-item-list) menu item."
  (display* "Error 'build-menu-icon-tab', " p ", " style "\n")
  (list 'vlist)
) ;define

(define (build-menu-extend p style bar?)
  "Make @(extend :menu-item :menu-item-list) menu item."
  (with l (build-menu-items (cdr p) style bar?) (markup-extend (car l) (cdr l)))
) ;define

(define (build-menu-style p style bar?)
  "Make @(extend :integer? :menu-item-list) menu item."
  (let* ((x (cadr p))
         (new-style (if (> x 0) (logior style x) (logand style (lognot (- x)))))
        ) ;
    (build-menu-items-list (cddr p) new-style bar?)
  ) ;let*
) ;define

(define (build-menu-minibar p style)
  "Make @(minibar :menu-item-list) menu items."
  (with new-style
    (logior style widget-style-mini)
    (markup-minibar-menu (build-menu-items (cdr p) new-style #t))
  ) ;with
) ;define

(define (build-menu-submenu p style)
  "Make @((:or -> =>) :menu-label :menu-item-list) menu item."
  (with (tag label . items)
    p
    (let ((button ((cond ((== tag '=>) markup-pulldown-button)
                         ((== tag '->) markup-pullright-button)
                   ) ;cond
                   (build-menu-label label style)
                   (object->promise-markup (lambda () (build-menu-widget (list 'vertical items) style))
                   ) ;object->promise-markup
                  ) ;
          ) ;button
         ) ;
      (if (tuple? label 'balloon 2)
        (let* ((text (caddr label))
               (ftxt (translate text))
               (twid (markup-text ftxt style (color "black") #t))
              ) ;
          (markup-balloon button twid)
        ) ;let*
        button
      ) ;if
    ) ;let
  ) ;with
) ;define

(define (build-menu-tile p style)
  "Make @(tile :integer? :menu-item-list) menu item."
  (with (tag width . items)
    p
    (markup-tmenu (build-menu-items items style #f) width)
  ) ;with
) ;define

(define (build-scrollable p style)
  "Make @(scrollable :menu-item-list) item."
  (with (tag . items)
    p
    (with inner
      (build-menu-items (list (cons 'vertical items)) style #f)
      (markup-scrollable (car inner) style)
    ) ;with
  ) ;with
) ;define

(define (decode-resize x default)
  (cond ((string? x) (list x x x default))
        ((list-3? x) (append x (list default)))
        ((list-4? x) x)
        (else (build-menu-error "bad length in " (object->string x)))
  ) ;cond
) ;define

(define (build-resize p style)
  "Make @(resize :%2 :menu-item-list) item."
  (with (tag w-cmd h-cmd . items)
    p
    (let ((w (w-cmd)) (h (h-cmd)))
      (with inner
        (build-menu-items (list (cons 'vertical items)) style #f)
        (with (w1 w2 w3 hpos)
          (decode-resize w "left")
          (with (h1 h2 h3 vpos)
            (decode-resize h "top")
            (markup-resize (car inner) style w1 h1 w2 h2 w3 h3 hpos vpos)
          ) ;with
        ) ;with
      ) ;with
    ) ;let
  ) ;with
) ;define

(define (build-hsplit p style)
  "Make @(hsplit :menu-item :menu-item) item."
  (with (tag . items)
    p
    (with l (build-menu-items items style #f) (markup-hsplit (car l) (cadr l)))
  ) ;with
) ;define

(define (build-vsplit p style)
  "Make @(vsplit :menu-item :menu-item) item."
  (with (tag . items)
    p
    (with l (build-menu-items items style #f) (markup-vsplit (car l) (cadr l)))
  ) ;with
) ;define

(define (build-ink p style)
  "Make @(ink) item."
  (with (tag cmd) p (markup-ink (object->command cmd)))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dynamic menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-menu-if p style bar?)
  "Make @(if :%1 :menu-item-list) menu items."
  (with (tag pred? . items)
    p
    (if (pred?) (build-menu-items-list items style bar?) '())
  ) ;with
) ;define

(define (build-menu-when p style bar?)
  "Make @(when :%1 :menu-item-list) menu items."
  (with (tag pred? . items)
    p
    (let* ((old-active? (== (logand style widget-style-inert) 0))
           (new-active? (and old-active? (pred?)))
           (new-style (logior style (if new-active? 0 (+ widget-style-inert widget-style-grey)))
           ) ;new-style
          ) ;
      (build-menu-items-list items new-style bar?)
    ) ;let*
  ) ;with
) ;define

(define (build-menu-for p style bar?)
  "Make @(for :%1 :%1) menu items."
  (with (tag gen-func vals-promise)
    p
    (let* ((vals (vals-promise)) (items (append-map gen-func vals)))
      (build-menu-items-list items style bar?)
    ) ;let*
  ) ;with
) ;define

(define (build-menu-mini p style bar?)
  "Make @(mini :%1 :menu-item-list) menu items."
  (with (tag pred? . items)
    p
    (let* ((style-maxi (logand style (lognot widget-style-mini)))
           (style-mini (logior style-maxi widget-style-mini))
           (new-style (if (pred?) style-mini style-maxi))
          ) ;
      (build-menu-items-list items new-style bar?)
    ) ;let*
  ) ;with
) ;define

(define (build-menu-link p style bar?)
  "Make @(link :%1) menu items."
  (with linked
   ((eval (cadr p)))
   (if linked
     (build-menu-items linked style bar?)
     (build-menu-error "bad link: " (object->string (cadr p)))
   ) ;if
  ) ;with
) ;define

(define (build-menu-dynamic p style bar?)
  "Make @(dynamic :%1) menu items."
  (with dyn
    (eval (cadr p))
    (if dyn
      (build-menu-items dyn style bar?)
      (build-menu-error "bad link: " (object->string (cadr p)))
    ) ;if
  ) ;with
) ;define

(define (build-menu-promise p style bar?)
  "Make @(promise :%1) menu items."
  (with value
   ((cadr p))
   ;; (build-menu-items value style bar?)
   (if (match? value ':menu-item)
     (build-menu-items value style bar?)
     (build-menu-error "promise did not yield a menu: " value)
   ) ;if
  ) ;with
) ;define

(define (build-refresh p style bar?)
  "Make @(refresh :%1 :string?) widget."
  (with (tag s kind)
    p
    (list (markup-refresh (if (string? s) s (symbol->string s)) kind))
  ) ;with
) ;define

(define (build-refreshable p style bar?)
  "Make @(refreshable :%1 :menu-item-list) menu items."
  (with (tag kind . items)
    p
    (list (markup-refreshable (lambda () (markup-vmenu (build-menu-items-list items style bar?)))
            (kind)
          ) ;markup-refreshable
    ) ;list
  ) ;with
) ;define

(define cached-widgets (make-ahash-table))

(define (build-cached p style bar?)
  "Make @(cached :%1 :menu-item-list) menu items."
  (with (tag kind valid? . items)
    p
    (let* ((kind* (kind))
           (fun (lambda ()
                  (or (and (valid?) (ahash-ref cached-widgets kind*))
                    (let* ((l (build-menu-items-list items style bar?)) (w (markup-vmenu l)))
                      (ahash-set! cached-widgets kind* w)
                      w
                    ) ;let*
                  ) ;or
                ) ;lambda
           ) ;fun
          ) ;
      (list (markup-refreshable fun kind*))
    ) ;let*
  ) ;with
) ;define

(tm-define (invalidate-now kind)
  (ahash-remove! cached-widgets kind)
  (refresh-now kind)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main routines for making menu items
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-menu-items-list l style bar?)
  "Make menu items for each element in @l and append results."
  (append-map (lambda (p) (build-menu-items p style bar?)) l)
) ;define

(define (build-menu-items p style bar?)
  "Make menu items @p. The items are on a bar if @bar? and of a given @style."
  ;; (display* "Make items " p ", " style "\n")
  (if (pair? p)
    (cond ((match? p '(input :%1 :string? :%1 :string?))
           (list (build-menu-input p style))
          ) ;
          ((translatable? (car p)) (list (build-menu-entry p style bar?)))
          ((symbol? (car p))
           (with result
             (ahash-ref build-menu-items-table (car p))
             (if (or (not result) (not (match? (cdr p) (car result))))
               (build-menu-items-list p style bar?)
               ((cadr result) p style bar?)
             ) ;if
           ) ;with
          ) ;
          ((match? (car p) ':menu-wide-label) (list (build-menu-entry p style bar?)))
          (else (build-menu-items-list p style bar?))
    ) ;cond
    (cond ((== p '---) (list (build-menu-hsep)))
          ((== p '|) (list (build-menu-vsep)))
          ;; '|
          ((== p '()) p)
          (else (list (build-menu-bad-format p style)))
    ) ;cond
  ) ;if
) ;define

(define-table build-menu-items-table
  (glue (:boolean? :boolean? :integer? :integer?)
    ,(lambda (p style bar?)
       (list (build-menu-glue (second p) (third p) (fourth p) (fifth p))))
  ) ;glue
  (color (:%1 :boolean? :boolean? :integer? :integer?)
    ,(lambda (p style bar?)
       (list (build-menu-color (second p)
               (third p)
               (fourth p)
               (fifth p)
               (sixth p))))
  ) ;color
  (group (:%1) ,(lambda (p style bar?) (list (build-menu-group (cadr p) style))))
  (text (:%1) ,(lambda (p style bar?) (list (build-menu-text (cadr p) style))))
  (invisible (:%1) ,(lambda (p style bar?) (list)))
  (symbol (:string? :*)
    ,(lambda (p style bar?) (list (build-menu-symbol p style)))
  ) ;symbol
  (texmacs-output (:%2)
    ,(lambda (p style bar?) (list (build-texmacs-output p style)))
  ) ;texmacs-output
  (texmacs-input (:%3)
    ,(lambda (p style bar?) (list (build-texmacs-input p style)))
  ) ;texmacs-input
  (input (:%1 :string? :%1 :string?)
    ,(lambda (p style bar?) (list (build-menu-input p style)))
  ) ;input
  (numeric-input (:%1 :string? :string? :integer? :integer? :integer? :integer?)
    ,(lambda (p style bar?) (list (build-numeric-input p style)))
  ) ;numeric-input
  (enum (:%3 :string?) ,(lambda (p style bar?) (list (build-enum p style))))
  (choice (:%3) ,(lambda (p style bar?) (list (build-choice p style))))
  (choices (:%3) ,(lambda (p style bar?) (list (build-choices p style))))
  (filtered-choice (:%4)
    ,(lambda (p style bar?) (list (build-filtered-choice p style)))
  ) ;filtered-choice
  (color-input (:%3) ,(lambda (p style bar?) (list (build-color-input p style))))
  (tree-view (:%3) ,(lambda (p style bar?) (list (build-tree-view p style))))
  (toggle (:%2) ,(lambda (p style bar?) (list (build-toggle p style))))
  (link (:%1) ,(lambda (p style bar?) (build-menu-link p style bar?)))
  (dynamic (:%1) ,(lambda (p style bar?) (build-menu-dynamic p style bar?)))
  (horizontal (:*)
    ,(lambda (p style bar?) (list (build-menu-horizontal p style)))
  ) ;horizontal
  (vertical (:*) ,(lambda (p style bar?) (list (build-menu-vertical p style))))
  (hlist (:*) ,(lambda (p style bar?) (list (build-menu-hlist p style))))
  (vlist (:*) ,(lambda (p style bar?) (list (build-menu-vlist p style))))
  (division (:%1 :*)
    ,(lambda (p style bar?) (list (build-menu-division p style)))
  ) ;division
  (class (:%1 :*) ,(lambda (p style bar?) (list (build-menu-class p style))))
  (aligned (:*) ,(lambda (p style bar?) (list (build-aligned p style))))
  (aligned-item (:%2)
    ,(lambda (p style bar?) (list (build-aligned-item p style)))
  ) ;aligned-item
  (tabs (:*) ,(lambda (p style bar?) (list (build-menu-tabs p style))))
  (tab (:*) ,(lambda (p style bar?) (list (build-menu-tab p style))))
  (icon-tabs (:*) ,(lambda (p style bar?) (list (build-menu-icon-tabs p style))))
  (icon-tab (:*) ,(lambda (p style bar?) (list (build-menu-icon-tab p style))))
  (minibar (:*) ,(lambda (p style bar?) (list (build-menu-minibar p style))))
  (extend (:%1 :*)
    ,(lambda (p style bar?) (list (build-menu-extend p style bar?)))
  ) ;extend
  (style (:%1 :*) ,(lambda (p style bar?) (build-menu-style p style bar?)))
  (-> (:%1 :*) ,(lambda (p style bar?) (list (build-menu-submenu p style))))
  (=> (:%1 :*) ,(lambda (p style bar?) (list (build-menu-submenu p style))))
  (tile (:integer? :*) ,(lambda (p style bar?) (list (build-menu-tile p style))))
  (scrollable (:*) ,(lambda (p style bar?) (list (build-scrollable p style))))
  (resize (:%2 :*) ,(lambda (p style bar?) (list (build-resize p style))))
  (hsplit (:%2) ,(lambda (p style bar?) (list (build-hsplit p style))))
  (vsplit (:%2) ,(lambda (p style bar?) (list (build-vsplit p style))))
  (ink (:%1) ,(lambda (p style bar?) (list (build-ink p style))))
  (if (:%1 :*) ,(lambda (p style bar?) (build-menu-if p style bar?)))
  (when (:%1 :*)
    ,(lambda (p style bar?) (build-menu-when p style bar?))
  ) ;when
  (for (:%1 :%1) ,(lambda (p style bar?) (build-menu-for p style bar?)))
  (mini (:%1 :*) ,(lambda (p style bar?) (build-menu-mini p style bar?)))
  (promise (:%1) ,(lambda (p style bar?) (build-menu-promise p style bar?)))
  (refresh (:%1 :string?) ,(lambda (p style bar?) (build-refresh p style bar?)))
  (refreshable (:%1 :*) ,(lambda (p style bar?)
                           (build-refreshable p style bar?)))
  (cached (:%1 :%1 :*) ,(lambda (p style bar?) (build-cached p style bar?)))
) ;define-table

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (build-menu-main p style)
  "Transform the menu @p into a widget."
  (with l
    (build-menu-items p style #f)
    (cond ((null? l) (build-menu-empty))
          ((and (list? l) (null? (cdr l))) (car l))
          (else (build-menu-bad-format p style))
    ) ;cond
  ) ;with
) ;define

(tm-define (build-menu-widget p style)
  (:synopsis "Transform a menu into a widget")
  (:argument p "a scheme object which represents the menu")
  (:argument style "menu style")
  ((wrap-catch build-menu-main) p style)
) ;tm-define

(define (get-gui-style)
  (with theme
    (get-preference "gui theme")
    (cond ((== theme "light") "gui-bright")
          ((== theme "dark") "gui-dark")
          (else "gui-button")
    ) ;cond
  ) ;with
) ;define

(tm-define (make-menu-widget** p style . opt-size)
  (let* ((cur (current-buffer))
         (u (url-append "tmfs://aux/gui" (url-unroot cur)))
         (doc (build-menu-widget p style))
         (w* (if (null? opt-size) 800 (quotient (* 1 (car opt-size)) 1)))
         (h* (if (nlist>1? opt-size) 600 (quotient (* 1 (cadr opt-size)) 1)))
         (w (string-append (number->string w*) "px"))
         (h (string-append (number->string h*) "px"))
         (sty `(style (tuple ,"generic"
                        ,(get-gui-style)
                        ,@(if (null? opt-size) '() '("side-tools"))))
         ) ;sty
         (tmo `(texmacs-output ,(lambda () `(top-widget ,doc)) ,(lambda () sty)))
         (tmi `(texmacs-input ,(lambda () `(top-widget ,doc))
                 ,(lambda () sty)
                 ,(lambda () u))
         ) ;tmi
         (rsz `(resize ,(lambda () w) ,(lambda () h) ,tmi))
         (wid (list 'vertical (list rsz) '(glue #f #t 0 0)))
        ) ;
    ;; (display* "doc = " doc "\n")
    (buffer-set-master u cur)
    (make-menu-widget wid style)
  ) ;let*
) ;tm-define
