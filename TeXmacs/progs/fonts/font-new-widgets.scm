
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : font-selector.scm
;; DESCRIPTION : Widget for font selection
;; COPYRIGHT   : (C) 2012  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; See menu-define.scm for the grammar of menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (fonts font-new-widgets)
  (:use (kernel gui menu-widget)
    (kernel texmacs pref-keys)
    (fonts font-sample)
    (generic format-edit)
    (generic document-edit)
    (utils library cursor)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Settings management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define selector-table (make-ahash-table))

;; key -> (family style size)：对话框打开瞬间的文档字体快照，供 Cancel/重置回放。
;; 必须在 register-specs（live 改动前）快照——文档字体的 live（init-multi）会改 init
;; 块，事后 get-init 读到的是改后的值，无法再取回原值。

(define initial-snapshot (make-ahash-table))

(define (selkey specs var)
  (with win
    (if (list-4? specs) (cadddr specs) (current-window))
    (list specs var (window->buffer win))
  ) ;with
) ;define

(tm-define (selector-set* specs var val)
  ;; (display* "Set " specs ", " var " <- " val "\n")
  (ahash-set! selector-table (selkey specs var) val)
  (refresh-now "font-style-selector")
  (refresh-now "font-selector-demo")
) ;tm-define

(tm-define (selector-notify specs)
  (with (getter setter . other)
    specs
    (with changes
      (selector-get-changes specs getter)
      (when (nnull? changes)
        (setter changes)
        ;; (with-window win (update-menus))
        (keyboard-focus-on "canvas")
      ) ;when
    ) ;with
  ) ;with
) ;tm-define

(tm-define (selector-set specs var val)
  ;; (display* "Set " specs ", " var " <- " val "\n")
  (when (!= val (selector-get specs var))
    (ahash-set! selector-table (selkey specs var) val)
    (selector-notify specs)
    (refresh-now "font-style-selector")
    (refresh-now "font-selector-demo")
  ) ;when
) ;tm-define

(tm-define (selector-reset* specs var)
  ;; (display* "Reset " specs ", " var "\n")
  (ahash-remove! selector-table (selkey specs var))
  (refresh-now "font-style-selector")
  (refresh-now "font-selector-demo")
) ;tm-define

(tm-define (selector-reset specs var)
  ;; (display* "Reset " specs ", " var "\n")
  (ahash-remove! selector-table (selkey specs var))
  (selector-notify specs)
  (refresh-now "font-style-selector")
  (refresh-now "font-selector-demo")
) ;tm-define

(define font-vars (list :family :style :size))

(define filter-vars
  (list :weight :slant :stretch :serif :spacing :case :device :category :glyphs)
) ;define

(define customize-vars
  (list "bold"
    "italic"
    "smallcaps"
    "sansserif"
    "typewriter"
    "math"
    "greek"
    "bbb"
    "cal"
    "frak"
    "embold"
    "embbb"
    "slant"
    "hmagnify"
    "vmagnify"
    "hextended"
    "vextended"
  ) ;list
) ;define

(define all-vars (append font-vars filter-vars customize-vars))

(tm-define (selector-get* specs var)
  ;; (display* "Get " specs ", " var "\n")
  (or (ahash-ref selector-table (selkey specs var))
    (cond ((== var :family) (car (initial-font-data specs)))
          ((== var :style) (cadr (initial-font-data specs)))
          ((== var :size) (caddr (initial-font-data specs)))
          ((in? var filter-vars) "Any")
          ((in? var customize-vars) (initial-customize-get specs var))
          (else #f)
    ) ;cond
  ) ;or
) ;tm-define

(tm-define (selector-get specs var)
  (with r
    (selector-get* specs var)
    ;; (display* "Get " specs ", " var " -> " r "\n")
    r
  ) ;with
) ;tm-define

(tm-define (selector-clean specs)
  (with (getter setter . other)
    specs
    (for (var all-vars) (ahash-remove! selector-table (selkey specs var)))
  ) ;with
) ;tm-define

(tm-define (selector-restore specs global?)
  (when global?
    ;; NOTE: non global => ':default' values not yet implemented
    (with vars
      (list (pref-font)
        (pref-font-base-size)
        (pref-math-font)
        (pref-prog-font)
        (pref-font-family)
        (pref-font-series)
        (pref-font-shape)
        (pref-font-effects)
      ) ;list
      (with (getter setter . other)
        specs
        (for (var all-vars) (ahash-remove! selector-table (selkey specs var)))
        (for (var vars) (setter (list var :default)))
      ) ;with
      (keyboard-focus-on "canvas")
      (delayed (:pause 250)
        (refresh-now "font-family-selector")
        (refresh-now "font-style-selector")
        (refresh-now "font-size-selector")
        (refresh-now "font-customized-selector")
        (refresh-now "font-selector-demo")
      ) ;delayed
    ) ;with
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Font samples
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (standard-selector-text)
  `(with ,"par-par-sep"
     ,"0.2em"
     (document ,"Lowercase: abcdefghijklmnopqrstuvwxyz"
       ,"Uppercase: ABCDEFGHIJKLMNOPQRSTUVWXYZ"
       ,"Numbers: 0123456789 +-*/^=<less><gtr>"
       ,"Special: ([{|}]) \"`,.:;!?'\" @#$%&_\\~"
       ,(string-append "Accented: <#E0><#E1><#E4><#E2><#E3><#E8><#E9><#EB><#EA><#EC><#ED><#EF>"
          "<#EE><#F2><#F3><#F6><#F4><#F5><#F9><#FA><#FC><#FB>")
       ,(string-append "Greek: <alpha><beta><gamma><delta><varepsilon><zeta><eta><theta>"
          "<iota><kappa><lambda><mu><nu><xi><omicron><pi>"
          "<rho><sigma><tau><upsilon><varphi><psi><chi><omega>")
       ,(string-append "Cyrillic: <#430><#431><#432><#433><#434><#435><#436><#437>"
          "<#438><#439><#43A><#43B><#43C><#43D><#43E><#43F>"
          "<#440><#441><#442><#443><#444><#445><#446><#447>"
          "<#448><#449><#44A><#44B><#44C><#44D><#44E><#44F>")
       ,(string-append "Mathematics: <leq><geq><leqslant><geqslant><prec><succ> "
          "<leftarrow><rightarrow><Leftarrow><Rightarrow><mapsto> "
          "<times><cdot><oplus><otimes>")
       (concat "Variants: "
         (strong "Bold")
         "  "
         (em "Italic")
         "  "
         (name "Small Capitals")
         "  "
         (samp "Sans Serif")
         "  "
         (kbd "Typewriter"))))
) ;define

