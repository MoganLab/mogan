
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

(texmacs-module (generic format-widgets)
  (:use (generic format-menu)
    (generic document-edit)
    (kernel gui menu-widget)
    (utils library cursor)
  ) ;:use
) ;texmacs-module


(tm-define (set-page-format-window-state opened?)
  (set-auxiliary-widget-state opened? 'page-format)
) ;tm-define
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (get-env-table l)
  (with t (make-ahash-table) (for (x l) (ahash-set! t x (get-env x))) t)
) ;tm-define

(tm-define (get-init-table l)
  (with t (make-ahash-table) (for (x l) (ahash-set! t x (get-init x))) t)
) ;tm-define

(tm-define (ahash-table-changes old new)
  (with diff
    (make-ahash-table)
    (for (key (map car (ahash-table->list new)))
      (when (!= (ahash-ref new key) (ahash-ref old key))
        (ahash-set! diff key (ahash-ref new key))
      ) ;when
    ) ;for
    diff
  ) ;with
) ;tm-define

(define (differences-list old new)
  (with diff (ahash-table-changes old new) (assoc->list (ahash-table->list diff)))
) ;define

(tm-define (page-type-pretty t)
  (cond ((== t "a0") "A0")
        ((== t "a1") "A1")
        ((== t "a2") "A2")
        ((== t "a3") "A3")
        ((== t "a4") "A4")
        ((== t "a5") "A5")
        ((== t "a6") "A6")
        ((== t "a7") "A7")
        ((== t "a8") "A8")
        ((== t "a9") "A9")
        ((== t "a10") "A10")
        ((== t "b0") "B0")
        ((== t "b1") "B1")
        ((== t "b2") "B2")
        ((== t "b3") "B3")
        ((== t "b4") "B4")
        ((== t "b5") "B5")
        ((== t "b6") "B6")
        (else t)
  ) ;cond
) ;tm-define

(tm-define (page-type-raw t)
  (cond ((== t "A0") "a0")
        ((== t "A1") "a1")
        ((== t "A2") "a2")
        ((== t "A3") "a3")
        ((== t "A4") "a4")
        ((== t "A5") "a5")
        ((== t "A6") "a6")
        ((== t "A7") "a7")
        ((== t "A8") "a8")
        ((== t "A9") "a9")
        ((== t "A10") "a10")
        ((== t "B0") "b0")
        ((== t "B1") "b1")
        ((== t "B2") "b2")
        ((== t "B3") "b3")
        ((== t "B4") "b4")
        ((== t "B5") "b5")
        ((== t "B6") "b6")
        (else t)
  ) ;cond
) ;tm-define

(define (change var val old new fun u)
  (ahash-set! new var val)
  (when (== u (current-buffer))
    (fun (differences-list old new))
    (for (key (map car (ahash-table->list new)))
      (ahash-set! old key (ahash-ref new key))
    ) ;for
  ) ;when
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Paragraph properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define paragraph-parameters
  (list "par-mode"
    "par-flexibility"
    "par-hyphen"
    "par-spacing"
    "par-kerning-stretch"
    "par-kerning-reduce"
    "par-expansion"
    "par-contraction"
    "par-kerning-margin"
    "par-width"
    "par-left"
    "par-right"
    "par-first"
    "par-no-first"
    "par-sep"
    "par-hor-sep"
    "par-ver-sep"
    "par-line-sep"
    "par-par-sep"
    "par-fnote-sep"
    "par-columns"
    "par-columns-sep"
  ) ;list
) ;tm-define

