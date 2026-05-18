
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : macro-widgets.scm
;; DESCRIPTION : widgets for editing macros
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (source macro-widgets)
  (:use (source macro-edit)
    (version version-edit)
    ;; FIXME: for selection-trees
    (generic format-edit)
    (generic document-part)
    (kernel gui menu-widget)
    (utils library cursor)
  ) ;:use
) ;texmacs-module

(import (only (liii hash-table) hash-table-keys))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Major operation mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (set-macro-window-state opened?)
  (set-auxiliary-widget-state opened? 'macro-editor)
) ;tm-define

(tm-define macro-major-mode :global)
(tm-define macro-major-focus #f)
(tm-define macro-major-history (list))

(define (initialize-macro-editor l mode)
  (terminate-macro-editor)
  (set! macro-major-mode mode)
  (set! macro-major-history (list))
  (when (func? mode :local)
    (set! macro-major-focus (tree->tree-pointer (focus-tree)))
  ) ;when
) ;define

(define (terminate-macro-editor . args)
  (when macro-major-focus
    (tree-pointer-detach macro-major-focus)
    (set! macro-major-focus #f)
  ) ;when

  ;; 如果传入了参数，则取出第一个参数作为 b 并转移焦点过去
  (when (pair? args)
    (let ((b (car args)))
      (buffer-focus b #t)
    ) ;let
  ) ;when
) ;define

(define (macro-editor-get l)
  (cond ((== macro-major-mode :global) (get-definition l))
        ((func? macro-major-mode :local)
         (and-with t
           (tree-pointer->tree macro-major-focus)
           (with val
             (tree-with-get t l)
             (if val (tm->tree `(assign ,l ,val)) (get-definition l))
           ) ;with
         ) ;and-with
        ) ;
        (else #f)
  ) ;cond
) ;define

(define (macro-editor-set l val u)
  (when (symbol? l)
    (set! l (symbol->string l))
  ) ;when
  (cond ((== macro-major-mode :global) (macro-set-value l val u))
        ((func? macro-major-mode :local)
         (and-with t (tree-pointer->tree macro-major-focus) (tree-with-set t l val))
        ) ;
  ) ;cond
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Global variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define macro-current-macro "")
(tm-define macro-current-filter "")
(tm-define macro-current-mode "Text")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines for macro editing widgets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (macro-retrieve* u)
  (and-with t
    (buffer-get-body u)
    (if (tm-is? t 'document) (set! t (tm-ref t :last)))
    (and (tm-in? t '(edit-macro edit-tag)) t)
  ) ;and-with
) ;define

(define (macro-retrieve u)
  (and-with t
    (macro-retrieve* u)
    (cond ((tm-is? (tree-ref t :last) 'inactive*)
           `(,(tm-label t) ,@(cDr (tm-children t)) ,(tree-ref t :last 0))
          ) ;
          ((tm-is? (tree-ref t :last) 'edit-math)
           `(,(tm-label t) ,@(cDr (tm-children t)) ,(tree-ref t :last 0))
          ) ;
          (else t)
    ) ;cond
  ) ;and-with
) ;define

(define (macro-retrieve-name u)
  (and-with t (macro-retrieve u) (tree->string (tm-ref t 0)))
) ;define

(define (get-macro-mode)
  macro-current-mode
) ;define

(define (set-macro-mode u mode)
  (set! macro-current-mode mode)
  (and-with t
    (macro-retrieve u)
    (with t*
      (macro-retrieve* u)
      (cond ((== mode "Source") (tree-set t* :last `(inactive* ,(cAr (tm-children t)))))
            ((== mode "Mathematics")
             (tree-set t* :last `(edit-math ,(cAr (tm-children t))))
            ) ;
            (else (tree-set t* :last (cAr (tm-children t))))
      ) ;cond
      (refresh-now "macro-editor-mode")
    ) ;with
  ) ;and-with
) ;define

(tm-define (toggle-source-mode)
  (:require (has-style-package? "macro-editor"))
  (with mode
    (if (!= (get-macro-mode) "Source") "Source" "Text")
    (set-macro-mode (current-buffer) mode)
  ) ;with
) ;tm-define

(define (preamble-insert pre ass)
  (with m
    (list-find (reverse (tree-children pre))
      (lambda (x) (and (tree-is? x 'assign) (tm-equal? (tm-ref x 0) (tm-ref ass 0))))
    ) ;list-find
    (if m (tree-set m ass) (tree-insert pre (tree-arity pre) (list ass)))
  ) ;with
) ;define

(define (macro-set-value l mac u)
  (let* ((b (buffer-get-master u))
         (m (buffer-get-master b))
         (buf (buffer-get-body b))
         (old (get-definition* l buf))
         (new `(assign ,l ,mac))
        ) ;
    (cond ((or (not (buffer-exists? u)) (not (buffer-exists? b))) #f)
          ((and old (tree->path old)) (tree-set old 1 mac))
          (else (when (not (document-has-preamble? buf))
                  (tree-insert! buf 0 '((hide-preamble (document ""))))
                ) ;when
            (when (document-has-preamble? buf)
              (with pre (tree-ref buf 0 0) (preamble-insert pre new))
            ) ;when
            (when (!= m b)
              (macro-set-value l mac b)
            ) ;when
          ) ;else
    ) ;cond
  ) ;let*
) ;define

(define (macro-value t)
  (if (tm-is? t 'edit-macro) `(macro ,@(cdr (tm-children t))) (tm-ref t 1))
) ;define

(define (macro-apply u)
  (and-with t
    (macro-retrieve u)
    (macro-editor-set (tree->string (tm-ref t 0)) (macro-value t) u)
    (invalidate-most-recent-view)
  ) ;and-with
) ;define

(define (build-macro-document* l def)
  (when (and (tm-func? def 'assign 2) (tm-equal? (tm-ref def 1) '(uninit)))
    (set! def `(assign ,(tm-ref def 0) (macro "")))
  ) ;when
  (when (and (tm-func? def 'assign 2)
          (tm-in? (tm-ref def 1) '(inactive* edit-math))
          (tm-func? (tm-ref def 1 0) 'macro)
        ) ;and
    (set! def `(assign ,(tm-ref def 0) ,(tm-ref def 1 0)))
  ) ;when
  (let* ((mac (if (tm-func? (tm-ref def 1) 'macro)
                `(edit-macro ,l ,@(tm-children (tm-ref def 1)))
                `(edit-tag ,l ,(tm-ref def 1))
              ) ;if
         ) ;mac
         (mac* (if (!= macro-current-mode "Source") mac `(,@(cDr mac)
                                                          (inactive* ,(cAr mac))))
         ) ;mac*
         (mac** (if (!= macro-current-mode "Mathematics")
                  mac*
                  `(,@(cDr mac*) (edit-math ,(cAr mac*)))
                ) ;if
         ) ;mac**
         (pre (document-get-preamble (buffer-tree)))
         (doc `(document (hide-preamble ,pre) ,mac**))
        ) ;
    doc
  ) ;let*
) ;define

(define (build-macro-document l)
  (cond ((and (== l "") (selection-active-any?))
         (build-macro-document* l `(assign ,l (macro ,(selection-tree))))
        ) ;
        (else (and-with def (macro-editor-get l) (build-macro-document* l def)))
  ) ;cond
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Editing a single macro
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((macro-editor u packs doc mode) quit)
  (padded ===
    (resize "400px" "300px" (texmacs-input doc `(style (tuple ,@packs)) u))
    ======
    (hlist (refreshable "macro-editor-mode"
             (enum (set-macro-mode u answer)
               '("Text" "Source" "Mathematics")
               (get-macro-mode)
               "12em"
             ) ;enum
           ) ;refreshable
      >>
      (explicit-buttons ("Shortcut"
                          (and-with t
                            (macro-retrieve u)
                            (let* ((s (tree->string (tm-ref t 0)))
                                   (sh (string-append "(make '" s ")"))
                                   (sh* (if (== s "") "" sh))
                                  ) ;
                              (open-shortcuts-editor "" sh*)
                            ) ;let*
                          ) ;and-with
                        ) ;
        //
        //
        ("Apply" (macro-apply u))
        //
        //
        ("Ok" (macro-apply u) (quit))
      ) ;explicit-buttons
    ) ;hlist
    ===
  ) ;padded
) ;tm-widget

(tm-tool* (macro-tool win u packs doc mode)
  (:name "Edit macro")
  (:quit (terminate-macro-editor))
  ===
  (horizontal //
    (vertical (resize "400px" "200px" (texmacs-input doc `(style (tuple ,@packs)) u))
      ======
      (division "plain"
        (hlist (refreshable "macro-editor-mode"
                 (enum (set-macro-mode u answer)
                   '("Text" "Source" "Mathematics")
                   (get-macro-mode)
                   "8em"
                 ) ;enum
               ) ;refreshable
          >>
          ("Apply" (macro-apply u))
        ) ;hlist
      ) ;division
    ) ;vertical
    //
  ) ;horizontal
) ;tm-tool*

(tm-define (editable-macro? l)
  (if (symbol? l) (set! l (symbol->string l)))
  (and (tree-label-extension? (string->symbol l))
    (nin? l (list "edit-macro" "edit-tag"))
    (get-definition l)
  ) ;and
) ;tm-define

(tm-define (open-macro-editor l mode)
  (:interactive #t)
  (change-auxiliary-widget-focus)
  (if (symbol? l) (set! l (symbol->string l)))
  (initialize-macro-editor l mode)
  (let* ((b (current-buffer-url))
         (u (string->url (string-append "tmfs://aux/edit-"
                           l
                           "-"
                           (url->string (url-tail (current-window)))
                         ) ;string-append
            ) ;string->url
         ) ;u
         (styps (embedded-style-list "macro-editor"))
         (macro-mode (if (in-math?) "Mathematics" "Text"))
         (doc (build-macro-document l))
         (tool (list 'macro-tool u styps doc mode))
        ) ;
    (set! macro-current-mode macro-mode)
    (when doc
      (buffer-set-master u b)
      (if (side-tools?)
        (tool-focus :right tool u)
        (auxiliary-widget (macro-editor u styps doc macro-mode)
          (lambda x (terminate-macro-editor b))
          (translate "Macro editor")
          u
        ) ;auxiliary-widget
      ) ;if
      (set-macro-window-state #t)
      (buffer-focus u #t)
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (edit-focus-macro)
  (:interactive #t)
  (with l
    (symbol->string (macro-label (focus-tree)))
    (when (editable-macro? l)
      (if (has-style-package? "macro-editor")
        (with old
          (macro-retrieve-name (current-buffer))
          (macros-editor-select (current-buffer) l "")
          (set! macro-major-history (cons old macro-major-history))
        ) ;with
        (open-macro-editor l :global)
      ) ;if
    ) ;when
  ) ;with
) ;tm-define

(tm-define (edit-previous-macro)
  (when (and (nnull? macro-major-history) (has-style-package? "macro-editor"))
    (macros-editor-select (current-buffer) (car macro-major-history) "")
    (set! macro-major-history (cdr macro-major-history))
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Contextual with-like macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (unary-tree? t)
  (and (== (tree-arity t) 1)
    (== (tree-minimal-arity t) 1)
    (== (tree-maximal-arity t) 1)
  ) ;and
) ;define

(define (add-context t body)
  (with p
    (tree-up t)
    (cond ((tree-is-buffer? t) body)
          ((and (tree-is? t 'document) (tree-is-buffer? p)) body)
          ((tree-is? t 'document) (add-context p `(document ,body)))
          ((tree-is? t 'with) (add-context p `(with ,@(cDr (tm-children t))
                                                ,body)))
          ((or (with-like? t) (unary-tree? t) (tree-in? t '(tformat ornament
                                                             ornamented)))
           (add-context p `(,(tm-label t) ,@(cDr (tm-children t)) ,body))
          ) ;
          ((tree-in? t '(table row cell)) (add-context p `(,(tm-label t) ,body)))
          (else (add-context (tree-up t) body))
    ) ;cond
  ) ;with
) ;define

(tm-define (can-create-context-macro?)
  (and (not (selection-active-any?))
    (with a '(arg "body") (!= (add-context (tree-up (cursor-tree)) a) a))
  ) ;and
) ;tm-define

(tm-define (create-context-macro l mode)
  (:interactive #t)
  (when (can-create-context-macro?)
    (if (symbol? l) (set! l (symbol->string l)))
    (set! macro-current-mode "Source")
    (let* ((b (current-buffer-url))
           (u (string->url (string-append "tmfs://aux/edit-"
                             l
                             "-"
                             (url->string (url-tail (current-window)))
                           ) ;string-append
              ) ;string->url
           ) ;u
           (styps (embedded-style-list "macro-editor"))
           (body (add-context (tree-up (cursor-tree)) '(arg "body")))
           (def `(assign ,l (inactive* (macro ,"body" ,body))))
           (doc (build-macro-document* l def))
           (tool (list 'macro-tool u styps doc "Source"))
          ) ;
      (when doc
        (buffer-set-master u b)
        (if (side-tools?)
          (tool-focus :right tool u)
          (dialogue-window (macro-editor u styps doc "Source")
            (lambda x (noop))
            "Macro editor"
          ) ;dialogue-window
        ) ;if
      ) ;when
    ) ;let*
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Table macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (contains-table? t)
  (or (and (tm-func? t 'table) t)
    (and (tm-compound? t) (list-or (map contains-table? (tm-children t))))
  ) ;or
) ;define

(tm-define (can-create-table-macro?)
  (or (inside? 'table)
    (and (selection-active-any?) (list-or (map contains-table? (selection-trees))))
  ) ;or
) ;tm-define

(define (position-inside-table)
  (or (inside? 'table)
    (and-with t
      (can-create-table-macro?)
      (while (tm-in? t '(tformat table row cell)) (set! t (tm-ref t 0)))
      (tree-go-to t :start)
    ) ;and-with
  ) ;or
) ;define

(define (tformat-subst-selection t tf)
  (cond ((tm-atomic? t) t)
        ((tm-func? t 'tformat)
         (with r
           (tformat-subst-selection (cAr (tm-children t)) tf)
           (if (tm-func? r 'tformat) r (append (cDr (tm-children t)) (list r)))
         ) ;with
        ) ;
        ((tm-in? t '(table tabular
                      tabular*
                      wide-tabular
                      block
                      block*
                      wide-block)) tf)
        (else (cons (tm-label t) (map (cut tformat-subst-selection <> tf) (tm-children t)))
        ) ;else
  ) ;cond
) ;define

(tm-define (create-table-macro l mode)
  (:interactive #t)
  (when (can-create-table-macro?)
    (position-inside-table)
    (if (symbol? l) (set! l (symbol->string l)))
    (set! macro-current-mode "Source")
    (let* ((b (current-buffer-url))
           (u (string->url (string-append "tmfs://aux/edit-"
                             l
                             "-"
                             (url->string (url-tail (current-window)))
                           ) ;string-append
              ) ;string->url
           ) ;u
           (styps (embedded-style-list "macro-editor"))
           (fm (table-get-format-all))
           (tf `(tformat ,@(tree-children fm) (arg "body")))
           (body (if (selection-active-any?)
                   (with sel (tm->stree (selection-tree)) (tformat-subst-selection sel tf))
                   tf
                 ) ;if
           ) ;body
           (def `(assign ,l (inactive* (macro ,"body" ,body))))
           (doc (build-macro-document* l def))
           (tool (list 'macro-tool u styps doc "Source"))
          ) ;
      (when doc
        (buffer-set-master u b)
        (if (side-tools?)
          (tool-focus :right tool u)
          (dialogue-window (macro-editor u styps doc "Source")
            (lambda x (noop))
            "Macro editor"
          ) ;dialogue-window
        ) ;if
      ) ;when
    ) ;let*
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Editing a macro chosen from the list of all defined macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (macros-editor-select u macro filter)
  (set! macro-current-macro macro)
  (set! macro-current-filter filter)
  (tree-set (buffer-get-body u) (build-macro-document macro-current-macro))
  (refresh-now "macros-editor-documentation")
) ;tm-define

(tm-define (macros-editor-select* win u macro filter)
  (macros-editor-select u macro filter)
  (with-window win (update-menus))
) ;tm-define

(tm-define (macros-editor-has-help?)
  (tmdoc-search-tag (string->symbol macro-current-macro))
) ;tm-define

(tm-define (macros-editor-current-help)
  (with doc
    (and (!= macro-current-macro "")
      (tmdoc-search-tag (string->symbol macro-current-macro))
    ) ;and
    (if doc (tm->stree doc) '(document (em "No documentation available.")))
  ) ;with
) ;tm-define

(tm-widget ((macros-editor u packs l) quit)
  (padded (horizontal (vertical (bold (text "Macro name"))
                        ===
                        ===
                        (resize "250px"
                          "500px"
                          (filtered-choice (macros-editor-select u answer filter)
                            l
                            macro-current-macro
                            macro-current-filter
                          ) ;filtered-choice
                        ) ;resize
                      ) ;vertical
            ///
            (vertical (bold (text "Macro definition"))
              ===
              ===
              (resize "500px"
                "220px"
                (texmacs-input (build-macro-document macro-current-macro)
                  `(style (tuple ,@packs))
                  u
                ) ;texmacs-input
              ) ;resize
              ===
              (glue #f #t 0 10)
              ===
              (bold (text "Documentation"))
              ===
              ===
              (horizontal (glue #t #f 0 0)
                (refreshable "macros-editor-documentation"
                  (resize "500px"
                    "220px"
                    (texmacs-output `(document (mini-paragraph ,"476guipx"
                                                 ,(macros-editor-current-help)))
                      '(style "tmdoc")
                    ) ;texmacs-output
                  ) ;resize
                ) ;refreshable
                (glue #t #f 0 0)
              ) ;horizontal
            ) ;vertical
          ) ;horizontal
    ======
    (hlist (refreshable "macro-editor-mode"
             (enum (set-macro-mode u answer)
               '("Text" "Source" "Mathematics")
               (get-macro-mode)
               "12em"
             ) ;enum
           ) ;refreshable
      >>
      (explicit-buttons ("Shortcut"
                          (and-with t
                            (macro-retrieve u)
                            (let* ((s (tree->string (tm-ref t 0)))
                                   (sh (string-append "(make '" s ")"))
                                   (sh* (if (== s "") "" sh))
                                  ) ;
                              (open-shortcuts-editor "" sh*)
                            ) ;let*
                          ) ;and-with
                        ) ;
        //
        //
        ("Apply" (macro-apply u))
        //
        //
        ("Ok" (macro-apply u) (quit))
      ) ;explicit-buttons
    ) ;hlist
  ) ;padded
) ;tm-widget

(tm-tool* (macros-tool win u packs l)
  (:name "Macro selector")
  (:quit (terminate-macro-editor))
  (centered (resize "250px"
              "150px"
              (filtered-choice (macros-editor-select* win u answer filter)
                l
                macro-current-macro
                macro-current-filter
              ) ;filtered-choice
            ) ;resize
  ) ;centered
  ===
  ======
  (division "title" (text "Macro editor"))
  (centered (resize "400px"
              "200px"
              (texmacs-input (build-macro-document macro-current-macro)
                `(style (tuple ,@packs))
                u
              ) ;texmacs-input
            ) ;resize
    ======
    (division "plain"
      (hlist (refreshable "macro-editor-mode"
               (enum (set-macro-mode u answer)
                 '("Text" "Source" "Mathematics")
                 (get-macro-mode)
                 "8em"
               ) ;enum
             ) ;refreshable
        >>
        ("Apply" (macro-apply u))
      ) ;hlist
    ) ;division
  ) ;centered
  (refreshable "macros-editor-documentation"
    (assuming (macros-editor-has-help?)
      ======
      ======
      (division "title" (text "Documentation"))
      (centered (resize "400px"
                  "300px"
                  (texmacs-output `(document (mini-paragraph ,"376guipx"
                                               ,(macros-editor-current-help)))
                    '(style (tuple "tmdoc" "side-tools"))
                  ) ;texmacs-output
                ) ;resize
      ) ;centered
    ) ;assuming
  ) ;refreshable
) ;tm-tool*

(define (get-key key-val)
  (tree->string (tree-ref key-val 0))
) ;define

(tm-define (all-defined-macros)
  (with env
    (tm-children (get-full-env))
    (sort (list-difference (map get-key env)
            (list "atom-decorations"
              "line-decorations"
              "page-decorations"
              "xoff-decorations"
              "yoff-decorations"
              "cell-decoration"
              "cell-format"
              "wide-framed-colored"
              "wide-std-framed-colored"
            ) ;list
          ) ;list-difference
      string<=?
    ) ;sort
  ) ;with
) ;tm-define

(tm-define (all-defined-macros*)
  (with env
    (tm-children (get-full-env))
    (sort (list-remove-duplicates (append (list-difference (map get-key env)
                                            (list "atom-decorations"
                                              "line-decorations"
                                              "page-decorations"
                                              "xoff-decorations"
                                              "yoff-decorations"
                                              "cell-decoration"
                                              "cell-format"
                                              "wide-framed-colored"
                                              "wide-std-framed-colored"
                                            ) ;list
                                          ) ;list-difference
                                    (hash-table-keys kbd-command-table)
                                    (tree-primitives)
                                  ) ;append
          ) ;list-remove-duplicates
      string<=?
    ) ;sort
  ) ;with
) ;tm-define

(tm-define (open-macros-editor mode)
  (:interactive #t)
  (initialize-macro-editor :all mode)
  (let* ((b (current-buffer-url))
         (u (string->url "tmfs://aux/macro-editor"))
         (names (all-defined-macros))
         (styps (embedded-style-list "macro-editor"))
         (tool (list 'macros-tool u styps names))
        ) ;
    (set! macro-current-mode "Text")
    (buffer-set-master u b)
    (if (side-tools?)
      (tool-focus :right tool u)
      (dialogue-window (macros-editor u styps names)
        (lambda x (terminate-macro-editor))
        "Macros editor"
        u
      ) ;dialogue-window
    ) ;if
  ) ;let*
) ;tm-define

(register-auxiliary-widget-type 'macro-editor
  (list (lambda () (open-macro-editor "" :global)))
) ;register-auxiliary-widget-type
