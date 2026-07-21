
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : pref-keys.scm
;; DESCRIPTION : preference key 常量的单一可信源。
;;               各弹窗（QML form 引擎，见 record/qml/plan.md）构造字段表、
;;               preference 读写、live setter 等使用方统一引用此处函数，
;;               消除 key 字符串散落各处的问题。
;;               注意：key 字面值必须与 tm-print.scm 等 define-preferences 注册
;;               处完全一致（key 改名会断开 notify 回调链路）。
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY whatsoever. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs pref-keys))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Page setup（文件 → 页面设置）
;; 权威定义：tm-print.scm 的 define-preferences（含 notify 回调）。
;; 用 define-public 函数而非值变量：函数跨模块绑定解析更可靠（mogan 惯例），
;; 且在调用方 quasiquote 里 ,(pref-...) 意图清晰。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (pref-page-setup-preview-command) "preview command")
(define-public (pref-page-setup-printing-command) "printing command")
(define-public (pref-page-setup-paper-type) "paper type")
(define-public (pref-page-setup-printer-dpi) "printer dpi")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Font selector（格式 → 字体）
;; 权威定义：font-new-widgets.scm 的 selector-restore / initial-font-data /
;; selector-get-changes 用这些 key 读写 init/preference。
;; :default（见 selector-restore）。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (pref-font) "font")
(define-public (pref-font-base-size) "font-base-size")
(define-public (pref-math-font) "math-font")
(define-public (pref-prog-font) "prog-font")
(define-public (pref-font-family) "font-family")
(define-public (pref-font-series) "font-series")
(define-public (pref-font-shape) "font-shape")
(define-public (pref-font-effects) "font-effects")

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preferences（编辑 → 首选项）
;; 权威定义：preferences-widgets.scm 的 define-preference-names 注册处。
;; 新增 QML facade（preferences-qml-meta 等）统一引用此处 key。
;; 注意：带 -> / : 的 key（如 "texmacs->html:mathjax"）与注册处字面值必须
;; 完全一致，notify 回调链路靠字符串匹配。
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; ---- General tab ----
(define-public (pref-general-look-and-feel) "look and feel")
(define-public (pref-general-language) "language")
(define-public (pref-general-complex-actions) "complex actions")
(define-public (pref-general-interactive-questions) "interactive questions")
(define-public (pref-general-detailed-menus) "detailed menus")
(define-public (pref-general-buffer-management) "buffer management")
(define-public (pref-general-gui-theme) "gui theme")
(define-public (pref-general-completion-style) "completion style")
(define-public (pref-general-magic-paste-shortcut) "magic-paste-shortcut")
(define-public (pref-general-keyboard-shortcut-style) "keyboard shortcut style")

;; ---- Keyboard tab ----
(define-public (pref-keyboard-text-spacebar) "text spacebar")
(define-public (pref-keyboard-math-spacebar) "math spacebar")
(define-public (pref-keyboard-automatic-quotes) "automatic quotes")
(define-public (pref-keyboard-automatic-brackets) "automatic brackets")
(define-public (pref-keyboard-cyrillic-input-method) "cyrillic input method")
;; Remote controllers (IR combos，editable)
(define-public (pref-ir-left) "ir-left")
(define-public (pref-ir-right) "ir-right")
(define-public (pref-ir-up) "ir-up")
(define-public (pref-ir-down) "ir-down")
(define-public (pref-ir-center) "ir-center")
(define-public (pref-ir-play) "ir-play")
(define-public (pref-ir-pause) "ir-pause")
(define-public (pref-ir-menu) "ir-menu")

;; ---- Mathematics tab ----
(define-public (pref-math-use-large-brackets) "use large brackets")
(define-public (pref-math-show-full-context) "show full context")
(define-public (pref-math-show-table-cells) "show table cells")
(define-public (pref-math-show-focus) "show focus")
(define-public (pref-math-show-only-semantic-focus) "show only semantic focus")
(define-public (pref-math-semantic-editing) "semantic editing")
(define-public (pref-math-semantic-selections) "semantic selections")
(define-public (pref-math-manual-remove-superfluous-invisible)
  "manual remove superfluous invisible"
) ;define-public
(define-public (pref-math-manual-insert-missing-invisible)
  "manual insert missing invisible"
) ;define-public
(define-public (pref-math-manual-homoglyph-correct) "manual homoglyph correct")

;; ---- Convert / Html tab ----
(define-public (pref-convert-html-css) "texmacs->html:css")
(define-public (pref-convert-html-mathjax) "texmacs->html:mathjax")
(define-public (pref-convert-html-mathml) "texmacs->html:mathml")
(define-public (pref-convert-html-images) "texmacs->html:images")
(define-public (pref-convert-html-css-stylesheet)
  "texmacs->html:css-stylesheet"
) ;define-public
(define-public (pref-convert-html-mathml-latex-annotations)
  "mathml->texmacs:latex-annotations"
) ;define-public

