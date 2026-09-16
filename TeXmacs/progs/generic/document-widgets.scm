
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
    (generic document-edit)
    (kernel gui menu-widget)
    (generic format-widgets)
    (utils library cursor)
  ) ;:use
) ;texmacs-module


(tm-define (set-page-headers-footers-window-state opened?)
  (set-auxiliary-widget-state opened? 'page-headers-footers)
) ;tm-define


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Style chooser widget (still to be implemented)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (select-style-among-widget l)
  (resize '("300px" "300px" "300px")
    '("200px" "300px" "1000px")
    (scrollable (choice (set-main-style answer) l "generic"))
  ) ;resize
) ;tm-widget

(tm-widget (select-common-style-widget)
  (dynamic (select-style-among-widget (list "article" "beamer" "book" "browser"
                                        "exam" "generic" "letter" "manual"
                                        "seminar" "source"
                                      ) ;list
           ) ;select-style-among-widget
  ) ;dynamic
) ;tm-widget

(tm-widget (select-education-style-widget)
  (dynamic (select-style-among-widget (list "compact" "exam")))
) ;tm-widget

(tm-widget (select-article-style-widget)
  (dynamic (select-style-among-widget (list "article" "tmarticle")))
) ;tm-widget

(tm-widget (select-any-style-widget)
  (dynamic (select-style-among-widget (list "article" "tmarticle")))
) ;tm-widget

(tm-widget (select-style-widget)
  (tabs (tab (text "Common") (dynamic (select-common-style-widget)))
    (tab (text "Education") (dynamic (select-education-style-widget)))
    (tab (text "Article") (dynamic (select-article-style-widget)))
    (tab (text "Any") (dynamic (select-any-style-widget)))
  ) ;tabs
) ;tm-widget

