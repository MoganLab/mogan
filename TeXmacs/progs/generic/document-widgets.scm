
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : document-widgets.scm
;; DESCRIPTION : widgets for setting global document properties
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic document-widgets)
  (:use (generic document-menu)
        (kernel gui menu-widget)
        (generic format-widgets)))


(tm-define (set-page-headers-footers-window-state opened?)
  (set-auxiliary-widget-state opened? 'page-headers-footers))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Style chooser widget (still to be implemented)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (select-style-among-widget l)
  (resize '("300px" "300px" "300px") '("200px" "300px" "1000px")
    (scrollable
      (choice (set-main-style answer) l "generic"))))

(tm-widget (select-common-style-widget)
  (dynamic (select-style-among-widget
            (list "article" "beamer" "book" "browser" "exam"
                  "generic" "letter" "manual" "seminar" "source"))))

(tm-widget (select-education-style-widget)
  (dynamic (select-style-among-widget
            (list "compact" "exam"))))

(tm-widget (select-article-style-widget)
  (dynamic (select-style-among-widget
            (list "article" "tmarticle"))))

(tm-widget (select-any-style-widget)
  (dynamic (select-style-among-widget
            (list "article" "tmarticle"))))

(tm-widget (select-style-widget)
  (tabs
    (tab (text "Common")
      (dynamic (select-common-style-widget)))
    (tab (text "Education")
      (dynamic (select-education-style-widget)))
    (tab (text "Article")
      (dynamic (select-article-style-widget)))
    (tab (text "Any")
      (dynamic (select-any-style-widget)))))

(tm-define (open-style-selector)
  (:interactive #t)
  (top-window select-style-widget "Select document style"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Source -> Preferences
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((source-tree-preferences-editor u) quit)
  (padded
    (refreshable "source-tree-preferences"
      (aligned
        (item (text "Main presentation style:")
          (enum (initial-set u "src-style" answer)
                '("angular" "scheme" "functional" "latex")
                (initial-get u "src-style") "10em"))
        (item (text "Tags with special rendering:")
          (enum (initial-set u "src-special" answer)
                '("raw" "format" "normal" "maximal")
                (initial-get u "src-special") "10em"))
        (item (text "Compactification:")
          (enum (initial-set u "src-compact" answer)
                '("none" "inline" "normal" "inline args" "all")
                (initial-get u "src-compact") "10em"))
        (item (text "Closing style:")
          (enum (initial-set u "src-close" answer)
                '("repeat" "long" "compact" "minimal")
                (initial-get u "src-close") "10em"))))
    ======
    (explicit-buttons
      (hlist
        >>>
        ("Reset"
         (initial-default u "src-style" "src-special"
                            "src-compact" "src-close")
         (refresh-now "source-tree-preferences"))
        // //
        ("Ok" (quit))))))

(tm-define (open-source-tree-preferences-window)
  (:interactive #t)
  (with u (current-buffer)
    (dialogue-window (source-tree-preferences-editor u) noop
                     "Document source tree preferences")))

(tm-define (open-source-tree-preferences)
  (:interactive #t)
  (if (side-tools?)
      (tool-select :right 'source-tree-preferences-tool)
      (open-source-tree-preferences-window)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Paragraph
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-document-paragraph-format-window)
  (:interactive #t)
  (let* ((old (get-init-table paragraph-parameters))
         (new (get-init-table paragraph-parameters))
         (u   (current-buffer)))
    (dialogue-window (paragraph-formatter old new init-multi u #t)
                     noop "Document paragraph format")))

(tm-define (open-document-paragraph-format)
  (:interactive #t)
  (if (side-tools?)
      (tool-select :right 'document-paragraph-tool)
      (open-document-paragraph-format-window)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Format
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (page-type-pretty t)
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
        (else t)))

(define (page-type-raw t)
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
        (else t)))

(define (page-size-list u)
  (if (initial-defined? u "beamer-style")
      (list "16:9" "8:5" "4:3" "5:4")
      (map page-type-pretty
           (list "a3" "a4" "a5" "b4" "b5" "letter" "legal" "executive"))))

(define (user-page-size? u)
  (== (initial-get u "page-type") "user"))

(define (encode-rendering s)
  (cond ((== s "Single Page") "paper")
        ((== s "Continuous Scroll") "papyrus")
        ((== s "Screen") "automatic")
        ((== s "Beamer") "beamer")
        ((== s "Two Page") "book")
        ((== s "Panorama") "panorama")
        (else s)))

(define (decode-rendering s)
  (cond ((== s "paper") "Single Page")
        ((== s "papyrus") "Continuous Scroll")
        ((== s "automatic") "Screen")
        ((== s "beamer") "Beamer")
        ((== s "book") "Two Page")
        ((== s "panorama") "Panorama")
        (else s)))

(define (page-rendering-options)
  (if (in-beamer?)
      '("Single Page" "Continuous Scroll" "Screen"
        "Beamer" "Two Page" "Panorama")
      '("Single Page" "Continuous Scroll" "Screen"
        "Two Page" "Panorama")))

(define (encode-crop-marks s)
  (cond ((== s "none") "")
        (else s)))

(define (decode-crop-marks s)
  (cond ((== s "") "none")
        (else s)))

(define (pn-name prefix index)
  (string-append prefix (number->string index)))

(define (make-pn-m-stree name first)
  (let* ((first-num (string->number first))
         (offset (if (and first-num (>= first-num 1)) (- first-num 1) 0)))
    `(macro (minus (value "page-nr") ,(number->string offset)))))

(define (make-pn-l-stree name m-name style)
  (let ((m-sym (string->symbol m-name)))
    `(macro (if (less (,m-sym) "1") "" (number (,m-sym) ,style)))))

(define (make-pn-g-stree name prev-name l-name ps pe)
  (let ((prev-sym (string->symbol prev-name))
    (l-sym (string->symbol l-name)))
  `(macro (if (or (less (value "page-nr") ,ps)
      (greater (value "page-nr") ,pe))
      (,prev-sym)
      (,l-sym)))))

(define (assign-page-number u pf ps pe nt)
  (let* ((seed (string->number (initial-get u "pn-next")))
         (next (if (and (integer? seed) (>= seed 1)) seed 1))
         (m-name (pn-name "pn-m" next))
         (l-name (pn-name "pn-l" next))
         (g-name (pn-name "pn-g" next))
         (prev-g-name (pn-name "pn-g" (- next 1))))
    (initial-set u "page-first" "1")
    (when (= next 1)
      (initial-set-tree u "pn-g0" '(macro (value "page-nr"))))
    (initial-set-tree u m-name (make-pn-m-stree m-name pf))
    (initial-set-tree u l-name (make-pn-l-stree l-name m-name nt))
    (initial-set-tree u g-name (make-pn-g-stree g-name prev-g-name l-name ps pe))
    (initial-set-tree u "page-the-page" `(macro (,(string->symbol g-name))))
    (initial-set u "pn-next" (number->string (+ next 1)))
    (refresh-window)))

(tm-widget ((page-number-style-editor u) quit)
  (let* ((pf "") (ps "") (pe "") (nt "arabic")
         (filled? (lambda (s) (and (string? s) (!= s "")))))
  (centered
    (aligned
      (item (text "Applying from:")
        (input (set! ps answer) "string" (list ps) "6em"))
      (item (text "Applying to:")
        (input (set! pe answer) "string" (list pe) "6em"))
      (item (text "First page:")
        (input (set! pf answer) "string" (list pf) "6em"))
      (item (text "Number style:")
        (enum (set! nt (cond ((== answer "1, 2, 3") "arabic")
                             ((== answer "i, ii, iii") "roman")
                             ((== answer "I, II, III") "Roman")
                             ((== answer "一, 二, 三") "hanzi")
                             (else answer)))
              '("1, 2, 3" "i, ii, iii" "I, II, III" "一, 二, 三")
              "1, 2, 3"
              "10em"))))
  ======
  (explicit-buttons
    (hlist
      >>>
      ("Cancel" (quit))
      // //
      ("Ok"
        (when (and (filled? pf) (filled? ps)
                    (filled? pe) (filled? nt))
          (assign-page-number u pf ps pe nt)
          (quit)))))))

(define (open-page-number-style-window u)
  (dialogue-window (page-number-style-editor u) noop
                   "Page number style layer"))

(tm-define (set-page-number-style-window-state opened?)
  (set-auxiliary-widget-state opened? 'page-number-style))

(tm-define (open-document-page-number)
  (:interactive #t)
  (change-auxiliary-widget-focus)
  (let ((u (current-buffer)))
    (auxiliary-widget (page-number-style-editor u)
                      noop
                      "Page number style" u)
    (set-page-number-style-window-state #t)))

(register-auxiliary-widget-type 'page-number-style (list open-document-page-number))

(tm-widget (page-formatter-format u quit)
  (centered
    (refreshable "page-format-settings"
      (aligned
        (item (text "Page rendering:")
          (enum (initial-set-page-rendering u (encode-rendering answer))
                (page-rendering-options)
                (decode-rendering (initial-get-page-rendering u)) "10em"))
        (item (text "Page type:")
          (enum (begin
                  (initial-set u "page-type" (page-type-raw answer))
                  (when (!= (page-type-raw answer) "user")
                    (initial-set u "page-width" "auto")
                    (initial-set u "page-height" "auto"))
                  (refresh-now "page-user-format-settings")
                  (refresh-now "page-format-settings"))
                (if (== (initial-get u "page-type") "user")
                    (cons-new (string-append (get-init "page-width") " x " (get-init "page-height"))
                              (page-size-list u))
                    (page-size-list u))
                (if (== (initial-get u "page-type") "user")
                    (string-append (get-init "page-width") " x " (get-init "page-height"))
                    (page-type-pretty (initial-get u "page-type"))) "10em"))
        (item (text "Orientation:")
          (enum (initial-set u "page-orientation" answer)
                '("portrait" "landscape")
                (initial-get u "page-orientation") "10em"))
        (item (text "Crop marks:")
          (enum (initial-set u "page-crop-marks" (encode-crop-marks answer))
                '("none" "a3" "a4" "letter")
                (decode-crop-marks (initial-get u "page-crop-marks"))
                "10em")))))
  ===
  (centered
    (refreshable "page-user-format-settings"
      (when (== (initial-get u "page-type") "user")
        (aligned
          (item (when (user-page-size? u) (text "Page width:"))
            (when (user-page-size? u)
              (enum (initial-set u "page-width" answer)
                    (list (initial-get u "page-width") "")
                    (initial-get u "page-width") "10em")))
          (item (when (user-page-size? u) (text "Page height:"))
            (when (user-page-size? u)
              (enum (initial-set u "page-height" answer)
                    (list (initial-get u "page-height") "")
                    (initial-get u "page-height") "10em")))))))
  ======
  (explicit-buttons
   (hlist
     >>>
     ("Reset"
      (initial-default u "page-medium" "page-type" "page-orientation"
                         "page-border" "page-packet" "page-offset"
                         "page-width" "page-height" "page-crop-marks")
      (refresh-now "page-format-settings")
      (refresh-now "page-user-format-settings"))
     // //
     ("Ok" (quit)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Margins
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (page-formatter-margins u quit)
  (padded
    (refreshable "page-margin-toggles"
      (centered
        (aligned
          (meti (hlist // (text "Determine margins from text width"))
            (toggle (begin
                      (initial-set u "page-width-margin"
                                   (if answer "true" "false"))
                      (refresh-now "page-margin-settings"))
                    (== (initial-get u "page-width-margin") "true")))
          (meti (hlist // (text "Same screen margins as on paper"))
            (toggle (begin
                      (initial-set u "page-screen-margin"
                                   (if answer "false" "true"))
                      (refresh-now "page-screen-margin-settings"))
                    (!= (initial-get u "page-screen-margin") "true"))))))
    ======
    (hlist
      (refreshable "page-margin-settings"
        (hlist (bold (text "Margins on paper")))
        === ===
        (if (!= (initial-get u "page-width-margin") "true")
            (aligned
              (item (text "(Odd page) Left:")
                (input (initial-set u "page-odd" answer) "string"
                       (list (initial-get u "page-odd")) "6em"))
              (item (text "(Even page) Left:")
                (input (initial-set u "page-even" answer) "string"
                       (list (initial-get u "page-odd")) "6em"))
              (item (text "(Odd page) Right:")
                (input (initial-set u "page-right" answer) "string"
                       (list (initial-get u "page-right")) "6em"))
              (item (text "Top:")
                (input (initial-set u "page-top" answer) "string"
                       (list (initial-get u "page-top")) "6em"))
              (item (text "Bottom:")
                (input (initial-set u "page-bot" answer) "string"
                       (list (initial-get u "page-bot")) "6em"))))
        (if (== (initial-get u "page-width-margin") "true")
            (aligned
              (item (text "Text width:")
                (input (initial-set u "par-width" answer) "string"
                       (list (initial-get u "par-width")) "6em"))
              (item (text "Odd page shift:")
                (input (initial-set u "page-odd-shift" answer) "string"
                       (list (initial-get u "page-odd-shift")) "6em"))
              (item (text "Even page shift:")
                (input (initial-set u "page-even-shift" answer) "string"
                       (list (initial-get u "page-even-shift")) "6em"))
              (item (text "Top:")
                (input (initial-set u "page-top" answer) "string"
                       (list (initial-get u "page-top")) "6em"))
              (item (text "Bottom:")
                (input (initial-set u "page-bot" answer) "string"
                       (list (initial-get u "page-bot")) "6em"))))
        (glue #f #t 0 0))
      /// ///
      (refreshable "page-screen-margin-settings"
        (when (== (initial-get u "page-screen-margin") "true")
          (hlist (bold (text "Margins on screen")))
          === ===
          (aligned
            (item (text "Left:")
              (input (initial-set u "page-screen-left" answer) "string"
                     (list (initial-get u "page-screen-left")) "6em"))
            (item (text "Right:")
              (input (initial-set u "page-screen-right" answer) "string"
                     (list (initial-get u "page-screen-right")) "6em"))
            (item (text "Top:")
              (input (initial-set u "page-screen-top" answer) "string"
                     (list (initial-get u "page-screen-top")) "6em"))
            (item (text "Bottom:")
              (input (initial-set u "page-screen-bot" answer) "string"
                     (list (initial-get u "page-screen-bot")) "6em")))
          (glue #f #t 0 0)))))
  ======
  (explicit-buttons
   (hlist
     >>>
     ("Reset"
      (initial-default u "page-odd" "page-even" "page-right"
                         "page-top" "page-bot" "par-width"
                         "page-odd-shift" "page-even-shift"
                         "page-screen-left" "page-screen-right"
                         "page-screen-top" "page-screen-bot"
                         "page-width-margin" "page-screen-margin")
      (refresh-now "page-margin-toggles")
      (refresh-now "page-margin-settings")
      (refresh-now "page-screen-margin-settings"))
     // //
     ("Ok" (quit)))))

(tm-widget (page-formatter-margins u quit)
  (:require (style-has? "std-latex-dtd"))
  (padded
    (centered
      (text "This style specifies page margins in the TeX way"))
    ===
    (refreshable "page-margin-toggles"
      (centered
        (aligned
          (meti (hlist // (text "Same screen margins as on paper"))
            (toggle (begin
                      (initial-set u "page-screen-margin"
                                   (if answer "false" "true"))
                      (refresh-now "page-screen-margin-settings"))
                    (!= (initial-get u "page-screen-margin") "true"))))))
    ======
    (hlist
      (refreshable "page-tex-hor-margins"
        (hlist (bold (text "Horizontal margins")))
        === ===
	(aligned
	  (item (text "oddsidemargin:")
	    (input (initial-set u "tex-odd-side-margin" answer) "string"
		   (list (initial-get u "tex-odd-side-margin")) "6em"))
	  (item (text "evensidemargin:")
	    (input (initial-set u "tex-even-side-margin" answer) "string"
		   (list (initial-get u "tex-even-side-margin")) "6em"))
	  (item (text "textwidth:")
	    (input (initial-set u "tex-text-width" answer) "string"
		   (list (initial-get u "tex-text-width")) "6em"))
	  (item (text "linewidth:")
	    (input (initial-set u "tex-line-width" answer) "string"
		   (list (initial-get u "tex-line-width")) "6em"))
	  (item (text "columnwidth:")
	    (input (initial-set u "tex-column-width" answer) "string"
		   (list (initial-get u "tex-column-width")) "6em")))
        (glue #f #t 0 0))
      /// ///
      (refreshable "page-tex-ver-margins"
        (hlist (bold (text "Vertical margins")))
        === ===
	(aligned
	  (item (text "topmargin:")
	    (input (initial-set u "tex-top-margin" answer) "string"
		   (list (initial-get u "tex-top-margin")) "6em"))
	  (item (text "headheight:")
	    (input (initial-set u "tex-head-height" answer) "string"
		   (list (initial-get u "tex-head-height")) "6em"))
	  (item (text "headsep:")
	    (input (initial-set u "tex-head-sep" answer) "string"
		   (list (initial-get u "tex-head-sep")) "6em"))
	  (item (text "textheight:")
	    (input (initial-set u "tex-text-height" answer) "string"
		   (list (initial-get u "tex-text-height")) "6em"))
	  (item (text "footskip:")
	    (input (initial-set u "tex-foot-skip" answer) "string"
		   (list (initial-get u "tex-foot-skip")) "6em")))
        (glue #f #t 0 0))
      /// ///
      (refreshable "page-screen-margin-settings"
        (when (== (initial-get u "page-screen-margin") "true")
          (hlist (bold (text "Margins on screen")))
          === ===
          (aligned
            (item (text "Left:")
              (input (initial-set u "page-screen-left" answer) "string"
                     (list (initial-get u "page-screen-left")) "6em"))
            (item (text "Right:")
              (input (initial-set u "page-screen-right" answer) "string"
                     (list (initial-get u "page-screen-right")) "6em"))
            (item (text "Top:")
              (input (initial-set u "page-screen-top" answer) "string"
                     (list (initial-get u "page-screen-top")) "6em"))
            (item (text "Bottom:")
              (input (initial-set u "page-screen-bot" answer) "string"
                     (list (initial-get u "page-screen-bot")) "6em")))
          (glue #f #t 0 0)))))
  ======
  (explicit-buttons
   (hlist
     >>>
     ("Reset"
      (initial-default u "tex-odd-side-margin" "tex-even-side-margin"
		         "tex-text-width" "tex-line-width" "tex-column-width"
                         "tex-top-margin" "tex-head-height" "tex-head-sep"
                         "tex-text-height" "tex-foot-skip"
                         "page-screen-left" "page-screen-right"
                         "page-screen-top" "page-screen-bot"
                         "page-width-margin" "page-screen-margin")
      (refresh-now "page-margin-toggles")
      (refresh-now "page-tex-hor-margins")
      (refresh-now "page-tex-ver-margins")
      (refresh-now "page-screen-margin-settings"))
     // //
     ("Ok" (quit)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Breaking
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (page-formatter-breaking u quit)
  (padded
    (refreshable "page-breaking-settings"
      (aligned
        (item (text "Page breaking algorithm:")
          (enum (initial-set u "page-breaking" answer)
                '("sloppy" "professional")
                (initial-get u "page-breaking") "10em"))
        (item (text "Allowed page height reduction:")
          (enum (initial-set u "page-shrink" answer)
                (cons-new (initial-get u "page-shrink")
                          '("0cm" "0.5cm" "1cm" ""))
                (initial-get u "page-shrink") "10em"))
        (item (text "Allowed page height extension:")
          (enum (initial-set u "page-extend" answer)
                (cons-new (initial-get u "page-extend")
                          '("0cm" "0.5cm" "1cm" ""))
                (initial-get u "page-extend") "10em"))
        (item (text "Vertical space stretchability:")
          (enum (initial-set u "page-flexibility" answer)
                (cons-new (initial-get u "page-flexibility")
                          '("0" "0.25" "0.5" "0.75" "1" ""))
                (initial-get u "page-flexibility") "10em")))))
  === ===
  (explicit-buttons
   (hlist
     >>>
     ("Reset"
      (initial-default u "page-breaking" "page-shrink"
                         "page-extend" "page-flexibility")
      (refresh-now "page-breaking-settings"))
     // //
     ("Ok" (quit)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Headers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define header-parameters
  (list "page-odd-header" "page-even-header"
        "page-odd-footer" "page-even-footer"))

(define (header-buffer var)
  (string->url
            (string-append "tmfs://aux/" var "/" 
                           (url->string (url-tail (get-auxiliary-widget-parent-url))))))

(define (header-buffers)
  (map header-buffer header-parameters))

(define (get-field-contents u)
  (and-with t (tm->stree (buffer-get-body u))
    (when (tm-func? t 'document 1)
      (set! t (tm-ref t 0)))
    t))

(define (apply-headers-settings u)
  (with l (list)
    (for (var header-parameters)
      (and-with doc (get-field-contents (header-buffer var))
        (set! l (cons `(,var ,doc) l))))
    (when (nnull? l)
      (delayed
        (:idle 10)
        (for (x l) (initial-set-tree u (car x) (cadr x)))
        (refresh-window)))))

(define (editing-headers?)
  (in? (current-buffer)
       (map header-buffer header-parameters)))

(tm-widget (page-formatter-headers u style quit)
  (padded
    (refreshable "page-header-settings"
      (for (var header-parameters)
        (bold (text (eval (parameter-name var))))
        ===
        (resize "480px" "100px"
            (texmacs-input `(document ,(initial-get-tree u var))
                           `(style (tuple ,@style "gui-base"))
                         (header-buffer var)))
        ===)))
  === ===
  (explicit-buttons
   (hlist
     // // // // //
     (text "Insert:")
     // //
     ("Tab" (when (editing-headers?) (make-htab "5mm")))
     // //
     ("Page number" (when (editing-headers?) (make 'page-the-page)))
     // //
     ("Total pages" (when (editing-headers?) (make 'page-the-total)))
     // //
     >>>
     // //
     ;;("Reset"
     ;; (initial-default u header-parameters)
     ;; (refresh-now "page-header-settings"))
     ;;// //
     ("Ok" (apply-headers-settings u) (begin (quit) (buffer-focus u #t)))
     // // // // //)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Advanced Headers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (make-header-condition start end parity content)
  (let* ((page-nr (list 'value "page-nr"))
         (range-test `(not (or (less ,page-nr ,start) (greater ,page-nr ,end)))))
    `(if ,range-test ,content "")))

(define (assign-advanced-header u start end parity content)
  (let* ((content-tree (if content content ""))
         (new-if (make-header-condition start end parity content-tree))  ; Scheme list: (if cond then else)
         (new-cond (cadr new-if))
         (new-then (caddr new-if)))

    ;; 将条件树解析为 (cond content) 对的列表
    (define (tree->pairs stree)
      (cond ((not (tm-func? stree 'if 3))
             (if (== stree "")  ; 空字符串表示没有内容
                 '()
                 (list (cons #t stree))))  ; #t 表示无条件，始终显示
            (else
             (cons (cons (cadr stree) (caddr stree))
                   (tree->pairs (cadddr stree))))))

    ;; 将 (cond content) 对列表转换回条件树
    (define (pairs->tree pairs)
      (if (null? pairs)
          ""
          (let ((pair (car pairs)))
            (if (eq? (car pair) #t)
                (cdr pair)  ; 无条件内容作为最终 else
                ;; 构建条件链，递归处理剩余对
                (let ((rest (cdr pairs)))
                  `(if ,(car pair) ,(cdr pair) ,(pairs->tree rest)))))))

    ;; 合并新条件到对列表中
    (define (merge-pairs pairs cond then)
      ;; 查找是否有相同条件并构建新列表
      (define (process-pairs pairs found new-pairs)
        (if (null? pairs)
            (if found
                (reverse new-pairs)  ; 已替换，返回新列表
                (cons (cons cond then) (reverse new-pairs))) ; 添加新条件到前面
            (let ((pair (car pairs)))
              (if (and (not (eq? (car pair) #t))
                       (tm-equal? (car pair) cond))
                  (process-pairs (cdr pairs) #t (cons (cons cond then) new-pairs)) ; 替换
                  (process-pairs (cdr pairs) found (cons pair new-pairs))))))
      (process-pairs pairs #f '()))

    ;; 主合并函数
    (define (merge-condition-tree stree)
      (let* ((pairs (tree->pairs stree))
             (merged-pairs (merge-pairs pairs new-cond new-then)))
        (pairs->tree merged-pairs)))

    (cond ((or (== parity "odd") (== parity "odd page"))
           (let ((old-tree (initial-get-tree u "page-odd-header")))
             (initial-set-tree u "page-odd-header"
               (merge-condition-tree (tm->stree old-tree)))))
          ((or (== parity "even") (== parity "even page"))
           (let ((old-tree (initial-get-tree u "page-even-header")))
             (initial-set-tree u "page-even-header"
               (merge-condition-tree (tm->stree old-tree)))))
          (else
           (let ((old-odd (initial-get-tree u "page-odd-header"))
                 (old-even (initial-get-tree u "page-even-header")))
             (initial-set-tree u "page-odd-header"
               (merge-condition-tree (tm->stree old-odd)))
             (initial-set-tree u "page-even-header"
               (merge-condition-tree (tm->stree old-even)))))))
  (refresh-window))

(tm-widget (page-formatter-advanced-header u style quit)
  (let* ((current-tree (initial-get-tree u "page-odd-header"))
         (content-tree (if (tm-func? current-tree 'if 3)
                          (tm-ref current-tree 2)
                          current-tree))
         (start "1") (end (number->string (get-page-count))) (parity "any") (content ""))
    (centered
      (refreshable "advanced-header-settings"
        (aligned
          (item (text "Applying from:")
            (input (set! start answer) "string" (list start) "6em"))
          (item (text "Applying to:")
            (input (set! end answer) "string" (list end) "6em"))
          (item (text "Parity:")
            (enum (begin (set! parity answer))
                  '("odd page" "even page" "any")
                  "any"
                  "10em"))
          (item (text "Content:")
            (resize "480px" "100px"
              (texmacs-input `(document ,content-tree)
                             `(style (tuple ,@style "gui-base"))
                             (string->url "tmfs://aux/advanced-header")))))))
    ===
    (explicit-buttons
      (hlist
        >>>
        ("Ok"
           (with content (get-field-contents (string->url "tmfs://aux/advanced-header"))
             (assign-advanced-header u start end parity content))
           (quit))
        // //
        ("Cancel" (quit))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Advanced Footers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (make-footer-condition start end parity content)
  (let* ((page-nr (list 'value "page-nr"))
         (range-test `(not (or (less ,page-nr ,start) (greater ,page-nr ,end)))))
    `(if ,range-test ,content "")))

(define (assign-advanced-footer u start end parity content)
  (let* ((content-tree (if content content ""))
         (new-if (make-footer-condition start end parity content-tree))  ; Scheme list: (if cond then else)
         (new-cond (cadr new-if))
         (new-then (caddr new-if)))

    ;; 将条件树解析为 (cond content) 对的列表
    (define (tree->pairs stree)
      (cond ((not (tm-func? stree 'if 3))
             (if (== stree "")  ; 空字符串表示没有内容
                 '()
                 (list (cons #t stree))))  ; #t 表示无条件，始终显示
            (else
             (cons (cons (cadr stree) (caddr stree))
                   (tree->pairs (cadddr stree))))))

    ;; 将 (cond content) 对列表转换回条件树
    (define (pairs->tree pairs)
      (if (null? pairs)
          ""
          (let ((pair (car pairs)))
            (if (eq? (car pair) #t)
                (cdr pair)  ; 无条件内容作为最终 else
                ;; 构建条件链，递归处理剩余对
                (let ((rest (cdr pairs)))
                  `(if ,(car pair) ,(cdr pair) ,(pairs->tree rest)))))))

    ;; 合并新条件到对列表中
    (define (merge-pairs pairs cond then)
      ;; 查找是否有相同条件并构建新列表
      (define (process-pairs pairs found new-pairs)
        (if (null? pairs)
            (if found
                (reverse new-pairs)  ; 已替换，返回新列表
                (cons (cons cond then) (reverse new-pairs))) ; 添加新条件到前面
            (let ((pair (car pairs)))
              (if (and (not (eq? (car pair) #t))
                       (tm-equal? (car pair) cond))
                  (process-pairs (cdr pairs) #t (cons (cons cond then) new-pairs)) ; 替换
                  (process-pairs (cdr pairs) found (cons pair new-pairs))))))
      (process-pairs pairs #f '()))

    ;; 主合并函数
    (define (merge-condition-tree stree)
      (let* ((pairs (tree->pairs stree))
             (merged-pairs (merge-pairs pairs new-cond new-then)))
        (pairs->tree merged-pairs)))

    (cond ((or (== parity "odd") (== parity "odd page"))
           (let ((old-tree (initial-get-tree u "page-odd-footer")))
             (initial-set-tree u "page-odd-footer"
               (merge-condition-tree (tm->stree old-tree)))))
          ((or (== parity "even") (== parity "even page"))
           (let ((old-tree (initial-get-tree u "page-even-footer")))
             (initial-set-tree u "page-even-footer"
               (merge-condition-tree (tm->stree old-tree)))))
          (else
           (let ((old-odd (initial-get-tree u "page-odd-footer"))
                 (old-even (initial-get-tree u "page-even-footer")))
             (initial-set-tree u "page-odd-footer"
               (merge-condition-tree (tm->stree old-odd)))
             (initial-set-tree u "page-even-footer"
               (merge-condition-tree (tm->stree old-even)))))))
  (refresh-window))

(tm-widget (page-formatter-advanced-footer u style quit)
  (let* ((current-tree (initial-get-tree u "page-odd-footer"))
         (content-tree (if (tm-func? current-tree 'if 3)
                          (tm-ref current-tree 2)
                          current-tree))
         (start "1") (end (number->string (get-page-count))) (parity "any") (content ""))
    (centered
      (refreshable "advanced-footer-settings"
        (aligned
          (item (text "Applying from:")
            (input (set! start answer) "string" (list start) "6em"))
          (item (text "Applying to:")
            (input (set! end answer) "string" (list end) "6em"))
          (item (text "Parity:")
            (enum (begin (set! parity answer))
                  '("odd page" "even page" "any")
                  "any"
                  "10em"))
          (item (text "Content:")
            (resize "480px" "100px"
              (texmacs-input `(document ,content-tree)
                             `(style (tuple ,@style "gui-base"))
                             (string->url "tmfs://aux/advanced-footer")))))))
    ===
    (explicit-buttons
      (hlist
        >>>
        ("Ok"
           (with content (get-field-contents (string->url "tmfs://aux/advanced-footer"))
             (assign-advanced-footer u start end parity content))
           (quit))
        // //
        ("Cancel" (quit))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((document-page-formatter u style) quit)
  (padded
    (tabs
      (tab (text "Format")
        (padded
          (dynamic (page-formatter-format u quit))))
      (tab (text "Margins")
        (padded
          (dynamic (page-formatter-margins u quit))))
      (tab (text "Breaking")
        (padded
          (dynamic (page-formatter-breaking u quit))))
      (tab (text "Advanced header")
        (padded
          (dynamic (page-formatter-advanced-header u style quit))))
      (tab (text "Advanced footer")
        (padded
          (dynamic (page-formatter-advanced-footer u style quit)))))))

(tm-define (open-document-page-format-window)
  (:interactive #t)
  (let* ((u  (current-buffer))
         (st (embedded-style-list "macro-editor")))
    (apply dialogue-window
           (cons* (document-page-formatter u st)
                  noop "Document page format"
                  (header-buffers)))))

(tm-define (open-document-page-format)
  (:interactive #t)
  (if (side-tools?)
      (tool-select :right 'document-page-tool)
      (open-document-page-format-window)))

(tm-define (open-page-headers-footers-window)
  (:interactive #t)
  ;(change-auxiliary-widget-focus)
  (let* ((u  (current-buffer))
         (st (embedded-style-list "macro-editor")))
    (apply auxiliary-widget
           (cons* (lambda (quit) (page-formatter-headers u st quit))
                  noop (translate "Headers and footers")
                  (header-buffers)))
    (set-page-headers-footers-window-state #t)
    (buffer-focus (header-buffer "page-odd-header") #t)))

(tm-define (open-page-headers-footers)
  (:interactive #t)
  (open-page-headers-footers-window))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Metadata
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((document-metadata-editor u) quit)
  (padded
    (refreshable "document-metadata"
      (aligned
        (item (text "Title:")
          (input (initial-set u "global-title" answer) "string"
                 (list (buffer-get-metadata u "title")) "24em"))
        (item (text "Author:")
          (input (initial-set u "global-author" answer) "string"
                 (list (buffer-get-metadata u "author")) "24em"))
        (item (text "Subject:")
          (input (initial-set u "global-subject" answer) "string"
                 (list (buffer-get-metadata u "subject")) "24em"))))
    ======
    (explicit-buttons
      (hlist
        >>>
        ("Reset"
         (initial-default u "global-title" "global-author" "global-subject")
         (refresh-now "document-metadata"))
        // //
        ("Ok" (quit))))))

(tm-define (open-document-metadata-window)
  (:interactive #t)
  (let* ((u (current-buffer)))
    (dialogue-window (document-metadata-editor u) noop "Document metadata")))

(tm-define (open-document-metadata)
  (:interactive #t)
  (if (side-tools?)
      (tool-select :right 'document-metadata-tool)
      (open-document-metadata-window)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Color
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (page-colors-background u)
  (pick-background "" (initial-set-tree u "bg-color" answer)))

(tm-widget (page-colors-foreground u)
  (pick-color (initial-set-tree u "color" answer)))

(tm-widget ((document-colors-picker u) quit)
  (padded
    (refreshable "page-colors"
      (tabs
        (tab (text "Background")
          (padded
            (dynamic (page-colors-background u))))
        (tab (text "Foreground")
          (padded
            (dynamic (page-colors-foreground u))))))
    ======
    (explicit-buttons
      (hlist
        >>>
        ("Reset"
         (initial-default u "bg-color" "color")
         (refresh-now "page-colors"))
        // //
        ("Ok" (quit))))))

(tm-define (open-document-colors-window)
  (:interactive #t)
  (with u (current-buffer)
    (dialogue-window (document-colors-picker u) noop "Document colors")))

(tm-define (open-document-colors)
  (:interactive #t)
  (if (side-tools?)
      (tool-select :right 'document-colors-tool)
      (open-document-colors-window)))

(register-auxiliary-widget-type 'page-headers-footers (list open-page-headers-footers-window))
