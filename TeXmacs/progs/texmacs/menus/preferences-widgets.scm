
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
    (texmacs menus preferences-tools)
    (language locale)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 通用包装：需重启字段的三态确认 + buffer management 副作用
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 偏好内部键 ↔ 显示文案的编解码表（General tab 用）
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
 ("simple" "Simplified menus")
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 编解码表（Keyboard tab 用）
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Convert 各子 tab 的编解码表（LaTeX / BibTeX 双向偏好 helper、image format
;; helper 见 preferences-tools.scm）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; LaTeX ----------

(define-preference-names "texmacs->latex:encoding"
 ("cork" "Cork with catcodes")
 ("utf-8" "Utf-8 with inputenc")
) ;define-preference-names

;; Verbatim ----------

(define-preference-names "texmacs->verbatim:encoding"
 ("auto" "Automatic")
 ("cork" "Cork")
 ("iso-8859-1" "ISO-8859-1")
 ("iso-8859-2" "ISO-8859-2")
 ("utf-8" "UTF-8")
) ;define-preference-names

(define-preference-names "verbatim->texmacs:encoding"
 ("utf-8" "UTF-8")
 ("auto" "Automatic")
 ("cork" "Cork")
 ("iso-8859-1" "ISO-8859-1")
 ("iso-8859-2" "ISO-8859-2")
) ;define-preference-names

;; Pdf ----------

(define-preference-names "texmacs->pdf:version"
 ("default" "default")
 ("1.4" "1.4")
 ("1.5" "1.5")
 ("1.6" "1.6")
 ("1.7" "1.7")
) ;define-preference-names

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Other tab 的编解码表（autosave / security / updater / scripting）
;; （updater last-check 等 helper 见 preferences-tools.scm）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

(tm-define (open-preferences) (:interactive #t) (cpp-preferences-dialog))

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
;; flags        —— 可选 plist：restart? / radio-group / enabled-when-key + enabled-when-val /
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
;;   enabled-when-key + enabled-when-val   条件锁定（依赖键取此值时本字段可勾，否则锁定灰显）
;;   group         字符串——分组标题（组首字段上发）
;;   hint          字符串——副说明
;;   column        整数——双栏布局列号 0/1（Math / experimental Other）

;; 需重启字段的内部键集合（固定）：look and feel / gui theme / language /
;; keyboard shortcut style。与 set-pretty-preference* 调用点一致。
;; 注：magic-paste-shortcut 自 [0853] 起改为 set-pretty-preference（无需重启），故不在此列。

(define preferences-qml-restart-keys
  (list "look and feel" "gui theme" "language" "keyboard shortcut style")
) ;define

;; ---- hint 文案（英文 key，供 translate 查翻译表） ----
;; 集中管理避免散落重复；facade preferences-qml-flags->assoc 对 hint 过 translate。

(define (hint-semantic-editing-only)
  "Enabled only when semantic editing is on"
) ;define

(define (hint-source-tracking-only)
  "Enabled only when source tracking is on"
) ;define

(define (hint-toggling-refreshes)
  "Toggling enables related fields"
) ;define

(define (hint-mutex-mathml-images)
  "Mutually exclusive with MathML / images"
) ;define

(define (hint-mutex-mathjax-images)
  "Mutually exclusive with MathJax / images"
) ;define

(define (hint-mutex-mathjax-mathml)
  "Mutually exclusive with MathJax / MathML"
) ;define

(define (hint-linked-both-directions)
  "Linked in both directions"
) ;define

(define (hint-macos-only)
  "macOS only"
) ;define

(define (hint-qt-only)
  "qt only"
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
    ;; IR combos（editable：用户可双击进入编辑态键入预设外的自定义键名，或清空）。
    ;; 左右两列双栏（layout 'two-col）：Left/Right/Up/Down 左列（column 0），
    ;; Center/Play/Pause/Menu 右列（column 1）。
    (list (pref-ir-left)
      "Left"
      '("pageup" "home" "up")
      '("PageUp" "Home" "Up")
      #t
      'group
      "Remote controllers with keyboard simulation"
      'group-span
      #t
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-ir-right)
      "Right"
      '("pagedown" "end" "down")
      '("PageDown" "End" "Down")
      #t
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-ir-up)
      "Up"
      '("home" "pageup" "up")
      '("Home" "PageUp" "Up")
      #t
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-ir-down)
      "Down"
      '("end" "pagedown" "down")
      '("End" "PageDown" "Down")
      #t
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-ir-center)
      "Center"
      '("S-return" "return" "space")
      '("S-Return" "Return" "Space")
      #t
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-ir-play)
      "Play"
      '("F5" "F6" "F7")
      '("F5" "F6" "F7")
      #t
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-ir-pause)
      "Pause"
      '("escape" "space" "F5")
      '("Escape" "Space" "F5")
      #t
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-ir-menu)
      "Menu"
      '("." "," "menu")
      '("." "," "Menu")
      #t
      'layout
      'two-col
      'column
      1
    ) ;list
  ) ;list
) ;define

