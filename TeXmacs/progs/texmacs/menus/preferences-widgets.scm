
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : preferences-widgets.scm
;; DESCRIPTION : the preferences widgets
;; COPYRIGHT   : (C) 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs menus preferences-widgets)
  (:use (kernel texmacs pref-keys)
    (texmacs menus preferences-menu)
    (texmacs texmacs tm-files)
    (language locale)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Wrapper
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (set-pretty-preference* which pretty-val)
  (let* ((old (get-pretty-preference which)))
    (when (!= old pretty-val)
      ;; 三按钮确认：重启 / 稍后（silent 写值，下次启动生效）/ 取消（回滚旧值）。
      (confirm-restart-and-act (restart-preference-title which)
        (lambda () (set-pretty-preference which pretty-val))
        (lambda () (set-pretty-preference which old))
        (lambda () (set-pretty-preference-silent which pretty-val))
      ) ;confirm-restart-and-act
    ) ;when
  ) ;let*
) ;tm-define

(define (on-buffer-management-changed pretty-val)
  (let ((can-use-tabbar? (== pretty-val "Multiple documents share window")))
    (begin
      (set-boolean-preference "tab bar" can-use-tabbar?)
      (show-icon-bar 4 can-use-tabbar?)
      (set-pretty-preference "buffer management" pretty-val)
    ) ;begin
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Appearance preferences
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-preference-names "look and feel"
 ("default" "Default")
 ("emacs" "Emacs")
 ("gnome" "Gnome")
 ("kde" "KDE")
 ("macos" "macOS")
 ("windows" "Windows")
) ;define-preference-names

(for (l supported-languages)
  (set-preference-name "language" l (upcase-first l))
) ;for

(define-preference-names "complex actions"
 ("menus" "Through the menus")
 ("popups" "Through popup windows")
) ;define-preference-names

(define-preference-names "interactive questions"
 ("footer" "On the footer")
 ("popup" "In popup windows")
) ;define-preference-names

(define-preference-names "completion style"
 ("inline" "Inline")
 ("popup" "Popup")
) ;define-preference-names

(define-preference-names "detailed menus"
 ("simple " "Simplified menus")
 ("detailed" "Detailed menus")
) ;define-preference-names

(define-preference-names "buffer management"
 ("separate" "Documents in separate windows")
 ("shared" "Multiple documents share window")
) ;define-preference-names

(define-preference-names "gui theme" ("liii" "Liii") ("liii-night" "Liii Dark"))

(define-preference-names "magic-paste-shortcut"
 ("ctrl+shift+v" "Ctrl+Shift+V")
 ("ctrl+v" "Ctrl+V")
) ;define-preference-names

;; macOS 上 Ctrl 键实际由 Command 承担，展示串改用 Cmd 前缀。
;; preference 内部存储保持 "ctrl+..." 不变（与 generic-kbd.scm 比对一致），
;; 仅覆盖 encode 表影响首选项面板的显示。
(when (os-macos?)
  (set-preference-name "magic-paste-shortcut" "ctrl+shift+v" "Cmd+Shift+V")
  (set-preference-name "magic-paste-shortcut" "ctrl+v" "Cmd+V")
) ;when

(tm-widget (general-preferences-widget)
  (aligned (item (text "Look and feel:")
             (enum (set-pretty-preference* "look and feel" answer)
               (cond ((os-win32?) '("Default" "Emacs" "Windows"))
                     ((os-macos?) '("Default" "Emacs" "macOS"))
                     (else '("Default" "Emacs" "Gnome" "KDE"))
               ) ;cond
               (get-pretty-preference "look and feel")
               "18em"
             ) ;enum
           ) ;item
    (item (text "User interface language:")
      (enum (set-language-and-notify (language-name-to-language answer))
        (map language-to-language-name supported-languages)
        (language-to-language-name (get-preference "language"))
        "18em"
      ) ;enum
    ) ;item
    (item (text "Complex actions:")
      (enum (set-pretty-preference "complex actions" answer)
        '("Through the menus" "Through popup windows")
        (get-pretty-preference "complex actions")
        "18em"
      ) ;enum
    ) ;item
    (item (text "Interactive questions:")
      (enum (set-pretty-preference "interactive questions" answer)
        '("On the footer" "In popup windows")
        (get-pretty-preference "interactive questions")
        "18em"
      ) ;enum
    ) ;item
    (item (text "Details in menus:")
      (enum (set-pretty-preference "detailed menus" answer)
        '("Simplified menus" "Detailed menus")
        (get-pretty-preference "detailed menus")
        "18em"
      ) ;enum
    ) ;item
    (item (text "Buffer management:")
      (enum (on-buffer-management-changed answer)
        '("Documents in separate windows" "Multiple documents share window")
        (get-pretty-preference "buffer management")
        "18em"
      ) ;enum
    ) ;item
    (item (text "User interface theme:")
      (enum (set-pretty-preference* "gui theme" answer)
        '("Liii" "Liii Dark")
        (get-pretty-preference "gui theme")
        "18em"
      ) ;enum
    ) ;item
    (item (text "Completion style:")
      (enum (set-pretty-preference "completion style" answer)
        '("Popup" "Inline")
        (get-pretty-preference "completion style")
        "18em"
      ) ;enum
    ) ;item
    (item (text "Magic paste shortcut:")
      (enum (set-pretty-preference "magic-paste-shortcut" answer)
        (if (os-macos?) '("Cmd+Shift+V" "Cmd+V") '("Ctrl+Shift+V" "Ctrl+V"))
        (get-pretty-preference "magic-paste-shortcut")
        "18em"
      ) ;enum
    ) ;item
  ) ;aligned
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard preferences
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-preference-names "text spacebar"
 ("default" "Default")
 ("allow multiple spaces" "Allow multiple spaces")
 ("glue multiple spaces" "Glue multiple spaces")
 ("no multiple spaces" "No multiple spaces")
) ;define-preference-names

(define-preference-names "math spacebar"
 ("default" "Default")
 ("allow spurious spaces" "Allow spurious spaces")
 ("avoid spurious spaces" "Avoid spurious spaces")
 ("no spurious spaces" "No spurious spaces")
) ;define-preference-names

(define-preference-names "automatic quotes"
 ("default" "Default")
 ("none" "Disabled")
 ("dutch" "Dutch")
 ("english" "English")
 ("french" "French")
 ("german" "German")
 ("spanish" "Spanish")
 ("swiss" "Swiss")
) ;define-preference-names

(define-preference-names "automatic brackets"
 ("off" "Disabled")
 ("mathematics" "Inside mathematics" "mathematics")
 ("on" "Enabled")
) ;define-preference-names

(define-preference-names "cyrillic input method"
 ("none" "None")
 ("translit" "Translit")
 ("jcuken" "Jcuken")
 ("yawerty" "Yawerty")
) ;define-preference-names

(tm-widget (keyboard-preferences-widget)
  ======
  (aligned (item (text "Space bar in text mode:")
             (enum (set-pretty-preference "text spacebar" answer)
               '("Default"
                 "No multiple spaces"
                 "Glue multiple spaces"
                 "Allow multiple spaces")
               (get-pretty-preference "text spacebar")
               "15em"
             ) ;enum
           ) ;item
    (item (text "Space bar in math mode:")
      (enum (set-pretty-preference "math spacebar" answer)
        '("Default"
          "No spurious spaces"
          "Avoid spurious spaces"
          "Allow spurious spaces")
        (get-pretty-preference "math spacebar")
        "15em"
      ) ;enum
    ) ;item
    (item (text "Automatic quotes:")
      (enum (set-pretty-preference "automatic quotes" answer)
        '("Default"
          "Disabled"
          "Dutch"
          "English"
          "French"
          "German"
          "Spanish"
          "Swiss")
        (get-pretty-preference "automatic quotes")
        "15em"
      ) ;enum
    ) ;item
    (item (text "Automatic brackets:")
      (enum (set-pretty-preference "automatic brackets" answer)
        '("Disabled" "Enabled" "Inside mathematics")
        (get-pretty-preference "automatic brackets")
        "15em"
      ) ;enum
    ) ;item
    (item (text "Cyrillic input method:")
      (enum (set-pretty-preference "cyrillic input method" answer)
        '("None" "Translit" "Jcuken" "Yawerty")
        (get-pretty-preference "cyrillic input method")
        "15em"
      ) ;enum
    ) ;item
    (assuming (os-macos?)
      (item (text "Keyboard shortcut style:")
        (enum (set-pretty-preference* "keyboard shortcut style" answer)
          '("Text" "Symbol")
          (get-pretty-preference "keyboard shortcut style")
          "15em"
        ) ;enum
      ) ;item
    ) ;assuming
  ) ;aligned
  ======
  ======
  (bold (text "Remote controllers with keyboard simulation"))
  ======
  (hlist (aligned (item (text "Left:")
                    (enum (set-preference "ir-left" answer)
                      '("pageup" "")
                      (get-preference "ir-left")
                      "8em"
                    ) ;enum
                  ) ;item
           (item (text "Right:")
             (enum (set-preference "ir-right" answer)
               '("pagedown" "")
               (get-preference "ir-right")
               "8em"
             ) ;enum
           ) ;item
           (item (text "Up:")
             (enum (set-preference "ir-up" answer)
               '("home" "")
               (get-preference "ir-up")
               "8em"
             ) ;enum
           ) ;item
           (item (text "Down:")
             (enum (set-preference "ir-down" answer)
               '("end" "")
               (get-preference "ir-down")
               "8em"
             ) ;enum
           ) ;item
         ) ;aligned
    ///
    (aligned (item (text "Center:")
               (enum (set-preference "ir-center" answer)
                 '("return" "S-return" "")
                 (get-preference "ir-center")
                 "8em"
               ) ;enum
             ) ;item
      (item (text "Play:")
        (enum (set-preference "ir-play" answer)
          '("F5" "")
          (get-preference "ir-play")
          "8em"
        ) ;enum
      ) ;item
      (item (text "Pause:")
        (enum (set-preference "ir-pause" answer)
          '("escape" "")
          (get-preference "ir-pause")
          "8em"
        ) ;enum
      ) ;item
      (item (text "Menu:")
        (enum (set-preference "ir-menu" answer)
          '("." "")
          (get-preference "ir-menu")
          "8em"
        ) ;enum
      ) ;item
    ) ;aligned
  ) ;hlist
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Mathematics preferences widget
;; FIXME: - "assuming" has no effect in refreshable widgets
;;        - Too much alignment tweaking
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (math-keyboard-preferences-widget)
  (bold (text "Keyboard"))
  ======
  (aligned (meti (text "Use extensible brackets")
             (toggle (set-boolean-preference "use large brackets" answer)
               (get-boolean-preference "use large brackets")
             ) ;toggle
           ) ;meti
  ) ;aligned
) ;tm-widget

(tm-widget (math-hints-preferences-widget)
  (bold (text "Contextual hints"))
  ======
  (refreshable "math-pref-context"
    (aligned (meti (text "Show full context")
               (toggle (set-boolean-preference "show full context" answer)
                 (get-boolean-preference "show full context")
               ) ;toggle
             ) ;meti
      (meti (text "Show table cells")
        (toggle (set-boolean-preference "show table cells" answer)
          (get-boolean-preference "show table cells")
        ) ;toggle
      ) ;meti
      (meti (text "Show current focus")
        (toggle (set-boolean-preference "show focus" answer)
          (get-boolean-preference "show focus")
        ) ;toggle
      ) ;meti
      (assuming (get-boolean-preference "semantic editing")
        (meti (text "Only show semantic focus")
          (toggle (set-boolean-preference "show only semantic focus" answer)
            (get-boolean-preference "show only semantic focus")
          ) ;toggle
        ) ;meti
      ) ;assuming
    ) ;aligned
  ) ;refreshable
) ;tm-widget

(tm-widget (math-semantics-preferences-widget)
  (bold (text "Semantics"))
  ======
  (refreshable "math-pref-semantic-selections"
    (aligned (meti (text "Semantic editing")
               (toggle (and (set-boolean-preference "semantic editing" answer)
                         (refresh-now "math-pref-semantic-selections")
                         (refresh-now "math-pref-context")
                       ) ;and
                 (get-boolean-preference "semantic editing")
               ) ;toggle
             ) ;meti
      (assuming (get-boolean-preference "semantic editing")
        (meti (text "Semantic selections")
          (toggle (set-boolean-preference "semantic selections" answer)
            (get-boolean-preference "semantic selections")
          ) ;toggle
        ) ;meti
      ) ;assuming
      (assuming #f
        (meti (text "Semantic correctness")
          (toggle (set-boolean-preference "semantic correctness" answer)
            (get-boolean-preference "semantic correctness")
          ) ;toggle
        ) ;meti
      ) ;assuming
    ) ;aligned
  ) ;refreshable
) ;tm-widget

(tm-widget (math-correction-preferences-widget)
  (bold (text "Correction"))
  ======
  (aligned (meti (text "Remove superfluous invisible operators")
             (toggle (set-boolean-preference "manual remove superfluous invisible" answer)
               (get-boolean-preference "manual remove superfluous invisible")
             ) ;toggle
           ) ;meti
    (meti (text "Insert missing invisible operators")
      (toggle (set-boolean-preference "manual insert missing invisible" answer)
        (get-boolean-preference "manual insert missing invisible")
      ) ;toggle
    ) ;meti
    (meti (text "Homoglyph substitutions")
      (toggle (set-boolean-preference "manual homoglyph correct" answer)
        (get-boolean-preference "manual homoglyph correct")
      ) ;toggle
    ) ;meti
  ) ;aligned
) ;tm-widget

(tm-widget (math-preferences-widget)
  (padded (hlist (vlist (dynamic (math-keyboard-preferences-widget))
                   ======
                   ======
                   (dynamic (math-hints-preferences-widget))
                   (glue #f #t 0 1)
                 ) ;vlist
            (glue #f #f 30 0)
            (vlist (dynamic (math-semantics-preferences-widget))
              ======
              ======
              (dynamic (math-correction-preferences-widget))
              (glue #f #t 0 1)
            ) ;vlist
          ) ;hlist
  ) ;padded
) ;tm-widget

(tm-widget (math-preferences-widget*)
  (dynamic (math-keyboard-preferences-widget))
  ======
  ======
  (dynamic (math-hints-preferences-widget))
  ======
  ======
  (dynamic (math-semantics-preferences-widget))
  ======
  ======
  (dynamic (math-correction-preferences-widget))
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Conversion preferences widget
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Html ----------

(define (export-formulas-as-mathjax on?)
  (set-boolean-preference "texmacs->html:mathjax" on?)
  (when on?
    (set-boolean-preference "texmacs->html:mathml" #f)
    (set-boolean-preference "texmacs->html:images" #f)
    (refresh-now "texmacs to html")
  ) ;when
) ;define

(define (export-formulas-as-mathml on?)
  (set-boolean-preference "texmacs->html:mathml" on?)
  (when on?
    (set-boolean-preference "texmacs->html:mathjax" #f)
    (set-boolean-preference "texmacs->html:images" #f)
    (refresh-now "texmacs to html")
  ) ;when
) ;define

(define (export-formulas-as-images on?)
  (set-boolean-preference "texmacs->html:images" on?)
  (when on?
    (set-boolean-preference "texmacs->html:mathjax" #f)
    (set-boolean-preference "texmacs->html:mathml" #f)
    (refresh-now "texmacs to html")
  ) ;when
) ;define

(tm-widget (html-preferences-widget)
  ======
  (bold (text "TeXmacs -> Html"))
  ===
  (refreshable "texmacs to html"
    (aligned (meti (hlist // (text "Use CSS for more advanced formatting"))
               (toggle (set-boolean-preference "texmacs->html:css" answer)
                 (get-boolean-preference "texmacs->html:css")
               ) ;toggle
             ) ;meti
      (meti (hlist // (text "Export mathematical formulas as MathJax"))
        (toggle (export-formulas-as-mathjax answer)
          (get-boolean-preference "texmacs->html:mathjax")
        ) ;toggle
      ) ;meti
      (meti (hlist // (text "Export mathematical formulas as MathML"))
        (toggle (export-formulas-as-mathml answer)
          (get-boolean-preference "texmacs->html:mathml")
        ) ;toggle
      ) ;meti
      (meti (hlist // (text "Export mathematical formulas as images"))
        (toggle (export-formulas-as-images answer)
          (get-boolean-preference "texmacs->html:images")
        ) ;toggle
      ) ;meti
    ) ;aligned
    ===
    (hlist (text "CSS stylesheet:")
      //
      (enum (set-preference "texmacs->html:css-stylesheet" answer)
        '("---"
          "https://www.texmacs.org/css/web-article.css"
          "https://www.texmacs.org/css/web-article-dark.css"
          "https://www.texmacs.org/css/web-article-colored.css"
          "https://www.texmacs.org/css/web-article-dark-colored.css"
          "")
        (get-preference "texmacs->html:css-stylesheet")
        "18em"
      ) ;enum
    ) ;hlist
  ) ;refreshable
  ======
  ======
  (bold (text "Html -> TeXmacs"))
  ===
  (refreshable "html -> texmacs"
    (aligned (meti (hlist // (text "Try to import formulas using LaTeX annotations"))
               (toggle (set-boolean-preference "mathml->texmacs:latex-annotations" answer)
                 (get-boolean-preference "mathml->texmacs:latex-annotations")
               ) ;toggle
             ) ;meti
    ) ;aligned
  ) ;refreshable
) ;tm-widget

;; LaTeX ----------

(define-preference-names "texmacs->latex:encoding"
 ("cork" "Cork with catcodes")
 ("utf-8" "Utf-8 with inputenc")
) ;define-preference-names

(define (get-latex-source-tracking)
  (or (get-boolean-preference "latex->texmacs:source-tracking")
    (get-boolean-preference "texmacs->latex:source-tracking")
  ) ;or
) ;define

(define (set-latex-source-tracking on?)
  (set-boolean-preference "latex->texmacs:source-tracking" on?)
  (set-boolean-preference "texmacs->latex:source-tracking" on?)
  (refresh-now "source-tracking")
) ;define

(define (get-latex-conservative)
  (and (get-boolean-preference "latex->texmacs:conservative")
    (get-boolean-preference "texmacs->latex:conservative")
  ) ;and
) ;define

(define (set-latex-conservative on?)
  (set-boolean-preference "latex->texmacs:conservative" on?)
  (set-boolean-preference "texmacs->latex:conservative" on?)
  (refresh-now "source-tracking")
) ;define

(define (get-latex-transparent-source-tracking)
  (or (get-boolean-preference "latex->texmacs:transparent-source-tracking")
    (get-boolean-preference "texmacs->latex:transparent-source-tracking")
  ) ;or
) ;define

(define (set-latex-transparent-source-tracking on?)
  (set-boolean-preference "latex->texmacs:transparent-source-tracking" on?)
  (set-boolean-preference "texmacs->latex:transparent-source-tracking" on?)
) ;define

(tm-widget (latex-preferences-widget)
  ======
  (bold (text "LaTeX -> TeXmacs"))
  ===
  (aligned (meti (hlist // (text "Import sophisticated objects as pictures"))
             (toggle (set-boolean-preference "latex->texmacs:fallback-on-pictures" answer)
               (get-boolean-preference "latex->texmacs:fallback-on-pictures")
             ) ;toggle
           ) ;meti
  ) ;aligned
  ======
  ======
  (bold (text "TeXmacs -> LaTeX"))
  ===
  (aligned (meti (hlist // (text "Replace TeXmacs styles with no LaTeX equivalents"))
             (toggle (set-boolean-preference "texmacs->latex:replace-style" answer)
               (get-boolean-preference "texmacs->latex:replace-style")
             ) ;toggle
           ) ;meti
    (meti (hlist // (text "Expand TeXmacs macros with no LaTeX equivalents"))
      (toggle (set-boolean-preference "texmacs->latex:expand-macros" answer)
        (get-boolean-preference "texmacs->latex:expand-macros")
      ) ;toggle
    ) ;meti
    (meti (hlist // (text "Expand user-defined macros"))
      (toggle (set-boolean-preference "texmacs->latex:expand-user-macros" answer)
        (get-boolean-preference "texmacs->latex:expand-user-macros")
      ) ;toggle
    ) ;meti
    (meti (hlist // (text "Export bibliographies as links"))
      (toggle (set-boolean-preference "texmacs->latex:indirect-bib" answer)
        (get-boolean-preference "texmacs->latex:indirect-bib")
      ) ;toggle
    ) ;meti
    (meti (hlist // (text "Allow for macro definitions in preamble"))
      (toggle (set-boolean-preference "texmacs->latex:use-macros" answer)
        (get-boolean-preference "texmacs->latex:use-macros")
      ) ;toggle
    ) ;meti
  ) ;aligned
  ===
  (aligned (item (text "Character encoding:")
             (enum (set-pretty-preference "texmacs->latex:encoding" answer)
               '("Utf-8 with inputenc" "Cork with catcodes")
               (get-pretty-preference "texmacs->latex:encoding")
               "15em"
             ) ;enum
           ) ;item
  ) ;aligned
  ======
  ======
  (bold (text "Conservative conversion options"))
  ===
  (refreshable "source-tracking"
    (aligned (meti (hlist // (text "Keep track of source code"))
               (toggle (set-latex-source-tracking answer) (get-latex-source-tracking))
             ) ;meti
      (meti (hlist // (text "Only convert changes with respect to tracked version"))
        (toggle (set-latex-conservative answer) (get-latex-conservative))
      ) ;meti
      (meti (when (get-latex-source-tracking)
              (hlist // (text "Guarantee transparent source tracking"))
            ) ;when
        (when (get-latex-source-tracking)
          (toggle (set-latex-transparent-source-tracking answer)
            (get-latex-transparent-source-tracking)
          ) ;toggle
        ) ;when
      ) ;meti
      (meti (when (get-latex-source-tracking)
              (hlist // (text "Store tracking information in LaTeX files"))
            ) ;when
        (when (get-latex-source-tracking)
          (toggle (set-boolean-preference "texmacs->latex:attach-tracking-info" answer)
            (get-boolean-preference "texmacs->latex:attach-tracking-info")
          ) ;toggle
        ) ;when
      ) ;meti
    ) ;aligned
  ) ;refreshable
) ;tm-widget

;; BibTeX ----------

(define (get-bibtm-conservative)
  (get-boolean-preference "bibtex->texmacs:conservative")
) ;define

(define (set-bibtm-conservative on?)
  (set-boolean-preference "bibtex->texmacs:conservative" on?)
) ;define

(define (get-tmbib-conservative)
  (get-boolean-preference "texmacs->bibtex:conservative")
) ;define

(define (set-tmbib-conservative on?)
  (set-boolean-preference "texmacs->bibtex:conservative" on?)
) ;define

(tm-widget (bibtex-preferences-widget)
  ===
  (bold (text "BibTeX -> TeXmacs"))
  ===
  (aligned (item (text "BibTeX command:")
             (enum (set-pretty-preference "bibtex command" answer)
               '("bibtex" "biber" "biblatex" "rubibtex" "")
               (get-pretty-preference "bibtex command")
               "15em"
             ) ;enum
           ) ;item
  ) ;aligned
  ===
  (aligned (meti (hlist // (text "Only convert changes when re-importing"))
             (toggle (set-bibtm-conservative answer) (get-bibtm-conservative))
           ) ;meti
  ) ;aligned
  ======
  ======
  (bold (text "TeXmacs -> BibTeX"))
  ===
  (aligned (meti (hlist // (text "Only convert changes with respect to imported version"))
             (toggle (set-tmbib-conservative answer) (get-tmbib-conservative))
           ) ;meti
  ) ;aligned
) ;tm-widget

;; Verbatim ----------

(define-preference-names "texmacs->verbatim:encoding"
 ("auto" "Automatic")
 ("cork" "Cork")
 ("iso-8859-1" "Iso-8859-1")
 ("iso-8859-2" "Iso-8859-2")
 ("utf-8" "UTF-8")
) ;define-preference-names

(define-preference-names "verbatim->texmacs:encoding"
 ("utf-8" "UTF-8")
 ("auto" "Automatic")
 ("cork" "Cork")
 ("iso-8859-1" "ISO-8859-1")
 ("iso-8859-2" "ISO-8859-2")
) ;define-preference-names

(tm-widget (verbatim-preferences-widget)
  ======
  (bold (text "TeXmacs -> Verbatim"))
  ===
  (aligned (meti (hlist //
                   (text "Use line wrapping for lines which are longer than 80 characters")
                 ) ;hlist
             (toggle (set-boolean-preference "texmacs->verbatim:wrap" answer)
               (get-boolean-preference "texmacs->verbatim:wrap")
             ) ;toggle
           ) ;meti
  ) ;aligned
  ===
  (aligned (item (text "Character encoding:")
             (enum (set-pretty-preference "texmacs->verbatim:encoding" answer)
               '("Automatic" "Cork" "ISO-8859-1" "ISO-8859-2" "UTF-8")
               (get-pretty-preference "texmacs->verbatim:encoding")
               "12em"
             ) ;enum
           ) ;item
  ) ;aligned
  ======
  ======
  (bold (text "Verbatim -> TeXmacs"))
  ===
  (aligned (meti (hlist // (text "Merge lines into paragraphs unless separated by blank lines"))
             (toggle (set-boolean-preference "verbatim->texmacs:wrap" answer)
               (get-boolean-preference "verbatim->texmacs:wrap")
             ) ;toggle
           ) ;meti
  ) ;aligned
  ===
  (aligned (item (text "Character encoding:")
             (enum (set-pretty-preference "verbatim->texmacs:encoding" answer)
               '("UTF-8" "Automatic" "Cork" "ISO-8859-1" "ISO-8859-2")
               (get-pretty-preference "verbatim->texmacs:encoding")
               "12em"
             ) ;enum
           ) ;item
  ) ;aligned
) ;tm-widget

;; Pdf ----------
(define-preference-names "texmacs->pdf:version"
 ("Default" "default")
 ("1.4" "1.4")
 ("1.5" "1.5")
 ("1.6" "1.6")
 ("1.7" "1.7")
) ;define-preference-names

(tm-widget (pdf-preferences-widget)
  ======
  (bold (text "TeXmacs -> Pdf/Postscript"))
  ===
  (aligned (meti (hlist // (text "Expand beamer slides"))
             (toggle (set-boolean-preference "texmacs->pdf:expand slides" answer)
               (get-boolean-preference "texmacs->pdf:expand slides")
             ) ;toggle
           ) ;meti
    (meti (hlist // (text "Use external pdf viewer"))
      (toggle (set-boolean-preference "use external pdf viewer" answer)
        (get-boolean-preference "use external pdf viewer")
      ) ;toggle
    ) ;meti
  ) ;aligned
  (assuming (supports-native-pdf?)
    (aligned (item (text "Pdf version number:")
               (enum (set-preference "texmacs->pdf:version" answer)
                 '("default" "1.4" "1.5" "1.6" "1.7")
                 (get-preference "texmacs->pdf:version")
                 "12em"
               ) ;enum
             ) ;item
    ) ;aligned
  ) ;assuming
) ;tm-widget

;; Images ----------

(define (pretty-format-list)
  (let* ((desired-image-format-list '(("svg" "Svg")
                                      ("eps" "Eps")
                                      ("png" "Png")
                                      ("tif" "Tiff")
                                      ("jpg" "Jpeg")
                                      ("pdf" "Pdf"))
         ) ;desired-image-format-list
         (valid-image-format-list (filter (lambda (x) (file-converter-exists? "x.pdf" (string-append "x." (car x))))
                                    desired-image-format-list
                                  ) ;filter
         ) ;valid-image-format-list
        ) ;
    (eval `(define-preference-names ,"texmacs->image:format"
             ,@valid-image-format-list)
    ) ;eval
    (cadr (apply map list valid-image-format-list))
  ) ;let*
) ;define

(tm-widget (image-preferences-widget)
  ======
  (bold (text "TeXmacs -> Image"))
  ===
  (aligned (item (text "Bitmap export resolution (dpi):")
             (enum (set-preference "texmacs->image:raster-resolution" answer)
               '("1200" "600" "300" "150" "")
               (get-preference "texmacs->image:raster-resolution")
               "8em"
             ) ;enum
           ) ;item
    (item (text "Clipboard image format:")
      (enum (set-pretty-preference "texmacs->image:format" answer)
        (pretty-format-list)
        (get-pretty-preference "texmacs->image:format")
        "8em"
      ) ;enum
    ) ;item
  ) ;aligned
) ;tm-widget

;; Mogan Scheme ----------

;; All converters ----------

(tm-widget (conversion-preferences-widget)
  ===
  (padded (tabs (tab (text "Html") (centered (dynamic (html-preferences-widget))))
            (tab (text "LaTeX") (centered (dynamic (latex-preferences-widget))))
            (tab (text "BibTeX") (centered (dynamic (bibtex-preferences-widget))))
            (tab (text "Verbatim") (centered (dynamic (verbatim-preferences-widget))))
            (assuming (or (supports-native-pdf?) (supports-ghostscript?))
              (tab (text "Pdf") (centered (dynamic (pdf-preferences-widget))))
            ) ;assuming
            (tab (text "Image") (centered (dynamic (image-preferences-widget))))
          ) ;tabs
  ) ;padded
  ===
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Other
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define autosave-enabled-label "On")

(define autosave-disabled-label "Off")

(tm-define (autosave-preferences-list)
  (list autosave-enabled-label autosave-disabled-label)
) ;tm-define

(tm-define (get-autosave-preference-label)
  (if (== (get-preference "autosave") "0")
    autosave-disabled-label
    autosave-enabled-label
  ) ;if
) ;tm-define

(tm-define (set-autosave-preference-label label)
  (set-preference "autosave" (if (== label autosave-disabled-label) "0" "120"))
) ;tm-define

(define-preference-names "autosave" ("120" "On") ("0" "Off"))

(define-preference-names "security"
 ("accept no scripts" "Accept no scripts")
 ("prompt on scripts" "Prompt on scripts")
 ("accept all scripts" "Accept all scripts")
) ;define-preference-names

(define-preference-names "updater:interval"
 ("0" "Never")
 ("0" "Unsupported")
 ("24" "Once a day")
 ("168" "Once a week")
 ("720" "Once a month")
) ;define-preference-names

(define-preference-names "document update times"
 ("1" "Once")
 ("2" "Twice")
 ("3" "Three times")
) ;define-preference-names

(define-preference-names "scripting language" ("none" "None"))

(define (updater-last-check-formatted)
  "Time since last update check formatted for use in the preferences dialog"
  (with c
    (updater-last-check)
    (if (<= c 0)
      "Never"
      (with h
        (ceiling (/ (- (current-time) c) 3600))
        (cond ((< h 24) (replace "Less than %1 hour(s) ago" h))
              ((< h 720) (replace "%1 days ago" (ceiling (/ h 24))))
              (else (translate "More than 1 month ago"))
        ) ;cond
      ) ;with
    ) ;if
  ) ;with
) ;define

(define (last-check-string)
  (if (use-plugin-updater?) (updater-last-check-formatted) "Never (unsupported)")
) ;define

(define (automatic-checks-choices)
  (if (use-plugin-updater?)
    '("Never" "Once a day" "Once a week" "Once a month")
    '("Unsupported")
  ) ;if
) ;define

(tm-define (scripts-preferences-list)
  (lazy-plugin-force)
  (with l
    (scripts-list)
    (for (x l) (set-preference-name "scripting language" x (scripts-name x)))
    (cons "None" (map scripts-name l))
  ) ;with
) ;tm-define

(tm-widget (script-preferences-widget)
  (aligned (item (text "Execution of scripts:")
             (enum (set-pretty-preference "security" answer)
               '("Accept no scripts" "Prompt on scripts" "Accept all scripts")
               (get-pretty-preference "security")
               "15em"
             ) ;enum
           ) ;item
  ) ;aligned
) ;tm-widget

(tm-widget (security-preferences-widget)
  (refreshable "security-preferences-refresher"
    (padded ======
      (bold (text "Wallet"))
      ===
      (dynamic (wallet-preferences-widget))
      ======
      ======
      (bold (text "Encryption"))
      ===
      (dynamic (gpg-preferences-widget))
      ;; ====== ======
      ;; (bold (text "Scripts"))
      ;; ===
      ;; (dynamic (script-preferences-widget))
    ) ;padded
  ) ;refreshable
) ;tm-widget

(tm-widget (misc-preferences-widget)
  (aligned (item (text "Automatically save:")
             (enum (set-autosave-preference-label answer)
               (autosave-preferences-list)
               (get-autosave-preference-label)
               "12em"
             ) ;enum
           ) ;item
    (item (text "Auto backup:")
      (hlist (enum (set-preference "autobackup" (string-downcase answer))
               '("On" "Off")
               (tmstring-upcase-first (get-preference "autobackup"))
               "12em"
             ) ;enum
        //
        (explicit-buttons ((eval (begin
                                   (when (not (defined? 'auto-backup-button-label))
                                     (use-modules (plugin autosave))
                                   ) ;when
                                   (auto-backup-button-label)
                                 ) ;begin
                           ) ;eval
                           (open-auto-backup-location)
                          ) ;
        ) ;explicit-buttons
      ) ;hlist
    ) ;item
    (item (text "Security:")
      (enum (set-pretty-preference "security" answer)
        '("Accept no scripts" "Prompt on scripts" "Accept all scripts")
        (get-pretty-preference "security")
        "12em"
      ) ;enum
    ) ;item
    (item (text "Scripting language:")
      (enum (set-pretty-preference "scripting language" answer)
        (scripts-preferences-list)
        (get-pretty-preference "scripting language")
        "12em"
      ) ;enum
    ) ;item
    (item (text "Document updates run:")
      (enum (set-pretty-preference "document update times" answer)
        '("Once" "Twice" "Three times")
        (get-pretty-preference "document update times")
        "12em"
      ) ;enum
    ) ;item
    (assuming (use-plugin-updater?)
      (item (text "Check for automatic updates:")
        (enum (set-pretty-preference "updater:interval" answer)
          (automatic-checks-choices)
          (get-pretty-preference "updater:interval")
          "12em"
        ) ;enum
      ) ;item
    ) ;assuming
    (assuming (use-plugin-updater?)
      (item (text "Last check:") (text (last-check-string)))
    ) ;assuming
  ) ;aligned
) ;tm-widget

(tm-widget (experimental-preferences-widget)
  (hlist (aligned (meti (hlist // (text "Fast environments"))
                    (toggle (set-boolean-preference "fast environments" answer)
                      (get-boolean-preference "fast environments")
                    ) ;toggle
                  ) ;meti
           (meti (hlist // (text "Alpha transparency"))
             (toggle (set-boolean-preference "experimental alpha" answer)
               (get-boolean-preference "experimental alpha")
             ) ;toggle
           ) ;meti
           (meti (hlist // (text "New style page breaking"))
             (toggle (set-boolean-preference "new style page breaking" answer)
               (get-boolean-preference "new style page breaking")
             ) ;toggle
           ) ;meti
           (meti (hlist // (text "Encryption"))
             (toggle (set-boolean-preference "experimental encryption" answer)
               (get-boolean-preference "experimental encryption")
             ) ;toggle
           ) ;meti
           (assuming (os-macos?)
             (meti (hlist // (text "Use native menubar"))
               (toggle (set-boolean-preference "use native menubar" answer)
                 (get-boolean-preference "use native menubar")
               ) ;toggle
             ) ;meti
           ) ;assuming
           (assuming (os-macos?)
             (meti (hlist // (text "Use unified toolbars"))
               (toggle (set-boolean-preference "use unified toolbar" answer)
                 (get-boolean-preference "use unified toolbar")
               ) ;toggle
             ) ;meti
           ) ;assuming
         ) ;aligned
    ///
    ///
    (aligned (meti (hlist // (text "Program bracket matching"))
               (toggle (set-boolean-preference "prog:highlight brackets" answer)
                 (get-boolean-preference "prog:highlight brackets")
               ) ;toggle
             ) ;meti
      (meti (hlist // (text "Automatic program brackets"))
        (toggle (set-boolean-preference "prog:automatic brackets" answer)
          (get-boolean-preference "prog:automatic brackets")
        ) ;toggle
      ) ;meti
      (meti (hlist // (text "Program bracket selections"))
        (toggle (set-boolean-preference "prog:select brackets" answer)
          (get-boolean-preference "prog:select brackets")
        ) ;toggle
      ) ;meti
      (meti (hlist // (text "Case-insensitive search"))
        (toggle (set-boolean-preference "case-insensitive-match" answer)
          (get-boolean-preference "case-insensitive-match")
        ) ;toggle
      ) ;meti
      (assuming (qt-gui?)
        (meti (hlist // (text "Use print dialogue"))
          (toggle (set-boolean-preference "gui:print dialogue" answer)
            (get-boolean-preference "gui:print dialogue")
          ) ;toggle
        ) ;meti
      ) ;assuming
      (meti (hlist // (text "Use fonts in texlive"))
        (toggle (set-boolean-preference "texlive:fonts" answer)
          (get-boolean-preference "texlive:fonts")
        ) ;toggle
      ) ;meti
    ) ;aligned
  ) ;hlist
) ;tm-widget

(tm-widget (experimental-preferences-widget*)
  (aligned (meti (hlist // (text "Encryption"))
             (toggle (set-boolean-preference "experimental encryption" answer)
               (get-boolean-preference "experimental encryption")
             ) ;toggle
           ) ;meti
    (meti (hlist // (text "Fast environments"))
      (toggle (set-boolean-preference "fast environments" answer)
        (get-boolean-preference "fast environments")
      ) ;toggle
    ) ;meti
    ;; (meti (hlist // (text "Alpha transparency"))
    ;;  (toggle (set-boolean-preference "experimental alpha" answer)
    ;;          (get-boolean-preference "experimental alpha")))
    (meti (hlist // (text "New style page breaking"))
      (toggle (set-boolean-preference "new style page breaking" answer)
        (get-boolean-preference "new style page breaking")
      ) ;toggle
    ) ;meti
    (meti (hlist // (text "Program bracket matching"))
      (toggle (set-boolean-preference "prog:highlight brackets" answer)
        (get-boolean-preference "prog:highlight brackets")
      ) ;toggle
    ) ;meti
    (meti (hlist // (text "Automatic program brackets"))
      (toggle (set-boolean-preference "prog:automatic brackets" answer)
        (get-boolean-preference "prog:automatic brackets")
      ) ;toggle
    ) ;meti
    (meti (hlist // (text "Program bracket selections"))
      (toggle (set-boolean-preference "prog:select brackets" answer)
        (get-boolean-preference "prog:select brackets")
      ) ;toggle
    ) ;meti
    ;; (meti (hlist // (text "Case-insensitive search"))
    ;;  (toggle (set-boolean-preference "case-insensitive-match" answer)
    ;;          (get-boolean-preference "case-insensitive-match")))
    (assuming (qt-gui?)
      (meti (hlist // (text "Use print dialogue"))
        (toggle (set-boolean-preference "gui:print dialogue" answer)
          (get-boolean-preference "gui:print dialogue")
        ) ;toggle
      ) ;meti
    ) ;assuming
    (assuming (os-macos?)
      (meti (hlist // (text "Use native menubar"))
        (toggle (set-boolean-preference "use native menubar" answer)
          (get-boolean-preference "use native menubar")
        ) ;toggle
      ) ;meti
      (meti (hlist // (text "Use unified toolbars"))
        (toggle (set-boolean-preference "use unified toolbar" answer)
          (get-boolean-preference "use unified toolbar")
        ) ;toggle
      ) ;meti
    ) ;assuming
  ) ;aligned
) ;tm-widget

(tm-widget (other-preferences-widget)
  (centered (dynamic (misc-preferences-widget))
    ======
    (bold (text "Experimental features (to be used with care)"))
    ======
    (dynamic (experimental-preferences-widget))
  ) ;centered
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences widget
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (preferences-widget)
  (centered (icon-tabs (icon-tab "tm_prefs_general.xpm"
                         (text "General")
                         (centered (dynamic (general-preferences-widget)))
                       ) ;icon-tab
              (icon-tab "tm_prefs_keyboard.xpm"
                (text "Keyboard")
                (centered (dynamic (keyboard-preferences-widget)))
              ) ;icon-tab
              ;; TODO: please implement nice icon tabs first before
              ;; adding new tabs in the preferences widget
              ;; The tabs currently take too much horizontal space
              (icon-tab "tm_prefs_math.xpm"
                (text "Mathematics")
                (centered (dynamic (math-preferences-widget)))
              ) ;icon-tab
              (icon-tab "tm_prefs_convert.xpm"
                (text "Convert")
                (dynamic (conversion-preferences-widget))
              ) ;icon-tab
              (icon-tab "tm_prefs_other.xpm"
                (text "Other")
                (centered (dynamic (other-preferences-widget)))
              ) ;icon-tab
            ) ;icon-tabs
  ) ;centered
) ;tm-widget

(tm-define (open-preferences-window)
  (:interactive #t)
  (top-window preferences-widget "User preferences")
) ;tm-define

;; QML 重写后：非 side-tools 模式直接开 QML 弹窗，side-tools 模式仍走旧
;; open-preferences-window（其 dynamic 各 tab 的旧 tm-widget body 仍保留作 fallback）。
(tm-define (open-preferences)
  (:interactive #t)
  (if (side-tools?)
    (begin
      (tool-select :right 'preferences-tool)
      (open-preferences-window)
    ) ;begin
    (cpp-preferences-dialog)
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; QML facade：preferences-qml-meta / -submit / -set-field
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 字段定义数据格式（紧凑、一眼可见选项列表；参考 paragraph-format-widgets.scm
;; 的 paragraph-basic-fields 顶部 define 模式）：
;;   (key label options options-pretty editable? . flags)
;;
;; key          —— 偏好键（内部存储形；走 pref-keys.scm 的 pref-* proc 引用，单一可信源）
;; label        —— 显示文案（Cork 编码的原文；meta 输出时经 translate 包装；不在这里预翻译——
;;                 field->descriptor 统一包装，避免翻译漏包 + 与 paragraph-format 一致）
;; options      —— 内部键列表（combo 专用）
;; options-pretty—— 翻译显示列表，与 options 等长同序（combo；空则回退显示 options 原文）
;; editable?    —— 是否允许双击进入可编辑输入态（combo 专用；toggle/info 忽略）
;; flags        —— 可选 plist：restart? / radio-group / visible-when-key + visible-when-val /
;;                 group / hint / column（见顶部契约文档）
;;
;; kind 分流（combo / toggle / info）由 options 是否非空决定：
;;   有 options / options-pretty -> combo（下拉）
;;   无 options（且 key 非空）  -> toggle（开关；value 为 "on"/"off"）
;;   key 为空                  -> info（只读展示行；无 setter、无 diff）
;;
;; flag plist 约定（参考 ParagraphFormat 的 meta 输出）：
;;   restart?      布尔——需重启字段（提交时先确认再 apply）
;;   radio-group   字符串——组内互斥（toggle；如 mathjax/mathml/images -> "html-formula-export"）
;;   visible-when-key + visible-when-val   条件可见性（依赖键取此值时本字段可见）
;;   group         字符串——分组标题（组首字段上发）
;;   hint          字符串——副说明
;;   column        整数——双栏布局列号 0/1（Math / experimental Other）

;; 需重启字段的内部键集合（固定）：look and feel / gui theme / language /
;; keyboard shortcut style / magic-paste-shortcut。与 set-pretty-preference* 调用点一致。

(define preferences-qml-restart-keys
  (list "look and feel"
    "gui theme"
    "language"
    "keyboard shortcut style"
    "magic-paste-shortcut"
  ) ;list
) ;define

;; ---- General fields ----

(define preferences-qml-general-fields
  (list
    ;; look and feel 选项按平台过滤（field->descriptor 时按平台谓词裁剪 options/options-pretty）。
    (list (pref-general-look-and-feel)
      "Look and feel"
      '("default" "emacs" "gnome" "kde" "macos" "windows")
      '("Default" "Emacs" "Gnome" "KDE" "macOS" "Windows")
      #f
      'restart?
      #t
      'platform-filter
      'look-and-feel
    ) ;list
    (list (pref-general-language)
      "User interface language"
      '()
      '()
      #f
      'restart?
      #t
      'language-options
    ) ;list
    (list (pref-general-complex-actions)
      "Complex actions"
      '("menus" "popups")
      '("Through the menus" "Through popup windows")
      #f
    ) ;list
    (list (pref-general-interactive-questions)
      "Interactive questions"
      '("footer" "popup")
      '("On the footer" "In popup windows")
      #f
    ) ;list
    (list (pref-general-detailed-menus)
      "Details in menus"
      '("simple" "detailed")
      '("Simplified menus" "Detailed menus")
      #f
    ) ;list
    (list (pref-general-buffer-management)
      "Buffer management"
      '("separate" "shared")
      '("Documents in separate windows" "Multiple documents share window")
      #f
    ) ;list
    (list (pref-general-gui-theme)
      "User interface theme"
      '("liii" "liii-night")
      '("Liii" "Liii Dark")
      #f
      'restart?
      #t
    ) ;list
    (list (pref-general-completion-style)
      "Completion style"
      '("popup" "inline")
      '("Popup" "Inline")
      #f
    ) ;list
    (list (pref-general-magic-paste-shortcut)
      "Magic paste shortcut"
      '("ctrl+shift+v" "ctrl+v")
      ;; magic-paste 的 options-pretty 随平台变（macOS Cmd+ / 其它 Ctrl+）。
      (if (os-macos?) '("Cmd+Shift+V" "Cmd+V") '("Ctrl+Shift+V" "Ctrl+V"))
      #f
      'restart?
      #t
    ) ;list
    ;; keyboard shortcut style 仅 macOS（field->descriptor 按平台谓词过滤）。
    (list (pref-general-keyboard-shortcut-style)
      "Keyboard shortcut style"
      '("text" "symbol")
      '("Text" "Symbol")
      #f
      'restart?
      #t
      'platform-filter
      'macos-only
    ) ;list
  ) ;list
) ;define

;; ---- Keyboard fields ----

(define preferences-qml-keyboard-fields
  (list (list (pref-keyboard-text-spacebar)
          "Space bar in text mode"
          '("default"
            "allow multiple spaces"
            "glue multiple spaces"
            "no multiple spaces")
          '("Default"
            "Allow multiple spaces"
            "Glue multiple spaces"
            "No multiple spaces")
          #f
          'group
          "Keyboard behavior"
        ) ;list
    (list (pref-keyboard-math-spacebar)
      "Space bar in math mode"
      '("default"
        "allow spurious spaces"
        "avoid spurious spaces"
        "no spurious spaces")
      '("Default"
        "Allow spurious spaces"
        "Avoid spurious spaces"
        "No spurious spaces")
      #f
    ) ;list
    (list (pref-keyboard-automatic-quotes)
      "Automatic quotes"
      '("default" "none" "dutch" "english" "french" "german" "spanish" "swiss")
      '("Default"
        "Disabled"
        "Dutch"
        "English"
        "French"
        "German"
        "Spanish"
        "Swiss")
      #f
    ) ;list
    (list (pref-keyboard-automatic-brackets)
      "Automatic brackets"
      '("off" "mathematics" "on")
      '("Disabled" "Inside mathematics" "Enabled")
      #f
    ) ;list
    (list (pref-keyboard-cyrillic-input-method)
      "Cyrillic input method"
      '("none" "translit" "jcuken" "yawerty")
      '("None" "Translit" "Jcuken" "Yawerty")
      #f
    ) ;list
    ;; IR combos（editable：用户可键入预设外的自定义键名）。
    (list (pref-ir-left)
      "Left"
      '("pageup" "home" "up")
      '("PageUp" "Home" "Up")
      #t
      'group
      "Remote controllers with keyboard simulation"
    ) ;list
    (list (pref-ir-right)
      "Right"
      '("pagedown" "end" "down")
      '("PageDown" "End" "Down")
      #t
    ) ;list
    (list (pref-ir-up) "Up" '("home" "pageup" "up") '("Home" "PageUp" "Up") #t)
    (list (pref-ir-down)
      "Down"
      '("end" "pagedown" "down")
      '("End" "PageDown" "Down")
      #t
    ) ;list
    (list (pref-ir-center)
      "Center"
      '("S-return" "return" "space")
      '("S-Return" "Return" "Space")
      #t
    ) ;list
    (list (pref-ir-play) "Play" '("F5" "F6" "F7") '("F5" "F6" "F7") #t)
    (list (pref-ir-pause)
      "Pause"
      '("escape" "space" "F5")
      '("Escape" "Space" "F5")
      #t
    ) ;list
    (list (pref-ir-menu) "Menu" '("." "," "menu") '("." "," "Menu") #t)
  ) ;list
) ;define

;; ---- Math fields（纯 toggles，双栏） ----

(define preferences-qml-math-fields
  (list
    ;; 左栏（column 0）
    (list (pref-math-use-large-brackets)
      "Use extensible brackets"
      '()
      '()
      #f
      'group
      "Keyboard"
      'column
      0
    ) ;list
    (list (pref-math-manual-remove-superfluous-invisible)
      "Remove superfluous invisible"
      '()
      '()
      #f
      'group
      "Correction"
      'column
      0
    ) ;list
    (list (pref-math-manual-insert-missing-invisible)
      "Insert missing invisible"
      '()
      '()
      #f
      'column
      0
    ) ;list
    (list (pref-math-manual-homoglyph-correct)
      "Homoglyph correct"
      '()
      '()
      #f
      'column
      0
    ) ;list
    ;; 右栏（column 1）—— Contextual hints
    (list (pref-math-show-full-context)
      "Show full context"
      '()
      '()
      #f
      'group
      "Contextual hints"
      'column
      1
    ) ;list
    (list (pref-math-show-table-cells) "Show table cells" '() '() #f 'column 1)
    (list (pref-math-show-focus) "Show focus" '() '() #f 'column 1)
    (list (pref-math-show-only-semantic-focus)
      "Show only semantic focus"
      '()
      '()
      #f
      'hint
      "仅 semantic editing 开时可见"
      'visible-when-key
      (pref-math-semantic-editing)
      'visible-when-val
      "on"
      'column
      1
    ) ;list
    ;; 右栏 —— Semantics
    (list (pref-math-semantic-editing)
      "Semantic editing"
      '()
      '()
      #f
      'group
      "Semantics"
      'hint
      "切换会刷新相关字段可见性"
      'column
      1
    ) ;list
    (list (pref-math-semantic-selections)
      "Semantic selections"
      '()
      '()
      #f
      'hint
      "仅 semantic editing 开时可见"
      'visible-when-key
      (pref-math-semantic-editing)
      'visible-when-val
      "on"
      'column
      1
    ) ;list
  ) ;list
) ;define

;; ---- Convert / Html fields ----

(define preferences-qml-convert-html-fields
  (list
    ;; TeXmacs → Html
    (list (pref-convert-html-css)
      "Use CSS for more advanced formatting"
      '()
      '()
      #f
      'group
      "TeXmacs → Html"
    ) ;list
    ;; Export formulas as（radio 互斥组——mathjax / mathml / images 三选一）。
    (list (pref-convert-html-mathjax)
      "Export formulas as MathJax"
      '()
      '()
      #f
      'hint
      "与 MathML / images 互斥"
      'radio-group
      "html-formula-export"
    ) ;list
    (list (pref-convert-html-mathml)
      "Export formulas as MathML"
      '()
      '()
      #f
      'hint
      "与 MathJax / images 互斥"
      'radio-group
      "html-formula-export"
    ) ;list
    (list (pref-convert-html-images)
      "Export formulas as images"
      '()
      '()
      #f
      'hint
      "与 MathJax / MathML 互斥"
      'radio-group
      "html-formula-export"
    ) ;list
    (list (pref-convert-html-css-stylesheet)
      "CSS stylesheet"
      '("---"
        "web-article.css"
        "web-article-dark.css"
        "web-article-colored.css"
        "web-article-dark-colored.css")
      '("---"
        "web-article.css"
        "web-article-dark.css"
        "web-article-colored.css"
        "web-article-dark-colored.css")
      #t
    ) ;list
    ;; Html → TeXmacs
    (list (pref-convert-html-mathml-latex-annotations)
      "Try to import formulas using LaTeX annotations"
      '()
      '()
      #f
      'group
      "Html → TeXmacs"
    ) ;list
  ) ;list
) ;define

;; ---- Convert / LaTeX fields ----

(define preferences-qml-convert-latex-fields
  (list
    ;; LaTeX → TeXmacs
    (list (pref-convert-latex-fallback-on-pictures)
      "Import sophisticated objects as pictures"
      '()
      '()
      #f
      'group
      "LaTeX → TeXmacs"
    ) ;list
    ;; TeXmacs → LaTeX
    (list (pref-convert-latex-replace-style)
      "Replace TeXmacs styles with no LaTeX equivalents"
      '()
      '()
      #f
      'group
      "TeXmacs → LaTeX"
    ) ;list
    (list (pref-convert-latex-expand-macros)
      "Expand TeXmacs macros with no LaTeX equivalents"
      '()
      '()
      #f
    ) ;list
    (list (pref-convert-latex-expand-user-macros)
      "Expand user-defined macros"
      '()
      '()
      #f
    ) ;list
    (list (pref-convert-latex-indirect-bib)
      "Export bibliographies as links"
      '()
      '()
      #f
    ) ;list
    (list (pref-convert-latex-use-macros)
      "Allow for macro definitions in preamble"
      '()
      '()
      #f
    ) ;list
    (list (pref-convert-latex-encoding)
      "Character encoding"
      '("utf-8" "cork")
      '("Utf-8 with inputenc" "Cork with catcodes")
      #f
    ) ;list
    ;; Conservative conversion options——source-tracking 开关联动
    ;; （统一展示键 "latex:source-tracking" 读 OR / 写双向；由 set-field 路由）。
    (list "latex:source-tracking"
      "Keep track of source code"
      '()
      '()
      #f
      'group
      "Conservative conversion options"
    ) ;list
    (list "latex:conservative"
      "Only convert changes w.r.t. tracked version"
      '()
      '()
      #f
      'hint
      "两方向联动"
    ) ;list
    (list "latex:transparent-source-tracking"
      "Guarantee transparent source tracking"
      '()
      '()
      #f
      'hint
      "仅 source-tracking 开时可见"
      'visible-when-key
      "latex:source-tracking"
      'visible-when-val
      "on"
    ) ;list
    (list (pref-convert-latex-attach-tracking-info)
      "Store tracking information in LaTeX files"
      '()
      '()
      #f
      'hint
      "仅 source-tracking 开时可见"
      'visible-when-key
      "latex:source-tracking"
      'visible-when-val
      "on"
    ) ;list
  ) ;list
) ;define

;; ---- Convert / BibTeX fields ----

(define preferences-qml-convert-bibtex-fields
  (list
    ;; BibTeX → TeXmacs
    (list (pref-convert-bibtex-command)
      "BibTeX command"
      '("bibtex" "biber" "biblatex" "rubibtex")
      '("bibtex" "biber" "biblatex" "rubibtex")
      #t
      'group
      "BibTeX → TeXmacs"
    ) ;list
    (list (pref-convert-bibtex-import-conservative)
      "Only convert changes when re-importing"
      '()
      '()
      #f
    ) ;list
    ;; TeXmacs → BibTeX
    (list (pref-convert-bibtex-export-conservative)
      "Only convert changes w.r.t. imported version"
      '()
      '()
      #f
      'group
      "TeXmacs → BibTeX"
    ) ;list
  ) ;list
) ;define

;; ---- Convert / Verbatim fields ----

(define preferences-qml-convert-verbatim-fields
  (list
    ;; TeXmacs → Verbatim
    (list (pref-convert-verbatim-export-wrap)
      "Line wrap for lines longer than 80 characters"
      '()
      '()
      #f
      'group
      "TeXmacs → Verbatim"
    ) ;list
    (list (pref-convert-verbatim-export-encoding)
      "Character encoding"
      '("auto" "cork" "iso-8859-1" "iso-8859-2" "utf-8")
      '("Automatic" "Cork" "ISO-8859-1" "ISO-8859-2" "UTF-8")
      #f
    ) ;list
    ;; Verbatim → TeXmacs
    (list (pref-convert-verbatim-import-wrap)
      "Merge lines into paragraphs unless blank-line separated"
      '()
      '()
      #f
      'group
      "Verbatim → TeXmacs"
    ) ;list
    (list (pref-convert-verbatim-import-encoding)
      "Character encoding"
      '("utf-8" "auto" "cork" "iso-8859-1" "iso-8859-2")
      '("UTF-8" "Automatic" "Cork" "ISO-8859-1" "ISO-8859-2")
      #f
    ) ;list
  ) ;list
) ;define

;; ---- Convert / Pdf fields ----

(define preferences-qml-convert-pdf-fields
  (list
    ;; TeXmacs → Pdf / Postscript
    (list (pref-convert-pdf-expand-slides)
      "Expand beamer slides"
      '()
      '()
      #f
      'group
      "TeXmacs → Pdf / Postscript"
    ) ;list
    (list (pref-use-external-pdf-viewer) "Use external pdf viewer" '() '() #f)
    ;; pdf version number 字段仅启用原生 PDF 时可见（supports-native-pdf?）。
    (list (pref-convert-pdf-version)
      "Pdf version number"
      '("default" "1.4" "1.5" "1.6" "1.7")
      '("default" "1.4" "1.5" "1.6" "1.7")
      #f
      'platform-filter
      'native-pdf-only
    ) ;list
  ) ;list
) ;define

;; ---- Convert / Image fields ----

(define preferences-qml-convert-image-fields
  (list
    ;; TeXmacs → Image
    (list (pref-convert-image-raster-resolution)
      "Bitmap export resolution (dpi)"
      '("1200" "600" "300" "150")
      '("1200" "600" "300" "150")
      #t
      'group
      "TeXmacs → Image"
    ) ;list
    ;; 剪贴板图片格式：options 动态按 file-converter-exists? 过滤（副作用——见 pretty-format-list）。
    ;; field->descriptor 在调用时拉取（options / options-pretty 同源，保证等长同序）。
    (list (pref-convert-image-format) "Clipboard image format" '() '() #f)
  ) ;list
) ;define

;; ---- Other / Misc fields ----

(define preferences-qml-other-misc-fields
  (list (list (pref-autosave)
          "Automatically save"
          '("120" "0")
          '("On" "Off")
          #f
          'group
          "Miscellaneous preferences"
        ) ;list
    (list (pref-autobackup) "Auto backup" '("on" "off") '("On" "Off") #f)
    (list (pref-security)
      "Security"
      '("accept no scripts" "prompt on scripts" "accept all scripts")
      '("Accept no scripts" "Prompt on scripts" "Accept all scripts")
      #f
    ) ;list
    ;; scripting language 的 options 动态按 scripts-list（lazy-plugin-force 副作用）。
    ;; field->descriptor 在调用时拉取 options / options-pretty。
    (list (pref-scripting-language) "Scripting language" '() '() #f)
    (list (pref-document-update-times)
      "Document updates run"
      '("1" "2" "3")
      '("Once" "Twice" "Three times")
      #f
    ) ;list
    ;; updater 字段仅启用插件更新器时可见（use-plugin-updater?）。
    (list (pref-updater-interval)
      "Check for automatic updates"
      '("0" "24" "168" "720")
      '("Never" "Once a day" "Once a week" "Once a month")
      #f
      'platform-filter
      'updater-only
    ) ;list
  ) ;list
) ;define

;; ---- Other / Experimental fields（双栏 toggles，带平台条件过滤） ----

(define preferences-qml-other-experimental-fields
  (list
    ;; 左栏（column 0）
    (list (pref-experimental-fast-environments)
      "Fast environments"
      '()
      '()
      #f
      'group
      "Experimental features (to be used with care)"
      'column
      0
    ) ;list
    (list (pref-experimental-alpha) "Alpha transparency" '() '() #f 'column 0)
    (list (pref-experimental-new-style-page-breaking)
      "New style page breaking"
      '()
      '()
      #f
      'column
      0
    ) ;list
    (list (pref-experimental-encryption) "Encryption" '() '() #f 'column 0)
    (list (pref-experimental-use-native-menubar)
      "Use native menubar"
      '()
      '()
      #f
      'hint
      "macOS only"
      'column
      0
      'platform-filter
      'macos-only
    ) ;list
    ;; 右栏（column 1）—— Experimental 程序员 / 搜索 / 打印 等。
    (list (pref-prog-highlight-brackets)
      "Program bracket matching"
      '()
      '()
      #f
      'column
      1
    ) ;list
    (list (pref-prog-automatic-brackets)
      "Automatic program brackets"
      '()
      '()
      #f
      'column
      1
    ) ;list
    (list (pref-prog-select-brackets)
      "Program bracket selections"
      '()
      '()
      #f
      'column
      1
    ) ;list
    (list (pref-case-insensitive-match)
      "Case-insensitive search"
      '()
      '()
      #f
      'column
      1
    ) ;list
    (list (pref-gui-print-dialogue)
      "Use print dialogue"
      '()
      '()
      #f
      'hint
      "qt only"
      'column
      1
      'platform-filter
      'qt-only
    ) ;list
    (list (pref-texlive-fonts) "Use fonts in texlive" '() '() #f 'column 1)
    (list (pref-experimental-use-unified-toolbar)
      "Use unified toolbars"
      '()
      '()
      #f
      'hint
      "macOS only"
      'column
      1
      'platform-filter
      'macos-only
    ) ;list
  ) ;list
) ;define

;; ---- 取字段的 flag 尾巴 ----
;; 紧凑格式 (key label options options-pretty editable? . flags)——前 5 项固定、
;; flags 是可变尾巴（plist）。list-tail 跳过前 5 项即得 flags。若无 flags（只有 5 项）、
;; 返回 '()（安全 no-op）。

(define (field-flags field)
  (if (>= (length field) 5) (list-tail field 5) '())
) ;define

;; ---- plist -> alist（把交替的 keyword / value 对转成 (keyword . value) 对） ----
;; flags 尾巴是 plist：'(restart? #t platform-filter macos-only) -> alist。
;; 用 car/cdr 直接取、递归 cddr——不用 with（mogan 的 with 对 dotted-pair 解构不稳）。

(define (preferences-qml-plist->alist plist)
  (cond ((or (null? plist) (null? (cdr plist))) '())
        (else (cons (cons (car plist) (cadr plist))
                (preferences-qml-plist->alist (cddr plist))
              ) ;cons
        ) ;else
  ) ;cond
) ;define

;; ---- flag plist -> assoc pairs ----
;; 遍历 alist、按 symbol 名映射成 bridge 可消费的 assoc pairs（platform-filter 是
;; scheme-side 过滤用的、此处跳过——bridge 无需感知）。

(define (preferences-qml-flags->assoc flags)
  (apply append
    (map (lambda (pair)
           (let ((kw (car pair)) (val (cdr pair)))
             (cond ((== kw 'restart?) (list (cons 'restart? val)))
                   ((== kw 'radio-group) (list (cons 'radioGroup val)))
                   ((== kw 'visible-when-key) (list (cons 'visibleWhenKey val)))
                   ((== kw 'visible-when-val) (list (cons 'visibleWhenVal val)))
                   ((== kw 'group) (list (cons 'group val)))
                   ((== kw 'hint) (list (cons 'hint val)))
                   ((== kw 'column) (list (cons 'column val)))
                   (else '())
             ) ;cond
           ) ;let
         ) ;lambda
      (preferences-qml-plist->alist flags)
    ) ;map
  ) ;apply
) ;define

;; ---- 平台过滤 predicate ----
;; 按 flag 里的 'platform-filter 值过滤字段：返回 #f 表示该字段在当前平台不显示。

(define (preferences-qml-platform-shows? field)
  (let* ((flags (preferences-qml-plist->alist (field-flags field)))
         (pf (assoc 'platform-filter flags))
        ) ;
    (cond ((not pf) #t)
          ((== (cdr pf) 'look-and-feel)
           ;; look and feel 的 options 在 field->descriptor 时按平台裁剪，字段本身始终显示。
           #t
          ) ;
          ((== (cdr pf) 'macos-only) (os-macos?))
          ((== (cdr pf) 'native-pdf-only) (supports-native-pdf?))
          ((== (cdr pf) 'qt-only) (qt-gui?))
          ((== (cdr pf) 'updater-only) (use-plugin-updater?))
          (else #t)
    ) ;cond
  ) ;let*
) ;define

;; ---- 取字段的当前值（内部键 / on/off / 翻译显示串） ----

(define (preferences-qml-current-value key kind options options-pretty)
  (cond ((== kind "combo")
         (let* ((pretty (get-pretty-preference key))
                (idx (list-find-index options-pretty (lambda (p) (== p pretty))))
               ) ;
           (if idx (list-ref options idx) pretty)
         ) ;let*
        ) ;
        ((== kind "toggle") (if (get-boolean-preference key) "on" "off"))
        (else "")
  ) ;cond
) ;define

;; ---- 动态解析某字段 key 的最终 options / options-pretty ----
;; 大多字段直接透传字段定义里的静态 options；少数动态选项（language / scripting
;; language / image format）在 meta 构建时拉取一次、覆盖空 options。look and feel
;; 的 options 按平台裁剪（保持 options / options-pretty 等长同序）。
;; 返回 (final-options final-options-pretty) 二元组。

(define (preferences-qml-resolve-options key options options-pretty)
  (cond
    ;; look and feel：按平台裁剪 options（options 静态含全部平台、options-pretty 同步裁剪）。
    ((== key (pref-general-look-and-feel))
     (let* ((laf-allowed (preferences-qml-general-look-and-feel-allowed))
            (laf-pairs (list-filter (map (lambda (ik) (cons ik (preferences-qml-general-look-and-feel-pretty ik)))
                                      options
                                    ) ;map
                         (lambda (pair) (member (car pair) laf-allowed))
                       ) ;list-filter
            ) ;laf-pairs
           ) ;
       (list (map car laf-pairs) (map cdr laf-pairs))
     ) ;let*
    ) ;
    ;; language：动态按 supported-languages 拉取 options（顶层 define 里用空 '() 避开
    ;; module 加载时批量求值崩溃）。options = optionsTr = supported-languages（语言内部
    ;; 键即显示名——首字母大写化由 line 62 的 set-preference-name 在 module 加载时
    ;; 已登记进 encode 表，故 optionsTr 直接复用 supported-languages 即可）。
    ((== key (pref-general-language))
     ;; supported-languages 是变量（绑定到语言列表）、不是函数——不带括号引用。
     (list supported-languages supported-languages)
    ) ;
    ;; scripting language：动态按 scripts-list（lazy-plugin-force 副作用）拉取 options。
    ((== key (pref-scripting-language))
     (let* ((sl (cons "none" (map scripts-name (scripts-list)))))
       (list sl (cons (translate "None") (map scripts-name (scripts-list))))
     ) ;let*
    ) ;
    ;; image format：动态按 pretty-format-list（file-converter-exists? 副作用）拉取 options。
    ((== key (pref-convert-image-format))
     (let* ((fl (pretty-format-list)))
       (list fl fl)
     ) ;let*
    ) ;
    ;; 其余字段：直接透传字段定义里的静态 options / options-pretty。
    (else (list options options-pretty))
  ) ;cond
) ;define

;; ---- field -> assoc-list field-descriptor ----
;; 把紧凑格式 (key label options options-pretty editable? . flags) 转成 assoc-list
;; field-descriptor 供 bridge 消费（参考 ParagraphFormat 的 meta 输出）。用 let* + list-ref
;; 显式取各位置——mogan 的 with 对 5 元 + rest 解构行为不稳。
;;
;; kind 分流：options 非空 -> combo；options 空 + key 非空 -> toggle；key 空 -> info。

(define (preferences-qml-field->descriptor field)
  (let* ((key (list-ref field 0))
         (label (list-ref field 1))
         (options (list-ref field 2))
         (options-pretty (list-ref field 3))
         (editable? (list-ref field 4))
         (flags (field-flags field))
         (opt-pairs (preferences-qml-resolve-options key options options-pretty))
         (final-options (car opt-pairs))
         (final-options-pretty (cadr opt-pairs))
         (kind (cond ((or (nlist? final-options) (null? final-options))
                      (if (== key "") "info" "toggle")
                     ) ;
                     (else "combo")
               ) ;cond
         ) ;kind
         (base (list (cons 'kind kind) (cons 'key key) (cons 'label (translate label))))
         (value-pairs (if (== kind "combo")
                        (list (cons 'options final-options)
                          (cons 'optionsTr (map translate final-options-pretty))
                          (cons 'editable editable?)
                          (cons 'value
                            (preferences-qml-current-value key kind final-options final-options-pretty)
                          ) ;cons
                        ) ;list
                        (list (cons 'value (preferences-qml-current-value key kind '() '())))
                      ) ;if
         ) ;value-pairs
         (flag-pairs (preferences-qml-flags->assoc flags))
        ) ;
    (append base value-pairs flag-pairs)
  ) ;let*
) ;define

;; ---- tab builder：遍历字段定义列表，map 成 assoc-list field-descriptor ----

(define (preferences-qml-build-tab fields)
  (map preferences-qml-field->descriptor
    (list-filter fields preferences-qml-platform-shows?)
  ) ;map
) ;define

;; ---- meta 总入口：组装 tab 树 ----
;; 返回 list of tab 描述符。每个 tab 为 (key label fields ...)：
;;   key    —— tab 内部键（"general" / "keyboard" / "mathematics" / "convert" / "other"）
;;   label  —— 已 translate 的 tab 标题
;;   fields —— 该 tab 的字段描述符列表（由 preferences-qml-build-tab 返回）
;; Convert tab 额外携带子 tab（sub-tabs）：sub-tabs 为 list of (sub-key sub-label sub-fields)。

(tm-define (preferences-qml-meta)
  (list (list "general"
          (translate "General")
          (preferences-qml-build-tab preferences-qml-general-fields)
        ) ;list
    (list "keyboard"
      (translate "Keyboard")
      (preferences-qml-build-tab preferences-qml-keyboard-fields)
    ) ;list
    (list "mathematics"
      (translate "Mathematics")
      (preferences-qml-build-tab preferences-qml-math-fields)
    ) ;list
    (list "convert"
      (translate "Convert")
      '()
      (list-filter (list (list "html"
                           (translate "Html")
                           (preferences-qml-build-tab preferences-qml-convert-html-fields)
                         ) ;list
                     (list "latex"
                       (translate "LaTeX")
                       (preferences-qml-build-tab preferences-qml-convert-latex-fields)
                     ) ;list
                     (list "bibtex"
                       (translate "BibTeX")
                       (preferences-qml-build-tab preferences-qml-convert-bibtex-fields)
                     ) ;list
                     (list "verbatim"
                       (translate "Verbatim")
                       (preferences-qml-build-tab preferences-qml-convert-verbatim-fields)
                     ) ;list
                     (if (or (supports-native-pdf?) (supports-ghostscript?))
                       (list "pdf"
                         (translate "Pdf")
                         (preferences-qml-build-tab preferences-qml-convert-pdf-fields)
                       ) ;list
                       #f
                     ) ;if
                     (list "image"
                       (translate "Image")
                       (preferences-qml-build-tab preferences-qml-convert-image-fields)
                     ) ;list
                   ) ;list
        identity
      ) ;list-filter
    ) ;list
    (list "other"
      (translate "Other")
      (append (preferences-qml-build-tab preferences-qml-other-misc-fields)
        (preferences-qml-build-tab preferences-qml-other-experimental-fields)
      ) ;append
    ) ;list
  ) ;list
) ;tm-define

;; ---- submit：应用 diff（先确认再 apply） ----
;; changed-assoc 为 scheme assoc list：((key . val) ...)，key / val 均为字符串（val 对
;; toggle 为 "on"/"off"，对 combo 为内部键）。统一为字符串 wire 格式——bridge 把 QML 的
;; bool toggle 序列化为 "on"/"off" 串后传入。
;;
;; 返回 "applied" / "restart" / "later" / "cancel"：
;;   applied —— 无需重启字段改动，全部直接 apply。
;;   restart —— 有需重启字段改动，用户确认重启，已 apply 全部 + 存盘 + restart-TeXmacs。
;;   later   —— 有需重启字段改动，用户选「稍后」，重启字段走 silent 写值（下次启动生效）。
;;   cancel  —— 有需重启字段改动，用户选「取消」，什么都不 apply（先确认再 apply，无需回滚）。

(tm-define (preferences-qml-submit changed-assoc)
  (with changed-keys
    (map car changed-assoc)
    (with restart-changed
      (list-filter changed-keys (lambda (k) (member k preferences-qml-restart-keys)))
      (with non-restart-changed
        (list-filter changed-keys (lambda (k) (not (member k restart-changed))))
        (if (null? restart-changed)
          ;; 无需重启字段改动：全部直接 apply。
          (begin
            (for (key changed-keys)
              (preferences-qml-set-field key (cdr (assoc key changed-assoc)))
            ) ;for
            "applied"
          ) ;begin
          ;; 有需重启字段改动：「先确认再 apply」——先弹 ConfirmRestart（标题用首个改动重启字段），
          ;; 再按用户选择分别 apply / silent 写值 / 不 apply。
          (with title
            (restart-preference-title (car restart-changed))
            (with choice
              (cpp-confirm-restart title (restart-effect-message))
              (cond ((== choice "restart")
                     (for (key changed-keys)
                       (preferences-qml-set-field key (cdr (assoc key changed-assoc)))
                     ) ;for
                     (when (not (defined? 'save-all-buffers))
                       (use-modules (plugin autosave))
                     ) ;when
                     (save-all-buffers)
                     (restart-TeXmacs)
                     "restart"
                    ) ;
                    ((== choice "later")
                     ;; 非重启字段：普通 setter 实时生效；重启字段：silent 写值（下次启动生效）。
                     (for (key non-restart-changed)
                       (preferences-qml-set-field key (cdr (assoc key changed-assoc)))
                     ) ;for
                     (for (key restart-changed)
                       (preferences-qml-set-field-silent key (cdr (assoc key changed-assoc)))
                     ) ;for
                     "later"
                    ) ;
                    (else
                      ;; cancel：什么都不 apply。先确认再 apply 的好处——重启字段改动尚未 apply，
                      ;; 故无需回滚；非重启字段也未 apply（与「先确认再 apply」的语义一致）。
                      "cancel"
                    ) ;else
              ) ;cond
            ) ;with
          ) ;with
        ) ;if
      ) ;with
    ) ;with
  ) ;with
) ;tm-define

;; ---- set-field：统一 setter，按 key 路由副作用 ----
;; 普通 key：走 set-pretty-preference（combo）或 set-boolean-preference（toggle）。
;; 副作用 key：路由到专用 setter——buffer management 联动 tab bar、formula radio 互斥、
;; latex source-tracking / conservative / transparent 双向写、autosave label↔120/0 映射等。
;; val 统一为字符串（toggle 为 "on"/"off"，combo 为内部键或 pretty 显示形——按 key 的 decode 表决定）。

(define (preferences-qml-set-field key val)
  (cond
    ;; buffer management：联动 tab bar boolean 偏好 + show-icon-bar 4 副作用。
    ((== key (pref-general-buffer-management)) (on-buffer-management-changed val))
    ;; latex 统一展示键：双向写（import + export 各一偏好）。
    ((== key "latex:source-tracking") (set-latex-source-tracking (== val "on")))
    ((== key "latex:conservative") (set-latex-conservative (== val "on")))
    ((== key "latex:transparent-source-tracking")
     (set-latex-transparent-source-tracking (== val "on"))
    ) ;
    ;; 其余 key：按 toggle / combo 分流。toggle 的 val 为 "on"/"off" 字符串。
    ((== val "on") (set-boolean-preference key #t))
    ((== val "off") (set-boolean-preference key #f))
    (else (set-pretty-preference key val))
  ) ;cond
) ;define

;; silent 写值版（later 分支用）：走 set-pretty-preference-silent / silent 写 boolean 偏好，
;; 当前会话不实时切，下次启动生效。需重启字段才用 silent——非重启字段走普通 set-field。

(define (preferences-qml-set-field-silent key val)
  (cond ((== key "latex:source-tracking")
         (set-boolean-preference (pref-latex-import-source-tracking) (== val "on"))
         (set-boolean-preference (pref-latex-export-source-tracking) (== val "on"))
         (save-preferences)
        ) ;
        ((== key "latex:conservative")
         (set-boolean-preference (pref-latex-import-conservative) (== val "on"))
         (set-boolean-preference (pref-latex-export-conservative) (== val "on"))
         (save-preferences)
        ) ;
        ((== key "latex:transparent-source-tracking")
         (set-boolean-preference (pref-latex-import-transparent-source-tracking)
           (== val "on")
         ) ;set-boolean-preference
         (set-boolean-preference (pref-latex-export-transparent-source-tracking)
           (== val "on")
         ) ;set-boolean-preference
         (save-preferences)
        ) ;
        ;; buffer management 走 silent：仅写偏好，不做 show-icon-bar 副作用（下次启动自然生效）。
        ((== key (pref-general-buffer-management))
         (set-pretty-preference-silent (pref-general-buffer-management) val)
        ) ;
        ;; 普通 key：用 set-pretty-preference-silent（带 decode 表路由）。
        ((== val "on") (set-boolean-preference key #t) (save-preferences))
        ((== val "off") (set-boolean-preference key #f) (save-preferences))
        (else (set-pretty-preference-silent key val))
  ) ;cond
) ;define

;; look and feel 平台允许的内部键列表（用于过滤 options）。

(define (preferences-qml-general-look-and-feel-allowed)
  (cond ((os-win32?) '("default" "emacs" "windows"))
        ((os-macos?) '("default" "emacs" "macos"))
        (else '("default" "emacs" "gnome" "kde"))
  ) ;cond
) ;define

;; look and feel 内部键 -> pretty 显示名（直接编码 define-preference-names 的 decode 表；
;; 这里硬编码是为了 meta 构建时按平台过滤后仍能给出等长同序的 options/options-pretty）。

(define (preferences-qml-general-look-and-feel-pretty ik)
  (cond ((== ik "default") (translate "Default"))
        ((== ik "emacs") (translate "Emacs"))
        ((== ik "gnome") (translate "Gnome"))
        ((== ik "kde") (translate "KDE"))
        ((== ik "macos") (translate "macOS"))
        ((== ik "windows") (translate "Windows"))
        (else ik)
  ) ;cond
) ;define
