
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : menu-widget.scm
;; DESCRIPTION : routines for generating menus
;; COPYRIGHT   : (C) 2002  Joris van der Hoeven, David Allouche
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; See menu-define.scm for the grammar of menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii hashlib))

(texmacs-module (kernel gui menu-widget)
  (:use (kernel gui menu-define) (kernel gui kbd-define))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu grammar
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-regexp-grammar (:translatable? (:or :string?
                                         (concat :*)
                                         (verbatim :%1)
                                         (text :tuple? :string?)
                                         (replace :string? :translatable?)
                                       ) ;:or
                       ) ;:translatable?
  (:menu-label (:or :translatable?
                 (color :%5)
                 (icon :string?)
                 (extend :menu-label :*)
                 (style :integer? :menu-label)
                 (balloon :menu-label :translatable?)
               ) ;:or
  ) ;:menu-label
  (:menu-wide-label (:or :menu-label
                      (check :menu-wide-label :string? :%1)
                      (shortcut :menu-wide-label :string?)
                    ) ;:or
  ) ;:menu-wide-label
  (:menu-item (:or ---
                |
                ;; |
                (group :%1)
                (text :%1)
                (invisible :%1)
                (glue :boolean? :boolean? :integer? :integer?)
                (color :%1 :boolean? :boolean? :integer? :integer?)
                (:menu-wide-label :%1)
                (symbol :string? :*)
                (texmacs-output :%2)
                (texmacs-input :%3)
                (input :%1 :string? :%1 :string?)
                (enum :%3 :string?)
                (choice :%3)
                (choices :%3)
                (filtered-choice :%4)
                (color-input :%3)
                (tree-view :%3)
                (toggle :%2)
                (horizontal :menu-item-list)
                (vertical :menu-item-list)
                (hlist :menu-item-list)
                (vlist :menu-item-list)
                (division :%1 :menu-item-list)
                (class :%1 :menu-item-list)
                (aligned :menu-item-list)
                (aligned-item :%2)
                (tabs :menu-item-list)
                (tab :menu-item-list)
                (icon-tabs :menu-item-list)
                (icon-tab :menu-item-list)
                (minibar :menu-item-list)
                (extend :menu-item :menu-item-list)
                (style :integer? :menu-item-list)
                (-> :menu-label :menu-item-list)
                (=> :menu-label :menu-item-list)
                (tile :integer? :menu-item-list)
                (scrollable :menu-item-list)
                (resize :%2 :menu-item-list)
                (hsplit :menu-item :menu-item)
                (vsplit :menu-item :menu-item)
                (refresh :%1 :string?)
                (refreshable :%1 :menu-item-list)
                (cached :%1 :%1 :menu-item-list)
                (if :%1 :menu-item-list)
                (when :%1
                  :menu-item-list
                ) ;when
                (for :%1 :%1)
                (mini :%1 :menu-item-list)
                (link :%1)
                (promise :%1)
                (ink :%1)
                (:menu-item-list)
              ) ;:or
  ) ;:menu-item
  (:menu-item-list (:repeat :menu-item))
  (:tab-page (tab-page :%4))
) ;define-regexp-grammar

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu utilities
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (make-menu-error . args)
  (apply tm-display-error args)
  (widget-text "Error" 0 (color "black") #t)
) ;define

(define (make-menu-bad-format p style)
  (make-menu-error "menu has bad format in " (object->string p))
) ;define