;; ---- Math fields（纯 toggles，单栏；分组顺序对齐原 tm-widget：
;;      Keyboard -> Contextual hints -> Semantics -> Correction） ----

(define preferences-qml-math-fields
  (list
    ;; Keyboard
    (list (pref-math-use-large-brackets)
      "Use extensible brackets"
      '()
      '()
      #f
      'group
      "Keyboard"
    ) ;list
    ;; Contextual hints
    (list (pref-math-show-full-context)
      "Show full context"
      '()
      '()
      #f
      'group
      "Contextual hints"
    ) ;list
    (list (pref-math-show-table-cells) "Show table cells" '() '() #f)
    (list (pref-math-show-focus) "Show focus" '() '() #f)
    (list (pref-math-show-only-semantic-focus)
      "Show only semantic focus"
      '()
      '()
      #f
      'hint
      (hint-semantic-editing-only)
      'enabled-when-key
      (pref-math-semantic-editing)
      'enabled-when-val
      "on"
    ) ;list
    ;; Semantics
    (list (pref-math-semantic-editing)
      "Semantic editing"
      '()
      '()
      #f
      'group
      "Semantics"
      'hint
      (hint-toggling-refreshes)
    ) ;list
    (list (pref-math-semantic-selections)
      "Semantic selections"
      '()
      '()
      #f
      'hint
      (hint-semantic-editing-only)
      'enabled-when-key
      (pref-math-semantic-editing)
      'enabled-when-val
      "on"
    ) ;list
    (list (pref-math-semantic-correctness) "Semantic correctness" '() '() #f)
    ;; Correction
    (list (pref-math-manual-remove-superfluous-invisible)
      "Remove superfluous invisible"
      '()
      '()
      #f
      'group
      "Correction"
    ) ;list
    (list (pref-math-manual-insert-missing-invisible)
      "Insert missing invisible"
      '()
      '()
      #f
    ) ;list
    (list (pref-math-manual-homoglyph-correct) "Homoglyph correct" '() '() #f)
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
      (hint-mutex-mathml-images)
      'radio-group
      "html-formula-export"
    ) ;list
    (list (pref-convert-html-mathml)
      "Export formulas as MathML"
      '()
      '()
      #f
      'hint
      (hint-mutex-mathjax-images)
      'radio-group
      "html-formula-export"
    ) ;list
    (list (pref-convert-html-images)
      "Export formulas as images"
      '()
      '()
      #f
      'hint
      (hint-mutex-mathjax-mathml)
      'radio-group
      "html-formula-export"
    ) ;list
    (list (pref-convert-html-css-stylesheet)
      "CSS stylesheet"
      '("---"
        "https://www.texmacs.org/css/web-article.css"
        "https://www.texmacs.org/css/web-article-dark.css"
        "https://www.texmacs.org/css/web-article-colored.css"
        "https://www.texmacs.org/css/web-article-dark-colored.css")
      '("---"
        "https://www.texmacs.org/css/web-article.css"
        "https://www.texmacs.org/css/web-article-dark.css"
        "https://www.texmacs.org/css/web-article-colored.css"
        "https://www.texmacs.org/css/web-article-dark-colored.css")
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
      "Only convert changes with respect to tracked version"
      '()
      '()
      #f
      'hint
      (hint-linked-both-directions)
    ) ;list
    (list "latex:transparent-source-tracking"
      "Guarantee transparent source tracking"
      '()
      '()
      #f
      'hint
      (hint-source-tracking-only)
      'enabled-when-key
      "latex:source-tracking"
      'enabled-when-val
      "on"
    ) ;list
    (list (pref-convert-latex-attach-tracking-info)
      "Store tracking information in LaTeX files"
      '()
      '()
      #f
      'hint
      (hint-source-tracking-only)
      'enabled-when-key
      "latex:source-tracking"
      'enabled-when-val
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
      "Only convert changes with respect to imported version"
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

;; ---- Convert / Mogan Scheme fields ----

(define preferences-qml-convert-mogan-scheme-fields
  (list (list (pref-convert-mogan-scheme-formatted)
          "Use the Formatted Mogan Scheme"
          '()
          '()
          #f
          'group
          "TeXmacs → Mogan Scheme"
        ) ;list
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
    (list (pref-autobackup)
      "Auto backup"
      '("on" "off")
      '("On" "Off")
      #f
      'action-button
      'open-auto-backup-location
    ) ;list
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
    ;; Last check：info 只读行，显示上次检查更新时间（updater 插件启用时）。
    ;; 'info flag 显式标 info kind（key 非空但无 setter、不入 diff）。
    (list "updater:last-check"
      "Last check"
      '()
      '()
      #f
      'info
      #t
      'platform-filter
      'updater-only
    ) ;list
  ) ;list
) ;define

;; ---- Other / Experimental fields（双栏 toggles，带平台条件过滤） ----

(define preferences-qml-other-experimental-fields
  (list
    ;; 双栏布局（layout 'two-col）：左栏 column 0、右栏 column 1。
    (list (pref-experimental-fast-environments)
      "Fast environments"
      '()
      '()
      #f
      'group
      "Experimental features (to be used with care)"
      'group-span
      #t
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-experimental-alpha)
      "Alpha transparency"
      '()
      '()
      #f
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-experimental-new-style-page-breaking)
      "New style page breaking"
      '()
      '()
      #f
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-experimental-encryption)
      "Encryption"
      '()
      '()
      #f
      'layout
      'two-col
      'column
      0
    ) ;list
    (list (pref-experimental-use-native-menubar)
      "Use native menubar"
      '()
      '()
      #f
      'hint
      (hint-macos-only)
      'layout
      'two-col
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
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-prog-automatic-brackets)
      "Automatic program brackets"
      '()
      '()
      #f
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-prog-select-brackets)
      "Program bracket selections"
      '()
      '()
      #f
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-case-insensitive-match)
      "Case-insensitive search"
      '()
      '()
      #f
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-gui-print-dialogue)
      "Use print dialogue"
      '()
      '()
      #f
      'hint
      (hint-qt-only)
      'layout
      'two-col
      'column
      1
      'platform-filter
      'qt-only
    ) ;list
    (list (pref-texlive-fonts)
      "Use fonts in texlive"
      '()
      '()
      #f
      'layout
      'two-col
      'column
      1
    ) ;list
    (list (pref-experimental-use-unified-toolbar)
      "Use unified toolbars"
      '()
      '()
      #f
      'hint
      (hint-macos-only)
      'layout
      'two-col
      'column
      1
      'platform-filter
      'macos-only
    ) ;list
  ) ;list
) ;define

