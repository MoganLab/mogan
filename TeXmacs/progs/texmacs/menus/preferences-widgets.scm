
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

(define (on-buffer-management-changed val)
  ;; val 由 preferences-qml-set-field 传入，是 internal key（"shared"/"separate"）。
  ;; 保留 pretty 显示串（"Multiple documents share window"）经 decode 表反查的兜底，
  ;; 防御未来接入 pretty 串的调用方。统一按 internal key 判断 shared。
  (let* ((internal (if (== val "shared")
                     val
                     (or (ahash-ref preference-decode-table (cons "buffer management" val)) val)
                   ) ;if
         ) ;internal
         (can-use-tabbar? (== internal "shared"))
        ) ;
    (set-boolean-preference "tab bar" can-use-tabbar?)
    (show-icon-bar 4 can-use-tabbar?)
    (set-preference "buffer management" internal)
  ) ;let*
) ;define

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
;; Convert 各子 tab 的编解码表 + 双向偏好读写 helper（LaTeX / BibTeX 源跟踪等）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

;; Pdf ----------
(define-preference-names "texmacs->pdf:version"
 ("Default" "default")
 ("1.4" "1.4")
 ("1.5" "1.5")
 ("1.6" "1.6")
 ("1.7" "1.7")
) ;define-preference-names

;; Images ----------

;; 返回 (internal-list pretty-list)，并顺带登记 texmacs->image:format 的 decode 表。
;; internal/pretty 等长同序，供 QML facade 的 options/optionsTr 对齐 combo 契约。

(define (image-format-list-pair)
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
    (apply map list valid-image-format-list)
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Other tab 的 helper + 编解码表（autosave / security / updater / scripting）
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
                   ;; enabled-when：字段始终显示，但仅当某 key 等于 val 时可勾（否则锁定灰显）。
                   ;; 用于 latex transparent / Store tracking、Math semantic：依赖键开才解锁。
                   ((== kw 'enabled-when-key) (list (cons 'enabledWhenKey val)))
                   ((== kw 'enabled-when-val) (list (cons 'enabledWhenVal val)))
                   ;; group / hint 文案：.scm 源码字面量是 UTF-8 字节（reader 不转 Cork），
                   ;; 先 utf8->cork 归一化，再 translate 查翻译表。否则含非 ASCII 的文案
                   ;; （如 "TeXmacs → Html" 的 → 箭头）被当 Cork 字节二次解码 → 乱码。
                   ;; bridge cork_to_utf8 再把 Cork 还原成 UTF-8 给 QML。
                   ((== kw 'group) (list (cons 'group (translate (utf8->cork val)))))
                   ;; group-span：该 group 标题横跨整行（统领下方左右两列），如 IR 的
                   ;; "Remote controllers with keyboard simulation"。未标的 group 在列内各自渲染。
                   ((== kw 'group-span) (list (cons 'groupSpan val)))
                   ((== kw 'hint) (list (cons 'hint (translate (utf8->cork val)))))
                   ((== kw 'column) (list (cons 'column val)))
                   ((== kw 'layout) (list (cons 'layout val)))
                   ;; action-button：combo 旁的行内按钮。val = action-name（symbol）。
                   ;; buttonLabel 由 preferences-qml-action-button-label 按 action 取（仿原
                   ;; tm-widget：先 use-modules 兜底加载插件，再调其 label 函数；未注入则空串）。
                   ;; buttonAction 透传 action 名，QML 点击经 bridge callAction -> facade 路由。
                   ((== kw 'action-button)
                    (list (cons 'buttonAction val)
                      (cons 'buttonLabel
                        (translate (utf8->cork (preferences-qml-action-button-label val)))
                      ) ;cons
                    ) ;list
                   ) ;
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
        ;; latex 双向偏好用统一展示键（latex:source-tracking / conservative /
        ;; transparent-source-tracking），非真实 preference key，读用 OR/AND helper。
        ((== key "latex:source-tracking") (if (get-latex-source-tracking) "on" "off"))
        ((== key "latex:conservative") (if (get-latex-conservative) "on" "off"))
        ((== key "latex:transparent-source-tracking")
         (if (get-latex-transparent-source-tracking) "on" "off")
        ) ;
        ;; Last check info：非真实 preference，显示上次检查更新时间（updater 插件注入）。
        ((== key "updater:last-check") (last-check-string))
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
    ;; module 加载时批量求值崩溃）。options = 小写内部键；optionsTr = pretty 显示名
    ;; （首字母大写形，由 line 62 的 set-preference-name 登记进 encode 表——
    ;; get-pretty-preference "language" 返回此形，须与 optionsTr 同序等长才能在
    ;; current_value 里反查到 index）。
    ((== key (pref-general-language))
     ;; supported-languages 是变量（绑定到语言列表）、不是函数——不带括号引用。
     (list supported-languages (map upcase-first supported-languages))
    ) ;
    ;; scripting language：动态按 scripts-list（lazy-plugin-force 副作用）拉取 options。
    ((== key (pref-scripting-language))
     (let* ((sl (cons "none" (map scripts-name (scripts-list)))))
       (list sl (cons (translate "None") (map scripts-name (scripts-list))))
     ) ;let*
    ) ;
    ;; image format：动态按 image-format-list-pair（file-converter-exists? 副作用）
    ;; 拉取 internal/pretty 双列——options 用 internal 键、optionsTr 用 pretty 显示名，
    ;; 与其它 combo 字段对齐（get-pretty-preference 返回 pretty、经 current_value 反查 index）。
    ((== key (pref-convert-image-format))
     (let* ((pair (image-format-list-pair)))
       (list (car pair) (cadr pair))
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
         (kind (cond ((list-find flags (lambda (x) (== x 'info))) "info")
                     ((or (nlist? final-options) (null? final-options))
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
;; action 函数由插件注入（(plugin autosave) 模块），仿原 tm-widget：先 use-modules 兜底
;; 加载，再调字面函数名。bridge callAction(name) 透传到 preferences-qml-call-action。

;; 取 action 按钮的显示文案。按 action 名路由（目前仅 open-auto-backup-location）。

(define (preferences-qml-action-button-label action)
  (cond ((== action 'open-auto-backup-location)
         (when (not (defined? 'auto-backup-button-label))
           (use-modules (plugin autosave))
         ) ;when
         (if (defined? 'auto-backup-button-label) (auto-backup-button-label) "")
        ) ;
        (else "")
  ) ;cond
) ;define


;; 执行 action（按钮点击）。按 name 路由，仿原 tm-widget 兜底加载模块。

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