;; ---- Convert / LaTeX tab ----
(define-public (pref-convert-latex-fallback-on-pictures)
  "latex->texmacs:fallback-on-pictures"
) ;define-public
(define-public (pref-convert-latex-replace-style)
  "texmacs->latex:replace-style"
) ;define-public
(define-public (pref-convert-latex-expand-macros)
  "texmacs->latex:expand-macros"
) ;define-public
(define-public (pref-convert-latex-expand-user-macros)
  "texmacs->latex:expand-user-macros"
) ;define-public
(define-public (pref-convert-latex-indirect-bib) "texmacs->latex:indirect-bib")
(define-public (pref-convert-latex-use-macros) "texmacs->latex:use-macros")
(define-public (pref-convert-latex-encoding) "texmacs->latex:encoding")
(define-public (pref-convert-latex-attach-tracking-info)
  "texmacs->latex:attach-tracking-info"
) ;define-public
;; source-tracking / conservative / transparent-source-tracking 是 facade 统一展示键
;; （latex 双向：import + export 各一偏好；meta 读用 OR / AND 合并，set 双向写）。
;; 以下为 import / export 各向的真实存储 key；统一键在 preferences-widgets.scm 内部。
(define-public (pref-latex-import-source-tracking)
  "latex->texmacs:source-tracking"
) ;define-public
(define-public (pref-latex-export-source-tracking)
  "texmacs->latex:source-tracking"
) ;define-public
(define-public (pref-latex-import-conservative) "latex->texmacs:conservative")
(define-public (pref-latex-export-conservative) "texmacs->latex:conservative")
(define-public (pref-latex-import-transparent-source-tracking)
  "latex->texmacs:transparent-source-tracking"
) ;define-public
(define-public (pref-latex-export-transparent-source-tracking)
  "texmacs->latex:transparent-source-tracking"
) ;define-public

;; ---- Convert / BibTeX tab ----
(define-public (pref-convert-bibtex-command) "bibtex command")
(define-public (pref-convert-bibtex-import-conservative)
  "bibtex->texmacs:conservative"
) ;define-public
(define-public (pref-convert-bibtex-export-conservative)
  "texmacs->bibtex:conservative"
) ;define-public

;; ---- Convert / Verbatim tab ----
(define-public (pref-convert-verbatim-export-encoding)
  "texmacs->verbatim:encoding"
) ;define-public
(define-public (pref-convert-verbatim-export-wrap) "texmacs->verbatim:wrap")
(define-public (pref-convert-verbatim-import-encoding)
  "verbatim->texmacs:encoding"
) ;define-public
(define-public (pref-convert-verbatim-import-wrap) "verbatim->texmacs:wrap")

;; ---- Convert / Pdf tab ----
(define-public (pref-convert-pdf-expand-slides) "texmacs->pdf:expand slides")
(define-public (pref-convert-pdf-version) "texmacs->pdf:version")
(define-public (pref-use-external-pdf-viewer) "use external pdf viewer")

;; ---- Convert / Image tab ----
(define-public (pref-convert-image-raster-resolution)
  "texmacs->image:raster-resolution"
) ;define-public
(define-public (pref-convert-image-format) "texmacs->image:format")

;; ---- Other / Misc tab ----
(define-public (pref-autosave) "autosave")
(define-public (pref-autobackup) "autobackup")
(define-public (pref-security) "security")
(define-public (pref-scripting-language) "scripting language")
(define-public (pref-document-update-times) "document update times")
(define-public (pref-updater-interval) "updater:interval")

;; ---- Other / Experimental toggles ----
(define-public (pref-experimental-fast-environments) "fast environments")
(define-public (pref-experimental-alpha) "experimental alpha")
(define-public (pref-experimental-new-style-page-breaking)
  "new style page breaking"
) ;define-public
(define-public (pref-experimental-encryption) "experimental encryption")
(define-public (pref-experimental-use-native-menubar) "use native menubar")
(define-public (pref-experimental-use-unified-toolbar) "use unified toolbar")
(define-public (pref-prog-highlight-brackets) "prog:highlight brackets")
(define-public (pref-prog-automatic-brackets) "prog:automatic brackets")
(define-public (pref-prog-select-brackets) "prog:select brackets")
(define-public (pref-case-insensitive-match) "case-insensitive-match")
(define-public (pref-gui-print-dialogue) "gui:print dialogue")
(define-public (pref-texlive-fonts) "texlive:fonts")