(define (math-selector-text)
  `(with ,"par-par-sep"
     ,"0.2em"
     (document (concat "Lowercase Roman: "
                 (math "a b c d e f g h i j k l m n o p q r s t u v w x y z"))
       (concat "Uppercase Roman: "
         (math "A B C D E F G H I J K L M N O P Q R S T U V W X Y Z"))
       (concat ,"Lowercase Greek: "
         (math ,(string-append "<alpha> <beta> <gamma> <delta> <varepsilon> <zeta> <eta>"
                  " <theta> <iota> <kappa> <lambda> <mu> <nu> <xi> <omicron>"
                  " <pi> <rho> <sigma> <tau> <upsilon> <varphi> <psi>"
                  " <chi> <omega>")))
       (concat ,"Uppercase Greek: "
         (math ,(string-append "<Alpha> <Beta> <Gamma> <Delta> <Epsilon> <Zeta> <Eta>"
                  " <Theta> <Iota> <Kappa> <Lambda> <Mu> <Nu> <Xi> <Omicron>"
                  " <Pi> <Rho> <Sigma> <Tau> <Upsilon> <Phi> <Psi>"
                  " <Chi> <Omega>")))
       (concat ,"Blackboard bold: "
         (math ,(string-append "<bbb-A> <bbb-B> <bbb-C> <bbb-D> <bbb-E> <bbb-F> <bbb-G>"
                  " <bbb-H> <bbb-I> <bbb-J> <bbb-K> <bbb-L> <bbb-M> <bbb-N>"
                  " <bbb-O> <bbb-P> <bbb-Q> <bbb-R> <bbb-S> <bbb-T> <bbb-U>"
                  " <bbb-V> <bbb-W> <bbb-X> <bbb-Y> <bbb-Z>")))
       (concat ,"Calligraphic: "
         (math ,(string-append "<cal-A> <cal-B> <cal-C> <cal-D> <cal-E> <cal-F> <cal-G>"
                  " <cal-H> <cal-I> <cal-J> <cal-K> <cal-L> <cal-M> <cal-N>"
                  " <cal-O> <cal-P> <cal-Q> <cal-R> <cal-S> <cal-T> <cal-U>"
                  " <cal-V> <cal-W> <cal-X> <cal-Y> <cal-Z>")))
       (concat ,"Fraktur: "
         (math ,(string-append "<frak-A> <frak-B> <frak-C> <frak-D> <frak-E> <frak-F>"
                  " <frak-G> <frak-H> <frak-I> <frak-J> <frak-K> <frak-L>"
                  " <frak-M> <frak-N> <frak-O> <frak-P> <frak-Q> <frak-R>"
                  " <frak-S> <frak-T> <frak-U> <frak-V> <frak-W> <frak-X>"
                  " <frak-Y> <frak-Z>")))))
) ;define

(define-public sample-text (standard-selector-text))
(define-public sample-kind "Standard")

(tm-define (set-font-sample-range hexa-start hexa-end)
  (:argument hexa-start "First unicode character in hexadecimal")
  (:argument hexa-end "Last unicode character in hexadecimal")
  (set! sample-text
    (build-character-table (hexadecimal->integer hexa-start)
      (hexadecimal->integer hexa-end)
    ) ;build-character-table
  ) ;set!
) ;tm-define

(define (set-font-sample-kind kind)
  (set! sample-kind kind)
  (cond ((== kind "Mathematics") (set! sample-text (math-selector-text)))
        ((== kind "ASCII") (set-font-sample-range "20" "7f"))
        ((== kind "Latin") (set-font-sample-range "80" "ff"))
        ((== kind "Greek") (set-font-sample-range "380" "3ff"))
        ((== kind "Cyrillic") (set-font-sample-range "400" "4ff"))
        ((== kind "CJK") (set-font-sample-range "4e00" "9fcc"))
        ((== kind "Hangul") (set-font-sample-range "ac00" "d7af"))
        ((== kind "Math Symbols") (set-font-sample-range "2000" "23ff"))
        ((== kind "Math Extra") (set-font-sample-range "2900" "2e7f"))
        ((== kind "Math Letters") (set-font-sample-range "1d400" "1d7ff"))
        ((== kind "Unicode 0000-0fff") (set-font-sample-range "0000" "0fff"))
        ((== kind "Unicode 1000-1fff") (set-font-sample-range "1000" "1fff"))
        ((== kind "Unicode 2000-2fff") (set-font-sample-range "2000" "2fff"))
        ((== kind "Unicode 3000-3fff") (set-font-sample-range "3000" "3fff"))
        ((== kind "Unicode 4000-4fff") (set-font-sample-range "4000" "4fff"))
        ((and (== kind "Selection") (selection-active-any?))
         (set! sample-text (tree->stree (selection-tree)))
        ) ;
        (else (set! sample-text (standard-selector-text)))
  ) ;cond
) ;define

(define (get-font-sample-kind)
  sample-kind
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Global state of font selector
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (initial-font-data specs)
  (let* ((getter (car specs))
         (fam (font-family-main (getter (pref-font))))
         (var (getter (pref-font-family)))
         (ser (getter (pref-font-series)))
         (sh (getter (pref-font-shape)))
         (sz (getter (pref-font-base-size)))
         (lf (logical-font-private fam var ser sh))
         ;; 用非精确 logical-font-search（而非 -exact）：exact 在 DB 无距离 0 命中时
         ;; 会合成编码 style 名（如 simsun），不在真实 style 列表里，导致选择器匹配
         ;; 不上不高亮。非精确搜索返回真实 DB 名，与 selected-styles 同源。
         (fn (logical-font-search lf))
        ) ;
    (list (or (car fn) "TeXmacs Computer Modern")
      (or (cadr fn) "Regular")
      (or sz "10")
    ) ;list
  ) ;let*
) ;define

;; 文档级字体（family/style/size），用 get-init 读。供 register-specs 在打开瞬间快照
;; （initial-snapshot）——事后 get-init 会被 live 写回污染（init-multi 改 init、
;; make-multi-with 插 with 块），故必须在改动前快照。

(define (font-document-default-data)
  (let* ((fam (font-family-main (or (get-init (pref-font)) "roman")))
         (var (or (get-init (pref-font-family)) "rm"))
         (ser (or (get-init (pref-font-series)) "medium"))
         (sh (or (get-init (pref-font-shape)) "right"))
         (sz (or (get-init (pref-font-base-size)) "10"))
         (lf (logical-font-private fam var ser sh))
         (fn (logical-font-search lf))
        ) ;
    (list (or (car fn) "TeXmacs Computer Modern") (or (cadr fn) "Regular") sz)
  ) ;let*
) ;define

(tm-define (selector-get-font specs)
  (logical-font-patch (logical-font-public (selector-get specs :family) (selector-get specs :style))
    (selected-properties specs)
  ) ;logical-font-patch
) ;tm-define

(tm-define (selector-get-changes specs getter)
  (if (== (selector-get specs :style) "Unknown")
    (list)
    (with fn
      (selector-get-font specs)
      (with l
        '()
        (when (!= (selector-font-effects specs) (getter (pref-font-effects)))
          (set! l (cons* (pref-font-effects) (selector-font-effects specs) l))
        ) ;when
        (when (!= (selector-get specs :size) (getter (pref-font-base-size)))
          (set! l (cons* (pref-font-base-size) (selector-get specs :size) l))
        ) ;when
        (when (!= (logical-font-shape fn) (getter (pref-font-shape)))
          (set! l (cons* (pref-font-shape) (logical-font-shape fn) l))
        ) ;when
        (when (!= (logical-font-series fn) (getter (pref-font-series)))
          (set! l (cons* (pref-font-series) (logical-font-series fn) l))
        ) ;when
        (when (!= (logical-font-variant fn) (getter (pref-font-family)))
          (set! l (cons* (pref-font-family) (logical-font-variant fn) l))
        ) ;when
        (when (!= (logical-font-family* specs fn) (getter (pref-font)))
          (set! l (cons* (pref-font) (logical-font-family* specs fn) l))
        ) ;when
        l
      ) ;with
    ) ;with
  ) ;if
) ;tm-define

(define (selector-font-simulate-data specs)
  (let* ((fn (selector-get-font specs))
         (fam (logical-font-family fn))
         (var (logical-font-variant fn))
         (ser (logical-font-series fn))
         (sh (logical-font-shape fn))
         (lf (logical-font-private fam var ser sh))
         (fn2 (logical-font-search lf))
         (fn1 (list (selector-get specs :family) (selector-get specs :style)))
         (sel (string-recompose (selected-properties specs) " "))
         (nm1 (string-append (car fn1) " " (cadr fn1)))
         (nm2 (string-append (car fn2) " " (cadr fn2)))
         (nm+ (if (== sel "") nm1 (string-append nm1 " + " sel)))
        ) ;
    ;; (display* "fn = " fn "\n")
    ;; (display* "lf = " lf "\n")
    ;; (display* "fn2= " fn2 "\n")
    (list nm1 nm+ nm2)
  ) ;let*
) ;define

(tm-widget (selector-font-simulate-widget specs)
  (with (fn1 fn1+ fn2)
    (selector-font-simulate-data specs)
    (assuming (or (!= fn1 fn1+) (!= fn1 fn2))
      (division "discrete"
        (aligned (item (bold (text "Requested:")) (text fn1+))
          (item (bold (text "Replaced by:")) (text fn2))
        ) ;aligned
      ) ;division
    ) ;assuming
  ) ;with
) ;tm-widget

(define (selector-font-demo-text specs)
  (with fn
    (selector-get-font specs)
    ;; (display* "Font: " fn "\n")
    ;; (display* "Internal font: " (logical-font-family* specs fn)
    ;;          ", " (logical-font-variant fn)
    ;;          ", " (logical-font-series fn)
    ;;          ", " (logical-font-shape fn)
    ;;          ", " (selector-get specs :size) "\n")
    `(document (with ,"font"
                 ,(logical-font-family* specs fn)
                 ,"font-family"
                 ,(logical-font-variant fn)
                 ,"font-series"
                 ,(logical-font-series fn)
                 ,"font-shape"
                 ,(logical-font-shape fn)
                 ,"font-base-size"
                 ,(selector-get specs :size)
                 ,"font-effects"
                 ,(selector-font-effects specs)
                 ,sample-text))
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Global state for font searching
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (selector-search-glyphs-decoded specs)
  (with s
    (selector-get specs :glyphs)
    (cond ((== s "ASCII") "Ascii")
          ((== s "Math Symbols") "MathSymbols")
          ((== s "Math Extra") "MathExtra")
          ((== s "Math Letters") "MathLetters")
          (else s)
    ) ;cond
  ) ;with
) ;define