(tm-widget (paragraph-formatter-basic old new fun u flag?)
  (aligned (item (text "Alignment:")
             (enum (change "par-mode" answer old new fun u)
               '("left" "center" "right" "justify")
               (ahash-ref new "par-mode")
               "10em"
             ) ;enum
           ) ;item
    (assuming (not flag?)
      (item ====== ======)
      (item (text "Left margin:")
        (enum (change "par-left" answer old new fun u)
          (cons-new (ahash-ref new "par-left") '("0tab" "1tab" "2tab" ""))
          (ahash-ref new "par-left")
          "10em"
        ) ;enum
      ) ;item
      (item (text "Right margin:")
        (enum (change "par-right" answer old new fun u)
          (cons-new (ahash-ref new "par-right") '("0tab" "1tab" "2tab" ""))
          (ahash-ref new "par-right")
          "10em"
        ) ;enum
      ) ;item
    ) ;assuming
    (item (text "First indentation:")
      (enum (change "par-first" answer old new fun u)
        (cons-new (ahash-ref new "par-first") '("0fn" "2fn" ""))
        (ahash-ref new "par-first")
        "10em"
      ) ;enum
    ) ;item
    (item ====== ======)
    (item (text "Interline space:")
      (enum (change "par-sep" answer old new fun u)
        (cons-new (ahash-ref new "par-sep") '("0fn" "0.2fn" "0.5fn" "1fn" ""))
        (ahash-ref new "par-sep")
        "10em"
      ) ;enum
    ) ;item
    (item (hlist // (text "Line spacing presets:"))
      (hlist ("1.5x"
               (begin
                 (change "par-sep" "0.5fn" old new fun u)
                 (refresh-now "paragraph-formatter")
               ) ;begin
             ) ;
        //
        //
        ("2.0x"
          (begin
            (change "par-sep" "1.0fn" old new fun u)
            (refresh-now "paragraph-formatter")
          ) ;begin
        ) ;
      ) ;hlist
    ) ;item
    (item (text "Interparagraph space:")
      (enum (change "par-par-sep" answer old new fun u)
        (cons-new (ahash-ref new "par-par-sep")
          '("0fn" "0.3333fn" "0.5fn" "0.6666fn" "1fn" "0.5fns" "")
        ) ;cons-new
        (ahash-ref new "par-par-sep")
        "10em"
      ) ;enum
    ) ;item
    (item ====== ======)
    (item (text "Number of columns:")
      (enum (begin
              (change "par-columns" answer old new fun u)
              (refresh-now "paragraph-formatter-columns-sep")
            ) ;begin
        '("1" "2" "3" "4" "5" "6")
        (ahash-ref new "par-columns")
        "10em"
      ) ;enum
    ) ;item
    (item (when (!= (ahash-ref new "par-columns") "1")
            (text "Column separation:")
          ) ;when
      (refreshable "paragraph-formatter-columns-sep"
        (when (!= (ahash-ref new "par-columns") "1")
          (enum (change "par-columns-sep" answer old new fun u)
            (cons-new (ahash-ref new "par-columns-sep") '("1fn" "2fn" "3fn" ""))
            (ahash-ref new "par-columns-sep")
            "10em"
          ) ;enum
        ) ;when
      ) ;refreshable
    ) ;item
  ) ;aligned
) ;tm-widget

(tm-widget (paragraph-formatter-advanced old new fun u)
  (aligned (item (text "Line breaking:")
             (enum (change "par-hyphen" answer old new fun u)
               '("normal" "professional")
               (ahash-ref new "par-hyphen")
               "10em"
             ) ;enum
           ) ;item
    (item ====== ======)
    (item (text "Extra interline space:")
      (enum (change "par-line-sep" answer old new fun u)
        (cons-new (ahash-ref new "par-line-sep")
          '("0fn" "0.025fns" "0.05fns" "0.1fns" "0.2fns" "0.5fns" "1fns" "")
        ) ;cons-new
        (ahash-ref new "par-line-sep")
        "10em"
      ) ;enum
    ) ;item
    (item (text "Minimal line separation:")
      (enum (change "par-ver-sep" answer old new fun u)
        (cons-new (ahash-ref new "par-ver-sep")
          '("0fn" "0.1fn" "0.2fn" "0.5fn" "1fn" "")
        ) ;cons-new
        (ahash-ref new "par-ver-sep")
        "10em"
      ) ;enum
    ) ;item
    (item (text "Horizontal collapse distance:")
      (enum (change "par-hor-sep" answer old new fun u)
        (cons-new (ahash-ref new "par-hor-sep")
          '("0.1fn" "0.2fn" "0.5fn" "1fn" "2fn" "5fn" "10fn" "100fn" "")
        ) ;cons-new
        (ahash-ref new "par-hor-sep")
        "10em"
      ) ;enum
    ) ;item
    (item ====== ======)
    (item (text "Space stretchability:")
      (enum (change "par-flexibility" answer old new fun u)
        (cons-new (ahash-ref new "par-flexibility") '("1" "2" "4" "1000" ""))
        (ahash-ref new "par-flexibility")
        "10em"
      ) ;enum
    ) ;item
    (item (text "CJK spacing:")
      (enum (change "par-spacing" answer old new fun u)
        '("plain" "quanjiao" "banjiao" "hangmobanjiao" "kaiming")
        (ahash-ref new "par-spacing")
        "10em"
      ) ;enum
    ) ;item
    (item ====== ======)
    (item (text "Intercharacter stretching:")
      (enum (change "par-kerning-stretch" answer old new fun u)
        (cons-new (ahash-ref new "par-kerning-stretch")
          '("auto" "tolerant" "0" "0.02" "0.05" "0.1" "0.2" "0.5" "1" "")
        ) ;cons-new
        (ahash-ref new "par-kerning-stretch")
        "10em"
      ) ;enum
    ) ;item
    (item (text "Intercharacter compression:")
      (enum (change "par-kerning-reduce" answer old new fun u)
        (cons-new (ahash-ref new "par-kerning-reduce")
          '("auto" "0" "0.01" "0.02" "0.03" "0.05" "0.1" "0.2" "")
        ) ;cons-new
        (ahash-ref new "par-kerning-reduce")
        "10em"
      ) ;enum
    ) ;item
    (item (text "Character expansion:")
      (enum (change "par-expansion" answer old new fun u)
        (cons-new (ahash-ref new "par-expansion")
          '("auto" "tolerant" "0" "0.01" "0.02" "0.05" "0.1" "0.2" "")
        ) ;cons-new
        (ahash-ref new "par-expansion")
        "10em"
      ) ;enum
    ) ;item
    (item (text "Character contraction:")
      (enum (change "par-contraction" answer old new fun u)
        (cons-new (ahash-ref new "par-contraction")
          '("auto" "tolerant" "0" "0.01" "0.02" "0.05" "0.1" "0.2" "")
        ) ;cons-new
        (ahash-ref new "par-contraction")
        "10em"
      ) ;enum
    ) ;item
  ) ;aligned
  ======
  (centered (aligned (meti (hlist // (text "Use margin kerning (protrusion)"))
                       (toggle (change "par-kerning-margin" (if answer "true" "false") old new fun u)
                         (== (ahash-ref new "par-kerning-margin") "true")
                       ) ;toggle
                     ) ;meti
            ) ;aligned
  ) ;centered
) ;tm-widget

(tm-widget ((paragraph-formatter old new fun u flag?) quit)
  (padded (refreshable "paragraph-formatter"
            (tabs (tab (text "Basic")
                    (padded (dynamic (paragraph-formatter-basic old new fun u flag?)))
                  ) ;tab
              (tab (text "Advanced")
                (padded (dynamic (paragraph-formatter-advanced old new fun u)))
              ) ;tab
            ) ;tabs
          ) ;refreshable
    (if flag?
      ===
      ===
      (explicit-buttons (hlist >>>
                          (if flag?
                           ("Reset"
                             (apply init-default paragraph-parameters)
                             (with t
                               (get-init-table paragraph-parameters)
                               (for (key (map car (ahash-table->list t)))
                                 (ahash-set! old key (ahash-ref t key))
                                 (ahash-set! new key (ahash-ref t key))
                               ) ;for
                             ) ;with
                             (refresh-now "paragraph-formatter")
                           ) ;
                          ) ;if
                        ) ;hlist
      ) ;explicit-buttons
    ) ;if
  ) ;padded
) ;tm-widget

(tm-define (open-paragraph-format-window)
  (:interactive #t)
  (let* ((old (get-env-table paragraph-parameters))
         (new (get-env-table paragraph-parameters))
         (u (current-buffer))
        ) ;
    (dialogue-window (paragraph-formatter old new make-multi-line-with u #f)
      noop
      "Paragraph format"
    ) ;dialogue-window
  ) ;let*
) ;tm-define

(tm-define (open-paragraph-format)
  (:interactive #t)
  (if (side-tools?)
    (tool-select :right 'format-paragraph-tool)
    (open-paragraph-format-window)
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Page properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (page-the-page-val body)
  (when (tm-func? body 'macro 1)
    (set! body (tm-ref body 0))
  ) ;when
  (cond ((tm-equal? body '(value "page-nr")) "normal")
        ((tm-equal? body '(number (value "page-nr") "roman")) "roman")
        ((tm-equal? body '(number (value "page-nr") "Roman")) "Roman")
        (else "unchanged")
  ) ;cond
) ;define

(define (page-this-bg-color-val col)
  (cond ((tm-atomic? col) "color")
        ((and (tm-func? col 'pattern 3)
           (tm-equal? (tm-ref col 1) "100%")
           (tm-equal? (tm-ref col 2) "100%")
         ) ;and
         "picture"
        ) ;
        ((tm-is? col 'pattern) "pattern")
        (else "unchanged")
  ) ;cond
) ;define

(define (collect-settings t settings)
  (cond ((tree-atomic? t) (noop))
        ((tree-func? t 'assign 2)
         (with var
           (tree->stree (tree-ref t 0))
           (cond ((== var "page-the-page")
                  (ahash-set! settings var (page-the-page-val (tree-ref t 1)))
                 ) ;
                 ((== var "page-this-bg-color")
                  (ahash-set! settings var (page-this-bg-color-val (tree-ref t 1)))
                 ) ;
                 (else (ahash-set! settings var (tree->stree (tree-ref t 1))))
           ) ;cond
         ) ;with
        ) ;
        (else (for-each (cut collect-settings <> settings) (tree-children t)))
  ) ;cond
) ;define

(define (cut-all t var)
  (cond ((tree-atomic? t) (noop))
        ((and (tree-func? t 'assign 2) (tm-equal? (tree-ref t 0) var)) (tree-cut t))
        (else (for-each (cut cut-all <> var) (tree-children t)))
  ) ;cond
) ;define

(define (page-the-page-body val)
  (cond ((== val "normal") '(value "page-nr"))
        ((== val "roman") '(number (value "page-nr") "roman"))
        ((== val "Roman") '(number (value "page-nr") "Roman"))
        (else '(value "page-nr"))
  ) ;cond
) ;define

(define (change-setting var val settings u)
  (and-with doc
    (tree-innermost 'document)
    (and-with par
      (tree-down doc)
      (ahash-set! settings var val)
      (with-buffer u
        (cut-all par var)
        (when (!= val "unchanged")
          (if (!= var "page-the-page")
            (insert `(assign ,var ,val))
            (insert `(assign ,var (macro ,(page-the-page-body val))))
          ) ;if
        ) ;when
        (refresh-window)
      ) ;with-buffer
    ) ;and-with
  ) ;and-with
) ;define

(define (change-background settings what u)
  (let* ((var "page-this-bg-color")
         (old (ahash-ref settings var))
         (setter (lambda (c) (change-setting var c settings u)))
        ) ;
    (cond ((== what "unchanged") (setter "unchanged"))
          ((== what "color") (interactive-color setter (if old (list old) (list))))
          ((== what "pattern") (open-pattern-selector setter "1cm"))
          ((== what "picture") (open-background-picture-selector setter))
    ) ;cond
  ) ;let*
) ;define

(define (header-buffer)
  (string->url (string-append "tmfs://aux/this-page-header"
                 "/"
                 (url->string (url-tail (get-auxiliary-widget-parent-url)))
               ) ;string-append
  ) ;string->url
) ;define

(define (footer-buffer)
  (string->url (string-append "tmfs://aux/this-page-footer"
                 "/"
                 (url->string (url-tail (get-auxiliary-widget-parent-url)))
               ) ;string-append
  ) ;string->url
) ;define

(define (editing-headers?)
  (in? (current-buffer) (list (header-buffer) (footer-buffer)))
) ;define

(define (get-field-contents u)
  (and-with t
    (tm->stree (buffer-get-body u))
    (when (tm-func? t 'document 1)
      (set! t (tm-ref t 0))
    ) ;when
    (and (!= t '(unchanged)) t)
  ) ;and-with
) ;define

(define (cut-tag t var)
  (cond ((tree-atomic? t) (noop))
        ((tree-func? t var 1) (tree-cut t))
        (else (for-each (cut cut-tag <> var) (tree-children t)))
  ) ;cond
) ;define

(define (get-tag-arg** l var)
  (and (nnull? l) (or (get-tag-arg* (car l) var) (get-tag-arg** (cdr l) var)))
) ;define

(define (get-tag-arg* t var)
  (cond ((tree-atomic? t) #f)
        ((tree-func? t var 1) (tm->stree (tm-ref t 0)))
        (else (get-tag-arg** (reverse (tree-children t)) var))
  ) ;cond
) ;define

(define (get-tag-arg u var)
  (with-buffer u
    (and-with doc
      (tree-innermost 'document)
      (and-with par (tree-down doc) (or (get-tag-arg* par var) '(unchanged)))
    ) ;and-with
  ) ;with-buffer
) ;define

(define (apply-page-settings-one u settings var val)
  (with-buffer u
    (and-with doc
      (tree-innermost 'document)
      (and-with par
        (tree-down doc)
        (cut-tag par var)
        (insert `(,var ,val))
        (refresh-window)
      ) ;and-with
    ) ;and-with
  ) ;with-buffer
) ;define

(define (apply-page-settings u settings)
  (and-with val
    (get-field-contents (header-buffer))
    (apply-page-settings-one u settings 'set-this-page-header val)
  ) ;and-with
  (and-with val
    (get-field-contents (footer-buffer))
    (apply-page-settings-one u settings 'set-this-page-footer val)
  ) ;and-with
) ;define

(tm-widget ((page-formatter u style settings flag?) quit)
  (padded (centered (aligned (item (text "This page number:")
                               (enum (change-setting "page-nr" answer settings u)
                                 '("unchanged" "")
                                 (or (ahash-ref settings "page-nr") "unchanged")
                                 "10em"
                               ) ;enum
                             ) ;item
                      (item (text "Page number rendering:")
                        (enum (change-setting "page-the-page" answer settings u)
                          '("unchanged" "normal" "roman" "Roman")
                          (or (ahash-ref settings "page-the-page") "unchanged")
                          "10em"
                        ) ;enum
                      ) ;item
                      (item (text "Page background:")
                        (enum (change-background settings answer u)
                          '("unchanged" "color" "pattern" "picture")
                          (or (ahash-ref settings "page-this-bg-color") "unchanged")
                          "10em"
                        ) ;enum
                      ) ;item
                    ) ;aligned
          ) ;centered
    ======
    (bold (text "This page header"))
    ===
    (resize (if flag? "480px" "100px")
      "60px"
      (texmacs-input `(document ,(get-tag-arg u 'set-this-page-header))
        `(style (tuple ,@style ,"gui-base"))
        (header-buffer)
      ) ;texmacs-input
    ) ;resize
    ===
    ===
    (bold (text "This page footer"))
    ===
    (resize (if flag? "480px" "100px")
      "60px"
      (texmacs-input `(document ,(get-tag-arg u 'set-this-page-footer))
        `(style (tuple ,@style ,"gui-base"))
        (footer-buffer)
      ) ;texmacs-input
    ) ;resize
    ======
    (explicit-buttons (hlist (text "Insert:")
                        //
                        //
                        ("Tab" (when (editing-headers?) (make-htab "5mm")))
                        //
                        //
                        ("Page number" (when (editing-headers?) (insert '(page-number))))
                        >>>
                        ("Ok" (apply-page-settings u settings) (begin (quit) (buffer-focus u #t)))
                      ) ;hlist
    ) ;explicit-buttons
  ) ;padded
) ;tm-widget

(tm-define (open-page-format-window)
  (:interactive #t)
  (change-auxiliary-widget-focus)
  (let* ((u (current-buffer))
         (st (embedded-style-list "macro-editor"))
         (t (make-ahash-table))
        ) ;
    (and-with doc
      (tree-innermost 'document)
      (and-with par (tree-down doc) (collect-settings par t))
    ) ;and-with
    (auxiliary-widget (page-formatter u st t #t)
      noop
      "Page format"
      (header-buffer)
      (footer-buffer)
    ) ;auxiliary-widget
    (set-page-format-window-state #t)
    (buffer-focus (header-buffer) #t)
  ) ;let*
) ;tm-define

(tm-define (open-page-format)
  (:interactive #t)
  (if (side-tools?)
    (let* ((u (current-buffer))
           (st (embedded-style-list "macro-editor"))
           (t (make-ahash-table))
          ) ;
      (and-with doc
        (tree-innermost 'document)
        (and-with par (tree-down doc) (collect-settings par t))
      ) ;and-with
      (tool-select :right (list 'format-page-tool u st t))
    ) ;let*
    (open-page-format-window)
  ) ;if
) ;tm-define

(register-auxiliary-widget-type 'page-format (list open-page-format-window))