;; 字段格式解析 / flag->assoc / 平台过滤 / current-value / resolve-options /
;; field->descriptor / build-tab 等 QML facade 纯函数族见 preferences-tools.scm。

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
                     (list "mogan-scheme"
                       (translate "Mogan Scheme")
                       (preferences-qml-build-tab preferences-qml-convert-mogan-scheme-fields)
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
;;   restart —— 有需重启字段改动，用户确认重启：非重启键实时 apply、重启键 silent 写值
;;              （实时切 language 等会触发 Qt 输入法重建 SIGSEGV，故重启键一律 silent，
;;              重启后加载新值）+ 存盘 + restart-TeXmacs。
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
                     ;; 非重启键实时 apply；重启键走 silent（不实时切）——实时切 language
                     ;; 等会立即触发 Qt 输入法/菜单重建，在 restart-TeXmacs 真正执行前
                     ;; SIGSEGV。既然紧接着重启，silent 写值后重启加载新值，效果等价。
                     (for (key non-restart-changed)
                       (preferences-qml-set-field key (cdr (assoc key changed-assoc)))
                     ) ;for
                     (for (key restart-changed)
                       (preferences-qml-set-field-silent key (cdr (assoc key changed-assoc)))
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

;; ---- action 按钮：combo 旁的行内按钮（如 Auto backup 打开备份目录） ----
;; action 函数由插件注入（(plugin autosave) 模块），先 use-modules 兜底加载、再调
;; 字面函数名。label 取值（preferences-qml-action-button-label）见 preferences-tools.scm；
;; bridge callAction(name) 透传到本模块的 preferences-qml-call-action 执行。

(tm-define (preferences-qml-call-action name)
  (cond ((== name "open-auto-backup-location")
         (when (not (defined? 'open-auto-backup-location))
           (use-modules (plugin autosave))
         ) ;when
         (when (defined? 'open-auto-backup-location)
           (open-auto-backup-location)
         ) ;when
        ) ;
  ) ;cond
) ;tm-define
