
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
    (generic paragraph-format-widgets)
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

(tm-define (open-paragraph-format-window)
  (:interactive #t)
  ;; 走 QML 对话框（非阻塞模态）：register-specs 存 specs 拿 int 句柄并快照打开时
  ;; 段落参数，cpp-paragraph-format-dialog 开 QML 对话框，paraBridge 调
  ;; paragraph-format-* facade 透传交互。每次 setPara 经 make-multi-line-with
  ;; live 写回；Cancel 经快照写回撤销，重置经快照写回（回到打开时），OK 落定。
  ;; 返回 tree 仅测试用。specs=(scope getter setter)，'paragraph 走段落 with 通路。
  (with specs
    (list 'paragraph get-env make-multi-line-with)
    (cpp-paragraph-format-dialog (paragraph-format-register-specs specs))
  ) ;with
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