(define (make-menu-empty)
  (widget-hmenu '())
) ;define

(define (delay-command cmd)
  (object->command (lambda () (exec-delayed cmd)))
) ;define

(define-macro (make-menu-command cmd)
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
  (define (pull-focus-transform src)
    (if (tuple? src 'pull-focus 2) `(,(caaddr src) (focus-tree)) src)
  ) ;define

  (with r
    (kbd-find-inv-binding (pull-focus-transform what))
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

(tm-define (kbd-find-shortcut-export what)
  (:synopsis "Find display shortcut string for command @what")
  (kbd-find-shortcut what #f)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu labels
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (translatable? s)
  (or (string? s) (func? s 'concat) (func? s 'verbatim) (func? s 'replace))
) ;define

(define (active? style)
  (== (logand style widget-style-inert) 0)
) ;define

(define (greyed? style)
  (!= (logand style widget-style-grey) 0)
) ;define

(define (verb? style)
  (!= (logand style widget-style-verb) 0)
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

(define (make-menu-label p style . opt)
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
    (cond ((and (list? p) (== (car p) 'verbatim)) (widget-text (cadr p) style col #t))
          ((translatable? p)
           (widget-text (adjust-translation p (translate p)) style col #t)
          ) ;
          ((tuple? p 'balloon 2) (make-menu-label (cadr p) style tt?))
          ((tuple? p 'extend)
           (with l
             (make-menu-items (cddr p) style tt?)
             (widget-extend (make-menu-label (cadr p) style tt?) l)
           ) ;with
          ) ;
          ((tuple? p 'style 2)
           (let* ((x (cadr p))
                  (new-style (if (> x 0) (logior style x) (logand style (lognot (- x)))))
                 ) ;
             (make-menu-label (caddr p) new-style tt?)
           ) ;let*
          ) ;
          ((tuple? p 'text 2) (widget-box (cadr p) (caddr p) col #t #t))
          ((tuple? p 'icon 1) (widget-xpm (cadr p)))
          ((tuple? p 'color 5)
           (widget-color (second p)
             (third p)
             (fourth p)
             (* (fifth p) 256)
             (* (sixth p) 256)
           ) ;widget-color
          ) ;
    ) ;cond
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Elementary menu items
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (make-menu-hsep)
  "Make @--- menu item."
  (widget-separator #f)
) ;define

(define (make-menu-vsep)
  "Make @| menu item."
  (widget-separator #t)
) ;define

(define (make-menu-glue hext? vext? minw minh)
  "Make @(glue :boolean? :boolean? :integer? :integer?) menu item."
  (widget-glue hext? vext? (* minw 256) (* minh 256))
) ;define

(define (make-menu-color col hext? vext? minw minh)
  "Make @(glue :1% :boolean? :boolean? :integer? :integer?) menu item."
  (widget-color col hext? vext? (* minw 256) (* minh 256))
) ;define

(define (make-menu-group s style)
  "Make @(group :string?) menu item."
  (widget-menu-group (adjust-translation s (translate s)) style)
) ;define

(define (make-menu-text s style)
  "Make @(text :string?) menu item."
  ;; (widget-text s style (color "black") #t)
  (widget-text (translate s) style (color "black") #f)
) ;define

(define (attach-resize t)
  (if (not global-resize)
    t
    (with (w1 w2 w3 wpos h1 h2 h3 hpos)
      global-resize
      (with attrs
        (list "page-medium"
          "papyrus"
          "page-type"
          "user"
          "page-width"
          w2
          "page-height"
          h2
          "page-odd"
          "4px"
          "page-even"
          "4px"
          "page-right"
          "4px"
          "page-top"
          "2px"
          "page-bot"
          "2px"
          "page-screen-left"
          "4px"
          "page-screen-right"
          "4px"
          "page-screen-top"
          "2px"
          "page-screen-bot"
          "2px"
        ) ;list
        (if (tm-is? t 'with)
          `(with ,@attrs ,@(cDr (tm-children t)) ,(cAr (tm-children t)))
          `(with ,@attrs ,t)
        ) ;if
      ) ;with
    ) ;with
  ) ;if
) ;define

(define (make-texmacs-output p style)
  "Make @(texmacs-output :%2) item."
  (with (tag t tmstyle) p (widget-texmacs-output (attach-resize (t)) (tmstyle)))
) ;define

(define (make-texmacs-input p style)
  "Make @(texmacs-input :%3) item."
  (with (tag t tmstyle name)
    p
    (widget-texmacs-input (attach-resize (t)) (tmstyle) (or (name) (url-none)))
  ) ;with
) ;define

(define (make-menu-input p style)
  "Make @(input :%1 :string? :%1 :string?) menu item."
  (with (tag cmd type props width)
    p
    (widget-input (object->command (menu-protect cmd)) type (props) style width)
  ) ;with
) ;define

(define (make-numeric-input p style)
  "Make @(numeric-input :%7) menu item."
  (with (tag cmd width unit min max step def)
    p
    (widget-numeric-input (object->command (menu-protect cmd))
      width
      unit
      min
      max
      step
      def
    ) ;widget-numeric-input
  ) ;with
) ;define

(define (make-enum p style)
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
      (widget-enum (object->command (menu-protect cmd*)) tvals tval style width)
    ) ;let*
  ) ;with
) ;define

(define (make-choice p style)
  "Make @(choice :%3) item."
  (with (tag cmd vals val)
    p
    (widget-choice (object->command (menu-protect cmd)) (vals) (val))
  ) ;with
) ;define

(define (make-choices p style)
  "Make @(choices :%3) item."
  (with (tag cmd vals mc)
    p
    (widget-choices (object->command (menu-protect cmd)) (vals) (mc))
  ) ;with
) ;define

(define (make-filtered-choice p style)
  "Make @(filtered-choice :%4) item."
  (with (tag cmd vals val filterstr)
    p
    (widget-filtered-choice (object->command (menu-protect cmd))
      (vals)
      (val)
      (filterstr)
    ) ;widget-filtered-choice
  ) ;with
) ;define

(define (make-color-input p style)
  "Make @(color-input :%3) menu item."
  (with (tag cmd bg? props)
    p
    (widget-color-picker (object->command (menu-protect cmd)) bg? (props))
  ) ;with
) ;define

(define (make-tree-view p style)
  "Make @(tree-view :%3) item."
  (with (tag cmd data roles)
    p
    (widget-tree-view (object->command (menu-protect cmd)) (data) (roles))
  ) ;with
) ;define


(define (make-toggle p style)
  "Make @(toggle :%2) item."
  (with (tag cmd on) p (widget-toggle (object->command cmd) (on) style))
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
      (with bal
        (widget-text (translate txt) style (color "black") #t)
        (widget-balloon but bal)
      ) ;with
    ) ;if
  ) ;with
) ;define

(define (make-menu-entry-button style bar? bal? check label short action)
  (let* ((command (make-menu-command (if (active? style) (apply action '()))))
         (l (make-menu-label label style))
         (pressed? (and bar? (!= check "")))
         (new-style (logior style (if pressed? widget-style-pressed 0)))
        ) ;
    (with but
      (if bar?
        (widget-menu-button l command "" "" new-style)
        (widget-menu-button l command check short style)
      ) ;if
      (if bal? but (add-menu-entry-balloon but style action))
    ) ;with
  ) ;let*
) ;define

(define-public (promise-source action)
  "Helper routines for menu-widget and kbd-define"
  (and (procedure? action)
    (with source
      (procedure-source action)
      (and (== (car source) 'lambda)
        (== (cadr source) '())
        (null? (cdddr source))
        (caddr source)
      ) ;and
    ) ;with
  ) ;and
) ;define-public

(define (make-menu-entry-shortcut label action opt-key)
  (cond (opt-key (kbd-system opt-key #t))
        ((pair? label) "")
        (else (with source
                (promise-source action)
                (if source (kbd-find-shortcut source #t) "")
              ) ;with
        ) ;else
  ) ;cond
) ;define

(define (make-menu-entry-check-sub result propose)
  (cond ((string? result) result)
        (result propose)
        (else "")
  ) ;cond
) ;define

(define (make-menu-entry-check opt-check action)
  (if opt-check
    (make-menu-entry-check-sub ((cadr opt-check)) (car opt-check))
    (with source
      (promise-source action)
      (cond ((not (and source (pair? source))) "")
            (else (with prop
                    (property (car source) :check-mark)
                    (make-menu-entry-check-sub (and prop (apply (cadr prop) (cdr source)))
                      (and prop (car prop))
                    ) ;make-menu-entry-check-sub
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

(define (make-menu-entry-dots label action)
  (with source
    (promise-source action)
    (if (and source (pair? source) (property (car source) :interactive))
      (menu-label-add-dots label)
      label
    ) ;if
  ) ;with
) ;define

(define (make-menu-entry-style style action)
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

(define (make-menu-entry-attrs label action opt-key opt-check)
  (cond ((match? label '(check :%1 :string? :%1))
         (make-menu-entry-attrs (cadr label) action opt-key (cddr label))
        ) ;
        ((match? label '(shortcut :%1 :string?))
         (make-menu-entry-attrs (cadr label) action (caddr label) opt-check)
        ) ;
        (else (values label action opt-key opt-check))
  ) ;cond
) ;define

(define (make-menu-entry-sub p style bar?)
  (receive (label action opt-key opt-check)
    (make-menu-entry-attrs (car p) (cAr p) #f #f)
    (make-menu-entry-button (make-menu-entry-style style action)
      bar?
      (tuple? (car p) 'balloon 2)
      (make-menu-entry-check opt-check action)
      (make-menu-entry-dots label action)
      (make-menu-entry-shortcut label action opt-key)
      action
    ) ;make-menu-entry-button
  ) ;receive
) ;define

(define (make-menu-entry p style bar?)
  "Make @:menu-wide-item menu item."

  (define (retrieve-shortcut p)
    (let* ((cmd (and (nnull? (cdr p)) (procedure? (cadr p)) (cadr p)))
           (source (and cmd (promise-source cmd)))
          ) ;
      (and source (kbd-find-shortcut source #f))
    ) ;let*
  ) ;define

  (define (create-text-widget text shortcut style)
    (let* ((txt (if (or (not shortcut) (== shortcut ""))
                  text
                  (if (string? text) (string-append text " (" shortcut ")") text)
                ) ;if
           ) ;txt
           (ftxt (translate txt))
          ) ;
      (widget-text ftxt style (color "black") #t)
    ) ;let*
  ) ;define

  (let ((but (make-menu-entry-sub p style bar?)) (label (car p)))
    (cond ((tuple? label 'balloon 2)
           (let* ((text (caddr label))
                  (shortcut (retrieve-shortcut p))
                  (twid (create-text-widget text shortcut style))
                 ) ;
             (widget-balloon but twid)
           ) ;let*
          ) ;
          ((and (tuple? label 'check 3) (tuple? (cadr label) 'balloon 2))
           (let* ((text (caddr (cadr label)))
                  (shortcut (retrieve-shortcut p))
                  (twid (create-text-widget text shortcut style))
                 ) ;
             (widget-balloon but twid)
           ) ;let*
          ) ;
          (else but)
    ) ;cond
  ) ;let
) ;define

(define (make-tab-page entry-data style bar?)

  (let* ((args (cdar entry-data))
         (url (first args))
         (title (second args))
         (close-btn (third args))
         (active? (fourth args))
        ) ;
    (widget-tab-page url
      (car (make-menu-items title style bar?))
      (car (make-menu-items close-btn style bar?))
      active?
    ) ;widget-tab-page
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Symbol fields
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (make-menu-symbol-button style font sym opt-cmd clr)
  (with col
    (if (greyed? style) (color "dark grey") clr)
    (if opt-cmd
      (widget-menu-button (widget-box font sym col #t #t)
        (make-menu-command (apply opt-cmd '()))
        ""
        ""
        style
      ) ;widget-menu-button
      (widget-menu-button (widget-box font sym col #t #t)
        (make-menu-command (insert sym))
        ""
        ""
        style
      ) ;widget-menu-button
    ) ;if
  ) ;with
) ;define

(define (make-menu-symbol p style font col)
  "Make @(symbol :string? :*) menu item."
  ;; Possibilities for p:
  ;;   <menu-symbol> :: (symbol <symbol-string> [<cmd>])
  (with (tag symstring . opt)
    p
    (with opt-cmd
      (and (nnull? opt) (car opt))
      (if (and opt-cmd (not (procedure? opt-cmd)))
        (make-menu-error "invalid symbol command in " p)
        (let* ((source (and opt-cmd (promise-source opt-cmd)))
               (sh (kbd-find-shortcut (if source source symstring) #f))
              ) ;
          (if (== sh "")
            (make-menu-symbol-button style font symstring opt-cmd col)
            (widget-balloon (make-menu-symbol-button style font symstring opt-cmd col)
              (make-menu-label (string-append "Keyboard equivalent: " sh) style)
            ) ;widget-balloon
          ) ;if
        ) ;let*
      ) ;if
    ) ;with
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Composite menus and submenus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (make-menu-horizontal p style)
  "Make @(horizontal :menu-item-list) menu item."
  (widget-hmenu (make-menu-items (cdr p) style #t))
) ;define

(define (make-menu-vertical p style)
  "Make @(vertical :menu-item-list) menu item."
  (widget-vmenu (make-menu-items (cdr p) style #f))
) ;define

(define (make-menu-hlist p style)
  "Make @(hlist :menu-item-list) menu item."
  (widget-hlist (make-menu-items (cdr p) style #t))
) ;define

(define (make-menu-vlist p style)
  "Make @(vlist :menu-item-list) menu item."
  (widget-vlist (make-menu-items (cdr p) style #f))
) ;define

(define (make-menu-division p style)
  "Make @(division :%1 :menu-item-list) item."
  (with (tag name . items)
    p
    (with inner
      (make-menu-items (list (cons 'vertical items)) style #f)
      (widget-division (name) (car inner))
    ) ;with
  ) ;with
) ;define

(define (make-menu-class p style)
  "Make @(class :%1 :menu-item-list) item."
  (with (tag name . items)
    p
    (with inner
      (make-menu-items (list (cons 'horizontal items)) style #f)
      (widget-division (name) (car inner))
    ) ;with
  ) ;with
) ;define

(define (make-aligned p style)
  "Make @(aligned :menu-item-list) item."
  (widget-aligned (make-menu-items (map cadr (cdr p)) style #f)
    (make-menu-items (map caddr (cdr p)) style #f)
  ) ;widget-aligned
) ;define

(define (make-aligned-item p style)
  "Make @(aligned-item :%2) item."
  (display* "Error 'make-aligned-item', " p ", " style "\n")
  (list 'vlist)
) ;define

(define (tab-key x)
  (cadr x)
) ;define

(define (tab-value x)
  (list 'vlist (cddr x))
) ;define

(define (make-menu-tabs p style)
  "Make @(tabs :menu-item-list) menu item."
  (widget-tabs (make-menu-items (map tab-key (cdr p)) style #f)
    (make-menu-items (map tab-value (cdr p)) style #f)
  ) ;widget-tabs
) ;define

(define (make-menu-tab p style)
  "Make @(tab :menu-item-list) menu item."
  (display* "Error 'make-menu-tab', " p ", " style "\n")
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

(define (make-menu-icon-tabs p style)
  "Make @(icon-tabs :menu-item-list) menu item."
  (with style*
    (logior style widget-style-mini)
    (widget-icon-tabs (map icon-tab-icon (cdr p))
      (make-menu-items (map icon-tab-key (cdr p)) style* #f)
      (make-menu-items (map icon-tab-value (cdr p)) style #f)
    ) ;widget-icon-tabs
  ) ;with
) ;define

(define (make-menu-icon-tab p style)
  "Make @(icon-tab :menu-item-list) menu item."
  (display* "Error 'make-menu-icon-tab', " p ", " style "\n")
  (list 'vlist)
) ;define

(define (make-menu-extend p style bar?)
  "Make @(extend :menu-item :menu-item-list) menu item."
  (with l (make-menu-items (cdr p) style bar?) (widget-extend (car l) (cdr l)))
) ;define

(define (make-menu-style p style bar?)
  "Make @(extend :integer? :menu-item-list) menu item."
  (let* ((x (cadr p))
         (new-style (if (> x 0) (logior style x) (logand style (lognot (- x)))))
        ) ;
    (make-menu-items-list (cddr p) new-style bar?)
  ) ;let*
) ;define

(define (make-menu-minibar p style)
  "Make @(minibar :menu-item-list) menu items."
  (with new-style
    (logior style widget-style-mini)
    (widget-minibar-menu (make-menu-items (cdr p) new-style #t))
  ) ;with
) ;define

(define (make-menu-submenu p style)
  "Make @((:or -> =>) :menu-label :menu-item-list) menu item."
  (with (tag label . items)
    p
    (let ((button ((cond ((== tag '=>) widget-pulldown-button)
                         ((== tag '->) widget-pullright-button)
                   ) ;cond
                   (make-menu-label label style)
                   (object->promise-widget (lambda () (make-menu-widget (list 'vertical items) style))
                   ) ;object->promise-widget
                  ) ;
          ) ;button
         ) ;
      (if (tuple? label 'balloon 2)
        (let* ((text (caddr label))
               (ftxt (translate text))
               (twid (widget-text ftxt style (color "black") #t))
              ) ;
          (widget-balloon button twid)
        ) ;let*
        button
      ) ;if
    ) ;let
  ) ;with
) ;define

(define (make-menu-tile p style)
  "Make @(tile :integer? :menu-item-list) menu item."
  (with (tag width . items)
    p
    (widget-tmenu (make-menu-items items style #f) width)
  ) ;with
) ;define

(define (make-scrollable p style)
  "Make @(scrollable :menu-item-list) item."
  (with (tag . items)
    p
    (with inner
      (make-menu-items (list (cons 'vertical items)) style #f)
      (widget-scrollable (car inner) style)
    ) ;with
  ) ;with
) ;define

(define (decode-resize x default)
  (cond ((string? x) (list x x x default))
        ((list-3? x) (append x (list default)))
        ((list-4? x) x)
        (else (make-menu-error "bad length in " (object->string x)))
  ) ;cond
) ;define

(define global-resize #f)

(define (make-resize p style)
  "Make @(resize :%2 :menu-item-list) item."
  (with (tag w-cmd h-cmd . items)
    p
    (let ((w (w-cmd)) (h (h-cmd)))
      (with (w1 w2 w3 hpos)
        (decode-resize w "left")
        (with (h1 h2 h3 vpos)
          (decode-resize h "top")
          (with-global global-resize
            (list w1 w2 w3 hpos h1 h2 h3 hpos)
            (with inner
              (make-menu-items (list (cons 'vertical items)) style #f)
              (widget-resize (car inner) style w1 h1 w2 h2 w3 h3 hpos vpos)
            ) ;with
          ) ;with-global
        ) ;with
      ) ;with
    ) ;let
  ) ;with
) ;define

(define (make-hsplit p style)
  "Make @(hsplit :menu-item :menu-item) item."
  (with (tag . items)
    p
    (with l (make-menu-items items style #f) (widget-hsplit (car l) (cadr l)))
  ) ;with
) ;define

(define (make-vsplit p style)
  "Make @(vsplit :menu-item :menu-item) item."
  (with (tag . items)
    p
    (with l (make-menu-items items style #f) (widget-vsplit (car l) (cadr l)))
  ) ;with
) ;define

(define (make-ink p style)
  "Make @(ink) item."
  (with (tag cmd) p (widget-ink (object->command cmd)))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dynamic menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (make-menu-if p style bar?)
  "Make @(if :%1 :menu-item-list) menu items."
  (with (tag pred? . items)
    p
    (if (pred?) (make-menu-items-list items style bar?) '())
  ) ;with
) ;define

(define (make-menu-when p style bar?)
  "Make @(when :%1 :menu-item-list) menu items."
  (with (tag pred? . items)
    p
    (let* ((old-active? (== (logand style widget-style-inert) 0))
           (new-active? (and old-active? (pred?)))
           (new-style (logior style (if new-active? 0 (+ widget-style-inert widget-style-grey)))
           ) ;new-style
          ) ;
      (make-menu-items-list items new-style bar?)
    ) ;let*
  ) ;with
) ;define

(define (make-menu-for p style bar?)
  "Make @(for :%1 :%1) menu items."
  (with (tag gen-func vals-promise)
    p
    (let* ((vals (vals-promise)) (items (append-map gen-func vals)))
      (make-menu-items-list items style bar?)
    ) ;let*
  ) ;with
) ;define

(define (make-menu-mini p style bar?)
  "Make @(mini :%1 :menu-item-list) menu items."
  (with (tag pred? . items)
    p
    (let* ((style-maxi (logand style (lognot widget-style-mini)))
           (style-mini (logior style-maxi widget-style-mini))
           (new-style (if (pred?) style-mini style-maxi))
          ) ;
      (make-menu-items-list items new-style bar?)
    ) ;let*
  ) ;with
) ;define

(define (make-menu-link p style bar?)
  "Make @(link :%1) menu items."
  (with linked
   ((eval (cadr p)))
   (if linked
     (make-menu-items linked style bar?)
     (make-menu-error "bad link: " (object->string (cadr p)))
   ) ;if
  ) ;with
) ;define

(define (make-menu-dynamic p style bar?)
  "Make @(dynamic :%1) menu items."
  (with dyn
    (eval (cadr p))
    (if dyn
      (make-menu-items dyn style bar?)
      (make-menu-error "bad link: " (object->string (cadr p)))
    ) ;if
  ) ;with
) ;define

(define (make-menu-promise p style bar?)
  "Make @(promise :%1) menu items."
  (with value
   ((cadr p))
   ;; (make-menu-items value style bar?)
   (if (match? value ':menu-item)
     (make-menu-items value style bar?)
     (make-menu-error "promise did not yield a menu: " value)
   ) ;if
  ) ;with
) ;define

(define (make-refresh p style bar?)
  "Make @(refresh :%1 :string?) widget."
  (with (tag s kind)
    p
    (list (widget-refresh (if (string? s) s (symbol->string s)) kind))
  ) ;with
) ;define

(define (make-refreshable p style bar?)
  "Make @(refreshable :%1 :menu-item-list) menu items."
  (with (tag kind . items)
    p
    (list (widget-refreshable (lambda () (widget-vmenu (make-menu-items-list items style bar?)))
            (kind)
          ) ;widget-refreshable
    ) ;list
  ) ;with
) ;define

(define cached-widgets (make-ahash-table))

(define (make-cached p style bar?)
  "Make @(cached :%1 :menu-item-list) menu items."
  (with (tag kind valid? . items)
    p
    (let* ((kind* (kind))
           (fun (lambda ()
                  (or (and (valid?) (ahash-ref cached-widgets kind*))
                    (let* ((l (make-menu-items-list items style bar?)) (w (widget-vmenu l)))
                      (ahash-set! cached-widgets kind* w)
                      w
                    ) ;let*
                  ) ;or
                ) ;lambda
           ) ;fun
          ) ;
      (list (widget-refreshable fun kind*))
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

(define (make-menu-items-list l style bar?)
  "Make menu items for each element in @l and append results."
  (append-map (lambda (p) (make-menu-items p style bar?)) l)
) ;define

(define (make-menu-items p style bar?)
  "Make menu items @p. The items are on a bar if @bar? and of a given @style."
  ;; (display* "Make items " p ", " style "\n")
  (if (pair? p)
    (cond ((match? p '(input :%1 :string? :%1 :string?)) (list (make-menu-input p style)))
          ((translatable? (car p)) (list (make-menu-entry p style bar?)))
          ((symbol? (car p))
           (with result
             (ahash-ref make-menu-items-table (car p))
             (if (or (not result) (not (match? (cdr p) (car result))))
               (make-menu-items-list p style bar?)
               ((cadr result) p style bar?)
             ) ;if
           ) ;with
          ) ;
          ((match? (car p) ':menu-wide-label) (list (make-menu-entry p style bar?)))
          ((match? (car p) ':tab-page) (list (make-tab-page p style bar?)))
          (else (make-menu-items-list p style bar?))
    ) ;cond
    (cond ((== p '---) (list (make-menu-hsep)))
          ((== p '|) (list (make-menu-vsep)))
          ;; '|
          ((== p '()) p)
          (else (list (make-menu-bad-format p style)))
    ) ;cond
  ) ;if
) ;define

(define-table make-menu-items-table
  (glue (:boolean? :boolean? :integer? :integer?)
    ,(lambda (p style bar?)
       (list (make-menu-glue (second p) (third p) (fourth p) (fifth p))))
  ) ;glue
  (color (:%1 :boolean? :boolean? :integer? :integer?)
    ,(lambda (p style bar?)
       (list (make-menu-color (second p)
               (third p)
               (fourth p)
               (fifth p)
               (sixth p))))
  ) ;color
  (group (:%1) ,(lambda (p style bar?) (list (make-menu-group (cadr p) style))))
  (text (:%1) ,(lambda (p style bar?) (list (make-menu-text (cadr p) style))))
  (invisible (:%1) ,(lambda (p style bar?) (list)))
  (symbol (:string? :*)
    ,(lambda (p style bar?)
       (let ((symbol-color (if (== (get-preference "gui theme") "liii-night")
                             "white"
                             "black")))
         (list (make-menu-symbol p style '() (color symbol-color)))))
  ) ;symbol
  (symbol-completion (:string? :*)
    ,(lambda (p style bar?)
       (let ((symbol-color (if (== (get-preference "gui theme") "liii-night")
                             "white"
                             "black")))
         (list (make-menu-symbol p
                 style
                 '(roman mr medium normal 10 600 0)
                 (color symbol-color)))))
  ) ;symbol-completion
  (symbol-completion* (:string? :*)
    ,(lambda (p style bar?)
       (let ((symbol-color (if (== (get-preference "gui theme") "liii-night")
                             "#ff6666"
                             "red")))
         (list (make-menu-symbol p
                 style
                 '(roman mr medium normal 10 600 -2)
                 (color symbol-color)))))
  ) ;symbol-completion*
  (texmacs-output (:%2)
    ,(lambda (p style bar?) (list (make-texmacs-output p style)))
  ) ;texmacs-output
  (texmacs-input (:%3)
    ,(lambda (p style bar?) (list (make-texmacs-input p style)))
  ) ;texmacs-input
  (input (:%1 :string? :%1 :string?)
    ,(lambda (p style bar?) (list (make-menu-input p style)))
  ) ;input
  (numeric-input (:%1 :string? :string? :integer? :integer? :integer? :integer?)
    ,(lambda (p style bar?) (list (make-numeric-input p style)))
  ) ;numeric-input
  (enum (:%3 :string?) ,(lambda (p style bar?) (list (make-enum p style))))
  (choice (:%3) ,(lambda (p style bar?) (list (make-choice p style))))
  (choices (:%3) ,(lambda (p style bar?) (list (make-choices p style))))
  (filtered-choice (:%4)
    ,(lambda (p style bar?) (list (make-filtered-choice p style)))
  ) ;filtered-choice
  (color-input (:%3) ,(lambda (p style bar?) (list (make-color-input p style))))
  (tree-view (:%3) ,(lambda (p style bar?) (list (make-tree-view p style))))
  (toggle (:%2) ,(lambda (p style bar?) (list (make-toggle p style))))
  (link (:%1) ,(lambda (p style bar?) (make-menu-link p style bar?)))
  (dynamic (:%1) ,(lambda (p style bar?) (make-menu-dynamic p style bar?)))
  (horizontal (:*) ,(lambda (p style bar?)
                      (list (make-menu-horizontal p style))))
  (vertical (:*) ,(lambda (p style bar?) (list (make-menu-vertical p style))))
  (hlist (:*) ,(lambda (p style bar?) (list (make-menu-hlist p style))))
  (vlist (:*) ,(lambda (p style bar?) (list (make-menu-vlist p style))))
  (division (:%1 :*) ,(lambda (p style bar?)
                        (list (make-menu-division p style))))
  (class (:%1 :*) ,(lambda (p style bar?) (list (make-menu-class p style))))
  (aligned (:*) ,(lambda (p style bar?) (list (make-aligned p style))))
  (aligned-item (:%2) ,(lambda (p style bar?)
                         (list (make-aligned-item p style))))
  (tabs (:*) ,(lambda (p style bar?) (list (make-menu-tabs p style))))
  (tab (:*) ,(lambda (p style bar?) (list (make-menu-tab p style))))
  (icon-tabs (:*) ,(lambda (p style bar?) (list (make-menu-icon-tabs p style))))
  (icon-tab (:*) ,(lambda (p style bar?) (list (make-menu-icon-tab p style))))
  (minibar (:*) ,(lambda (p style bar?) (list (make-menu-minibar p style))))
  (extend (:%1 :*)
    ,(lambda (p style bar?) (list (make-menu-extend p style bar?)))
  ) ;extend
  (style (:%1 :*) ,(lambda (p style bar?) (make-menu-style p style bar?)))
  (-> (:%1 :*) ,(lambda (p style bar?) (list (make-menu-submenu p style))))
  (=> (:%1 :*) ,(lambda (p style bar?) (list (make-menu-submenu p style))))
  (tile (:integer? :*) ,(lambda (p style bar?) (list (make-menu-tile p style))))
  (scrollable (:*) ,(lambda (p style bar?) (list (make-scrollable p style))))
  (resize (:%2 :*) ,(lambda (p style bar?) (list (make-resize p style))))
  (hsplit (:%2) ,(lambda (p style bar?) (list (make-hsplit p style))))
  (vsplit (:%2) ,(lambda (p style bar?) (list (make-vsplit p style))))
  (ink (:%1) ,(lambda (p style bar?) (list (make-ink p style))))
  (if (:%1 :*) ,(lambda (p style bar?) (make-menu-if p style bar?)))
  (when (:%1 :*)
    ,(lambda (p style bar?) (make-menu-when p style bar?))
  ) ;when
  (for (:%1 :%1) ,(lambda (p style bar?) (make-menu-for p style bar?)))
  (mini (:%1 :*) ,(lambda (p style bar?) (make-menu-mini p style bar?)))
  (promise (:%1) ,(lambda (p style bar?) (make-menu-promise p style bar?)))
  (refresh (:%1 :string?) ,(lambda (p style bar?) (make-refresh p style bar?)))
  (refreshable (:%1 :*) ,(lambda (p style bar?) (make-refreshable p style bar?)))
  (cached (:%1 :%1 :*) ,(lambda (p style bar?) (make-cached p style bar?)))
) ;define-table

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Menu expansion
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define menu-expand-link-cache (make-ahash-table))

(define (static-menu-link? name)
  "Menus whose expanded result never changes at runtime."
  (in? name
    '(style-menu add-package-menu
       remove-package-menu
       toggle-package-menu
       basic-theme-menu
       document-page-size-menu
       document-language-menu
       document-short-font-menu
       document-font-base-size-menu
       page-rendering-menu
       page-layout-menu
       document-columns-menu
       print-menu-inline
       new-file-menu
       load-menu
       save-menu
       close-menu
       color-menu
       document-encryption-menu
       document-columns-menu)
  ) ;in?
) ;define

(define (menu-expand-link p)
  "Expand menu link @p."
  (let* ((name (cadr p))
         (cached (and (static-menu-link? name) (ahash-ref menu-expand-link-cache name)))
        ) ;
    (if cached
      cached
      (let* ((linked ((eval name))) (result (if linked (menu-expand linked) p)))
        (when (and (static-menu-link? name) linked)
          (ahash-set! menu-expand-link-cache name result)
        ) ;when
        result
      ) ;let*
    ) ;if
  ) ;let*
) ;define

(define (menu-expand-dynamic p)
  "Expand menu link @p."
  (let* ((dyn (eval (cadr p))) (result (if dyn (menu-expand dyn) p)))
    result
  ) ;let*
) ;define

(define (menu-expand-resize p)
  "Expand resize menu @p."
  (with (tag h-cmd v-cmd . items)
    p
    (cons* 'resize (h-cmd) (v-cmd) (menu-expand-list items))
  ) ;with
) ;define

(define (menu-expand-if p)
  "Expand conditional menu @p."
  (with (tag pred? . items) p (if (pred?) (menu-expand-list items) '()))
) ;define

(define (menu-expand-when p)
  "Expand potentially greyed menu @p."
  (with (tag pred? . items)
    p
    (if (pred?)
      (cons* 'when #t (menu-expand-list items))
      (cons* 'when #f (replace-procedures items))
    ) ;if
  ) ;with
) ;define

(define (menu-expand-for p)
  "Expand menu @p generated using for loop."
  (with (tag gen-func vals-promise)
    p
    (let* ((vals (vals-promise)) (items (append-map gen-func vals)))
      (menu-expand-list items)
    ) ;let*
  ) ;with
) ;define

(define (menu-expand-mini p)
  "Expand mini menu @p."
  (with (tag pred? . items) p (cons* 'mini (pred?) (menu-expand-list items)))
) ;define

(define (menu-expand-promise p)
  "Expand promised menu @p."
  (with value
   ((cadr p))
   ;; (menu-expand value)
   (if (match? value ':menu-item) (menu-expand value) p)
  ) ;with
) ;define

(define (menu-expand-texmacs-input p)
  "Expand texmacs-input item @p."
  `(texmacs-input ,(replace-procedures (cadr p))
     ,(replace-procedures (caddr p))
     ,(replace-procedures (cadddr p)))
) ;define

(define (menu-expand-texmacs-output p)
  "Expand output menu item @p."
  (with (tag doc tmstyle) p `(texmacs-output (quote ,(doc)) (quote ,(tmstyle))))
) ;define

(define (menu-expand-input p)
  "Expand input menu item @p."
  `(input ,(replace-procedures (cadr p))
     ,(caddr p)
     ,(with r
        ((cadddr p))
        (if (pair? r) (car r) (replace-procedures (cadddr p))))
     ,(fifth p))
) ;define

(define (menu-expand-enum p)
  "Expand enum item @p."
  `(enum ,(replace-procedures (cadr p))
     ,(replace-procedures (caddr p))
     ,((cadddr p))
     ,(fifth p))
) ;define

(define (menu-expand-choice p)
  "Expand choice item @p."
  `(,(car p) ,(replace-procedures (cadr p)) ,((caddr p)) ,((cadddr p)))
) ;define

(define (menu-expand-filtered-choice p)
  "Expand filtered choice item @p."
  `(,(car p)
    ,(replace-procedures (cadr p))
    ,:proposals
    ;; ,((caddr p))
    ,((cadddr p))
    ,(replace-procedures (car (cddddr p))))
) ;define

(define (menu-expand-color-input p)
  "Expand color-input menu item @p."
  `(input ,(replace-procedures (cadr p))
     ,(caddr p)
     ,(with r
        ((cadddr p))
        (if (pair? r) (car r) (replace-procedures (cadddr p)))))
) ;define

(define (menu-expand-tree-view p)
  "Expand tree-view item @p."
  (display* "menu-expand-tree-view\n")
  `(,(car p) ,(replace-procedures (cadr p)) ,(caddr p) ,(cadddr p))
) ;define

(define (menu-expand-toggle p)
  "Expand toggle item @p."
  `(toggle ,(replace-procedures (cadr p)) ,((caddr p)))
) ;define

(define menu-expand-count 0)

(define (menu-expand-list l)
  "Expand links and conditional menus in list of menus @l."
  (map (lambda (item)
         (set! menu-expand-count (+ menu-expand-count 1))
         (menu-expand item)
       ) ;lambda
    l
  ) ;map
) ;define

(define must-eval-list '(input enum choice filtered-choice toggle))

(define (replace-procedures x)
  (cond ((procedure? x) (procedure-source x))
        ((and (pair? x) (in? (car x) must-eval-list)) (menu-expand x))
        ((list? x) (map replace-procedures x))
        (else x)
  ) ;cond
) ;define

(tm-define (menu-expand p)
  (:type (-> object object))
  (:synopsis "Expand links and conditional menus in menu @p")
  ;; (display* "Expand " p "\n")
  (cond ((npair? p) (replace-procedures p))
        ((string? (car p)) (list (car p)))
        ((symbol? (car p))
         (with result
           (ahash-ref menu-expand-table (car p))
           (if result ((car result) p) p)
         ) ;with
        ) ;
        ((match? (car p) '(check :menu-wide-label :string? :%1))
         (with a
           (cdar p)
           (list (list 'check (menu-expand (car a)) (cadr a) ((caddr a)))
             (replace-procedures (cadr p))
           ) ;list
         ) ;with
        ) ;
        ((match? (car p) '(shortcut :menu-wide-label :string?))
         (with a
           (cdar p)
           (list (list 'shortcut (menu-expand (car a)) (cadr a))
             (replace-procedures (cadr p))
           ) ;list
         ) ;with
        ) ;
        ((match? (car p) ':menu-wide-label) (replace-procedures p))
        (else (menu-expand-list p))
  ) ;cond
) ;tm-define

(tm-define (cache-menu? r)
  (:type (-> object bool))
  (:synopsis "Cache expanded menu @r")
  (cond ((symbol? r) (!= r 'input))
        ((pair? r) (and (cache-menu? (car r)) (cache-menu? (cdr r))))
        (else #t)
  ) ;cond
) ;tm-define

(define-table menu-expand-table
  (--- ,(lambda (p) `(--- ,@(menu-expand-list (cdr p)))))
  (| ,(lambda (p) `(| ,@(menu-expand-list (cdr p)))))
  (group ,replace-procedures)
  (text ,replace-procedures)
  (invisible ,replace-procedures)
  (glue ,replace-procedures)
  (color ,replace-procedures)
  (symbol ,replace-procedures)
  (texmacs-input ,menu-expand-texmacs-input)
  (texmacs-output ,menu-expand-texmacs-output)
  (input ,menu-expand-input)
  (enum ,menu-expand-enum)
  (choice ,menu-expand-choice)
  (choices ,menu-expand-choice)
  (filtered-choice ,menu-expand-filtered-choice)
  (color-input ,menu-expand-color-input)
  (tree-view ,menu-expand-tree-view)
  (toggle ,menu-expand-toggle)
  (link ,menu-expand-link p)
  (dynamic ,menu-expand-dynamic p)
  (horizontal ,(lambda (p) `(horizontal ,@(menu-expand-list (cdr p)))))
  (vertical ,(lambda (p) `(vertical ,@(menu-expand-list (cdr p)))))
  (hlist ,(lambda (p) `(hlist ,@(menu-expand-list (cdr p)))))
  (vlist ,(lambda (p) `(vlist ,@(menu-expand-list (cdr p)))))
  (division ,(lambda (p) `(division ,((cadr p)) ,@(menu-expand-list (cddr p)))))
  (class ,(lambda (p) `(class ,(cadr p) ,@(menu-expand-list (cddr p)))))
  (aligned ,(lambda (p) `(aligned ,@(menu-expand-list (cdr p)))))
  (aligned-item ,(lambda (p) `(aligned-item ,@(menu-expand-list (cdr p)))))
  (tabs ,(lambda (p) `(tabs ,@(menu-expand-list (cdr p)))))
  (tab ,(lambda (p) `(tab ,@(menu-expand-list (cdr p)))))
  (icon-tabs ,(lambda (p) `(icon-tabs ,@(menu-expand-list (cdr p)))))
  (icon-tab ,(lambda (p) `(icon-tab ,@(menu-expand-list (cdr p)))))
  (minibar ,(lambda (p) `(minibar ,@(menu-expand-list (cdr p)))))
  (extend ,(lambda (p) `(extend ,(cadr p) ,@(menu-expand-list (cddr p)))))
  (style ,(lambda (p) `(style ,(cadr p) ,@(menu-expand-list (cddr p)))))
  (-> ,replace-procedures)
  (=> ,replace-procedures)
  (tile ,replace-procedures)
  (scrollable ,(lambda (p) `(scrollable ,@(menu-expand-list (cdr p)))))
  (resize ,menu-expand-resize)
  (hsplit ,(lambda (p) `(hsplit ,@(menu-expand-list (cdr p)))))
  (vsplit ,(lambda (p) `(vsplit ,@(menu-expand-list (cdr p)))))
  (ink ,replace-procedures)
  (if ,menu-expand-if)
  (when ,menu-expand-when
  ) ;when
  (for ,menu-expand-for)
  (mini ,menu-expand-mini)
  (promise ,menu-expand-promise)
  (refresh ,replace-procedures)
  (refreshable ,replace-procedures)
  (cached ,replace-procedures)
) ;define-table

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (make-menu-main p style)
  "Transform the menu @p into a widget."
  (with l
    (make-menu-items p style #f)
    (cond ((null? l) (make-menu-empty))
          ((and (list? l) (null? (cdr l))) (car l))
          (else (make-menu-bad-format p style))
    ) ;cond
  ) ;with
) ;define

(tm-define (make-menu-widget p style)
  (:synopsis "Transform a menu into a widget")
  (:argument p "a scheme object which represents the menu")
  (:argument style "menu style")
  ((wrap-catch make-menu-main) p style)
) ;tm-define

(tm-define (make-menu-widget* p style . opt-size)
  (set! global-resize #f)
  (if (has-markup-gui?)
    (apply make-menu-widget** (cons* p style opt-size))
    (make-menu-widget p style)
  ) ;if
) ;tm-define

(define (decode-options opts)
  (let* ((bufs (list)) (qcmd noop))
    (while (and (pair? opts) (url? (car opts)))
      (set! bufs (cons (car opts) bufs))
      (set! opts (cdr opts))
    ) ;while
    (when (pair? opts)
      (set! qcmd (car opts))
      (set! opts (cdr opts))
    ) ;when
    (list bufs qcmd)
  ) ;let*
) ;define

(define window-deleters (make-ahash-table))

(define (make-window-deleter win bufs)
  (for-each (lambda (buf) (and-with old-del (ahash-ref window-deleters buf) (old-del)))
    bufs
  ) ;for-each
  (with del
    (lambda ()
      (for-each (lambda (buf) (ahash-remove! window-deleters buf)) bufs)
      (alt-window-delete win)
    ) ;lambda
    (for-each (lambda (buf) (ahash-set! window-deleters buf del)) bufs)
    del
  ) ;with
) ;define

(tm-define (top-window menu-promise name . opts)
  (:interactive #t)
  (with (bufs qqq)
    (decode-options opts)
    (let* ((win (alt-window-handle))
           (del (make-window-deleter win bufs))
           (qui (object->command (lambda () (qqq) (del))))
           (men (menu-promise))
           (scm (list 'vertical men))
           (wid (make-menu-widget* scm 0))
          ) ;
      (alt-window-create-quit win wid (translate name) qui)
      (alt-window-show win)
    ) ;let*
  ) ;with
) ;tm-define

(tm-define (dialogue-window menu-promise cmd name . opts)
  (:interactive #t)
  (with (bufs qqq)
    (decode-options opts)
    (let* ((win (alt-window-handle))
           (del (make-window-deleter win bufs))
           (qui (object->command (lambda () (qqq) (del))))
           (lbd (lambda x (apply cmd x) (del)))
           (men (menu-promise lbd))
           (scm (list 'vertical men))
           (wid (make-menu-widget* scm 0))
          ) ;
      (alt-window-create-quit win wid (translate name) qui)
      (alt-window-show win)
    ) ;let*
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Other top-level windows
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (interactive-window wid-promise cmd name)
  (:interactive #t)
  (let* ((win (alt-window-handle))
         (lbd (lambda x (apply cmd x) (alt-window-delete win)))
         (com (object->command (menu-protect lbd)))
         (wid (wid-promise com))
        ) ;
    (alt-window-create-plain win wid (translate name))
    (alt-window-show win)
  ) ;let*
) ;tm-define

(tm-define (interactive-print done u)
  (:interactive #t)
  (with p
    (lambda (com) (widget-printer com u))
    (interactive-window p done "Print document")
  ) ;with
) ;tm-define

(tm-define (interactive-rgb-picker cmd l)
  (:interactive #t)
  (with cmd*
    (lambda (col) (when col (cmd col)))
    (dialogue-window rgb-color-picker cmd* "Choose color")
  ) ;with
) ;tm-define

(tm-define (interactive-color cmd proposals)
  (:interactive #t)
  (set! proposals (map tm->tree proposals))
  (if (not (qt-gui?))
    (interactive-rgb-picker cmd proposals)
    (with p
      (lambda (com) (widget-color-picker com #f proposals))
      (with cmd*
        (lambda (t) (when t (cmd (tm->stree t))))
        (interactive-window p cmd* "Choose color")
      ) ;with
    ) ;with
  ) ;if
) ;tm-define

(tm-define (interactive-background cmd proposals)
  (:interactive #t)
  (set! proposals (map tm->tree proposals))
  (if (not (qt-gui?))
    (interactive-rgb-picker cmd proposals)
    ;; (with p (lambda (com) (widget-color-picker com #t proposals))
    (with p
      (lambda (com) (widget-color-picker com #f proposals))
      (with cmd*
        (lambda (t) (when t (cmd (tm->stree t))))
        (interactive-window p cmd* "Choose background")
      ) ;with
    ) ;with
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Reporting errors of system commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((system-error-widget cmd out err) done)
  (padded (resize '("300px" "600px" "1200px")
            '("275px" "400px" "600px")
            (centered (bold (text "Input command")))
            (scrollable (for (x (string-decompose cmd "\n")) (hlist // (text x) >>)))
            ===
            (centered (bold (text "Standard Output")))
            (scrollable (for (x (string-decompose out "\n")) (hlist // (text x) >>)))
            ===
            (centered (bold (text "Error output")))
            (scrollable (for (x (string-decompose err "\n")) (hlist // (text x) >>)))
            ===
            (bottom-buttons >> ("Ok" (done)))
          ) ;resize
  ) ;padded
) ;tm-widget

(tm-define (report-system-error win-name cmd out err)
  (:synopsis "Display command @cmd with its standard outputs @out and @err")
  (when (list? cmd)
    (set! cmd (string-recompose cmd " "))
  ) ;when
  (set! out (utf8->cork out))
  (set! err (utf8->cork err))
  (dialogue-window (system-error-widget cmd out err) noop win-name)
  #f
) ;tm-define

(tm-widget ((message-widget msg) done)
  (padded (centered (vlist (for (x (string-decompose msg "\n")) (text x))))
    ===
    (centered (explicit-buttons ("Ok" (done))))
  ) ;padded
) ;tm-widget

(tm-define (show-message msg title)
  (:interactive #t)
  (dialogue-window (message-widget msg) noop title)
) ;tm-define

(tm-define (restart-message)
  (:interactive #t)
  (show-message "Restart TeXmacs in order to let changes take effect"
    "Notification"
  ) ;show-message
) ;tm-define

(tm-widget ((notify-dialogue message) cmd)
  (padded (text message) === (bottom-buttons >> ("Ok" (cmd "Ok")) >>))
) ;tm-widget

(tm-define (notify-now message)
  (delayed (:idle 1)
    (dialogue-window (notify-dialogue message) noop "Notification")
  ) ;delayed
) ;tm-define

(tm-define (notify-restart . args)
  (notify-now "Restart TeXmacs in order to let changes take effect")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Attaching global information to widgets and tools
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define global-key-table (make-ahash-table))

(tm-define (global-ref . key) (ahash-ref global-key-table key))

(tm-define (global-set . key-val)
  (ahash-set! global-key-table (cDr key-val) (cAr key-val))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Attaching side tools to windows
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define window-tools-table (make-ahash-table))
(tm-define lazy-tool-table (make-ahash-table))

(define-public-macro (lazy-tool module . tools)
  `(for (tool (quote ,tools))
     (if (pair? tool) (set! tool (car tool)))
     ;; (display* "Lazy tool " tool ", " ',module "\n")
     (ahash-set! lazy-tool-table tool (quote ,module)))
) ;define-public-macro

(define (lazy-tool-force . tools)
  (for (tool tools)
    (if (pair? tool) (set! tool (car tool)))
    ;; (display* "Loading tool " tool "\n")
    (and-with module (ahash-ref lazy-tool-table tool) (eval `(use-modules ,module)))
    (ahash-remove! lazy-tool-table tool)
  ) ;for
) ;define

(tm-define (window->tools win . pos-l)
  (if (null? pos-l)
    (list)
    (with (pos . pos-r)
      pos-l
      (with tools
        (ahash-ref window-tools-table (list win pos))
        (or (and tools (nnull? tools) tools) (apply window->tools (cons win pos-r)))
      ) ;with
    ) ;with
  ) ;if
) ;tm-define

(define (find-positions tool win l)
  (if (null? l)
    l
    (with r
      (find-positions tool win (cdr l))
      (with (x . t)
        l
        (with (key val)
          (if (list-2? x) x (list "" ""))
          (with (key-win key-pos)
            (if (list-2? key) key (list "" ""))
            (if (and (== key-win win) (== val tool)) (cons key-pos r) r)
          ) ;with
        ) ;with
      ) ;with
    ) ;with
  ) ;if
) ;define

(tm-define (tool->positions tool win)
  (with l (ahash-table->list window-tools-table) (find-positions tool win l))
) ;tm-define

(tm-define (tool-bottom? tool win)
  (with l
    (tool->positions tool win)
    (or (in? :transient-bottom l) (in? :bottom l))
  ) ;with
) ;tm-define

(tm-define (tool-side? tool win) (not (tool-bottom? tool win)))

(define (notify-side-tools n show?)
  (when (!= show? (visible-side-tools? n))
    (show-side-tools n show?)
  ) ;when
) ;define

(define (notify-bottom-tools n show?)
  (when (!= show? (visible-bottom-tools? n))
    (show-bottom-tools n show?)
  ) ;when
) ;define

(tm-define (extra-bottom-tools?) #f)

(tm-define (has-bottom-tools? . opt-win)
  (with win
    (if (null? opt-win) (current-window) (car opt-win))
    (with l
      (window->tools win :transient-bottom :bottom)
      (or (== (get-preference "keyboard tool") "on") (extra-bottom-tools?) (nnull? l))
    ) ;with
  ) ;with
) ;tm-define

(tm-define (update-bottom-tools . opt-win)
  (show-bottom-tools 0 (apply has-bottom-tools? opt-win))
  (when (not (extra-bottom-tools?))
    (keyboard-focus-on "canvas")
  ) ;when
) ;tm-define

(tm-define (set-window-tools win pos l)
  (apply lazy-tool-force l)
  (ahash-set! window-tools-table (list win pos) l)
  (let* ((l0 (window->tools win :transient-right :right :bottom-right))
         (l1 (window->tools win :transient-left :left :bottom-left))
        ) ;
    (notify-side-tools 0 (nnull? l0))
    (notify-side-tools 1 (nnull? l1))
    (notify-bottom-tools 0 (has-bottom-tools? win))
    (keyboard-focus-on "canvas")
  ) ;let*
) ;tm-define

(tm-define (set-window-tool win pos tool)
  (set-window-tools win pos (list tool))
) ;tm-define

(tm-define (tool-active? pos tool . opt-win)
  (when (func? tool 'quote)
    (set! tool (cadr tool))
  ) ;when
  (when (string? tool)
    (set! tool (string->symbol tool))
  ) ;when
  (when (symbol? tool)
    (set! tool (list tool))
  ) ;when
  (with win
    (if (null? opt-win) (current-window) (car opt-win))
    (and-with l (window->tools win pos) (in? tool l))
  ) ;with
) ;tm-define

(tm-define (tool-select pos tool . opt-win)
  (:check-mark "v" tool-active?)
  (when (string? tool)
    (set! tool (string->symbol tool))
  ) ;when
  (when (symbol? tool)
    (set! tool (list tool))
  ) ;when
  (with win
    (if (null? opt-win) (current-window) (car opt-win))
    (set-window-tool win pos tool)
  ) ;with
) ;tm-define

(tm-define (tool-focus pos tool u)
  (:check-mark "v" tool-active?)
  (if (tool-active? pos tool)
    (buffer-focus* u #f)
    (begin
      (tool-select pos tool)
      (delayed (:pause 250) (buffer-focus* u #f))
    ) ;begin
  ) ;if
) ;tm-define

(tm-define (tool-toggle pos tool . opt-win)
  (:check-mark "v" tool-active?)
  (when (string? tool)
    (set! tool (string->symbol tool))
  ) ;when
  (when (symbol? tool)
    (set! tool (list tool))
  ) ;when
  (with win
    (if (null? opt-win) (current-window) (car opt-win))
    (with l
      (window->tools win pos)
      (if (in? tool l)
        (set-window-tools win pos (list-remove l tool))
        (set-window-tools win pos (cons tool l))
      ) ;if
    ) ;with
  ) ;with
) ;tm-define

(tm-define (tool-close pos tool quit . opt-win)
  (if (== pos :any)
    (for (pos* (list :transient-right
                 :right
                 :bottom-right
                 :transient-left
                 :left
                 :bottom-left
                 :transient-bottom
                 :bottom
               ) ;list
         ) ;pos*
      (apply tool-close (cons* pos* tool quit opt-win))
    ) ;for
    (let* ((win (if (null? opt-win) (current-window) (car opt-win)))
           (l (window->tools win pos))
           (f (list-filter l (lambda (t) (!= (car t) tool))))
          ) ;
      (when (!= f l)
        (when quit
          (quit)
        ) ;when
        (buffer-focus (window->buffer win) #f)
        (set-window-tools win pos f)
      ) ;when
    ) ;let*
  ) ;if
) ;tm-define

(tm-define ((tool-quit tool quit . opt-win) . args)
  (apply tool-close (cons* :any tool quit opt-win))
) ;tm-define

(tm-define (no-active-tools? pos . opt-win)
  (with win
    (if (null? opt-win) (current-window) (car opt-win))
    (with l (window->tools win pos) (null? l))
  ) ;with
) ;tm-define

(tm-define (close-tools pos . opt-win)
  (:check-mark "v" no-active-tools?)
  (with win
    (if (null? opt-win) (current-window) (car opt-win))
    (set-window-tools win pos (list))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Defining side tools
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (texmacs-side-tool win tool . opts)
  (division "title"
    (text (string-append "Missing '" (object->string (car tool)) "' tool"))
  ) ;division
) ;tm-widget

(define (get-name-tool tool body)
  (cond ((null? body) #f)
        ((keyword? (car body)) (get-name-tool tool (cdr body)))
        ((and (func? (car body) :name 1) (null? (cddr tool))) (cadar body))
        ((func? (car body) :name 1)
         ;; (display* "Name = "
         ;;          `(with (,@(cddr tool)) (cdr tool) ,(cadar body)) "\n")
         `(with (,@(cddr tool)) (cdr tool) ,(cadar body))
        ) ;
        ((not (and (pair? (car body)) (keyword? (caar body)))) #f)
        (else (get-name-tool tool (cdr body)))
  ) ;cond
) ;define

(define (get-quit-tool tool body)
  (cond ((null? body) #f)
        ((keyword? (car body)) (get-quit-tool tool (cdr body)))
        ((func? (car body) :quit 1)
         `(with (,@(cddr tool)) (cdr tool) (lambda ,() ,(cadar body)))
        ) ;
        ((not (and (pair? (car body)) (keyword? (caar body)))) #f)
        (else (get-quit-tool tool (cdr body)))
  ) ;cond
) ;define

(define (finalize-tool body pos)
  (cond ((null? body) (lambda (x) x))
        ((and (== (car body) :side-centered) (== pos :side))
         (with finalize
           (finalize-tool (cdr body) pos)
           (lambda (x) `(centered ,(finalize x)))
         ) ;with
        ) ;
        ((and (== (car body) :bottom-indent) (== pos :bottom))
         (with finalize
           (finalize-tool (cdr body) pos)
           (lambda (x)
             `(hlist (glue #f #f 7 0)
                (vlist === ,(finalize x) ===)
                (glue #f #f 7 0))
           ) ;lambda
         ) ;with
        ) ;
        ((or (keyword? (car body)) (and (pair? (car body)) (keyword? (caar body))))
         (finalize-tool (cdr body) pos)
        ) ;
        (else (lambda (x) x))
  ) ;cond
) ;define

(define (preprocess-tool body)
  (cond ((null? body) body)
        ((or (keyword? (car body)) (and (pair? (car body)) (keyword? (caar body))))
         (preprocess-tool (cdr body))
        ) ;
        (else body)
  ) ;cond
) ;define

(tm-define-macro (tm-tool* tool . obody)
  (let* ((name (get-name-tool tool obody))
         (quit (get-quit-tool tool obody))
         (finalize-side (finalize-tool obody :side))
         (finalize-bottom (finalize-tool obody :bottom))
         (body (preprocess-tool obody))
        ) ;
    ;; (display* "body = " body "\n")
    `(begin
       (tm-widget ,tool ,@body)
       (tm-widget (texmacs-side-tool ,(cadr tool) tool . opts)
         (,:require (== (car tool) (quote ,(car tool))))
         (if (and (in? :title opts) ,name)
           (division ,"title"
             (hlist (text ,name)
               >>
               (division ,"plain"
                 (,"x"
                  (tool-close ,:any (quote ,(car tool)) ,quit ,(cadr tool)))))))
         (assuming (tool-side? tool win)
           ,(finalize-side `(dynamic (,(car tool)
                                      ,(cadr tool)
                                      ,@(map (lambda (i)
                                               `(list-ref tool ,(- i 1)))
                                          (.. 2 (length tool)))))))
         (assuming (tool-bottom? tool win)
           ,(finalize-bottom `(dynamic (,(car tool)
                                        ,(cadr tool)
                                        ,@(map (lambda (i)
                                                 `(list-ref tool ,(- i 1)))
                                            (.. 2 (length tool)))))))))
  ) ;let*
) ;tm-define-macro

(tm-define-macro (tm-tool tool . body)
  `(tm-tool* ,tool ,:side-centered ,:bottom-indent ,@body)
) ;tm-define-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Auxiliary Widget
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 全局哈希表，存储辅助窗口状态

(define auxiliary-widget-table (make-ahash-table))

;; widget-type到action的映射表

(define widget-type->action (make-ahash-table))

;; 注册widget类型和对应的action函数
(tm-define (register-auxiliary-widget-type widget-type action-func)
  (ahash-set! widget-type->action widget-type action-func)
) ;tm-define

;; 设置辅助窗口状态
(tm-define (set-auxiliary-widget-state opened? widget-type)
  (ahash-set! auxiliary-widget-table
    (current-view-url)
    (list opened? widget-type)
  ) ;ahash-set!
) ;tm-define

;; 获取辅助窗口状态
(tm-define (get-auxiliary-widget-state)
  (ahash-ref auxiliary-widget-table (current-view-url))
) ;tm-define

;; 关闭辅助窗口
(tm-define (close-auxiliary-widget)
  (let ((state (get-auxiliary-widget-state)))
    (when state
      (set-auxiliary-widget-state #f (cadr state))
      (show-auxiliary-widget #f)
      (let ((actions (ahash-ref widget-type->action (cadr state))))
        (when (and (pair? actions) (pair? (cdr actions)))
          (with close-action (second actions) (close-action))
        ) ;when
      ) ;let
    ) ;when
  ) ;let
) ;tm-define

;; 刷新辅助窗口
(tm-define (refresh-auxiliary-widget)
  (let ((state (get-auxiliary-widget-state)))
    (cond ((not state) (show-auxiliary-widget #f))
          ;; 状态为空，隐藏辅助窗口
          ((not (car state)) (show-auxiliary-widget #f))
          ;; 第一个是#f，隐藏辅助窗口
          (else (let* ((widget-type (cadr state))
                       (action-list (ahash-ref widget-type->action widget-type))
                      ) ;
                  (if (and action-list (pair? action-list))
                    (with open-action (car action-list) (open-action))
                    (show-auxiliary-widget #f)
                  ) ;if
                ) ;let*
          ) ;else
    ) ;cond
  ) ;let
) ;tm-define
;; 列表为空或未找到，隐藏窗口

(tm-define (auxiliary-widget menu-promise cmd name . opts)
  (:interactive #t)
  (with (bufs qqq)
    (decode-options opts)
    (let* ((del (lambda () (show-auxiliary-widget #f)))
           (qui (object->command (lambda () (qqq) (del))))
           (lbd (lambda x (apply cmd x) (del)))
           (men (menu-promise lbd))
           (scm (list 'vertical men))
           (wid (make-menu-widget* scm 0))
          ) ;
      (set-auxiliary-widget wid name)
      (show-auxiliary-widget #t)
    ) ;let*
  ) ;with
) ;tm-define
