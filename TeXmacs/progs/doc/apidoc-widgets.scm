
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : apidoc-widgets.scm
;; DESCRIPTION : Widgets for the API doc. system
;; COPYRIGHT   : (C) 2012 Miguel de Benito Delgado
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; The contents of this file are preliminary and simple. Things TO-DO are:
;;  - Use gui:help-window-visible in init-texmacs.scm (or elsewhere)
;;  - Move generic window and preference handling procedures elsewhere
;;  - this list
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (doc apidoc-widgets)
  (:use (doc apidoc-funcs) (doc apidoc-collect) (kernel texmacs tm-preferences))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; A contextual help widget
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define help-win-handle 0)

(define (window-get-geometry id)
  (with (w h)
    (alt-window-get-size id)
    (with (x y) (alt-window-get-position id) `(,x ,y ,w ,h))
  ) ;with
) ;define

(define (window-set-geometry id geom)
  (alt-window-set-position id (car geom) (cadr geom))
  (alt-window-set-size id (caddr geom) (cadddr geom))
) ;define

(define (document? t)
  (tree-is? t 'document)
) ;define

(define (help-win-tree)
  (with ids
    (id->trees "__doc__popup__")
    (if (null? ids)
      (tree "")
      (with t (tree-search-upwards (car ids) document?) (if t t (tree "")))
    ) ;if
  ) ;with
) ;define

(define (help-win-active?)
  (and (> help-win-handle 0) (not (tree-empty? (help-win-tree))))
) ;define

(define (help-win-display where key)
  (set-preference "gui:help-win-viewing" (list where key))
  (with t
    (help-win-tree)
    (cond ((== where "scheme") (tree-set! t ($doc-explain-scm key)))
          ((== where "macros") (tree-set! t ($doc-explain-macro key)))
          (else (tree-set! t ($para (replace "Error: %1" "help-win-display"))))
    ) ;cond
  ) ;with
) ;define

(define (help-win-show)
  (with geo
    (get-preference "gui:help-window-geometry")
    (if (list? geo) (window-set-geometry help-win-handle geo))
  ) ;with
  (alt-window-show help-win-handle)
  (set-preference "gui:help-window-visible" #t)
) ;define

(define (help-win-hide)
  (alt-window-hide help-win-handle)
  (set-preference "gui:help-window-visible" #f)
  (set-preference "gui:help-window-geometry"
    (window-get-geometry help-win-handle)
  ) ;set-preference
) ;define

(define (help-win-delete)
  (alt-window-delete help-win-handle)
) ;define

(define (help-win-activate win)
  (set! help-win-handle win)
) ;define

(tm-define (doc-widget where key)
  (cond ((== where "scheme")
         (menu-dynamic (resize '("300px" "600px" "9999px")
                         '("200px" "300px" "9999px")
                         (texmacs-input ($doc-explain-scm key) '(style "tmdoc") #f)
                       ) ;resize
         ) ;menu-dynamic
        ) ;
        ((== where "macros")
         (menu-dynamic (resize '("300px" "600px" "9999px")
                         '("200px" "300px" "9999px")
                         (texmacs-input ($doc-explain-macro key) '(style "tmdoc") #f)
                       ) ;resize
         ) ;menu-dynamic
        ) ;
        (else (menu-dynamic ("ERROR: unknown documentation set" (help-win-hide))))
  ) ;cond
) ;tm-define

(tm-define (help-window where key)
  (:synopsis "Display the help window for @key in the @where documentation")
  (if (and (string? key) (!= key ""))
    (if (help-win-active?)
      (begin
        (help-win-display where key)
        (help-win-show)
      ) ;begin
      (let* ((win (alt-window-handle))
             (qui (object->command (lambda () (help-win-hide))))
             (men (doc-widget where key))
             (scm (list 'vertical men))
             (wid (make-menu-widget scm 0))
             (geo (get-preference "gui:help-window-geometry"))
            ) ;
        (alt-window-create-quit win wid "Documentation" qui)
        (alt-window-show win)
        (if (list? geo) (window-set-geometry win geo))
        (help-win-activate win)
      ) ;let*
    ) ;if
  ) ;if
) ;tm-define

;;;;; Preference handling for the help widget

(define (notify-help-win-preference pref val)
  (cond ((== pref "gui:help-window-geometry")
         (if (> help-win-handle 0) (window-set-geometry help-win-handle val))
        ) ;
        ((== pref "gui:help-window-viewing")
         (with (where key) val (help-window where key))
        ) ;
        ((== pref "gui:help-window-visible") (noop))
  ) ;cond
) ;define

(define-preferences ("gui:help-window-geometry" '(400 -400 400 300) notify-help-win-preference)
 ("gui:help-window-viewing" '("" "") notify-help-win-preference)
 ("gui:help-window-visible" #f notify-help-win-preference)
) ;define-preferences

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; module browser widget
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define mw-module "")
(tm-define mw-symbol "")
(tm-define mw-module-filter "")
(tm-define mw-symbol-filter "")

(tm-define (mw-all-modules)
  (map module->string (list-submodules-recursive '(())))
) ;tm-define

(tm-define (mw-all-symbols)
  (map symbol->string (or (module-exported (string->module mw-module)) '()))
) ;tm-define

(tm-widget (symbol-doc-widget)
  (resize '("200px" "400px" "9000px")
    '("100px" "200px" "3000px")
    (texmacs-input ($doc-explain-scm mw-symbol) '(style "tmdoc") #f)
  ) ;resize
) ;tm-widget

(tm-widget (symbol-doc-buttons)
  (explicit-buttons >>
   ("Insert template"
     (insert ($doc-symbol-template (string->symbol mw-symbol) #f ""))
   ) ;
  ) ;explicit-buttons
) ;tm-widget

(tm-widget (module-list-widget)
  (vertical (bold (text "Module"))
    (filtered-choice (begin
                       (set! mw-module answer)
                       (set! mw-symbol "")
                       (set! mw-module-filter filter)
                     ) ;begin
      (mw-all-modules)
      mw-module
      mw-module-filter
    ) ;filtered-choice
  ) ;vertical
) ;tm-widget

(tm-widget (module-symbols-widget)
  (vertical (bold (text "Symbols"))
    (filtered-choice (begin
                       (set! mw-symbol answer)
                       (set! mw-symbol-filter filter)
                     ) ;begin
      (mw-all-symbols)
      mw-symbol
      mw-symbol-filter
    ) ;filtered-choice
  ) ;vertical
) ;tm-widget

(tm-widget (module-browser)
  (vertical (hsplit (resize '("200px" "300px" "4000px")
                      '("300px" "500px" "4000px")
                      (link module-list-widget)
                    ) ;resize
              (refresh module-symbols-widget auto)
            ) ;hsplit
  ) ;vertical
) ;tm-widget

(tm-define (open-module-browser)
  (set! mw-module "")
  (set! mw-symbol "")
  (set! mw-module-filter "")
  (set! mw-symbol-filter "")
  (top-window module-browser "Module browser")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; symbol browser widget
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define clw-string "")
(tm-define clw-string-filter "")

(tm-widget (symbol-browser-widget)
  (vertical (bold (text "Symbol"))
    (resize '("200px" "350px" "4000px")
      '("300px" "600px" "4000px")
      (filtered-choice (begin
                         (set! clw-string answer)
                         (set! clw-string-filter filter)
                       ) ;begin
        (list-sort (scheme-completions-dump) string<?)
        clw-string
        clw-string-filter
      ) ;filtered-choice
    ) ;resize
    (explicit-buttons ("See documentation" (help-window "scheme" (tmstring->string clw-string)))
    ) ;explicit-buttons
  ) ;vertical
) ;tm-widget

(tm-define (open-symbol-browser)
  (scheme-completions-rebuild)
  (set! clw-string "")
  (set! clw-string-filter "")
  (top-window symbol-browser-widget "Symbol browser")
) ;tm-define
