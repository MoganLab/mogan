
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : generic-menu.scm
;; DESCRIPTION : default focus menu
;; COPYRIGHT   : (C) 2010  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic generic-menu)
  (:use (utils edit variants)
    (utils edit selections)
    (generic generic-edit)
    (generic format-edit)
    (generic format-geometry-edit)
    (generic document-edit)
    (source source-edit)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Variants
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (focus-variants-of t) (variants-of (tree-label t)))

(tm-define (focus-tag-name l)
  (let* ((s (symbol->string l)) (th (member->theme s)))
    (if th
      (with ns
        (string-drop s (+ (string-length th) 1))
        (focus-tag-name (string->symbol ns))
      ) ;with
      (if (symbol-unnumbered? l)
        (focus-tag-name (symbol-drop-right l 1))
        (with r (tree-name (tree l)) (string-replace r "-" " "))
      ) ;if
    ) ;if
  ) ;let*
) ;tm-define

(tm-menu (focus-variant-menu t)
  (push-focus t
    (for (v (focus-variants-of t))
     ((eval (focus-tag-name v)) (pull-focus t (variant-set-keep-numbering t v)))
    ) ;for
  ) ;push-focus
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for hidden fields
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (string-variable-name? t i)
  (and (== (tree-child-type t i) "variable")
    (tree-in? t '(with attr style-with style-with*))
    (tree-atomic? (tree-ref t i))
    (!= (tree->stree (tree-ref t i)) "")
  ) ;and
) ;tm-define

(tm-define (hidden-child? t i)
  (and (not (tree-accessible-child? t i))
    (not (string-variable-name? t i))
    (!= (type->format (tree-child-type t i)) "n.a.")
  ) ;and
) ;tm-define

(tm-define (child-proposals t i) #f)

(define (hidden-children t)
  (with fun
    (lambda (i) (if (hidden-child? t i) (list (tree-ref t i)) (list)))
    (append-map fun (.. 0 (tree-arity t)))
  ) ;with
) ;define

(define (tree-child-name* t i)
  (with s
    (tree-child-name t i)
    (cond ((!= s "") s)
          ((and (> i 0) (string-variable-name? t (- i 1)))
           (with r (tree->string (tree-ref t (- i 1))) (string-replace r "-" " "))
          ) ;
          ((> (length (hidden-children t)) 1) "")
          ((== (tree-child-type t i) "regular") "")
          (else (tree-child-type t i))
    ) ;cond
  ) ;with
) ;define

(define (tree-child-long-name* t i)
  (with s
    (tree-child-long-name t i)
    (cond ((!= s "") s)
          ((and (> i 0) (string-variable-name? t (- i 1)))
           (with r (tree->string (tree-ref t (- i 1))) (string-replace r "-" " "))
          ) ;
          ((> (length (hidden-children t)) 1) "")
          ((== (tree-child-type t i) "regular") "")
          (else (tree-child-type t i))
    ) ;cond
  ) ;with
) ;define

(define (type->format type)
  (cond ((== type "adhoc") "n.a.")
        ((== type "raw") "n.a.")
        ((== type "url")
         ;; FIXME: filename editing is way too slow in Qt and
         ;; tab completion does not seem to work anyway
         (if (qt-gui?) "string" "smart-file")
        ) ;
        ((== type "graphical") "n.a.")
        ((== type "point") "n.a.")
        ((== type "obsolete") "n.a.")
        ((== type "unknown") "n.a.")
        ((== type "error") "n.a.")
        (else "string")
  ) ;cond
) ;define

(define (type->width type)
  (cond ((== type "boolean") "5em")
        ((== type "integer") "5em")
        ((== type "length") "5em")
        ((== type "numeric") "5em")
        ((== type "identifier") "8em")
        ((== type "duration") "5em")
        (else "1w")
  ) ;cond
) ;define

(tm-define (inputter-active? t type)
  (cond ((== type "length") (tm-rich-length? t))
        (else (tree-atomic? t))
  ) ;cond
) ;tm-define

(tm-define (inputter-decode t type)
  (cond ((== type "length") (tm->rich-length t))
        (else (tree->string t))
  ) ;cond
) ;tm-define

(tm-define (inputter-encode s type)
  (cond ((== type "length") (rich-length->tm s))
        (else s)
  ) ;cond
) ;tm-define

(tm-menu (string-input-name t i)
  (let* ((name (tree-child-name* t i)) (s (string-append (upcase-first name) ":")))
    (assuming (== name "") //)
    (assuming (!= name "") (glue #f #f 3 0) (mini #t (group (eval s))))
  ) ;let*
) ;tm-menu

(tm-menu (string-input-icon t i)
  (push-focus t
    (let* ((name (tree-child-name* t i))
           (type (tree-child-type t i))
           (s (string-append (upcase-first name) ":"))
           (active? (inputter-active? (tree-ref t i) type))
           (props (child-proposals t i))
           (show-verbatim? (and props
                             (list-find props (lambda (x) (and (list? x) (== (car x) 'verbatim))))
                           ) ;and
           ) ;show-verbatim?
           (in (if active? (inputter-decode (tree-ref t i) type) "n.a."))
           (fm (type->format type))
           (w (type->width type))
           (setter (lambda (x)
                     (pull-focus t
                       (when x
                         (tree-set t i (inputter-encode x type))
                         (focus-tree-modified t)
                       ) ;when
                     ) ;pull-focus
                   ) ;lambda
           ) ;setter
          ) ;
      (dynamic (string-input-name t i))
      (assuming props
        (mini #t
          (=> (eval (if show-verbatim? (list 'verbatim in) in))
            (for (prop props)
              (cond ((string? prop)
                     (let ((eval-result (eval prop)))
                       ;; 处理verbatim表达式
                       (if (and (list? eval-result) (== (car eval-result) 'verbatim))
                         (eval-result (setter (cadr eval-result)))
                         (eval-result (setter prop))
                       ) ;if
                     ) ;let
                    ) ;
                    ((and (list? prop) (== (car prop) 'verbatim))
                     ((eval prop) (setter (cadr prop)))
                    ) ;
                    ((== prop :other)
                     ---
                     ("Other" (interactive setter (list (upcase-first name) fm in)))
                    ) ;
              ) ;cond
            ) ;for
          ) ;=>
        ) ;mini
      ) ;assuming
      (assuming (not props)
        (when active?
          (mini #t (input (setter answer) fm (list in) w))
        ) ;when
      ) ;assuming
    ) ;let*
  ) ;push-focus
) ;tm-menu

(tm-menu (string-input-icon t i)
  (:require (== (tree-child-type t i) "color"))
  (push-focus t
    (let* ((name (tree-child-name* t i))
           (s (string-append (upcase-first name) ":"))
           (active? (inputter-active? (tree-ref t i) "color"))
           (in (if active? (inputter-decode (tree-ref t i) "color") ""))
           (setter (lambda (x)
                     (pull-focus t
                       (when x
                         (tree-set t i (inputter-encode x "color"))
                         (focus-tree-modified t)
                       ) ;when
                     ) ;pull-focus
                   ) ;lambda
           ) ;setter
          ) ;
      (dynamic (string-input-name t i))
      (=> (color (tree->stree (tree-ref t i)) #f #f 24 16)
        (pick-background "" (setter answer))
        ---
        ("Palette" (interactive-color setter '()))
        ("Pattern" (open-pattern-selector setter "1cm"))
        ("Gradient" (open-gradient-selector setter))
        ("Picture" (open-background-picture-selector setter))
        ("Other" (interactive setter (list (upcase-first name) "color" in)))
      ) ;=>
    ) ;let*
  ) ;push-focus
) ;tm-menu

(tm-define (child-proposals t i)
  (:require (== (tree-child-type t i) "duration"))
  (list "0.25s" "0.5s" "1s" "1.5s" "2s" "2.5s" "3s" "4s" "5s" "10s" :other)
) ;tm-define

(tm-menu (string-input-menu t i)
  (push-focus t
    (let* ((name (tree-child-long-name* t i))
           (s `(concat ,"Set " ,name))
           (prompt (upcase-first name))
           (type (tree-child-type t i))
           (fm (type->format type))
           (active? (inputter-active? (tree-ref t i) type))
           (props (child-proposals t i))
           (show-verbatim? (and props
                             (list-find props (lambda (x) (and (list? x) (== (car x) 'verbatim))))
                           ) ;and
           ) ;show-verbatim?
           (in (if active? (inputter-decode (tree-ref t i) type) "n.a."))
           (setter (lambda (x)
                     (pull-focus t
                       (when x
                         (tree-set t i (inputter-encode x type))
                         (focus-tree-modified t)
                       ) ;when
                     ) ;pull-focus
                   ) ;lambda
           ) ;setter
          ) ;
      (assuming (!= name "")
        (assuming props
          (=> (eval (if show-verbatim? (list 'verbatim s) s))
            (for (prop props)
              (cond ((string? prop)
                     (let ((eval-result (eval prop)))
                       ;; 处理verbatim表达式
                       (if (and (list? eval-result) (== (car eval-result) 'verbatim))
                         (eval-result (setter (cadr eval-result)))
                         (eval-result (setter prop))
                       ) ;if
                     ) ;let
                    ) ;
                    ((and (list? prop) (== (car prop) 'verbatim))
                     ((eval prop) (setter (cadr prop)))
                    ) ;
                    ((== prop :other)
                     ---
                     ("Other" (interactive setter (list (upcase-first name) fm in)))
                    ) ;
              ) ;cond
            ) ;for
          ) ;=>
        ) ;assuming
        (assuming (not props)
          (when (inputter-active? (tree-ref t i) type)
            ((eval s)
             (interactive setter (list prompt fm (inputter-decode (tree-ref t i) type)))
            ) ;
          ) ;when
        ) ;assuming
      ) ;assuming
    ) ;let*
  ) ;push-focus
) ;tm-menu

(tm-menu (string-input-icon t i)
  (:require (string-variable-name? t i))
  (with c
    (tree-ref t i)
    (with s
      (if (tree-atomic? c) (tree->string c) "n.a.")
      (glue #f #f 3 0)
      (mini #t (group (eval (string-append s ":"))))
    ) ;with
  ) ;with
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Unified accessors for local and global parameters
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parameter-test? l val mode)
  (cond ((not (tm? val)) #f)
        ((== mode :global) (== (get-init-tree l) (tm->tree val)))
        ((and (func? mode :local) (tree-is? (focus-tree) (cadr mode)))
         (== (tree-with-get (focus-tree) l) (tm->tree val))
        ) ;
        (else #f)
  ) ;cond
) ;tm-define

(tm-define (parameter-set l val mode)
  (cond ((not (tm? val)) (noop))
        ((== mode :global) (set-init-env l val))
        ((and (func? mode :local) (parameter-local-special-set l val mode)) (noop))
        ((and (func? mode :local) (tree-is? (focus-tree) (cadr mode)))
         (with-set (focus-tree) l val)
        ) ;
  ) ;cond
) ;tm-define

(tm-define (parameter-interactive-set l mode)
  (:interactive #t)
  (interactive (lambda (s) (parameter-set l s mode))
    (list (or (logic-ref env-var-description% l) l) "string" (parameter-get l mode))
  ) ;interactive
) ;tm-define

(tm-define (parameter-interactive-set l mode)
  (:require (and (tree-label-macro? (string->symbol l))
              (not (tm-atomic? (parameter-get l mode)))
            ) ;and
  ) ;:require
  (open-macro-editor l mode)
) ;tm-define

(define (parameter-get* l mode)
  (cond ((== mode :global) (tm->stree (get-init-tree l)))
        ((func? mode :local) (tm->stree (get-env-tree l)))
        (else "")
  ) ;cond
) ;define

(tm-define (parameter-get l mode)
  (with t
    (parameter-get* l mode)
    (if (and (tm-func? t 'macro 1) (tm-atomic? (tm-ref t 0))) (tm-ref t 0) t)
  ) ;with
) ;tm-define

(tm-define (parameter-get-string l mode) (force-string (parameter-get l mode)))

(tm-define (parameter-default? l mode)
  (cond ((== mode :global) (not (init-has? l)))
        ((and (func? mode :local) (tree-is? (focus-tree) (cadr mode)))
         (not (tree-with-get (focus-tree) l))
        ) ;
        (else #f)
  ) ;cond
) ;tm-define

(tm-define (parameter-reset l mode)
  (cond ((== mode :global) (init-default-one l))
        ((and (func? mode :local) (tree-is? (focus-tree) (cadr mode)))
         (tree-with-reset (focus-tree) l)
        ) ;
  ) ;cond
) ;tm-define

(tm-define (parameter-enabled? l mode) (parameter-test? l "true" mode))

(tm-define (parameter-toggle l mode)
  (:check-mark "*" parameter-enabled?)
  (with new
    (if (== (parameter-get l mode) "true") "false" "true")
    (parameter-reset l mode)
    (delayed (when (!= new (parameter-get l mode)) (parameter-set l new mode)))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Submenus for editing various types of style parameters
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (parameter-value? c)
  (or (string? c) (and (list-2? c) (string? (car c))))
) ;define

(tm-menu (parameter-choice-menu l cs mode)
  (with ss
    (list-filter cs parameter-value?)
    ((check "Default" "*" (parameter-default? l mode)) (parameter-reset l mode))
    (if (nnull? ss)
      ---
      (for (c ss)
        (assuming (string? c)
         ((check (eval (upcase-first c)) "*" (parameter-test? l c mode))
          (parameter-set l c mode)
         ) ;
        ) ;assuming
        (assuming (and (list-2? c) (string? (car c)))
         ((check (eval (car c)) "*" (parameter-test? l (cadr c) mode))
          (parameter-set l (cadr c) mode)
         ) ;
        ) ;assuming
      ) ;for
    ) ;if
    (if (and (nnull? ss) (in? :other cs)) ---)
    (if (in? :other cs) ("Other" (parameter-interactive-set l mode)))
  ) ;with
) ;tm-menu

(tm-menu (parameter-submenu l mode)
  (dynamic (parameter-choice-menu l (list :other) mode))
) ;tm-menu

(tm-menu (parameter-submenu l mode)
  (:require (== (tree-label-type (string->symbol l)) "color"))
  (with setter
    (lambda (col)
      (delayed (:idle 250)
        (parameter-set l col mode)
        (when (or (== l "text-bg-color") (== l "marked-color"))
          (set-preference "marked-color" col)
        ) ;when
      ) ;delayed
    ) ;lambda
    ((check "None" "*" (parameter-default? l mode)) (parameter-reset l mode))
    ---
    (pick-background "" (setter answer))
    ---
    (if (in? l (list "locus-color" "visited-color"))
     ((check "Preserve" "*" (parameter-test? l "preserve" mode))
      (parameter-set l "preserve" mode)
     ) ;
    ) ;if
    ("Palette" (interactive-color setter '()))
    ("Pattern" (open-pattern-selector setter "1cm"))
    ("Gradient" (open-gradient-selector setter))
    ("Picture" (open-background-picture-selector setter))
    ("Other" (parameter-interactive-set l mode))
  ) ;with
) ;tm-menu

(tm-menu (parameter-submenu l mode)
  ;; (:require (== (tree-label-type (string->symbol l)) "font"))
  (:require (string-ends? l "-font"))
  ((check "Default" "*" (parameter-default? l mode)) (parameter-reset l mode))
  ---
  ((check "Roman" "*" (parameter-test? l "roman" mode))
   (parameter-set l "roman" mode)
  ) ;
  ((check "Stix" "*" (parameter-test? l "stix" mode))
   (parameter-set l "stix" mode)
  ) ;
  ((check "Bonum" "*" (parameter-test? l "bonum" mode))
   (parameter-set l "bonum" mode)
  ) ;
  ((check "Pagella" "*" (parameter-test? l "pagella" mode))
   (parameter-set l "pagella" mode)
  ) ;
  ((check "Schola" "*" (parameter-test? l "schola" mode))
   (parameter-set l "schola" mode)
  ) ;
  ((check "Termes" "*" (parameter-test? l "termes" mode))
   (parameter-set l "termes" mode)
  ) ;
  ---
  (with prefix
    (string-drop-right l 4)
    ("Other" (open-document-other-font-selector prefix))
  ) ;with
) ;tm-menu

(tm-menu (parameter-submenu l mode)
  (:require (== (tree-label-type (string->symbol l)) "font-size"))
  ((check "Default" "*" (parameter-default? l mode)) (parameter-reset l mode))
  ---
  ((check "Small" "*" (parameter-test? l "0.841" mode))
   (parameter-set l "0.841" mode)
  ) ;
  ((check "Normal" "*" (parameter-test? l "1" mode)) (parameter-set l "1" mode))
  ((check "Large" "*" (parameter-test? l "1.189" mode))
   (parameter-set l "1.189" mode)
  ) ;
  ((check "Very large" "*" (parameter-test? l "1.414" mode))
   (parameter-set l "1.414" mode)
  ) ;
  ((check "Huge" "*" (parameter-test? l "1.682" mode))
   (parameter-set l "1.682" mode)
  ) ;
  ---
  ("Other" (parameter-interactive-set l mode))
) ;tm-menu

(tm-menu (parameter-submenu l mode)
  (:require (parameter-choice-list l))
  (with cs (parameter-choice-list l) (dynamic (parameter-choice-menu l cs mode)))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Editing style parameters
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parameter-name l)
  (focus-tag-name (string->symbol (tree-name (list (string->symbol l)))))
) ;tm-define

(tm-menu (focus-parameter-menu-item l mode)
 ((eval (parameter-name l)) (open-macro-editor l mode))
) ;tm-menu

(tm-menu (focus-parameter-menu-item l mode)
  (:require (and (tree-label-parameter? (string->symbol l))
              (string? (parameter-get l mode))
              (nin? (tree-label-type (string->symbol l)) (list "unknown" "regular" "adhoc"))
            ) ;and
  ) ;:require
  (-> (eval (focus-tag-name (string->symbol l)))
    (dynamic (parameter-choice-menu l (list :other) mode))
  ) ;->
) ;tm-menu

(tm-menu (focus-parameter-menu-item l mode)
  (:require (and (tree-label-parameter? (string->symbol l))
              (string? (parameter-get l mode))
              (== (tree-label-type (string->symbol l)) "boolean")
            ) ;and
  ) ;:require
  ((check (eval (focus-tag-name (string->symbol l)))
     "v"
     (== (parameter-get l mode) "true")
   ) ;check
   (parameter-toggle l mode)
  ) ;
) ;tm-menu

(tm-menu (focus-parameter-menu-item l mode)
  (:require (and (tree-label-parameter? (string->symbol l))
              (or (== (tree-label-type (string->symbol l)) "color")
                ;; (== (tree-label-type (string->symbol l)) "font")
                (string-ends? l "-font")
                (== (tree-label-type (string->symbol l)) "font-size")
              ) ;or
            ) ;and
  ) ;:require
  (-> (eval (focus-tag-name (string->symbol l)))
    (dynamic (parameter-submenu l mode))
  ) ;->
) ;tm-menu

(tm-menu (focus-parameter-menu-item l mode)
  (:require (parameter-choice-list l))
  (-> (eval (focus-tag-name (string->symbol l)))
    (dynamic (parameter-submenu l mode))
  ) ;->
) ;tm-menu

(tm-define (parameter-show-in-menu? l) (not (member->theme l)))

(tm-define (parameter-show-in-menu? l)
  (:require (in? l
              (list "the-label"
                "auto-nr"
                "current-part"
                "language"
                "page-nr"
                "page-the-page"
                "prog-language"
                "caption-summarized"
                "figure-width"
              ) ;list
            ) ;in?
  ) ;:require
  #f
) ;tm-define

(define (focus-parameters-list t mode)
  (let* ((ls (list-filter (search-parameters (tree-label t)) parameter-show-in-menu?))
         (xs (if (== mode :global) (list) (map car (customizable-parameters-memo t))))
         (no (if (== mode :global) inhibit-global-table inhibit-local-table))
        ) ;
    (list-filter (list-difference ls xs) (lambda (x) (not (ahash-ref no x))))
  ) ;let*
) ;define

(define parameters-list-cache (make-ahash-table))

(define (focus-parameters-list-memo t mode)
  (with key
    (list (tree-label t) mode (tree->stree (get-style-tree)))
    (when (not (ahash-ref parameters-list-cache key))
      (ahash-set! parameters-list-cache key (focus-parameters-list t mode))
    ) ;when
    (ahash-ref parameters-list-cache key)
  ) ;with
) ;define

(tm-define (style-clear-cache)
  (former)
  (set! parameters-list-cache (make-ahash-table))
) ;tm-define

(tm-menu (focus-parameters-menu t mode)
  (with base
    (focus-parameters-list-memo t mode)
    (with ps
      (rendering-parameters-merge t base mode)
      (if (nnull? ps)
        (group "Style parameters")
        (for (p ps) (dynamic (focus-parameter-menu-item p mode)))
        (if (tree-label-extension? (tree-label t)) ---)
      ) ;if
    ) ;with
  ) ;with
) ;tm-menu

(tm-menu (focus-theme-parameters-submenu th mode)
  (with mems
    (theme->members th)
    (for (mem mems)
      (with var
        (string-append th "-" mem)
        (dynamic (focus-parameter-menu-item var mode))
      ) ;with
    ) ;for
  ) ;with
) ;tm-menu

(tm-menu (focus-theme-parameters-menu t mode)
  (with ths
    (search-tag-themes t)
    (if (nnull? ths)
      (group "Theme parameters")
      (for (th ths) (-> (eval th) (dynamic (focus-theme-parameters-submenu th mode))))
      ---
    ) ;if
  ) ;with
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The main Focus menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-ancestor-menu t))

(tm-menu (focus-toggle-menu t)
  (push-focus t
    (assuming (numbered-context? t)
      ;; FIXME: itemize, enumerate, eqnarray*
      ((check "Numbered" "v" (pull-focus t (numbered-numbered? t)))
       (pull-focus t (numbered-toggle t))
      ) ;
    ) ;assuming
    (assuming (alternate-context? t)
     ((check (eval (alternate-second-name t))
        "v"
        (pull-focus t (alternate-second? t))
      ) ;check
      (pull-focus t (alternate-toggle t))
     ) ;
    ) ;assuming
    (assuming (!= (tree-children t) (tree-accessible-children t))
     ((check "Show hidden" "v" (pull-focus t (tree-is? t :up 'inactive)))
      (pull-focus t (inactive-toggle t))
     ) ;
    ) ;assuming
  ) ;push-focus
) ;tm-menu

(tm-menu (focus-float-menu t))
(tm-menu (focus-animate-menu t))
(tm-menu (focus-misc-menu t))

(tm-menu (focus-style-options-menu t)
  (with opts
    (search-tag-options t)
    (if (nnull? opts)
      (group "Style options")
      (for (opt opts)
       ((check (balloon (eval (style-get-menu-name opt)) (eval (style-get-documentation opt)))
          "v"
          (has-style-package? opt)
        ) ;check
        (toggle-style-package opt)
       ) ;
      ) ;for
      (if (tree-label-extension? (tree-label t)) ---)
    ) ;if
  ) ;with
) ;tm-menu

(tm-menu (focus-tag-edit-menu l)
  (if (tree-label-extension? l)
    (let* ((s (symbol->string l)) (cmd (string-append "(make '" s ")")))
      (when (editable-macro? l)
        ("Edit macro" (edit-focus-macro))
      ) ;when
      (when (has-macro-source? l)
        ("Edit source" (edit-focus-macro-source))
      ) ;when
      (assuming (not (has-user-shortcut? cmd))
       ("Create shortcut" (open-shortcuts-editor "" cmd))
      ) ;assuming
      (assuming (has-user-shortcut? cmd)
       ("Edit shortcut" (open-shortcuts-editor "" cmd))
      ) ;assuming
    ) ;let*
  ) ;if
) ;tm-menu

(tm-menu (focus-tag-customize-menu l)
  (if (tree-label-extension? l)
    (when (editable-macro? l)
      ("Customize macro" (open-macro-editor l (list :local l)))
    ) ;when
  ) ;if
) ;tm-menu

(tm-menu (focus-preferences-menu t)
  (dynamic (focus-style-options-menu t))
  (dynamic (focus-parameters-menu t :global))
  (dynamic (focus-theme-parameters-menu t :global))
  (dynamic (focus-tag-edit-menu (tree-label t)))
) ;tm-menu

(tm-menu (focus-rendering-menu t)
  (dynamic (focus-parameters-menu t (list :local (tree-label t))))
  (dynamic (focus-theme-parameters-menu t (list :local (tree-label t))))
  (dynamic (focus-tag-customize-menu (tree-label t)))
) ;tm-menu

(tm-menu (focus-search-menu t)
 ("Search in database" (focus-open-search-tool t))
) ;tm-menu

(tm-menu (focus-tag-menu t)
  (with l
    (focus-variants-of t)
    (assuming (<= (length l) 1)
      (inert ((eval (focus-tag-name (tree-label t))) (noop) (noop)))
    ) ;assuming
    (assuming (> (length l) 1)
      (-> (eval (focus-tag-name (tree-label t))) (dynamic (focus-variant-menu t)))
    ) ;assuming
  ) ;with
  (dynamic (focus-toggle-menu t))
  (dynamic (focus-float-menu t))
  (dynamic (focus-animate-menu t))
  (dynamic (focus-misc-menu t))
  (assuming (focus-has-preferences? t)
    (-> "Preferences" (dynamic (focus-preferences-menu t)))
  ) ;assuming
  (assuming (focus-has-parameters? t)
    (-> "Rendering" (dynamic (focus-rendering-menu t)))
  ) ;assuming
  ("Describe" (focus-help))
  ("Delete" (remove-structure-upwards))
  (assuming (focus-has-search-menu? t)
    (-> "Search" (dynamic (focus-search-menu t)))
  ) ;assuming
  (assuming (focus-can-search? t)
   ("Search in database" (focus-open-search-tool t))
  ) ;assuming
) ;tm-menu

(tm-menu (focus-move-menu t)
 ("Previous similar" (traverse-previous))
 ("Next similar" (traverse-next))
 ("First similar" (traverse-first))
 ("Last similar" (traverse-last))
 (assuming (cursor-inside? t)
  ("Exit left" (structured-exit-left))
  ("Exit right" (structured-exit-right))
 ) ;assuming
) ;tm-menu

(tm-menu (focus-insert-menu t)
  (assuming (and (structured-horizontal? t) (not (structured-vertical? t)))
    (when (focus-can-insert? t)
      ("Insert argument before" (structured-insert-left))
      ("Insert argument after" (structured-insert-right))
    ) ;when
    (when (focus-can-remove? t)
      ("Remove argument before" (structured-remove-left))
      ("Remove argument after" (structured-remove-right))
    ) ;when
  ) ;assuming
  (assuming (structured-vertical? t)
   ("Insert above" (structured-insert-up))
   (assuming (not (list-structured-insert-context?))
    ("Insert left" (structured-insert-left))
    ("Insert right" (structured-insert-right))
   ) ;assuming
   ("Insert below" (structured-insert-down))
   ("Remove upwards" (structured-remove-up))
   (assuming (not (list-structured-insert-context?))
    ("Remove leftwards" (structured-remove-left))
    ("Remove rightwards" (structured-remove-right))
   ) ;assuming
   ("Remove downwards" (structured-remove-down))
  ) ;assuming
) ;tm-menu

(tm-menu (focus-extra-menu t))

(tm-define (hidden-inputter-children t)
  (append-map (lambda (c)
                (if (and-with i
                      (tree-index c)
                      (with type (tree-child-type t i) (inputter-active? c type))
                    ) ;and-with
                  (list c)
                  (list)
                ) ;if
              ) ;lambda
    (hidden-children t)
  ) ;append-map
) ;tm-define

(tm-menu (focus-hidden-menu t)
  (assuming (nnull? (hidden-inputter-children t))
    ---
    (for (i (.. 0 (tree-arity t)))
      (assuming (hidden-child? t i) (dynamic (string-input-menu t i)))
    ) ;for
  ) ;assuming
) ;tm-menu

(tm-menu (focus-hidden-menu t) (:require (pure-alternate-context? t)))

(tm-menu (focus-label-menu t)
  (assuming (focus-label t)
    ---
    (with s
      (focus-get-label t)
      ((eval (string-append "#" s))
       (interactive (lambda (l) (focus-set-label t l)) (list "Label" "string" s))
      ) ;
    ) ;with
  ) ;assuming
) ;tm-menu

(tm-menu (standard-focus-menu t)
  (dynamic (focus-ancestor-menu t))
  (dynamic (focus-tag-menu t))
  (assuming (focus-can-move? t) --- (dynamic (focus-move-menu t)))
  (assuming (focus-can-insert-remove? t) --- (dynamic (focus-insert-menu t)))
  (dynamic (focus-hidden-menu t))
  (dynamic (focus-extra-menu t))
  (dynamic (focus-label-menu t))
) ;tm-menu

(tm-menu (focus-menu) (dynamic (standard-focus-menu (focus-tree))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The main focus icons bar
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-ancestor-icons t))

(tm-menu (focus-toggle-icons t)
  (push-focus t
    (assuming (numbered-context? t)
     ((check (balloon (icon "tm_numbered.xpm") "Toggle numbering")
        "v"
        (pull-focus t (numbered-numbered? t))
      ) ;check
      (pull-focus t (numbered-toggle t))
     ) ;
    ) ;assuming
    (assuming (alternate-first? t)
     ((check (balloon (icon (eval (alternate-first-icon t)))
               (eval (pull-focus t (alternate-second-name t)))
             ) ;balloon
        "v"
        #f
      ) ;check
      (pull-focus t (alternate-toggle t))
     ) ;
    ) ;assuming
    (assuming (alternate-second? t)
     ((check (balloon (icon (eval (alternate-second-icon t)))
               (eval (pull-focus t (alternate-second-name t)))
             ) ;balloon
        "v"
        #t
      ) ;check
      (pull-focus t (alternate-toggle t))
     ) ;
    ) ;assuming
    (assuming (!= (tree-children t) (tree-accessible-children t))
     ((check (balloon (icon "tm_show_hidden.xpm") "Show hidden")
        "v"
        (pull-focus t (tree-is? t :up 'inactive))
      ) ;check
      (pull-focus t (inactive-toggle t))
     ) ;
    ) ;assuming
  ) ;push-focus
) ;tm-menu

(tm-menu (focus-float-icons t))
(tm-menu (focus-animate-icons t))
(tm-menu (focus-misc-icons t))
(tm-menu (focus-tag-extra-icons t))

(tm-menu (focus-tag-icons t)
  (dynamic (focus-toggle-icons t))
  (dynamic (focus-float-icons t))
  (dynamic (focus-animate-icons t))
  (dynamic (focus-misc-icons t))
  (mini #t
    (with l
      (focus-variants-of t)
      (assuming (<= (length l) 1)
        (inert ((eval `(verbatim ,(focus-tag-name (tree-label t)))) (noop)))
      ) ;assuming
      (assuming (> (length l) 1)
        (=> (balloon (eval `(verbatim ,(focus-tag-name (tree-label t))))
              (eval (string-append "Structured variant ("
                      (string-append (translate (kbd-system-rewrite "A-S-up"))
                        (string-append "/"
                          (string-append (translate (kbd-system-rewrite "A-S-down")) (string-append ")"))
                        ) ;string-append
                      ) ;string-append
                    ) ;string-append
              ) ;eval
            ) ;balloon
          (dynamic (focus-variant-menu t))
        ) ;=>
      ) ;assuming
    ) ;with
  ) ;mini
  (dynamic (focus-tag-extra-icons t))
  (assuming (cursor-inside? t)
   ((balloon (icon "tm_exit_left.xpm") "Exit tag on the left")
    (structured-exit-left)
   ) ;
   ((balloon (icon "tm_exit_right.xpm") "Exit tag on the right")
    (structured-exit-right)
   ) ;
   ((balloon (icon "tm_focus_delete.xpm") "Remove tag") (remove-structure-upwards))
  ) ;assuming
  (assuming (focus-has-preferences? t)
    (=> (balloon (icon "tm_focus_prefs.xpm") "Preferences for tag")
      (dynamic (focus-preferences-menu t))
    ) ;=>
  ) ;assuming
  (assuming (focus-has-parameters? t)
    (=> (balloon (icon "tm_theme.xpm") "Rendering options for tag")
      (dynamic (focus-rendering-menu t))
    ) ;=>
  ) ;assuming
  ((balloon (icon "tm_focus_help.xpm") "Describe tag") (focus-help))
  (assuming (focus-has-search-menu? t)
    (=> (balloon (icon "tm_focus_search.xpm") "Search")
      (dynamic (focus-search-menu t))
    ) ;=>
  ) ;assuming
  (assuming (focus-can-search? t)
   ((balloon (icon "tm_focus_search.xpm") "Search in database")
    (focus-open-search-tool t)
   ) ;
  ) ;assuming
) ;tm-menu

(tm-menu (focus-move-icons t)
 ((balloon (icon "tm_similar_first.xpm") "Go to first similar tag")
  (traverse-first)
 ) ;
 ((balloon (icon "tm_similar_previous.xpm") "Go to previous similar tag")
  (traverse-previous)
 ) ;
 ((balloon (icon "tm_similar_next.xpm") "Go to next similar tag")
  (traverse-next)
 ) ;
 ((balloon (icon "tm_similar_last.xpm") "Go to last similar tag")
  (traverse-last)
 ) ;
) ;tm-menu

(tm-menu (focus-insert-icons t)
  (assuming (and (structured-horizontal? t) (not (structured-vertical? t)))
    (when (focus-can-insert? t)
      ((balloon (icon "tm_insert_left.xpm") "Structured insert at the left")
       (structured-insert-left)
      ) ;
      ((balloon (icon "tm_insert_right.xpm") "Structured insert at the right")
       (structured-insert-right)
      ) ;
    ) ;when
    (when (focus-can-remove? t)
      ((balloon (icon "tm_delete_left.xpm") "Structured remove leftwards")
       (structured-remove-left)
      ) ;
      ((balloon (icon "tm_delete_right.xpm") "Structured remove rightwards")
       (structured-remove-right)
      ) ;
    ) ;when
  ) ;assuming
  (assuming (structured-vertical? t)
   ((balloon (icon "tm_insert_up.xpm") "Structured insert above")
    (structured-insert-up)
   ) ;
   (assuming (not (list-structured-insert-context?))
    ((balloon (icon "tm_insert_left.xpm") "Structured insert at the left")
     (structured-insert-left)
    ) ;
    ((balloon (icon "tm_insert_right.xpm") "Structured insert at the right")
     (structured-insert-right)
    ) ;
   ) ;assuming
   ((balloon (icon "tm_insert_down.xpm") "Structured insert below")
    (structured-insert-down)
   ) ;
   ((balloon (icon "tm_delete_up.xpm") "Structured remove upwards")
    (structured-remove-up)
   ) ;
   (assuming (not (list-structured-insert-context?))
    ((balloon (icon "tm_delete_left.xpm") "Structured remove leftwards")
     (structured-remove-left)
    ) ;
    ((balloon (icon "tm_delete_right.xpm") "Structured remove rightwards")
     (structured-remove-right)
    ) ;
   ) ;assuming
   ((balloon (icon "tm_delete_down.xpm") "Structured remove downwards")
    (structured-remove-down)
   ) ;
  ) ;assuming
) ;tm-menu

(tm-menu (focus-extra-icons t))

(tm-menu (focus-hidden-icons t)
  (for (i (.. 0 (tree-arity t)))
    (assuming (hidden-child? t i) (dynamic (string-input-icon t i)))
  ) ;for
) ;tm-menu

(tm-menu (focus-hidden-icons t) (:require (pure-alternate-context? t)))

(tm-menu (focus-label-icons t)
  (push-focus t
    (assuming (focus-label t)
      (with s
        (focus-get-label t)
        (glue #f #f 3 0)
        (mini #t (group "Label:"))
        (mini #t
          (input (pull-focus t (focus-set-label t answer)) "string" (list s) "12em")
        ) ;mini
      ) ;with
    ) ;assuming
  ) ;push-focus
) ;tm-menu

(tm-menu (standard-focus-icons t)
  (dynamic (focus-ancestor-icons t))
  (assuming (focus-can-move? t) (minibar (dynamic (focus-move-icons t))) //)
  (assuming (focus-can-insert-remove? t)
    (minibar (dynamic (focus-insert-icons t)))
    //
  ) ;assuming
  (minibar (dynamic (focus-tag-icons t)))
  (dynamic (focus-hidden-icons t))
  (dynamic (focus-extra-icons t))
  (dynamic (focus-label-icons t))
  //
) ;tm-menu

(tm-menu (texmacs-focus-icons)
  (assuming (in-graphics?) (dynamic (graphics-focus-icons)))
  (assuming (not (in-graphics?)) (dynamic (standard-focus-icons (focus-tree))))
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus for customizable environments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-customizable-menu-item var name mode)
  (with setter
    (lambda (val) (parameter-set var val mode))
    ((eval name) (interactive setter (list name "string" (parameter-get var mode))))
  ) ;with
) ;tm-menu

(tm-menu (focus-customizable-menu-item var name mode)
  (:require (parameter-choice-list var))
  (-> (eval name) (dynamic (parameter-submenu var mode)))
) ;tm-menu

(tm-menu (focus-customizable-menu-item var name mode)
  (:require (== (tree-label-type (string->symbol var)) "color"))
  (-> (eval name) (dynamic (parameter-submenu var mode)))
) ;tm-menu

(tm-menu (focus-extra-menu t)
  (:require (customizable-context? t))
  ---
  (for (p (customizable-parameters-memo t))
    (with (var name)
      p
      (with mode
        (list :local (tree-label t))
        (dynamic (focus-customizable-menu-item var name mode))
      ) ;with
    ) ;with
  ) ;for
) ;tm-menu

(tm-menu (focus-customizable-icons-item var name mode)
  (input (parameter-set var answer mode)
    "string"
    (list (parameter-get-string var mode))
    "5em"
  ) ;input
) ;tm-menu

(tm-menu (focus-customizable-icons-item var name mode)
  (:require (parameter-choice-list var))
  (mini #t
    (=> (eval (parameter-get-string var mode))
      (dynamic (parameter-submenu var mode))
    ) ;=>
  ) ;mini
) ;tm-menu

(tm-menu (focus-customizable-icons-item var name mode)
  (:require (== (tree-label-type (string->symbol var)) "color"))
  (=> (color (parameter-get var mode) #f #f 24 16)
    (dynamic (parameter-submenu var mode))
  ) ;=>
) ;tm-menu

(tm-menu (focus-extra-icons t)
  (:require (customizable-context? t))
  (for (p (customizable-parameters-memo t))
    (with (var name)
      p
      (with mode
        (list :local (tree-label t))
        (glue #f #f 3 0)
        (mini #t (group (eval (string-append name ":"))))
        (dynamic (focus-customizable-icons-item var name mode))
      ) ;with
    ) ;with
  ) ;for
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Hook for interactive commands
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define interactive-tool-table (make-ahash-table))

(tm-define (set-interactive-tool-arg win fun i val)
  (if val
    (ahash-set! interactive-tool-table (list win fun i) val)
    (tool-close :any 'interactive-tool #f win)
  ) ;if
) ;tm-define

(tm-define (get-interactive-tool-arg win fun i)
  (ahash-ref interactive-tool-table (list win fun i))
) ;tm-define

(tm-define (interactive-retype type i)
  (string-append "interactive-" (number->string i))
) ;tm-define

(tm-define (interactive-tool-arg win fun args i w)
  (with (var type . vals)
    (list-ref args i)
    `(item (text ,var)
       (input (if answer
                (begin
                  (set-interactive-tool-arg (quote ,win) (quote ,fun) ,i answer)
                  (if (< (+ ,i ,1) ,(length args))
                    (keyboard-focus-on (string-append ,"interactive-"
                                         (number->string (+ ,i ,1))))
                    (interactive-ok (quote ,win) (quote ,fun) (quote ,args))))
                (tool-close ,:any 'interactive-tool ,#f (quote ,win)))
         ,(interactive-retype type i)
         (quote ,(rcons vals ""))
         ,w))
  ) ;with
) ;tm-define

(tm-define (interactive-ok win fun args)
  (let* ((get (cut get-interactive-tool-arg win fun <>))
         (inds (.. 0 (length args)))
         (vals (map get inds))
        ) ;
    (apply fun vals)
    (with learn
      (map cons (map number->string inds) vals)
      (learn-interactive fun learn)
    ) ;with
    (tool-close :any 'interactive-tool #f win)
  ) ;let*
) ;tm-define

(tm-tool (interactive-tool win fun args)
  (:name (interactive-title fun))
  (dynamic (eval (let* ((side? (tool-side? `(interactive-tool ,fun ,args) win))
                        (width (if side? "12em" "24em"))
                       ) ;
                   (for (i (.. 0 (length args)))
                     (with (var type . vals)
                       (list-ref args i)
                       (with default
                         (if (null? vals) "" (car vals))
                         (set-interactive-tool-arg win fun i default)
                       ) ;with
                     ) ;with
                   ) ;for
                   (cond ((list-1? args)
                          (with (var type . vals)
                            (list-ref args 0)
                            `(menu-dynamic (hlist (text ,var)
                                             //
                                             //
                                             (input (begin
                                                      (set-interactive-tool-arg (quote
                                                                                  ,win)
                                                        (quote ,fun)
                                                        ,0
                                                        answer)
                                                      (when answer
                                                        (interactive-ok (quote
                                                                          ,win)
                                                          (quote ,fun)
                                                          (quote ,args))))
                                               ,(interactive-retype type 0)
                                               (quote ,(rcons vals ""))
                                               ,width)
                                             >>>))
                          ) ;with
                         ) ;
                         (side? `(menu-dynamic ===
                                   (aligned ,@(map (cut interactive-tool-arg
                                                     win
                                                     fun
                                                     args
                                                     <>
                                                     width)
                                                (.. 0 (length args))))
                                   ===
                                   (hlist >>>
                                     (division ,"plain"
                                       (,"Ok"
                                        (interactive-ok (quote ,win)
                                          (quote ,fun)
                                          (quote ,args))))))
                         ) ;side?
                         (else `(menu-dynamic (hlist (vlist (aligned ,@(map (cut interactive-tool-arg
                                                                              win
                                                                              fun
                                                                              args
                                                                              <>
                                                                              width)
                                                                         (.. 0
                                                                           (length args)))))
                                                //
                                                //
                                                //
                                                (vlist (glue #f #t 0 0)
                                                  (division ,"plain"
                                                    (hlist (,"Ok"
                                                            (interactive-ok (quote
                                                                              ,win)
                                                              (quote ,fun)
                                                              (quote ,args)))
                                                      >>>)))))
                         ) ;else
                   ) ;cond
                 ) ;let*
           ) ;eval
  ) ;dynamic
) ;tm-tool

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Immediately load document-menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic document-menu))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Rendering options for tag helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (rendering-parameters-merge t base mode)
  (if (func? mode :local)
    (let ((global (focus-parameters-list-memo t :global)))
      (if (and (in? "item-nr" global) (not (in? "item-nr" base)))
        (append base (list "item-nr"))
        base
      ) ;if
    ) ;let
    base
  ) ;if
) ;tm-define

(tm-define (parameter-item-nr-set l val mode)
  (with focus
    (focus-tree)
    (let* ((same? (tree-is? focus (cadr mode)))
           (item-node (tree-search-upwards focus '(item item*)))
           (list-node (and item-node
                        (or (tree-search-upwards item-node (enumerate-tag-list))
                          (tree-search-upwards item-node (list-tag-list))
                        ) ;or
                      ) ;and
           ) ;list-node
           (target (cond (list-node list-node)
                         (same? focus)
                         (item-node item-node)
                         (else focus)
                   ) ;cond
           ) ;target
           (done? #f)
          ) ;
      (if (not (or same? item-node))
        #f
        (begin
          (when (and item-node list-node)
            (with wrapper
              (tree-up item-node)
              (when (and wrapper
                      (tree-is? wrapper 'with)
                      (let loop
                        ((i 0))
                        (cond ((>= i (- (tree-arity wrapper) 1)) #f)
                              ((tm-equal? (tree-ref wrapper i) l) #t)
                              (else (loop (+ i 2)))
                        ) ;cond
                      ) ;let
                    ) ;and
                (tree-with-reset wrapper l)
                (set! done? #t)
              ) ;when
            ) ;with
          ) ;when
          (when target
            (tree-with-set target l val)
            (set! done? #t)
          ) ;when
          done?
        ) ;begin
      ) ;if
    ) ;let*
  ) ;with
) ;tm-define

(tm-define (parameter-local-special-set l val mode)
  ;; Extend this dispatch when adding more rendering options handled via locals.
  (case (string->symbol l)
        ((item-nr) (parameter-item-nr-set l val mode))
        (else #f)
  ) ;case
) ;tm-define