(tm-define (open-style-selector)
  (:interactive #t)
  (top-window select-style-widget "Select document style")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Source -> Preferences
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((source-tree-preferences-editor u) quit)
  (padded
    (refreshable "source-tree-preferences"
      (aligned (item (text "Main presentation style:")
                 (enum (initial-set u "src-style" answer)
                   '("angular" "scheme" "functional" "latex")
                   (initial-get u "src-style")
                   "10em"
                 ) ;enum
               ) ;item
        (item (text "Tags with special rendering:")
          (enum (initial-set u "src-special" answer)
            '("raw" "format" "normal" "maximal")
            (initial-get u "src-special")
            "10em"
          ) ;enum
        ) ;item
        (item (text "Compactification:")
          (enum (initial-set u "src-compact" answer)
            '("none" "inline" "normal" "inline args" "all")
            (initial-get u "src-compact")
            "10em"
          ) ;enum
        ) ;item
        (item (text "Closing style:")
          (enum (initial-set u "src-close" answer)
            '("repeat" "long" "compact" "minimal")
            (initial-get u "src-close")
            "10em"
          ) ;enum
        ) ;item
      ) ;aligned
    ) ;refreshable
    ======
    (explicit-buttons (hlist >>>
                       ("Reset"
                         (initial-default u "src-style" "src-special" "src-compact" "src-close")
                         (refresh-now "source-tree-preferences")
                       ) ;
                       //
                       //
                       ("Ok" (quit))
                      ) ;hlist
    ) ;explicit-buttons
  ) ;padded
) ;tm-widget

(tm-define (open-source-tree-preferences-window)
  (:interactive #t)
  (with u
    (current-buffer)
    (dialogue-window (source-tree-preferences-editor u)
      noop
      "Document source tree preferences"
    ) ;dialogue-window
  ) ;with
) ;tm-define

(tm-define (open-source-tree-preferences)
  (:interactive #t)
  (if (side-tools?)
    (tool-select :right 'source-tree-preferences-tool)
    (open-source-tree-preferences-window)
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Paragraph
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-document-paragraph-format-window)
  (:interactive #t)
  ;; 走 QML 对话框（与「格式→段落」同骨架，区别在数据通路）：register-specs 存
  ;; 'document scope specs，cpp-paragraph-format-dialog 开同一个 QML 对话框。
  ;; 'document 走 init 通路：get-init 读、init-multi 写文档 initial；基础 tab
  ;; 隐藏 par-left/par-right；重置走 init-default 恢复默认，Cancel 经快照写回。
  (with specs
    (list 'document get-init init-multi)
    (cpp-paragraph-format-dialog (paragraph-format-register-specs specs))
  ) ;with
) ;tm-define

(tm-define (open-document-paragraph-format)
  (:interactive #t)
  (if (side-tools?)
    (tool-select :right 'document-paragraph-tool)
    (open-document-paragraph-format-window)
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Format
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (page-size-list u)
  (if (initial-defined? u "beamer-style")
    (list "16:9" "8:5" "4:3" "5:4")
    (map page-type-pretty
      (list "a3" "a4" "a5" "b4" "b5" "letter" "legal" "executive")
    ) ;map
  ) ;if
) ;define

(define (user-page-size? u)
  (== (initial-get u "page-type") "user")
) ;define

(define (encode-rendering s)
  (cond ((== s "Single Page") "paper")
        ((== s "Continuous Scroll") "papyrus")
        ((== s "Screen") "automatic")
        ((== s "Beamer") "beamer")
        ((== s "Two Page") "book")
        ((== s "Panorama") "panorama")
        (else s)
  ) ;cond
) ;define

(define (decode-rendering s)
  (cond ((== s "paper") "Single Page")
        ((== s "papyrus") "Continuous Scroll")
        ((== s "automatic") "Screen")
        ((== s "beamer") "Beamer")
        ((== s "book") "Two Page")
        ((== s "panorama") "Panorama")
        (else s)
  ) ;cond
) ;define

(define (page-rendering-options)
  (if (in-beamer?)
    '("Single Page" "Continuous Scroll" "Beamer" "Two Page" "Panorama")
    '("Single Page" "Continuous Scroll" "Two Page" "Panorama")
  ) ;if
) ;define

(define (encode-crop-marks s)
  (cond ((== s "none") "")
        (else s)
  ) ;cond
) ;define

(define (decode-crop-marks s)
  (cond ((== s "") "none")
        (else s)
  ) ;cond
) ;define

(define (pn-name prefix index)
  (string-append prefix (number->string index))
) ;define

(define (make-pn-m-stree name first)
  (let* ((first-num (string->number first))
         (offset (if (and first-num (>= first-num 1)) (- first-num 1) 0))
        ) ;
    `(macro (minus (value "page-nr") ,(number->string offset)))
  ) ;let*
) ;define

(define (make-pn-l-stree name m-name style)
  (let ((m-sym (string->symbol m-name)))
    `(macro (if (less (,m-sym) ,"1") ,"" (number (,m-sym) ,style)))
  ) ;let
) ;define

(define (make-pn-g-stree name prev-name l-name ps pe)
  (let ((prev-sym (string->symbol prev-name)) (l-sym (string->symbol l-name)))
    `(macro (if (or (less (value "page-nr") ,ps)
                  (greater (value "page-nr") ,pe))
              (,prev-sym)
              (,l-sym)))
  ) ;let
) ;define

(define pn-current-buffer #f)

(tm-define (pn-get-buffer) (or pn-current-buffer (current-buffer)))

(tm-define (pn-extract-range g-stree)
  (catch #t
    (lambda ()
      (let* ((body
               (if (and (pair? g-stree) (== (car g-stree) 'macro)) (cadr g-stree) g-stree)
             ) ;body
             (cond-part (and (pair? body) (== (car body) 'if) (cadr body)))
             (or-clause (and (pair? cond-part) (== (car cond-part) 'or) cond-part))
             (less-part (cadr or-clause))
             (greater-part (caddr or-clause))
             (ps (caddr less-part))
             (pe-expr (caddr greater-part))
             (pe (if (equal? pe-expr '(page-the-total))
                   "total"
                   (if (string? pe-expr) pe-expr (object->string pe-expr))
                 ) ;if
             ) ;pe
            ) ;
        (cons (if (string? ps) ps (object->string ps)) pe)
      ) ;let*
    ) ;lambda
    (lambda (key . args) #f)
  ) ;catch
) ;tm-define

(tm-define (pn-extract-style l-stree)
  (catch #t
    (lambda ()
      (let ((body
              (if (and (pair? l-stree) (== (car l-stree) 'macro)) (cadr l-stree) l-stree)
            ) ;body
           ) ;
        (cond ((equal? body "") "blank")
              ((and (pair? body) (== (car body) 'if))
               (let* ((else-part (cadddr body))
                      (style (and (pair? else-part) (== (car else-part) 'number) (caddr else-part)))
                     ) ;
                 (if (string? style) style (if (symbol? style) (symbol->string style) "arabic"))
               ) ;let*
              ) ;
              (else "arabic")
        ) ;cond
      ) ;let
    ) ;lambda
    (lambda (key . args) "arabic")
  ) ;catch
) ;tm-define

(define (pn-all-inits u)
  (let* ((raw
           (if u (with-buffer u (tm->stree (get-all-inits))) (tm->stree (get-all-inits)))
         ) ;raw
         (items
           (if (and (pair? raw) (== (car raw) 'collection)) (cdr raw) '())
         ) ;items
        ) ;
    (map
      (lambda (item)
        (if (and (pair? item) (== (car item) 'associate) (pair? (cdr item)))
          (cons (cadr item) (caddr item))
          (cons "" "")
        ) ;if
      ) ;lambda
      items
    ) ;map
  ) ;let*
) ;define

(define (pn-read-rules u)
  (catch #t
    (lambda ()
      ;; 规则从 1 起连续编号，扫到首个缺失的 pn-g<k> 即止，pn-next 仅作存在性标记
      (let ((inits (pn-all-inits u)))
        (let loop
          ((k 1) (res '()))
          (let ((g-stree (assoc-ref inits (pn-name "pn-g" k)))
                (l-stree (assoc-ref inits (pn-name "pn-l" k)))
               ) ;
            (if (not g-stree)
              (reverse res)
              (let ((range (pn-extract-range g-stree))
                    (style (if l-stree (pn-extract-style l-stree) "arabic"))
                   ) ;
                (if range
                  (loop (+ k 1) (cons (list (car range) (cdr range) style) res))
                  (loop (+ k 1) res)
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;let
      ) ;let
    ) ;lambda
    (lambda (key . args) '())
  ) ;catch
) ;define

(define (pn-ui-labels)
  (list (cons 'title (translate "Page number settings"))
    (cons 'hint
      (translate "All pages are arranged vertically and scrollable; drag handle to select range, auto-scrolls near edge"
      ) ;translate
    ) ;cons
    (cons 'secRange (translate "① Range for new rule"))
    (cons 'subRange
      (translate "Directly enter page range, click [to end of document], or drag handle"
      ) ;translate
    ) ;cons
    (cons 'fromPage (translate "From page"))
    (cons 'toPage (translate "to page"))
    (cons 'pageUnit (translate "page unit"))
    (cons 'secStyle (translate "② Choose page number style"))
    (cons 'btnAddRule (translate "+ Add rule"))
    (cons 'secRules (translate "Added rules"))
    (cons 'subRules
      (translate "Rules connect sequentially; only the last rule can be deleted")
    ) ;cons
    (cons 'emptyRules
      (translate "No rules yet — all pages numbered continuously")
    ) ;cons
    (cons 'rangeDone (translate "All pages covered by rules"))
    (cons 'rangePrompt
      (translate "Starting from page %1, drag handle to select end page")
    ) ;cons
    (cons 'rangeUpto (translate "Page %1 ~ Page %2 (%3 pages)"))
    (cons 'toEnd (translate "to end of document"))
    (cons 'physPage (translate "Page %1"))
    (cons 'fromSample (translate "from %1"))
    (cons 'styleArabic (translate "Arabic numerals"))
    (cons 'styleRoman (translate "roman::page_number"))
    (cons 'styleRomanUpper (translate "Roman::page_number"))
    (cons 'styleHanzi (translate "Chinese numerals"))
    (cons 'styleBlank (translate "Hide page numbers"))
    (cons 'styleBlankSample (translate "(hidden)"))
    (cons 'arabicTip
      (translate "Defaulted from page %1 to the end so that page number 1 starts from page %1."
      ) ;translate
    ) ;cons
    (cons 'romanTip
      (translate "Roman numeral page numbering (i, ii... or I, II...) is generally used for the table of contents and preface of books."
      ) ;translate
    ) ;cons
  ) ;list
) ;define

(tm-define (pn-qml-meta)
  (catch #t
    (lambda ()
      (let* ((u (pn-get-buffer))
             (total
               (max 1 (if u (with-buffer u (get-page-count)) (get-page-count)))
             ) ;total
             (rules (pn-read-rules u))
             (labels (pn-ui-labels))
            ) ;
        `((total . ,total) (rules . ,rules) (labels . ,labels))
      ) ;let*
    ) ;lambda
    (lambda (key . args)
      (list (cons 'total 1) (cons 'rules (list)) (cons 'labels (pn-ui-labels)))
    ) ;lambda
  ) ;catch
) ;tm-define

(tm-define (pn-qml-submit rules)
  (catch #t
    (lambda ()
      (let* ((u (pn-get-buffer)) (had-system? (and u (initial-has? u "pn-next"))))
        (when (and u had-system?)
          (let* ((inits (pn-all-inits u))
                 (pn-vars (filter (lambda (k) (string-starts? k "pn-")) (map car inits)))
                 (all-clean (cons "page-the-page" pn-vars))
                ) ;
            (apply initial-default u all-clean)
          ) ;let*
        ) ;when
        (when (and u (pair? rules))
          (initial-set u "page-first" "1")
          (initial-set-tree u "pn-g0" '(macro (value "page-nr")))
          (let loop
            ((k 1) (rem rules) (last-g "pn-g0"))
            (if (null? rem)
              (begin
                (initial-set-tree u "page-the-page" `(macro (,(string->symbol last-g))))
                (initial-set u "pn-next" (number->string k))
              ) ;begin
              (let* ((rule (car rem))
                     (ps (list-ref rule 0))
                     (pe (list-ref rule 1))
                     (style (list-ref rule 2))
                     (m-name (pn-name "pn-m" k))
                     (l-name (pn-name "pn-l" k))
                     (g-name (pn-name "pn-g" k))
                     (prev-g-name (pn-name "pn-g" (- k 1)))
                     (pe-stree (if (string=? pe "total") '(page-the-total) pe))
                    ) ;
                (if (string=? style "blank")
                  (initial-set-tree u l-name '(macro ""))
                  (begin
                    (initial-set-tree u m-name (make-pn-m-stree m-name ps))
                    (initial-set-tree u l-name (make-pn-l-stree l-name m-name style))
                  ) ;begin
                ) ;if
                (initial-set-tree u
                  g-name
                  (make-pn-g-stree g-name prev-g-name l-name ps pe-stree)
                ) ;initial-set-tree
                (loop (+ k 1) (cdr rem) g-name)
              ) ;let*
            ) ;if
          ) ;let
        ) ;when
        (refresh-window)
      ) ;let*
    ) ;lambda
    (lambda (key . args) (noop))
  ) ;catch
) ;tm-define

(tm-define (open-document-page-number)
  (:interactive #t)
  (set! pn-current-buffer (current-buffer))
  (cpp-page-number-dialog)
) ;tm-define

(tm-widget (page-formatter-format u quit)
  (centered
    (refreshable "page-format-settings"
      (aligned
        (item (text "Page rendering:")
          (enum (initial-set-page-rendering u (encode-rendering answer))
            (page-rendering-options)
            (decode-rendering (initial-get-page-rendering u))
            "10em"
          ) ;enum
        ) ;item
        (item (text "Page type:")
          (enum
            (begin
              (initial-set u "page-type" (page-type-raw answer))
              (when (!= (page-type-raw answer) "user")
                (initial-set u "page-width" "auto")
                (initial-set u "page-height" "auto")
              ) ;when
              (refresh-now "page-user-format-settings")
              (refresh-now "page-format-settings")
            ) ;begin
            (if (== (initial-get u "page-type") "user")
              (cons-new (string-append (get-init "page-width") " x " (get-init "page-height"))
                (page-size-list u)
              ) ;cons-new
              (page-size-list u)
            ) ;if
            (if (== (initial-get u "page-type") "user")
              (string-append (get-init "page-width") " x " (get-init "page-height"))
              (page-type-pretty (initial-get u "page-type"))
            ) ;if
            "10em"
          ) ;enum
        ) ;item
        (item (text "Orientation:")
          (enum (initial-set u "page-orientation" answer)
            '("portrait" "landscape")
            (initial-get u "page-orientation")
            "10em"
          ) ;enum
        ) ;item
        (item (text "Crop marks:")
          (enum (initial-set u "page-crop-marks" (encode-crop-marks answer))
            '("none" "a3" "a4" "letter")
            (decode-crop-marks (initial-get u "page-crop-marks"))
            "10em"
          ) ;enum
        ) ;item
      ) ;aligned
    ) ;refreshable
  ) ;centered
  ===
  (centered
    (refreshable "page-user-format-settings"
      (when (== (initial-get u "page-type") "user")
        (aligned
          (item (when (user-page-size? u)
                  (text "Page width:")
                ) ;when
            (when (user-page-size? u)
              (enum (initial-set u "page-width" answer)
                (list (initial-get u "page-width") "")
                (initial-get u "page-width")
                "10em"
              ) ;enum
            ) ;when
          ) ;item
          (item (when (user-page-size? u)
                  (text "Page height:")
                ) ;when
            (when (user-page-size? u)
              (enum (initial-set u "page-height" answer)
                (list (initial-get u "page-height") "")
                (initial-get u "page-height")
                "10em"
              ) ;enum
            ) ;when
          ) ;item
        ) ;aligned
      ) ;when
    ) ;refreshable
  ) ;centered
  ======
  (explicit-buttons (hlist >>>
                     ("Reset"
                       (initial-default u "page-medium" "page-type"
                         "page-orientation" "page-border" "page-packet"
                         "page-offset" "page-width" "page-height"
                         "page-crop-marks"
                       ) ;initial-default
                       (refresh-now "page-format-settings")
                       (refresh-now "page-user-format-settings")
                     ) ;
                     //
                     //
                     ("Ok" (quit))
                    ) ;hlist
  ) ;explicit-buttons
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Margins
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (page-margin-get-mm-initial u var)
  (let ((val (initial-get u var)))
    (cond ((or (not val) (== val "") (== val "auto")) 30)
          ((tm-length? val)
           (with decoded
             (length-decode val)
             (if decoded (inexact->exact (round (/ decoded 6047.0))) 30)
           ) ;with
          ) ;
          (else 30)
    ) ;cond
  ) ;let
) ;define

(define (page-margin-set-mm-initial u var val)
  (initial-set u var (string-append (number->string val) "mm"))
) ;define

(tm-widget (page-formatter-margins u quit)
  (padded
    (refreshable "page-margin-toggles"
      (centered
        (aligned
          (meti (hlist // (text "Determine margins from text width"))
            (toggle (begin
                      (initial-set u "page-width-margin" (if answer "true" "false"))
                      (refresh-now "page-margin-settings")
                    ) ;begin
              (== (initial-get u "page-width-margin") "true")
            ) ;toggle
          ) ;meti
          (meti (hlist // (text "Same screen margins as on paper"))
            (toggle (begin
                      (initial-set u "page-screen-margin" (if answer "false" "true"))
                      (refresh-now "page-screen-margin-settings")
                    ) ;begin
              (!= (initial-get u "page-screen-margin") "true")
            ) ;toggle
          ) ;meti
        ) ;aligned
      ) ;centered
    ) ;refreshable
    ======
    (hlist
      (refreshable "page-margin-settings"
        (hlist (bold (text "Margins on paper")))
        ===
        ===
        (if (!= (initial-get u "page-width-margin") "true")
          (aligned
            (item (text "(Odd page) Left:")
              (hlist (numeric-input (page-margin-set-mm-initial u "page-odd" answer)
                       "4em"
                       "mm"
                       0
                       500
                       1
                       (page-margin-get-mm-initial u "page-odd")
                     ) ;numeric-input
              ) ;hlist
            ) ;item
            (item (text "(Even page) Left:")
              (hlist (numeric-input (page-margin-set-mm-initial u "page-even" answer)
                       "4em"
                       "mm"
                       0
                       500
                       1
                       (page-margin-get-mm-initial u "page-even")
                     ) ;numeric-input
              ) ;hlist
            ) ;item
            (item (text "(Odd page) Right:")
              (hlist (numeric-input (page-margin-set-mm-initial u "page-right" answer)
                       "4em"
                       "mm"
                       0
                       500
                       1
                       (page-margin-get-mm-initial u "page-right")
                     ) ;numeric-input
              ) ;hlist
            ) ;item
            (item (text "Top:")
              (input (initial-set u "page-top" answer)
                "string"
                (list (initial-get u "page-top"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Bottom:")
              (input (initial-set u "page-bot" answer)
                "string"
                (list (initial-get u "page-bot"))
                "6em"
              ) ;input
            ) ;item
          ) ;aligned
        ) ;if
        (if (== (initial-get u "page-width-margin") "true")
          (aligned
            (item (text "Text width:")
              (input (initial-set u "par-width" answer)
                "string"
                (list (initial-get u "par-width"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Odd page shift:")
              (input (initial-set u "page-odd-shift" answer)
                "string"
                (list (initial-get u "page-odd-shift"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Even page shift:")
              (input (initial-set u "page-even-shift" answer)
                "string"
                (list (initial-get u "page-even-shift"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Top:")
              (input (initial-set u "page-top" answer)
                "string"
                (list (initial-get u "page-top"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Bottom:")
              (input (initial-set u "page-bot" answer)
                "string"
                (list (initial-get u "page-bot"))
                "6em"
              ) ;input
            ) ;item
          ) ;aligned
        ) ;if
        (glue #f #t 0 0)
      ) ;refreshable
      ///
      ///
      (refreshable "page-screen-margin-settings"
        (when (== (initial-get u "page-screen-margin") "true")
          (hlist (bold (text "Margins on screen")))
          ===
          ===
          (aligned
            (item (text "Left:")
              (input (initial-set u "page-screen-left" answer)
                "string"
                (list (initial-get u "page-screen-left"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Right:")
              (input (initial-set u "page-screen-right" answer)
                "string"
                (list (initial-get u "page-screen-right"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Top:")
              (input (initial-set u "page-screen-top" answer)
                "string"
                (list (initial-get u "page-screen-top"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Bottom:")
              (input (initial-set u "page-screen-bot" answer)
                "string"
                (list (initial-get u "page-screen-bot"))
                "6em"
              ) ;input
            ) ;item
          ) ;aligned
          (glue #f #t 0 0)
        ) ;when
      ) ;refreshable
    ) ;hlist
  ) ;padded
  ======
  (explicit-buttons (hlist >>>
                     ("Reset"
                       (initial-default u "page-odd" "page-even" "page-right"
                         "page-top" "page-bot" "par-width" "page-odd-shift"
                         "page-even-shift" "page-screen-left"
                         "page-screen-right" "page-screen-top" "page-screen-bot"
                         "page-width-margin" "page-screen-margin"
                       ) ;initial-default
                       (refresh-now "page-margin-toggles")
                       (refresh-now "page-margin-settings")
                       (refresh-now "page-screen-margin-settings")
                     ) ;
                     //
                     //
                     ("Ok" (quit))
                    ) ;hlist
  ) ;explicit-buttons
) ;tm-widget

(tm-widget (page-formatter-margins u quit)
  (:require (style-has? "std-latex-dtd"))
  (padded (centered (text "This style specifies page margins in the TeX way"))
    ===
    (refreshable "page-margin-toggles"
      (centered
        (aligned
          (meti (hlist // (text "Same screen margins as on paper"))
            (toggle (begin
                      (initial-set u "page-screen-margin" (if answer "false" "true"))
                      (refresh-now "page-screen-margin-settings")
                    ) ;begin
              (!= (initial-get u "page-screen-margin") "true")
            ) ;toggle
          ) ;meti
        ) ;aligned
      ) ;centered
    ) ;refreshable
    ======
    (hlist
      (refreshable "page-tex-hor-margins"
        (hlist (bold (text "Horizontal margins")))
        ===
        ===
        (aligned
          (item (text "oddsidemargin:")
            (input (initial-set u "tex-odd-side-margin" answer)
              "string"
              (list (initial-get u "tex-odd-side-margin"))
              "6em"
            ) ;input
          ) ;item
          (item (text "evensidemargin:")
            (input (initial-set u "tex-even-side-margin" answer)
              "string"
              (list (initial-get u "tex-even-side-margin"))
              "6em"
            ) ;input
          ) ;item
          (item (text "textwidth:")
            (input (initial-set u "tex-text-width" answer)
              "string"
              (list (initial-get u "tex-text-width"))
              "6em"
            ) ;input
          ) ;item
          (item (text "linewidth:")
            (input (initial-set u "tex-line-width" answer)
              "string"
              (list (initial-get u "tex-line-width"))
              "6em"
            ) ;input
          ) ;item
          (item (text "columnwidth:")
            (input (initial-set u "tex-column-width" answer)
              "string"
              (list (initial-get u "tex-column-width"))
              "6em"
            ) ;input
          ) ;item
        ) ;aligned
        (glue #f #t 0 0)
      ) ;refreshable
      ///
      ///
      (refreshable "page-tex-ver-margins"
        (hlist (bold (text "Vertical margins")))
        ===
        ===
        (aligned
          (item (text "topmargin:")
            (input (initial-set u "tex-top-margin" answer)
              "string"
              (list (initial-get u "tex-top-margin"))
              "6em"
            ) ;input
          ) ;item
          (item (text "headheight:")
            (input (initial-set u "tex-head-height" answer)
              "string"
              (list (initial-get u "tex-head-height"))
              "6em"
            ) ;input
          ) ;item
          (item (text "headsep:")
            (input (initial-set u "tex-head-sep" answer)
              "string"
              (list (initial-get u "tex-head-sep"))
              "6em"
            ) ;input
          ) ;item
          (item (text "textheight:")
            (input (initial-set u "tex-text-height" answer)
              "string"
              (list (initial-get u "tex-text-height"))
              "6em"
            ) ;input
          ) ;item
          (item (text "footskip:")
            (input (initial-set u "tex-foot-skip" answer)
              "string"
              (list (initial-get u "tex-foot-skip"))
              "6em"
            ) ;input
          ) ;item
        ) ;aligned
        (glue #f #t 0 0)
      ) ;refreshable
      ///
      ///
      (refreshable "page-screen-margin-settings"
        (when (== (initial-get u "page-screen-margin") "true")
          (hlist (bold (text "Margins on screen")))
          ===
          ===
          (aligned
            (item (text "Left:")
              (input (initial-set u "page-screen-left" answer)
                "string"
                (list (initial-get u "page-screen-left"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Right:")
              (input (initial-set u "page-screen-right" answer)
                "string"
                (list (initial-get u "page-screen-right"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Top:")
              (input (initial-set u "page-screen-top" answer)
                "string"
                (list (initial-get u "page-screen-top"))
                "6em"
              ) ;input
            ) ;item
            (item (text "Bottom:")
              (input (initial-set u "page-screen-bot" answer)
                "string"
                (list (initial-get u "page-screen-bot"))
                "6em"
              ) ;input
            ) ;item
          ) ;aligned
          (glue #f #t 0 0)
        ) ;when
      ) ;refreshable
    ) ;hlist
  ) ;padded
  ======
  (explicit-buttons (hlist >>>
                     ("Reset"
                       (initial-default u "tex-odd-side-margin"
                         "tex-even-side-margin" "tex-text-width"
                         "tex-line-width" "tex-column-width" "tex-top-margin"
                         "tex-head-height" "tex-head-sep" "tex-text-height"
                         "tex-foot-skip" "page-screen-left" "page-screen-right"
                         "page-screen-top" "page-screen-bot" "page-width-margin"
                         "page-screen-margin"
                       ) ;initial-default
                       (refresh-now "page-margin-toggles")
                       (refresh-now "page-tex-hor-margins")
                       (refresh-now "page-tex-ver-margins")
                       (refresh-now "page-screen-margin-settings")
                     ) ;
                     //
                     //
                     ("Ok" (quit))
                    ) ;hlist
  ) ;explicit-buttons
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Breaking
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (page-formatter-breaking u quit)
  (padded
    (refreshable "page-breaking-settings"
      (aligned (item (text "Page breaking algorithm:")
                 (enum (initial-set u "page-breaking" answer)
                   '("sloppy" "professional")
                   (initial-get u "page-breaking")
                   "10em"
                 ) ;enum
               ) ;item
        (item (text "Allowed page height reduction:")
          (enum (initial-set u "page-shrink" answer)
            (cons-new (initial-get u "page-shrink") '("0cm" "0.5cm" "1cm" ""))
            (initial-get u "page-shrink")
            "10em"
          ) ;enum
        ) ;item
        (item (text "Allowed page height extension:")
          (enum (initial-set u "page-extend" answer)
            (cons-new (initial-get u "page-extend") '("0cm" "0.5cm" "1cm" ""))
            (initial-get u "page-extend")
            "10em"
          ) ;enum
        ) ;item
        (item (text "Vertical space stretchability:")
          (enum (initial-set u "page-flexibility" answer)
            (cons-new (initial-get u "page-flexibility") '("0" "0.25" "0.5"
                                                           "0.75" "1" ""))
            (initial-get u "page-flexibility")
            "10em"
          ) ;enum
        ) ;item
      ) ;aligned
    ) ;refreshable
  ) ;padded
  ===
  ===
  (explicit-buttons (hlist >>>
                     ("Reset"
                       (initial-default u "page-breaking" "page-shrink"
                         "page-extend" "page-flexibility"
                       ) ;initial-default
                       (refresh-now "page-breaking-settings")
                     ) ;
                     //
                     //
                     ("Ok" (quit))
                    ) ;hlist
  ) ;explicit-buttons
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Headers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define header-parameters
  (list "page-odd-header" "page-even-header" "page-odd-footer" "page-even-footer")
) ;define

(define (header-buffer var)
  (string->url
    (string-append "tmfs://aux/"
      var
      "/"
      (url->string (url-tail (get-auxiliary-widget-parent-url)))
    ) ;string-append
  ) ;string->url
) ;define

(define (header-buffers)
  (map header-buffer header-parameters)
) ;define

(define (get-field-contents u)
  (and-with t
    (tm->stree (buffer-get-body u))
    (when (tm-func? t 'document 1)
      (set! t (tm-ref t 0))
    ) ;when
    t
  ) ;and-with
) ;define

(define (apply-headers-settings u)
  (with l
    (list)
    (for (var header-parameters)
      (and-with doc
        (get-field-contents (header-buffer var))
        (set! l (cons `(,var ,doc) l))
      ) ;and-with
    ) ;for
    (when (nnull? l)
      (delayed (:idle 10)
        (for (x l) (initial-set-tree u (car x) (cadr x)))
        (refresh-window)
      ) ;delayed
    ) ;when
  ) ;with
) ;define

(define (editing-headers?)
  (in? (current-buffer) (map header-buffer header-parameters))
) ;define

(tm-widget (page-formatter-headers u style quit)
  (padded
    (refreshable "page-header-settings"
      (for (var header-parameters)
        (bold (text (eval (parameter-name var))))
        ===
        (resize "480px"
          "100px"
          (texmacs-input
            `(document ,(initial-get-tree u var))
            `(style (tuple ,@style ,"gui-base"))
            (header-buffer var)
          ) ;texmacs-input
        ) ;resize
        ===
      ) ;for
    ) ;refreshable
  ) ;padded
  ===
  ===
  (explicit-buttons
    (hlist //
      //
      //
      //
      //
      (text "Insert:")
      //
      //
      ("Tab" (when (editing-headers?) (make-htab "5mm")))
      //
      //
      ("Page number" (when (editing-headers?) (make 'page-the-page)))
      //
      //
      ("Total pages" (when (editing-headers?) (make 'page-the-total)))
      //
      //
      >>>
      //
      //
      ;; ("Reset"
      ;; (initial-default u header-parameters)
      ;; (refresh-now "page-header-settings"))
      ;; // //
      ("Ok" (apply-headers-settings u) (begin (quit) (buffer-focus u #t)))
      //
      //
      //
      //
      //
    ) ;hlist
  ) ;explicit-buttons
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Advanced Headers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (make-header-condition start end parity content)
  (let* ((page-nr (list 'value "page-nr"))
         (range-test
           `(not (or (less ,page-nr ,start) (greater ,page-nr ,end)))
         ) ;range-test
        ) ;
    `(if ,range-test ,content ,"")
  ) ;let*
) ;define

(define (assign-advanced-header u start end parity content)
  (let* ((content-tree (if content content ""))
         (new-if (make-header-condition start end parity content-tree))
         (new-cond (cadr new-if))
         (new-then (caddr new-if))
        ) ;

    ;; 将条件树解析为 (cond content) 对的列表
    (define (tree->pairs stree)
      (cond
       ((not (tm-func? stree 'if 3)) (if (== stree "") '() (list (cons #t stree))))
       (else (cons (cons (cadr stree) (caddr stree)) (tree->pairs (cadddr stree))))
      ) ;cond
    ) ;define

    ;; 将 (cond content) 对列表转换回条件树
    (define (pairs->tree pairs)
      (if (null? pairs)
        ""
        (let ((pair (car pairs)))
          (if (eq? (car pair) #t)
            (cdr pair)
            ;; 构建条件链，递归处理剩余对
            (let ((rest (cdr pairs)))
              `(if ,(car pair) ,(cdr pair) ,(pairs->tree rest))
            ) ;let
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    ;; 合并新条件到对列表中
    (define (merge-pairs pairs cond then)
      ;; 查找是否有相同条件并构建新列表
      (define (process-pairs pairs found new-pairs)
        (if (null? pairs)
          (if found (reverse new-pairs) (cons (cons cond then) (reverse new-pairs)))
          (let ((pair (car pairs)))
            (if
              (and (not (eq? (car pair) #t)) (tm-equal? (car pair) cond))
              (process-pairs (cdr pairs) #t (cons (cons cond then) new-pairs))
              (process-pairs (cdr pairs) found (cons pair new-pairs))
            ) ;if
          ) ;let
        ) ;if
      ) ;define
      (process-pairs pairs #f '())
    ) ;define

    ;; 主合并函数
    (define (merge-condition-tree stree)
      (let* ((pairs (tree->pairs stree))
             (merged-pairs (merge-pairs pairs new-cond new-then))
            ) ;
        (pairs->tree merged-pairs)
      ) ;let*
    ) ;define

    (cond
     ((or (== parity "odd") (== parity "odd page"))
      (let ((old-tree (initial-get-tree u "page-odd-header")))
        (initial-set-tree u
          "page-odd-header"
          (merge-condition-tree (tm->stree old-tree))
        ) ;initial-set-tree
      ) ;let
     ) ;
     ((or (== parity "even") (== parity "even page"))
      (let ((old-tree (initial-get-tree u "page-even-header")))
        (initial-set-tree u
          "page-even-header"
          (merge-condition-tree (tm->stree old-tree))
        ) ;initial-set-tree
      ) ;let
     ) ;
     (else
       (let ((old-odd (initial-get-tree u "page-odd-header"))
             (old-even (initial-get-tree u "page-even-header"))
            ) ;
         (initial-set-tree u
           "page-odd-header"
           (merge-condition-tree (tm->stree old-odd))
         ) ;initial-set-tree
         (initial-set-tree u
           "page-even-header"
           (merge-condition-tree (tm->stree old-even))
         ) ;initial-set-tree
       ) ;let
     ) ;else
    ) ;cond
  ) ;let*
  (refresh-window)
) ;define

(tm-widget (page-formatter-advanced-header u style quit)
  (let* ((current-tree (initial-get-tree u "page-odd-header"))
         (content-tree (if (tm-func? current-tree 'if 3) (tm-ref current-tree 2) current-tree)
         ) ;content-tree
         (start "1")
         (end (number->string (get-page-count)))
         (parity "any")
         (content "")
        ) ;
    (centered
      (refreshable "advanced-header-settings"
        (aligned (item (text "Applying from:")
                   (input (set! start answer) "string" (list start) "6em")
                 ) ;item
          (item (text "Applying to:") (input (set! end answer) "string" (list end) "6em"))
          (item (text "Parity:")
            (enum (begin (set! parity answer)) '("odd page" "even page" "any") "any" "10em")
          ) ;item
          (item (text "Content:")
            (resize "480px"
              "100px"
              (texmacs-input `(document ,content-tree)
                `(style (tuple ,@style ,"gui-base"))
                (string->url "tmfs://aux/advanced-header")
              ) ;texmacs-input
            ) ;resize
          ) ;item
        ) ;aligned
      ) ;refreshable
    ) ;centered
    ===
    (explicit-buttons
      (hlist >>>
       ("Ok"
         (with content
           (get-field-contents (string->url "tmfs://aux/advanced-header"))
           (assign-advanced-header u start end parity content)
         ) ;with
         (quit)
       ) ;
       //
       //
       ("Cancel" (quit))
      ) ;hlist
    ) ;explicit-buttons
  ) ;let*
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page / Advanced Footers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(define (make-footer-condition start end parity content)
  (let* ((page-nr (list 'value "page-nr"))
         (range-test
           `(not (or (less ,page-nr ,start) (greater ,page-nr ,end)))
         ) ;range-test
        ) ;
    `(if ,range-test ,content ,"")
  ) ;let*
) ;define

(define (assign-advanced-footer u start end parity content)
  (let* ((content-tree (if content content ""))
         (new-if (make-footer-condition start end parity content-tree))
         (new-cond (cadr new-if))
         (new-then (caddr new-if))
        ) ;

    ;; 将条件树解析为 (cond content) 对的列表
    (define (tree->pairs stree)
      (cond
       ((not (tm-func? stree 'if 3)) (if (== stree "") '() (list (cons #t stree))))
       (else (cons (cons (cadr stree) (caddr stree)) (tree->pairs (cadddr stree))))
      ) ;cond
    ) ;define

    ;; 将 (cond content) 对列表转换回条件树
    (define (pairs->tree pairs)
      (if (null? pairs)
        ""
        (let ((pair (car pairs)))
          (if (eq? (car pair) #t)
            (cdr pair)
            ;; 构建条件链，递归处理剩余对
            (let ((rest (cdr pairs)))
              `(if ,(car pair) ,(cdr pair) ,(pairs->tree rest))
            ) ;let
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    ;; 合并新条件到对列表中
    (define (merge-pairs pairs cond then)
      ;; 查找是否有相同条件并构建新列表
      (define (process-pairs pairs found new-pairs)
        (if (null? pairs)
          (if found (reverse new-pairs) (cons (cons cond then) (reverse new-pairs)))
          (let ((pair (car pairs)))
            (if
              (and (not (eq? (car pair) #t)) (tm-equal? (car pair) cond))
              (process-pairs (cdr pairs) #t (cons (cons cond then) new-pairs))
              (process-pairs (cdr pairs) found (cons pair new-pairs))
            ) ;if
          ) ;let
        ) ;if
      ) ;define
      (process-pairs pairs #f '())
    ) ;define

    ;; 主合并函数
    (define (merge-condition-tree stree)
      (let* ((pairs (tree->pairs stree))
             (merged-pairs (merge-pairs pairs new-cond new-then))
            ) ;
        (pairs->tree merged-pairs)
      ) ;let*
    ) ;define

    (cond
     ((or (== parity "odd") (== parity "odd page"))
      (let ((old-tree (initial-get-tree u "page-odd-footer")))
        (initial-set-tree u
          "page-odd-footer"
          (merge-condition-tree (tm->stree old-tree))
        ) ;initial-set-tree
      ) ;let
     ) ;
     ((or (== parity "even") (== parity "even page"))
      (let ((old-tree (initial-get-tree u "page-even-footer")))
        (initial-set-tree u
          "page-even-footer"
          (merge-condition-tree (tm->stree old-tree))
        ) ;initial-set-tree
      ) ;let
     ) ;
     (else
       (let ((old-odd (initial-get-tree u "page-odd-footer"))
             (old-even (initial-get-tree u "page-even-footer"))
            ) ;
         (initial-set-tree u
           "page-odd-footer"
           (merge-condition-tree (tm->stree old-odd))
         ) ;initial-set-tree
         (initial-set-tree u
           "page-even-footer"
           (merge-condition-tree (tm->stree old-even))
         ) ;initial-set-tree
       ) ;let
     ) ;else
    ) ;cond
  ) ;let*
  (refresh-window)
) ;define

(tm-widget (page-formatter-advanced-footer u style quit)
  (let* ((current-tree (initial-get-tree u "page-odd-footer"))
         (content-tree (if (tm-func? current-tree 'if 3) (tm-ref current-tree 2) current-tree)
         ) ;content-tree
         (start "1")
         (end (number->string (get-page-count)))
         (parity "any")
         (content "")
        ) ;
    (centered
      (refreshable "advanced-footer-settings"
        (aligned (item (text "Applying from:")
                   (input (set! start answer) "string" (list start) "6em")
                 ) ;item
          (item (text "Applying to:") (input (set! end answer) "string" (list end) "6em"))
          (item (text "Parity:")
            (enum (begin (set! parity answer)) '("odd page" "even page" "any") "any" "10em")
          ) ;item
          (item (text "Content:")
            (resize "480px"
              "100px"
              (texmacs-input `(document ,content-tree)
                `(style (tuple ,@style ,"gui-base"))
                (string->url "tmfs://aux/advanced-footer")
              ) ;texmacs-input
            ) ;resize
          ) ;item
        ) ;aligned
      ) ;refreshable
    ) ;centered
    ===
    (explicit-buttons
      (hlist >>>
       ("Ok"
         (with content
           (get-field-contents (string->url "tmfs://aux/advanced-footer"))
           (assign-advanced-footer u start end parity content)
         ) ;with
         (quit)
       ) ;
       //
       //
       ("Cancel" (quit))
      ) ;hlist
    ) ;explicit-buttons
  ) ;let*
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Page
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((document-page-formatter u style) quit)
  (padded
    (tabs
      (tab (text "Format") (padded (dynamic (page-formatter-format u quit))))
      (tab (text "Margins") (padded (dynamic (page-formatter-margins u quit))))
      (tab (text "Breaking") (padded (dynamic (page-formatter-breaking u quit))))
      (tab (text "Advanced header")
        (padded (dynamic (page-formatter-advanced-header u style quit)))
      ) ;tab
      (tab (text "Advanced footer")
        (padded (dynamic (page-formatter-advanced-footer u style quit)))
      ) ;tab
    ) ;tabs
  ) ;padded
) ;tm-widget

(tm-define (open-document-page-format-window)
  (:interactive #t)
  (let* ((u (current-buffer)) (st (embedded-style-list "macro-editor")))
    (apply dialogue-window
      (cons* (document-page-formatter u st)
        noop
        "Document page format"
        (header-buffers)
      ) ;cons*
    ) ;apply
  ) ;let*
) ;tm-define

(tm-define (open-document-page-format)
  (:interactive #t)
  (if (side-tools?)
    (tool-select :right 'document-page-tool)
    (open-document-page-format-window)
  ) ;if
) ;tm-define

(tm-define (open-page-headers-footers-window)
  (:interactive #t)
  (let* ((u (current-buffer)) (st (embedded-style-list "macro-editor")))
    (apply auxiliary-widget
      (cons* (lambda (quit) (page-formatter-headers u st quit))
        noop
        (translate "Headers and footers")
        (header-buffers)
      ) ;cons*
    ) ;apply
    (set-page-headers-footers-window-state #t)
    (buffer-focus (header-buffer "page-odd-header") #t)
  ) ;let*
) ;tm-define

(tm-define (open-page-headers-footers)
  (:interactive #t)
  (open-page-headers-footers-window)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Metadata
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget ((document-metadata-editor u) quit)
  (padded
    (refreshable "document-metadata"
      (aligned
        (item (text "Title:")
          (input (initial-set u "global-title" answer)
            "string"
            (list (buffer-get-metadata u "title"))
            "24em"
          ) ;input
        ) ;item
        (item (text "Author:")
          (input (initial-set u "global-author" answer)
            "string"
            (list (buffer-get-metadata u "author"))
            "24em"
          ) ;input
        ) ;item
        (item (text "Subject:")
          (input (initial-set u "global-subject" answer)
            "string"
            (list (buffer-get-metadata u "subject"))
            "24em"
          ) ;input
        ) ;item
      ) ;aligned
    ) ;refreshable
    ======
    (explicit-buttons (hlist >>>
                       ("Reset"
                         (initial-default u "global-title" "global-author" "global-subject")
                         (refresh-now "document-metadata")
                       ) ;
                       //
                       //
                       ("Ok" (quit))
                      ) ;hlist
    ) ;explicit-buttons
  ) ;padded
) ;tm-widget

(tm-define (open-document-metadata-window)
  (:interactive #t)
  (let* ((u (current-buffer)))
    (dialogue-window (document-metadata-editor u) noop "Document metadata")
  ) ;let*
) ;tm-define

(tm-define (open-document-metadata)
  (:interactive #t)
  (if (side-tools?)
    (tool-select :right 'document-metadata-tool)
    (open-document-metadata-window)
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document -> Color
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (page-colors-background u)
  (pick-background "" (initial-set-tree u "bg-color" answer))
) ;tm-widget

(tm-widget (page-colors-foreground u)
  (pick-color (initial-set-tree u "color" answer))
) ;tm-widget

(tm-widget ((document-colors-picker u) quit)
  (padded
    (refreshable "page-colors"
      (tabs
        (tab (text "Background") (padded (dynamic (page-colors-background u))))
        (tab (text "Foreground") (padded (dynamic (page-colors-foreground u))))
      ) ;tabs
    ) ;refreshable
    ======
    (explicit-buttons (hlist >>>
                       ("Reset" (initial-default u "bg-color" "color") (refresh-now "page-colors"))
                       //
                       //
                       ("Ok" (quit))
                      ) ;hlist
    ) ;explicit-buttons
  ) ;padded
) ;tm-widget

(tm-define (open-document-colors-window)
  (:interactive #t)
  (with u
    (current-buffer)
    (dialogue-window (document-colors-picker u) noop "Document colors")
  ) ;with
) ;tm-define

(tm-define (open-document-colors)
  (:interactive #t)
  (if (side-tools?)
    (tool-select :right 'document-colors-tool)
    (open-document-colors-window)
  ) ;if
) ;tm-define

(register-auxiliary-widget-type 'page-headers-footers
  (list open-page-headers-footers-window)
) ;register-auxiliary-widget-type