(define (selected-properties specs)
  (with l
    (list (selector-get specs :weight)
      (selector-get specs :slant)
      (selector-get specs :stretch)
      (selector-get specs :serif)
      (selector-get specs :spacing)
      (selector-get specs :case)
      (selector-get specs :device)
      (selector-get specs :category)
      (selector-search-glyphs-decoded specs)
    ) ;list
    (list-filter l (cut != <> "Any"))
  ) ;with
) ;define

(tm-define-macro (selector-set* specs var val)
  `(begin
     (selector-set ,specs ,var ,val)
     (delayed (refresh-now "font-family-selector")))
) ;tm-define-macro

(tm-define (selected-families specs)
  (search-font-families (selected-properties specs))
) ;tm-define

(tm-define (selected-styles specs family)
  (search-font-styles family (selected-properties specs))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Global state for font customization
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (selector-customize?) #f)

(tm-define (selector-customize! on?) (refresh-now "font-customized-selector"))

(tm-define (selector-customize-get specs which default)
  (or (selector-get specs which) default)
) ;tm-define

(tm-define (selector-customize-set! specs which val)
  (if (or (== val "")
        (== val "default")
        (== val "Default")
        (== val (font-effect-default which))
      ) ;or
    (selector-reset specs which)
    (selector-set specs which val)
  ) ;if
) ;tm-define

(tm-define (selector-customize-get* specs which default)
  (with val
    (selector-customize-get specs which default)
    (cond ((== val "roman") "TeXmacs Computer Modern")
          ((== val "bonum") "TeX Gyre Bonum")
          ((== val "pagella") "TeX Gyre Pagella")
          ((== val "schola") "TeX Gyre Schola")
          ((== val "termes") "TeX Gyre Termes")
          (else val)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (selector-customize-set!* specs which val)
  (cond ((== val "TeXmacs Computer Modern") (set! val "roman"))
        ((== val "TeX Gyre Bonum") (set! val "bonum"))
        ((== val "TeX Gyre Pagella") (set! val "pagella"))
        ((== val "TeX Gyre Schola") (set! val "schola"))
        ((== val "TeX Gyre Termes") (set! val "termes"))
  ) ;cond
  (selector-customize-set! specs which val)
) ;tm-define

(define (initial-customize-get specs var)
  (let* ((getter (car specs))
         (fam (getter "font"))
         (effs (getter "font-effects"))
         (rval #f)
        ) ;
    (for (kv (string-tokenize-by-char fam #\,))
      (with l
        (string-tokenize-by-char kv #\=)
        (when (== (length l) 2)
          (with (var2 val) l (when (== var var2) (set! rval val)))
        ) ;when
      ) ;with
    ) ;for
    (for (kv (string-tokenize-by-char effs #\,))
      (with l
        (string-tokenize-by-char kv #\=)
        (when (== (length l) 2)
          (with (var2 val)
            l
            (cond ((== var2 "bold") (if (== var "embold") (set! rval val)))
                  ((== var2 "bbb") (if (== var "embbb") (set! rval val)))
                  ((== var2 var) (set! rval val))
            ) ;cond
          ) ;with
        ) ;when
      ) ;with
    ) ;for
    rval
  ) ;let*
) ;define

(define (logical-font-family* specs fn)
  (let* ((fam (logical-font-family fn))
         (bf (selector-customize-get specs "bold" #f))
         (it (selector-customize-get specs "italic" #f))
         (sc (selector-customize-get specs "smallcaps" #f))
         (ss (selector-customize-get specs "sansserif" #f))
         (tt (selector-customize-get specs "typewriter" #f))
         (math (selector-customize-get specs "math" #f))
         (greek (selector-customize-get specs "greek" #f))
         (bbb (selector-customize-get specs "bbb" #f))
         (cal (selector-customize-get specs "cal" #f))
         (frak (selector-customize-get specs "frak" #f))
        ) ;
    (if bf (set! fam (string-append "bold=" bf "," fam)))
    (if it (set! fam (string-append "italic=" it "," fam)))
    (if sc (set! fam (string-append "smallcaps=" sc "," fam)))
    (if ss (set! fam (string-append "sansserif=" ss "," fam)))
    (if tt (set! fam (string-append "typewriter=" tt "," fam)))
    (if math (set! fam (string-append "math=" math "," fam)))
    (if greek (set! fam (string-append "greek=" greek "," fam)))
    (if bbb (set! fam (string-append "bbb=" bbb "," fam)))
    (if cal (set! fam (string-append "cal=" cal "," fam)))
    (if frak (set! fam (string-append "frak=" frak "," fam)))
    fam
  ) ;let*
) ;define

(define (selector-font-effects specs)
  (let* ((effs (list))
         (embold (selector-customize-get specs "embold" #f))
         (embbb (selector-customize-get specs "embbb" #f))
         (slant (selector-customize-get specs "slant" #f))
         (hmag (selector-customize-get specs "hmagnify" #f))
         (vmag (selector-customize-get specs "vmagnify" #f))
         (hext (selector-customize-get specs "hextended" #f))
         (vext (selector-customize-get specs "vextended" #f))
        ) ;
    (with add
      (lambda (var val)
        (when val
          (set! effs (rcons effs (string-append var "=" val)))
        ) ;when
      ) ;lambda
      (add "hmagnify" hmag)
      (add "vmagnify" vmag)
      (add "hextended" hext)
      (add "vextended" vext)
      (add "bold" embold)
      (add "bbb" embbb)
      (add "slant" slant)
      (string-recompose effs ",")
    ) ;with
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Font selector
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (font-default-sizes)
  '("5"
    "5.5"
    "6.5"
    "7.5"
    "8"
    "9"
    "10"
    "10.5"
    "12"
    "14"
    "15"
    "16"
    "18"
    "20"
    "22"
    "24"
    "26"
    "36"
    "42"
    "48"
    "64"
    "72"
    "96"
    "128"
    "144"
    "192")
) ;tm-define

(tm-define (font-default-sizes*)
  '("8"
    "8.5"
    "9"
    "9.5"
    "10"
    "10.5"
    "11"
    "11.5"
    "12"
    "14"
    "16"
    "18"
    "20"
    "24"
    "")
) ;tm-define

(tm-widget (font-family-selector* specs)
  (resize "300px"
    "350px"
    (scrollable (choice (selector-set specs :family answer)
                  (selected-families specs)
                  (selector-get specs :family)
                ) ;choice
    ) ;scrollable
  ) ;resize
) ;tm-widget

(tm-widget (font-style-selector* specs)
  (hlist (bold (text "Style"))
    //
    //
    (enum (selector-set specs :style answer)
      (selected-styles specs (selector-get specs :family))
      (selector-get specs :style)
      "10em"
    ) ;enum
  ) ;hlist
) ;tm-widget

(tm-widget (font-size-selector* specs)
  (hlist (bold (text "Size"))
    //
    //
    (enum (selector-set specs :size answer)
      (font-default-sizes*)
      (selector-get specs :size)
      "3em"
    ) ;enum
  ) ;hlist
) ;tm-widget

(define (font-sample-bg-color)
  (if (== (get-preference "gui theme") "liii-night") "#404040" "white")
) ;define

(define (font-sample-fg-color)
  (if (== (get-preference "gui theme") "liii-night") "#e0e0e0" "black")
) ;define

(tm-widget (font-properties-selector* specs)
  (aligned
    ;; (item (text "Base family:")
    ;;  (enum (selector-set specs :family answer)
    ;;        (font-database-families)
    ;;        (selector-get specs :family) "150px"))
    ;; (item (text "Base style:")
    ;;  (enum (selector-set specs :style answer)
    ;;        (font-database-styles (selector-get specs :family))
    ;;        (selector-get specs :style) "150px"))
    ;; (item ====== ======)
    (item (text "Weight:")
      (enum (selector-set* specs :weight answer)
        '("Any" "Thin" "Light" "Medium" "Bold" "Black")
        (selector-get specs :weight)
        "150px"
      ) ;enum
    ) ;item
    (item (text "Slant:")
      (enum (selector-set* specs :slant answer)
        '("Any" "Normal" "Italic" "Oblique")
        (selector-get specs :slant)
        "150px"
      ) ;enum
    ) ;item
    (item (text "Stretch:")
      (enum (selector-set* specs :stretch answer)
        '("Any" "Condensed" "Unextended" "Wide")
        (selector-get specs :stretch)
        "150px"
      ) ;enum
    ) ;item
    (item (text "Case:")
      (enum (selector-set* specs :case answer)
        '("Any" "Mixed" "Small Capitals")
        (selector-get specs :case)
        "150px"
      ) ;enum
    ) ;item
    (item ====== ======)
    (item (text "Serif:")
      (enum (selector-set* specs :serif answer)
        '("Any" "Serif" "Sans Serif")
        (selector-get specs :serif)
        "150px"
      ) ;enum
    ) ;item
    (item (text "Spacing:")
      (enum (selector-set* specs :spacing answer)
        '("Any" "Proportional" "Monospaced")
        (selector-get specs :spacing)
        "150px"
      ) ;enum
    ) ;item
    (item (text "Device:")
      (enum (selector-set* specs :device answer)
        '("Any" "Print" "Typewriter" "Digital" "Pen" "Art Pen" "Chalk" "Marker")
        (selector-get specs :device)
        "150px"
      ) ;enum
    ) ;item
    (item (text "Category:")
      (enum (selector-set* specs :category answer)
        '("Any"
          "Ancient"
          "Attached"
          "Calligraphic"
          "Comic"
          "Decorative"
          "Distorted"
          "Gothic"
          "Handwritten"
          "Initials"
          "Medieval"
          "Miscellaneous"
          "Outline"
          "Retro"
          "Scifi"
          "Title")
        (selector-get specs :category)
        "150px"
      ) ;enum
    ) ;item
    (item ====== ======)
    (item (text "Glyphs:")
      (enum (selector-set* specs :glyphs answer)
        '("Any"
          "ASCII"
          "Latin"
          "Greek"
          "Cyrillic"
          "CJK"
          "Hangul"
          "Math Symbols"
          "Math Extra"
          "Math Letters")
        (selector-get specs :glyphs)
        "150px"
      ) ;enum
    ) ;item
  ) ;aligned
  (horizontal (glue #f #t 0 0))
  ;; (horizontal
  ;;  >>>
  ;;  (toggle (selector-customize! answer) (selector-customize?)) ///
  ;;  (text "Advanced customizations")
  ;;  >>>)
) ;tm-widget

(define (font-effect-defaults which)
  (cond ((== which "embold") '("1" "1.25" "1.5" "2" "2.5" "3" "3.5" "4" ""))
        ((== which "embbb") '("1" "1.5" "2" "2.5" "3" "3.5" "4" "4.5" "5" ""))
        ((== which "slant")
         '("-0.5"
           "-0.25"
           "-0.1"
           "0"
           "0.1"
           "0.2"
           "0.25"
           "0.3"
           "0.4"
           "0.5"
           "0.75"
           "1"
           "")
        ) ;
        (else '("0.5"
                "0.6"
                "0.7"
                "0.8"
                "0.9"
                "1"
                "1.1"
                "1.2"
                "1.3"
                "1.4"
                "1.5"
                "1.6"
                "1.8"
                "2"
                "")
        ) ;else
  ) ;cond
) ;define

(define (font-effect-default which)
  (cond ((== which "slant") "0")
        (else "1")
  ) ;cond
) ;define

(tm-widget (font-effect-selector specs which)
  (enum (selector-customize-set! specs which answer)
    (font-effect-defaults which)
    (selector-customize-get specs which (font-effect-default which))
    "50px"
  ) ;enum
) ;tm-widget

(tm-widget (font-effects-selector specs)
  (vertical (aligned (item (text "Slant:") (dynamic (font-effect-selector specs "slant")))
              (item (text "Embold:") (dynamic (font-effect-selector specs "embold")))
              (item (text "Double stroke:") (dynamic (font-effect-selector specs "embbb")))
              (item (text "Extended:") (dynamic (font-effect-selector specs "hextended")))
              ;; (item (text "Extend vertically:")
              ;;  (dynamic (font-effect-selector specs "vextended")))
              (item (text "Magnify horizontally:")
                (dynamic (font-effect-selector specs "hmagnify"))
              ) ;item
              (item (text "Magnify vertically:")
                (dynamic (font-effect-selector specs "vmagnify"))
              ) ;item
            ) ;aligned
    (horizontal (glue #f #t 0 0))
  ) ;vertical
) ;tm-widget

(define (default-subfonts-list which)
  '("TeXmacs Computer Modern"
    "Stix"
    "TeX Gyre Bonum"
    "TeX Gyre Pagella"
    "TeX Gyre Schola"
    "TeX Gyre Termes")
) ;define

(define (default-subfonts which)
  (with l
    (cons "Default" (default-subfonts-list which))
    (if (in? which l) (append l (list "")) (append l (list which "")))
  ) ;with
) ;define

(tm-widget (subfont-selector specs which)
  (enum (selector-customize-set!* specs which answer)
    (default-subfonts (selector-customize-get* specs which "Default"))
    (selector-customize-get* specs which "Default")
    "160px"
  ) ;enum
) ;tm-widget

(tm-widget (font-variant-selector specs)
  (vertical (aligned (item (text "Bold:") (dynamic (subfont-selector specs "bold")))
              (item (text "Italic:") (dynamic (subfont-selector specs "italic")))
              (item (text "Small capitals:") (dynamic (subfont-selector specs "smallcaps")))
              (item (text "Sans serif:") (dynamic (subfont-selector specs "sansserif")))
              (item (text "Typewriter:") (dynamic (subfont-selector specs "typewriter")))
            ) ;aligned
    (horizontal (glue #f #t 0 0))
  ) ;vertical
) ;tm-widget

(tm-widget (font-math-selector specs)
  (vertical (aligned (item (text "Mathematics:") (dynamic (subfont-selector specs "math")))
              (item (text "Greek:") (dynamic (subfont-selector specs "greek")))
              (item (text "Blackboard bold:") (dynamic (subfont-selector specs "bbb")))
              (item (text "Calligraphic:") (dynamic (subfont-selector specs "cal")))
              (item (text "Fraktur:") (dynamic (subfont-selector specs "frak")))
            ) ;aligned
    (horizontal (glue #f #t 0 0))
  ) ;vertical
) ;tm-widget

(tm-define (font-import name)
  (font-database-extend-local name)
  (refresh-now "font-family-selector")
  (refresh-now "font-style-selector")
  (refresh-now "font-size-selector")
  (refresh-now "font-selector-demo")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main widgets
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-tool* (font-tool win name getter setter global?)
  (:name name)
  (with tool
    `(font-tool ,name ,getter ,setter ,global?)
    (with specs
      (list getter setter global? win)
      (with wide?
        (tool-bottom? tool win)
        (centered (vertical ===
                    (refreshable "font-family-selector" (dynamic (font-family-selector* specs)))
                    ===
                    (horizontal (refreshable "font-style-selector" (dynamic (font-style-selector* specs)))
                      >>>
                      (refreshable "font-size-selector" (dynamic (font-size-selector* specs)))
                    ) ;horizontal
                  ) ;vertical
        ) ;centered
        (assuming global?
          (division "discrete"
            (hlist >> ("Restore defaults" (selector-restore specs global?)))
            ===
          ) ;division
        ) ;assuming
        ======
        (section-tabs "font-tool-tabs"
          win
          (section-tab "Filters"
            (centered (dynamic (font-properties-selector* specs)))
            ======
            (refreshable "font-selector-demo"
              (dynamic (selector-font-simulate-widget specs))
            ) ;refreshable
          ) ;section-tab
          (section-tab "Effects" (centered (dynamic (font-effects-selector specs))))
          (section-tab "Variants" (centered (dynamic (font-variant-selector specs))))
          (section-tab "Mathematics" (centered (dynamic (font-math-selector specs))))
          (section-tab "More"
            (division "plain"
              (padded ("Import font" (choose-file font-import "Import font" ""))
               ("Scan disk for more fonts" (scan-disk-for-fonts))
               ("Clear font cache" (clear-font-cache))
              ) ;padded
            ) ;division
          ) ;section-tab
        ) ;section-tabs
      ) ;with
    ) ;with
  ) ;with
) ;tm-tool*

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; High level window interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 三个 window 入口切到 QML：register-specs 存 specs 拿 int 句柄并快照打开时字体，
;; cpp-font-selector-dialog 开 QML 对话框（非阻塞模态），fontBridge 调 font-selector-*
;; facade 透传交互。selector-set 实时写回（live）；Cancel/重置经快照写回撤销，OK 补齐
;; 差异落定。返回 tree 仅测试用。
(tm-define (open-font-selector-window)
  (:interactive #t)
  (with specs
    (list get-env make-multi-with #f)
    (selector-clean specs)
    (cpp-font-selector-dialog (font-selector-register-specs specs))
  ) ;with
) ;tm-define

(tm-define (open-document-font-selector-window)
  (:interactive #t)
  (with specs
    (list get-init init-multi #t)
    (selector-clean specs)
    (cpp-font-selector-dialog (font-selector-register-specs specs))
  ) ;with
) ;tm-define

(define ((prefixed-get-init prefix) var)
  (if (init-has? (string-append prefix var))
    (get-init (string-append prefix var))
    (get-init var)
  ) ;if
) ;define

(define ((prefixed-init-multi prefix) l)
  (when (and (nnull? l) (nnull? (cdr l)))
    (init-env (string-append prefix (car l)) (cadr l))
    ((prefixed-init-multi prefix) (cddr l))
  ) ;when
) ;define

(tm-define (open-document-other-font-selector-window prefix)
  (let* ((getter (prefixed-get-init prefix))
         (setter (prefixed-init-multi prefix))
         (specs (list getter setter #t))
        ) ;
    (selector-clean specs)
    (cpp-font-selector-dialog (font-selector-register-specs specs))
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; High level tool interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-font-tool name getter setter global?)
  (let* ((win (current-window))
         (specs (list getter setter global? win))
         (tool `(font-tool ,name ,getter ,setter ,global?))
        ) ;
    (selector-clean specs)
    (tool-select :right tool win)
  ) ;let*
) ;tm-define

(tm-define (open-font-selector)
  (:interactive #t)
  (if (side-tools?)
    (open-font-tool "Font" get-env make-multi-with #f)
    (open-font-selector-window)
  ) ;if
) ;tm-define

(tm-define (open-document-font-selector)
  (:interactive #t)
  (if (side-tools?)
    (open-font-tool "Document font" get-init init-multi #t)
    (open-document-font-selector-window)
  ) ;if
) ;tm-define

(tm-define (open-document-other-font-selector prefix)
  (:interactive #t)
  (if (side-tools?)
    (let* ((getter (prefixed-get-init prefix)) (setter (prefixed-init-multi prefix)))
      (open-font-tool "Font selector" getter setter #t)
    ) ;let*
    (open-document-other-font-selector-window prefix)
  ) ;if
) ;tm-define

(tm-define (short-font-menu-name)
  (with name
    (font-family-main (get-init "font"))
    (if (== name "sys-chinese") (translate "Font") (upcase-first (utf8->cork name)))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; QML font selector facade（record/qml/font-selector.md Phase 4）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 把上面的 selector-* 状态机经一个内聚 facade 暴露给 C++ FontSelectorBridge。
;; specs 含 getter/setter 过程，不能跨 glue 序列化，故维护 int 句柄注册表：开
;; 对话框时存 specs 返回 key，key 注入 QML，bridge 每次调用带 key，facade 按 key
;; 还原 specs。bridge 经 eval_scheme 调用本节 proc，无需逐个 glue。

(define specs-registry (make-ahash-table))

(define specs-registry-next 1)

(tm-define (font-selector-register-specs specs)
  (with key
    specs-registry-next
    (ahash-set! specs-registry key specs)
    ;; 快照打开瞬间的文档字体，供 Cancel/重置回放（live 改动后 get-init 读不到原值）。
    (ahash-set! initial-snapshot key (font-document-default-data))
    (set! specs-registry-next (+ specs-registry-next 1))
    key
  ) ;with
) ;tm-define

(tm-define (font-selector-lookup-specs key)
  (or (ahash-ref specs-registry key)
    (throw 'font-selector-qml "unknown specs key" key)
  ) ;or
) ;tm-define

;; 三栏：family / style / size。返回 scheme list of string。
(tm-define (font-selector-families key)
  (selected-families (font-selector-lookup-specs key))
) ;tm-define

(tm-define (font-selector-styles key family)
  (selected-styles (font-selector-lookup-specs key) family)
) ;tm-define

(tm-define (font-selector-sizes key) (font-default-sizes))

(tm-define (font-selector-get key var)
  (selector-get (font-selector-lookup-specs key) var)
) ;tm-define

;; live 写回：selector-set 经 selector-notify 实时写 buffer/init。refresh-now 对 QML
;; 对话框里不存在的 tm-widget 标签是 no-op。
(tm-define (font-selector-set key var val)
  (selector-set (font-selector-lookup-specs key) var val)
) ;tm-define

;; 联动 setter：set 后一并返回需刷新的依赖 + 预览（光栅化 data URL），省 QML 二次
;; 往返。返回 assoc list，bridge 转 QVariantMap。family 改动刷新 style 列表。
(tm-define (font-selector-set-family key family)
  (font-selector-set key :family family)
  `((styles unquote (font-selector-styles key family))
    (preview unquote (font-selector-preview key)))
) ;tm-define

(tm-define (font-selector-set-style key style)
  (font-selector-set key :style style)
  `((preview unquote (font-selector-preview key)))
) ;tm-define
(tm-define (font-selector-set-size key size)
  (font-selector-set key :size size)
  `((preview unquote (font-selector-preview key)))
) ;tm-define

;; 9 项 Filter 的可选项（集中自原 tm-widget 内联）。var 用 string（无冒号），facade
;; 内部 string->keyword 转——bridge/QML 全程普通 string，避免 keyword 跨界。

(define font-filter-options
  (list (cons "weight" '("Any" "Thin" "Light" "Medium" "Bold" "Black"))
    (cons "slant" '("Any" "Normal" "Italic" "Oblique"))
    (cons "stretch" '("Any" "Condensed" "Unextended" "Wide"))
    (cons "serif" '("Any" "Serif" "Sans Serif"))
    (cons "spacing" '("Any" "Proportional" "Monospaced"))
    (cons "case" '("Any" "Mixed" "Small Capitals"))
    (cons "device"
      '("Any" "Print" "Typewriter" "Digital" "Pen" "Art Pen" "Chalk" "Marker")
    ) ;cons
    (cons "category"
      '("Any"
        "Ancient"
        "Attached"
        "Calligraphic"
        "Comic"
        "Decorative"
        "Distorted"
        "Gothic"
        "Handwritten"
        "Initials"
        "Medieval"
        "Miscellaneous"
        "Outline"
        "Retro"
        "Scifi"
        "Title")
    ) ;cons
    (cons "glyphs"
      '("Any"
        "ASCII"
        "Latin"
        "Greek"
        "Cyrillic"
        "CJK"
        "Hangul"
        "Math Symbols"
        "Math Extra"
        "Math Letters")
    ) ;cons
  ) ;list
) ;define

(define font-filter-labels
  (list (cons "weight" "Weight")
    (cons "slant" "Slant")
    (cons "stretch" "Stretch")
    (cons "serif" "Serif")
    (cons "spacing" "Spacing")
    (cons "case" "Case")
    (cons "device" "Device")
    (cons "category" "Category")
    (cons "glyphs" "Glyphs")
  ) ;list
) ;define

(define (font-filter-label var)
  (or (assoc-ref font-filter-labels var) "Filter")
) ;define

;; 返回 (label var (options...) (optionsTr...) value) 五元组列表。label/optionsTr 翻译
;; 显示，options/value 保持英文原值（存储/过滤/回传）——与老版 tm-widget make-enum
;; 的 dec 反查等价的 key/value 分离，QML EnumCombo 显示 optionsTr、回传 options[index]。
(tm-define (font-selector-filter-meta key)
  (with specs
    (font-selector-lookup-specs key)
    (map (lambda (cell)
           (list (translate (font-filter-label (car cell)))
             (car cell)
             (cdr cell)
             (map translate (cdr cell))
             (selector-get specs (string->keyword (car cell)))
           ) ;list
         ) ;lambda
      font-filter-options
    ) ;map
  ) ;with
) ;tm-define

;; filter 写回走 selector-set（selector-set* 是 tm-define-macro 不能直接调用），
;; 返回 {families, preview} 由 bridge 驱动 QML 刷新。var 为 string（无冒号），内部
;; 转 keyword。
(tm-define (font-selector-set-filter key var val)
  (selector-set (font-selector-lookup-specs key) (string->keyword var) val)
  `((families unquote (font-selector-families key))
    (preview unquote (font-selector-preview key)))
) ;tm-define

;; 预览光栅化：同步返回 data URL。bg-color/magnification 包裹照搬 font-sample-text。
(tm-define (font-selector-preview key)
  (with specs
    (font-selector-lookup-specs key)
    (cpp-rasterize-widget (widget-texmacs-output `(with ,"bg-color"
                                                    ,(font-sample-bg-color)
                                                    ,"color"
                                                    ,(font-sample-fg-color)
                                                    ,"magnification"
                                                    ,"1.6"
                                                    ,(selector-font-demo-text specs))
                            '(style "generic")
                          ) ;widget-texmacs-output
    ) ;cpp-rasterize-widget
  ) ;with
) ;tm-define

;; 样本类型。

(define font-sample-kinds
  '("Standard"
    "Mathematics"
    "Selection"
    "ASCII"
    "Latin"
    "Greek"
    "Cyrillic"
    "CJK"
    "Hangul"
    "Math Symbols"
    "Math Extra"
    "Math Letters"
    "Unicode 0000-0fff"
    "Unicode 1000-1fff"
    "Unicode 2000-2fff"
    "Unicode 3000-3fff"
    "Unicode 4000-4fff")
) ;define

(tm-define (font-selector-sample-kinds key) font-sample-kinds)
(tm-define (font-selector-current-sample-kind key) sample-kind)
;; 设样本类型，返回 {preview}（样本内容随类型变）。
(tm-define (font-selector-set-sample-kind key kind)
  (set-font-sample-kind kind)
  `((preview unquote (font-selector-preview key)))
) ;tm-define

;; Advanced 定制（Effects / Variants / Mathematics）。每项返回
;; (group label which (options...) value)，QML 据此渲染 EnumCombo。

(define font-effect-meta
  (list (list "Effects" "Slant" "slant")
    (list "Effects" "Embold" "embold")
    (list "Effects" "Double stroke" "embbb")
    (list "Effects" "Extended" "hextended")
    (list "Effects" "Magnify horizontally" "hmagnify")
    (list "Effects" "Magnify vertically" "vmagnify")
  ) ;list
) ;define

(define font-variant-meta
  (list (list "Variants" "Bold" "bold")
    (list "Variants" "Italic" "italic")
    (list "Variants" "Small capitals" "smallcaps")
    (list "Variants" "Sans serif" "sansserif")
    (list "Variants" "Typewriter" "typewriter")
  ) ;list
) ;define

(define font-math-meta
  (list (list "Mathematics" "Mathematics" "math")
    (list "Mathematics" "Greek" "greek")
    (list "Mathematics" "Blackboard bold" "bbb")
    (list "Mathematics" "Calligraphic" "cal")
    (list "Mathematics" "Fraktur" "frak")
  ) ;list
) ;define

;; which 是否为子字体类（Variants/Mathematics 组），决定 options/value 来源。

(define (font-selector-subfont? which)
  (in? which
    '("bold"
      "italic"
      "smallcaps"
      "sansserif"
      "typewriter"
      "math"
      "greek"
      "bbb"
      "cal"
      "frak")
  ) ;in?
) ;define

(define (font-selector-customize-get-value specs which)
  (if (font-selector-subfont? which)
    (selector-customize-get* specs which "Default")
    (selector-customize-get specs which (font-effect-default which))
  ) ;if
) ;define

(define (font-selector--customize-item specs meta)
  (with (group label which)
    meta
    (with opts
      (if (font-selector-subfont? which)
        (default-subfonts (selector-customize-get* specs which "Default"))
        (font-effect-defaults which)
      ) ;if
      ;; optionsTr 对 options map translate：字体名/数值无字典项原样返回，"Default"
      ;; 等界面词翻译。与 filter-meta 同构的 key/value 分离。
      (list group
        (translate label)
        which
        opts
        (map translate opts)
        (font-selector-customize-get-value specs which)
      ) ;list
    ) ;with
  ) ;with
) ;define

(tm-define (font-selector-customize-meta key)
  (with specs
    (font-selector-lookup-specs key)
    (map (lambda (m) (font-selector--customize-item specs m))
      (append font-effect-meta font-variant-meta font-math-meta)
    ) ;map
  ) ;with
) ;tm-define

(tm-define (font-selector-customize-get key which)
  (font-selector-customize-get-value (font-selector-lookup-specs key) which)
) ;tm-define

;; 设定制项（影响 demo text 渲染，预览 live widget 自动重绘）。
(tm-define (font-selector-customize-set key which val)
  (with specs
    (font-selector-lookup-specs key)
    (if (font-selector-subfont? which)
      (selector-customize-set!* specs which val)
      (selector-customize-set! specs which val)
    ) ;if
  ) ;with
) ;tm-define

;; 把字体写回 initial-snapshot（打开对话框时），Cancel/重置共用。快照填 selector-table，
;; 再 selector-get-changes + 一次 setter 写回。不依赖 mark-cancel——它对 init 块无效、对
;; buffer 丢选区；显式写回两条 setter 路径都适用，单次 setter 避免多次 make-multi-with
;; 嵌套吞选区。

(define (font-selector-revert-to-snapshot key)
  (with specs
    (ahash-ref specs-registry key)
    (when specs
      (with (getter setter . other)
        specs
        (selector-clean specs)
        (with snap
          (ahash-ref initial-snapshot key)
          (ahash-set! selector-table (selkey specs :family) (car snap))
          (ahash-set! selector-table (selkey specs :style) (cadr snap))
          (ahash-set! selector-table (selkey specs :size) (caddr snap))
        ) ;with
        (with changes
          (selector-get-changes specs getter)
          (when (nnull? changes)
            (setter changes)
          ) ;when
        ) ;with
      ) ;with
    ) ;when
  ) ;with
) ;define

;; 注销 specs + 快照（reset 不调用：对话框仍打开）。

(define (font-selector-cleanup key)
  (ahash-remove! specs-registry key)
  (ahash-remove! initial-snapshot key)
) ;define

;; Cancel：撤销本次对话框的字体改动，写回打开对话框时（initial-snapshot）。
(tm-define (font-selector-cancel key)
  (font-selector-revert-to-snapshot key)
  (font-selector-cleanup key)
) ;tm-define

;; OK：补齐 selector-table 与当前文档的差异（live 路径大部分已实时写入），写回落定。
(tm-define (font-selector-commit key)
  (with specs
    (font-selector-lookup-specs key)
    (with (getter setter . other)
      specs
      (with changes
        (selector-get-changes specs getter)
        (when (nnull? changes)
          (setter changes)
        ) ;when
        changes
      ) ;with
    ) ;with
  ) ;with
  (font-selector-cleanup key)
) ;tm-define

;; 重置：字体回到打开对话框时（initial-snapshot，首次打开=文档默认）。机制同 Cancel。
(tm-define (font-selector-restore key) (font-selector-revert-to-snapshot key))

;; Import 由按钮显式触发，走 choose-file（QML 对话框在其下保持打开）。
(tm-define (font-selector-import key)
  (choose-file font-import "Import font" "")
) ;tm-define

;; 固定 UI 文案的翻译，供 QML 一次性拉取。key 为稳定标识符，value 经 translate
;; 跟随界面语言。
(tm-define (font-selector-ui-labels key)
  `((family unquote (translate "Font family"))
    (style unquote (translate "Style"))
    (size unquote (translate "Size"))
    (sample unquote (translate "Sample"))
    (filter unquote (translate "Filter"))
    (advanced unquote (translate "Advanced"))
    (import unquote (translate "Import"))
    (reset unquote (translate "Reset"))
    (ok unquote (translate "Ok"))
    (cancel unquote (translate "Cancel"))
    (done unquote (translate "Done")))
) ;tm-define
